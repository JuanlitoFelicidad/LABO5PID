// =============================================================================
//  GainSchedule : le code. FOURNI, complet.
// =============================================================================

#include "GainSchedule.h"

GainSchedule::GainSchedule(const GainPoint* points, int nbPoints)
    : points_(points), nb_(nbPoints), valide_(points != nullptr && nbPoints >= 1 && nbPoints <= kMaxPoints)
{
    for (int n = 1; valide_ && n < nb_; n = n + 1) {
        valide_ = points_[n].vitesseKmh > points_[n - 1].vitesseKmh;
    }
}

GainPoint GainSchedule::at(double vitesseKmh) const
{
    GainPoint gains = {vitesseKmh, 0.0, 0.0, 0.0};
    if (!valide_) {
        return gains;
    }

    // Hors de la table : le point le plus proche.
    const GainPoint& premier = points_[0];
    const GainPoint& dernier = points_[nb_ - 1];
    if (vitesseKmh <= premier.vitesseKmh) {
        gains = premier;
    } else if (vitesseKmh >= dernier.vitesseKmh) {
        gains = dernier;
    } else {
        // Les deux points autour de la vitesse, puis la ligne droite entre eux.
        int n = 1;
        while (n < nb_ - 1 && points_[n].vitesseKmh < vitesseKmh) {
            n = n + 1;
        }
        const GainPoint& a = points_[n - 1];
        const GainPoint& b = points_[n];
        const double f = (vitesseKmh - a.vitesseKmh) / (b.vitesseKmh - a.vitesseKmh);
        gains.kp = a.kp + f * (b.kp - a.kp);
        gains.ki = a.ki + f * (b.ki - a.ki);
        gains.kd = a.kd + f * (b.kd - a.kd);
    }
    gains.vitesseKmh = vitesseKmh;
    return gains;
}
