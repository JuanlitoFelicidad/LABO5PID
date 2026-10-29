// =============================================================================
//  TcpLink : le lien reseau, en style C. FOURNI.
//
//  Le meme comportement qu'au laboratoire 2, en fonctions courtes : chacune
//  fait une seule chose, en 60 lignes au plus (regle 4 de The Power of 10).
//
//  Regle du fichier : UNE OPERATION RESEAU ECHOUE, LE LIEN SE FERME. Apres un
//  ReadLine ou un WriteLine qui retourne false, IsLinkOpen retourne false, et
//  LinkError dit pourquoi.
// =============================================================================

#include "TcpLink.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace conduite {

namespace {

// La fin de tout echec, ecrite une fois : noter le message, fermer le lien,
// retourner false.
bool Fail(TcpLink& lien, const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(lien.erreur, sizeof(lien.erreur), format, arguments);
    va_end(arguments);
    CloseLink(lien);
    return false;
}

// L'adresse et le port, verifies d'abord. Un port hors plage deviendrait un
// autre port, sans message : htons(-1) donne 65535. inet_pton accepte
// SEULEMENT les adresses numeriques : un nom demanderait getaddrinfo, inutile
// ici.
bool PrepareAddress(TcpLink& lien, const char* adresseIpv4, int port, sockaddr_in& adresse)
{
    if (port < 1 || port > 65535) {
        return Fail(lien, "port hors plage : %d (attendu entre 1 et 65535)", port);
    }
    adresse.sin_family = AF_INET;
    adresse.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, adresseIpv4, &adresse.sin_addr) != 1) {
        return Fail(lien, "adresse IPv4 invalide : %s (un nom de machine n'est pas accepte)",
                    adresseIpv4);
    }
    return true;
}

// La prise est prete a ecrire : la connexion n'a pas forcement reussi. Il faut
// lire l'erreur gardee par la prise (SO_ERROR). Si CETTE lecture echoue :
// echec, jamais succes.
bool WaitForConnection(TcpLink& lien, const char* adresseIpv4, double delaiMaxS)
{
    pollfd attente{};
    attente.fd = lien.descripteur;
    attente.events = POLLOUT;
    const int prets = ::poll(&attente, 1, static_cast<int>(delaiMaxS * 1000.0));
    if (prets == 0) {
        return Fail(lien, "aucune reponse de %s apres %d s", adresseIpv4,
                    static_cast<int>(delaiMaxS));
    }
    if (prets < 0) {
        return Fail(lien, "poll() a echoue : %s", std::strerror(errno));
    }

    int erreur = 0;
    socklen_t taille = sizeof(erreur);
    if (::getsockopt(lien.descripteur, SOL_SOCKET, SO_ERROR, &erreur, &taille) < 0) {
        return Fail(lien, "getsockopt(SO_ERROR) a echoue : %s", std::strerror(errno));
    }
    if (erreur != 0) {
        return Fail(lien, "connexion refusee : %s", std::strerror(erreur));
    }
    return true;
}

// Connexion NON BLOQUANTE, puis attente bornee. Un connect() ordinaire vers un
// port ou rien n'ecoute reste bloque environ deux minutes sous Windows : le
// pare-feu jette les paquets au lieu de les refuser, et le programme parait
// fige. Apres la connexion, la prise repasse en mode bloquant : ReadLine attend
// la prochaine ligne.
bool Connect(TcpLink& lien, const sockaddr_in& adresse, const char* adresseIpv4, double delaiMaxS)
{
    const int drapeaux = ::fcntl(lien.descripteur, F_GETFL, 0);
    if (drapeaux < 0) {
        return Fail(lien, "fcntl(F_GETFL) a echoue : %s", std::strerror(errno));
    }
    if (::fcntl(lien.descripteur, F_SETFL, drapeaux | O_NONBLOCK) < 0) {
        return Fail(lien, "fcntl(F_SETFL, O_NONBLOCK) a echoue : %s", std::strerror(errno));
    }

    const int resultat = ::connect(lien.descripteur,
                                   reinterpret_cast<const sockaddr*>(&adresse), sizeof(adresse));
    if (resultat < 0 && errno != EINPROGRESS) {
        return Fail(lien, "connexion impossible : %s", std::strerror(errno));
    }
    if (resultat < 0 && !WaitForConnection(lien, adresseIpv4, delaiMaxS)) {
        return false;   // WaitForConnection a deja note l'erreur et ferme le lien
    }

    if (::fcntl(lien.descripteur, F_SETFL, drapeaux) < 0) {
        return Fail(lien, "impossible de revenir en mode bloquant : %s", std::strerror(errno));
    }
    return true;
}

// Desactive l'algorithme de Nagle : il peut retenir un petit message pour
// remplir un segment. Un echec n'est pas fatal : la latence peut monter, sans
// plus. Il est quand meme note : utile pour chercher un RETARD.
void DisableNagle(TcpLink& lien)
{
    int actif = 1;
    if (::setsockopt(lien.descripteur, IPPROTO_TCP, TCP_NODELAY, &actif, sizeof(actif)) < 0) {
        std::snprintf(lien.erreur, sizeof(lien.erreur),
                      "TCP_NODELAY refuse (%s) : le lien fonctionne, la latence peut monter",
                      std::strerror(errno));
    }
}

// Une ligne complete dans le tampon ? Si oui : copiee dans `ligne`, puis
// retiree du tampon. Une ligne trop longue pour `ligne` : `ouvert` devient
// faux, et le lien est abandonne.
bool TakeLine(TcpLink& lien, char* ligne, std::size_t taille, bool& ouvert)
{
    const char* fin = static_cast<const char*>(std::memchr(lien.tampon, '\n', lien.utilises));
    if (fin == nullptr) {
        return false;
    }
    const std::size_t longueur = static_cast<std::size_t>(fin - lien.tampon);
    std::size_t copie = longueur;
    if (copie > 0 && lien.tampon[copie - 1] == '\r') {
        copie = copie - 1;   // on accepte \r\n
    }
    if (copie >= taille) {
        ouvert = Fail(lien, "ligne de %zu octets, tampon de %zu : lien abandonne", copie, taille);
        return false;
    }
    std::memcpy(ligne, lien.tampon, copie);
    ligne[copie] = '\0';

    // Decaler ce qui suit la ligne au debut du tampon.
    const std::size_t reste = lien.utilises - (longueur + 1);
    std::memmove(lien.tampon, fin + 1, reste);
    lien.utilises = reste;
    return true;
}

// Lit ce qui arrive sur la prise, a la suite du tampon. false : le lien est
// ferme. Trois cas distincts : l'autre bout a ferme (pas une panne), une vraie
// erreur (avec son message), ou EINTR (un signal pendant l'attente : on
// recommence).
bool Receive(TcpLink& lien)
{
    if (lien.utilises >= sizeof(lien.tampon)) {
        // Tampon plein, sans '\n' : l'autre bout ne respecte pas le protocole.
        return Fail(lien, "ligne demesuree, aucun saut de ligne en %zu octets", sizeof(lien.tampon));
    }
    const ssize_t recus = ::recv(lien.descripteur, lien.tampon + lien.utilises,
                                 sizeof(lien.tampon) - lien.utilises, 0);
    if (recus > 0) {
        lien.utilises = lien.utilises + static_cast<std::size_t>(recus);
        return true;
    }
    if (recus == 0) {
        return Fail(lien, "le simulateur a ferme le lien");
    }
    if (errno == EINTR) {
        return true;
    }
    return Fail(lien, "recv() a echoue : %s", std::strerror(errno));
}

}  // namespace

bool OpenLink(TcpLink& lien, const char* adresseIpv4, int port, double delaiMaxS)
{
    CloseLink(lien);
    lien.erreur[0] = '\0';
    lien.utilises = 0;

    sockaddr_in adresse{};
    if (!PrepareAddress(lien, adresseIpv4, port, adresse)) {
        return false;
    }
    lien.descripteur = ::socket(AF_INET, SOCK_STREAM, 0);
    if (lien.descripteur < 0) {
        return Fail(lien, "socket() a echoue : %s", std::strerror(errno));
    }
    if (!Connect(lien, adresse, adresseIpv4, delaiMaxS)) {
        return false;
    }
    DisableNagle(lien);
    return true;
}

void CloseLink(TcpLink& lien)
{
    if (lien.descripteur >= 0) {
        ::close(lien.descripteur);
        lien.descripteur = -1;
    }
}

bool IsLinkOpen(const TcpLink& lien)
{
    return lien.descripteur >= 0;
}

const char* LinkError(const TcpLink& lien)
{
    return lien.erreur;
}

bool ReadLine(TcpLink& lien, char* ligne, std::size_t taille)
{
    bool ouvert = lien.descripteur >= 0;
    bool trouvee = false;
    while (ouvert && !trouvee) {
        trouvee = TakeLine(lien, ligne, taille, ouvert);
        if (ouvert && !trouvee) {
            ouvert = Receive(lien);
        }
    }
    return trouvee;
}

bool WriteLine(TcpLink& lien, const char* ligne)
{
    if (lien.descripteur < 0) {
        return false;
    }
    char trame[kLongueurMaxLigne + 2];
    const int n = std::snprintf(trame, sizeof(trame), "%s\n", ligne);
    if (n < 0 || n >= static_cast<int>(sizeof(trame))) {
        std::snprintf(lien.erreur, sizeof(lien.erreur), "ligne trop longue a emettre");
        return false;
    }

    // MSG_NOSIGNAL : SANS LUI, ecrire dans un lien que le simulateur vient de
    // fermer envoie SIGPIPE, et le processus meurt sans message. Avec lui, send
    // retourne -1 et errno vaut EPIPE : une erreur ordinaire, avec un message.
    const std::size_t total = static_cast<std::size_t>(n);
    std::size_t envoyes = 0;
    bool ouvert = true;
    while (ouvert && envoyes < total) {
        const ssize_t ecrits = ::send(lien.descripteur, trame + envoyes, total - envoyes, MSG_NOSIGNAL);
        if (ecrits > 0) {
            envoyes = envoyes + static_cast<std::size_t>(ecrits);
        } else if (ecrits == 0) {
            ouvert = Fail(lien, "le simulateur a ferme le lien");
        } else if (errno != EINTR) {
            ouvert = Fail(lien, "send() a echoue : %s", std::strerror(errno));
        }
        // ecrits < 0 et EINTR : un signal a interrompu l'envoi; on recommence.
    }
    return ouvert;
}

int SplitLine(char* ligne, const char* jetons[], int max)
{
    int nb = 0;
    char* p = ligne;
    while (*p != '\0' && nb < max) {
        while (*p == ' ') {
            p = p + 1;
        }
        if (*p != '\0') {
            jetons[nb] = p;
            nb = nb + 1;
            while (*p != '\0' && *p != ' ') {
                p = p + 1;
            }
            if (*p == ' ') {
                *p = '\0';   // le jeton se termine ICI, dans la ligne d'origine
                p = p + 1;
            }
        }
    }
    return nb;
}

// std::from_chars et std::to_chars lisent et ecrivent TOUJOURS le point
// decimal, peu importe la langue du systeme : le protocole l'exige. D'ou ni
// sscanf ni atof.
//
// Les deux fonctions lisent dans une variable TEMPORAIRE, et changent la
// sortie seulement en cas de succes. Sinon, "123abc" serait rejete, mais 123
// serait deja dans votre variable.

bool ParseInt(const char* jeton, int& valeur)
{
    const char* fin = jeton + std::strlen(jeton);
    int lu = 0;
    const auto resultat = std::from_chars(jeton, fin, lu);
    if (resultat.ec != std::errc{} || resultat.ptr != fin) {
        return false;
    }
    valeur = lu;
    return true;
}

bool ParseDouble(const char* jeton, double& valeur)
{
    const char* fin = jeton + std::strlen(jeton);
    double lu = 0.0;
    const auto resultat = std::from_chars(jeton, fin, lu);
    if (resultat.ec != std::errc{} || resultat.ptr != fin) {
        return false;
    }
    // from_chars accepte "nan" et "inf" : refuses ici, comme le simulateur
    // les refuse de son cote.
    if (!std::isfinite(lu)) {
        return false;
    }
    valeur = lu;
    return true;
}

void FormatDouble(char* tampon, std::size_t taille, double valeur)
{
    if (taille == 0) {
        return;
    }
    const auto resultat = std::to_chars(tampon, tampon + taille - 1, valeur,
                                        std::chars_format::fixed, 6);
    if (resultat.ec == std::errc{}) {
        *resultat.ptr = '\0';
    } else {
        tampon[0] = '\0';
    }
}

}  // namespace conduite
