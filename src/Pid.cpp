// =============================================================================
//  Pid : le code de la classe. Tout est fourni, sauf compute() : VOTRE fonction.
//
//  TODO 1 (etape 1) : le calcul du PID, avec la sortie bornee.
//  TODO 2 (etape 7) : l'anti-emballement, dans la meme fonction.
// =============================================================================

#include "Pid.h"

// -----------------------------------------------------------------------------
//  FOURNI : range les gains et les bornes. La memoire part vide.
// -----------------------------------------------------------------------------
Pid::Pid(double kp, double ki, double kd, double sortieMin, double sortieMax)
    : kp_(kp), ki_(ki), kd_(kd), sortieMin_(sortieMin), sortieMax_(sortieMax)
{
    // Bornes inversees : on les remet dans l'ordre.
    if (sortieMin_ > sortieMax_) {
        sortieMin_ = sortieMax;
        sortieMax_ = sortieMin;
    }
}

// -----------------------------------------------------------------------------
//  FOURNI : change les gains, garde la memoire (l'aire du I, la mesure d'avant).
//
//  Appele par conduite_taches quand un gain change au clavier (etape 5), et par
//  course a chaque tic, avec les gains de la table (etapes 8 et 9).
//  Un reset() ou un nouvel objet Pid effacerait l'aire : le I tomberait a zero
//  d'un coup, et la sortie aussi.
// -----------------------------------------------------------------------------
void Pid::setGains(double kp, double ki, double kd)
{
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

// -----------------------------------------------------------------------------
//  FOURNI : choisit la protection du terme I (voir le TODO 2) et la largeur de
//  la bande, dans l'unite de la mesure.
// -----------------------------------------------------------------------------
void Pid::setAntiWindup(AntiWindup mode, double bande)
{
    antiWindup_ = mode;
    bande_ = bande;
}

// -----------------------------------------------------------------------------
//  FOURNI : vide la memoire, au depart d'une course.
//  L'aire repart de zero. Le prochain tic n'a pas de mesure d'avant : D = 0.
// -----------------------------------------------------------------------------
void Pid::reset()
{
    integrale_ = 0.0;
    mesurePrecedente_ = 0.0;
    premiereMesure_ = true;
    p_ = 0.0;
    i_ = 0.0;
    d_ = 0.0;
    demande_ = 0.0;
}

// -----------------------------------------------------------------------------
//  TODO 1 (etape 1) : une periode de calcul.
//
//  Ce que compute() doit faire, dans l'ordre :
//
//    1. l'erreur : consigne moins mesure;
//    2. P : kp_ fois l'erreur;
//    3. I : l'erreur fois dt s'ajoute a integrale_, puis I vaut ki_ fois integrale_;
//    4. D : la pente de la MESURE, (mesure - mesurePrecedente_) / dt, fois -kd_.
//       Le signe moins : quand la mesure monte vers la consigne, l'erreur baisse.
//       Au premier tic, pas de pente : D vaut 0;
//    5. la somme P + I + D, rangee dans demande_;
//    6. la sortie : demande_ ramenee entre sortieMin_ et sortieMax_;
//    7. retenir la mesure pour le prochain tic, et ranger P, I et D dans p_, i_ et
//       d_ (termP(), termI() et termD() les donnent a la boucle).
//
//  Le TODO 2, plus bas, attend l'etape 7 : d'ici la, ignorez antiWindup_.
//  essai_pid ne le verifie pas.
//
//  TODO 2 (etape 7) : l'anti-emballement, selon antiWindup_.
//
//  Le probleme : l'emballement du terme integral (integral windup).
//    - Le I additionne l'erreur a chaque tic. C'est lui qui efface le petit
//      ecart que le P laisse.
//    - La sortie a des bornes. Dans une cote, la pedale est au plancher (1),
//      et la voiture reste quand meme sous la consigne.
//    - Le I additionne toujours. La somme P + I + D demande 1,5, puis 2, puis
//      3. La pedale reste a 1 : ce surplus ne sert a rien, mais il s'accumule
//      dans integrale_.
//    - Apres le sommet, l'erreur change de signe. Le I doit d'abord vider
//      l'aire accumulee. Pendant ce temps, la pedale reste enfoncee, et la
//      voiture va trop vite.
//
//  Le remede : le I n'accumule pas ce qui ne peut pas servir. antiWindup_
//  choisit la facon :
//
//    None : aucune protection. Le I accumule toujours.
//
//    ConditionalIntegration, l'integration conditionnelle (conditional
//    integration) :
//      - calculez la somme P + I + D avec l'aire de ce tic;
//      - si elle sort des bornes ET que l'erreur pousse du meme bord,
//        n'ajoutez pas l'aire de ce tic : integrale_ garde sa valeur;
//      - si l'erreur pousse de l'autre bord, ajoutez l'aire : elle aide la
//        sortie a quitter sa borne.
//
//    Band, la bande :
//      - si l'erreur, en valeur absolue, depasse bande_, remettez integrale_
//        a zero : la sortie vaut P + D;
//      - sinon, le PID complet.
//      Loin de la consigne, le P suffit. Le I sert pres de la consigne.
//
//  Le P, le D, la somme et les bornes : comme au TODO 1.
// -----------------------------------------------------------------------------
double Pid::compute(double consigne, double mesure, double dt)
{
    (void)consigne;   // a retirer des que vous vous en servez
    (void)mesure;
    (void)dt;
    return 0.0;
}
