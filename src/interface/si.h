#ifndef N64_SI_H
#define N64_SI_H
void pif_to_dram(u32 pif_address, u32 dram_address);
void dram_to_pif(u32 dram_address, u32 pif_address);
void on_si_dma_complete();
void write_word_sireg(u32 address, u32 value);
u32 read_word_sireg(u32 address);

#endif //N64_SI_H
