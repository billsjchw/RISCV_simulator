#ifndef PREDICTOR_HPP
#define PREDICTOR_HPP 1

#include <cstring>

class Predictor {
private:
    unsigned hist;
    int tab[4];
public:
    Predictor() {
        std::memset(tab, 0, sizeof(tab));
    }
    bool predict() {
        if (tab[hist & 0x3] < 2)
            return false;
        else
            return true;
    }
    void update(bool taken) {
        if (!taken && tab[hist & 0x3] != 0)
            --tab[hist & 0x3];
        if (taken && tab[hist & 0x3] != 3)
            ++tab[hist & 0x3];
        hist <<= 1;
        hist &= taken;
    }
};

#endif