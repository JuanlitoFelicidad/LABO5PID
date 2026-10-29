// =============================================================================
//  course : la course finale, a l'etape 9. FOURNI, complet. Les scenarios
//  PALIERS et AUCUN servent aussi a l'etape 8 : la table de gains au travail.
//
//  Les deux boucles tournent ensemble, avec VOS reglages :
//    - la direction, a chaque tic (30 Hz). Ses gains suivent la vitesse
//      mesuree, dans la table de l'etape 8 (GainSchedule);
//    - la vitesse, a 5 Hz : un tic sur six. La vitesse de la voiture change
//      lentement, en secondes. Entre deux envois, le simulateur garde la
//      derniere pedale recue.
//
//  Les panneaux du scenario changent la consigne de vitesse et la voie. Le
//  simulateur compte les contraventions et l'energie. A l'arrivee, le
//  programme affiche son bilan.
//
//  Dans ce fichier, vous changez seulement le bloc A CHANGER, plus bas. Le
//  reste, vous pouvez le lire, mais ne devriez pas le changer. Commencez votre
//  lecture par SpeedLoop().
//
//      ./build/course [adresse] [port] [nom]
//      ./build/course ADRESSE 5000 essai41
//
//  Deux fichiers, nommes d'apres l'essai et le scenario :
//      essai41_VILLAGE_direction.csv   30 Hz, en metres, avec le volant
//      essai41_VILLAGE_vitesse.csv      5 Hz, en km/h, avec la pedale
// =============================================================================

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "Command.h"
#include "CsvRecorder.h"
#include "GainSchedule.h"
#include "Pid.h"
#include "SetpointRamp.h"
#include "SimulatorLink.h"
#include "TcpLink.h"
#include "Trial.h"

namespace {

// -----------------------------------------------------------------------------
//  A CHANGER : le scenario, VOS reglages des etapes 4, 7 et 8, et la rampe. Apres
//  un changement, recompilez : cmake --build build.
// -----------------------------------------------------------------------------

// Votre nom, une fois. Le simulateur le recoit par la commande EQUIPE, et
// nomme son fichier de bilan avec.
const char* kEquipe = "equipe_a_nommer";

// Le scenario.
// Etape 8 : PALIERS, le parcours de la fin de l'etape 4. Au choix, AUCUN : une
// route de courbes, sans panneaux ni arrivee; la vitesse visee se change au
// gradateur du simulateur.
// Etape 9 : VILLAGE, AUTOROUTE, RANG ou MONTAGNE.
const char* kScenario = "VILLAGE";

// La direction : votre table de l'etape 8. Une ligne par vitesse : la vitesse
// en km/h, puis Kp, Ki et Kd. Les vitesses en ordre croissant. Gardez les
// vitesses; mettez les gains que vous avez regles a chaque vitesse.
const GainPoint kTableDirection[] = {
    {30.0, 0.5, 0.0, 0.0},
    {60.0, 0.5, 0.0, 0.0},    // vos gains de l'etape 2
    {100.0, 0.5, 0.0, 0.0},
};

// La vitesse : vos reglages des etapes 4 et 7, les trois gains et la
// protection du terme I. En ligne de commande : aucun, conditionnel ou bande.
// Ici : None, ConditionalIntegration ou Band.
const double kVitesseKp = 0.1;
const double kVitesseKi = 0.0;
const double kVitesseKd = 0.0;
const Pid::AntiWindup kVitesseAnti = Pid::AntiWindup::None;

// La rampe de l'etape 3, en metres par seconde : la consigne glisse vers la
// voie du panneau. AUTOROUTE change de voie au chantier : a 1.0, les 3,5 m
// prennent 3,5 s. Plus petit : plus doux, plus long, et l'ecart a la voie
// visee monte dans le bilan. 0.0 : la consigne change d'un coup. A essayer a
// l'etape 9, si vous voulez.
const double kRampeMs = 1.0;

// -----------------------------------------------------------------------------
//  NE PAS CHANGER : les memes valeurs pour toutes les courses et tout le
//  groupe. Sinon, deux bilans ne se comparent plus.
// -----------------------------------------------------------------------------
const double kBandeKmh = 10.0;          // la bande de l'etape 7, en km/h
const double kCadenceVitesseHz = 5.0;   // la boucle de vitesse : 5 fois par seconde

// La route du scenario AUCUN.
const char* kRouteLibre = "COURBES_LEGERES";
const double kVitesseLibreKmh = 60.0;

const double kPedaleMin = -1.0;
const double kPedaleMax = 1.0;
const double kVolantMin = -1.0;
const double kVolantMax = 1.0;
const double kDureeMaxS = 300.0;          // la borne de la boucle : les scenarios finissent avant
const std::size_t kMaxLignes = 18000;     // dix minutes a 30 Hz, reservees au depart

// Les deux boucles, et leur memoire.
struct Loops {
    Pid direction;
    Pid vitesse;
    GainSchedule table;
    SetpointRamp rampe;
    CsvRecorder courbesDirection;
    CsvRecorder courbesVitesse;
};

struct TrialState {
    double tempsS = 0.0;              // le temps depuis le depart de la course
    double depuisLaVitesseS = 0.0;    // le temps depuis le dernier calcul de la vitesse
    int retardsLocaux = 0;
};

bool IsFreeRoad()
{
    return std::strcmp(kScenario, "AUCUN") == 0;
}

// Un scenario impose ses conditions. Avec AUCUN, le programme les envoie.
void SendCourseSettings(conduite::SimulatorLink& lien)
{
    lien.Send(conduite::TeamCommand(kEquipe));
    lien.Send(conduite::SettingCommand("scenario", kScenario));
    if (IsFreeRoad()) {
        lien.Send(conduite::SettingCommand("vitesse", "PEDALE"));
        lien.Send(conduite::SettingCommand("route", kRouteLibre));
        lien.Send(conduite::SettingCommand("vitesse_kmh", kVitesseLibreKmh));
        lien.Send(conduite::SettingCommand("ecart_initial_m", 0.0));
    }
}

// Au depart d'une course : memoires, rampe et courbes repartent a neuf.
void Restart(Loops& b, TrialState& essai, double position)
{
    essai.tempsS = 0.0;
    essai.depuisLaVitesseS = 0.0;
    b.direction.reset();
    b.vitesse.reset();
    b.rampe.start(position);
    b.courbesDirection.clear();
    b.courbesVitesse.clear();
}

// La boucle de vitesse, a sa cadence. Son dt : le temps depuis son dernier
// calcul, pas la periode du tic. La marge d'un demi-tic absorbe l'arrondi du
// dt du simulateur (six decimales : six tics font 0,199998 s).
void SpeedLoop(conduite::SimulatorLink& lien, const conduite::Mesure& mesure, Loops& b, TrialState& essai)
{
    essai.depuisLaVitesseS = essai.depuisLaVitesseS + mesure.dt;
    if (essai.depuisLaVitesseS + 0.5 * mesure.dt < 1.0 / kCadenceVitesseHz) {
        return;   // pas encore son tour : le simulateur garde la derniere pedale
    }
    const double dt = essai.depuisLaVitesseS;
    essai.depuisLaVitesseS = 0.0;
    const double pedale = b.vitesse.compute(mesure.consigne_kmh, mesure.vitesse_kmh, dt);
    lien.Send(conduite::SpeedCurvesCommand(b.vitesse.termP(), b.vitesse.termI(), b.vitesse.termD()));
    lien.Send(conduite::PedalCommand(pedale));
    b.courbesVitesse.record(essai.tempsS, mesure.consigne_kmh, mesure.vitesse_kmh, pedale,
                            b.vitesse.termP(), b.vitesse.termI(), b.vitesse.termD());
}

// Un tic : la vitesse si c'est son tour, puis la direction, qui ferme le tic.
void Tick(conduite::SimulatorLink& lien, const conduite::Mesure& mesure, Loops& b, TrialState& essai)
{
    const auto debut = std::chrono::steady_clock::now();
    const double position = -mesure.ecart;   // la voiture par rapport au centre, positive a droite

    if (mesure.nouveau_depart) {
        Restart(b, essai, position);
    }

    SpeedLoop(lien, mesure, b, essai);

    // Les gains de la direction : ceux de la table, a la vitesse mesuree.
    // setGains() garde la memoire du regulateur.
    const GainPoint gains = b.table.at(mesure.vitesse_kmh);
    b.direction.setGains(gains.kp, gains.ki, gains.kd);
    const double consigne = b.rampe.next(mesure.decalage_m, mesure.dt);
    const double volant = b.direction.compute(consigne, position, mesure.dt);

    lien.Send(conduite::CurvesCommand(b.direction.termP(), b.direction.termI(), b.direction.termD()));
    lien.Send(conduite::SteeringCommand(volant));   // le VOLANT ferme toujours le tic
    b.courbesDirection.record(essai.tempsS, consigne, position, volant,
                              b.direction.termP(), b.direction.termI(), b.direction.termD());
    essai.tempsS = essai.tempsS + mesure.dt;

    // Un tour plus long que sa periode compte un retard.
    const std::chrono::duration<double> duree = std::chrono::steady_clock::now() - debut;
    if (duree.count() > mesure.dt) {
        essai.retardsLocaux = essai.retardsLocaux + 1;
    }
}

// La boucle de l'essai. Elle s'arrete a l'arrivee, a une sortie de route, si le
// lien se ferme, ou si le simulateur n'a pas de vitesse. Sa borne : kDureeMaxS
// (regle 2 de The Power of 10).
void RunTrial(conduite::SimulatorLink& lien, Loops& b, TrialState& essai)
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
            Tick(lien, mesure, b, essai);
            fini = essai.tempsS >= kDureeMaxS;
        }
    }
}

// Construit les noms des deux fichiers. false : ils ne tiennent pas dans les
// tampons.
bool BuildFileNames(char* direction, char* vitesse, std::size_t taille, const char* nomEssai)
{
    const int n1 = std::snprintf(direction, taille, "%s_%s_direction.csv", nomEssai, kScenario);
    const int n2 = std::snprintf(vitesse, taille, "%s_%s_vitesse.csv", nomEssai, kScenario);
    return n1 > 0 && n1 < static_cast<int>(taille) && n2 > 0 && n2 < static_cast<int>(taille);
}

// Lit la ligne de commande : [adresse] [port] [nom].
bool ReadArguments(int argc, char** argv, const char*& hote, int& port, const char*& nomEssai)
{
    bool ok = argc <= 4;
    if (argc > 1) {
        hote = argv[1];
    }
    if (argc > 2 && !conduite::ParseInt(argv[2], port)) {
        std::cout << "Port illisible : " << argv[2] << "\n";
        ok = false;
    }
    if (argc > 3) {
        nomEssai = argv[3];
    }
    if (!ok) {
        std::cout << "Usage : " << argv[0] << " [adresse] [port] [nom]\n";
    }
    return ok;
}

}  // namespace

int main(int argc, char** argv)
{
    const char* hote = "192.168.56.1";
    int port = 5000;
    const char* nomEssai = "course";
    char fichierDirection[160];
    char fichierVitesse[160];
    if (!ReadArguments(argc, argv, hote, port, nomEssai) ||
        !BuildFileNames(fichierDirection, fichierVitesse, sizeof(fichierDirection), nomEssai)) {
        return 1;
    }
    std::cout << std::unitbuf;   // chaque ligne parait tout de suite, meme redirigee

    // Tout ce qui alloue se fait ICI, avant la course.
    const int nbPoints = static_cast<int>(sizeof(kTableDirection) / sizeof(kTableDirection[0]));
    Loops b = {Pid(0.0, 0.0, 0.0, kVolantMin, kVolantMax),
               Pid(kVitesseKp, kVitesseKi, kVitesseKd, kPedaleMin, kPedaleMax),
               GainSchedule(kTableDirection, nbPoints), SetpointRamp(kRampeMs),
               CsvRecorder(kMaxLignes, kEntetePosition), CsvRecorder(kMaxLignes, kEnteteVitesse)};
    b.vitesse.setAntiWindup(kVitesseAnti, kBandeKmh);
    if (!b.table.valid()) {
        std::cout << "La table de la direction est invalide : de 1 a 8 lignes, des vitesses croissantes.\n";
        return 1;
    }

    conduite::SimulatorLink lien(hote, port);
    if (!lien.IsOpen()) {
        std::cout << "Pas de lien.\n";
        return 1;
    }
    SendCourseSettings(lien);
    std::cout << "Connecte a " << hote << ":" << port << "   scenario " << kScenario
              << "   vitesse : Kp=" << kVitesseKp << "  Ki=" << kVitesseKi << "  Kd=" << kVitesseKd
              << "  " << AntiWindupName(kVitesseAnti) << "   direction : " << nbPoints << " lignes\n";

    TrialState essai;
    RunTrial(lien, b, essai);

    SaveCurves(b.courbesDirection, fichierDirection);
    SaveCurves(b.courbesVitesse, fichierVitesse);
    std::cout << "Echeances manquees, vues par le simulateur : " << lien.LateWarnings() << "\n"
              << "Retards constates par notre boucle       : " << essai.retardsLocaux << "\n";
    return 0;
}   // <-- ICI, le destructeur de `lien` ferme le lien. C'est le RAII.
