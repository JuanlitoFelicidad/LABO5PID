// =============================================================================
//  Trial : ce qui est commun aux programmes d'essai. FOURNI, complet.
//
//  Ecrit une fois, ici :
//    - la lecture de la ligne de commande de conduite et de vitesse. Les deux
//      prennent les memes arguments, dans le meme ordre :
//          [adresse] [port] [kp] [ki] [kd] [nom] [anti]
//      anti : aucun, conditionnel ou bande (voir le TODO 2 de Pid.cpp);
//    - ce qui s'affiche a la fin d'un essai (conduite, vitesse et course).
// =============================================================================

#pragma once

#include "CsvRecorder.h"
#include "Pid.h"
#include "SimulatorLink.h"

struct TrialArguments {
    const char* hote = "192.168.56.1";
    int port = 5000;
    double kp = 0.0;
    double ki = 0.0;
    double kd = 0.0;
    const char* nomEssai = "essai";
    Pid::AntiWindup antiWindup = Pid::AntiWindup::None;
};

// Lit la ligne de commande dans `a`. `a` arrive avec les valeurs par defaut du
// programme. Chaque argument est verifie, chaque erreur est affichee.
// false : au moins une erreur.
bool ReadTrialArguments(int argc, char** argv, TrialArguments& a);

// Le nom de la protection, comme en ligne de commande et dans les noms de
// fichiers : aucun, conditionnel ou bande.
const char* AntiWindupName(Pid::AntiWindup mode);

// Affiche pourquoi la boucle s'est arretee : l'arrivee (avec le bilan du
// simulateur), une sortie de route, ou le lien ferme.
void ReportStop(const conduite::SimulatorLink& lien, double tempsS);

// Ecrit le fichier CSV, apres la course. Affiche le nom et le nombre de
// lignes, ou l'erreur.
void SaveCurves(const CsvRecorder& enregistreur, const char* nomFichier);
