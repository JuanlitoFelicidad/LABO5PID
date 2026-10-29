// =============================================================================
//  Trial : le code. FOURNI, complet.
// =============================================================================

#include "Trial.h"

#include <cstring>
#include <iostream>

#include "TcpLink.h"

namespace {

// Lit un gain : un nombre avec le point decimal, jamais negatif (un gain
// negatif inverserait la correction).
bool ReadGain(const char* texte, const char* nom, double& gain)
{
    double lu = 0.0;
    if (!conduite::ParseDouble(texte, lu)) {
        std::cout << nom << " illisible : " << texte << " (un nombre, avec le point decimal)\n";
        return false;
    }
    if (lu < 0.0) {
        std::cout << nom << " negatif : " << texte << " (un gain negatif inverserait la correction)\n";
        return false;
    }
    gain = lu;
    return true;
}

bool ReadAntiWindup(const char* texte, Pid::AntiWindup& mode)
{
    const char* noms[] = {"aucun", "conditionnel", "bande"};
    const Pid::AntiWindup modes[] = {Pid::AntiWindup::None, Pid::AntiWindup::ConditionalIntegration,
                                     Pid::AntiWindup::Band};
    for (int n = 0; n < 3; n = n + 1) {
        if (std::strcmp(texte, noms[n]) == 0) {
            mode = modes[n];
            return true;
        }
    }
    std::cout << "Anti-emballement inconnu : " << texte << " (aucun, conditionnel ou bande)\n";
    return false;
}

}  // namespace

const char* AntiWindupName(Pid::AntiWindup mode)
{
    if (mode == Pid::AntiWindup::ConditionalIntegration) {
        return "conditionnel";
    }
    if (mode == Pid::AntiWindup::Band) {
        return "bande";
    }
    return "aucun";
}

bool ReadTrialArguments(int argc, char** argv, TrialArguments& a)
{
    bool ok = argc <= 8;
    if (!ok) {
        std::cout << "Trop d'arguments.\n";
    }
    if (argc > 1) {
        a.hote = argv[1];
    }
    if (argc > 2 && !conduite::ParseInt(argv[2], a.port)) {
        std::cout << "Port illisible : " << argv[2] << "\n";
        ok = false;
    }
    if (argc > 3) {
        ok = ReadGain(argv[3], "Kp", a.kp) && ok;
    }
    if (argc > 4) {
        ok = ReadGain(argv[4], "Ki", a.ki) && ok;
    }
    if (argc > 5) {
        ok = ReadGain(argv[5], "Kd", a.kd) && ok;
    }
    if (argc > 6) {
        a.nomEssai = argv[6];
    }
    if (argc > 7) {
        ok = ReadAntiWindup(argv[7], a.antiWindup) && ok;
    }
    if (!ok) {
        std::cout << "Usage : " << argv[0] << " [adresse] [port] [kp] [ki] [kd] [nom] [anti]\n";
    }
    return ok;
}

void ReportStop(const conduite::SimulatorLink& lien, double tempsS)
{
    if (lien.IsFinished()) {
        std::cout << "\nCourse terminee.\n";
        for (const std::string& ligne : lien.Report()) {
            std::cout << "  " << ligne << "\n";
        }
    } else if (lien.WentOffRoad()) {
        std::cout << "Sortie de route a " << tempsS << " s.\n";
    } else {
        std::cout << "\nLe lien s'est ferme.\n";
    }
}

void SaveCurves(const CsvRecorder& enregistreur, const char* nomFichier)
{
    if (enregistreur.save(nomFichier)) {
        std::cout << "Courbes : " << nomFichier << " (" << enregistreur.size() << " lignes)\n";
    } else {
        std::cout << "Impossible d'ecrire " << nomFichier << "\n";
    }
    if (enregistreur.full()) {
        std::cout << "Attention : la place des courbes etait pleine, la fin de l'essai manque.\n";
    }
}
