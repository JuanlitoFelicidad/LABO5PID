// Clock.cpp : toutes les mesures passent par l'horloge monotone, qui ne recule
// jamais; l'heure lisible, elle, vient de l'horloge du mur.
#include "Clock.h"

Clock::Clock() {
    tampon[0] = '\0';
}

const char* Clock::now() {
    time_t heure = time(NULL);
    struct tm heureLocale;

    localtime_r(&heure, &heureLocale);
    strftime(tampon, sizeof(tampon), "%H:%M:%S", &heureLocale);
    return tampon;
}

void Clock::startMeasureUs() {
    clock_gettime(CLOCK_MONOTONIC, &debutMesure);
}

long Clock::stopMeasureUs() {
    struct timespec fin;

    clock_gettime(CLOCK_MONOTONIC, &fin);
    long secondes = fin.tv_sec - debutMesure.tv_sec;
    long nanos = fin.tv_nsec - debutMesure.tv_nsec;
    return secondes * 1000000L + nanos / 1000L;
}

void Clock::sleepUntilNextTic(int periodeMs) {
    const long nsParMs = 1000000L;
    const long nsParSeconde = 1000000000L;

    if (!ticInitialise) {
        clock_gettime(CLOCK_MONOTONIC, &prochainTic);   // l'origine : maintenant
        ticInitialise = true;
    }

    // l'heure du prochain tic : l'ancienne plus une période, jamais « maintenant
    // plus une période ». C'est ce qui empêche la période de dériver.
    // Les secondes et les millisecondes s'ajoutent séparément : sur un Pi en 32 bits,
    // un long ne contient pas plus de 2,1 secondes comptées en nanosecondes.
    prochainTic.tv_sec = prochainTic.tv_sec + periodeMs / 1000;
    prochainTic.tv_nsec = prochainTic.tv_nsec + (periodeMs % 1000) * nsParMs;
    if (prochainTic.tv_nsec >= nsParSeconde) {
        prochainTic.tv_nsec = prochainTic.tv_nsec - nsParSeconde;
        prochainTic.tv_sec = prochainTic.tv_sec + 1;
    }

    /* TODO 3 : dormir jusqu'à l'heure prochainTic, pas pendant une durée, avec
       clock_nanosleep (man clock_nanosleep) : horloge CLOCK_MONOTONIC, flag
       TIMER_ABSTIME, dernier argument NULL. */
       clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &prochainTic, NULL);
}
