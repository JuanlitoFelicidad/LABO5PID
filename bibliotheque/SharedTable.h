#ifndef SHAREDTABLE_H
#define SHAREDTABLE_H

#include "Mutex.h"

class SharedTable {
private:
    unsigned char tableau[100];
    Mutex mutex;
    bool protectionActive;

public:
    SharedTable(bool protectionActive);

    void fill(unsigned char valeur);
    bool verify();
    void setProtection(bool active);
};

#endif