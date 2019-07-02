#include <fstream>
using namespace std;

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
            ret = ret << 8 + storage[addr + i];
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
    static Inst * parse(unsigned code);
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
protected:
    int src1, src2, dest;
public:
    static Inst * parse(unsigned code);
private:
    void set(int src1, int src2, int dest) {
        this->src1 = src1;
        this->src2 = src2;
        this->dest = dest;
    }
};

class ITypeInst: public Inst {
protected:
    unsigned imm;
    int src, dest, shamt;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, int src, int dest, int shamt) {
        this->imm = imm;
        this->src = src;
        this->dest = dest;
        this->shamt = shamt;
    }
};

class STypeInst: public Inst {
protected:
    unsigned imm;
    int src1, src2;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, int src1, int src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class BTypeInst: public Inst {
protected:
    unsigned imm;
    int src1, src2;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, int src1, int src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class UTypeInst: public Inst {
protected:
    unsigned imm;
    int dest;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, int dest) {
        this->imm = imm;
        this->dest = dest;
    }
};

class JTypeInst: public Inst {
protected:
    unsigned imm;
    int dest;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, int dest) {
        this->imm = imm;
        this->dest = dest;
    }
};

Inst * Inst::parse(unsigned code) {
    unsigned char opcode = code & 0x7F;
    if (opcode == 0x33)
        return RTypeInst::parse(code);
    else if (opcode == 0x67 || opcode == 0x3 || opcode == 0x13)
        return ITypeInst::parse(code);
    else if (opcode == 0x23)
        return STypeInst::parse(code);
    else if (opcode == 0x63)
        return BTypeInst::parse(code);
    else if (opcode == 0x37 || opcode == 0x17)
        return UTypeInst::parse(code);
    else if (opcode = 0x6F)
        return JTypeInst::parse(code);
}

class LUI: public UTypeInst {
};

class AUIPC: public UTypeInst {
};

class JAL: public JTypeInst {
};

class JALR: public ITypeInst {
};

class BEQ: public BTypeInst {
};

class BNE: public BTypeInst {
};

class BLT: public BTypeInst {
};

class BGE: public BTypeInst {
};

class BLTU: public BTypeInst {
};

class BGEU: public BTypeInst {
};

class LB: public ITypeInst {
};

class LH: public ITypeInst {
};

class LW: public ITypeInst {
};

class LBU: public ITypeInst {
};

class LHU: public ITypeInst {
};

class SB: public STypeInst {
};

class SH: public STypeInst {
};

class SW: public STypeInst {
};

class ADDI: public ITypeInst {
};

class SLTI: public ITypeInst {
};

class SLTIU: public ITypeInst {
};

class XORI: public ITypeInst {
};

class ORI: public ITypeInst {
};

class ANDI: public ITypeInst {
};

class SLLI: public ITypeInst {
};

class SRLI: public ITypeInst {
};

class SRAI: public ITypeInst {
};

class ADD: public RTypeInst {
};

class SUB: public RTypeInst {
};

class SLL: public RTypeInst {
};

class SLT: public RTypeInst {
};

class SLTU: public RTypeInst {
};

class XOR: public RTypeInst {
};

class SRL: public RTypeInst {
};

class SRA: public RTypeInst {
};

class OR: public RTypeInst {
};

class AND: public RTypeInst {
};

Inst * RTypeInst::parse(unsigned code) {
    RTypeInst * ret;
    unsigned char funct3 = code >> 12 & 0x7;
    unsigned char funct7 = code >> 25;
    if (funct3 == 0 && funct7 == 0)
        ret = new ADD;
    else if (funct3 == 0 && funct7 = 0x20)
        ret = new SUB;
    else if (funct3 == 0x1)
        ret = new SLL;
    else if (funct3 === 0x2)
        ret = new SLT;
    else if (funct3 == 0x3)
        ret = new SLTU;
    else if (funct3 == 0x4)
        ret = new XOR;
    else if (funct3 == 0x5 && funct7 == 0)
        ret = new SRL;
    else if (funct3 == 0x5 && funct7 == 0x20)
        ret = new SRA;
    else if (funct3 == 0x6)
        ret = new OR;
    else if (funct3 == 0x7)
        ret = new AND;
    int src1 = code >> 15 & 0x1F;
    int src2 = code >> 20 & 0x1F;
    int dest = code >> 7 & 0x1F;
    ret->set(src1, src2, dest);
    return (Inst *) ret;
}

Inst * ITypeInst::parse(unsigned code) {
    ITypeInst * ret;
    unsigned char opcode = code & 0x7F;
    unsigned char funct3 = code >> 12 & 0x7;
    unsigned char funct7 = code >> 25;
    if (opcode == 0x67)
        ret = new JALR;
    else if (opcode == 0x3) {
        if (funct3 == 0)
            ret = new LB;
        else if (funct3 == 0x1)
            ret = new LH;
        else if (funct3 == 0x2)
            ret = new LW;
        else if (funct3 == 0x4)
            ret = new LBU;
        else if (funct3 == 0x5)
            ret = new LHU;
    } else if (opcode == 0x13) {
        if (funct3 == 0)
            ret = new ADDI;
        else if (funct3 == 0x2)
            ret = new SLTI;
        else if (funct3 == 0x3)
            ret = new SLTIU;
        else if (funct3 == 0x4)
            ret = new XORI;
        else if (funct3 == 0x6)
            ret = new ORI;
        else if (funct3 == 0x7)
            ret = new ANDI;
        else if (funct3 == 0x1)
            ret = new SLLI;
        else if (funct3 == 0x5 && funct7 == 0)
            ret = new SRLI;
        else if (funct3 == 0x5 && funct7 == 0x20)
            ret = new SRAI;
    }
    unsigned imm = code >> 20;
    int src = code >> 15 & 0x1F;
    int dest = code >> 7 & 0x1F;
    unsigned shamt = imm & 0x1F;
    ret->set(imm, src, dest, shamt);
    return (Inst *) ret;
}

Inst * STypeInst::parse(unsigned code) {
    STypeInst * ret;
    unsigned funct3 = code >> 12 & 0x7;
    if (funct3 == 0)
        ret = new SB;
    else if (funct3 = 0x1)
        ret = new SH;
    else if (funct3 == 0x2)
        ret = new SW;
    unsigned imm = code >> 7 & 0x1F + code >> 25 & 0x7F << 5;
    int src1 = code >> 15 & 0x1F;
    int src2 = code >> 20 & 0x1F;
    ret->set(imm, src1, src2);
    return (Inst *) ret;
}

Inst * BTypeInst::parse(unsigned code) {
    BTypeInst * ret;
    unsigned char funct3 = code >> 12 & 0x7;
    if (funct3 == 0)
        ret = new BEQ;
    else if (funct3 == 0x1)
        ret = new BNE;
    else if (funct3 == 0x4)
        ret = new BLT;
    else if (funct3 == 0x5)
        ret = new BGE;
    else if (funct3 == 0x6)
        ret = new BLTU;
    else if (funct3 == 0x7)
        ret = new BGEU;
    unsigned imm = code >> 8 & 0xF << 1 + code >> 25 & 0x3F << 5 +
        code >> 7 & 0x1 << 11 + code >> 31 & 1 << 12;
    int src1 = code >> 15 & 0x1F;
    int src2 = code >> 20 & 0x1F;
    return (Inst *) ret;
}

Inst * UTypeInst::parse(unsigned code) {
    UTypeInst * ret;
    unsigned char opcode = code & 0x7F;
    if (opcode == 0x37)
        ret = new LUI;
    else if (opcode == 0x17)
        ret = new AUIPC;
    unsigned imm = code & 0xFFFFF000;
    int dest = code >> 7 & 0x1F;
    ret->set(imm, dest);
    return (Inst *) ret;
}

Inst * JTypeInst::parse(unsigned code) {
    JTypeInst * ret = new JALR;
    unsigned imm = code >> 21 & 0x3FF << 1 + code >> 20 & 0x1 << 11 +
        code >> 12 & 0xFF << 12 + code >> 31 & 0x1 << 20;
    int dest = code >> 7 & 0x1F;
    ret->set(imm, dest);
    return (Inst *) ret;
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