// Thread.cpp : le wrapper de pthreads. La politique, la priorité et le cœur se règlent
// à la création; la fin est coopérative (stop() puis join(), jamais d'arrêt brutal).
#include "Thread.h"

#include <cstdio>
#include <cstring>

Thread::Thread(const char* nom, int politique, int priorite, int coeur)
    : politique(politique), priorite(priorite), coeur(coeur) {
    strncpy(this->nom, nom, sizeof(this->nom) - 1);
    this->nom[sizeof(this->nom) - 1] = '\0';
}

Thread::~Thread() {
    stop();
    join();
}

bool Thread::start() {
    pthread_attr_t attributs;
    struct sched_param parametres;

    pthread_attr_init(&attributs);
    memset(&parametres, 0, sizeof(parametres));   // initialisée dans TOUS les cas

    if (politique != SCHED_OTHER) {
        // sans EXPLICIT_SCHED, le fil hériterait de la politique de son créateur
        pthread_attr_setinheritsched(&attributs, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attributs, politique);
        parametres.sched_priority = priorite;
        pthread_attr_setschedparam(&attributs, &parametres);
    }
    if (coeur >= 0) {
        cpu_set_t ensemble;
        CPU_ZERO(&ensemble);
        CPU_SET(coeur, &ensemble);
        pthread_attr_setaffinity_np(&attributs, sizeof(ensemble), &ensemble);
    }

    enMarche = true;
    int code = -1;
    /* TODO 2 : créer le fil d'exécution avec pthread_create (man pthread_create).
       Le fil exécute handler, qui doit recevoir this en argument; les attributs
       sont prêts. Rangez le code de retour dans la variable code, déclarée juste
       au-dessus. Donc : code = pthread_create(...). */

       code = pthread_create(&fil, &attributs, handler, this);

    pthread_attr_destroy(&attributs);

    if (code != 0) {
        enMarche = false;
        if (code == -1) {
            fprintf(stderr, "%s : le fil n'a pas été créé : le TODO 2 est à remplir.\n", nom);
        } else {
            fprintf(stderr, "%s : démarrage refusé (code %d).", nom, code);
            fprintf(stderr, " En temps réel, lancez le programme avec sudo.\n");
        }
        return false;
    }
    demarre = true;
    return true;
}

void Thread::stop() {
    enMarche = false;
}

void Thread::join() {
    if (demarre) {
        pthread_join(fil, NULL);
        demarre = false;
    }
}

void* Thread::handler(void* argument) {
    Thread* tache = (Thread*)argument;

    pthread_setname_np(pthread_self(), tache->nom);   // le nom que ps -L affiche
    tache->task();
    return NULL;
}
