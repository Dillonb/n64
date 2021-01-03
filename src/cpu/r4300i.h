#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <util.h>
#include <log.h>
#include "mips_instruction_decode.h"

// Exceptions
#define EXCEPTION_INTERRUPT            0
#define EXCEPTION_COPROCESSOR_UNUSABLE 11

#define R4300I_REG_LR 31

#define R4300I_CP0_REG_INDEX    0
#define R4300I_CP0_REG_RANDOM   1
#define R4300I_CP0_REG_ENTRYLO0 2
#define R4300I_CP0_REG_ENTRYLO1 3
#define R4300I_CP0_REG_CONTEXT  4
#define R4300I_CP0_REG_PAGEMASK 5
#define R4300I_CP0_REG_WIRED    6
#define R4300I_CP0_REG_7        7
#define R4300I_CP0_REG_BADVADDR 8
#define R4300I_CP0_REG_COUNT    9
#define R4300I_CP0_REG_ENTRYHI  10
#define R4300I_CP0_REG_COMPARE  11
#define R4300I_CP0_REG_STATUS   12
#define R4300I_CP0_REG_CAUSE    13
#define R4300I_CP0_REG_EPC      14
#define R4300I_CP0_REG_PRID     15
#define R4300I_CP0_REG_CONFIG   16
#define R4300I_CP0_REG_LLADDR   17
#define R4300I_CP0_REG_WATCHLO  18
#define R4300I_CP0_REG_WATCHHI  19
#define R4300I_CP0_REG_XCONTEXT 20
#define R4300I_CP0_REG_21       21
#define R4300I_CP0_REG_22       22
#define R4300I_CP0_REG_23       23
#define R4300I_CP0_REG_24       24
#define R4300I_CP0_REG_25       25
#define R4300I_CP0_REG_PARITYER 26
#define R4300I_CP0_REG_CACHEER  27
#define R4300I_CP0_REG_TAGLO    28
#define R4300I_CP0_REG_TAGHI    29
#define R4300I_CP0_REG_ERR_EPC  30
#define R4300I_CP0_REG_31       31

#define CP0_STATUS_WRITE_MASK 0xFF57FFFF
#define CP0_CONFIG_WRITE_MASK 0x0FFFFFFF

#define CPU_MODE_KERNEL 0
#define CPU_MODE_SUPERVISOR 1 /* TODO this is probably wrong */
#define CPU_MODE_USER 2 /* TODO this is probably wrong */

#define OPC_CP0    0b010000
#define OPC_CP1    0b010001
#define OPC_CP2    0b010010
#define OPC_LD     0b110111
#define OPC_LUI    0b001111
#define OPC_ADDI   0b001000
#define OPC_ADDIU  0b001001
#define OPC_DADDI  0b011000
#define OPC_ANDI   0b001100
#define OPC_LBU    0b100100
#define OPC_LHU    0b100101
#define OPC_LH     0b100001
#define OPC_LW     0b100011
#define OPC_LWU    0b100111
#define OPC_BEQ    0b000100
#define OPC_BEQL   0b010100
#define OPC_BGTZ   0b000111
#define OPC_BGTZL  0b010111
#define OPC_BLEZ   0b000110
#define OPC_BLEZL  0b010110
#define OPC_BNE    0b000101
#define OPC_BNEL   0b010101
#define OPC_CACHE  0b101111
#define OPC_REGIMM 0b000001
#define OPC_SPCL   0b000000
#define OPC_SB     0b101000
#define OPC_SH     0b101001
#define OPC_SD     0b111111
#define OPC_SW     0b101011
#define OPC_ORI    0b001101
#define OPC_J      0b000010
#define OPC_JAL    0b000011
#define OPC_SLTI   0b001010
#define OPC_SLTIU  0b001011
#define OPC_XORI   0b001110
#define OPC_DADDIU 0b011001
#define OPC_LB     0b100000
#define OPC_LDC1   0b110101
#define OPC_SDC1   0b111101
#define OPC_LWC1   0b110001
#define OPC_SWC1   0b111001
#define OPC_LWL    0b100010
#define OPC_LWR    0b100110
#define OPC_SWL    0b101010
#define OPC_SWR    0b101110
#define OPC_LDL    0b011010
#define OPC_LDR    0b011011
#define OPC_SDL    0b101100
#define OPC_SDR    0b101101
#define OPC_LL     0b110000
#define OPC_LLD    0b110100
#define OPC_SC     0b111000
#define OPC_SCD    0b111100

// Coprocessor
#define COP_MF    0b00000
#define COP_DMF   0b00001
#define COP_CF    0b00010
#define COP_MT    0b00100
#define COP_DMT   0b00101
#define COP_CT    0b00110
#define COP_BC    0b01000


#define COP_BC_BCF  0b00000
#define COP_BC_BCT  0b00001
#define COP_BC_BCFL 0b00010
#define COP_BC_BCTL 0b00011

// Coprocessor FUNCT
#define COP_FUNCT_ADD        0b000000
#define COP_FUNCT_TLBR_SUB   0b000001
#define COP_FUNCT_TLBWI_MULT 0b000010
#define COP_FUNCT_DIV        0b000011
#define COP_FUNCT_SQRT       0b000100
#define COP_FUNCT_ABS        0b000101
#define COP_FUNCT_MOV        0b000110
#define COP_FUNCT_TLBP       0b001000
#define COP_FUNCT_TRUNC_L    0b001001
#define COP_FUNCT_TRUNC_W    0b001101
#define COP_FUNCT_ERET       0b011000
#define COP_FUNCT_CVT_S      0b100000
#define COP_FUNCT_CVT_D      0b100001
#define COP_FUNCT_CVT_W      0b100100
#define COP_FUNCT_CVT_L      0b100101
#define COP_FUNCT_NEG        0b000111
#define COP_FUNCT_C_F        0b110000
#define COP_FUNCT_C_UN       0b110001
#define COP_FUNCT_C_EQ       0b110010
#define COP_FUNCT_C_UEQ      0b110011
#define COP_FUNCT_C_OLT      0b110100
#define COP_FUNCT_C_ULT      0b110101
#define COP_FUNCT_C_OLE      0b110110
#define COP_FUNCT_C_ULE      0b110111
#define COP_FUNCT_C_SF       0b111000
#define COP_FUNCT_C_NGLE     0b111001
#define COP_FUNCT_C_SEQ      0b111010
#define COP_FUNCT_C_NGL      0b111011
#define COP_FUNCT_C_LT       0b111100
#define COP_FUNCT_C_NGE      0b111101
#define COP_FUNCT_C_LE       0b111110
#define COP_FUNCT_C_NGT      0b111111


// Floating point
#define FP_FMT_SINGLE 16
#define FP_FMT_DOUBLE 17
#define FP_FMT_W      20
#define FP_FMT_L      21

// Special
#define FUNCT_SLL    0b000000
#define FUNCT_SRL    0b000010
#define FUNCT_SRA    0b000011
#define FUNCT_SRAV   0b000111
#define FUNCT_SLLV   0b000100
#define FUNCT_SRLV   0b000110
#define FUNCT_JR     0b001000
#define FUNCT_JALR   0b001001
#define FUNCT_MFHI   0b010000
#define FUNCT_MTHI   0b010001
#define FUNCT_MFLO   0b010010
#define FUNCT_MTLO   0b010011
#define FUNCT_DSLLV  0b010100
#define FUNCT_DSRLV  0b010110
#define FUNCT_MULT   0b011000
#define FUNCT_MULTU  0b011001
#define FUNCT_DIV    0b011010
#define FUNCT_DIVU   0b011011
#define FUNCT_DMULT  0b011100
#define FUNCT_DMULTU 0b011101
#define FUNCT_DDIV   0b011110
#define FUNCT_DDIVU  0b011111
#define FUNCT_ADD    0b100000
#define FUNCT_ADDU   0b100001
#define FUNCT_AND    0b100100
#define FUNCT_SUB    0b100010
#define FUNCT_SUBU   0b100011
#define FUNCT_OR     0b100101
#define FUNCT_XOR    0b100110
#define FUNCT_NOR    0b100111
#define FUNCT_SLT    0b101010
#define FUNCT_SLTU   0b101011
#define FUNCT_DADD   0b101100
#define FUNCT_DADDU  0b101101
#define FUNCT_DSUBU  0b101111
#define FUNCT_TEQ    0b110100
#define FUNCT_TNE    0b110110
#define FUNCT_DSLL   0b111000
#define FUNCT_DSRL   0b111010
#define FUNCT_DSRA   0b111011
#define FUNCT_DSLL32 0b111100
#define FUNCT_DSRL32 0b111110
#define FUNCT_DSRA32 0b111111

#define FUNCT_BREAK 0b001101


// REGIMM
#define RT_BLTZ   0b00000
#define RT_BLTZL  0b00010
#define RT_BGEZ   0b00001
#define RT_BGEZL  0b00011
#define RT_BLTZAL 0b10000
#define RT_BGEZAL 0b10001


typedef union cp0_status {
    word raw;
    struct {
        bool ie:1;
        bool exl:1;
        bool erl:1;
        byte ksu:2;
        bool ux:1;
        bool sx:1;
        bool kx:1;
        byte im:8;
        unsigned ds:9;
        bool re:1;
        bool fr:1;
        bool rp:1;
        bool cu0:1;
        bool cu1:1;
        bool cu2:1;
        bool cu3:1;
    } PACKED;
    struct {
        unsigned:16;
        bool de:1;
        bool ce:1;
        bool ch:1;
        bool:1;
        bool sr:1;
        bool ts:1;
        bool bev:1;
        bool:1;
        bool its:1;
        unsigned:7;
    } PACKED;
} cp0_status_t;

ASSERTWORD(cp0_status_t);

typedef union cp0_cause {
    struct {
        byte:8;
        byte interrupt_pending:8;
        unsigned:16;
    };
    struct {
        byte:2;
        byte exception_code:5;
        bool:1;
        bool ip0:1;
        bool ip1:1;
        bool ip2:1;
        bool ip3:1;
        bool ip4:1;
        bool ip5:1;
        bool ip6:1;
        bool ip7:1;
        unsigned:12;
        byte coprocessor_error:2;
        bool:1;
        bool branch_delay:1;
    };
    word raw;
} cp0_cause_t;

ASSERTWORD(cp0_cause_t);

typedef union cp0_entry_lo {
    word raw;
    struct {
        bool g:1;
        bool v:1;
        bool d:1;
        unsigned c:3;
        unsigned pfn:20;
        unsigned:6;
    };
} cp0_entry_lo_t;

ASSERTWORD(cp0_entry_lo_t);

typedef union cp0_page_mask {
    word raw;
    struct {
        unsigned:13;
        unsigned mask:12;
        unsigned:7;
    };
} cp0_page_mask_t;

ASSERTWORD(cp0_page_mask_t);

typedef union cp0_entry_hi {
    struct {
        unsigned asid:8;
        unsigned:5;
        unsigned vpn2:19;
    };
    word raw;
} cp0_entry_hi_t;

ASSERTWORD(cp0_entry_hi_t);

#define CP0_ENTRY_HI_64_READ_MASK 0xC00000FFFFFFE0FF
typedef union cp0_entry_hi_64 {
    struct {
        unsigned asid:8;
        unsigned:5;
        unsigned vpn2:27;
        unsigned fill:22;
        unsigned r:2;
    } PACKED;
    dword raw;
} cp0_entry_hi_64_t;

ASSERTDWORD(cp0_entry_hi_64_t);


typedef struct tlb_entry {
    union {
        struct {
            unsigned:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned pfn:20;
            unsigned:6;
        };
        word raw;
    } entry_lo0;

    union {
        struct {
            unsigned:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned pfn:20;
            unsigned:6;
        };
        word raw;
    } entry_lo1;

    union {
        word raw;
        struct {
            unsigned asid:8;
            unsigned:4;
            bool g:1;
            unsigned vpn2:19;
        };
    } entry_hi;

    union {
        struct {
            unsigned:13;
            unsigned mask:12;
            unsigned:7;
        };
        word raw;
    } page_mask;

    // "parsed"
    bool global;
    bool valid;
    byte asid;

} tlb_entry_t;

typedef struct tlb_entry_64 {
    union {
        struct {
            unsigned:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned pfn:20;
            unsigned long:38;
        };
        word raw;
    } entry_lo0;

    union {
        struct {
            unsigned:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned pfn:20;
            unsigned long:38;
        };
        word raw;
    } entry_lo1;

    union {
        word raw;
        struct {
            unsigned asid:8;
            unsigned:4;
            bool g:1;
            unsigned vpn2:27;
            unsigned:22;
            unsigned r:2;
        };
    } entry_hi;

    union {
        struct {
            unsigned:13;
            unsigned mask:12;
            unsigned long:39;
        };
        dword raw;
    } page_mask;

    // "parsed"
    bool global;
    bool valid;
    byte asid;
    // not present in 32 bit TLB
    byte region;
} tlb_entry_64_t;

typedef union watch_lo {
    word raw;
    struct {
        bool w:1;
        bool r:1;
        bool:1;
        unsigned paddr0:29;
    };
} watch_lo_t;

ASSERTWORD(watch_lo_t);

typedef struct cp0 {
    word index;
    word random;
    cp0_entry_lo_t entry_lo0;
    cp0_entry_lo_t entry_lo1;
    word context;
    dword context_64;
    cp0_page_mask_t page_mask;
    word wired;
    word r7;
    word bad_vaddr;
    dword count;
    cp0_entry_hi_t entry_hi;
    cp0_entry_hi_64_t entry_hi_64;
    word compare;
    cp0_status_t status;
    cp0_cause_t cause;
    dword EPC;
    word PRId;
    word config;
    word lladdr;
    watch_lo_t watch_lo;
    word watch_hi;
    dword x_context;
    word r21;
    word r22;
    word r23;
    word r24;
    word r25;
    word parity_error;
    word cache_error;
    word tag_lo;
    word tag_hi;
    dword error_epc;
    word r31;

    tlb_entry_t    tlb[32];
    tlb_entry_64_t tlb_64[32];

    bool kernel_mode;
    bool supervisor_mode;
    bool user_mode;
    bool is_64bit_addressing;
} cp0_t;

typedef union fcr0 {
    word raw;
} fcr0_t;

typedef union fcr31 {
    word raw;

    struct {
        byte rounding_mode:2;
        bool flag_inexact_operation:1;
        bool flag_underflow:1;
        bool flag_overflow:1;
        bool flag_division_by_zero:1;
        bool flag_invalid_operation:1;
        bool enable_inexact_operation:1;
        bool enable_underflow:1;
        bool enable_overflow:1;
        bool enable_division_by_zero:1;
        bool enable_invalid_operation:1;
        bool cause_inexact_operation:1;
        bool cause_underflow:1;
        bool cause_overflow:1;
        bool cause_division_by_zero:1;
        bool cause_invalid_operation:1;
        bool cause_unimplemented_operation:1;
        unsigned:5;
        bool compare:1;
        bool fs:1;
        unsigned:7;
    };

    struct {
        unsigned:7;
        byte enable:5;
        byte cause:6;
        unsigned:14;
    } PACKED;
} fcr31_t;

ASSERTWORD(fcr31_t);

typedef union fgr {
    dword raw;
    struct {
        word lo:32;
        word hi:32;
    } __attribute__((packed));
} fgr_t;

ASSERTDWORD(fgr_t);

typedef struct r4300i {
    dword gpr[32];

    dword pc;
    dword next_pc;
    dword prev_pc;

    dword mult_hi;
    dword mult_lo;

    bool llbit;

    fcr0_t  fcr0;
    fcr31_t fcr31;

    fgr_t f[32];

    cp0_t cp0;

    // Cached value of `cp0.cause.interrupt_pending & cp0.status.im`
    byte interrupts;

    // In a branch delay slot?
    bool branch;

    // Did an exception just happen?
    bool exception;

    byte (*read_byte)(dword);
    void (*write_byte)(dword, byte);

    half (*read_half)(dword);
    void (*write_half)(dword, half);

    word (*read_word)(dword);
    void (*write_word)(dword, word);

    dword (*read_dword)(dword);
    void (*write_dword)(dword, dword);

    word (*resolve_virtual_address)(dword, cp0_t*);
} r4300i_t;

typedef void(*mipsinstr_handler_t)(r4300i_t*, mips_instruction_t);

void r4300i_step(r4300i_t* cpu);
void r4300i_handle_exception(r4300i_t* cpu, dword pc, word code, sword coprocessor_error);
mipsinstr_handler_t r4300i_instruction_decode(dword pc, mips_instruction_t instr);
void r4300i_interrupt_update(r4300i_t* cpu);

extern const char* register_names[];
extern const char* cp0_register_names[];

INLINE void set_pc_word_r4300i(r4300i_t* cpu, word new_pc) {
    cpu->pc = (sdword)((sword)new_pc);
    cpu->next_pc = cpu->pc + 4;
}

INLINE void set_pc_dword_r4300i(r4300i_t* cpu, dword new_pc) {
    cpu->pc = new_pc;
    cpu->next_pc = cpu->pc + 4;
}

INLINE void cp0_status_updated(r4300i_t* cpu) {
    bool exception = cpu->cp0.status.exl || cpu->cp0.status.erl;

    cpu->cp0.kernel_mode     =  exception || cpu->cp0.status.ksu == CPU_MODE_KERNEL;
    cpu->cp0.supervisor_mode = !exception && cpu->cp0.status.ksu == CPU_MODE_SUPERVISOR;
    cpu->cp0.user_mode       = !exception && cpu->cp0.status.ksu == CPU_MODE_USER;
    cpu->cp0.is_64bit_addressing =
            (cpu->cp0.kernel_mode && cpu->cp0.status.kx)
            || (cpu->cp0.supervisor_mode && cpu->cp0.status.sx)
               || (cpu->cp0.user_mode && cpu->cp0.status.ux);
}

#endif //N64_R4300I_H
