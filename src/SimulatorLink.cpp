// =============================================================================
//  SimulatorLink : le code. FOURNI.
//
//  La classe du laboratoire 2. Une fonction par sorte de ligne recue, et une
//  boucle sans continue ni break.
// =============================================================================

#include "SimulatorLink.h"

#include <unistd.h>

#include <cstring>
#include <iostream>

#include "Command.h"

namespace conduite {

namespace {

constexpr int kMaxJetons = 12;   // la ligne la plus longue du protocole en a 8

}  // namespace

// Le lien s'ouvre ICI, et nulle part ailleurs. Reessaie toutes les 2 s : le
// simulateur peut demarrer apres le programme.
SimulatorLink::SimulatorLink(const char* hote, int port)
{
    std::cout << "Recherche du simulateur sur " << hote << ":" << port << "\n";
    int essai = 1;
    while (!OpenLink(lien_, hote, port)) {
        std::cout << "  essai " << essai << " : " << LinkError(lien_)
                  << ".  Nouvelle tentative dans 2 s. (Ctrl+C pour abandonner)\n";
        ::sleep(2);
        essai = essai + 1;
    }
}

// Le destructeur ferme le lien : la garantie du RAII.
SimulatorLink::~SimulatorLink()
{
    CloseLink(lien_);
}

void SimulatorLink::Send(const Command& commande)
{
    char ligne[kLongueurMaxLigne];
    commande.FormatLine(ligne, sizeof(ligne));
    if (!WriteLine(lien_, ligne)) {
        std::cerr << "[client] envoi impossible : " << LinkError(lien_) << "\n";
    }
}

bool SimulatorLink::WaitMeasurement(Mesure& mesure)
{
    char ligne[kLongueurMaxLigne];
    bool mesureRecue = false;
    bool rendreLaMain = false;
    while (!rendreLaMain && ReadLine(lien_, ligne, sizeof(ligne))) {
        rendreLaMain = HandleLine(ligne, mesure, mesureRecue);
    }
    // Sortie : une mesure, une sortie de route, la fin de la course, ou le lien
    // ferme. Le lien ne se ferme PAS ici : c'est le travail du destructeur.
    return mesureRecue;
}

// Une ligne recue. Retourne vrai quand WaitMeasurement doit rendre la main.
// Mot-cle inconnu : ignore, sans message (protocole, regle 2.1). Mot-cle connu
// mais mal forme : un message, et la boucle continue.
bool SimulatorLink::HandleLine(char* ligne, Mesure& mesure, bool& mesureRecue)
{
    const char* jetons[kMaxJetons];
    const int nb = SplitLine(ligne, jetons, kMaxJetons);
    bool rendreLaMain = false;

    if (nb == 0) {
        // une ligne vide : rien a faire
    } else if (std::strcmp(jetons[0], "MESURE") == 0) {
        mesureRecue = ReadMeasurement(jetons, nb, mesure);
        rendreLaMain = mesureRecue;
    } else if (std::strcmp(jetons[0], "VITESSE") == 0) {
        ReadSpeed(jetons, nb);
    } else if (std::strcmp(jetons[0], "CONSIGNE") == 0) {
        ReadSetpoint(jetons, nb);
    } else if (std::strcmp(jetons[0], "DEPART") == 0) {
        ReadStart(jetons, nb);
    } else if (std::strcmp(jetons[0], "BONJOUR") == 0) {
        ReadHello(jetons, nb);
    } else if (std::strcmp(jetons[0], "SORTIE_DE_ROUTE") == 0) {
        sorti_de_route_ = true;   // tout est fige : la boucle decide quand repartir
        rendreLaMain = true;
    } else if (std::strcmp(jetons[0], "RETARD") == 0) {
        retards_ = retards_ + 1;
    } else if (std::strcmp(jetons[0], "BILAN") == 0 && nb == 3) {
        bilan_.push_back(std::string(jetons[1]) + " " + jetons[2]);   // fin de course : allouer est correct
    } else if (std::strcmp(jetons[0], "FIN") == 0) {
        termine_ = true;
        rendreLaMain = true;
    } else if (std::strcmp(jetons[0], "ERREUR") == 0) {
        std::cerr << "[simulateur]";
        for (int i = 0; i < nb; i = i + 1) {
            std::cerr << " " << jetons[i];
        }
        std::cerr << "\n";
    }
    return rendreLaMain;
}

// MESURE <tick> <ecart_m> <dt_s>, quatre jetons. Retourne vrai si la mesure est
// lisible. Sinon : la ligne est rejetee, avec un message.
bool SimulatorLink::ReadMeasurement(const char* jetons[], int nb, Mesure& mesure)
{
    Mesure lue;
    if (nb != 4) {
        std::cerr << "[client] MESURE mal formee, ligne rejetee\n";
        return false;
    }
    if (!ParseInt(jetons[1], lue.tick) || !ParseDouble(jetons[2], lue.ecart) ||
        !ParseDouble(jetons[3], lue.dt)) {
        std::cerr << "[client] MESURE illisible, ligne rejetee\n";
        return false;
    }
    lue.nouveau_depart = nouveau_depart_;
    nouveau_depart_ = false;
    lue.vitesse_kmh = vitesse_kmh_;
    lue.consigne_kmh = consigne_kmh_;
    lue.decalage_m = decalage_m_;
    mesure = lue;
    return true;
}

// BONJOUR CONDUITE <version>, trois jetons. La version dit si le simulateur
// envoie la vitesse (1.5 et plus).
void SimulatorLink::ReadHello(const char* jetons[], int nb)
{
    double version = 0.0;
    if (nb == 3 && ParseDouble(jetons[2], version)) {
        version_ = version;
    }
}

// VITESSE <tick> <vitesse_kmh>, trois jetons. Elle arrive juste avant la MESURE
// du meme tic : gardee pour cette mesure.
void SimulatorLink::ReadSpeed(const char* jetons[], int nb)
{
    int tick = 0;
    double vitesse = 0.0;
    if (nb != 3 || !ParseInt(jetons[1], tick) || !ParseDouble(jetons[2], vitesse)) {
        std::cerr << "[client] VITESSE mal formee, ligne rejetee\n";
        return;
    }
    vitesse_kmh_ = vitesse;
}

// CONSIGNE <tick> <vitesse_kmh> <decalage_m>, quatre jetons. Elle arrive
// seulement au depart et quand elle change : la derniere est gardee.
void SimulatorLink::ReadSetpoint(const char* jetons[], int nb)
{
    int tick = 0;
    double vitesse = 0.0;
    double decalage = 0.0;
    if (nb != 4 || !ParseInt(jetons[1], tick) || !ParseDouble(jetons[2], vitesse) ||
        !ParseDouble(jetons[3], decalage)) {
        std::cerr << "[client] CONSIGNE mal formee, ligne rejetee\n";
        return;
    }
    consigne_kmh_ = vitesse;
    decalage_m_ = decalage;
}

// DEPART <tick> <periode_s> <vitesse_ms> <largeur_voie_m> <nb_voies> <route> <rayon_m>,
// huit jetons.
void SimulatorLink::ReadStart(const char* jetons[], int nb)
{
    Depart depart;
    if (nb != 8) {
        std::cerr << "[client] DEPART mal forme, ligne rejetee\n";
        return;
    }
    if (!ParseInt(jetons[1], depart.tick) || !ParseDouble(jetons[2], depart.periode_s) ||
        !ParseDouble(jetons[3], depart.vitesse_ms) || !ParseDouble(jetons[4], depart.largeur_voie_m) ||
        !ParseInt(jetons[5], depart.nb_voies) || !ParseDouble(jetons[7], depart.rayon_m)) {
        std::cerr << "[client] DEPART illisible, ligne rejetee\n";
        return;
    }
    depart.route = jetons[6];   // une course commence : allouer ici est correct
    dernier_depart_ = depart;
    nouveau_depart_ = true;
    sorti_de_route_ = false;
}

}  // namespace conduite
