// Keyboard.h : le clavier en mode direct. Les touches arrivent une à une, sans
// Entrée et sans écho; le terminal est remis dans son état au destructeur.
#pragma once

#include <termios.h>

class Keyboard {
public:
    Keyboard();
    ~Keyboard();

    // Un clavier ne se copie pas : il n'y a qu'un terminal à remettre dans son état.
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    bool kbhit(int attenteMs = 0);   // true si un caractère attend (au plus attenteMs)
    char getch();                    // lit un caractère; 0 si rien à lire

private:
    struct termios reglagesInitiaux;
    bool modeDirect = false;
};
