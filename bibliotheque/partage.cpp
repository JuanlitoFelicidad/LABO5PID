#include "Clock.h"
#include "Keyboard.h"
#include "Screen.h"
#include "SharedValue.h"
#include "Thread.h"

class WriterTask : public Thread {
public:
    WriterTask(const char* nom, SharedValue* valeur)
        : Thread(nom), valeur(valeur) {
    }

protected:
    void task();

private:
    SharedValue* valeur;
    Clock horloge;
};

void WriterTask::task() {
    const int periodeMs = 100;
    const float pas = 0.5f;
    float somme = 0.0f;

    while (enMarche) {
        somme = somme + pas;
        valeur->write(somme);
        horloge.sleepUntilNextTic(periodeMs);
    }
}

int main() {
    const int attenteLectureMs = 500;   // le principal lit deux fois par seconde

    Screen ecran;
    Keyboard clavier;
    SharedValue valeur;
    WriterTask ecrivain("partage-ecrit", &valeur);

    ecran.printAt(1, 1, "Partage d'une valeur. q : quitter");
    ecran.printAt(1, 3, "valeur :");
    ecrivain.start();

    bool fini = false;
    while (!fini) {
        if (clavier.kbhit(attenteLectureMs)) {
            if (clavier.getch() == 'q') {
                fini = true;
            }
        }
        ecran.printAt(10, 3, (double)valeur.read());
    }

    ecrivain.stop();
    ecrivain.join();
    ecran.printAt(1, 5, "Fini.");
    return 0;
}
