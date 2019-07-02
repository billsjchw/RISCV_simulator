#include <iostream>
#include <fstream>
using namespace std;

const int BYTE_LEN = 8;
const unsigned END_ADDR = 0x30004;

class Memory {
private:
    static const int CAPACITY = 0x3FFFF;
    unsigned char storage[CAPACITY];
public:
    initialize() {
    }
    unsigned read_dword(unsigned addr) {
        unsigned ret;
        for (int i = 0; i < 4; ++i)
            ret = ret << BYTE_LEN + storage[addr + i];
        return ret;
    }
};

class Register {
private:
    unsigned storage;
public:
    void load(unsigned value) {
        storage = value;
    }
    unsigned read() {
        return storage;
    }
};

const int REGISTER_NUM = 32;

Memory mem;
Register reg[REGISTER_NUM];
Register pc;

class Inst {
public:
    virtual void exec() {};
    virtual void mem_access() {};
    virtual void write_back() {};
    static Inst * parser(unsigned code);
};

Inst * ctrl_inst;
bool prog_end;
unsigned timer;
unsigned ret_val;

unsigned IFID_code;
unsigned IFID_next_seq_pc;
unsigned IDEX_next_seq_pc;
unsigned EXMEM_next_seq_pc;
unsigned IDEX_rval1, IDEX_rval2;
unsigned EXMEM_data;
unsigned MEMWB_data;

class RTypeInst: public Inst {
private:
    int src1, src2, dest;
public:
    static const int OPCODE = 0x33;
    static Inst * parser(unsigned code);
    void set(int src1, int src2, int dest) {
        this->src1 = src1;
        this->src2 = src2;
        this->dest = dest;
    }
};

class ITypeInst: public Inst {
private:
    unsigned imm;
    int src, dest, shamt;
public:
    static const int OPCODE1 = 0x67;
    static const int OPCODE2 = 0x3;
    static const int OPCODE3 = 0x13;
    static Inst * parser(unsigned code);
    void set(unsigned imm, int src, int dest, int shamt) {
        this->imm = imm;
        this->src = src;
        this->dest = dest;
        this->shamt = shamt;
    }
};

class STypeInst: public Inst {
private:
    unsigned imm;
    int src1, src2;
public:
    static const int OPCODE = 0x23;
    static Inst * parser(unsigned code);
    void set(unsigned imm, int src1, int src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class BTypeInst: public Inst {
private:
    unsigned imm;
    int src1, src2;
public:
    static const int OPCODE = 0x63;
    static Inst * parser(unsigned code);
    void set(unsigned imm, int src1, int src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class UTypeInst: public Inst {
private:
    unsigned imm;
    int dest;
public:
    static unsigned char OPCODE1 = 0x37;
    static unsigned char OPCODE2 = 0x17;
    static Inst * parser(unsigned code);
    void set(unsigned imm, int dest) {
        this->imm = imm;
        this->dest = dest;
    }
};

Inst * Inst::parser(unsigned code) {
    opcode = code & 0x7F;
    if (opcode == RTypeInst::OPCODE)
        return RTypeInst::parser(code);
    else if (opcode == ITypeInst::OPCODE1 || opcode == ITypeInst::OPCODE2 ||
        opcode == ITypeInst::OPCODE3)
        return ITypeInst::parser(code);
    else if (opcode == STypeInst::OPCODE)
        return STypeInst::parser(code);
    else if (opcode == BTypeInst::OPCODE)
        return BTypeInst::parser(code);
    else if (opcode == UTypeInst::OPCODE1 || opcode == UTypeInst::OPCODE2)
        return UTypeInst::parser(code);
}

class InstFetch {
public:
    void work() {
        unsigned addr = pc.read();
        IFID_code = mem.read_dword(addr);
        IFID_next_seq_pc = addr + 4;
    }
};

class InstDecode {
public:
    void work() {
        IDEX_next_seq_pc = IFID_next_seq_pc;
    }
};

class Execute {
public:
    void work(Inst * inst) {
        inst->exec();
        EXMEM_next_seq_pc = IDEX_next_seq_pc;
    }
};

class MemoryAccess {
public:
    void work(Inst * inst) {
        inst->mem_access();
    }
};

class WriteBack {
public:
    void work(Inst * inst) {
        inst->write_back();
    }
};

InstFetch inst_fetch;
InstDecode inst_decode;
Execute execute;
MemoryAccess mem_access;
WriteBack write_back;

int main() {
    pc.load(0);
    mem.initialize();
    prog_end = false;
    timer = 0;
    ctrl_inst = NULL;

    while (!prog_end) {
        switch (timer) {
            case 0: inst_fetch.work(); break;
            case 1: inst_decode.work(); break;
            case 2: execute.work(ctrl_inst); break;
            case 3: mem_access.work(ctrl_inst); break;
            case 4: write_back.work(ctrl_inst); break;
            default: break;
        }
        timer = (timer + 1) % 5;
    }

    return ret_val;
}