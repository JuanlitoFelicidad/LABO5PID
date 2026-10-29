// =============================================================================
//  vitesse : la boucle de vitesse, pour les etapes 4 et 7. FOURNI, complet.
//
//  Deux regulateurs, deux objets de la meme classe Pid, donc votre compute()
//  dans les deux :
//    - la direction, comme avant. Mesure : la position. Sortie : le volant.
//      Avec vos gains de l'etape 2, la voiture reste sur sa voie;
//    - la vitesse. Mesure : la vitesse. Consigne : celle du simulateur.
//      Sortie : la pedale (1 : l'accelerateur au plancher; -1 : le frein a
//      fond).
//
//  Un lancement = un essai sur un scenario, du depart a l'arrivee. A l'arrivee,
//  le programme affiche le bilan du simulateur : la vitesse maximale,
//  l'energie, les contraventions.
//
//  Dans ce fichier, vous changez seulement le bloc A CHANGER, plus bas. Le
//  reste, vous pouvez le lire, mais ne devriez pas le changer. Commencez votre
//  lecture par Tick().
//
//      ./build/vitesse [adresse] [port] [kp] [ki] [kd] [nom] [anti]
//      ./build/vitesse ADRESSE 5000 0.1 0 0 essai21
//
//  Les arguments, dans l'ordre :
//    adresse, port   le simulateur : l'adresse qu'il affiche, et le port 5000
//    kp, ki, kd      les gains de la VITESSE. Ceux de la direction sont dans
//                    le bloc A CHANGER.
//    nom             le nom de l'essai, au debut du nom du fichier
//    anti            la protection du terme I contre l'emballement, celle de
//                    l'etape 7 : aucun, conditionnel ou bande. Explications :
//                    le TODO 2 de Pid.cpp.
//  Un argument absent prend sa valeur par defaut : 5000, 0.1 0 0, essai et
//  aucun. L'adresse, donnez-la toujours.
//
//  Deux fichiers, nommes d'apres l'essai, le scenario et les gains :
//      essai21_ECHELON_kp0.1_ki0_kd0_aucun.csv             la vitesse, en km/h
//      essai21_ECHELON_kp0.1_ki0_kd0_aucun_direction.csv   la direction, en metres
//  Colonnes de la vitesse : temps_s,consigne_kmh,mesure_kmh,erreur_kmh,pedale,p,i,d
//  Colonnes de la direction : temps_s,consigne_m,mesure_m,erreur_m,volant,p,i,d
// =============================================================================

#include <chrono>
#include <cstdio>
#include <iostream>

#include "Command.h"
#include "CsvRecorder.h"
#include "Pid.h"
#include "SimulatorLink.h"
#include "Trial.h"

namespace {

// -----------------------------------------------------------------------------
//  A CHANGER : le scenario, et les gains de la direction. Apres un changement,
//  recompilez : cmake --build build. Le scenario fixe le reste : la route, le
//  relief, la vitesse de depart, les panneaux. Les gains de la vitesse : ligne
//  de commande.
// -----------------------------------------------------------------------------

// Votre nom, une fois. Le simulateur le recoit par la commande EQUIPE, et
// nomme son fichier de bilan avec.
const char* kEquipe = "equipe_a_nommer";

// Le scenario. Etape 4 : ECHELON, puis PALIERS. Etape 7 : COLLINE.
const char* kScenario = "ECHELON";

// Les gains de la direction : la voiture reste sur sa voie pendant que vous
// reglez la vitesse. Avant le premier essai, mettez vos gains de l'etape 2.
// Les deux scenarios sont en ligne droite, avec un depart au centre : la
// direction y travaille peu.
const double kDirectionKp = 0.5;
const double kDirectionKi = 0.0;
const double kDirectionKd = 0.0;

// -----------------------------------------------------------------------------
//  NE PAS CHANGER : les memes valeurs pour tous les essais et tout le groupe.
//  Sinon, deux courbes ne se comparent plus.
// -----------------------------------------------------------------------------

// La bande de la vitesse, en km/h, avec anti = bande. Sous 10 km/h d'erreur :
// P, I et D. Au-dela : P et D.
const double kBandeKmh = 10.0;

const double kPedaleMin = -1.0;            // le frein a fond
const double kPedaleMax = 1.0;             // l'accelerateur au plancher
const double kVolantMin = -1.0;
const double kVolantMax = 1.0;
const double kDureeMaxS = 300.0;           // la borne de la boucle : le scenario finit avant
const std::size_t kMaxLignes = 18000;      // dix minutes a 30 Hz, reservees au depart

// Les deux regulateurs : deux objets, une seule classe.
struct Regulators {
    Pid direction;
    Pid vitesse;
};

// Les courbes des deux regulateurs : un fichier chacun.
struct Curves {
    CsvRecorder vitesse;
    CsvRecorder direction;
};

// Ce que la boucle garde d'un tic a l'autre.
struct TrialState {
    double tempsS = 0.0;       // le temps depuis le depart de la course
    int retardsLocaux = 0;     // les tours plus longs que leur periode
};

// Construit les noms des deux fichiers. false : ils ne tiennent pas dans les
// tampons (nom d'essai trop long).
bool BuildFileNames(char* vitesse, char* direction, std::size_t taille, const TrialArguments& a)
{
    const int n1 = std::snprintf(vitesse, taille, "%s_%s_kp%g_ki%g_kd%g_%s.csv", a.nomEssai, kScenario,
                                 a.kp, a.ki, a.kd, AntiWindupName(a.antiWindup));
    const int n2 = std::snprintf(direction, taille, "%s_%s_kp%g_ki%g_kd%g_%s_direction.csv", a.nomEssai,
                                 kScenario, a.kp, a.ki, a.kd, AntiWindupName(a.antiWindup));
    return n1 > 0 && n1 < static_cast<int>(taille) && n2 > 0 && n2 < static_cast<int>(taille);
}

// Le scenario impose ses conditions, et relance la course.
void SendCourseSettings(conduite::SimulatorLink& lien)
{
    lien.Send(conduite::TeamCommand(kEquipe));
    lien.Send(conduite::SettingCommand("scenario", kScenario));
}

// Un tic : la mesure recue, la pedale et le volant envoyes, une ligne par CSV.
void Tick(conduite::SimulatorLink& lien, const conduite::Mesure& mesure, Regulators& r,
          Curves& courbes, TrialState& essai)
{
    const auto debut = std::chrono::steady_clock::now();
    const double position = -mesure.ecart;   // la voiture par rapport au centre, positive a droite

    if (mesure.nouveau_depart) {
        essai.tempsS = 0.0;
        r.direction.reset();         // nouvelle course : memoire des deux regulateurs videe
        r.vitesse.reset();
        courbes.vitesse.clear();     // nouvelle course : courbes videes
        courbes.direction.clear();
    }

    // La direction vise la voie de la consigne. La vitesse vise la vitesse de la
    // consigne.
    const double volant = r.direction.compute(mesure.decalage_m, position, mesure.dt);
    const double pedale = r.vitesse.compute(mesure.consigne_kmh, mesure.vitesse_kmh, mesure.dt);

    lien.Send(conduite::SpeedCurvesCommand(r.vitesse.termP(), r.vitesse.termI(), r.vitesse.termD()));
    lien.Send(conduite::PedalCommand(pedale));
    lien.Send(conduite::CurvesCommand(r.direction.termP(), r.direction.termI(), r.direction.termD()));
    lien.Send(conduite::SteeringCommand(volant));   // le VOLANT ferme toujours le tic
    courbes.vitesse.record(essai.tempsS, mesure.consigne_kmh, mesure.vitesse_kmh, pedale,
                           r.vitesse.termP(), r.vitesse.termI(), r.vitesse.termD());
    courbes.direction.record(essai.tempsS, mesure.decalage_m, position, volant,
                             r.direction.termP(), r.direction.termI(), r.direction.termD());
    essai.tempsS = essai.tempsS + mesure.dt;

    // Comme au laboratoire 2 : un tour plus long que sa periode compte un retard.
    const std::chrono::duration<double> duree = std::chrono::steady_clock::now() - debut;
    if (duree.count() > mesure.dt) {
        essai.retardsLocaux = essai.retardsLocaux + 1;
    }
}

// La boucle de l'essai. Elle s'arrete a l'arrivee, a une sortie de route, si le
// lien se ferme, ou si le simulateur n'a pas de vitesse. Sa borne : kDureeMaxS
// (regle 2 de The Power of 10).
void RunTrial(conduite::SimulatorLink& lien, Regulators& r, Curves& courbes, TrialState& essai)
{
    bool fini = false;
    while (!fini) {
        conduite::Mesure mesure;
        if (!lien.WaitMeasurement(mesure)) {
            ReportStop(lien, essai.tempsS);   // l'arrivee, une sortie de route ou le lien ferme
            fini = true;
        } else if (!lien.SupportsSpeed()) {
            std::cout << "Ce simulateur ne parle pas de vitesse : il faut la version 1.5 du protocole.\n";
            fini = true;
        } else {
            Tick(lien, mesure, r, courbes, essai);
            fini = essai.tempsS >= kDureeMaxS;
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    TrialArguments a;
    a.kp = 0.1;   // 0,1 de pedale par km/h d'ecart
    if (!ReadTrialArguments(argc, argv, a)) {
        return 1;
    }
    char fichierVitesse[160];
    char fichierDirection[160];
    if (!BuildFileNames(fichierVitesse, fichierDirection, sizeof(fichierVitesse), a)) {
        std::cout << "Nom d'essai trop long : " << a.nomEssai << "\n";
        return 1;
    }
    std::cout << std::unitbuf;   // chaque ligne parait tout de suite, meme redirigee

    // Tout ce qui alloue se fait ICI, avant la course.
    Regulators r = {Pid(kDirectionKp, kDirectionKi, kDirectionKd, kVolantMin, kVolantMax),
                    Pid(a.kp, a.ki, a.kd, kPedaleMin, kPedaleMax)};
    r.vitesse.setAntiWindup(a.antiWindup, kBandeKmh);
    Curves courbes = {CsvRecorder(kMaxLignes, kEnteteVitesse), CsvRecorder(kMaxLignes, kEntetePosition)};

    // Le lien s'ouvre ici et se ferme tout seul a la fin de main : le RAII.
    conduite::SimulatorLink lien(a.hote, a.port);
    if (!lien.IsOpen()) {
        std::cout << "Pas de lien.\n";
        return 1;
    }
    SendCourseSettings(lien);
    std::cout << "Connecte a " << a.hote << ":" << a.port << "   scenario " << kScenario
              << "   vitesse : Kp=" << a.kp << "  Ki=" << a.ki << "  Kd=" << a.kd
              << "   anti-emballement : " << AntiWindupName(a.antiWindup) << "\n";

    TrialState essai;
    RunTrial(lien, r, courbes, essai);

    SaveCurves(courbes.vitesse, fichierVitesse);
    SaveCurves(courbes.direction, fichierDirection);
    std::cout << "Echeances manquees, vues par le simulateur : " << lien.LateWarnings() << "\n"
              << "Retards constates par notre boucle       : " << essai.retardsLocaux << "\n";
    return 0;
}   // <-- ICI, le destructeur de `lien` ferme le lien. C'est le RAII.
