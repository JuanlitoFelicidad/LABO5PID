// SharedSettings.cpp : FOURNI. La section critique la plus courte : prendre
// le mutex, copier, le rendre.
#include "SharedSettings.h"

Settings SharedSettings::read()
{
    mutex.take();
    Settings copie = reglages;
    mutex.release();
    return copie;
}

void SharedSettings::write(const Settings& nouveaux)
{
    mutex.take();
    reglages = nouveaux;
    mutex.release();
}
