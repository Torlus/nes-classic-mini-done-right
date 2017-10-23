#ifndef NES_PPU_INCLUDED
#define NES_PPU_INCLUDED

#include <cstdint>
#include <cstdio>

class PPU {
public:
    PPU(void);

    uint32_t cycles;

    void reset(void);
    void step(void);
    void log(FILE *stream);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    
    uint8_t (*mem_read)(uint16_t address);
    void (*mem_write)(uint16_t address, uint8_t data);

private:
    // PPUCTRL
    uint16_t nt_base;
    bool add32;
    uint16_t sppt_base;
    uint16_t bgpt_base;
    bool ssz16;
    bool bdout;
    bool nmi_vbl;
    
    // PPUMASK
    bool grayscale;
    bool showbg_left;
    bool showsp_left;
    bool showbg;
    bool showsp;
    bool r_em;
    bool g_em;
    bool b_em;

    // PPUSTATUS
    uint8_t last_write;
    bool sp_ovf;
    bool sp0_hit;
    bool vbl;

    // OAMADDR
    uint8_t oam_addr;

    // OAMDATA
    uint8_t oam_data;

    // For 16-bit registers
    bool second_write;

    // PPUSCROLL
    uint8_t scroll_x;
    uint8_t scroll_y;

    // PPUADDR
    uint16_t ppu_addr;

    // PPUDATA
    uint8_t ppu_data;

    // OAMDMA
    uint16_t oam_dma_base;

    // OAM
    uint8_t OAM[256];

};

#endif // NES_PPU_INCLUDED
