#ifndef NES_CPU6502_INCLUDED
#define NES_CPU6502_INCLUDED

#include <cstdint>
#include <cstdio>

class CPU6502 {
public:
    uint16_t PC;        // Program Counter
    uint8_t A, X, Y, S; // Registers
    bool N, Z, C, V;    // ALU Flags
    bool I, D;          // Other Flags
    
    bool irq, nmi;      // Interrupt Requests Logic Levels
    
    uint8_t opcode;     // Current Opcode
    uint32_t cycles;    // Cycles Counter

    CPU6502(void);
    
    uint8_t (*read)(uint16_t address);
    void (*write)(uint16_t address, uint8_t data);

    void reset(void);
    void step(void);
    void log(FILE *stream);
private:
	uint16_t addr;
    uint16_t tmp;
    
    void izx(void);
    void izy(void);
    void ind(void);
    void zp(void);
    void zpx(void);
    void zpy(void);
    void imp(void);
    void imm(void);
    void abs(void);
    void abx(void);
    void aby(void);
    void rel(void);
    void rmw(void);
    void fnz(uint16_t v);
    void fnzb(uint16_t v);
    void fnzc(uint16_t v);
    void branch(bool taken);
    
    void adc(void);
    void ahx(void);
    void anc(void);
    void _and(void);
    void ane(void);
    void alr(void);
    void arr(void);
    void asl(void);
    void asla(void);
    void bit(void);
    void brk(void);
    void bcc(void);
    void bcs(void);
    void beq(void);
    void bne(void);
    void bmi(void);
    void bpl(void);
    void bvc(void);
    void bvs(void);
    void clc(void);
    void cld(void);
    void cli(void);
    void clv(void);
    void cmp(void);
    void cpx(void);
    void cpy(void);
    void dcp(void);
    void dec(void);
    void dex(void);
    void dey(void);
    void eor(void);
    void inc(void);
    void inx(void);
    void iny(void);
    void isc(void);
    void jmp(void);
    void jsr(void);
    void kil(void);
    void lda(void);
    void ldx(void);
    void ldy(void);
    void ora(void);
    void rol(void);
    void rla(void);
    void ror(void);
    void rra(void);
    void las(void);
    void lax(void);
    void lsr(void);
    void lsra(void);
    void nop(void);
    void pha(void);
    void php(void);
    void pla(void);
    void plp(void);
    void rti(void);
    void rts(void);
    void sax(void);
    void sbc(void);
    void sbx(void);
    void sec(void);
    void sed(void);
    void sei(void);
    void shs(void);
    void shx(void);
    void shy(void);
    void slo(void);
    void sre(void);
    void sta(void);
    void stx(void);
    void sty(void);
    void tax(void);
    void tay(void);
    void tsx(void);
    void txa(void);
    void txs(void);
    void tya(void);
};

#endif // NES_CPU6502_INCLUDED
