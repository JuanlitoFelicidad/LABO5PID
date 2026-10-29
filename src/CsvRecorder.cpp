// =============================================================================
//  CsvRecorder : le code. FOURNI, complet.
// =============================================================================

#include "CsvRecorder.h"

#include <cstdio>

CsvRecorder::CsvRecorder(std::size_t maxLignes, const char* entete)
    : max_(maxLignes), entete_(entete)
{
    // Toute la memoire de la course s'alloue ICI, avant le depart. Dans la
    // boucle, push_back remplit cette place, sans allouer.
    lignes_.reserve(maxLignes);
}

void CsvRecorder::clear()
{
    lignes_.clear();   // la place reservee reste
}

void CsvRecorder::record(double tempsS, double consigne, double mesure, double sortie,
                         double p, double i, double d)
{
    if (lignes_.size() >= max_) {
        return;   // plein : la ligne est perdue, mais rien n'alloue dans la boucle
    }
    Ligne ligne;
    ligne.temps = tempsS;
    ligne.consigne = consigne;
    ligne.mesure = mesure;
    ligne.sortie = sortie;
    ligne.p = p;
    ligne.i = i;
    ligne.d = d;
    lignes_.push_back(ligne);
}

bool CsvRecorder::save(const char* nomFichier) const
{
    std::FILE* fichier = std::fopen(nomFichier, "w");
    if (fichier == nullptr) {
        return false;
    }
    // Une ecriture ratee (disque plein, carte retiree) n'est pas un succes :
    // ok garde le premier echec, jusqu'au fclose.
    bool ok = std::fprintf(fichier, "%s\n", entete_) > 0;
    for (std::size_t n = 0; ok && n < lignes_.size(); n = n + 1) {
        const Ligne& l = lignes_[n];
        ok = std::fprintf(fichier, "%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                          l.temps, l.consigne, l.mesure, l.consigne - l.mesure,
                          l.sortie, l.p, l.i, l.d) > 0;
    }
    if (std::fclose(fichier) != 0) {
        ok = false;
    }
    return ok;
}
