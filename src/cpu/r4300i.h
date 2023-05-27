#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdalign.h>

#include <util.h>
#include <log.h>
#include "mips_instruction_decode.h"

// Exceptions
#define EXCEPTION_INTERRUPT            0
#define EXCEPTION_TLB_MODIFICATION     1
#define EXCEPTION_TLB_MISS_LOAD        2
#define EXCEPTION_TLB_MISS_STORE       3
#define EXCEPTION_ADDRESS_ERROR_LOAD   4
#define EXCEPTION_ADDRESS_ERROR_STORE  5
#define EXCEPTION_BUS_ERROR_INS_FETCH  6
#define EXCEPTION_BUS_ERROR_LOAD_STORE 7
#define EXCEPTION_SYSCALL              8
#define EXCEPTION_BREAKPOINT           9
#define EXCEPTION_RESERVED_INSTR       10
#define EXCEPTION_COPROCESSOR_UNUSABLE 11
#define EXCEPTION_ARITHMETIC_OVERFLOW  12
#define EXCEPTION_TRAP                 13
#define EXCEPTION_FLOATING_POINT       15
#define EXCEPTION_WATCH                23

// FPU rounding modes
#define R4300I_CP1_ROUND_NEAREST 0
#define R4300I_CP1_ROUND_ZERO 1
#define R4300I_CP1_ROUND_POSINF 2
#define R4300I_CP1_ROUND_NEGINF 3

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
#define CP0_CONFIG_WRITE_MASK 0x0F00800F

#define CPU_MODE_KERNEL 0
#define CPU_MODE_SUPERVISOR 1
#define CPU_MODE_USER 2

#define OPC_CP0    0b010000
#define OPC_CP1    0b010001
#define OPC_CP2    0b010010
#define OPC_CP3    0b010011
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

#define OPC_RDHWR  0b011111

// Coprocessor
#define COP_MF    0b00000
#define COP_DMF   0b00001
#define COP_CF    0b00010
#define COP_DCF   0b00011
#define COP_MT    0b00100
#define COP_DMT   0b00101
#define COP_CT    0b00110
#define COP_DCT   0b00111
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
#define COP_FUNCT_TLBWR_MOV  0b000110
#define COP_FUNCT_TLBP       0b001000
#define COP_FUNCT_ROUND_L    0b001000
#define COP_FUNCT_TRUNC_L    0b001001
#define COP_FUNCT_CEIL_L     0b001010
#define COP_FUNCT_FLOOR_L    0b001011
#define COP_FUNCT_ROUND_W    0b001100
#define COP_FUNCT_TRUNC_W    0b001101
#define COP_FUNCT_CEIL_W     0b001110
#define COP_FUNCT_FLOOR_W    0b001111
#define COP_FUNCT_ERET       0b011000
#define COP_FUNCT_WAIT       0b100000
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
#define FP_FMT_WORD   20
#define FP_FMT_LONG   21

// Special
#define FUNCT_SLL     0b000000
#define FUNCT_SRL     0b000010
#define FUNCT_SRA     0b000011
#define FUNCT_SRAV    0b000111
#define FUNCT_SLLV    0b000100
#define FUNCT_SRLV    0b000110
#define FUNCT_JR      0b001000
#define FUNCT_JALR    0b001001
#define FUNCT_SYSCALL 0b001100
#define FUNCT_SYNC    0b001111
#define FUNCT_MFHI    0b010000
#define FUNCT_MTHI    0b010001
#define FUNCT_MFLO    0b010010
#define FUNCT_MTLO    0b010011
#define FUNCT_DSLLV   0b010100
#define FUNCT_DSRLV   0b010110
#define FUNCT_DSRAV   0b010111
#define FUNCT_MULT    0b011000
#define FUNCT_MULTU   0b011001
#define FUNCT_DIV     0b011010
#define FUNCT_DIVU    0b011011
#define FUNCT_DMULT   0b011100
#define FUNCT_DMULTU  0b011101
#define FUNCT_DDIV    0b011110
#define FUNCT_DDIVU   0b011111
#define FUNCT_ADD     0b100000
#define FUNCT_ADDU    0b100001
#define FUNCT_AND     0b100100
#define FUNCT_SUB     0b100010
#define FUNCT_SUBU    0b100011
#define FUNCT_OR      0b100101
#define FUNCT_XOR     0b100110
#define FUNCT_NOR     0b100111
#define FUNCT_SLT     0b101010
#define FUNCT_SLTU    0b101011
#define FUNCT_DADD    0b101100
#define FUNCT_DADDU   0b101101
#define FUNCT_DSUB    0b101110
#define FUNCT_DSUBU   0b101111
#define FUNCT_TGE     0b110000
#define FUNCT_TGEU    0b110001
#define FUNCT_TLT     0b110010
#define FUNCT_TLTU    0b110011
#define FUNCT_TEQ     0b110100
#define FUNCT_TNE     0b110110
#define FUNCT_DSLL    0b111000
#define FUNCT_DSRL    0b111010
#define FUNCT_DSRA    0b111011
#define FUNCT_DSLL32  0b111100
#define FUNCT_DSRL32  0b111110
#define FUNCT_DSRA32  0b111111

#define FUNCT_BREAK 0b001101


// REGIMM
#define RT_BLTZ    0b00000
#define RT_BLTZL   0b00010
#define RT_BGEZ    0b00001
#define RT_BGEZL   0b00011
#define RT_TGEI    0b01000
#define RT_TGEIU   0b01001
#define RT_TLTI    0b01010
#define RT_TLTIU   0b01011
#define RT_TEQI    0b01100
#define RT_TNEI    0b01110
#define RT_BLTZAL  0b10000
#define RT_BGEZAL  0b10001
#define RT_BGEZALL 0b10011

typedef enum bus_access {
    BUS_LOAD,
    BUS_STORE
} bus_access_t;

#define STATUS_EXL_MASK (1 << 1)
#define STATUS_ERL_MASK (1 << 2)
#define STATUS_CU1_MASK (1 << 29)
typedef union cp0_status {
    u32 raw;
    struct {
        unsigned ie:1;
        unsigned exl:1;
        unsigned erl:1;
        unsigned ksu:2;
        unsigned ux:1;
        unsigned sx:1;
        unsigned kx:1;
        unsigned im:8;
        unsigned ds:9;
        unsigned re:1;
        unsigned fr:1;
        unsigned rp:1;
        unsigned cu0:1;
        unsigned cu1:1;
        unsigned cu2:1;
        unsigned cu3:1;
    } PACKED;
    struct {
        unsigned:16;
        unsigned de:1;
        unsigned ce:1;
        unsigned ch:1;
        unsigned:1;
        unsigned sr:1;
        unsigned ts:1;
        unsigned bev:1;
        unsigned:1;
        unsigned its:1;
        unsigned:7;
    } PACKED;
} PACKED cp0_status_t;

ASSERTWORD(cp0_status_t);

typedef union cp0_cause {
    struct {
        unsigned:8;
        unsigned interrupt_pending:8;
        unsigned:16;
    };
    struct {
        unsigned:2;
        unsigned exception_code:5;
        unsigned:1;
        unsigned ip0:1;
        unsigned ip1:1;
        unsigned ip2:1;
        unsigned ip3:1;
        unsigned ip4:1;
        unsigned ip5:1;
        unsigned ip6:1;
        unsigned ip7:1;
        unsigned:12;
        unsigned coprocessor_error:2;
        unsigned:1;
        unsigned branch_delay:1;
    };
    u32 raw;
} cp0_cause_t;

ASSERTWORD(cp0_cause_t);

typedef union cp0_entry_lo {
    u32 raw;
    struct {
        unsigned g:1;
        unsigned v:1;
        unsigned d:1;
        unsigned c:3;
        unsigned pfn:20;
        unsigned:6;
    };
} cp0_entry_lo_t;

ASSERTWORD(cp0_entry_lo_t);

typedef union cp0_page_mask {
    u32 raw;
    struct {
        unsigned:13;
        unsigned mask:12;
        unsigned:7;
    };
} cp0_page_mask_t;

ASSERTWORD(cp0_page_mask_t);

typedef union cp0_entry_hi {
    struct {
        u64 asid:8;
        u64:5;
        u64 vpn2:27;
        u64 fill:22;
        u64 r:2;
    } PACKED;
    u64 raw;
} cp0_entry_hi_t;

ASSERTDWORD(cp0_entry_hi_t);

#define CP0_ENTRY_LO_WRITE_MASK 0x3FFFFFFF
#define CP0_ENTRY_HI_WRITE_MASK 0xC00000FFFFFFE0FF
#define CP0_PAGEMASK_WRITE_MASK 0x1FFE000


typedef struct tlb_entry {
    bool initialized;
    union {
        struct {
            unsigned:1;
            unsigned valid:1;
            unsigned dirty:1;
            unsigned c:3;
            unsigned pfn:20;
            unsigned:6;
        };
        u32 raw;
    } entry_lo0;

    union {
        struct {
            unsigned:1;
            unsigned valid:1;
            unsigned dirty:1;
            unsigned c:3;
            unsigned pfn:20;
            unsigned:6;
        };
        u32 raw;
    } entry_lo1;

    cp0_entry_hi_t entry_hi;

    union {
        struct {
            unsigned:13;
            unsigned mask:12;
            unsigned:7;
        };
        u32 raw;
    } page_mask;

    // "parsed"
    bool global;
} tlb_entry_t;

typedef union watch_lo {
    u32 raw;
    struct {
        unsigned w:1;
        unsigned r:1;
        unsigned:1;
        unsigned paddr0:29;
    };
} watch_lo_t;

ASSERTWORD(watch_lo_t);

typedef union cp0_context {
    u64 raw;
    struct {
        u64:4;
        u64 badvpn2:19;
        u64 ptebase:41;
    };
} cp0_context_t;

ASSERTDWORD(cp0_context_t);

typedef union cp0_x_context {
    u64 raw;
    struct {
        u64:4;
        u64 badvpn2:27;
        u64 r:2;
        u64 ptebase:31;
    } PACKED;
} cp0_x_context_t;

ASSERTDWORD(cp0_x_context_t);

typedef enum tlb_error {
    TLB_ERROR_NONE,
    TLB_ERROR_MISS,
    TLB_ERROR_INVALID,
    TLB_ERROR_MODIFICATION,
    TLB_ERROR_DISALLOWED_ADDRESS
} tlb_error_t;

static inline u32 get_tlb_exception_code(tlb_error_t error, bus_access_t bus_access) {
    switch (error) {
        case TLB_ERROR_NONE:
            logfatal("Getting TLB exception code when no error occurred!");
        case TLB_ERROR_INVALID:
        case TLB_ERROR_MISS:
            return bus_access == BUS_LOAD ? EXCEPTION_TLB_MISS_LOAD : EXCEPTION_TLB_MISS_STORE;
        case TLB_ERROR_MODIFICATION:
            return EXCEPTION_TLB_MODIFICATION;
        case TLB_ERROR_DISALLOWED_ADDRESS:
            return bus_access == BUS_LOAD ? EXCEPTION_ADDRESS_ERROR_LOAD : EXCEPTION_ADDRESS_ERROR_STORE;
        default:
            logfatal("Getting TLB exception code for error not in switch statement! (%d)", error);
    }
}

typedef struct cp0 {
    u32 index;
    u32 random;
    cp0_entry_lo_t entry_lo0;
    cp0_entry_lo_t entry_lo1;
    cp0_context_t context;
    cp0_page_mask_t page_mask;
    u32 wired;
    u64 bad_vaddr;
    u64 count;
    cp0_entry_hi_t entry_hi;
    u32 compare;
    cp0_status_t status;
    cp0_cause_t cause;
    u64 EPC;
    u32 PRId;
    u32 config;
    u32 lladdr;
    watch_lo_t watch_lo;
    u32 watch_hi;
    cp0_x_context_t x_context;
    u32 parity_error;
    u32 cache_error;
    u32 tag_lo;
    u32 tag_hi;
    u64 error_epc;

    u64 open_bus; // Last value written to any COP0 register

    tlb_entry_t    tlb[32];
    tlb_error_t tlb_error;

    bool kernel_mode;
    bool supervisor_mode;
    bool user_mode;
    bool is_64bit_addressing;
} cp0_t;

typedef union fcr0 {
    u32 raw;
} fcr0_t;

#define FCR31_COMPARE_SHIFT 23
#define FCR31_COMPARE_MASK (1 << (FCR31_COMPARE_SHIFT))

typedef union fcr31 {
    u32 raw;

    struct {
        unsigned rounding_mode:2;
        unsigned flag_inexact_operation:1;
        unsigned flag_underflow:1;
        unsigned flag_overflow:1;
        unsigned flag_division_by_zero:1;
        unsigned flag_invalid_operation:1;
        unsigned enable_inexact_operation:1;
        unsigned enable_underflow:1;
        unsigned enable_overflow:1;
        unsigned enable_division_by_zero:1;
        unsigned enable_invalid_operation:1;
        unsigned cause_inexact_operation:1;
        unsigned cause_underflow:1;
        unsigned cause_overflow:1;
        unsigned cause_division_by_zero:1;
        unsigned cause_invalid_operation:1;
        unsigned cause_unimplemented_operation:1;
        unsigned:5;
        unsigned compare:1;
        unsigned flush_subnormals:1;
        unsigned:7;
    } PACKED;

    struct {
        unsigned:2;
        unsigned flag:5;
        unsigned enable:5;
        unsigned cause:6;
        unsigned:14;
    } PACKED;
} PACKED fcr31_t;

ASSERTWORD(fcr31_t);

typedef union fgr {
    u64 raw;
    struct {
        u32 lo;
        u32 hi;
    };
} fgr_t;

ASSERTDWORD(fgr_t);

typedef struct r4300i {
    u64 gpr[32];
    fgr_t f[32];

    u64 pc;
    u64 next_pc;
    u64 prev_pc;

    u64 mult_hi;
    u64 mult_lo;

    bool llbit;

    fcr0_t  fcr0;
    fcr31_t fcr31;


    cp0_t cp0;
    u64 cp2_latch;

    // Cached value of `cp0.cause.interrupt_pending & cp0.status.im`
    u8 interrupts;

    // In a branch delay slot?
    bool branch;
    bool prev_branch;
    bool branch_likely_taken;

    // Did an exception just happen?
    bool exception;

    // Consts for the JIT
    alignas(16) u32 s_mask[4];
    alignas(16) u64 d_mask[2];
    alignas(16) u32 s_neg[4];
    alignas(16) u64 d_neg[2];
    alignas(16) u32 s_abs[4];
    alignas(16) u64 d_abs[2];

    s64 int64_min;

} r4300i_t;

extern r4300i_t* n64cpu_ptr;
#define N64CPU (*n64cpu_ptr)
#define N64CP0 N64CPU.cp0

typedef void(*mipsinstr_handler_t)(mips_instruction_t);

void on_tlb_exception(u64 address);
void r4300i_step();
void r4300i_handle_exception(u64 pc, u32 code, int coprocessor_error);
mipsinstr_handler_t r4300i_instruction_decode(u64 pc, mips_instruction_t instr);
void r4300i_interrupt_update();
bool instruction_stable(mips_instruction_t instr);

extern const char* register_names[];
extern const char* cp0_register_names[];
extern const char* cp1_register_names[];

typedef enum {
        MIPS_REG_ZERO = 0,
        MIPS_REG_AT   = 1,
        MIPS_REG_V0   = 2,
        MIPS_REG_V1   = 3,
        MIPS_REG_A0   = 4,
        MIPS_REG_A1   = 5,
        MIPS_REG_A2   = 6,
        MIPS_REG_A3   = 7,
        MIPS_REG_T0   = 8,
        MIPS_REG_T1   = 9,
        MIPS_REG_T2   = 10,
        MIPS_REG_T3   = 11,
        MIPS_REG_T4   = 12,
        MIPS_REG_T5   = 13,
        MIPS_REG_T6   = 14,
        MIPS_REG_T7   = 15,
        MIPS_REG_S0   = 16,
        MIPS_REG_S1   = 17,
        MIPS_REG_S2   = 18,
        MIPS_REG_S3   = 19,
        MIPS_REG_S4   = 20,
        MIPS_REG_S5   = 21,
        MIPS_REG_S6   = 22,
        MIPS_REG_S7   = 23,
        MIPS_REG_T8   = 24,
        MIPS_REG_T9   = 25,
        MIPS_REG_K0   = 26,
        MIPS_REG_K1   = 27,
        MIPS_REG_GP   = 28,
        MIPS_REG_SP   = 29,
        MIPS_REG_FP   = 30,
        MIPS_REG_RA   = 31
} mips_register_t;

INLINE void set_pc_word_r4300i(u32 new_pc) {
    N64CPU.prev_pc = N64CPU.pc;
    N64CPU.pc = (s64)((s32)new_pc);
    N64CPU.next_pc = N64CPU.pc + 4;
}

INLINE void set_pc_dword_r4300i(u64 new_pc) {
    N64CPU.prev_pc = N64CPU.pc;
    N64CPU.pc = new_pc;
    N64CPU.next_pc = N64CPU.pc + 4;
}

INLINE void cp0_status_updated() {
    bool exception = N64CPU.cp0.status.exl || N64CPU.cp0.status.erl;

    N64CPU.cp0.kernel_mode     =  exception || N64CPU.cp0.status.ksu == CPU_MODE_KERNEL;
    N64CPU.cp0.supervisor_mode = !exception && N64CPU.cp0.status.ksu == CPU_MODE_SUPERVISOR;
    N64CPU.cp0.user_mode       = !exception && N64CPU.cp0.status.ksu == CPU_MODE_USER;
    N64CPU.cp0.is_64bit_addressing =
            (N64CPU.cp0.kernel_mode && N64CPU.cp0.status.kx)
            || (N64CPU.cp0.supervisor_mode && N64CPU.cp0.status.sx)
               || (N64CPU.cp0.user_mode && N64CPU.cp0.status.ux);
    r4300i_interrupt_update();
}

#define checkcp1_preservecause do { if (!N64CPU.cp0.status.cu1) { r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_COPROCESSOR_UNUSABLE, 1); return; } } while(0)
#define checkcp1 do { checkcp1_preservecause; N64CPU.fcr31.cause = 0; } while(0)
#define checkcp2 do { if (!N64CPU.cp0.status.cu2) { r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_COPROCESSOR_UNUSABLE, 2); return; } } while(0)

#endif //N64_R4300I_H
