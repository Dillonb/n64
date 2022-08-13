#ifndef N64_RDP_H
#define N64_RDP_H
#include <system/n64system.h>

#ifdef __cplusplus
extern "C" {
#endif

void load_rdp_plugin(const char* filename);
void write_word_dpcreg(u32 address, u32 value);
u32 read_word_dpcreg(u32 address);
void rdp_run_command();
void rdp_update_screen();
void rdp_status_reg_write(u32 value);

#ifdef __cplusplus
}
#endif
#endif //N64_RDP_H
