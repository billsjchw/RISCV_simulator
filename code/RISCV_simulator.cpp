#include <iostream>
using namespace std;

const unsigned END_ADDR = 0x30004;

class Memory {
private:
    static const unsigned CAPACITY = 0x3FFFF;
    unsigned char storage[CAPACITY];
public:
    void initialize() {
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
    void load_dword(unsigned addr, unsigned data) {
        for (int i = 0; i < 4; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void load_word(unsigned addr, unsigned data) {
        for (int i = 0; i < 2; ++i) {
            storage[addr + i] = data;
            data >>= 8;
        }
    }
    void load(unsigned addr, unsigned data) {
        storage[addr] = data;
    }
};

class Register {
private:
    unsigned storage;
    bool read_only;
public:
    Register(): read_only(false) {}
    void load(unsigned value) {
        if (!read_only)
            storage = value;
    }
    unsigned read() {
        return storage;
    }
    void set_read_only(bool flag) {
        read_only = flag;
    }
};

const unsigned REGISTER_NUM = 32;

Memory mem;
Register reg[REGISTER_NUM];
Register pc;

class Inst {
public:
    virtual void inst_fetch() {
        pc.load(pc.read() + 4);
    }
    virtual void inst_decode() {}
    virtual void exec() {}
    virtual void mem_access() {}
    virtual void write_back() {}
    static Inst * parse(unsigned code);
};

Inst * inst;
bool prog_end;
unsigned timer;
unsigned ret_val;

unsigned IFID_inst_addr;
unsigned IDEX_inst_addr;
unsigned IDEX_rval1, IDEX_rval2;
unsigned EXMEM_data;
unsigned EXMEM_addr;
unsigned MEMWB_data;

class RTypeInst: public Inst {
protected:
    unsigned src1, src2, dest;
public:
    static Inst * parse(unsigned code);
    void inst_decode() {
        IDEX_rval1 = reg[src1].read();
        IDEX_rval2 = reg[src2].read();
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
        IDEX_rval1 = reg[src].read();
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
        IDEX_rval1 = reg[src1].read();
        IDEX_rval2 = reg[src2].read();
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
        IDEX_rval1 = reg[src1].read();
        IDEX_rval2 = reg[src2].read();
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
        reg[dest].load(imm);
    }
};

class AUIPC: public UTypeInst {
public:
    void exec() {
        EXMEM_data = IDEX_inst_addr + imm;
    }
    void mem_access() {
        MEMWB_data = EXMEM_data;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class JAL: public JTypeInst {
public:
    void inst_fetch() {
        pc.load(pc.read() + imm);
    }
    void exec() {
        EXMEM_data = IDEX_inst_addr + 4;
    }
    void mem_access() {
        MEMWB_data = EXMEM_data;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class JALR: public ITypeInst {
public:
    void exec() {
        pc.load((IDEX_rval1 + imm) & 0xFFFFFFFE);
        EXMEM_data = IDEX_inst_addr + 4;
    }
    void mem_access() {
        MEMWB_data = EXMEM_data;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class BEQ: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1 == IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class BNE: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1 != IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class BLT: public BTypeInst {
public:
    void exec() {
        if ((int) IDEX_rval1 < (int) IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class BGE: public BTypeInst {
public:
    void exec() {
        if ((int) IDEX_rval1 >= (int) IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class BLTU: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1 < IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class BGEU: public BTypeInst {
public:
    void exec() {
        if (IDEX_rval1 >= IDEX_rval2)
            pc.load(IDEX_inst_addr + imm);
    }
};

class LoadInst: public ITypeInst {
public:
    void exec() {
        EXMEM_addr = IDEX_rval1 + imm;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class LB: public LoadInst {
public:
    void mem_access() {
        MEMWB_data = sgnext(mem.read(EXMEM_addr), 7);
    }
};

class LH: public LoadInst {
public:
    void mem_access() {
        MEMWB_data = sgnext(mem.read_word(EXMEM_addr), 15);
    }
};

class LW: public LoadInst {
public:
    void mem_access() {
        MEMWB_data = mem.read_dword(EXMEM_addr);
    }
};

class LBU: public LoadInst {
public:
    void mem_access() {
        MEMWB_data = mem.read(EXMEM_addr);
    }
};

class LHU: public LoadInst {
public:
    void mem_access() {
        MEMWB_data = mem.read_word(EXMEM_addr);
    }
};

class StoreInst: public STypeInst {
public:
    void exec() {
        EXMEM_addr = IDEX_rval1 + imm;
        EXMEM_data = IDEX_rval2;
    }
};

class SB: public StoreInst {
public:
    void exec() {
        EXMEM_addr = IDEX_rval1 + imm;
        EXMEM_data = IDEX_rval2;
        if (EXMEM_addr == END_ADDR) {
            ret_val = reg[10].read() & 0xFF;
            prog_end = true;
        }
    }
    void mem_access() {
        mem.load(EXMEM_addr, EXMEM_data);
    }
};

class SH: public StoreInst {
public:
    void mem_access() {
        mem.load_word(EXMEM_addr, EXMEM_data);
    }
};

class SW: public StoreInst {
public:
    void mem_access() {
        mem.load_dword(EXMEM_addr, EXMEM_data);
    }
};

class ITypeCalcInst: public ITypeInst {
public:
    void mem_access() {
        MEMWB_data = EXMEM_data;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class ADDI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 + imm;
    }
};

class SLTI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = (int) IDEX_rval1 < (int) imm;
    }
};

class SLTIU: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 < imm;
    }
};

class XORI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 ^ imm;
    }
};

class ORI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 | imm;
    }
};

class ANDI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 & imm;
    }
};

class SLLI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 << shamt;
    }
};

class SRLI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 >> shamt;
    }
};

class SRAI: public ITypeCalcInst {
public:
    void exec() {
        EXMEM_data = (int) IDEX_rval1 >> shamt;
    }
};

class RTypeCalcInst: public RTypeInst {
public:
    void mem_access() {
        MEMWB_data = EXMEM_data;
    }
    void write_back() {
        reg[dest].load(MEMWB_data);
    }
};

class ADD: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 + IDEX_rval2;
    }
};

class SUB: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 - IDEX_rval2;
    }
};

class SLL: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 << (IDEX_rval2 & 0x1F);
    }
};

class SLT: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = (int) IDEX_rval1 < (int) IDEX_rval2;
    }
};

class SLTU: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 < IDEX_rval2;
    }
};

class XOR: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 ^ IDEX_rval2;
    }
};

class SRL: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 >> (IDEX_rval2 & 0x1F);
    }
};

class SRA: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = (int) IDEX_rval1 >> (IDEX_rval2 & 0x1F);
    }
};

class OR: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 | IDEX_rval2;
    }
};

class AND: public RTypeCalcInst {
public:
    void exec() {
        EXMEM_data = IDEX_rval1 & IDEX_rval2;
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
        unsigned addr = pc.read();
        IFID_inst_addr = addr;
        delete inst;
        inst = Inst::parse(mem.read_dword(addr));
        inst->inst_fetch();
    }
};

class InstDecode {
public:
    void work(Inst * inst) {
        IDEX_inst_addr = IFID_inst_addr;
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
    pc.load(0);
    mem.initialize();
    reg[0].load(0);
    reg[0].set_read_only(true);
    prog_end = false;
    timer = 0;
    inst = NULL;

    while (!prog_end) {
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