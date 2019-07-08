#include <iostream>
#include "Inst.hpp"
using namespace std;

int main() {
    mem.init();
    reg[0].set_zero();
    pc.write(0);
    ret = false;
    inst[ID] = new NOP;
    inst[EX] = new NOP;
    inst[MEM] = new NOP;
    inst[WB] = new NOP;

    while (!ret) {
        stall = false;
        bubble = false;
        inst[WB]->write_back();
        delete inst[WB];
        inst[MEM]->mem_access();
        inst[WB] = inst[MEM];
        inst[EX]->execute();
        inst[MEM] = inst[EX];
        if (bubble) {
            delete inst[ID];
            inst[EX] = new NOP;
            inst[ID] = new NOP;
        } else {
            inst[ID]->inst_decode();
            if (stall)
                inst[EX] = new NOP;
            else {
                inst[EX] = inst[ID];
                unsigned cur_pc = pc.read();
                unsigned code = mem.read_dword(cur_pc);
                inst[IF] = Inst::parse(code);
                inst[IF]->pc_modify();
                inst[ID] = inst[IF];
            }
        }
    }

    cout << (reg[10].read() & 0xFF) << endl;
    delete inst[ID];
    delete inst[EX];
    delete inst[MEM];
    delete inst[WB];
    return 0;
}