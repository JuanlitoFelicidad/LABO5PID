#include "Keyboard.h"

#include <sys/select.h>
#include <unistd.h>

Keyboard::Keyboard() {
    if (tcgetattr(STDIN_FILENO, &reglagesInitiaux) == 0) {
        struct termios reglagesDirects = reglagesInitiaux;

        reglagesDirects.c_lflag = reglagesDirects.c_lflag & ~(ICANON | ECHO);
        reglagesDirects.c_cc[VMIN] = 0;
        reglagesDirects.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &reglagesDirects);
        modeDirect = true;
    }
}

Keyboard::~Keyboard() {
    if (modeDirect) {
        tcsetattr(STDIN_FILENO, TCSANOW, &reglagesInitiaux);
    }
}

bool Keyboard::kbhit(int attenteMs) {
    fd_set descripteurs;
    struct timeval attente;

    FD_ZERO(&descripteurs);
    FD_SET(STDIN_FILENO, &descripteurs);
    attente.tv_sec = attenteMs / 1000;
    attente.tv_usec = (attenteMs % 1000) * 1000;

    return select(STDIN_FILENO + 1, &descripteurs, NULL, NULL, &attente) > 0;
}

char Keyboard::getch() {
    char caractere = 0;

    if (read(STDIN_FILENO, &caractere, 1) < 1) {
        caractere = 0;
    }
    return caractere;
}
