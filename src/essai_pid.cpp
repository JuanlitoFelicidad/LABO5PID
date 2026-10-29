// =============================================================================
//  essai_pid : le programme d'essai de la classe Pid. FOURNI, complet. Vous
//  pouvez le lire, mais ne devriez pas le changer.
//
//  Verifie votre compute() avant de conduire la voiture. Sans simulateur ni
//  reseau : le programme invente une consigne et une mesure, appelle compute()
//  a 10 Hz (dt = 0,1 s), et compare chaque terme a sa valeur attendue. Huit
//  verifications, chacune nommee. Tout est juste : la derniere ligne dit
//  "8 verifications sur 8". Le TODO 2 (l'anti-emballement) n'est pas verifie
//  ici : le TODO 1 suffit pour les huit.
//
//      ./build/essai_pid
//
//  Les gains de l'essai : Kp 0,3, Ki 0,6, Kd 0,2, les bornes -1 et 1.
// =============================================================================

#include <cmath>
#include <cstdio>

#include "Pid.h"

namespace {

const double kDt = 0.1;   // 10 Hz : des chiffres faciles a verifier de tete

// Le compte des verifications, d'un essai a l'autre.
struct Score {
    int reussies = 0;
    int faites = 0;
};

bool Close(double a, double b)
{
    return std::fabs(a - b) < 1e-6;
}

void Check(Score& score, const char* nom, bool ok, const char* detail)
{
    const char* verdict = "ECHEC";
    if (ok) {
        verdict = "OK";
        score.reussies = score.reussies + 1;
    }
    score.faites = score.faites + 1;
    std::printf("  %-5s %s\n        %s\n", verdict, nom, detail);
}

void PrintHeader(const char* titre)
{
    std::printf("\n%s\n", titre);
    std::printf("  %6s %9s %9s %9s %9s %9s\n", "tic", "consigne", "mesure", "P", "I", "D");
}

void PrintRow(int tic, double consigne, double mesure, const Pid& pid)
{
    // "+ 0.0" efface le signe d'un zero negatif : 0.000 au lieu de -0.000.
    std::printf("  %6d %9.3f %9.3f %9.3f %9.3f %9.3f\n", tic, consigne, mesure,
                pid.termP() + 0.0, pid.termI() + 0.0, pid.termD() + 0.0);
}

// A. Une erreur en echelon : la consigne vaut 1, la mesure reste a 0, dix tics.
// P doit rester a 0,3. I doit monter de 0,06 par tic (0,6 x 1 m x 0,1 s) et valoir
// 0,6 apres 10 tics. D doit rester a 0 : la mesure ne bouge pas.
void TestStep(Score& score, Pid& pid)
{
    PrintHeader("A. Erreur en echelon : consigne 1, mesure 0, 10 tics");
    bool pConstant = true;
    bool iEnRampe = true;
    bool dNul = true;
    bool sommeJuste = true;
    for (int tic = 1; tic <= 10; tic = tic + 1) {
        const double sortie = pid.compute(1.0, 0.0, kDt);
        PrintRow(tic, 1.0, 0.0, pid);
        pConstant = pConstant && Close(pid.termP(), 0.3);
        iEnRampe = iEnRampe && Close(pid.termI(), 0.6 * 1.0 * kDt * tic);
        dNul = dNul && Close(pid.termD(), 0.0);
        sommeJuste = sommeJuste && Close(sortie, pid.termP() + pid.termI() + pid.termD());
    }
    Check(score, "P constant", pConstant, "P = Kp x erreur = 0,3 a chaque tic");
    Check(score, "I en rampe", iEnRampe, "I = Ki x aire : 0,06 par tic, 0,6 apres 1 s");
    Check(score, "D nul", dNul, "la mesure ne bouge pas, donc pas de pente");
    Check(score, "sortie = P + I + D", sommeJuste, "la somme des trois termes, sans borne atteinte");
}

// B. La consigne change brusquement, la mesure non. Un D calcule sur l'erreur
// donnerait un coup : 0,2 x (1 m / 0,1 s) = 2. Le D sur la mesure reste a 0.
void TestSetpointChange(Score& score, Pid& pid)
{
    PrintHeader("B. La consigne passe de 1 a 2, la mesure reste a 0");
    pid.compute(2.0, 0.0, kDt);
    PrintRow(11, 2.0, 0.0, pid);
    Check(score, "D sur la mesure", Close(pid.termD(), 0.0),
          "la consigne a change, pas la mesure : D = 0 (sur l'erreur, il vaudrait 2)");
}

// C. La mesure bouge : de 0 a 0,3 en un tic. La pente vaut 3 m/s, et le D vaut
// -0,2 x 3 = -0,6 : il freine la montee.
void TestMovingMeasurement(Score& score, Pid& pid)
{
    PrintHeader("C. La mesure passe de 0 a 0,3 en un tic");
    pid.compute(2.0, 0.3, kDt);
    PrintRow(12, 2.0, 0.3, pid);
    Check(score, "D = -Kd x pente", Close(pid.termD(), -0.6),
          "pente de la mesure 3 m/s, D = -0,2 x 3 = -0,6 : le signe moins freine");
}

// D. Les bornes : Kp 10, consigne 1, mesure 0. Le calcul demande 10, le volant
// s'arrete a 1.
void TestBounds(Score& score)
{
    PrintHeader("D. Les bornes : Kp 10, consigne 1, mesure 0");
    Pid fort(10.0, 0.0, 0.0, -1.0, 1.0);
    const double bornee = fort.compute(1.0, 0.0, kDt);
    PrintRow(1, 1.0, 0.0, fort);
    std::printf("  sortie retournee : %.3f   demandee : %.3f\n", bornee, fort.requested());
    Check(score, "sortie bornee", Close(bornee, 1.0) && Close(fort.requested(), 10.0),
          "le calcul demande 10, compute() retourne 1, requested() garde le 10");
}

// E. reset() : la memoire repart vide. Apres le reset, un tic avec une erreur
// de 1 doit donner I = 0,06 (l'aire d'un seul tic) et D = 0 (pas de mesure
// d'avant), meme si la mesure etait a 0,3 juste avant.
void TestReset(Score& score, Pid& pid)
{
    PrintHeader("E. reset(), puis un tic : consigne 1, mesure 0");
    pid.reset();
    pid.compute(1.0, 0.0, kDt);
    PrintRow(1, 1.0, 0.0, pid);
    Check(score, "reset vide la memoire", Close(pid.termI(), 0.06) && Close(pid.termD(), 0.0),
          "I repart a 0,06 (un seul tic d'aire), D = 0 (la mesure d'avant est oubliee)");
}

}  // namespace

int main()
{
    std::printf("Essai de la classe Pid : Kp 0,3  Ki 0,6  Kd 0,2  bornes -1 et 1  dt 0,1 s\n");

    Score score;
    Pid pid(0.3, 0.6, 0.2, -1.0, 1.0);
    TestStep(score, pid);               // les essais A, B et C se suivent sur le meme Pid :
    TestSetpointChange(score, pid);     // l'integrale et la mesure d'avant comptent
    TestMovingMeasurement(score, pid);
    TestBounds(score);
    TestReset(score, pid);

    std::printf("\n%d verifications sur %d.\n", score.reussies, score.faites);
    if (score.reussies != score.faites) {
        return 1;
    }
    return 0;
}
