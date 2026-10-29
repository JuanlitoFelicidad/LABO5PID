// =============================================================================
//  conduite_taches : la boucle de la direction dans une tache de la
//  bibliotheque du laboratoire 4. FOURNI, complet. L'etape 5. Elle demande
//  seulement le laboratoire 4.
//
//  Deux taches :
//    - la tache de commande (ControlTask), la SEULE qui lit et ecrit sur le
//      lien. A chaque mesure : elle lit les reglages partages, calcule le
//      volant avec votre Pid::compute(), l'envoie, garde la ligne du CSV;
//    - le programme principal : le clavier et l'ecran. Il ecrit les reglages
//      dans SharedSettings; la tache les lit a chaque tic. C'est la boite aux
//      lettres qui ecrase, comme SharedValue au laboratoire 4.
//
//      ./build/conduite_taches [adresse] [port]
//
//  Les touches :
//      p, P     divise Kp par deux, double Kp
//      i, I     la meme chose pour Ki
//      d, D     la meme chose pour Kd
//               Sous 0,01, un gain tombe a 0. Un gain nul repart de 0,1
//               (Kd : 0,05).
//      1, 2, 3  choisit la voie : gauche, centre, droite
//      a        passe a l'anti-emballement suivant : aucun, conditionnel, bande
//      n        ecrit l'essai et en commence un nouveau, avec les memes gains
//      q        quitte
//
//  Un gain ou la protection qui change ferme aussi l'essai en cours : son
//  fichier garde les gains qui l'ont roule, et la course repart du debut avec
//  les nouveaux. Un fichier = un reglage, depuis le depart.
//
//  Dans ce fichier, vous changez seulement le bloc A CHANGER, plus bas. Le
//  reste, vous pouvez le lire, mais ne devriez pas le changer. Commencez votre
//  lecture par ControlTask::task().
// =============================================================================

#include <cstdio>
#include <iostream>

#include "Command.h"
#include "CsvRecorder.h"
#include "Keyboard.h"
#include "Pid.h"
#include "Screen.h"
#include "SetpointRamp.h"
#include "SharedSettings.h"
#include "SimulatorLink.h"
#include "Thread.h"
#include "Trial.h"

namespace {

// -----------------------------------------------------------------------------
//  A CHANGER : votre nom, une fois. Les gains, la voie et l'anti-emballement se
//  changent au clavier, pendant que la voiture roule.
// -----------------------------------------------------------------------------
const char* kEquipe = "equipe_a_nommer";   // nomme le fichier de bilan du simulateur

// -----------------------------------------------------------------------------
//  NE PAS CHANGER : les conditions de l'etape 2, la ligne droite au depart
//  decale. Le reglage au clavier se compare ainsi a vos essais de l'etape 2.
// -----------------------------------------------------------------------------
const char* kRoute = "DROITE";
const double kVitesseKmh = 60.0;
const int    kFrequenceHz = 30;
const int    kVoies = 3;
const double kEcartInitialM = 0.2;

const double kRampeMs = 1.0;          // la consigne glisse vers la voie choisie, a 1 m/s
const double kBandeM = 0.7;           // avec la bande : PID sous 0,7 m d'erreur
const double kVolantMin = -1.0;
const double kVolantMax = 1.0;
const std::size_t kMaxLignes = 18000; // dix minutes a 30 Hz, reservees au depart

}  // namespace

// =============================================================================
//  La tache de commande
// =============================================================================
class ControlTask : public Thread {
public:
    ControlTask(const char* hote, int port, SharedSettings* reglagesPartages)
        : Thread("commande"),
          lien(hote, port),   // le lien s'ouvre ICI, et se fermera avec la tache
          reglages(reglagesPartages),
          pid(0.5, 0.0, 0.0, kVolantMin, kVolantMax),
          rampe(kRampeMs),
          enregistreur(kMaxLignes)
    {
    }

    bool isOpen() const { return lien.IsOpen(); }

    // L'ecran se cree apres le lien : les messages de connexion restent
    // lisibles. Il se donne a la tache juste avant start().
    void setScreen(Screen* nouvelEcran) { ecran = nouvelEcran; }

protected:
    void task() override;

private:
    void sendCourseSettings();
    void tick(const conduite::Mesure& mesure);
    void show(int tic, double position, double consigne, double volant);
    void endTrial();   // ecrit le CSV de l'essai en cours, puis repart a zero

    conduite::SimulatorLink lien;
    SharedSettings* reglages;
    Screen* ecran = nullptr;
    Pid pid;
    SetpointRamp rampe;
    CsvRecorder enregistreur;

    double tempsS = 0.0;
    int numeroEssai = 1;
};

// La boucle de la tache, tant que enMarche est vrai.
//   Une mesure : un tic.
//   Une sortie de route : l'essai s'ecrit, et la course repart.
//   Sinon (la course est finie, ou le lien s'est ferme) : la tache s'arrete.
void ControlTask::task()
{
    sendCourseSettings();
    while (enMarche) {
        conduite::Mesure mesure;
        if (lien.WaitMeasurement(mesure)) {
            tick(mesure);
        } else if (lien.WentOffRoad()) {
            endTrial();
            if (ecran != nullptr) {
                ecran->printAt(1, 8, "Sortie de route : l'essai est ecrit, la course repart.   ");
            }
            lien.Send(conduite::RestartCommand());
        } else {
            enMarche = false;   // la course est finie, ou le lien s'est ferme
        }
    }
    endTrial();
}

void ControlTask::sendCourseSettings()
{
    lien.Send(conduite::TeamCommand(kEquipe));
    lien.Send(conduite::SettingCommand("route", kRoute));
    lien.Send(conduite::SettingCommand("vitesse_kmh", kVitesseKmh));
    lien.Send(conduite::SettingCommand("frequence_hz", kFrequenceHz));
    lien.Send(conduite::SettingCommand("voies", kVoies));
    lien.Send(conduite::SettingCommand("ecart_initial_m", kEcartInitialM));
}

// Un tic. Les reglages se lisent une fois : une copie coherente, prise sous le
// mutex.
void ControlTask::tick(const conduite::Mesure& mesure)
{
    const Settings r = reglages->read();
    const double position = -mesure.ecart;   // la voiture par rapport au centre, positive a droite

    if (mesure.nouveau_depart) {
        tempsS = 0.0;
        pid.reset();
        enregistreur.clear();
        rampe.start(position);
    }

    if (r.numeroEssai != numeroEssai) {
        // Un gain change, la protection change, ou la touche n. Le fichier de cet
        // essai s'ecrit avec les gains qui l'ont roule. Ensuite, les nouveaux
        // gains s'appliquent, et la course repart du debut : un fichier = un
        // reglage, depuis le depart. Le volant de ce tic part quand meme : le
        // simulateur ne compte pas d'echeance manquee.
        endTrial();
        numeroEssai = r.numeroEssai;
        pid.setGains(r.kp, r.ki, r.kd);
        pid.setAntiWindup(r.antiWindup, kBandeM);
        lien.Send(conduite::SteeringCommand(0.0));
        lien.Send(conduite::RestartCommand());
    } else {
        const double consigne = rampe.next(r.voieM, mesure.dt);
        const double volant = pid.compute(consigne, position, mesure.dt);
        lien.Send(conduite::CurvesCommand(pid.termP(), pid.termI(), pid.termD()));
        lien.Send(conduite::SteeringCommand(volant));
        enregistreur.record(tempsS, consigne, position, volant, pid.termP(), pid.termI(), pid.termD());
        tempsS = tempsS + mesure.dt;
        show(mesure.tick, position, consigne, volant);
    }
}

// L'etat de la boucle, sur les lignes 5 et 6 de l'ecran. L'ecran a son propre
// mutex.
void ControlTask::show(int tic, double position, double consigne, double volant)
{
    if (ecran == nullptr) {
        return;
    }
    const char* etat = "      ";
    if (pid.requested() > kVolantMax || pid.requested() < kVolantMin) {
        etat = "SATURE";
    }
    char texte[100];
    std::snprintf(texte, sizeof(texte), "tic %6d   t %6.1f s   position %7.3f m   volant %6.3f   %s",
                  tic, tempsS, position, volant, etat);
    ecran->printAt(1, 5, texte);
    std::snprintf(texte, sizeof(texte), "P %7.3f   I %7.3f   D %7.3f   consigne %5.2f m   retards %d",
                  pid.termP(), pid.termI(), pid.termD(), consigne, lien.LateWarnings());
    ecran->printAt(1, 6, texte);
}

// Ecrit le fichier de l'essai en cours (nomme d'apres son numero et ses gains),
// puis vide les courbes.
void ControlTask::endTrial()
{
    if (enregistreur.size() == 0) {
        return;
    }
    char nomFichier[96];
    std::snprintf(nomFichier, sizeof(nomFichier), "taches-%02d_kp%g_ki%g_kd%g_%s.csv", numeroEssai,
                  pid.kp(), pid.ki(), pid.kd(), AntiWindupName(pid.antiWindup()));
    const bool ecrit = enregistreur.save(nomFichier);
    if (ecran != nullptr) {
        char texte[140];
        if (ecrit) {
            std::snprintf(texte, sizeof(texte), "dernier fichier : %s (%d lignes)          ",
                          nomFichier, static_cast<int>(enregistreur.size()));
        } else {
            std::snprintf(texte, sizeof(texte), "impossible d'ecrire %s          ", nomFichier);
        }
        ecran->printAt(1, 9, texte);
    }
    enregistreur.clear();
}

// =============================================================================
//  Le programme principal : le clavier et l'ecran
// =============================================================================
namespace {

// Double un gain. Un gain nul repart de sa valeur de depart.
double DoubleOrStart(double gain, double depart)
{
    if (gain <= 0.0) {
        return depart;
    }
    return gain * 2.0;
}

// Divise un gain par deux. Sous le plancher, il tombe a zero.
double HalveOrZero(double gain, double plancher)
{
    const double moitie = gain / 2.0;
    if (moitie < plancher) {
        return 0.0;
    }
    return moitie;
}

Pid::AntiWindup NextAntiWindup(Pid::AntiWindup mode)
{
    if (mode == Pid::AntiWindup::None) {
        return Pid::AntiWindup::ConditionalIntegration;
    }
    if (mode == Pid::AntiWindup::ConditionalIntegration) {
        return Pid::AntiWindup::Band;
    }
    return Pid::AntiWindup::None;
}

// Vrai pour les touches qui changent un gain ou la protection.
bool ChangesGains(char touche)
{
    const char touches[] = "pPiIdDa";
    for (int n = 0; touches[n] != '\0'; n = n + 1) {
        if (touche == touches[n]) {
            return true;
        }
    }
    return false;
}

// Applique la touche a la copie r. Retourne vrai si un reglage a change.
// Un gain ou la protection qui change commence aussi un nouvel essai.
bool ApplyKey(char touche, Settings& r)
{
    bool change = true;
    switch (touche) {
        case 'p': r.kp = HalveOrZero(r.kp, 0.01); break;
        case 'P': r.kp = DoubleOrStart(r.kp, 0.1); break;
        case 'i': r.ki = HalveOrZero(r.ki, 0.01); break;
        case 'I': r.ki = DoubleOrStart(r.ki, 0.1); break;
        case 'd': r.kd = HalveOrZero(r.kd, 0.01); break;
        case 'D': r.kd = DoubleOrStart(r.kd, 0.05); break;
        case '1': r.voieM = -3.5; break;
        case '2': r.voieM = 0.0; break;
        case '3': r.voieM = 3.5; break;
        case 'a': r.antiWindup = NextAntiWindup(r.antiWindup); break;
        case 'n': r.numeroEssai = r.numeroEssai + 1; break;
        default: change = false; break;
    }
    if (ChangesGains(touche)) {
        r.numeroEssai = r.numeroEssai + 1;
    }
    return change;
}

void ShowSettings(Screen& ecran, const Settings& r)
{
    char texte[120];
    std::snprintf(texte, sizeof(texte),
                  "Kp %6.3f   Ki %6.3f   Kd %6.3f   anti-emballement : %-12s   voie %5.1f m   essai %d",
                  r.kp, r.ki, r.kd, AntiWindupName(r.antiWindup), r.voieM, r.numeroEssai);
    ecran.printAt(1, 3, texte);
}

// Une touche : lire les reglages, les modifier, les ecrire d'un bloc.
// Retourne vrai pour quitter.
bool HandleKey(char touche, SharedSettings& reglages, Screen& ecran)
{
    if (touche == 'q') {
        return true;
    }
    Settings r = reglages.read();
    if (ApplyKey(touche, r)) {
        reglages.write(r);
        ShowSettings(ecran, r);
    }
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    const char* hote = "192.168.56.1";
    int port = 5000;
    if (argc > 1) {
        hote = argv[1];
    }
    if (argc > 2 && !conduite::ParseInt(argv[2], port)) {
        std::cout << "Port illisible : " << argv[2] << "\n";
        return 1;
    }

    SharedSettings reglages;
    ControlTask commande(hote, port, &reglages);   // attend le simulateur avant d'effacer l'ecran
    if (!commande.isOpen()) {
        std::cout << "Pas de lien.\n";
        return 1;
    }

    Screen ecran;
    Keyboard clavier;
    commande.setScreen(&ecran);
    ecran.printAt(1, 1, "Laboratoire 5 : la boucle dans une tache.   q : quitter");
    ecran.printAt(1, 2, "p/P i/I d/D : un gain divise ou double   1 2 3 : la voie   a : anti-emballement   n : nouvel essai");
    ShowSettings(ecran, reglages.read());

    if (!commande.start()) {
        ecran.printAt(1, 11, "La tache n'a pas demarre.");
        return 1;
    }
    bool fini = false;
    while (!fini) {
        if (clavier.kbhit(100)) {
            fini = HandleKey(clavier.getch(), reglages, ecran);
        }
    }
    commande.stop();
    commande.join();
    ecran.printAt(1, 11, "Fini.");
    return 0;
}
