// =============================================================================
//  Command : chaque message envoye au simulateur est un OBJET. FOURNI.
//
//  Le patron commande (command pattern) du laboratoire 2. La boucle manipule
//  des objets (braquer, repartir, s'inscrire), jamais du texte. Chaque objet
//  ecrit sa ligne du protocole, au dernier moment. Si le protocole change,
//  une seule classe change.
// =============================================================================

#pragma once

#include <cstdio>
#include <cstring>

#include "TcpLink.h"

namespace conduite {

/// Le contrat : une commande ecrit sa propre ligne du protocole.
class Command {
public:
    virtual ~Command() = default;

    /// Ecrit la ligne (SANS le '\n' final) dans `ligne`.
    /// Aucune allocation : des tampons fixes seulement.
    virtual void FormatLine(char* ligne, std::size_t taille) const = 0;
};

/// EQUIPE <nom> : une seule fois, apres la connexion.
class TeamCommand : public Command {
public:
    explicit TeamCommand(const char* nom) : nom_(nom) {}

    void FormatLine(char* ligne, std::size_t taille) const override {
        std::snprintf(ligne, taille, "EQUIPE %s", nom_);
    }

private:
    const char* nom_;
};

/// REGLAGE <cle> <valeur> : les conditions de la course.
class SettingCommand : public Command {
public:
    SettingCommand(const char* cle, const char* valeur) : cle_(cle) {
        std::snprintf(valeur_, sizeof(valeur_), "%s", valeur);
    }

    SettingCommand(const char* cle, double valeur) : cle_(cle) {
        FormatDouble(valeur_, sizeof(valeur_), valeur);
    }

    /// Trois cles sont des ENTIERS : frequence_hz, voies et seed. Le simulateur
    /// refuse "30.000000". Cette surcharge ecrit "30".
    SettingCommand(const char* cle, int valeur) : cle_(cle) {
        std::snprintf(valeur_, sizeof(valeur_), "%d", valeur);
    }

    void FormatLine(char* ligne, std::size_t taille) const override {
        std::snprintf(ligne, taille, "REGLAGE %s %s", cle_, valeur_);
    }

private:
    const char* cle_;
    char        valeur_[32];
};

/// VOLANT <valeur> : de -1 a 1. Il ferme le tic : le simulateur l'attend a
/// chaque periode.
class SteeringCommand : public Command {
public:
    explicit SteeringCommand(double commande) : commande_(commande) {}

    void FormatLine(char* ligne, std::size_t taille) const override {
        char valeur[32];
        FormatDouble(valeur, sizeof(valeur), commande_);
        std::snprintf(ligne, taille, "VOLANT %s", valeur);
    }

private:
    double commande_;
};

/// COURBES <p> <i> <d> : toujours trois nombres, pour l'oscilloscope.
class CurvesCommand : public Command {
public:
    CurvesCommand(double p, double i, double d) : CurvesCommand("COURBES", p, i, d) {}

    void FormatLine(char* ligne, std::size_t taille) const override {
        char p[32], i[32], d[32];
        FormatDouble(p, sizeof(p), p_);
        FormatDouble(i, sizeof(i), i_);
        FormatDouble(d, sizeof(d), d_);
        std::snprintf(ligne, taille, "%s %s %s %s", motCle_, p, i, d);
    }

protected:
    // Pour une autre ligne de trois termes : seul le mot-cle change.
    CurvesCommand(const char* motCle, double p, double i, double d)
        : motCle_(motCle), p_(p), i_(i), d_(d) {}

private:
    const char* motCle_;
    double p_;
    double i_;
    double d_;
};

/// COURBES_VITESSE <p> <i> <d> : les trois termes du regulateur de vitesse, pour
/// l'oscilloscope (protocole 1.5). La ligne de COURBES avec un autre mot-cle :
/// l'heritage reutilise le reste.
class SpeedCurvesCommand : public CurvesCommand {
public:
    SpeedCurvesCommand(double p, double i, double d) : CurvesCommand("COURBES_VITESSE", p, i, d) {}
};

/// PEDALE <valeur> : de -1 (frein a fond) a 1 (plancher), protocole 1.5.
/// Facultative, avant le VOLANT du meme tic. Le simulateur garde la derniere
/// recue : pas besoin d'en envoyer une a chaque tic.
class PedalCommand : public Command {
public:
    explicit PedalCommand(double pedale) : pedale_(pedale) {}

    void FormatLine(char* ligne, std::size_t taille) const override {
        char valeur[32];
        FormatDouble(valeur, sizeof(valeur), pedale_);
        std::snprintf(ligne, taille, "PEDALE %s", valeur);
    }

private:
    double pedale_;
};

/// RELANCE : la course repart du debut (apres une sortie de route, ou a la
/// touche n de conduite_taches).
class RestartCommand : public Command {
public:
    void FormatLine(char* ligne, std::size_t taille) const override {
        std::snprintf(ligne, taille, "RELANCE");
    }
};

}  // namespace conduite
