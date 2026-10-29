// =============================================================================
//  GainSchedule : la table de gains (gain scheduling). FOURNIE, complete.
//
//  Un PID regle a une vitesse vaut pour cette vitesse. La table garde un
//  reglage par vitesse, le votre. A chaque tic, elle donne les gains de la
//  vitesse mesuree. Un exemple, avec des chiffres inventes :
//
//      vitesse   Kp    Ki    Kd
//        40     1.0   0.0   0.20
//        80     2.0   0.0   0.40        a 60 km/h : Kp 1.5 et Kd 0.30, la
//                                       moitie du chemin entre les deux lignes
//
//  Entre deux points : la ligne droite entre leurs gains (interpolation
//  lineaire). Sous le premier point ou au-dessus du dernier : les gains du
//  point le plus proche, sans extrapolation.
//
//  La boucle donne les gains au regulateur avec setGains(), qui garde sa
//  memoire.
// =============================================================================

#pragma once

/// Un point de la table : une vitesse, et les gains regles a cette vitesse.
struct GainPoint {
    double vitesseKmh;
    double kp;
    double ki;
    double kd;
};

class GainSchedule {
public:
    // Au plus kMaxPoints points : un nombre de pas borne pour la parcourir.
    static constexpr int kMaxPoints = 8;

    // Les points, en ordre croissant de vitesse. La table ne copie rien : elle
    // garde l'adresse du tableau. Le tableau doit vivre aussi longtemps que la
    // table (un tableau const en tete du programme).
    GainSchedule(const GainPoint* points, int nbPoints);

    // Vrai si la table est utilisable : de 1 a kMaxPoints points, des vitesses
    // strictement croissantes. course le verifie avant la course.
    bool valid() const { return valide_; }

    // Les gains pour cette vitesse. Une table invalide rend des gains nuls.
    GainPoint at(double vitesseKmh) const;

private:
    const GainPoint* points_;
    int nb_;
    bool valide_;
};
