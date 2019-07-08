#ifndef REGISTER_HPP
#define REGISTER_HPP 1

class Register {
private:
    unsigned storage;
    bool zero;
public:
    Register(): zero(false) {}
    unsigned read() {
        return zero ? 0 : storage;
    }
    void write(unsigned data) {
        storage = data;
    }
    void set_zero() {
        zero = true;
    }
};

#endif