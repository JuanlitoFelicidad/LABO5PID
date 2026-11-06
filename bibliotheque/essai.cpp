// essai.cpp : le programme d'essai de la bibliothèque. Fourni complet : il
// compile dès le départ, et il se comporte correctement quand les TODO 1 à 3
// sont remplis. Deux tâches périodiques affichent chacune, sur leur ligne, leur
// compteur de tours, l'heure et la période qu'elles mesurent, pendant que le
// programme principal surveille le clavier.
#include <cstdio>

#include "Clock.h"
#include "Keyboard.h"
#include "Screen.h"
#include "Thread.h"

class BlinkTask : public Thread {
public:
    BlinkTask(const char* nom, Screen* ecran, int ligne, int periodeMs)
        : Thread(nom), ecran(ecran), ligne(ligne), periodeMs(periodeMs) {
    }

protected:
    void task();

private:
    Screen* ecran;
    int ligne;
    int periodeMs;
    Clock horloge;
    long tours = 0;
};

void BlinkTask::task() {
    const char symboles[] = "|/-\\";
    char curseur[2] = {' ', '\0'};
    char texte[24];

    horloge.startMeasureUs();
    while (enMarche) {
        curseur[0] = symboles[tours % 4];
        ecran->printAt(3, ligne, curseur);
        ecran->printAt(6, ligne, tours);
        ecran->printAt(14, ligne, horloge.now());
        tours = tours + 1;
        horloge.sleepUntilNextTic(periodeMs);

        // la période mesurée : le temps écoulé depuis le réveil précédent,
        // arrondi à la milliseconde. Elle doit rester égale à periodeMs.
        long periodeMesureeMs = (horloge.stopMeasureUs() + 500L) / 1000L;
        horloge.startMeasureUs();
        snprintf(texte, sizeof(texte), "%5ld ms", periodeMesureeMs);
        ecran->printAt(24, ligne, texte);
    }
}

int main() {
    const int periodeRapideMs = 200;
    const int periodeLenteMs = 300;

    Screen ecran;
    Keyboard clavier;
    BlinkTask rapide("essai-rapide", &ecran, 3, periodeRapideMs);
    BlinkTask lente("essai-lente", &ecran, 4, periodeLenteMs);

    ecran.printAt(1, 1, "Essai de la bibliothèque. q : quitter");
    ecran.printAt(6, 2, "tours   heure     période mesurée");
    rapide.start();
    lente.start();

    bool fini = false;
    while (!fini) {
        if (clavier.kbhit(100)) {
            if (clavier.getch() == 'q') {
                fini = true;
            }
        }
    }

    rapide.stop();
    lente.stop();
    rapide.join();
    lente.join();

    ecran.printAt(1, 6, "Fini.");
    return 0;
}
