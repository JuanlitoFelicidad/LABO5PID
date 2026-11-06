// Mutex.cpp : un wrapper du mutex de pthreads, rien de plus.
#include "Mutex.h"

Mutex::Mutex() {
    pthread_mutex_init(&mutex, NULL);
}

Mutex::~Mutex() {
    pthread_mutex_destroy(&mutex);
}

void Mutex::take() {
    /* TODO 1a : prendre le mutex avec pthread_mutex_lock (man pthread_mutex_lock). */
    pthread_mutex_lock(&mutex);
}

void Mutex::release() {
    /* TODO 1b : rendre le mutex. La fonction est décrite dans la même page de man. */
    pthread_mutex_unlock(&mutex);
}
