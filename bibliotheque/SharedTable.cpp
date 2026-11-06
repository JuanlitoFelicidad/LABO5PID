#include "SharedTable.h"

// Constructeur
SharedTable::SharedTable(bool protectionActive)
    : protectionActive(protectionActive) {

    for (int i = 0; i < 100; i++) {
        tableau[i] = 0;
    }
}

// Remplit le tableau et calcule le checksum
void SharedTable::fill(unsigned char valeur) {

    if (protectionActive) {
        mutex.take();
    }

    unsigned int somme = 0;

    for (int i = 0; i < 99; i++) {
        tableau[i] = valeur;
        somme += tableau[i];
    }

    tableau[99] = static_cast<unsigned char>(somme % 256);

    if (protectionActive) {
        mutex.release();
    }
}

// Vérifie si le checksum est correct
bool SharedTable::verify() {

    if (protectionActive) {
        mutex.take();
    }

    unsigned int somme = 0;

    for (int i = 0; i < 99; i++) {
        somme += tableau[i];
    }

    bool valide = (tableau[99] == (somme % 256));

    if (protectionActive) {
        mutex.release();
    }

    return valide;
}

// Active ou désactive la protection
void SharedTable::setProtection(bool active) {
    protectionActive = active;
}