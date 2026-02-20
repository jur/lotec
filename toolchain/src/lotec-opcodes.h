/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef LOTECOPCODES_H
#define LOTECOPCODES_H

enum lotec_opcode {
	OP_NOP = 0,
	OP_LI,
	OP_ADDI,
	OP_ANDI,
	OP_ORI,
	OP_XORI,
	OP_SUBI,
	OP_CMPI,
	OP_SHRI,
	OP_SHLI,

	OP_MOV = 17,
	OP_ADD,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_SUB,
	OP_CMP,
	OP_SHR,
	OP_SHL,

	OP_LDB = 28,
	OP_STB,
	OP_JUMP,
	OP_BRANCH,
};

enum lotec_cond {
	COND_AL = 0,
	COND_EQ,
	COND_GT,
	COND_LT,
	COND_NE,
	COND_GE,
	COND_LE,
	COND_NV
};

enum lotec_register {
	REG_R0 = 0,
	REG_R1,
	REG_R2,
	REG_R3,
	REG_R4,

	REG_FLAGS,

	REG_PCL,
	REG_PCH,
};

#endif
