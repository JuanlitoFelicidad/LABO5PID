// Mutex.h : le mutex (exclusion mutuelle). Celui qui l'a pris est seul dans la
// section critique : on prépare avant, on prend, on touche, on rend.
#pragma once

#include <pthread.h>

class Mutex {
public:
    Mutex();
    ~Mutex();

    // Un mutex ne se copie pas : la copie serait un autre mutex, qui ne protège rien.
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void take();      // prend le mutex; attend s'il est déjà pris
    void release();   // rend le mutex

private:
    pthread_mutex_t mutex;
};
