#ifndef N64_SI_H
#define N64_SI_H
void pif_to_dram(word pif_address, word dram_address);
void dram_to_pif(word dram_address, word pif_address);
void on_si_dma_complete();
void write_word_sireg(word address, word value);
word read_word_sireg(word address);

#endif //N64_SI_H
