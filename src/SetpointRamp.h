// =============================================================================
//  SetpointRamp : la rampe de consigne. FOURNIE.
//
//  La consigne appliquee glisse vers la consigne voulue, d'au plus
//  vitesse x dt par tic. Vitesse nulle : pas de rampe, la consigne change d'un
//  coup. La rampe agit AVANT le PID : P, I et D voient un changement doux.
// =============================================================================

#pragma once

class SetpointRamp {
public:
    // La vitesse de la rampe, en unites de la consigne par seconde (1 m/s pour
    // un changement de voie). 0 : pas de rampe. Negative : comme 0.
    explicit SetpointRamp(double vitesseParSeconde);

    // Au depart d'une course : la rampe part de la position actuelle. Rien ne
    // change d'un coup au premier tic.
    void start(double depart);

    // La consigne de ce tic : un pas vers la consigne voulue.
    double next(double voulue, double dt);

    double value() const { return appliquee_; }

private:
    double vitesse_;
    double appliquee_ = 0.0;
};
