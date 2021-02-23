#ifndef N64_RDP_H
#define N64_RDP_H
#include <system/n64system.h>
#include "mupen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

void load_rdp_plugin(const char* filename);
void write_word_dpcreg(word address, word value);
word read_word_dpcreg(word address);
void rdp_cleanup();
void rdp_run_command();
void rdp_update_screen();
void rdp_status_reg_write(word value);
GFX_INFO get_gfx_info();

#ifdef __cplusplus
}
#endif
#endif //N64_RDP_H
