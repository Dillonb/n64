#include <stdlib.h>
#include <string.h>
#include <system/n64system.h>
#include <cpu/rsp.h>
#include <cpu/rsp_vector_instructions.h>

void print_vureg_comparing_ln(vu_reg_t* reg, const half* compare) {
    printf("Actual:   ");
    for (int i = 0; i < 8; i++) {
        if (compare[i] != reg->elements[i]) {
            printf(COLOR_RED);
        }
        printf("%04X ", reg->elements[i]);
        if (compare[i] != reg->elements[i]) {
            printf(COLOR_END);
        }
    }
    printf("\n");
}

void print_expected(const half* expected) {
    printf("Expected: ");
    for (int i = 0; i < 8; i++) {
        printf("%04X ", expected[i]);
    }
    printf("\n");
}

int main(int argc, char** argv) {
    init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, false);

    half initial_acch[] = { 0x7FFF, 0x7C5B, 0x77E8, 0x7443, 0x6C95, 0x68F2, 0x647F, 0x60DB };
    half initial_accm[] = { 0xF060, 0x609F, 0x125A, 0xB17A, 0xF5C7, 0x0249, 0x8EC8, 0xCA2A };
    half initial_accl[] = { 0x7E67, 0x78E5, 0x885D, 0x465B, 0xF40D, 0x7A2B, 0x19E8, 0x63A3 };
    half initial_vs[]   = { 0x75A7, 0x75A6, 0x75A5, 0x75A4, 0x75A3, 0x75A2, 0x75A1, 0x75A0 };
    half initial_vt[]   = { 0x75AF, 0x75AE, 0x75AD, 0x75AC, 0x75AB, 0x75AA, 0x75A9, 0x75A8 };


    half expected_acch[] = { 0x8000, 0x7C5B, 0x77E8, 0x7443, 0x6C96, 0x68F2, 0x647F, 0x60DC };
    half expected_accm[] = { 0x2674, 0x96B3, 0x486D, 0xE78D, 0x2BDA, 0x385B, 0xC4D9, 0x003B };
    half expected_accl[] = { 0xDD9B, 0x626D, 0xFC39, 0x448B, 0x7C91, 0x8D03, 0xB714, 0x8B23 };
    half expected_vd[]  = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF };


    memcpy(N64RSP.acc.h.elements, initial_acch, sizeof(half) * 8);
    memcpy(N64RSP.acc.m.elements, initial_accm, sizeof(half) * 8);
    memcpy(N64RSP.acc.l.elements, initial_accl, sizeof(half) * 8);

    memcpy(N64RSP.vu_regs[1].elements, initial_vs, sizeof(half) * 8);
    memcpy(N64RSP.vu_regs[2].elements, initial_vt, sizeof(half) * 8);


    mips_instruction_t instr;
    instr.cp2_vec.vs = 1;
    instr.cp2_vec.vt = 2;
    instr.cp2_vec.vd = 3;
    instr.cp2_vec.e  = 12;

    rsp_vec_vmadm(instr);

    for (int e = 0; e < 8; e++) {
        if (expected_vd[e] != N64RSP.vu_regs[3].elements[e]) {
            print_expected(expected_vd);
            print_vureg_comparing_ln(&N64RSP.vu_regs[3], expected_vd);
#ifdef N64_USE_SIMD
            if (expected_vd[e] == 0x7FFF && N64RSP.vu_regs[3].elements[e] == 0x8000) {
                printf("this is expected, the SIMD implementation is buggy!\n");
            }
#else
            logfatal("VD mismatch!");
#endif
        }
        if (expected_acch[e] != N64RSP.acc.h.elements[e]) {
            print_expected(expected_acch);
            print_vureg_comparing_ln(&N64RSP.acc.h, expected_vd);
            logfatal("acch mismatch!");
        }
        if (expected_accm[e] != N64RSP.acc.m.elements[e]) {
            print_expected(expected_accm);
            print_vureg_comparing_ln(&N64RSP.acc.m, expected_vd);
            logfatal("accm mismatch!");
        }
        if (expected_accl[e] != N64RSP.acc.l.elements[e]) {
            print_expected(expected_accl);
            print_vureg_comparing_ln(&N64RSP.acc.l, expected_vd);
            logfatal("accl mismatch!");
        }
    }

    printf("Passed!\n");
}
