#include <iostream>
#include <cstdio>
#include <typeinfo>
using namespace std;

class Memory {
private:
    unsigned char storage[0x400000];
public:
    void init() {
        unsigned addr = 0;
        while (!cin.eof()) {
            unsigned byte;
            while (cin >> hex >> byte) {
                storage[addr] = byte;
                ++addr;
            }
            cin.clear();
            cin.get();
            cin >> addr;
        }
    }
    int load_dword(unsigned addr) {
        int ret;
        for (int i = 3; i >= 0; --i)
            ret = (ret << 8) + storage[addr + i];
        return ret;
    }
    unsigned load_word(unsigned addr) {
        unsigned ret;
        for (int i = 1; i >= 0; --i)
            ret = (ret << 8) + storage[addr + i];
        return ret;
    }
    unsigned load(unsigned addr) {
        return storage[addr];
    }
    void store_dword(unsigned addr, unsigned data) {
        for (int i = 0; i < 4; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void store_word(unsigned addr, unsigned data) {
        for (int i = 0; i < 2; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void store(unsigned addr, unsigned data) {
        storage[addr] = data;
    }
};

class Register {
protected:
    unsigned storage;
    bool zero;
public:
    Register(): zero(false) {}
    void store(unsigned value) {
        if (!zero)
            storage = value;
    }
    virtual unsigned load() {
        return storage;
    }
    void set_zero() {
        storage = 0;
        zero = true;
    }
};

Memory mem;
Register reg[32];
Register pc;
Register IFID_addr;
Register IDEX_rval1, IDEX_rval2, IDEX_addr;
Register EXMEM_data, EXMEM_addr;
Register MEMWB;

class Inst {
public:
    virtual void inst_fetch() {
        pc.store(pc.load() + 4);
    }
    virtual void inst_decode() {}
    virtual void exec() {}
    virtual void mem_access() {}
    virtual void write_back() {}
    static Inst * parse(unsigned code);
};

Inst * inst;
bool ret_flag;
unsigned timer;
unsigned ret_val;

class RTypeInst: public Inst {
protected:
    unsigned src1, src2, dest;
public:
    static Inst * parse(unsigned code);
    void inst_decode() {
        IDEX_rval1.store(reg[src1].load());
        IDEX_rval2.store(reg[src2].load());
    }
private:
    void set(unsigned src1, unsigned src2, unsigned dest) {
        this->src1 = src1;
        this->src2 = src2;
        this->dest = dest;
    }
};

class ITypeInst: public Inst {
protected:
    unsigned imm;
    unsigned src, dest, shamt;
public:
    static Inst * parse(unsigned code);
    void inst_decode() {
        IDEX_rval1.store(reg[src].load());
    }
private:
    void set(unsigned imm, unsigned src, unsigned dest, unsigned shamt) {
        this->imm = imm;
        this->src = src;
        this->dest = dest;
        this->shamt = shamt;
    }
};

class STypeInst: public Inst {
protected:
    unsigned imm;
    unsigned src1, src2;
public:
    static Inst * parse(unsigned code);
    void inst_decode() {
        IDEX_rval1.store(reg[src1].load());
        IDEX_rval2.store(reg[src2].load());
    }
private:
    void set(unsigned imm, unsigned src1, unsigned src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class BTypeInst: public Inst {
protected:
    unsigned imm;
    unsigned src1, src2;
public:
    static Inst * parse(unsigned code);
    void inst_decode() {
        IDEX_rval1.store(reg[src1].load());
        IDEX_rval2.store(reg[src2].load());
    }
private:
    void set(unsigned imm, unsigned src1, unsigned src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
};

class UTypeInst: public Inst {
protected:
    unsigned imm;
    unsigned dest;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, unsigned dest) {
        this->imm = imm;
        this->dest = dest;
    }
};

class JTypeInst: public Inst {
protected:
    unsigned imm;
    unsigned dest;
public:
    static Inst * parse(unsigned code);
private:
    void set(unsigned imm, unsigned dest) {
        this->imm = imm;
        this->dest = dest;
    }
};

Inst * Inst::parse(unsigned code) {
    unsigned opcode = code & 0x7F;
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

unsigned sgnext(unsigned imm, int hi) {
    if (imm & (1 << hi))
        imm |= 0xFFFFFFFF >> hi << hi;
    return imm;
}

class LUI: public UTypeInst {
public:
    void write_back() {
        reg[dest].store(imm);
    }
};

class AUIPC: public UTypeInst {
public:
    void exec() {
        EXMEM_addr.store(IDEX_addr.load() + imm);
    }
    void mem_access() {
        MEMWB.store(EXMEM_addr.load());
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class JAL: public JTypeInst {
public:
    void inst_fetch() {
        pc.store(pc.load() + imm);
    }
    void exec() {
        EXMEM_data.store(IDEX_addr.load() + 4);
    }
    void mem_access() {
        MEMWB.store(EXMEM_data.load());
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class JALR: public ITypeInst {
public:
    void exec() {
        pc.store((IDEX_rval1.load() + imm) & 0xFFFFFFFE);
        EXMEM_data.store(IDEX_addr.load() + 4);
    }
    void mem_access() {
        MEMWB.store(EXMEM_data.load());
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class BEQ: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1.load() == IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class BNE: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1.load() != IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class BLT: public BTypeInst {
public:
    void exec() {
        if ((int) IDEX_rval1.load() < (int) IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class BGE: public BTypeInst {
public:
    void exec() {
        if ((int) IDEX_rval1.load() >= (int) IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class BLTU: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1.load() < IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class BGEU: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1.load() >= IDEX_rval2.load())
            pc.store(IDEX_addr.load() + imm);
    }
};

class storeInst: public ITypeInst {
public:
    void exec() {
        EXMEM_addr.store(IDEX_rval1.load() + imm);
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class LB: public storeInst {
public:
    void mem_access() {
        MEMWB.store(sgnext(mem.load(EXMEM_addr.load()), 7));
    }
};

class LH: public storeInst {
public:
    void mem_access() {
        MEMWB.store(sgnext(mem.load_word(EXMEM_addr.load()), 15));
    }
};

class LW: public storeInst {
public:
    void mem_access() {
        MEMWB.store(mem.load_dword(EXMEM_addr.load()));
    }
};

class LBU: public storeInst {
public:
    void mem_access() {
        MEMWB.store(mem.load(EXMEM_addr.load()));
    }
};

class LHU: public storeInst {
public:
    void mem_access() {
        MEMWB.store(mem.load_word(EXMEM_addr.load()));
    }
};

class StoreInst: public STypeInst {
public:
    void exec() {
        EXMEM_addr.store(IDEX_rval1.load() + imm);
        EXMEM_data.store(IDEX_rval2.load());
    }
};

class SB: public StoreInst {
public:
    void exec() {
        EXMEM_addr.store(IDEX_rval1.load() + imm);
        EXMEM_data.store(IDEX_rval2.load());
        if (EXMEM_addr.load() == 0x30004) {
            ret_val = reg[10].load() & 0xFF;
            ret_flag = true;
        }
    }
    void mem_access() {
        mem.store(EXMEM_addr.load(), EXMEM_data.load());
    }
};

class SH: public StoreInst {
public:
    void mem_access() {
        mem.store_word(EXMEM_addr.load(), EXMEM_data.load());
    }
};

class SW: public StoreInst {
public:
    void mem_access() {
        mem.store_dword(EXMEM_addr.load(), EXMEM_data.load());
    }
};

class ITypeCalcInst: public ITypeInst {
public:
    void mem_access() {
        MEMWB.store(EXMEM_data.load());
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class ADDI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() + imm);
    }
};

class SLTI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store((int) IDEX_rval1.load() < (int) imm);
    }
};

class SLTIU: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() < imm);
    }
};

class XORI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() ^ imm);
    }
};

class ORI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() | imm);
    }
};

class ANDI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() & imm);
    }
};

class SLLI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() << shamt);
    }
};

class SRLI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() >> shamt);
    }
};

class SRAI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data.store((int) IDEX_rval1.load() >> shamt);
    }
};

class RTypeCalcInst: public RTypeInst {
public:
    void mem_access() {
        MEMWB.store(EXMEM_data.load());
    }
    void write_back() {
        reg[dest].store(MEMWB.load());
    }
};

class ADD: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() + IDEX_rval2.load());
    }
};

class SUB: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() - IDEX_rval2.load());
    }
};

class SLL: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() << (IDEX_rval2.load() & 0x1F));
    }
};

class SLT: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store((int) IDEX_rval1.load() < (int) IDEX_rval2.load());
    }
};

class SLTU: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() < IDEX_rval2.load());
    }
};

class XOR: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() ^ IDEX_rval2.load());
    }
};

class SRL: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() >> (IDEX_rval2.load() & 0x1F));
    }
};

class SRA: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store((int) IDEX_rval1.load() >> (IDEX_rval2.load() & 0x1F));
    }
};

class OR: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() | IDEX_rval2.load());
    }
};

class AND: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data.store(IDEX_rval1.load() & IDEX_rval2.load());
    }
};

Inst * RTypeInst::parse(unsigned code) {
    RTypeInst * ret;
    unsigned funct3 = code >> 12 & 0x7;
    unsigned funct7 = code >> 25;
    if (funct3 == 0 && funct7 == 0)
        ret = new ADD;
    else if (funct3 == 0 && funct7 == 0x20)
        ret = new SUB;
    else if (funct3 == 0x1)
        ret = new SLL;
    else if (funct3 == 0x2)
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
    unsigned src1 = code >> 15 & 0x1F;
    unsigned src2 = code >> 20 & 0x1F;
    unsigned dest = code >> 7 & 0x1F;
    ret->set(src1, src2, dest);
    return (Inst *) ret;
}

Inst * ITypeInst::parse(unsigned code) {
    ITypeInst * ret;
    unsigned opcode = code & 0x7F;
    unsigned funct3 = code >> 12 & 0x7;
    unsigned funct7 = code >> 25;
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
    unsigned imm = sgnext(code >> 20, 11);
    unsigned src = code >> 15 & 0x1F;
    unsigned dest = code >> 7 & 0x1F;
    unsigned shamt = imm & 0x1F;
    ret->set(imm, src, dest, shamt);
    return (Inst *) ret;
}

Inst * STypeInst::parse(unsigned code) {
    STypeInst * ret;
    unsigned funct3 = code >> 12 & 0x7;
    if (funct3 == 0)
        ret = new SB;
    else if (funct3 == 0x1)
        ret = new SH;
    else if (funct3 == 0x2)
        ret = new SW;
    unsigned imm = sgnext((code >> 7 & 0x1F) + ((code >> 25 & 0x7F) << 5), 11);
    unsigned src1 = code >> 15 & 0x1F;
    unsigned src2 = code >> 20 & 0x1F;
    ret->set(imm, src1, src2);
    return (Inst *) ret;
}

Inst * BTypeInst::parse(unsigned code) {
    BTypeInst * ret;
    unsigned funct3 = code >> 12 & 0x7;
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
    unsigned imm = sgnext(((code >> 8 & 0xF) << 1) + ((code >> 25 & 0x3F) << 5) +
        ((code >> 7 & 0x1) << 11) + ((code >> 31 & 1) << 12), 12);
    unsigned src1 = code >> 15 & 0x1F;
    unsigned src2 = code >> 20 & 0x1F;
    ret->set(imm, src1, src2);
    return (Inst *) ret;
}

Inst * UTypeInst::parse(unsigned code) {
    UTypeInst * ret;
    unsigned opcode = code & 0x7F;
    if (opcode == 0x37)
        ret = new LUI;
    else if (opcode == 0x17)
        ret = new AUIPC;
    unsigned imm = code & 0xFFFFF000;
    unsigned dest = code >> 7 & 0x1F;
    ret->set(imm, dest);
    return (Inst *) ret;
}

Inst * JTypeInst::parse(unsigned code) {
    JTypeInst * ret = new JAL;
    unsigned imm = sgnext(((code >> 21 & 0x3FF) << 1) + ((code >> 20 & 0x1) << 11) +
        ((code >> 12 & 0xFF) << 12) + ((code >> 31 & 0x1) << 20), 20);
    unsigned dest = code >> 7 & 0x1F;
    ret->set(imm, dest);
    return (Inst *) ret;
}


class InstFetch {
public:
    void work() {
        unsigned addr = pc.load();
        delete inst;
        inst = Inst::parse(mem.load_dword(addr));
        IFID_addr.store(addr);
        inst->inst_fetch();
    }
};

class InstDecode {
public:
    void work(Inst * inst) {
        IDEX_addr.store(IFID_addr.load());
        inst->inst_decode();
    }
};

class Execute {
public:
    void work(Inst * inst) {
        inst->exec();
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
    freopen("test.data", "r", stdin);
    pc.store(0);
    mem.init();
    reg[0].set_zero();
    ret_flag = false;
    timer = 0;
    inst = NULL;

    while (!ret_flag) {
        switch (timer) {
            case 0: inst_fetch.work(); break;
            case 1: inst_decode.work(inst); break;
            case 2: execute.work(inst); break;
            case 3: mem_access.work(inst); break;
            case 4: write_back.work(inst); break;
            default: break;
        }
        timer = (timer + 1) % 5;
    }

    cout << ret_val << endl;
    return 0;
}