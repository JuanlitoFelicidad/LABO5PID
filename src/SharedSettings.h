// SharedSettings.h : FOURNI. Les reglages : le clavier les change, la tache
// de commande les lit a chaque tic. La donnee et son mutex vont ensemble,
// comme SharedValue au laboratoire 4 : on y touche seulement par read et
// write.
#pragma once

#include "Mutex.h"
#include "Pid.h"

// Tout ce que le clavier peut changer pendant la course.
struct Settings {
    double kp = 0.5;
    double ki = 0.0;
    double kd = 0.0;
    double voieM = 0.0;                                   // la consigne : 0, -3,5 ou 3,5
    Pid::AntiWindup antiWindup = Pid::AntiWindup::None;
    int numeroEssai = 1;                                  // s'il change : la tache ecrit le CSV et repart
};

class SharedSettings {
public:
    Settings read();                     // une copie des reglages, le mutex pris
    void write(const Settings& nouveaux); // remplace les reglages d'un bloc, le mutex pris

private:
    Settings reglages;
    Mutex mutex;
};
