# ProcessorCacheEmulator

## Overview

This repository contains the implementation for hardest lab of the Computer Architecture course. 
The lab involves translating a C program into RISC-V assembly and simulating the processor and cache behavior with LRU and bit-pLRU replacement policies. 
In addition, the code has been translated from RISC-V to machine code.

The following [RISC-V command sets](https://github.com/riscv/riscv-isa-manual/releases/tag/riscv-isa-release-d2c44bb-2024-04-18) are supported:
- RV32I
- RV32M

## Project Structure

- `src/`: Contains the source code for the assembly code emulation.
- `asm/`: Contains the RISC-V assembly code and machine code.
- `bin/`: Contains the source code for the converting RISC-V to machine code.

## Arguments

The program accepts the following command-line arguments:

- `--replacement <policy>`: Cache replacement policy (0, 1, or 2).
  - `0`: Output results for all policies.
  - `1`: Output results for LRU only.
  - `2`: Output results for bit-pLRU only.
- `--asm <assembly_file>`: Specifies the file containing the RISC-V assembly code to execute.
- `--bin <machine_code_file>`: Specifies the output file for the machine code (normal mode only).

## Example Usage

```bash
./cache_sim --replacement 1 --asm rv32.asm --bin rv32.bin
```

## Output
The simulation results are printed to the standard output in the following format:

```perl
LRU hit rate: %3.4f%%
pLRU hit rate: %3.4f%%
```

If there were no memory accesses, the output will be:

```yaml
LRU hit rate: nan%
pLRU hit rate: nan%
```

If a policy is not supported, the output will be:
```yaml
pLRU unsupported
```

## Implementation Details
### Part 1: Assembly Translation

The given C program is translated into RISC-V assembly and saved in **asm/rv32.asm**. \
There is also a translation into machine code, it is located in the **asm/rv32.bin** file.

```c
#define M 64
#define N 60
#define K 32

int8_t a[M][K];
int16_t b[K][N];
int32_t c[M][N];

void mmul() {
    int8_t *pa = a;
    int32_t *pc = c;
    for (int y = 0; y < M; y++) {
        for (int x = 0; x < N; x++) {
            int16_t *pb = b;
            int32_t s = 0;
            for (int k = 0; k < K; k++) {
                s += pa[k] * pb[x];
                pb += N;
            }
            pc[x] = s;
        }
        pa += K;
        pc += N;
    }
}
```

### Part 2: Processor and Cache Simulation
The cache simulation uses the following parameters:

MEM_SIZE: Memory size \
CACHE_SIZE: Cache size excluding overhead \
CACHE_LINE_SIZE: Size of each cache line \
CACHE_LINE_COUNT: Number of cache lines \
CACHE_WAY: Cache associativity \
CACHE_SETS: Number of cache sets \
ADDR_LEN: Address length in bits \
CACHE_TAG_LEN: Tag length in bits \
CACHE_INDEX_LEN: Index length in bits \
CACHE_OFFSET_LEN: Offset length in bits

You can set these parameters yourself by changing the corresponding constants in the **src/Cache Utils.hpp** and **src/MemoryUtils.hpp** files. \

-----------------------
*Remember, these parameters are related and random numbers can lead to unexpected results.*

-----------------------

## Compilation and Execution
### Compilation

To compile the simulation, run the following command:

```bash
make
```

### Execution

To run the simulation, use the following command:

```bash
./cache_sim --replacement <policy> --asm <assembly_file> --bin <machine_code_file>
```

