// =============================================================================
//  SetpointRamp : le code. FOURNI.
// =============================================================================

#include "SetpointRamp.h"

SetpointRamp::SetpointRamp(double vitesseParSeconde)
    : vitesse_(vitesseParSeconde)
{
    // Une vitesse negative : pas de rampe.
    if (vitesse_ < 0.0) {
        vitesse_ = 0.0;
    }
}

void SetpointRamp::start(double depart)
{
    appliquee_ = depart;
}

double SetpointRamp::next(double voulue, double dt)
{
    if (vitesse_ <= 0.0 || dt <= 0.0) {
        appliquee_ = voulue;   // pas de rampe (ou dt nul) : la consigne change d'un coup
        return appliquee_;
    }
    const double pasMax = vitesse_ * dt;
    double pas = voulue - appliquee_;
    if (pas > pasMax) {
        pas = pasMax;
    }
    if (pas < -pasMax) {
        pas = -pasMax;
    }
    appliquee_ = appliquee_ + pas;
    return appliquee_;
}
