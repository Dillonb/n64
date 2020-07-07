#ifndef N64_RDP_H
#define N64_RDP_H
#include "../system/n64system.h"
void load_rdp_plugin(n64_system_t* system, const char* filename);
void write_word_dpcreg(n64_system_t* system, word address, word value);
void rdp_cleanup();
void rdp_run_command();
#endif //N64_RDP_H
