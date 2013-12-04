#include "cpu.h"
#include "os.h"
#include "utils.h"
#include "syscall.h"
reg_word   R[CPU_REG_NUM],
		   LO, HI,
		   CP0[32],
		   PC;
int exception_occurred;

extern int page_table[MAX_PROCESS_NUM][USER_MAX_PAGE_NUM];
extern int cur_process;
#define CP0_ExCode  ((CP0[Cause] & CP0_Cause_ExcCode) >> 2)

static void unsigned_multiply (reg_word v1, reg_word v2);

static void signed_multiply (reg_word v1, reg_word v2);

static int is_aligned(int addr, int b_h_w)
{
	switch (b_h_w) {
		case BYTE:
			return 1;
		case H_WORD:
			return ~(addr & 0x1);
		case WORD:
			return ~(addr & 0x4);
		default:
			return 0;
	}
}

static OpInfo opTable[] = {
    {R_TYPE, RFMT}, {BCOND, IFMT}, {OP_J, JFMT}, {OP_JAL, JFMT},
    {OP_BEQ, IFMT}, {OP_BNE, IFMT}, {OP_BLEZ, IFMT}, {OP_BGTZ, IFMT},
    {OP_ADDI, IFMT}, {OP_ADDIU, IFMT}, {OP_SLTI, IFMT}, {OP_SLTIU, IFMT},
    {OP_ANDI, IFMT}, {OP_ORI, IFMT}, {OP_XORI, IFMT}, {OP_LUI, IFMT},
    {SYSTEM, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_LB, IFMT}, {OP_LH, IFMT}, {OP_UNIMP, IFMT}, {OP_LW, IFMT},
    {OP_LBU, IFMT}, {OP_LHU, IFMT}, {OP_UNIMP, IFMT}, {OP_RES, IFMT},
    {OP_SB, IFMT}, {OP_SH, IFMT}, {OP_UNIMP, IFMT}, {OP_SW, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_UNIMP, IFMT}, {OP_RES, IFMT},
    {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}
};

static int R_type_table[] = {
    OP_SLL,  OP_RES,   OP_SRL,  OP_SRA,  OP_SLLV,    OP_RES,   OP_SRLV, OP_SRAV,
    OP_JR,   OP_JALR,  OP_RES,  OP_RES,  OP_SYSCALL, OP_BREAK, OP_RES,  OP_RES,
    OP_MFHI, OP_MTHI,  OP_MFLO, OP_MTLO, OP_RES,     OP_RES,   OP_RES,  OP_RES,
    OP_MULT, OP_MULTU, OP_DIV,  OP_DIVU, OP_RES,     OP_RES,   OP_RES,  OP_RES,
    OP_ADD,  OP_ADDU,  OP_SUB,  OP_SUBU, OP_AND,     OP_OR,    OP_XOR,  OP_NOR,
    OP_RES,  OP_RES,   OP_SLT,  OP_SLTU, OP_RES,     OP_RES,   OP_RES,  OP_RES,
    OP_RES,  OP_RES,   OP_RES,  OP_RES,  OP_RES,     OP_RES,   OP_RES,  OP_RES,
    OP_RES,  OP_RES,   OP_RES,  OP_RES,  OP_RES,     OP_RES,   OP_RES,  OP_RES
};

int hardware_mem_map(mem_addr addr)
{
	int sign = addr & 0x80000000;
	if (sign == 1) {
		; // kernel part
	}
	else {
		sign = addr & 0x0000f000;
		if (sign) // data segment
			return ((page_table[cur_process][(addr >> 9) + MAX_TEXT_PAGE_NUM]) << 9 ) | (addr & 0x000001ff);
		else
			return ((page_table[cur_process][(addr >> 9)]) << 9 ) | (addr & 0x000001ff);
	}
}

/**
 * TODO
 * for now, we can only read 4 byte at a time!
 */
int
read_mem(mem_addr addr, int b_h_w, reg_word *val)
{
	*val = *(reg_word *)((char *)memory + hardware_mem_map(addr));
	return 1;
	// if (is_aligned(int addr, int b_h_w)(addr, b_h_w)) {
	// 	mem_addr mapped_addr = get_mapped_addr(addr);
	// 	*val = memory[ mapped_addr >> 2];
	// 	return 1;
	// }
	// // else {
	// // 	RAISE_EXCEPTION (ExcCode_IBE, CP0[BadVAddr] = addr);
	// // 	return 0;
	// // }
	// return 0;
}

/**
 * TODO:
 * now make sure that b_h_w is 4
 */
void
write_mem(reg_word src_addr, int b_h_w, reg_word write_word)
{
	*(reg_word *)((char *)memory + hardware_mem_map(src_addr)) = write_word;

	// if (is_aligned(int addr, int b_h_w)(src_addr, b_h_w)) {
	// 	mem_addr mapped_src_addr = get_mapped_addr(src_addr);
	// 	mem_addr src_addr_word = mapped_src_addr / 4;
	// 	mem_addr src_addr_offset = mapped_src_addr % 4;
	// 	int shift;
	// 	switch (b_h_w % 4) {
	// 		case BYTE:
	// 			switch (src_addr_offset) {
	// 				case 0:
	// 					shift = 0x00ffffff;
	// 					break;
	// 				case 1:
	// 					shift = 0xff00ffff;
	// 					break;
	// 				case 2:
	// 					shift = 0xffff00ff;
	// 					break;
	// 				case 3:
	// 					shift = 0xffffff00;
	// 					break;
	// 			}
	// 			break;
	// 		case H_WORD:
	// 			shift = 0xffff;
	// 			shift <<= ( src_addr_offset << 3 );
	// 			break;
	// 		case WORD:
	// 			shift = 0x0;
	// 			break;
	// 	}
	// 	memory[ src_addr_word ] = ( memory[ src_addr_word ] & shift ) | (write_word & (~shift));
	// }
}

void
decode(inst32 raw_inst, instruction *d_inst)
{
    OpInfo *opPtr;
    d_inst->rs = RS(raw_inst);
    d_inst->rt = RT(raw_inst);
    d_inst->rd = RD(raw_inst);
    opPtr = &opTable[OP(raw_inst)];
    d_inst->opCode = opPtr->opCode;
    // get immediate
    if (opPtr->format == IFMT) {
		d_inst->immediate = raw_inst & 0xffff;
		SIGN_EXTEND(d_inst->immediate);
	}
	else if (opPtr->format == RFMT)
		d_inst->immediate = SHAMT(raw_inst);
	else
		d_inst->immediate = raw_inst & 0x3ffffff;

	// R_type意味着R
    switch (d_inst->opCode) {
    	case R_TYPE:
			d_inst->opCode = R_type_table[FUNC(raw_inst)];
			break;
		case BCOND:
		{
			int i = raw_inst & 0x1f0000;
			if (i == 0)
		    	d_inst->opCode = OP_BLTZ;
			else if (i == 0x10000)
		    	d_inst->opCode = OP_BGEZ;
			else if (i == 0x100000)
		    	d_inst->opCode = OP_BLTZAL;
			else if (i == 0x110000)
		    	d_inst->opCode = OP_BGEZAL;
			else
		    	d_inst->opCode = OP_UNIMP;
		}
		case SYSTEM:
		{
			if (0 == d_inst->rs)
				d_inst->opCode = OP_MFC0;
			else if (0x4 == d_inst->rs)
				d_inst->opCode = OP_MTC0;
			else if (0x10 == d_inst->rs)
				d_inst->opCode = OP_ERET;
			else
		    	d_inst->opCode = OP_UNIMP;
		}
	}
}

typedef struct
{
	int reg_num;
	char *reg_name;
} reg_info_t;
reg_info_t reg_info_table[CPU_REG_NUM] =
{
	{REG_ZERO, "$zero"}, {REG_AT, "$at"}, {REG_V0, "$v0"}, {REG_V1, "$v1"},
	{REG_A0, "$a0"}, {REG_A1, "$a1"}, {REG_A2, "$a2"}, {REG_A3, "$a3"},
	{REG_T0, "$t0"}, {REG_T1, "$t1"}, {REG_T2, "$t2"}, {REG_T3, "$t3"},
	{REG_T4, "$t4"}, {REG_T5, "$t5"}, {REG_T6, "$t6"}, {REG_T7, "$t7"},
	{REG_S0, "$s0"}, {REG_S1, "$s1"}, {REG_S2, "$s2"}, {REG_S3, "$s3"},
	{REG_S4, "$s4"}, {REG_S5, "$s5"}, {REG_S6, "$s6"}, {REG_S7, "$s7"},
	{REG_T8, "$t8"}, {REG_T9, "$t9"}, {REG_K0, "$k0"}, {REG_K1, "$k1"},
	{REG_GP, "$gp"}, {REG_SP, "$sp"}, {REG_FP, "$fp"}, {REG_RA, "$ra"},
};

/**
 * only used for debug
 */
static void show_reg()
{
	int i;
	for (i = 0; i < CPU_REG_NUM; i++) {
		printf("%s: %d 	", reg_info_table[i].reg_name, R[reg_info_table[i].reg_num]);
		if (i == 3)
			printf("\n");
	}
}
void
raise_exception (int excode)
{
  if (ExcCode_Int != excode
      || ((CP0[Status] & CP0_Status_IE) /* Allow interrupt if IE and !EXL */
	  && !(CP0[Status] & CP0_Status_EXL)))
    {
      /* Ignore interrupt exception when interrupts disabled.  */
  		exception_occurred = 1;
		if ((CP0[Status] & CP0_Status_EXL) == 0)
		{
		  /* Faulting instruction's address */
		  CP0[EPC] = PC;
		}
      /* ToDo: set CE field of Cause register to coprocessor causing exception */

      /* Record cause of exception */
      CP0[Cause] = (CP0[Cause] & ~CP0_Cause_ExcCode) | (excode << 2);

      /* Turn on EXL bit to prevent subsequent interrupts from affecting EPC */
      CP0[Status] |= CP0_Status_EXL;
    }
}

void
handle_exception ()
{
  if (CP0_ExCode != ExcCode_Int)
    log_file("Exception occurred at PC=0x%08x\n", CP0[EPC]);

  exception_occurred = 0;

  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
		log_file("  Unaligned address in inst/data fetch: 0x%08x\n", CP0[BadVAddr]);
      break;

    case ExcCode_AdES:
		log_file("  Unaligned address in store: 0x%08x\n", CP0[BadVAddr]);
      break;

    case ExcCode_IBE:
		log_file("  Bad address in text read: 0x%08x\n", CP0[BadVAddr]);
      break;

    case ExcCode_DBE:
		log_file("  Bad address in data/stack read: 0x%08x\n", CP0[BadVAddr]);
      break;

    case ExcCode_Sys:
		log_file("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
		log_file("  Reserved instruction execution\n");
      break;

    case ExcCode_Ov:
		log_file("  Arithmetic overflow\n");
      break;

    default:
		log_file("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}

int run(int start_addr, int max_run_step)
{
	instruction inst;
	reg_word raw;
	int i = 0;
	PC = start_addr;
	reg_word nextPC = PC;
	for (; i < max_run_step; ++i) {
		PC = nextPC;
		if (!read_mem(PC, 4, &raw))
			run_error ("Attempt to execute non-instruction at 0x%08x\n", PC);
		decode(raw, &inst);

		nextPC += BYTES_PER_WORD;
		switch (inst.opCode) {
			case OP_ADD:
		    {
				reg_word vs = R[inst.rs], vt = R[inst.rt];
				R[inst.rd] = vs + vt;
				break;
		    }
		    case OP_ADDI:
		    {
				reg_word vs = R[inst.rs], imm = inst.immediate;
				R[inst.rt] = vs + imm;
				break;
		    }

		    case OP_ADDIU:
				R[inst.rt] = R[inst.rs] + (u_reg_word) inst.immediate;
				break;

			case OP_ADDU:
			    R[inst.rd] = R[inst.rs] + R[inst.rt];
			    break;

			case OP_AND:
			    R[inst.rd] = R[inst.rs] & R[inst.rt];
			    break;

			case OP_ANDI:
			    R[inst.rt] = R[inst.rs] & (0xffff & inst.immediate);
			    break;

			case OP_BEQ:
				if (R[inst.rs] == R[inst.rt])
					nextPC += SHIFT2(inst.immediate);
			    break;

			case OP_BGEZAL:
			    R[31] = nextPC; // here pc has already + 4
			case OP_BGEZ:
				if (SIGN_BIT (R[inst.rs]) == 0)
					nextPC += SHIFT2(inst.immediate);
			    break;

			case OP_BGTZ:
				if (R[inst.rs] != 0 && SIGN_BIT (R[inst.rs]) == 0)
					nextPC += SHIFT2(inst.immediate);
			    break;

			case OP_BLEZ:
				if (R[inst.rs] == 0 || SIGN_BIT (R[inst.rs]) != 0)
					nextPC += SHIFT2(inst.immediate);
			    break;

			case OP_BLTZAL:
			    R[31] = nextPC;
			case OP_BLTZ:
				if (SIGN_BIT (R[inst.rs]) != 0)
					nextPC += SHIFT2(inst.immediate);
			    break;

			case OP_BNE:
				if (R[inst.rs] != R[inst.rt])
					nextPC += SHIFT2(inst.immediate);
			    break;
			case OP_DIV:
		      		// 这不抛异常？ or undefined behavior
				 if (R[inst.rt] != 0
				  	&& !(R[inst.rs] == 0x80000000 && R[inst.rt] == 0xffffffff))
				{
				  LO = (reg_word) R[inst.rs] / (reg_word) R[inst.rt];
				  HI = (reg_word) R[inst.rs] % (reg_word) R[inst.rt];
				}
				break;

		  	case OP_DIVU:
				if (R[inst.rt] != 0) {
				  	LO = (u_reg_word) R[inst.rs] / (u_reg_word) R[inst.rt];
				  	HI = (u_reg_word) R[inst.rs] % (u_reg_word) R[inst.rt];
				}
				break;


		  	case OP_JAL:
				R[31] = nextPC;
		  	case OP_J:
				nextPC = (nextPC & 0xf0000000) | SHIFT2(inst.immediate);
				break;

		  	case OP_JALR:
				R[31] = nextPC;
		  	case OP_JR:
				nextPC = R[inst.rs];
				break;

		  	case OP_LB:
		  	case OP_LBU:
		  	{
		  		mem_word value;
				if (!read_mem(R[inst.rs] + inst.immediate, 1, &value))
				    return 0;

				if ((value & 0x80) && (inst.opCode == OP_LB))
				    value |= 0xffffff00;
				else
				    value &= 0xff;
				R[inst.rt] = value;
				break;
			}

		  	case OP_LH:
		  	case OP_LHU:
		  	{
				mem_addr addr = R[inst.rs] + inst.immediate;
		  		mem_word value;
				if (!read_mem(addr, 2, &value))
				    return 0;

				if ((value & 0x8000) && (inst.opCode == OP_LH))
				    value |= 0xffff0000;
				else
				    value &= 0xffff;
				R[inst.rt] = value;
				break;
			}

		  	case OP_LUI:
				// DEBUG('m', "Executing: LUI r%d,%d\n", inst.rt, inst.immediate);
				R[inst.rt] = inst.immediate << 16;
				break;

		  	case OP_LW:
		  	{
				mem_addr addr = R[inst.rs] + inst.immediate;
		  		mem_word value;
				if (!read_mem(addr, 4, &value))
				    return 0;
				R[inst.rt] = value;
				break;
			}

			case OP_MFHI:
				R[inst.rd] = HI;
				break;

			case OP_MFLO:
				R[inst.rd] = LO;
				break;

			case OP_MTHI:
			    HI = R[inst.rs];
			    break;

			case OP_MTLO:
			    LO = R[inst.rs];
			    break;

			case OP_MULT:
			    signed_multiply(R[inst.rs], R[inst.rt]);
			    break;

			case OP_MULTU:
			    unsigned_multiply (R[inst.rs], R[inst.rt]);
			    break;

			case OP_NOR:
				R[inst.rd] = ~(R[inst.rs] | R[inst.rt]);
				break;

			case OP_OR:
				R[inst.rd] = R[inst.rs] | R[inst.rt];
				break;

			case OP_ORI:
				/* note that ori andi xori are all ze*/
				R[inst.rt] = R[inst.rs] | (0xffff & inst.immediate);
				break;

			case OP_SB:
			   write_mem (R[inst.rs] + inst.immediate, 1, R[inst.rt]);
			   break;

			case OP_SH:
			   write_mem (R[inst.rs] + inst.immediate, 2, R[inst.rt]);
			   break;

			case OP_SLL:
			{
				if (inst.immediate >= 0 && inst.immediate < 32)
				  	R[inst.rd] = R[inst.rt] << inst.immediate;
				else
				  	R[inst.rd] = R[inst.rt];
				break;
			}

			case OP_SLLV:
			{
				int shamt = (R[inst.rs] & 0x1f);

				if (shamt >= 0 && shamt < 32)
				 	R[inst.rd] = R[inst.rt] << shamt;
				else
				 	R[inst.rd] = R[inst.rt];
				break;
			}

			case OP_SLT:
				if (R[inst.rs] < R[inst.rt])
					R[inst.rd] = 1;
			    else
					R[inst.rd] = 0;
			    break;

			case OP_SLTI:
				if (R[inst.rs] < inst.immediate)
					R[inst.rt] = 1;
			    else
					R[inst.rt] = 0;
			    break;

			case OP_SLTIU:
				if ((u_reg_word) R[inst.rs] < (u_reg_word) inst.immediate)
				  	R[inst.rt] = 1;
				else
				  	R[inst.rt] = 0;
				break;

			case OP_SLTU:
				if ((u_reg_word) R[inst.rs] < (u_reg_word) R[inst.rt])
					R[inst.rd] = 1;
			    else
					R[inst.rd] = 0;
			    break;

			case OP_SRA:
				if (inst.immediate >= 0 && inst.immediate < 32)
				  	R[inst.rd] = R[inst.rt] >> inst.immediate;
				else
				  	R[inst.rd] = R[inst.rt];
				break;

			case OP_SRAV:
			{
				int shamt = R[inst.rs] & 0x1f;
				if (shamt >= 0 && shamt < 32)
				  	R[inst.rd] = R[inst.rt] >> shamt;
				else
				  	R[inst.rd] = R[inst.rt];
				break;
			}

			case OP_SRL:
			{
				u_reg_word val = R[inst.rt];
				if (inst.immediate >= 0 && inst.immediate < 32)
				  	R[inst.rd] = val >> inst.immediate;
				else
				 	R[inst.rd] = val;
				break;
			}

			case OP_SRLV:
			{
				int shamt = R[inst.rs] & 0x1f;
				u_reg_word val = R[inst.rt];

				if (shamt >= 0 && shamt < 32)
				  	R[inst.rd] = val >> shamt;
				else
				  	R[inst.rd] = val;
				break;
			}

			case OP_SUB:
			{
				reg_word vs = R[inst.rs], vt = R[inst.rt];
				R[inst.rd] = vs - vt;
				break;
			}

			case OP_SUBU:
				R[inst.rd] = (u_reg_word) R[inst.rs] - (u_reg_word) R[inst.rt];
			    break;

			case OP_SW:
				write_mem (R[inst.rs] + inst.immediate, 4, R[inst.rt]);
			    break;

			case OP_XOR:
			    R[inst.rd] = R[inst.rs] ^ R[inst.rt];
			    break;

			case OP_XORI:
			    R[inst.rt] = R[inst.rs] ^ (0xffff & inst.immediate);
			    break;

			case OP_SYSCALL:
			    if (!do_syscall ())
					return 0;
			    break;
			case OP_MFC0:
				R[inst.rt] = CP0[inst.rd];
				break;
			case OP_MTC0:
				CP0[inst.rd] = R[inst.rt];
				break;

			case OP_ERET:
				CP0[Status] &= ~CP0_Status_EXL;	/* Clear EXL bit */
				nextPC = CP0[EPC]; 		/* Jump to EPC */
				break;
			case OP_BREAK:
				if (inst.rd == 1)
				/* Debugger breakpoint */
					raise_exception(ExcCode_Bp);
				else
					raise_exception(ExcCode_Bp);

			case OP_RES:
		    case OP_UNIMP:
				raise_exception(ExcCode_RI);
			default:
				assert(0);
		}
		if (exception_occurred)
			handle_exception ();
	}
}

static void unsigned_multiply (reg_word v1, reg_word v2)
{
	u_reg_word a, b, c, d;
	u_reg_word bd, ad, cb, ac;
	u_reg_word mid, mid2, carry_mid = 0;

/**
	like
	ab
   *cd
   -----
   => ac << 32 + (ad + bc) << 16 + bd
*/
	a = (v1 >> 16) & 0xffff;
	b = v1 & 0xffff;
	c = (v2 >> 16) & 0xffff;
	d = v2 & 0xffff;

	bd = b * d;
	ad = a * d;
	cb = c * b;
	ac = a * c;

	mid = ad + cb;

	if (mid < ad || mid < cb)
	/* Arithmetic overflow or carry-out */
		carry_mid = 1;

	mid2 = mid + ((bd >> 16) & 0xffff);
	if (mid2 < mid || mid2 < ((bd >> 16) & 0xffff))
	/* Arithmetic overflow or carry-out */
		carry_mid += 1;

	LO = (bd & 0xffff) | ((mid2 & 0xffff) << 16);
	HI = ac + (carry_mid << 16) + ((mid2 >> 16) & 0xffff);
}

static void signed_multiply (reg_word v1, reg_word v2)
{
	int neg_sign = 0;

	if (v1 < 0)
	{
	  	v1 = - v1;
	  	neg_sign = 1;
	}
  	if (v2 < 0)
    {
      	v2 = - v2;
      	neg_sign = ! neg_sign;
    }

  	unsigned_multiply (v1, v2);
  	if (neg_sign)
    {
      	LO = ~LO;
      	HI = ~HI;
      	LO += 1;
      	if (LO == 0)
			HI += 1;
    }
}
