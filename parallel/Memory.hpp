#ifndef MEMORY_HPP
#define MEMORY_HPP 1

#include <iostream>

class Memory {
private:
    unsigned char storage[0x400000];
public:
    void init() {
        unsigned addr = 0;
        while (!std::cin.eof()) {
            unsigned byte;
            while (std::cin >> std::hex >> byte) {
                storage[addr] = byte;
                ++addr;
            }
            std::cin.clear();
            std::cin.get();
            std::cin >> addr;
        }
    }
    int read_dword(unsigned addr) {
        int ret;
        for (int i = 3; i >= 0; --i)
            ret = (ret << 8) + storage[addr + i];
        return ret;
    }
    unsigned read_word(unsigned addr) {
        unsigned ret;
        for (int i = 1; i >= 0; --i)
            ret = (ret << 8) + storage[addr + i];
        return ret;
    }
    unsigned read(unsigned addr) {
        return storage[addr];
    }
    void write_dword(unsigned addr, unsigned data) {
        for (int i = 0; i < 4; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void write_word(unsigned addr, unsigned data) {
        for (int i = 0; i < 2; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void write(unsigned addr, unsigned data) {
        storage[addr] = data;
    }
};

#endif