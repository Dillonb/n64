#include <string.h>
#include <system/n64system.h>
#include <cpu/rsp.h>
#include <cpu/rsp_vector_instructions.h>

bool print_vureg_comparing_ln(vu_reg_t* reg, const u16* compare) {
    bool failed = false;
    printf("Expected: ");
    for (int i = 0; i < 8; i++) {
        printf("%04X ", compare[i]);
    }
    printf("\n");
    printf("Actual:   ");
    for (int i = 0; i < 8; i++) {
        if (compare[i] != reg->elements[i]) {
            failed = true;
            printf(COLOR_RED);
        }
        printf("%04X ", reg->elements[i]);
        if (compare[i] != reg->elements[i]) {
            printf(COLOR_END);
        }
    }
    printf("\n");
    return failed;
}

int main(int argc, char** argv) {
    init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, false);

    u16 initial_acch[] = {0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF };
    u16 initial_accm[] = {0xF060, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    u16 initial_accl[] = {0x7E67, 0xFFFC, 0xFFFC, 0xFFFC, 0xFFFC, 0xFFFC, 0xFFFC, 0xFFFC };
    u16 initial_vs[]   = {0x75A7, 0x08AA, 0x08AA, 0x08AA, 0x08AA, 0x08AA, 0x08AA, 0x08AA };
    u16 initial_vt[]   = {0x75AC, 0x08BB, 0x08BB, 0x08BB, 0x08BB, 0x08BB, 0x08BB, 0x08BB };

    u16 expected_vd[]   = {0x7FFF, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000 };
    u16 expected_acch[] = {0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000 };
    u16 expected_accm[] = {0x2674, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B };
    u16 expected_accl[] = {0xDD9B, 0xA42A, 0xA42A, 0xA42A, 0xA42A, 0xA42A, 0xA42A, 0xA42A };


    memcpy(N64RSP.acc.h.elements, initial_acch, sizeof(u16) * 8);
    memcpy(N64RSP.acc.m.elements, initial_accm, sizeof(u16) * 8);
    memcpy(N64RSP.acc.l.elements, initial_accl, sizeof(u16) * 8);

    memcpy(N64RSP.vu_regs[1].elements, initial_vs, sizeof(u16) * 8);
    memcpy(N64RSP.vu_regs[2].elements, initial_vt, sizeof(u16) * 8);


    mips_instruction_t instr;
    instr.cp2_vec.vs = 1;
    instr.cp2_vec.vt = 2;
    instr.cp2_vec.vd = 3;
    instr.cp2_vec.e  = 0;

    rsp_vec_vmadm(instr);

    bool failed = false;

    printf("VD\n");
    failed |= print_vureg_comparing_ln(&N64RSP.vu_regs[3], expected_vd);
    printf("acc.h\n");
    failed |= print_vureg_comparing_ln(&N64RSP.acc.h, expected_acch);
    printf("acc.m\n");
    failed |= print_vureg_comparing_ln(&N64RSP.acc.m, expected_accm);
    printf("acc.l\n");
    failed |= print_vureg_comparing_ln(&N64RSP.acc.l, expected_accl);

    if (failed) {
        //logfatal("Tests failed!");
    } else {
        printf("Passed!\n");
    }
}
