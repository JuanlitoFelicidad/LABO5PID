// Clock.h : l'horloge d'une tâche. L'heure lisible, la mesure d'une durée, et le
// sommeil jusqu'au prochain tic. Une instance par tâche : rien n'y est partagé.
#pragma once

#include <ctime>

class Clock {
public:
    Clock();

    const char* now();                       // "HH:MM:SS", pour l'affichage
    void startMeasureUs();                   // début d'une mesure de durée
    long stopMeasureUs();                    // fin de la mesure; la durée en microsecondes
    void sleepUntilNextTic(int periodeMs);   // dort jusqu'au prochain multiple de la période

private:
    struct timespec debutMesure;
    struct timespec prochainTic;
    bool ticInitialise = false;
    char tampon[16];
};
