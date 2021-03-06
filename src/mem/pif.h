#ifndef N64_PIF_H
#define N64_PIF_H

#include <system/n64system.h>

#ifdef __cplusplus
extern "C" {
#endif

void pif_rom_execute();
void process_pif_command();
void load_pif_rom(const char* pif_rom_path);

#ifdef __cplusplus
}
#endif
#endif //N64_PIF_H
