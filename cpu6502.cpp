#include <cstdint>
#include <cstdio>
#include "cpu6502.h"

CPU6502::CPU6502(void) {
	irq = nmi = false;

	tmp = 0; 
	addr = 0;
	opcode = 0;
	cycles = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Subroutines - addressing modes & flags
////////////////////////////////////////////////////////////////////////////////

void CPU6502::izx(void) {
	uint16_t a = (read(PC++) + X) & 0xFF;
	addr = (read(a + 1) << 8) | read(a);
	cycles += 6;
}

void CPU6502::izy(void) {
	uint16_t a = read(PC++);
	uint16_t paddr = (read((a + 1) & 0xFF) << 8) | read(a);
	addr = (paddr + Y);
	if ( (paddr & 0x100) != (addr & 0x100) ) {
		cycles += 6;
	} else {
		cycles += 5;
	}
}

void CPU6502::ind(void) {
	uint16_t a = read(PC);
	a |= read( (PC & 0xFF00) | ((PC + 1) & 0xFF) ) << 8;
	addr = read(a);
	addr |= (read(a + 1) << 8);
	cycles += 5;
}

void CPU6502::zp(void) {
	addr = read(PC++);
	cycles += 3;
}

void CPU6502::zpx(void) {
	addr = (read(PC++) + X) & 0xFF;
	cycles += 4;
}

void CPU6502::zpy(void) {
	addr = (read(PC++) + Y) & 0xFF;
	cycles += 4;
}

void CPU6502::imp(void) {
	cycles += 2;
}

void CPU6502::imm(void) {
	addr = PC++;
	cycles += 2;
}

void CPU6502::abs(void) {
	addr = read(PC++);
	addr |= (read(PC++) << 8);
	cycles += 4;
}

void CPU6502::abx(void) {
	uint16_t paddr = read(PC++);
	paddr |= (read(PC++) << 8);
	addr = (paddr + X);
	if ( (paddr & 0x100) != (addr & 0x100) ) {
		cycles += 5;
	} else {
		cycles += 4;
	}
}

void CPU6502::aby(void) {
	uint16_t paddr = read(PC++);
	paddr |= (read(PC++) << 8);
	addr = (paddr + Y);
	if ( (paddr & 0x100) != (addr & 0x100) ) {
		cycles += 5;
	} else {
		cycles += 4;
	}
}

void CPU6502::rel(void) {
	addr = read(PC++);
	if (addr & 0x80) {
		addr -= 0x100;
	}
	addr += PC;
	cycles += 2;
}

////////////////////////////////////////////////////////////////////////////////

void CPU6502::rmw(void) {
	write(addr, tmp & 0xFF);
	cycles += 2;
}

////////////////////////////////////////////////////////////////////////////////

void CPU6502::fnz(uint16_t v) {
	Z = ((v & 0xFF) == 0);
	N = ((v & 0x80) != 0);
}

// Borrow
void CPU6502::fnzb(uint16_t v) {
	Z = ((v & 0xFF) == 0);
	N = ((v & 0x80) != 0);
	C = ((v & 0x100) == 0);
}

// Carry
void CPU6502::fnzc(uint16_t v) {
	Z = ((v & 0xFF) == 0);
	N = ((v & 0x80) != 0);
	C = ((v & 0x100) != 0);
}

void CPU6502::branch(bool taken) {
	if (taken) {
		if ( (addr & 0x100) != (PC & 0x100) ) {
			cycles += 2;
		} else {
			cycles += 1;
		}
		PC = addr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Subroutines - instructions
////////////////////////////////////////////////////////////////////////////////
void CPU6502::adc(void) {
	uint16_t v = read(addr);
	uint16_t c = (C ? 1 : 0);
	uint16_t r = A + v + c;
	if (D) {
		uint8_t al = (A & 0x0F) + (v & 0x0F) + c;
		if (al > 9) al += 6;
		uint8_t ah = (A >> 4) + (v >> 4) + ((al > 15) ? 1 : 0);
		Z = ((r & 0xFF) == 0);
		N = ((ah & 8) != 0);
		V = ((~(A ^ v) & (A ^ (ah << 4)) & 0x80) != 0);
		if (ah > 9) ah += 6;
		C = (ah > 15);
		A = ((ah << 4) | (al & 15)) & 0xFF;
	} else {
		Z = ((r & 0xFF) == 0);
		N = ((r & 0x80) != 0);
		V = ((~(A ^ v) & (A ^ r) & 0x80) != 0);
		C = ((r & 0x100) != 0);
		A = r & 0xFF;
	}
}

void CPU6502::ahx(void) {
	tmp = ((addr >> 8) + 1) & A & X;
	write(addr, tmp & 0xFF); 
}
	

void CPU6502::alr(void) {
	tmp = read(addr) & A;
	tmp = ((tmp & 1) << 8) | (tmp >> 1); 
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::anc(void) {
	tmp = read(addr);
	tmp |= ((tmp & 0x80) & (A & 0x80)) << 1;
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::_and(void) {
	A &= read(addr);
	fnz(A);
}

void CPU6502::ane(void) {
	tmp = read(addr) & A & (A | 0xEE);
	fnz(tmp);
	A = tmp & 0xFF;	
}

void CPU6502::arr(void) {
	tmp = read(addr) & A;
	C = ((tmp & 0x80) != 0);
	V = ((((tmp >> 7) & 1) ^ ((tmp >> 6) & 1)) != 0);
	if (D) {
		uint8_t al = (tmp & 0x0F) + (tmp & 1);
		if (al > 5) al += 6;
		uint8_t ah = ((tmp >> 4) & 0x0F) + ((tmp >> 4) & 1);
		if (ah > 5) {
			al += 6;
			C = true;
		} else {
			C = false;
		}
		tmp = (ah << 4) | al;
	}
	fnz(tmp);
	A = tmp & 0xFF;
}

void CPU6502::asl(void) {
	tmp = read(addr) << 1;
	fnzc(tmp);
	tmp &= 0xFF;
}
void CPU6502::asla(void) {
	tmp = A << 1;
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::bit(void) {
	tmp = read(addr);
	N = ((tmp & 0x80) != 0);
	V = ((tmp & 0x40) != 0);
	Z = ((tmp & A) == 0);
}

void CPU6502::brk(void) {
	PC++;
	write(S + 0x100, PC >> 8);
	S = (S - 1) & 0xFF;
	write(S + 0x100, PC & 0xFF);
	S = (S - 1) & 0xFF;
	uint8_t v = (N ? 1 << 7 : 0);
	v |= (V ? 1 << 6 : 0);
	v |= 3 << 4;
	v |= (D ? 1 << 3 : 0);
	v |= (I ? 1 << 2 : 0);
	v |= (Z ? 1 << 1 : 0);
	v |= (C ? 1 : 0);
	write(S + 0x100, v);
	S = (S - 1) & 0xFF;
	I = true;
	D = false;
	PC = (read(0xFFFF) << 8) | read(0xFFFE);
	cycles += 5;
}

void CPU6502::bcc(void) { branch( !C ); }
void CPU6502::bcs(void) { branch( C ); }
void CPU6502::beq(void) { branch( Z ); }
void CPU6502::bne(void) { branch( !Z ); }
void CPU6502::bmi(void) { branch( N ); }
void CPU6502::bpl(void) { branch( !N ); }
void CPU6502::bvc(void) { branch( !V ); }
void CPU6502::bvs(void) { branch( V ); }


void CPU6502::clc(void) { C = false; }
void CPU6502::cld(void) { D = false; }
void CPU6502::cli(void) { I = false; }
void CPU6502::clv(void) { V = false; }

void CPU6502::cmp(void) {
	tmp = A - read(addr);
	fnzb(tmp);
}

void CPU6502::cpx(void) {
	tmp = X - read(addr);
	fnzb(tmp);
}

void CPU6502::cpy(void) {
	tmp = Y - read(addr);
	fnzb(tmp);
}

void CPU6502::dcp(void) {
	tmp = (read(addr) - 1) & 0xFF;
	tmp = A - tmp;
	fnzb(tmp);
}

void CPU6502::dec(void) {
	tmp = (read(addr) - 1) & 0xFF;
	fnz(tmp);
}

void CPU6502::dex(void) {
	X = (X - 1) & 0xFF;
	fnz(X);
}

void CPU6502::dey(void) {
	Y = (Y - 1) & 0xFF;
	fnz(Y);
}

void CPU6502::eor(void) {
	A ^= read(addr);
	fnz(A);
}

void CPU6502::inc(void) {
	tmp = (read(addr) + 1) & 0xFF;
	fnz(tmp);
}

void CPU6502::inx(void) {
	X = (X + 1) & 0xFF;
	fnz(X);
}

void CPU6502::iny(void) {
	Y = (Y + 1) & 0xFF;
	fnz(Y);
}

void CPU6502::isc(void) {
	uint16_t v = (read(addr) + 1) & 0xFF;
	uint16_t c = 1 - (C ? 1 : 0);
	uint16_t r = A - v - c;
	if (D) {
		uint8_t al = (A & 0x0F) - (v & 0x0F) - c;
		if (al > 0x80) al -= 6;
		uint8_t ah = (A >> 4) - (v >> 4) - ((al > 0x80) ? 1 : 0);
		Z = ((r & 0xFF) == 0);
		N = ((r & 0x80) != 0);
		V = (((A ^ v) & (A ^ r) & 0x80) != 0);
		C = ((r & 0x100) != 0) ? 0 : 1;
		if (ah > 0x80) ah -= 6;
		A = ((ah << 4) | (al & 15)) & 0xFF;
	} else {
		Z = ((r & 0xFF) == 0);
		N = ((r & 0x80) != 0);
		V = (((A ^ v) & (A ^ r) & 0x80) != 0);
		C = ((r & 0x100) != 0) ? 0 : 1;
		A = r & 0xFF;
	}
}


void CPU6502::jmp(void) {
	PC = addr;
	cycles--;
}

void CPU6502::jsr(void) {
	write(S + 0x100, (PC - 1) >> 8);
	S = (S - 1) & 0xFF;
	write(S + 0x100, (PC - 1) & 0xFF);
	S = (S - 1) & 0xFF;
	PC = addr;
	cycles += 2;
}

void CPU6502::las(void) {
	S = X = A = read(addr) & S;
	fnz(A);
}


void CPU6502::lax(void) {
	X = A = read(addr);
	fnz(A);
}


void CPU6502::lda(void) {
	A = read(addr);
	fnz(A);
}

void CPU6502::ldx(void) {
	X = read(addr);
	fnz(X);
}

void CPU6502::ldy(void) {
	Y = read(addr);
	fnz(Y);
}

void CPU6502::ora(void) {
	A |= read(addr);
	fnz(A);
}

void CPU6502::rol(void) {
	tmp = (read(addr) << 1) | (C ? 1 : 0);
	fnzc(tmp);
	tmp &= 0xFF;
}
void CPU6502::rla(void) {
	tmp = (A << 1) | (C ? 1 : 0);
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::ror(void) {
	tmp = read(addr);
	tmp = ((tmp & 1) << 8) | ((C ? 1 : 0) << 7) | (tmp >> 1);
	fnzc(tmp);
	tmp &= 0xFF;
}
void CPU6502::rra(void) {
	tmp = ((A & 1) << 8) | ((C ? 1 : 0) << 7) | (A >> 1);
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::kil(void) {

}

void CPU6502::lsr(void) {
	tmp = read(addr);
	tmp = ((tmp & 1) << 8) | (tmp >> 1);
	fnzc(tmp);
	tmp &= 0xFF;
}
void CPU6502::lsra(void) {
	tmp = ((A & 1) << 8) | (A >> 1);
	fnzc(tmp);
	A = tmp & 0xFF;
}


void CPU6502::nop(void) { }

void CPU6502::pha(void) {
	write(S + 0x100, A);
	S = (S - 1) & 0xFF;
	cycles++;
}

void CPU6502::php(void) {
	uint8_t v = (N ? 1 << 7 : 0);
	v |= (V ? 1 << 6 : 0);
	v |= 3 << 4;
	v |= (D ? 1 << 3 : 0);
	v |= (I ? 1 << 2 : 0);
	v |= (Z ? 1 << 1 : 0);
	v |= (C ? 1 : 0);
	write(S + 0x100, v);
	S = (S - 1) & 0xFF;
	cycles++;
}

void CPU6502::pla(void) {
	S = (S + 1) & 0xFF;
	A = read(S + 0x100);
	fnz(A);
	cycles += 2;
}

void CPU6502::plp(void) {
	S = (S + 1) & 0xFF;
	tmp = read(S + 0x100);
	N = ((tmp & 0x80) != 0);
	V = ((tmp & 0x40) != 0);
	D = ((tmp & 0x08) != 0);
	I = ((tmp & 0x04) != 0);
	Z = ((tmp & 0x02) != 0);
	C = ((tmp & 0x01) != 0);
	cycles += 2;
}

void CPU6502::rti(void) {
	S = (S + 1) & 0xFF;
	tmp = read(S + 0x100);
	N = ((tmp & 0x80) != 0);
	V = ((tmp & 0x40) != 0);
	D = ((tmp & 0x08) != 0);
	I = ((tmp & 0x04) != 0);
	Z = ((tmp & 0x02) != 0);
	C = ((tmp & 0x01) != 0);
	S = (S + 1) & 0xFF;
	PC = read(S + 0x100);
	S = (S + 1) & 0xFF;
	PC |= read(S + 0x100) << 8;
	cycles += 4;
}

void CPU6502::rts(void) {
	S = (S + 1) & 0xFF;
	PC = read(S + 0x100);
	S = (S + 1) & 0xFF;
	PC |= read(S + 0x100) << 8;
	PC++;
	cycles += 4;
}

void CPU6502::sax(void) {
	write(addr, A & X);
}

void CPU6502::sbc(void) {
	uint16_t v = read(addr);
	uint16_t c = 1 - (C ? 1 : 0);
	uint16_t r = A - v - c;
	if (D) {
		uint8_t al = (A & 0x0F) - (v & 0x0F) - c;
		if (al > 0x80) al -= 6;
		uint8_t ah = (A >> 4) - (v >> 4) - ((al > 0x80) ? 1 : 0);
		Z = ((r & 0xFF) == 0);
		N = ((r & 0x80) != 0);
		V = (((A ^ v) & (A ^ r) & 0x80) != 0);
		C = ((r & 0x100) != 0) ? 0 : 1;
		if (ah > 0x80) ah -= 6;
		A = ((ah << 4) | (al & 15)) & 0xFF;
	} else {
		Z = ((r & 0xFF) == 0);
		N = ((r & 0x80) != 0);
		V = (((A ^ v) & (A ^ r) & 0x80) != 0);
		C = ((r & 0x100) != 0) ? 0 : 1;
		A = r & 0xFF;
	}
}

void CPU6502::sbx(void) {
	tmp = read(addr) - (A & X);
	fnzb(tmp);
	X = (tmp & 0xFF);
}

void CPU6502::sec(void) { C = 1; }
void CPU6502::sed(void) { D = 1; }
void CPU6502::sei(void) { I = 1; }

void CPU6502::shs(void) {
	tmp = ((addr >> 8) + 1) & A & X;
	write(addr, tmp & 0xFF);
	S = (tmp & 0xFF); 
}

void CPU6502::shx(void) {
	tmp = ((addr >> 8) + 1) & X;
	write(addr, tmp & 0xFF); 
}

void CPU6502::shy(void) {
	tmp = ((addr >> 8) + 1) & Y;
	write(addr, tmp & 0xFF); 
}


void CPU6502::slo(void) {
	tmp = read(addr) << 1;
	tmp |= A;
	fnzc(tmp);
	A = tmp & 0xFF;
}

void CPU6502::sre(void) {
	uint8_t v = read(addr);
	tmp = ((v & 1) << 8) | (v >> 1);
	tmp ^= A;
	fnzc(tmp);
	A = tmp & 0xFF;
}


void CPU6502::sta(void) {
	write(addr, A);
}

void CPU6502::stx(void) {
	write(addr, X);
}

void CPU6502::sty(void) {
	write(addr, Y);
}

void CPU6502::tax(void) {
	X = A;
	fnz(X);
}

void CPU6502::tay(void) {
	Y = A;
	fnz(Y);
}

void CPU6502::tsx(void) {
	X = S;
	fnz(X);
}

void CPU6502::txa(void) {
	A = X;
	fnz(A);
}

void CPU6502::txs(void) {
	S = X;
}

void CPU6502::tya(void) {
	A = Y;
	fnz(A);
}

////////////////////////////////////////////////////////////////////////////////
// CPU control
////////////////////////////////////////////////////////////////////////////////

void CPU6502::reset(void) {
	A = X = Y = S = 0;
	N = C = V = false;
	Z = true;
	I = D = false;

	opcode = 0x4C;
	PC = (read(0xFFFD) << 8) | read(0xFFFC);
}

void CPU6502::step(void) {
	PC++;
	switch(opcode) {
/*  BRK     */ case 0x00: imp(); brk(); break;
/*  ORA izx */ case 0x01: izx(); ora(); break;
/* *KIL     */ case 0x02: imp(); kil(); break;
/* *SLO izx */ case 0x03: izx(); slo(); rmw(); break;
/* *NOP zp  */ case 0x04: zp(); nop(); break;
/*  ORA zp  */ case 0x05: zp(); ora(); break;
/*  ASL zp  */ case 0x06: zp(); asl(); rmw(); break;
/* *SLO zp  */ case 0x07: zp(); slo(); rmw(); break;
/*  PHP     */ case 0x08: imp(); php(); break;
/*  ORA imm */ case 0x09: imm(); ora(); break;
/*  ASL     */ case 0x0A: imp(); asla(); break;
/* *ANC imm */ case 0x0B: imm(); anc(); break;
/* *NOP abs */ case 0x0C: abs(); nop(); break;
/*  ORA abs */ case 0x0D: abs(); ora(); break;
/*  ASL abs */ case 0x0E: abs(); asl(); rmw(); break;
/* *SLO abs */ case 0x0F: abs(); slo(); rmw(); break;

/*  BPL rel */ case 0x10: rel(); bpl(); break;
/*  ORA izy */ case 0x11: izy(); ora(); break;
/* *KIL     */ case 0x12: imp(); kil(); break;
/* *SLO izy */ case 0x13: izy(); slo(); rmw(); break;
/* *NOP zpx */ case 0x14: zpx(); nop(); break;
/*  ORA zpx */ case 0x15: zpx(); ora(); break;
/*  ASL zpx */ case 0x16: zpx(); asl(); rmw(); break;
/* *SLO zpx */ case 0x17: zpx(); slo(); rmw(); break;
/*  CLC     */ case 0x18: imp(); clc(); break;
/*  ORA aby */ case 0x19: aby(); ora(); break;
/* *NOP     */ case 0x1A: imp(); nop(); break;
/* *SLO aby */ case 0x1B: aby(); slo(); rmw(); break;
/* *NOP abx */ case 0x1C: abx(); nop(); break;
/*  ORA abx */ case 0x1D: abx(); ora(); break;
/*  ASL abx */ case 0x1E: abx(); asl(); rmw(); break;
/* *SLO abx */ case 0x1F: abx(); slo(); rmw(); break;

/*  JSR abs */ case 0x20: abs(); jsr(); break;
/*  AND izx */ case 0x21: izx(); _and(); break;
/* *KIL     */ case 0x22: imp(); kil(); break;
/* *RLA izx */ case 0x23: izx(); rla(); rmw(); break;
/*  BIT zp  */ case 0x24: zp(); bit(); break;
/*  AND zp  */ case 0x25: zp(); _and(); break;
/*  ROL zp  */ case 0x26: zp(); rol(); rmw(); break;
/* *RLA zp  */ case 0x27: zp(); rla(); rmw(); break;
/*  PLP     */ case 0x28: imp(); plp(); break;
/*  AND imm */ case 0x29: imm(); _and(); break;
/*  ROL     */ case 0x2A: imp(); rla(); break;
/* *ANC imm */ case 0x2B: imm(); anc(); break;
/*  BIT abs */ case 0x2C: abs(); bit(); break;
/*  AND abs */ case 0x2D: abs(); _and(); break;
/*  ROL abs */ case 0x2E: abs(); rol(); rmw(); break;
/* *RLA abs */ case 0x2F: abs(); rla(); rmw(); break;

/*  BMI rel */ case 0x30: rel(); bmi(); break;
/*  AND izy */ case 0x31: izy(); _and(); break;
/* *KIL     */ case 0x32: imp(); kil(); break;
/* *RLA izy */ case 0x33: izy(); rla(); rmw(); break;
/* *NOP zpx */ case 0x34: zpx(); nop(); break;
/*  AND zpx */ case 0x35: zpx(); _and(); break;
/*  ROL zpx */ case 0x36: zpx(); rol(); rmw(); break;
/* *RLA zpx */ case 0x37: zpx(); rla(); rmw(); break;
/*  SEC     */ case 0x38: imp(); sec(); break;
/*  AND aby */ case 0x39: aby(); _and(); break;
/* *NOP     */ case 0x3A: imp(); nop(); break;
/* *RLA aby */ case 0x3B: aby(); rla(); rmw(); break;
/* *NOP abx */ case 0x3C: abx(); nop(); break;
/*  AND abx */ case 0x3D: abx(); _and(); break;
/*  ROL abx */ case 0x3E: abx(); rol(); rmw(); break;
/* *RLA abx */ case 0x3F: abx(); rla(); rmw(); break;

/*  RTI     */ case 0x40: imp(); rti(); break;
/*  EOR izx */ case 0x41: izx(); eor(); break;
/* *KIL     */ case 0x42: imp(); kil(); break;
/* *SRE izx */ case 0x43: izx(); sre(); rmw(); break;
/* *NOP zp  */ case 0x44: zp(); nop(); break;
/*  EOR zp  */ case 0x45: zp(); eor(); break;
/*  LSR zp  */ case 0x46: zp(); lsr(); rmw(); break;
/* *SRE zp  */ case 0x47: zp(); sre(); rmw(); break;
/*  PHA     */ case 0x48: imp(); pha(); break;
/*  EOR imm */ case 0x49: imm(); eor(); break;
/*  LSR     */ case 0x4A: imp(); lsra(); break;
/* *ALR imm */ case 0x4B: imm(); alr(); break;
/*  JMP abs */ case 0x4C: abs(); jmp(); break;
/*  EOR abs */ case 0x4D: abs(); eor(); break;
/*  LSR abs */ case 0x4E: abs(); lsr(); rmw(); break;
/* *SRE abs */ case 0x4F: abs(); sre(); rmw(); break;

/*  BVC rel */ case 0x50: rel(); bvc(); break;
/*  EOR izy */ case 0x51: izy(); eor(); break;
/* *KIL     */ case 0x52: imp(); kil(); break;
/* *SRE izy */ case 0x53: izy(); sre(); rmw(); break;
/* *NOP zpx */ case 0x54: zpx(); nop(); break;
/*  EOR zpx */ case 0x55: zpx(); eor(); break;
/*  LSR zpx */ case 0x56: zpx(); lsr(); rmw(); break;
/* *SRE zpx */ case 0x57: zpx(); sre(); rmw(); break;
/*  CLI     */ case 0x58: imp(); cli(); break;
/*  EOR aby */ case 0x59: aby(); eor(); break;
/* *NOP     */ case 0x5A: imp(); nop(); break;
/* *SRE aby */ case 0x5B: aby(); sre(); rmw(); break;
/* *NOP abx */ case 0x5C: abx(); nop(); break;
/*  EOR abx */ case 0x5D: abx(); eor(); break;
/*  LSR abx */ case 0x5E: abx(); lsr(); rmw(); break;
/* *SRE abx */ case 0x5F: abx(); sre(); rmw(); break;

/*  RTS     */ case 0x60: imp(); rts(); break;
/*  ADC izx */ case 0x61: izx(); adc(); break;
/* *KIL     */ case 0x62: imp(); kil(); break;
/* *RRA izx */ case 0x63: izx(); rra(); rmw(); break;
/* *NOP zp  */ case 0x64: zp(); nop(); break;
/*  ADC zp  */ case 0x65: zp(); adc(); break;
/*  ROR zp  */ case 0x66: zp(); ror(); rmw(); break;
/* *RRA zp  */ case 0x67: zp(); rra(); rmw(); break;
/*  PLA     */ case 0x68: imp(); pla(); break;
/*  ADC imm */ case 0x69: imm(); adc(); break;
/*  ROR     */ case 0x6A: imp(); rra(); break;
/* *ARR imm */ case 0x6B: imm(); arr(); break;
/*  JMP ind */ case 0x6C: ind(); jmp(); break;
/*  ADC abs */ case 0x6D: abs(); adc(); break;
/*  ROR abs */ case 0x6E: abs(); ror(); rmw(); break;
/* *RRA abs */ case 0x6F: abs(); rra(); rmw(); break;

/*  BVS rel */ case 0x70: rel(); bvs(); break;
/*  ADC izy */ case 0x71: izy(); adc(); break;
/* *KIL     */ case 0x72: imp(); kil(); break;
/* *RRA izy */ case 0x73: izy(); rra(); rmw(); break;
/* *NOP zpx */ case 0x74: zpx(); nop(); break;
/*  ADC zpx */ case 0x75: zpx(); adc(); break;
/*  ROR zpx */ case 0x76: zpx(); ror(); rmw(); break;
/* *RRA zpx */ case 0x77: zpx(); rra(); rmw(); break;
/*  SEI     */ case 0x78: imp(); sei(); break;
/*  ADC aby */ case 0x79: aby(); adc(); break;
/* *NOP     */ case 0x7A: imp(); nop(); break;
/* *RRA aby */ case 0x7B: aby(); rra(); rmw(); break;
/* *NOP abx */ case 0x7C: abx(); nop(); break;
/*  ADC abx */ case 0x7D: abx(); adc(); break;
/*  ROR abx */ case 0x7E: abx(); ror(); rmw(); break;
/* *RRA abx */ case 0x7F: abx(); rra(); rmw(); break;

/* *NOP imm */ case 0x80: imm(); nop(); break;
/*  STA izx */ case 0x81: izx(); sta(); break;
/* *NOP imm */ case 0x82: imm(); nop(); break;
/* *SAX izx */ case 0x83: izx(); sax(); break;
/*  STY zp  */ case 0x84: zp(); sty(); break;
/*  STA zp  */ case 0x85: zp(); sta(); break;
/*  STX zp  */ case 0x86: zp(); stx(); break;
/* *SAX zp  */ case 0x87: zp(); sax(); break;
/*  DEY     */ case 0x88: imp(); dey(); break;
/* *NOP imm */ case 0x89: imm(); nop(); break;
/*  TXA     */ case 0x8A: imp(); txa(); break;
/* *ANE imm */ case 0x8B: imm(); ane(); break;
/*  STY abs */ case 0x8C: abs(); sty(); break;
/*  STA abs */ case 0x8D: abs(); sta(); break;
/*  STX abs */ case 0x8E: abs(); stx(); break;
/* *SAX abs */ case 0x8F: abs(); sax(); break;

/*  BCC rel */ case 0x90: rel(); bcc(); break;
/*  STA izy */ case 0x91: izy(); sta(); break;
/* *KIL     */ case 0x92: imp(); kil(); break;
/* *AHX izy */ case 0x93: izy(); ahx(); break;
/*  STY zpx */ case 0x94: zpx(); sty(); break;
/*  STA zpx */ case 0x95: zpx(); sta(); break;
/*  STX zpy */ case 0x96: zpy(); stx(); break;
/* *SAX zpy */ case 0x97: zpy(); sax(); break;
/*  TYA     */ case 0x98: imp(); tya(); break;
/*  STA aby */ case 0x99: aby(); sta(); break;
/*  TXS     */ case 0x9A: imp(); txs(); break;
/* *SHS aby */ case 0x9B: aby(); shs(); break;
/* *SHY abx */ case 0x9C: abx(); shy(); break;
/*  STA abx */ case 0x9D: abx(); sta(); break;
/* *SHX aby */ case 0x9E: aby(); shx(); break;
/* *AHX aby */ case 0x9F: aby(); ahx(); break;

/*  LDY imm */ case 0xA0: imm(); ldy(); break;
/*  LDA izx */ case 0xA1: izx(); lda(); break;
/*  LDX imm */ case 0xA2: imm(); ldx(); break;
/* *LAX izx */ case 0xA3: izx(); lax(); break;
/*  LDY zp  */ case 0xA4: zp(); ldy(); break;
/*  LDA zp  */ case 0xA5: zp(); lda(); break;
/*  LDX zp  */ case 0xA6: zp(); ldx(); break;
/* *LAX zp  */ case 0xA7: zp(); lax(); break;
/*  TAY     */ case 0xA8: imp(); tay(); break;
/*  LDA imm */ case 0xA9: imm(); lda(); break;
/*  TAX     */ case 0xAA: imp(); tax(); break;
/* *LAX imm */ case 0xAB: imm(); lax(); break;
/*  LDY abs */ case 0xAC: abs(); ldy(); break;
/*  LDA abs */ case 0xAD: abs(); lda(); break;
/*  LDX abs */ case 0xAE: abs(); ldx(); break;
/* *LAX abs */ case 0xAF: abs(); lax(); break;

/*  BCS rel */ case 0xB0: rel(); bcs(); break;
/*  LDA izy */ case 0xB1: izy(); lda(); break;
/* *KIL     */ case 0xB2: imp(); kil(); break;
/* *LAX izy */ case 0xB3: izy(); lax(); break;
/*  LDY zpx */ case 0xB4: zpx(); ldy(); break;
/*  LDA zpx */ case 0xB5: zpx(); lda(); break;
/*  LDX zpy */ case 0xB6: zpy(); ldx(); break;
/* *LAX zpy */ case 0xB7: zpy(); lax(); break;
/*  CLV     */ case 0xB8: imp(); clv(); break;
/*  LDA aby */ case 0xB9: aby(); lda(); break;
/*  TSX     */ case 0xBA: imp(); tsx(); break;
/* *LAS aby */ case 0xBB: aby(); las(); break;
/*  LDY abx */ case 0xBC: abx(); ldy(); break;
/*  LDA abx */ case 0xBD: abx(); lda(); break;
/*  LDX aby */ case 0xBE: aby(); ldx(); break;
/* *LAX aby */ case 0xBF: aby(); lax(); break;

/*  CPY imm */ case 0xC0: imm(); cpy(); break;
/*  CMP izx */ case 0xC1: izx(); cmp(); break;
/* *NOP imm */ case 0xC2: imm(); nop(); break;
/* *DCP izx */ case 0xC3: izx(); dcp(); rmw(); break;
/*  CPY zp  */ case 0xC4: zp(); cpy(); break;
/*  CMP zp  */ case 0xC5: zp(); cmp(); break;
/*  DEC zp  */ case 0xC6: zp(); dec(); rmw(); break;
/* *DCP zp  */ case 0xC7: zp(); dcp(); rmw(); break;
/*  INY     */ case 0xC8: imp(); iny(); break;
/*  CMP imm */ case 0xC9: imm(); cmp(); break;
/*  DEX     */ case 0xCA: imp(); dex(); break;
/* *SBX imm */ case 0xCB: imm(); sbx(); break;
/*  CPY abs */ case 0xCC: abs(); cpy(); break;
/*  CMP abs */ case 0xCD: abs(); cmp(); break;
/*  DEC abs */ case 0xCE: abs(); dec(); rmw(); break;
/* *DCP abs */ case 0xCF: abs(); dcp(); rmw(); break;

/*  BNE rel */ case 0xD0: rel(); bne(); break;
/*  CMP izy */ case 0xD1: izy(); cmp(); break;
/* *KIL     */ case 0xD2: imp(); kil(); break;
/* *DCP izy */ case 0xD3: izy(); dcp(); rmw(); break;
/* *NOP zpx */ case 0xD4: zpx(); nop(); break;
/*  CMP zpx */ case 0xD5: zpx(); cmp(); break;
/*  DEC zpx */ case 0xD6: zpx(); dec(); rmw(); break;
/* *DCP zpx */ case 0xD7: zpx(); dcp(); rmw(); break;
/*  CLD     */ case 0xD8: imp(); cld(); break;
/*  CMP aby */ case 0xD9: aby(); cmp(); break;
/* *NOP     */ case 0xDA: imp(); nop(); break;
/* *DCP aby */ case 0xDB: aby(); dcp(); rmw(); break;
/* *NOP abx */ case 0xDC: abx(); nop(); break;
/*  CMP abx */ case 0xDD: abx(); cmp(); break;
/*  DEC abx */ case 0xDE: abx(); dec(); rmw(); break;
/* *DCP abx */ case 0xDF: abx(); dcp(); rmw(); break;

/*  CPX imm */ case 0xE0: imm(); cpx(); break;
/*  SBC izx */ case 0xE1: izx(); sbc(); break;
/* *NOP imm */ case 0xE2: imm(); nop(); break;
/* *ISC izx */ case 0xE3: izx(); isc(); rmw(); break;
/*  CPX zp  */ case 0xE4: zp(); cpx(); break;
/*  SBC zp  */ case 0xE5: zp(); sbc(); break;
/*  INC zp  */ case 0xE6: zp(); inc(); rmw(); break;
/* *ISC zp  */ case 0xE7: zp(); isc(); rmw(); break;
/*  INX     */ case 0xE8: imp(); inx(); break;
/*  SBC imm */ case 0xE9: imm(); sbc(); break;
/*  NOP     */ case 0xEA: imp(); nop(); break;
/* *SBC imm */ case 0xEB: imm(); sbc(); break;
/*  CPX abs */ case 0xEC: abs(); cpx(); break;
/*  SBC abs */ case 0xED: abs(); sbc(); break;
/*  INC abs */ case 0xEE: abs(); inc(); rmw(); break;
/* *ISC abs */ case 0xEF: abs(); isc(); rmw(); break;

/*  BEQ rel */ case 0xF0: rel(); beq(); break;
/*  SBC izy */ case 0xF1: izy(); sbc(); break;
/* *KIL     */ case 0xF2: imp(); kil(); break;
/* *ISC izy */ case 0xF3: izy(); isc(); rmw(); break;
/* *NOP zpx */ case 0xF4: zpx(); nop(); break;
/*  SBC zpx */ case 0xF5: zpx(); sbc(); break;
/*  INC zpx */ case 0xF6: zpx(); inc(); rmw(); break;
/* *ISC zpx */ case 0xF7: zpx(); isc(); rmw(); break;
/*  SED     */ case 0xF8: imp(); sed(); break;
/*  SBC aby */ case 0xF9: aby(); sbc(); break;
/* *NOP     */ case 0xFA: imp(); nop(); break;
/* *ISC aby */ case 0xFB: aby(); isc(); rmw(); break;
/* *NOP abx */ case 0xFC: abx(); nop(); break;
/*  SBC abx */ case 0xFD: abx(); sbc(); break;
/*  INC abx */ case 0xFE: abx(); inc(); rmw(); break;
/* *ISC abx */ case 0xFF: abx(); isc(); rmw(); break;
	}
	opcode = read(PC);
}

void CPU6502::log(FILE *stream) {
	fprintf(stream, "nPC=%04X cyc=%09d [%02X] %c%c%c%c%c%c A=%02X X=%02X Y=%02X S=%02X\n",
		PC, cycles % 1000000000, opcode, 
		(C ? 'C' : '-'),
		(N ? 'N' : '-'),
		(Z ? 'Z' : '-'),
		(V ? 'V' : '-'),
		(D ? 'D' : '-'),
		(I ? 'I' : '-'), 
		A, X, Y, S);
}
