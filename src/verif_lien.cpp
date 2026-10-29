// =============================================================================
//  verif_lien : la preuve que la chaine fonctionne. FOURNI, complet.
//
//  Se connecte au simulateur et affiche TEL QUEL tout ce qu'il envoie. Ne
//  repond RIEN : la voiture ne recoit aucune commande.
//    - Des lignes MESURE defilent : le reseau, le pare-feu et le simulateur
//      fonctionnent. Le probleme est dans le programme.
//    - Rien ne defile : le probleme est dans le lien (adresse, reseau,
//      pare-feu, simulateur).
//
//      ./build/verif_lien ADRESSE 5000
//
//  L'adresse est celle que le simulateur affiche. Ctrl+C pour quitter.
// =============================================================================

#include <unistd.h>

#include <iostream>

#include "TcpLink.h"

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

    std::cout << std::unitbuf;   // chaque ligne parait tout de suite, meme redirigee

    conduite::TcpLink lien;
    std::cout << "Recherche du simulateur sur " << hote << ":" << port << "\n";
    int essai = 1;
    while (!conduite::OpenLink(lien, hote, port)) {
        std::cout << "  essai " << essai << " : " << conduite::LinkError(lien)
                  << ".  Nouvelle tentative dans 2 s. (Ctrl+C pour abandonner)\n";
        ::sleep(2);
        essai = essai + 1;
    }
    std::cout << "Connecte. Tout ce qui suit vient du simulateur, tel quel :\n\n";

    char ligne[conduite::kLongueurMaxLigne];
    while (conduite::ReadLine(lien, ligne, sizeof(ligne))) {
        std::cout << ligne << "\n";
    }

    std::cout << "\nLe lien s'est ferme : " << conduite::LinkError(lien) << "\n";
    conduite::CloseLink(lien);
    return 0;
}
