#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

class CPU {
public:
    uint32_t pc = 0;
    uint32_t regs[32] = {0};
    bool halted = false;
    int exit_code = 0;

    static constexpr uint32_t PAGE_SIZE = 4096;
    std::unordered_map<uint32_t, std::vector<uint8_t>> pages;

    uint32_t get_reg(uint32_t i) const { return i == 0 ? 0u : regs[i]; }
    void set_reg(uint32_t i, uint32_t v) { if (i != 0) regs[i] = v; }

    uint8_t  load8(uint32_t addr);
    uint16_t load16(uint32_t addr);
    uint32_t load32(uint32_t addr);
    void store8(uint32_t addr, uint8_t v);
    void store16(uint32_t addr, uint16_t v);
    void store32(uint32_t addr, uint32_t v);

    void load_program(const std::vector<uint8_t>& bytes, uint32_t base = 0);

    bool step();

private:
    uint8_t* byte_ptr(uint32_t addr);
    void execute(uint32_t inst);
};
