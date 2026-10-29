// =============================================================================
//  conduite : la boucle de commande de la direction. FOURNI, complet.
//
//  La boucle du laboratoire 2, avec la classe Pid, la rampe de consigne et
//  l'enregistrement des courbes. Dans tout le paquet, une seule fonction a
//  ecrire : Pid::compute(), dans Pid.cpp.
//
//  Dans ce fichier, vous changez seulement le bloc A CHANGER, plus bas. Le
//  reste, vous pouvez le lire, mais ne devriez pas le changer. Commencez votre
//  lecture par Tick().
//
//  Un lancement = un essai. Le programme regle le simulateur, conduit pendant
//  kDureeEssaiS secondes, puis ecrit les courbes dans un fichier CSV. Une
//  sortie de route arrete l'essai plus tot.
//
//      ./build/conduite [adresse] [port] [kp] [ki] [kd] [nom] [anti]
//      ./build/conduite ADRESSE 5000 0.5 0 0 essai01
//
//  Les arguments, dans l'ordre :
//    adresse, port   le simulateur : l'adresse qu'il affiche, et le port 5000
//    kp, ki, kd      les gains de la direction
//    nom             le nom de l'essai, au debut du nom du fichier
//    anti            la protection du terme I contre l'emballement : aucun,
//                    conditionnel ou bande. Explications : le TODO 2 de
//                    Pid.cpp. Aucune etape ne la demande pour la direction :
//                    on la regle a l'etape 7, sur la vitesse.
//  Un argument absent prend sa valeur par defaut : 5000, 0.5 0 0, essai et
//  aucun. L'adresse, donnez-la toujours.
//
//  Le nom du fichier dit l'essai, ses conditions et ses gains :
//      essai01_DROITE_60kmh_e0.2_kp0.5_ki0_kd0_aucun.csv
//  Deux essais differents n'ecrivent jamais le meme fichier. Le meme essai
//  refait remplace l'ancien fichier.
// =============================================================================

#include <chrono>
#include <cstdio>
#include <iostream>

#include "Command.h"
#include "CsvRecorder.h"
#include "Pid.h"
#include "SetpointRamp.h"
#include "SimulatorLink.h"
#include "Trial.h"

namespace {

// -----------------------------------------------------------------------------
//  A CHANGER : les conditions de l'essai. Chaque ligne dit a quelle etape la
//  changer, et quelle valeur y mettre. Apres un changement, recompilez :
//  cmake --build build. Les gains ne sont pas ici : ligne de commande.
// -----------------------------------------------------------------------------

// Votre nom, une fois, avant le premier essai. Le simulateur le recoit par la
// commande EQUIPE, et nomme son fichier de bilan avec.
const char* kEquipe = "equipe_a_nommer";

// La route : DROITE, VIRAGE, COURBES_LEGERES ou COURBES_FORTES.
// Etape 2 : DROITE, puis VIRAGE, puis COURBES_FORTES.
// Etape 8 : DROITE, puis COURBES_LEGERES pour verifier chaque reglage.
// Aux autres etapes : DROITE.
const char* kRoute = "DROITE";

// La vitesse de la voiture, en km/h. Etape 8 : 30.0, puis 100.0. Aux autres
// etapes : 60.0.
const double kVitesseKmh = 60.0;

// L'ecart au depart, en metres : la voiture part a cote du centre (0.2 : a
// gauche), et le regulateur doit l'y ramener.
// 0.2 sur la ligne droite. 0.0 au virage, dans les courbes et a l'etape 3.
const double kEcartInitialM = 0.2;

// Etape 3 seulement : la consigne change, et la voiture change de voie.
//   kVoieCibleM : la voie visee, en metres depuis le centre, positive a
//     droite. 3.5 : le centre de la voie de droite.
//   kChangementDeVoieS : le moment du changement, en secondes apres le depart.
//     0.0 : pas de changement de voie.
//   kRampeMs : 0.0, la consigne change d'un coup. Sinon, elle glisse vers la
//     voie visee a kRampeMs metres par seconde.
// Apres l'etape 3 : les trois a 0.0, et kEcartInitialM a 0.2.
const double kVoieCibleM = 0.0;            // etape 3 : 3.5
const double kChangementDeVoieS = 0.0;     // etape 3 : 2.0
const double kRampeMs = 0.0;               // etape 3, troisieme essai : 1.0

// -----------------------------------------------------------------------------
//  NE PAS CHANGER : les memes valeurs pour tous les essais et tout le groupe.
//  Sinon, deux courbes ne se comparent plus.
// -----------------------------------------------------------------------------
const int    kFrequenceHz = 30;
const int    kVoies = 3;
const double kDureeEssaiS = 30.0;

// La bande, en metres, avec anti = bande. Sous 0,7 m d'erreur : P, I et D.
// Au-dela : P et D. 0,7 m = 20 % d'un changement de voie de 3,5 m.
const double kBandeM = 0.7;

const double kVolantMin = -1.0;
const double kVolantMax = 1.0;
const std::size_t kMaxLignes = 18000;      // dix minutes a 30 Hz, reservees au depart

// Ce que la boucle garde d'un tic a l'autre.
struct TrialState {
    double tempsS = 0.0;       // le temps depuis le depart de la course
    int retardsLocaux = 0;     // les tours plus longs que leur periode
};

// Construit le nom du fichier. false : le nom ne tient pas dans le tampon (nom
// d'essai trop long).
bool BuildFileName(char* nom, std::size_t taille, const TrialArguments& a)
{
    char voie[64] = "";
    if (kChangementDeVoieS > 0.0) {
        std::snprintf(voie, sizeof(voie), "_voie%g_a%gs_rampe%g", kVoieCibleM, kChangementDeVoieS, kRampeMs);
    }
    const int n = std::snprintf(nom, taille, "%s_%s_%gkmh_e%g_kp%g_ki%g_kd%g%s_%s.csv",
                                a.nomEssai, kRoute, kVitesseKmh, kEcartInitialM, a.kp, a.ki, a.kd,
                                voie, AntiWindupName(a.antiWindup));
    return n > 0 && n < static_cast<int>(taille);
}

// Les conditions de la course, envoyees juste apres la connexion. Les cles
// entieres (frequence_hz, voies) s'ecrivent sans decimales.
void SendCourseSettings(conduite::SimulatorLink& lien)
{
    lien.Send(conduite::TeamCommand(kEquipe));
    lien.Send(conduite::SettingCommand("route", kRoute));
    lien.Send(conduite::SettingCommand("vitesse_kmh", kVitesseKmh));
    lien.Send(conduite::SettingCommand("frequence_hz", kFrequenceHz));
    lien.Send(conduite::SettingCommand("voies", kVoies));
    lien.Send(conduite::SettingCommand("ecart_initial_m", kEcartInitialM));
}

// La consigne voulue a ce moment : le centre, puis la voie visee.
double DesiredSetpoint(double tempsS)
{
    if (kChangementDeVoieS > 0.0 && tempsS >= kChangementDeVoieS) {
        return kVoieCibleM;
    }
    return 0.0;
}

// Un tic : la mesure recue, le volant envoye, une ligne du CSV en memoire.
void Tick(conduite::SimulatorLink& lien, const conduite::Mesure& mesure, Pid& pid,
          SetpointRamp& rampe, CsvRecorder& enregistreur, TrialState& essai)
{
    const auto debut = std::chrono::steady_clock::now();

    // MESURE donne le centre par rapport a la voiture. Pid veut la voiture par
    // rapport au centre, positive a droite : le meme nombre, de signe inverse.
    const double position = -mesure.ecart;

    if (mesure.nouveau_depart) {
        essai.tempsS = 0.0;
        pid.reset();             // nouvelle course : memoire videe
        enregistreur.clear();    // nouvelle course : courbes videes
        rampe.start(position);   // la rampe part de la position actuelle
    }

    const double consigne = rampe.next(DesiredSetpoint(essai.tempsS), mesure.dt);
    const double volant = pid.compute(consigne, position, mesure.dt);

    lien.Send(conduite::CurvesCommand(pid.termP(), pid.termI(), pid.termD()));   // l'oscilloscope
    lien.Send(conduite::SteeringCommand(volant));                                // la seule ligne qui conduit
    enregistreur.record(essai.tempsS, consigne, position, volant, pid.termP(), pid.termI(), pid.termD());
    essai.tempsS = essai.tempsS + mesure.dt;

    // Comme au laboratoire 2 : un tour plus long que sa periode compte un retard.
    const std::chrono::duration<double> duree = std::chrono::steady_clock::now() - debut;
    if (duree.count() > mesure.dt) {
        essai.retardsLocaux = essai.retardsLocaux + 1;
    }
}

// La boucle de l'essai. Elle s'arrete apres kDureeEssaiS secondes, a une sortie
// de route, a la fin de la course ou si le lien se ferme. Sa borne : la duree
// de l'essai (regle 2 de The Power of 10).
void RunTrial(conduite::SimulatorLink& lien, Pid& pid, SetpointRamp& rampe,
              CsvRecorder& enregistreur, TrialState& essai)
{
    bool fini = false;
    while (!fini) {
        conduite::Mesure mesure;
        if (lien.WaitMeasurement(mesure)) {
            Tick(lien, mesure, pid, rampe, enregistreur, essai);
            fini = essai.tempsS >= kDureeEssaiS;
            if (fini) {
                std::cout << "Fin de l'essai apres " << kDureeEssaiS << " s.\n";
            }
        } else {
            ReportStop(lien, essai.tempsS);   // fin de course, sortie de route ou lien ferme
            fini = true;
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    TrialArguments a;
    a.kp = 0.5;   // le Kp du premier essai
    if (!ReadTrialArguments(argc, argv, a)) {
        return 1;
    }
    char nomFichier[160];
    if (!BuildFileName(nomFichier, sizeof(nomFichier), a)) {
        std::cout << "Nom d'essai trop long : " << a.nomEssai << "\n";
        return 1;
    }
    std::cout << std::unitbuf;   // chaque ligne parait tout de suite, meme redirigee

    // Tout ce qui alloue se fait ICI, avant la course : la place des courbes est
    // reservee une fois. Rien n'alloue dans la boucle.
    Pid pid(a.kp, a.ki, a.kd, kVolantMin, kVolantMax);
    pid.setAntiWindup(a.antiWindup, kBandeM);
    SetpointRamp rampe(kRampeMs);
    CsvRecorder enregistreur(kMaxLignes);

    // Le lien s'ouvre ici et se ferme tout seul a la fin de main : le RAII.
    conduite::SimulatorLink lien(a.hote, a.port);
    if (!lien.IsOpen()) {
        std::cout << "Pas de lien.\n";
        return 1;
    }
    SendCourseSettings(lien);
    std::cout << "Connecte a " << a.hote << ":" << a.port << "   Kp=" << a.kp << "  Ki=" << a.ki
              << "  Kd=" << a.kd << "   anti-emballement : " << AntiWindupName(a.antiWindup) << "\n";

    TrialState essai;
    RunTrial(lien, pid, rampe, enregistreur, essai);

    // Fin de la course : le fichier s'ecrit maintenant, jamais pendant la boucle.
    SaveCurves(enregistreur, nomFichier);
    std::cout << "Echeances manquees, vues par le simulateur : " << lien.LateWarnings() << "\n"
              << "Retards constates par notre boucle       : " << essai.retardsLocaux << "\n";
    return 0;
}   // <-- ICI, le destructeur de `lien` ferme le lien. C'est le RAII.
