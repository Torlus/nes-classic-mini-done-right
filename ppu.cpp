#include <cstdint>
#include <cstdio>
#include "ppu.h"

PPU::PPU(void) {
    cycles = 0;
}

void PPU::reset(void) {
    // PPUCTRL
    nt_base = 0x2000;
    add32 = false;
    sppt_base = 0x0000;
    bgpt_base = 0x0000;
    ssz16 = false;
    bdout = false;
    nmi_vbl = false;

    // PPUMASK
    grayscale = false;
    showbg_left = false;
    showsp_left = false;
    showbg = false;
    showsp = false;
    r_em = false;
    g_em = false;
    b_em = false;

    // PPUSTATUS
    last_write = 0x00;
    sp_ovf = false;
    sp0_hit = false;
    vbl = false;

    // OAMADDR
    oam_addr = 0x00;

    // OAMDATA
    oam_data = 0x00;

    // For 16-bit registers
    second_write = false;

    // PPUSCROLL
    scroll_x = 0x00;
    scroll_y = 0x00;

    // PPUADDR
    ppu_addr = 0x0000;

    // PPUDATA
    ppu_data = 0x00;

    // OAMDMA
    oam_dma_base = 0x0000;
}

void PPU::write(uint16_t address, uint8_t data) {
    if (address == 0x4014) {
        oam_dma_base = data;
        // TODO: Implement DMA transfer  
    } else if ((address & 0xE000) == 0x2000) {
        last_write = data;
        switch(address & 7) {
        case 0x0:
            nt_base = 0x2000 | ((data & 3) << 10);
            add32 = ( (data & (1 << 2)) != 0 );
            sppt_base = ( data & (1 << 3) ? 0x1000 : 0x0000);
            bgpt_base = ( data & (1 << 4) ? 0x1000 : 0x0000);
            ssz16 = ( (data & (1 << 5)) != 0 );
            bdout = ( (data & (1 << 6)) != 0 );
            nmi_vbl = ( (data & (1 << 7)) != 0 );
            break;
        case 0x1:
            grayscale = ( (data & (1 << 0)) != 0 );
            showbg_left = ( (data & (1 << 1)) != 0 );
            showsp_left = ( (data & (1 << 2)) != 0 );
            showbg = ( (data & (1 << 3)) != 0 );
            showsp = ( (data & (1 << 4)) != 0 );
            r_em = ( (data & (1 << 5)) != 0 );
            g_em = ( (data & (1 << 6)) != 0 );
            b_em = ( (data & (1 << 7)) != 0 );
            break;
        case 0x3:
            oam_addr = data;
            break;
        case 0x4:
            OAM[oam_addr++] = data;
            break;
        case 0x5:
            if (second_write) {
                scroll_y = data; 
            } else {
                scroll_x = data;
            }
            second_write = !second_write;
            break;
        case 0x6:
            if (second_write) {
                ppu_addr = (ppu_addr & 0xFF00) | data; 
            } else {
                ppu_addr = (ppu_addr & 0x00FF) | ((uint16_t)data << 8);
            }
            second_write = !second_write;
            break;
        case 0x7:
            mem_write(ppu_addr, data);
            if (add32) {
                ppu_addr += 32;
            } else {
                ppu_addr += 1;
            }
            break;
        default:
            fprintf(stderr, "PPU: write to a RO register @%04X : %02X\n", address, data);
            break;
        }
    } else {
        fprintf(stderr, "PPU: unmapped write @%04X : %02X\n", address, data);
    }
}

uint8_t PPU::read(uint16_t address) {
    uint8_t data = 0xFF;
    if ((address & 0xE000) == 0x2000) {
        switch(address & 7) {
        case 0x2:
            data = last_write & 0x1F;
            data |= (sp_ovf ? (1 << 5) : 0);
            data |= (sp0_hit ? (1 << 6) : 0);
            data |= (vbl ? (1 << 7) : 0);
            break;
        case 0x4:
            data = OAM[oam_addr];
            break;
        case 0x7:
            data = mem_read(ppu_addr); // FIXME
            if (add32) {
                ppu_addr += 32;
            } else {
                ppu_addr += 1;
            }
            break;            
        default:
            fprintf(stderr, "PPU: read from a WO register @%04X\n", address);
            break;
        }
    } else {
        fprintf(stderr, "PPU: unmapped read @%04X\n", address);
    }
    return data;
}