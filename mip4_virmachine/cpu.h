#ifndef CPU_H
#define CPU_H
#include "global.h"

#define OP_ADD		1
#define OP_ADDI		2
#define OP_ADDIU	3
#define OP_ADDU		4
#define OP_AND		5
#define OP_ANDI		6
#define OP_BEQ		7
#define OP_BGEZ		8
#define OP_BGEZAL	9
#define OP_BGTZ		10
#define OP_BLEZ		11
#define OP_BLTZ		12
#define OP_BLTZAL	13
#define OP_BNE		14

#define OP_DIV		16
#define OP_DIVU		17
#define OP_J		18
#define OP_JAL		19
#define OP_JALR		20
#define OP_JR		21
#define OP_LB		22
#define OP_LBU		23
#define OP_LH		24
#define OP_LHU		25
#define OP_LUI		26
#define OP_LW		27

#define OP_MFHI		31
#define OP_MFLO		32

#define OP_MTHI		34
#define OP_MTLO		35
#define OP_MULT		36
#define OP_MULTU	37
#define OP_NOR		38
#define OP_OR		39
#define OP_ORI		40
#define OP_SB		42
#define OP_SH		43
#define OP_SLL		44
#define OP_SLLV		45
#define OP_SLT		46
#define OP_SLTI		47
#define OP_SLTIU	48
#define OP_SLTU		49
#define OP_SRA		50
#define OP_SRAV		51
#define OP_SRL		52
#define OP_SRLV		53
#define OP_SUB		54
#define OP_SUBU		55
#define OP_SW		56
#define OP_XOR		59
#define OP_XORI		60
#define OP_SYSCALL	61
#define OP_UNIMP	62
#define OP_RES		63
#define MaxOpcode	63

#define R_TYPE      100
#define BCOND	    101
#define SYSTEM      102
#define OP_MTC0     103
#define OP_MFC0     104
#define OP_ERET     105
#define OP_BREAK    106

#define IFMT 1
#define JFMT 2
#define RFMT 3

#define RS(INST)        ((INST) >> 21) & 0x1f
#define RT(INST)        ((INST) >> 16) & 0x1f
#define RD(INST)        ((INST) >> 11) & 0x1f
#define SHAMT(INST)     ((INST) >> 6 ) & 0x1f
#define OP(INST)        ((INST) >> 26) & 0x3f
#define FUNC(INST)      ((INST) & 0x3f)

#define SIGN_BIT(X) ((X) & 0x80000000)

#define SIGN_EXTEND(VAL)    \
    {       \
        if ((VAL) & 0x8000)     \
            (VAL) |= 0xffff0000;    \
    } \



#define ARITH_OVFL(RESULT, OP1, OP2) (SIGN_BIT (OP1) == SIGN_BIT (OP2) \
                      && SIGN_BIT (OP1) != SIGN_BIT (RESULT))

#define SHIFT2(x) ((x) << 2)

#define RAISE_EXCEPTION(EXCODE, MISC)                   \
    {                               \
    raise_exception(EXCODE);                    \
    MISC;                               \
    }                               \

#define BadVAddr 8
#define Status  12
#define Cause   13
#define EPC     14

#define CP0_Status_IE       0x1
#define CP0_Status_EXL      0x2
#define CPO_Status_KSU      0x8
#define CP0_Cause_ExcCode   0x0000007c

#define EXCEPTION_ADDR      0x80000180

#define ExcCode_Int 0   /* Interrupt */
#define ExcCode_Mod 1   /* TLB modification (not implemented) */
#define ExcCode_TLBL    2   /* TLB exception (not implemented) */
#define ExcCode_TLBS    3   /* TLB exception (not implemented) */

#define ExcCode_AdEL    4   /* Address error (load/fetch) */
#define ExcCode_AdES    5   /* Address error (store) */
#define ExcCode_IBE 6   /* Bus error, instruction fetch */
#define ExcCode_DBE 7   /* Bus error, data reference */

#define ExcCode_Sys 8   /* Syscall exception */
#define ExcCode_Bp  9   /* Breakpoint exception 一条CPU没有定义的指令*/
#define ExcCode_RI  10  /* Reserve instruction */
#define ExcCode_Ov  12  /* Arithmetic overflow */

enum {
    REG_ZERO, REG_AT, REG_V0, REG_V1,
    REG_A0, REG_A1, REG_A2, REG_A3,
    REG_T0, REG_T1, REG_T2, REG_T3,
    REG_T4, REG_T5, REG_T6, REG_T7,
    REG_S0, REG_S1, REG_S2, REG_S3,
    REG_S4, REG_S5, REG_S6, REG_S7,
    REG_T8, REG_T9, REG_K0, REG_K1,
    REG_GP, REG_SP, REG_FP, REG_RA,
};

typedef struct  {
    int opCode;		/* "Translated" op code. */
    int format;		/* Format type (IFMT or JFMT or RFMT) */
} OpInfo;

typedef struct {
    inst_opcode opCode: 8;
    inst_reg rs: 5, rt: 5, rd: 5;
    inst_imm immediate: 16;
} instruction;
extern mem_word *memory;
#define CPU_REG_NUM     32
extern reg_word R[CPU_REG_NUM];
int run(int start_addr, int max_run_step);

#endif