#include <cstdlib>
#include <cstdint>
#include "cpu6502.h"

uint8_t mem[0x10000];

uint8_t cpu_read(uint16_t address) {
    return mem[address];
}
void cpu_write(uint16_t address, uint8_t data) {
    mem[address] = data;
}

int main() {
    FILE *prog = fopen("6502_functional_test.bin", "r");
    printf("Read %lu bytes\n", fread(mem, 1, 0x10000, prog));
    fclose(prog);
    
    CPU6502 cpu;
    cpu.read = cpu_read;
    cpu.write = cpu_write;
    cpu.reset();
    cpu.opcode = cpu_read(0x1000);
    cpu.PC = 0x1000;
    // cpu.log(stdout);
    uint16_t prevPC = 0x1000;
    for(;;) {
        cpu.step();
        // cpu.log(stdout);
        if (cpu.PC == prevPC) {
            break;
        }
        prevPC = cpu.PC;
    }
    if (cpu.PC == 0x3B1C) {
        printf("Success!\n");
    } else {
        printf("Failed at $%04X.\n", cpu.PC);
    }

}