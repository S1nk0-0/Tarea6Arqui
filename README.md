# Simulador de RISC-V (RV32I) — HW#6 Arquitectura de Computadores

Simulador del ISA base RISC-V de 32 bits (RV32I). Carga un binario crudo en
memoria, mantiene el estado arquitectural (PC, 32 registros, memoria plana
little-endian) y permite ejecución paso por paso con inspección de registros
y memoria desde un REPL.

## Compilación

### Con CMake (CLion / línea de comandos)
```
mkdir build && cd build
cmake ..
cmake --build .
```

### Directo con g++ / MinGW
```
g++ -std=c++17 -O2 src/*.cpp -o rv-sim
```

## Uso
```
./rv-sim programa.bin
```

### Comandos del REPL
| Comando | Acción |
|---|---|
| `pc` | muestra el contador de programa |
| `step [n]` | ejecuta n instrucciones (default 1), mostrando el desensamblado |
| `run` | ejecuta hasta halt |
| `regs [r ...]` | muestra registros (`x5`, `t0`, `10`...) o los 32 si no se dan |
| `mem <ini> [fin]` | muestra bytes de memoria en hex |
| `disasm [addr] [n]` | desensambla n instrucciones |
| `reset` | reinicia el simulador |
| `exit` | salir |

Los registros aceptan nombre ABI (`sp`, `t0`, `a0`...), `xN`, o el número.
Las direcciones aceptan `0x...` o decimal.

## Arquitectura del código
- `src/cpu.hpp` / `src/cpu.cpp` — estado de la CPU, memoria paginada
  little-endian, ciclo fetch-decode-execute, las 40 instrucciones RV32I.
- `src/disasm.hpp` / `src/disasm.cpp` — desensamblador (nombres ABI).
- `src/main.cpp` — carga del binario y REPL.

### Decisiones de diseño
- **Memoria paginada** (`unordered_map` de páginas de 4 KiB) para tener un
  address space de 32 bits sin reservar 4 GB.
- **Little-endian** en todos los accesos de 16/32 bits.
- **x0 cableado a 0**: lecturas siempre devuelven 0, escrituras se descartan.
- **Inmediatos con extensión de signo** por tipo (I, S, B, U, J).
- **Halt**: una instrucción `0x00000000` o un `ecall` de exit detienen la CPU.

## Extras incluidos (sobre lo obligatorio)
- Desensamblador integrado (`disasm`, y se muestra en cada `step`).
- Soporte mínimo de `ecall` estilo SPIM: `a7=1` print_int, `a7=4` print_string,
  `a7=10`/`93` exit.

## Pruebas

Los tres programas del enunciado ya están incluidos y **verificados**:

| Programa | Comando de verificación | Resultado esperado | Estado |
|---|---|---|---|
| riscvtest | `run` → `mem 0x64 0x67` | `19 00 00 00` (0x19=25) | ✅ |
| quicksort | `run` → `mem 0x1000 0x101b` | `01 02 03 04 06 08 09` | ✅ |
| árbol simétrico | `run` → `regs a0` | `a0 = 0x00000001` | ✅ |

```
./rv-sim tests/riscvtest.bin
> run
> mem 0x64 0x67       # -> 19 00 00 00
```

El `run` detecta el fin del programa cuando llega a un salto incondicional a
sí mismo (el idiom `j .` / loop final que usan estos programas), o ante un
`ecall` de exit.

### Sobre el ensamblado
El simulador ejecuta binario crudo, no ensambla. Los `.bin` de `tests/` fueron
generados desde los `.s` oficiales con `tests/rvasm.py`, un mini-ensamblador
que también expande pseudoinstrucciones (`li`, `mv`, `call`, `ret`, `j`).
`tests/verify.py` confirma que el riscvtest.bin coincide **bit a bit** con el
machine code oficial de Harris & Harris.

Alternativamente puedes generar los `.bin` desde CPUlator: Compile and Load
(F5) → Memory (Ctrl+M) → File format **Raw** → Save (start 0, end 0x2000) →
Save to file.
