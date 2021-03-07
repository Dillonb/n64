#ifndef N64_MIPS_INSTRUCTION_DECODE_H
#define N64_MIPS_INSTRUCTION_DECODE_H

#include <stdbool.h>
#include <util.h>

typedef union mips_instruction {
    word raw;

    struct {
#ifdef N64_BIG_ENDIAN
        bool op0:1;
        bool op1:1;
        bool op2:1;
        bool op3:1;
        bool op4:1;
        bool op5:1;
        unsigned:26;
#else
        unsigned:26;
        bool op5:1;
        bool op4:1;
        bool op3:1;
        bool op2:1;
        bool op1:1;
        bool op0:1;
#endif
    };

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned:26;
#else
        unsigned:26;
        unsigned op:6;
#endif
    };

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned rs:5;
        unsigned rt:5;
        unsigned immediate:16;
#else
        unsigned immediate:16;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
#endif
    } i;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned base:5;
        unsigned ft:5;
        unsigned offset:16;
#else
        unsigned offset:16;
        unsigned ft:5;
        unsigned base:5;
        unsigned op:6;
#endif
    } fi;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned target:26;
#else
        unsigned target:26;
        unsigned op:6;
#endif
    } j;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned rs:5;
        unsigned rt:5;
        unsigned rd:5;
        unsigned sa:5;
        unsigned funct:6;
#else
        unsigned funct:6;
        unsigned sa:5;
        unsigned rd:5;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
#endif
    } r;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned fmt:5;
        unsigned ft:5;
        unsigned fs:5;
        unsigned fd:5;
        unsigned funct:6;
#else
        unsigned funct:6;
        unsigned fd:5;
        unsigned fs:5;
        unsigned ft:5;
        unsigned fmt:5;
        unsigned op:6;
#endif
    } fr;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned base:5;
        unsigned vt:5;
        unsigned funct:5;
        unsigned element:4;
        unsigned offset:7;
#else
        unsigned offset:7;
        unsigned element:4;
        unsigned funct:5;
        unsigned vt:5;
        unsigned base:5;
        unsigned op:6;
#endif
    } v;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        bool is_vec:1;
        unsigned e:4;
        unsigned vt:5;
        unsigned vs:5;
        unsigned vd:5;
        unsigned funct:6;
#else
        unsigned funct:6;
        unsigned vd:5;
        unsigned vs:5;
        unsigned vt:5;
        unsigned e:4;
        bool is_vec:1;
        unsigned op:6;
#endif
    } cp2_vec;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned op:6;
        unsigned funct:5;
        unsigned rt:5;
        unsigned rd:5;
        unsigned e:4;
        unsigned:7;
#else
        unsigned:7;
        unsigned e:4;
        unsigned rd:5;
        unsigned rt:5;
        unsigned funct:5;
        unsigned op:6;
#endif
    } cp2_regmove;

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned:26;
        unsigned funct0:1;
        unsigned funct1:1;
        unsigned funct2:1;
        unsigned funct3:1;
        unsigned funct4:1;
        unsigned funct5:1;
#else
        unsigned funct5:1;
        unsigned funct4:1;
        unsigned funct3:1;
        unsigned funct2:1;
        unsigned funct1:1;
        unsigned funct0:1;
        unsigned:26;
#endif
    };

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned:11;
        unsigned rt0:1;
        unsigned rt1:1;
        unsigned rt2:1;
        unsigned rt3:1;
        unsigned rt4:1;
        unsigned:16;
#else
        unsigned:16;
        unsigned rt4:1;
        unsigned rt3:1;
        unsigned rt2:1;
        unsigned rt1:1;
        unsigned rt0:1;
        unsigned:11;
#endif
    };

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned:6;
        unsigned rs0:1;
        unsigned rs1:1;
        unsigned rs2:1;
        unsigned rs3:1;
        unsigned rs4:1;
        unsigned:21;
#else
        unsigned:21;
        unsigned rs4:1;
        unsigned rs3:1;
        unsigned rs2:1;
        unsigned rs1:1;
        unsigned rs0:1;
        unsigned:6;
#endif
    };

    struct {
#ifdef N64_BIG_ENDIAN
        unsigned:21;
        unsigned last11:11;
#else
        unsigned last11:11;
        unsigned:21;
#endif
    };

} PACKED mips_instruction_t;

#endif //N64_MIPS_INSTRUCTION_DECODE_H
