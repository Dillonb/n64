#ifndef RSP_DISASSEMBLE_H
#define RSP_DISASSEMBLE_H
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <util.h>

int disassemble_rsp_unique(u32 address, u32 raw, char* buf, int buflen);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif //RSP_DISASSEMBLE_H