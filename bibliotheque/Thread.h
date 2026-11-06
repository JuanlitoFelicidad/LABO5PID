// Thread.h : une tâche = une classe dérivée qui implante task().
#pragma once

#include <pthread.h>

class Thread {
public:
    // politique : SCHED_OTHER, SCHED_FIFO ou SCHED_RR (sched(7)).
    // priorite  : 1 à 99 en temps réel; 0 en SCHED_OTHER.
    // coeur     : numéro du cœur, ou -1 pour laisser le noyau choisir.
    Thread(const char* nom, int politique = SCHED_OTHER, int priorite = 0, int coeur = -1);
    virtual ~Thread();

    // Une tâche ne se copie pas : elle n'a qu'un seul fil d'exécution.
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    bool start();   // démarre la tâche; false si le système refuse (temps réel sans sudo)
    void stop();    // demande la fin : enMarche tombe, la boucle de task() se termine
    void join();    // attend que la tâche soit terminée

protected:
    virtual void task() = 0;          // la boucle de la tâche : while (enMarche) { ... }
    volatile bool enMarche = false;   // écrit par stop() seulement; la boucle le lit

private:
    static void* handler(void* argument);   // le point d'entrée du fil; argument == this

    pthread_t fil;
    bool demarre = false;
    char nom[16];    // 15 caractères utiles : la limite du noyau pour un nom de thread
    int politique;
    int priorite;
    int coeur;
};
