// =============================================================================
//  SimulatorLink : le lien avec le simulateur. FOURNI.
//
//  La classe du laboratoire 2, avec le RAII : le lien s'ouvre au CONSTRUCTEUR
//  et se ferme au DESTRUCTEUR. Le compilateur garantit l'appel du destructeur.
//  Aucun appel pour fermer, nulle part dans la boucle.
//
//  Convention du cours : noms de types et de methodes en anglais, variables
//  et commentaires en francais.
// =============================================================================

#pragma once

#include <string>
#include <vector>

#include "TcpLink.h"

namespace conduite {

class Command;  // declaree seulement : Send() prend une reference

/// Une mesure du simulateur. Une par tic.
struct Mesure {
    int    tick = 0;
    double ecart = 0.0;   ///< metres. Le centre par rapport a la voiture : POSITIF = a droite.
    double dt = 0.0;      ///< secondes depuis la mesure d'avant.
    bool   nouveau_depart = false;  ///< vrai au premier tic qui suit un DEPART.

    // Le mode PEDALE (protocole 1.5). Zero si le simulateur ne les envoie pas.
    double vitesse_kmh = 0.0;   ///< la VITESSE du meme tic, recue juste avant la MESURE.
    double consigne_kmh = 0.0;  ///< la derniere CONSIGNE de vitesse. Change aux panneaux.
    double decalage_m = 0.0;    ///< la derniere voie visee, depuis le centre, positive a droite.
};

/// Les parametres de la course, recus dans la ligne DEPART.
struct Depart {
    int         tick = 0;
    double      periode_s = 0.0;
    double      vitesse_ms = 0.0;
    double      largeur_voie_m = 0.0;
    int         nb_voies = 0;
    std::string route;          ///< DROITE, VIRAGE, COURBES_LEGERES ou COURBES_FORTES
    double      rayon_m = 0.0;  ///< rayon du virage, ou le plus serre du parcours
};

class SimulatorLink {
public:
    /// Ouvre le lien. Reessaie toutes les 2 s : le simulateur peut demarrer
    /// apres le programme.
    SimulatorLink(const char* hote, int port);

    /// Ferme le lien. Garanti.
    ~SimulatorLink();

    // Un lien ne se copie pas : un seul descripteur.
    SimulatorLink(const SimulatorLink&) = delete;
    SimulatorLink& operator=(const SimulatorLink&) = delete;

    bool IsOpen() const { return IsLinkOpen(lien_); }

    /// Envoie une commande. La commande ecrit sa ligne; le lien l'envoie.
    void Send(const Command& commande);

    /// Bloque jusqu'a la prochaine mesure.
    ///
    ///   true  : la mesure est remplie. Calculer, puis repondre.
    ///   false : trois cas, dans cet ordre.
    ///           IsFinished()   la course est terminee;
    ///           WentOffRoad()  sortie de route. TOUT EST FIGE jusqu'a une
    ///                          RELANCE;
    ///           sinon          le lien s'est ferme.
    bool WaitMeasurement(Mesure& mesure);

    const Depart& LastStart() const { return dernier_depart_; }

    /// Vrai si le simulateur parle le protocole 1.5 : la vitesse, la consigne et
    /// la pedale. Le BONJOUR le dit, a la connexion.
    bool SupportsSpeed() const { return version_ >= 1.5; }

    bool IsFinished() const { return termine_; }
    bool WentOffRoad() const { return sorti_de_route_; }
    int  LateWarnings() const { return retards_; }
    const std::vector<std::string>& Report() const { return bilan_; }

private:
    // Une fonction par sorte de ligne recue. HandleLine retourne vrai quand
    // WaitMeasurement doit rendre la main.
    bool HandleLine(char* ligne, Mesure& mesure, bool& mesureRecue);
    bool ReadMeasurement(const char* jetons[], int nb, Mesure& mesure);
    void ReadStart(const char* jetons[], int nb);
    void ReadHello(const char* jetons[], int nb);
    void ReadSpeed(const char* jetons[], int nb);
    void ReadSetpoint(const char* jetons[], int nb);

    TcpLink lien_;
    Depart  dernier_depart_;
    bool    nouveau_depart_ = false;
    bool    sorti_de_route_ = false;
    bool    termine_ = false;
    int     retards_ = 0;
    double  version_ = 0.0;        // le numero du BONJOUR
    double  vitesse_kmh_ = 0.0;    // la derniere VITESSE
    double  consigne_kmh_ = 0.0;   // la derniere CONSIGNE, gardee jusqu'a la suivante
    double  decalage_m_ = 0.0;

    // Le bilan se remplit a la FIN de la course, jamais pendant : il peut
    // allouer. Une fois : allouer, oui. A chaque tic : jamais.
    std::vector<std::string> bilan_;
};

}  // namespace conduite
