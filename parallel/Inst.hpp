#ifndef INST_HPP
#define INST_HPP 1

#include "Register.hpp"
#include "Memory.hpp"

Memory mem;
Register reg[32];
Register pc;
bool stall, bubble, ret;

enum Stage {IF, ID, EX, MEM, WB};

unsigned sgnext(unsigned imm, int hi) {
    if (imm & (1 << hi))
        imm |= 0xFFFFFFFF >> hi << hi;
    return imm;
}

class Inst {
public:
    virtual void pc_modify() {
        unsigned cur_pc = pc.read();
        pc.write(cur_pc + 4);
    }
    virtual void inst_decode() {}
    virtual void execute() {}
    virtual void mem_access() {}
    virtual void write_back() {}
    virtual bool forward(Stage stage, unsigned src, unsigned & rval) {
        return false;
    }
    virtual ~Inst() {}
    static Inst * parse(unsigned code);
};

Inst * inst[5];

class SrcInst: public Inst {
public:
    void get_fwd(unsigned src, unsigned & rval) {
        if (!src) return;
        if (inst[EX]->forward(EX, src, rval)) return;
        if (inst[MEM]->forward(MEM, src, rval)) return;
        inst[WB]->forward(WB, src, rval);
    }
};

class RTypeInst: public SrcInst {
protected:
    unsigned src1, src2, dest;
    unsigned lhs, rhs, ans;
public:
    void inst_decode() {
        lhs = reg[src1].read();
        rhs = reg[src2].read();
        get_fwd(src1, lhs);
        get_fwd(src2, rhs);
    }
    void write_back() {
        reg[dest].write(ans);
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            rval = ans;
            return true;
        }
        return false;
    }
    void set(unsigned src1, unsigned src2, unsigned dest) {
        this->src1 = src1;
        this->src2 = src2;
        this->dest = dest;
    }
    static Inst * parse(unsigned code);
};

class ADD: public RTypeInst {
public:
    void execute() {
        ans = lhs + rhs;
    }
};

class SUB: public RTypeInst {
public:
    void execute() {
        ans = lhs - rhs;
    }
};

class SLL: public RTypeInst {
public:
    void execute() {
        ans = lhs << (rhs & 0x1F);
    }
};

class SLT: public RTypeInst {
public:
    void execute() {
        ans = (int) lhs < (int) rhs;
    }
};

class SLTU: public RTypeInst {
public:
    void execute() {
        ans = lhs < rhs;
    }
};

class XOR: public RTypeInst {
public:
    void execute() {
        ans = lhs ^ rhs;
    }
};

class SRL: public RTypeInst {
public:
    void execute() {
        ans = lhs >> (rhs & 0x1F);
    }
};

class SRA: public RTypeInst {
public:
    void execute() {
        ans = (int) lhs >> (rhs & 0x1F);
    }
};

class OR: public RTypeInst {
public:
    void execute() {
        ans = lhs | rhs;
    }
};

class AND: public RTypeInst {
public:
    void execute() {
        ans = lhs & rhs;
    }
};

class ITypeInst: public SrcInst {
protected:
    unsigned imm, src, dest;
    unsigned rval, ans;
public:
    void inst_decode() {
        rval = reg[src].read();
        get_fwd(src, rval);
    }
    void write_back() {
        reg[dest].write(ans);
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            rval = ans;
            return true;
        }
        return false;
    }
    void set(unsigned imm, unsigned src, unsigned dest) {
        this->imm = imm;
        this->src = src;
        this->dest = dest;
    }
    static Inst * parse(unsigned code);
};

class JALR: public ITypeInst {
protected:
    unsigned cur_pc, pred_pc;
public:
    void pc_modify() {
        cur_pc = pc.read();
        pred_pc = cur_pc + 4;
        pc.write(pred_pc);
    }
    void execute() {
        unsigned next_pc = (rval + imm) >> 1 << 1;
        if (pred_pc != next_pc) {
            pc.write(next_pc);
            bubble = true;
        }
        ans = cur_pc + 4;
    }
};

class ADDI: public ITypeInst {
public:
    void execute() {
        ans = rval + imm;
    }
};

class NOP: public ADDI {
public:
    NOP() {
        set(0, 0, 0);
    }
};

class SLTI: public ITypeInst {
public:
    void execute() {
        ans = (int) rval < (int) imm;
    }
};

class SLTIU: public ITypeInst {
public:
    void execute() {
        ans = rval < imm;
    }
};

class XORI: public ITypeInst {
public:
    void execute() {
        ans = rval ^ imm;
    }
};

class ORI: public ITypeInst {
public:
    void execute() {
        ans = rval | imm;
    }
};

class ANDI: public ITypeInst {
public:
    void execute() {
        ans = rval & imm;
    }
};

class SLLI: public ITypeInst {
public:
    void execute() {
        ans = rval << (imm & 0x1F);
    }
};

class SRLI: public ITypeInst {
public:
    void execute() {
        ans = rval >> (imm & 0x1F);
    }
};

class SRAI: public ITypeInst {
public:
    void execute() {
        ans = (int) rval >> (imm & 0x1F);
    }
};

class LoadInst: public ITypeInst {
protected:
    unsigned addr;
public:
    void execute() {
        addr = rval + imm;
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            if (stage == EX)
                stall = true;
            else
                rval = ans;
            return true;
        }
        return false;
    }
 };

class LB: public LoadInst {
public:
    void mem_access() {
        ans = sgnext(mem.read(addr), 7);
    }
};

class LH: public LoadInst {
public:
    void mem_access() {
        ans = sgnext(mem.read_word(addr), 15);
    }
};

class LW: public LoadInst {
public:
    void mem_access() {
        ans = mem.read_dword(addr);
    }
};

class LBU: public LoadInst {
public:
    void mem_access() {
        ans = mem.read(addr);
    }
};

class LHU: public LoadInst {
public:
    void mem_access() {
        ans = mem.read_word(addr);
    }
};

class STypeInst: public SrcInst {
protected:
    unsigned imm, src1, src2;
    unsigned base, data, addr;
public:
    void inst_decode() {
        base = reg[src1].read();
        data = reg[src2].read();
        get_fwd(src1, base);
        get_fwd(src2, data);
    }
    void execute() {
        addr = base + imm;
    }
    void set(unsigned imm, unsigned src1, unsigned src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
    static Inst * parse(unsigned code);
};

class SB: public STypeInst {
public:
    void mem_access() {
        mem.write(addr, data);
        ret = addr == 0x30004;
    }
};

class SH: public STypeInst {
public:
    void mem_access() {
        mem.write_word(addr, data);
        ret = addr == 0x30004;
    }
};

class SW: public STypeInst {
public:
    void mem_access() {
        mem.write_dword(addr, data);
        ret = addr == 0x30004;
    }
};

class BTypeInst: public SrcInst {
protected:
    unsigned imm, src1, src2;
    unsigned lhs, rhs, cur_pc, pred_pc;
public:
    void pc_modify() {
        cur_pc = pc.read();
        pred_pc = cur_pc + 4;
        pc.write(pred_pc);
    }
    void inst_decode() {
        lhs = reg[src1].read();
        rhs = reg[src2].read();
        get_fwd(src1, lhs);
        get_fwd(src2, rhs);
    }
    void execute() {
        unsigned next_pc;
        next_pc = cur_pc + (judge(lhs, rhs) ? imm : 4);
        if (pred_pc != next_pc) {
            pc.write(next_pc);
            bubble = true;
        }
    }
    virtual bool judge(unsigned lhs, unsigned rhs) {
        return true;
    }
    void set(unsigned imm, unsigned src1, unsigned src2) {
        this->imm = imm;
        this->src1 = src1;
        this->src2 = src2;
    }
    static Inst * parse(unsigned code);
};

class BEQ: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return lhs == rhs;
    }
};

class BNE: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return lhs != rhs;
    }
};

class BLT: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return (int) lhs < (int) rhs;
    }
};

class BGE: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return (int) lhs >= (int) rhs;
    }
};

class BLTU: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return lhs < rhs;
    }
};

class BGEU: public BTypeInst {
public:
    bool judge(unsigned lhs, unsigned rhs) {
        return lhs >= rhs;
    }
};

class UTypeInst: public Inst {
protected:
    unsigned imm, dest;
public:
    void set(unsigned imm, unsigned dest) {
        this->imm = imm;
        this->dest = dest;
    }
    static Inst * parse(unsigned code);
};

class LUI: public UTypeInst {
public:
    void write_back() {
        reg[dest].write(imm);
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            rval = imm;
            return true;
        }
        return false;
    }
};

class AUIPC: public UTypeInst {
protected:
    unsigned cur_pc, ans;
public:
    void pc_modify() {
        cur_pc = pc.read();
        pc.write(cur_pc + 4);
    }
    void execute() {
        ans = cur_pc + imm;
    }
    void write_back() {
        reg[dest].write(ans);
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            rval = ans;
            return true;
        }
        return false;
    }
};

class JTypeInst: public Inst {
protected:
    unsigned imm, dest;
public:
    void set(unsigned imm, unsigned dest) {
        this->imm = imm;
        this->dest = dest;
    }
    static Inst * parse(unsigned code);
};

class JAL: public JTypeInst {
protected:
    unsigned cur_pc, ans;
public:
    void pc_modify() {
        cur_pc = pc.read();
        pc.write(cur_pc + imm);
    }
    void execute() {
        ans = cur_pc + 4;
    }
    void write_back() {
        reg[dest].write(ans);
    }
    bool forward(Stage stage, unsigned src, unsigned & rval) {
        if (src == dest) {
            rval = ans;
            return true;
        }
        return false;
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
    ret->set(imm, src, dest);
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

#endif