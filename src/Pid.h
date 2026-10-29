// =============================================================================
//  Pid : un regulateur PID. FOURNI : ce fichier ne change pas.
//
//  La boucle l'appelle une fois par periode, avec la consigne, la mesure et dt.
//  Il retourne la sortie. Il ne connait ni les threads ni le simulateur.
//
//  Dans Pid.cpp, tout est fourni sauf compute() : TODO 1, puis TODO 2.
//  essai_pid verifie compute() sans simulateur : lancez-le avant de conduire.
//
//  Convention du cours : types et methodes en anglais, variables et
//  commentaires en francais.
// =============================================================================

#pragma once

class Pid {
public:
    // La protection du terme I quand la sortie est a une borne (etape 7).
    //   None                    : aucune. L'aire s'accumule toujours.
    //   ConditionalIntegration  : pas d'aire ajoutee quand la somme sortirait
    //                             des bornes et que l'erreur pousse du meme bord
    //   Band                    : hors de la bande, l'aire repart a zero et la
    //                             sortie vaut P + D; dans la bande, le PID complet
    enum class AntiWindup { None, ConditionalIntegration, Band };

    // Les gains, puis les bornes de la sortie (-1 et 1 pour le volant et la
    // pedale).
    Pid(double kp, double ki, double kd, double sortieMin, double sortieMax);

    // Une periode. consigne : ou on veut etre. mesure : ou on est. dt : le
    // temps depuis l'appel d'avant, en secondes. Retourne la sortie, bornee.
    double compute(double consigne, double mesure, double dt);

    // Vide la memoire (l'aire, la mesure d'avant). Au depart d'une course.
    void reset();

    // Change les gains et garde la memoire. Appele par conduite_taches (le
    // clavier, etape 5) et par course (la table de gains, etapes 8 et 9).
    void setGains(double kp, double ki, double kd);
    double kp() const { return kp_; }
    double ki() const { return ki_; }
    double kd() const { return kd_; }

    // La protection du terme I (etape 7). bande : dans l'unite de la mesure.
    // Avec 0,7, le I travaille seulement quand l'erreur est sous 0,7 m.
    void setAntiWindup(AntiWindup mode, double bande = 0.0);
    AntiWindup antiWindup() const { return antiWindup_; }

    // P, I et D du dernier compute(), pour COURBES et le fichier CSV.
    double termP() const { return p_; }
    double termI() const { return i_; }
    double termD() const { return d_; }

    // La somme P + I + D AVANT les bornes. Plus grande que la borne : la
    // sortie sature.
    double requested() const { return demande_; }

private:
    double kp_, ki_, kd_;
    double sortieMin_, sortieMax_;
    AntiWindup antiWindup_ = AntiWindup::None;
    double bande_ = 0.0;

    // La memoire du regulateur.
    double integrale_ = 0.0;          // l'aire sous l'erreur, depuis le depart
    double mesurePrecedente_ = 0.0;   // la mesure d'avant, pour la pente du D
    bool premiereMesure_ = true;      // vrai : pas de mesure d'avant, donc D = 0

    // Le dernier calcul, pour les accesseurs.
    double p_ = 0.0, i_ = 0.0, d_ = 0.0, demande_ = 0.0;
};
