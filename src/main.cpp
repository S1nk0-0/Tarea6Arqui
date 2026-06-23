#include "cpu.hpp"
#include "disasm.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>

static const char* ABI[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

static bool read_file(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamsize n = f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize((size_t)n);
    f.read(reinterpret_cast<char*>(out.data()), n);
    return true;
}

static int parse_reg(const std::string& s) {
    if (s.empty()) return -1;
    if (s[0] == 'x') { try { return std::stoi(s.substr(1)); } catch (...) { return -1; } }
    for (int i = 0; i < 32; ++i) if (s == ABI[i]) return i;
    try { return std::stoi(s); } catch (...) { return -1; }
}

static uint32_t parse_u32(const std::string& s) {
    return (uint32_t)std::stoul(s, nullptr, 0);
}

static void print_reg(const CPU& c, int i) {
    printf("x%-2d (%-4s) = 0x%08x\n", i, ABI[i], c.get_reg(i));
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <programa.bin>\n";
        return 1;
    }
    std::vector<uint8_t> prog;
    if (!read_file(argv[1], prog)) {
        std::cerr << "No se pudo abrir: " << argv[1] << "\n";
        return 1;
    }

    CPU cpu;
    cpu.load_program(prog, 0x00000000);
    std::cout << "\"" << argv[1] << "\" cargado a memoria (" << prog.size() << " bytes).\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd; iss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "exit" || cmd == "quit" || cmd == "q") {
            std::cout << "See you next time...\n";
            break;
        }
        else if (cmd == "pc") {
            printf("pc = 0x%08x\n", cpu.pc);
        }
        else if (cmd == "step" || cmd == "s") {
            int n = 1; std::string arg; if (iss >> arg) { try { n = std::stoi(arg); } catch(...){} }
            for (int k = 0; k < n; ++k) {
                if (cpu.halted) { std::cout << "CPU detenida.\n"; break; }
                uint32_t inst = cpu.load32(cpu.pc);
                printf("0x%08x:  %s\n", cpu.pc, disassemble(inst, cpu.pc).c_str());
                cpu.step();
            }
        }
        else if (cmd == "run" || cmd == "r" || cmd == "c") {
            long guard = 0;
            while (!cpu.halted && guard++ < 100000000L) cpu.step();
            std::cout << "Ejecucion detenida (pc=0x" << std::hex << cpu.pc
                      << std::dec << ").\n";
        }
        else if (cmd == "regs") {
            std::string r;
            bool any = false;
            while (iss >> r) { int i = parse_reg(r); if (i >= 0 && i < 32) { print_reg(cpu, i); any = true; } }
            if (!any) {
                for (int i = 0; i < 32; ++i) print_reg(cpu, i);
            }
        }
        else if (cmd == "mem") {
            std::string sa, sb; iss >> sa >> sb;
            if (sa.empty()) { std::cout << "Uso: mem <inicio> [fin]\n"; continue; }
            uint32_t a = parse_u32(sa);
            uint32_t b = sb.empty() ? a : parse_u32(sb);
            printf("Memoria (0x%x-0x%x):", a, b);
            for (uint32_t x = a; x <= b; ++x) printf(" %02X", cpu.load8(x));
            printf("\n");
        }
        else if (cmd == "disasm" || cmd == "d") {
            std::string sa, sn; iss >> sa >> sn;
            uint32_t a = sa.empty() ? cpu.pc : parse_u32(sa);
            int n = sn.empty() ? 8 : std::stoi(sn);
            for (int k = 0; k < n; ++k, a += 4) {
                uint32_t inst = cpu.load32(a);
                printf("0x%08x:  %08x  %s\n", a, inst, disassemble(inst, a).c_str());
            }
        }
        else if (cmd == "reset") {
            cpu = CPU();
            cpu.load_program(prog, 0);
            std::cout << "Estado reiniciado.\n";
        }
        else if (cmd == "help" || cmd == "h") {
            std::cout <<
              "Comandos:\n"
              "  pc                 muestra el PC\n"
              "  step [n]           ejecuta n instrucciones (default 1)\n"
              "  run                ejecuta hasta halt\n"
              "  regs [r ...]       muestra registros (x5, t0, 10...) o todos\n"
              "  mem <ini> [fin]    muestra bytes de memoria\n"
              "  disasm [addr] [n]  desensambla n instrucciones\n"
              "  reset              reinicia el simulador\n"
              "  exit               salir\n";
        }
        else {
            std::cout << "Comando desconocido: " << cmd << " (usa 'help')\n";
        }
    }

    std::cout << "Program exited with code " << cpu.exit_code << ".\n";
    return cpu.exit_code;
}
