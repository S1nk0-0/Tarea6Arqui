#include "cpu.hpp"
#include <iostream>
#include <cstdio>

uint8_t* CPU::byte_ptr(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    uint32_t off  = addr % PAGE_SIZE;
    auto it = pages.find(page);
    if (it == pages.end())
        it = pages.emplace(page, std::vector<uint8_t>(PAGE_SIZE, 0)).first;
    return &it->second[off];
}

uint8_t CPU::load8(uint32_t addr) { return *byte_ptr(addr); }

uint16_t CPU::load16(uint32_t addr) {
    return (uint16_t)load8(addr) | ((uint16_t)load8(addr + 1) << 8);
}

uint32_t CPU::load32(uint32_t addr) {
    return (uint32_t)load8(addr)
         | ((uint32_t)load8(addr + 1) << 8)
         | ((uint32_t)load8(addr + 2) << 16)
         | ((uint32_t)load8(addr + 3) << 24);
}

void CPU::store8(uint32_t addr, uint8_t v) { *byte_ptr(addr) = v; }

void CPU::store16(uint32_t addr, uint16_t v) {
    store8(addr,     v & 0xFF);
    store8(addr + 1, (v >> 8) & 0xFF);
}

void CPU::store32(uint32_t addr, uint32_t v) {
    store8(addr,     v & 0xFF);
    store8(addr + 1, (v >> 8) & 0xFF);
    store8(addr + 2, (v >> 16) & 0xFF);
    store8(addr + 3, (v >> 24) & 0xFF);
}

void CPU::load_program(const std::vector<uint8_t>& bytes, uint32_t base) {
    for (size_t i = 0; i < bytes.size(); ++i)
        store8(base + (uint32_t)i, bytes[i]);
}

static inline uint32_t bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}

static inline int32_t sext(uint32_t x, int n) {
    uint32_t m = 1u << (n - 1);
    return (int32_t)((x ^ m) - m);
}

static inline int32_t imm_I(uint32_t i) { return sext(bits(i, 31, 20), 12); }

static inline int32_t imm_S(uint32_t i) {
    uint32_t v = (bits(i, 31, 25) << 5) | bits(i, 11, 7);
    return sext(v, 12);
}

static inline int32_t imm_B(uint32_t i) {
    uint32_t v = (bits(i, 31, 31) << 12)
               | (bits(i, 7, 7)   << 11)
               | (bits(i, 30, 25) << 5)
               | (bits(i, 11, 8)  << 1);
    return sext(v, 13);
}

static inline int32_t imm_U(uint32_t i) { return (int32_t)(i & 0xFFFFF000u); }

static inline int32_t imm_J(uint32_t i) {
    uint32_t v = (bits(i, 31, 31) << 20)
               | (bits(i, 19, 12) << 12)
               | (bits(i, 20, 20) << 11)
               | (bits(i, 30, 21) << 1);
    return sext(v, 21);
}

bool CPU::step() {
    if (halted) return false;
    uint32_t inst = load32(pc);
    if (inst == 0) {
        halted = true;
        exit_code = 0;
        return false;
    }
    uint32_t prev_pc = pc;
    execute(inst);
    regs[0] = 0;
    if (!halted && pc == prev_pc) {
        halted = true;
        exit_code = 0;
    }
    return !halted;
}

void CPU::execute(uint32_t inst) {
    uint32_t opcode = bits(inst, 6, 0);
    uint32_t rd     = bits(inst, 11, 7);
    uint32_t funct3 = bits(inst, 14, 12);
    uint32_t rs1    = bits(inst, 19, 15);
    uint32_t rs2    = bits(inst, 24, 20);
    uint32_t funct7 = bits(inst, 31, 25);

    uint32_t next_pc = pc + 4;

    uint32_t a = get_reg(rs1);
    uint32_t b = get_reg(rs2);

    switch (opcode) {

    case 0x03: {
        uint32_t addr = a + (uint32_t)imm_I(inst);
        switch (funct3) {
            case 0x0: set_reg(rd, (uint32_t)(int32_t)(int8_t) load8(addr)); break;
            case 0x1: set_reg(rd, (uint32_t)(int32_t)(int16_t)load16(addr)); break;
            case 0x2: set_reg(rd, load32(addr)); break;
            case 0x4: set_reg(rd, (uint32_t)load8(addr)); break;
            case 0x5: set_reg(rd, (uint32_t)load16(addr)); break;
            default: goto bad;
        }
        break;
    }

    case 0x13: {
        int32_t imm = imm_I(inst);
        uint32_t shamt = bits(inst, 24, 20);
        switch (funct3) {
            case 0x0: set_reg(rd, a + (uint32_t)imm); break;
            case 0x2: set_reg(rd, ((int32_t)a < imm) ? 1 : 0); break;
            case 0x3: set_reg(rd, (a < (uint32_t)imm) ? 1 : 0); break;
            case 0x4: set_reg(rd, a ^ (uint32_t)imm); break;
            case 0x6: set_reg(rd, a | (uint32_t)imm); break;
            case 0x7: set_reg(rd, a & (uint32_t)imm); break;
            case 0x1: set_reg(rd, a << shamt); break;
            case 0x5:
                if (funct7 == 0x20) set_reg(rd, (uint32_t)((int32_t)a >> shamt));
                else                set_reg(rd, a >> shamt);
                break;
            default: goto bad;
        }
        break;
    }

    case 0x17: set_reg(rd, pc + (uint32_t)imm_U(inst)); break;

    case 0x23: {
        uint32_t addr = a + (uint32_t)imm_S(inst);
        switch (funct3) {
            case 0x0: store8(addr,  (uint8_t)b); break;
            case 0x1: store16(addr, (uint16_t)b); break;
            case 0x2: store32(addr, b); break;
            default: goto bad;
        }
        break;
    }

    case 0x33: {
        switch (funct3) {
            case 0x0:
                if (funct7 == 0x20) set_reg(rd, a - b);
                else                set_reg(rd, a + b);
                break;
            case 0x1: set_reg(rd, a << (b & 31)); break;
            case 0x2: set_reg(rd, ((int32_t)a < (int32_t)b) ? 1 : 0); break;
            case 0x3: set_reg(rd, (a < b) ? 1 : 0); break;
            case 0x4: set_reg(rd, a ^ b); break;
            case 0x5:
                if (funct7 == 0x20) set_reg(rd, (uint32_t)((int32_t)a >> (b & 31)));
                else                set_reg(rd, a >> (b & 31));
                break;
            case 0x6: set_reg(rd, a | b); break;
            case 0x7: set_reg(rd, a & b); break;
            default: goto bad;
        }
        break;
    }

    case 0x37: set_reg(rd, (uint32_t)imm_U(inst)); break;

    case 0x63: {
        int32_t off = imm_B(inst);
        bool take = false;
        switch (funct3) {
            case 0x0: take = (a == b); break;
            case 0x1: take = (a != b); break;
            case 0x4: take = ((int32_t)a <  (int32_t)b); break;
            case 0x5: take = ((int32_t)a >= (int32_t)b); break;
            case 0x6: take = (a <  b); break;
            case 0x7: take = (a >= b); break;
            default: goto bad;
        }
        if (take) next_pc = pc + (uint32_t)off;
        break;
    }

    case 0x67: {
        uint32_t target = (a + (uint32_t)imm_I(inst)) & ~1u;
        set_reg(rd, pc + 4);
        next_pc = target;
        break;
    }

    case 0x6F: {
        set_reg(rd, pc + 4);
        next_pc = pc + (uint32_t)imm_J(inst);
        break;
    }

    case 0x73: {
        uint32_t a7 = get_reg(17);
        if (a7 == 1) {
            std::cout << (int32_t)get_reg(10);
        } else if (a7 == 4) {
            uint32_t p = get_reg(10);
            char c;
            while ((c = (char)load8(p++)) != 0) std::cout << c;
        } else if (a7 == 10 || a7 == 93) {
            halted = true;
            exit_code = (a7 == 93) ? (int)get_reg(10) : 0;
        }
        break;
    }

    default:
    bad:
        std::cerr << "Instruccion invalida en pc=0x" << std::hex << pc
                  << " (inst=0x" << inst << ")\n" << std::dec;
        halted = true;
        exit_code = 1;
        return;
    }

    pc = next_pc;
}
