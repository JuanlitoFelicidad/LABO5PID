// =============================================================================
//  TcpLink : le lien TCP avec le simulateur, en style C. FOURNI.
//
//  Le meme lien qu'au laboratoire 2, decoupe en petites fonctions.
//
//  Pourquoi du style C ? Les sockets du systeme sont en C : des fonctions
//  libres, une structure passee en parametre, et rien qui ferme a votre place.
//  Un lien ouvert puis oublie reste ouvert. SimulatorLink regle ce probleme
//  avec le RAII.
//
//  RIEN N'ALLOUE ICI : tampons de taille fixe, pas de std::string.
//
//  Le protocole : protocole.md, remis au laboratoire 2.
// =============================================================================

#pragma once

#include <cstddef>

namespace conduite {

/// Longueur maximale d'une ligne du protocole, '\n' exclus (protocole.md, s.2).
constexpr std::size_t kLongueurMaxLigne = 256;

/// L'etat d'un lien TCP. Pas de constructeur qui ouvre, pas de destructeur qui
/// ferme : SimulatorLink s'en charge.
struct TcpLink {
    int         descripteur = -1;              ///< la prise (socket), -1 = ferme
    char        tampon[kLongueurMaxLigne * 4]; ///< octets recus, en attente d'un '\n'
    std::size_t utilises = 0;                  ///< combien d'octets attendent
    char        erreur[160] = "";              ///< dernier message d'erreur
};

// ---------------------------------------------------------------------------
//  Ouvrir et fermer
// ---------------------------------------------------------------------------

/// Ouvre le lien vers le simulateur. Une adresse IPv4 NUMERIQUE seulement :
/// "localhost" ou un nom de machine sont refuses. Le port : de 1 a 65535.
///
/// delaiMaxS : quand rien n'ecoute en face, le pare-feu de Windows JETTE les
/// paquets au lieu de les refuser. Un connect() ordinaire resterait bloque deux
/// minutes, sans message. Pas trop court non plus : le premier contact peut
/// prendre quelques secondes, le temps que le pare-feu decide.
bool OpenLink(TcpLink& lien, const char* adresseIpv4, int port, double delaiMaxS = 12.0);

/// Ferme le lien. Sans danger sur un lien deja ferme.
void CloseLink(TcpLink& lien);

bool IsLinkOpen(const TcpLink& lien);

/// Le dernier message d'erreur du lien, en clair.
const char* LinkError(const TcpLink& lien);

// ---------------------------------------------------------------------------
//  Lire et ecrire des lignes
//
//  REGLE COMMUNE : ReadLine ou WriteLine retourne false a cause du reseau, et
//  le lien est FERME au passage. IsLinkOpen retourne alors false, et LinkError
//  dit pourquoi. Jamais de lien a moitie utilisable.
// ---------------------------------------------------------------------------

/// Bloque jusqu'a la prochaine ligne complete, et la copie dans `ligne` (sans
/// le '\n', sans le '\r'). false : le lien s'est ferme, ou la ligne depasse
/// `taille`.
///
/// AUCUN DELAI MAXIMAL. Un simulateur connecte qui n'envoie plus rien : l'appel
/// attend toujours. C'est voulu : le simulateur donne la cadence. C'est la seule
/// boucle sans borne du programme (regle 2 de The Power of 10). Elle attend le
/// reseau, et s'arrete quand le lien se ferme.
///
/// TCP envoie des octets, pas des lignes : un paquet peut couper une ligne en
/// deux, ou en contenir trois. Les octets s'accumulent dans le tampon du lien;
/// une ligne sort seulement quand son '\n' est arrive.
bool ReadLine(TcpLink& lien, char* ligne, std::size_t taille);

/// Ecrit la ligne sur le lien, avec le '\n'. false : le lien s'est ferme.
bool WriteLine(TcpLink& lien, const char* ligne);

// ---------------------------------------------------------------------------
//  Aides d'analyse. Sans allocation, elles aussi.
// ---------------------------------------------------------------------------

/// Decoupe la ligne EN PLACE sur les espaces : chaque espace de fin de jeton
/// devient un '\0', et `jetons` recoit l'adresse de chaque debut. Retourne le
/// nombre de jetons (au plus `max`). Les jetons pointent DANS votre tampon :
/// aucune copie.
///
/// Separe seulement sur l'ESPACE, sans guillemets : c'est la regle du
/// protocole (protocole.md, s.2).
int SplitLine(char* ligne, const char* jetons[], int max);

/// Lit un entier. false si le jeton n'est pas EXACTEMENT un entier : "123abc"
/// est refuse. En cas d'echec, `valeur` ne change pas.
bool ParseInt(const char* jeton, int& valeur);

/// Lit un reel AVEC LE POINT DECIMAL, peu importe la langue du systeme.
/// Refuse "nan" et "inf" : dans une boucle de commande, un NaN fausse tous
/// les calculs suivants, sans alarme. En cas d'echec, `valeur` ne change pas.
bool ParseDouble(const char* jeton, double& valeur);

/// Ecrit un reel avec le point decimal, six decimales, notation fixe.
void FormatDouble(char* tampon, std::size_t taille, double valeur);

}  // namespace conduite
