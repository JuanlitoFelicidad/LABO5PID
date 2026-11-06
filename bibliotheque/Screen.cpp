#include "Screen.h"

#include <cstdio>

Screen::Screen() {
    clear();
    printf("\033[?25l");   // cache le curseur
    fflush(stdout);
}

Screen::~Screen() {
    printf("\033[?25h\033[10;1H\n");   // remontre le curseur, descend sous l'affichage
    fflush(stdout);
}

void Screen::clear() {
    mutex.take();
    printf("\033[2J\033[H");
    fflush(stdout);
    mutex.release();
}

void Screen::printAt(int colonne, int ligne, const char* texte) {
    mutex.take();   // une seule tâche écrit à la fois
    printf("\033[%d;%dH%s", ligne, colonne, texte);
    fflush(stdout);
    mutex.release();
}

void Screen::printAt(int colonne, int ligne, int valeur) {
    printAt(colonne, ligne, (long)valeur);
}

void Screen::printAt(int colonne, int ligne, long valeur) {
    char texte[24];

    snprintf(texte, sizeof(texte), "%ld", valeur);
    printAt(colonne, ligne, texte);
}

void Screen::printAt(int colonne, int ligne, double valeur) {
    char texte[24];

    snprintf(texte, sizeof(texte), "%.2f", valeur);
    printAt(colonne, ligne, texte);
}
