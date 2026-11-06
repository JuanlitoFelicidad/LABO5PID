#include <unistd.h>

#include "Clock.h"
#include "Keyboard.h"
#include "Screen.h"
#include "Thread.h"

// La configuration du jour : changez ces constantes, recompilez, relancez.
// 1  : SCHED_FIFO, {93, 92, 91, 90}, cœurs {0, 0, 0, 0}
// 2  : SCHED_FIFO, {90, 90, 90, 90}, cœurs {0, 0, 0, 0}
// 2b : SCHED_FIFO, {90, 90, 90, 90}, cœurs {0, 1, 2, 3}
// 3  : SCHED_RR,   {90, 90, 90, 90}, cœurs {0, 0, 0, 0}
// 4  : SCHED_RR,   {90, 90, 90, 91}, cœurs {0, 0, 0, 0}
// 5  : SCHED_OTHER, {0, 0, 0, 0},    cœurs {-1, -1, -1, -1}
const int politiqueDesTaches = SCHED_OTHER;
const int prioritesDesTaches[4] = {0, 0, 0, 0};
const int coeursDesTaches[4] = {-1, -1, -1, -1};

class WorkTask : public Thread {
public:
    WorkTask(const char* nom, Screen* ecran, int ligne, int politique, int priorite, int coeur)
        : Thread(nom, politique, priorite, coeur), ecran(ecran), ligne(ligne) {
    }

protected:
    void task();

private:
    void charge();

    Screen* ecran;
    int ligne;
    Clock horloge;
};

void WorkTask::charge() {
    const long toursExternes = 70000;   // environ 5 s par rafale sur un Pi 3; à ajuster
    const long toursInternes = 10000;

    volatile long compteur = 0;
    for (long i = 0; i < toursExternes; i = i + 1) {
        for (long j = 0; j < toursInternes; j = j + 1) {
            compteur = compteur + 1;
        }
    }
}

void WorkTask::task() {
    const int secondesRepos = 30;

    while (enMarche) {
        ecran->printAt(3, ligne, horloge.now());        // le début de la rafale
        horloge.startMeasureUs();
        charge();
        long dureeMs = horloge.stopMeasureUs() / 1000L;
        ecran->printAt(13, ligne, horloge.now());       // la fin de la rafale
        ecran->printAt(23, ligne, dureeMs);             // la durée mesurée, en ms

        // le repos, une seconde à la fois : q n'attend jamais plus d'une seconde
        for (int s = 0; s < secondesRepos && enMarche; s = s + 1) {
            sleep(1);
        }
    }
}

int main() {
    Screen ecran;
    Keyboard clavier;

    // quatre instances de la même classe, créées ici : rien ne s'alloue avec new
    WorkTask tache1("travail-1", &ecran, 3, politiqueDesTaches,
                    prioritesDesTaches[0], coeursDesTaches[0]);
    WorkTask tache2("travail-2", &ecran, 4, politiqueDesTaches,
                    prioritesDesTaches[1], coeursDesTaches[1]);
    WorkTask tache3("travail-3", &ecran, 5, politiqueDesTaches,
                    prioritesDesTaches[2], coeursDesTaches[2]);
    WorkTask tache4("travail-4", &ecran, 6, politiqueDesTaches,
                    prioritesDesTaches[3], coeursDesTaches[3]);
    WorkTask* taches[4] = {&tache1, &tache2, &tache3, &tache4};   // pour les boucles

    ecran.printAt(1, 1, "Les cinq ordonnancements. q : quitter");
    ecran.printAt(3, 2, "debut     fin       duree (ms)");

    for (int i = 0; i < 4; i = i + 1) {
        taches[i]->start();
    }

    bool fini = false;
    while (!fini) {
        if (clavier.kbhit(100)) {
            if (clavier.getch() == 'q') {
                fini = true;
            }
        }
    }

    for (int i = 0; i < 4; i = i + 1) {
        taches[i]->stop();
    }
    for (int i = 0; i < 4; i = i + 1) {
        taches[i]->join();
    }

    ecran.printAt(1, 8, "Fini.");
    return 0;
}
