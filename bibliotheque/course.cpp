#include "Keyboard.h"
#include "Screen.h"
#include "SharedTable.h"
#include "Thread.h"

class FillTask : public Thread {
public:
    FillTask(const char* nom, SharedTable* table, unsigned char premiereValeur)
        : Thread(nom), table(table), valeur(premiereValeur) {
    }

    long rafales = 0;   // lu par le principal, pour l'affichage seulement

protected:
    void task() {
        while (enMarche) {
            table->fill(valeur);
            valeur = valeur + 1;
            rafales = rafales + 1;
        }
    }

private:
    SharedTable* table;
    unsigned char valeur;
};

class CheckTask : public Thread {
public:
    CheckTask(const char* nom, SharedTable* table)
        : Thread(nom), table(table) {
    }

    long verifications = 0;   // lus par le principal, pour l'affichage seulement
    long sommesFausses = 0;

protected:
    void task() {
        while (enMarche) {
            if (!table->verify()) {
                sommesFausses = sommesFausses + 1;
            }
            verifications = verifications + 1;
        }
    }

private:
    SharedTable* table;
};

int main() {
    Screen ecran;
    Keyboard clavier;
    bool protectionActive = false;   // on commence sans protection; la touche c l'inverse
    SharedTable table(protectionActive);
    FillTask remplisseuseA("course-ecrit-a", &table, 0);
    FillTask remplisseuseB("course-ecrit-b", &table, 128);
    CheckTask verificatriceA("course-verif-a", &table);
    CheckTask verificatriceB("course-verif-b", &table);

    ecran.printAt(1, 1, "La race condition comptee. c : protection, q : quitter");
    ecran.printAt(1, 2, protectionActive ? "protection : ACTIVE " : "protection : RETIREE");
    ecran.printAt(1, 4, "rafales ecrites :");
    ecran.printAt(1, 5, "verifications   :");
    ecran.printAt(1, 6, "sommes fausses  :");

    remplisseuseA.start();
    remplisseuseB.start();
    verificatriceA.start();
    verificatriceB.start();

    bool fini = false;
    while (!fini) {
        if (clavier.kbhit(500)) {
            char touche = clavier.getch();
            if (touche == 'q') {
                fini = true;
            } else if (touche == 'c') {
                // personne ne touche au tableau pendant qu'on change sa protection
                remplisseuseA.stop();
                remplisseuseB.stop();
                verificatriceA.stop();
                verificatriceB.stop();
                remplisseuseA.join();
                remplisseuseB.join();
                verificatriceA.join();
                verificatriceB.join();

                protectionActive = !protectionActive;
                table.setProtection(protectionActive);
                ecran.printAt(1, 2, protectionActive ? "protection : ACTIVE " : "protection : RETIREE");

                // les compteurs repartent de zéro pour la nouvelle protection
                remplisseuseA.rafales = 0;
                remplisseuseB.rafales = 0;
                verificatriceA.verifications = 0;
                verificatriceB.verifications = 0;
                verificatriceA.sommesFausses = 0;
                verificatriceB.sommesFausses = 0;

                remplisseuseA.start();
                remplisseuseB.start();
                verificatriceA.start();
                verificatriceB.start();
            }
        }
        ecran.printAt(19, 4, remplisseuseA.rafales + remplisseuseB.rafales);
        ecran.printAt(19, 5, verificatriceA.verifications + verificatriceB.verifications);
        ecran.printAt(19, 6, verificatriceA.sommesFausses + verificatriceB.sommesFausses);
    }

    remplisseuseA.stop();
    remplisseuseB.stop();
    verificatriceA.stop();
    verificatriceB.stop();
    remplisseuseA.join();
    remplisseuseB.join();
    verificatriceA.join();
    verificatriceB.join();

    ecran.printAt(1, 8, "Fini.");
    return 0;
}
