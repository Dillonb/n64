#ifndef N64_DISASSEMBLE_H
#define N64_DISASSEMBLE_H

#include <util.h>

#ifdef __cplusplus
#include <string>
enum class DisassemblyArch {
    HOST,
    GUEST,
    GUEST_RSP
};
std::string disassemble_multi(DisassemblyArch arch, uintptr_t address, u8 *code, size_t code_size);
extern "C" {
#endif  // __cplusplus

#include <util.h>

int disassemble(u32 address, u32 raw, char *buf, int buflen);
int disassemble_rsp(u32 address, u32 raw, char *buf, int buflen);
void print_multi_host(uintptr_t address, u8 *code, size_t code_size);
void print_multi_guest(uintptr_t address, u8 *code, size_t code_size);
void print_multi_guest_rsp(uintptr_t address, u8 *code, size_t code_size);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif //N64_DISASSEMBLE_H
