#include "disasm.hpp"
#include <cstdio>

static const char* ABI[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

static uint32_t bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}
static int32_t sext(uint32_t x, int n) {
    uint32_t m = 1u << (n - 1);
    return (int32_t)((x ^ m) - m);
}

std::string disassemble(uint32_t inst, uint32_t pc) {
    char buf[128];
    uint32_t opcode = bits(inst, 6, 0);
    uint32_t rd  = bits(inst, 11, 7);
    uint32_t f3  = bits(inst, 14, 12);
    uint32_t rs1 = bits(inst, 19, 15);
    uint32_t rs2 = bits(inst, 24, 20);
    uint32_t f7  = bits(inst, 31, 25);
    const char* RD = ABI[rd]; const char* R1 = ABI[rs1]; const char* R2 = ABI[rs2];

    int32_t iI = sext(bits(inst, 31, 20), 12);
    int32_t iS = sext((bits(inst,31,25)<<5)|bits(inst,11,7), 12);
    int32_t iB = sext((bits(inst,31,31)<<12)|(bits(inst,7,7)<<11)|
                      (bits(inst,30,25)<<5)|(bits(inst,11,8)<<1), 13);
    int32_t iJ = sext((bits(inst,31,31)<<20)|(bits(inst,19,12)<<12)|
                      (bits(inst,20,20)<<11)|(bits(inst,30,21)<<1), 21);
    uint32_t iU = inst & 0xFFFFF000u;
    uint32_t shamt = bits(inst, 24, 20);

    switch (opcode) {
    case 0x03:
        switch (f3) {
            case 0: snprintf(buf,sizeof buf,"lb   %s, %d(%s)",RD,iI,R1); break;
            case 1: snprintf(buf,sizeof buf,"lh   %s, %d(%s)",RD,iI,R1); break;
            case 2: snprintf(buf,sizeof buf,"lw   %s, %d(%s)",RD,iI,R1); break;
            case 4: snprintf(buf,sizeof buf,"lbu  %s, %d(%s)",RD,iI,R1); break;
            case 5: snprintf(buf,sizeof buf,"lhu  %s, %d(%s)",RD,iI,R1); break;
            default: snprintf(buf,sizeof buf,".word 0x%08x",inst);
        } break;
    case 0x13:
        switch (f3) {
            case 0: snprintf(buf,sizeof buf,"addi %s, %s, %d",RD,R1,iI); break;
            case 2: snprintf(buf,sizeof buf,"slti %s, %s, %d",RD,R1,iI); break;
            case 3: snprintf(buf,sizeof buf,"sltiu %s, %s, %d",RD,R1,iI); break;
            case 4: snprintf(buf,sizeof buf,"xori %s, %s, %d",RD,R1,iI); break;
            case 6: snprintf(buf,sizeof buf,"ori  %s, %s, %d",RD,R1,iI); break;
            case 7: snprintf(buf,sizeof buf,"andi %s, %s, %d",RD,R1,iI); break;
            case 1: snprintf(buf,sizeof buf,"slli %s, %s, %u",RD,R1,shamt); break;
            case 5: snprintf(buf,sizeof buf,"%s %s, %s, %u",f7==0x20?"srai":"srli",RD,R1,shamt); break;
            default: snprintf(buf,sizeof buf,".word 0x%08x",inst);
        } break;
    case 0x17: snprintf(buf,sizeof buf,"auipc %s, 0x%x",RD,iU>>12); break;
    case 0x23:
        switch (f3) {
            case 0: snprintf(buf,sizeof buf,"sb   %s, %d(%s)",R2,iS,R1); break;
            case 1: snprintf(buf,sizeof buf,"sh   %s, %d(%s)",R2,iS,R1); break;
            case 2: snprintf(buf,sizeof buf,"sw   %s, %d(%s)",R2,iS,R1); break;
            default: snprintf(buf,sizeof buf,".word 0x%08x",inst);
        } break;
    case 0x33: {
        const char* op="?";
        if (f3==0) op = (f7==0x20)?"sub":"add";
        else if (f3==1) op="sll"; else if (f3==2) op="slt";
        else if (f3==3) op="sltu"; else if (f3==4) op="xor";
        else if (f3==5) op=(f7==0x20)?"sra":"srl";
        else if (f3==6) op="or"; else if (f3==7) op="and";
        snprintf(buf,sizeof buf,"%s  %s, %s, %s",op,RD,R1,R2);
    } break;
    case 0x37: snprintf(buf,sizeof buf,"lui  %s, 0x%x",RD,iU>>12); break;
    case 0x63: {
        const char* op="?";
        switch(f3){case 0:op="beq";break;case 1:op="bne";break;case 4:op="blt";break;
                   case 5:op="bge";break;case 6:op="bltu";break;case 7:op="bgeu";break;}
        snprintf(buf,sizeof buf,"%s  %s, %s, 0x%x",op,R1,R2,pc+iB);
    } break;
    case 0x67: snprintf(buf,sizeof buf,"jalr %s, %d(%s)",RD,iI,R1); break;
    case 0x6F: snprintf(buf,sizeof buf,"jal  %s, 0x%x",RD,pc+iJ); break;
    case 0x73: snprintf(buf,sizeof buf,"%s", bits(inst,20,20)?"ebreak":"ecall"); break;
    default: snprintf(buf,sizeof buf,".word 0x%08x",inst);
    }
    return std::string(buf);
}
