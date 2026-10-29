// =============================================================================
//  CsvRecorder : les courbes d'un essai, en memoire pendant la course, puis
//  dans un fichier CSV a la fin. FOURNI, complet.
//
//  Pourquoi a la fin ? Une ecriture sur le disque peut bloquer la boucle
//  quelques millisecondes : du jitter, sur une periode de 33 ms a 30 Hz. Donc :
//    - le constructeur alloue la memoire, UNE fois, avant la course;
//    - record() range une ligne par tic, sans allouer;
//    - save() ecrit le fichier, apres la course.
//
//  Le fichier :
//
//      temps_s,consigne_m,mesure_m,erreur_m,volant,p,i,d
//      0.000,0.0000,-0.2000,0.2000,0.1000,0.1000,0.0000,0.0000
//      0.033,0.0000,-0.2000,0.2000,0.1000,0.1000,0.0000,0.0000
//      ...
//
//  Virgule entre les colonnes, point decimal.
//  erreur : consigne moins mesure. Consigne au centre : l'ecart du simulateur.
//  volant : la sortie, bornee. p + i + d : la demande, avant les bornes.
//
//  La boucle de vitesse range les memes huit colonnes, en km/h, avec la pedale
//  comme sortie. Seul l'en-tete change (kEnteteVitesse).
// =============================================================================

#pragma once

#include <cstddef>
#include <vector>

// Les deux en-tetes : la direction, en metres, et la vitesse, en km/h.
constexpr const char* kEntetePosition = "temps_s,consigne_m,mesure_m,erreur_m,volant,p,i,d";
constexpr const char* kEnteteVitesse = "temps_s,consigne_kmh,mesure_kmh,erreur_kmh,pedale,p,i,d";

class CsvRecorder {
public:
    // Reserve la place de maxLignes lignes, une fois. A 30 Hz, 18 000 lignes
    // font dix minutes. L'en-tete nomme les colonnes.
    explicit CsvRecorder(std::size_t maxLignes, const char* entete = kEntetePosition);

    // Oublie les lignes : un nouvel essai. La place reservee reste.
    void clear();

    // Une ligne par tic. N'alloue jamais : place pleine, la ligne est perdue,
    // et full() le dit.
    void record(double tempsS, double consigne, double mesure, double sortie,
                double p, double i, double d);

    // Ecrit le fichier. false : le fichier n'a pas pu s'ecrire au complet.
    bool save(const char* nomFichier) const;

    std::size_t size() const { return lignes_.size(); }
    bool full() const { return lignes_.size() >= max_; }

private:
    struct Ligne {
        double temps, consigne, mesure, sortie, p, i, d;
    };

    std::vector<Ligne> lignes_;   // reserve() au constructeur : plus d'allocation ensuite
    std::size_t max_;
    const char* entete_;
};
