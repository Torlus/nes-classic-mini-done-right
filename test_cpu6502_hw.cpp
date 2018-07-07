#include "Vcpu65xx.h"
#include "verilated.h"

#if VM_TRACE
#include "verilated_vcd_c.h"
#endif

// const vluint64_t CYCLES_TOTAL = (1L << 29);
const vluint64_t CYCLES_TOTAL = 400000L * 1000L;
const vluint64_t CYCLES_RESET = 10;
const vluint64_t CYCLES_ENABLE = (1L << 2) - 1L;

#define CYCLES_PER_PS 1000
#define LOG_DURATION_US 100

int main(int argc, char **argv, char **env) {
  vluint64_t hc = 0;
  vluint64_t us = 0;
  vluint64_t us_log_start = 0;

  vluint64_t cycles = 0;

  vluint8_t mem[0x10000];

  vluint8_t clk = 0;
  vluint8_t enable = 0;
  vluint8_t reset = 1;
  vluint8_t nmi_n = 1;
  vluint8_t irq_n = 1;
  vluint8_t so_n = 1;
  vluint8_t din;

  Verilated::commandArgs(argc, argv);

  FILE *prog = fopen("6502_functional_test.bin", "r");
  printf("Read %lu bytes\n", fread(mem, 1, 0x10000, prog));
  fclose(prog);

  // Init top verilog instance
  Vcpu65xx* top = new Vcpu65xx;

  #if VM_TRACE
    // Init VCD trace dump
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace (tfp, 99);
    tfp->spTrace()->set_time_resolution ("1 ps");
    tfp->open ("verilator_tb.vcd");
  #endif

  us_log_start = (CYCLES_TOTAL * CYCLES_PER_PS / 1000000) - LOG_DURATION_US;

  for(hc = 0L; hc < (CYCLES_TOTAL << 1); hc++) {
    reset = ((hc >> 1) < CYCLES_RESET ? 1 : 0);
    enable = (((hc >> 1) & CYCLES_ENABLE) == CYCLES_ENABLE ? 1 : 0);

    if (clk && !reset && enable) {
      cycles++;
    }

    if ((hc >> 1) * CYCLES_PER_PS / 1000000 >= us) {
      printf("@ %lu us\r", us);
      us++;
    }

    clk ^= 1;
    top->clk = clk;
    top->enable = enable;
    top->reset = reset;
    top->nmi_n = nmi_n;
    top->irq_n = irq_n;
    top->so_n = so_n;

    top->eval();

    if (top->we) {
      mem[top->addr] = top->dout;
    }
    if (top->addr == 0xFFFC) {
      top->din = 0x00;
    } else if (top->addr == 0xFFFD) {
      top->din = 0x10;
    } else {
      top->din = mem[top->addr];
    }

#if VM_TRACE
    // Dump signals into VCD file
    if (tfp && (us >= us_log_start)) {
			tfp->dump(hc * (CYCLES_PER_PS >> 1));
		}
#endif

    if (top->debugPc == 0x3B1E) {
      printf("\nSuccess! Cycles: %lu\n", cycles);
      break;
    }

    if (Verilated::gotFinish())  exit(0);
  }
  top->final();

  printf("\nDone.\n");

  #if VM_TRACE
    if (tfp) tfp->close();
  #endif

  exit(0);
}
