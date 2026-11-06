// Screen.h : l'écran partagé. Chaque tâche écrit à sa position, une seule à la
// fois : le mutex interne fait respecter l'accord. Sans lui, deux écritures
// simultanées se mélangent à l'écran.
#pragma once

#include "Mutex.h"

class Screen {
public:
    Screen();    // efface l'écran et cache le curseur
    ~Screen();   // remontre le curseur, laisse l'affichage en place

    // Un écran ne se copie pas : il n'y en a qu'un, et un seul mutex le protège.
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    void clear();
    void printAt(int colonne, int ligne, const char* texte);
    void printAt(int colonne, int ligne, int valeur);
    void printAt(int colonne, int ligne, long valeur);
    void printAt(int colonne, int ligne, double valeur);

private:
    Mutex mutex;
};
