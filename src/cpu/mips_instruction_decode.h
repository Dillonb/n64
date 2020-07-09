#ifndef N64_MIPS_INSTRUCTION_DECODE_H
#define N64_MIPS_INSTRUCTION_DECODE_H

#include <stdbool.h>
#include "../common/util.h"

typedef union mips_instruction {
    word raw;

    struct {
        unsigned:26;
        bool op5:1;
        bool op4:1;
        bool op3:1;
        bool op2:1;
        bool op1:1;
        bool op0:1;
    };

    struct {
        unsigned:26;
        unsigned op:6;
    };

    struct {
        unsigned immediate:16;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } i;

    struct {
        unsigned offset:16;
        unsigned ft:5;
        unsigned base:5;
        unsigned op:6;
    } fi;

    struct {
        unsigned target:26;
        unsigned op:6;
    } j;

    struct {
        unsigned funct:6;
        unsigned sa:5;
        unsigned rd:5;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } r;

    struct {
        unsigned funct:6;
        unsigned fd:5;
        unsigned fs:5;
        unsigned ft:5;
        unsigned fmt:5;
        unsigned op:6;
    } fr;

    struct {
        unsigned offset:7;
        unsigned element:4;
        unsigned funct:5;
        unsigned vt:5;
        unsigned base:5;
        unsigned op:6;
    } lwc2; // TODO is this format used by other instruction classes?

    struct {
        unsigned funct5:1;
        unsigned funct4:1;
        unsigned funct3:1;
        unsigned funct2:1;
        unsigned funct1:1;
        unsigned funct0:1;
        unsigned:26;
    };

    struct {
        unsigned:16;
        unsigned rt4:1;
        unsigned rt3:1;
        unsigned rt2:1;
        unsigned rt1:1;
        unsigned rt0:1;
        unsigned:11;
    };

    struct {
        unsigned:21;
        unsigned rs4:1;
        unsigned rs3:1;
        unsigned rs2:1;
        unsigned rs1:1;
        unsigned rs0:1;
        unsigned:6;
    };

    struct {
        unsigned last11:11;
        unsigned:21;
    };

} mips_instruction_t;

#endif //N64_MIPS_INSTRUCTION_DECODE_H
