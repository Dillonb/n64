#ifndef N64_RDP_H
#define N64_RDP_H
#include <system/n64system.h>
#include "mupen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

void load_rdp_plugin(n64_system_t* system, const char* filename);
void write_word_dpcreg(n64_system_t* system, word address, word value);
word read_word_dpcreg(n64_system_t* system, word address);
void rdp_cleanup();
void rdp_run_command(n64_system_t* system);
void rdp_update_screen(n64_system_t* system);
void rdp_status_reg_write(n64_system_t* system, word value);
GFX_INFO get_gfx_info(n64_system_t* system);

#ifdef __cplusplus
}
#endif
#endif //N64_RDP_H
