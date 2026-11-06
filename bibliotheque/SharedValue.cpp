#include "SharedValue.h"

float SharedValue::read() {
    mutex.take();

    float copie = valeur;

    mutex.release();

    return copie;
}

void SharedValue::write(float nouvelleValeur) {
    mutex.take();

    valeur = nouvelleValeur;

    mutex.release();
}