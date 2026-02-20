/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lotec-opcodes.h"

#define MAX_BUF_SIZE 256
#define TOK_SIZE 20
#define LABEL_SIZE 100

#define PRINTOPCODE(st, ...) do { \
		if (st->pass == 1) { \
			printf(__VA_ARGS__); \
		} \
	} while(0)

typedef struct {
	char label[MAX_BUF_SIZE];
	uint16_t address;
} label_t;

struct parse_state {
	int pos;
	int lineno;
	int col;
	uint16_t address;
	int tok_pos;
	int lineskip;
	int pass;

	char buffer[MAX_BUF_SIZE];
	char label[MAX_BUF_SIZE];
	int tokens[TOK_SIZE];
	int tokens_col[TOK_SIZE];
	uint32_t values[TOK_SIZE];

	int numlabels;
	label_t labels[LABEL_SIZE];
};


enum token {
	TOK_NOP,
	TOK_LI,
	TOK_ADDI,
	TOK_ANDI,
	TOK_ORI,
	TOK_XORI,
	TOK_SUBI,
	TOK_CMPI,
	TOK_SHRI,
	TOK_SHLI,
	TOK_RORI,
	TOK_ROLI,

	TOK_MOV,
	TOK_ADD,
	TOK_AND,
	TOK_OR,
	TOK_XOR,
	TOK_SUB,
	TOK_CMP,
	TOK_SHR,
	TOK_SHL,
	TOK_ROR,
	TOK_ROL,

	TOK_LDB,
	TOK_STB,

	TOK_JAL,
	TOK_JEQ,
	TOK_JGT,
	TOK_JLT,
	TOK_JNE,
	TOK_JGE,
	TOK_JLE,
	TOK_JNV,

	TOK_BAL,
	TOK_BEQ,
	TOK_BGT,
	TOK_BLT,
	TOK_BNE,
	TOK_BGE,
	TOK_BLE,
	TOK_BNV,

	TOK_ADD_LABEL,
	TOK_LABEL,
	TOK_R0,
	TOK_R1,
	TOK_R2,
	TOK_R3,
	TOK_R4,
	TOK_FLAGS,
	TOK_PCH,
	TOK_PCL,
	TOK_VAL,
	TOK_ADDRESS,
	TOK_COMMA,

	TOK_INVAL,
};

#define TOK_STRING(TOK) case TOK: return #TOK ;

static const char *get_token_name(enum token token)
{
	switch(token) {
		TOK_STRING(TOK_NOP)
		TOK_STRING(TOK_LI)
		TOK_STRING(TOK_ADDI)
		TOK_STRING(TOK_ANDI)
		TOK_STRING(TOK_ORI)
		TOK_STRING(TOK_XORI)
		TOK_STRING(TOK_SUBI)
		TOK_STRING(TOK_CMPI)
		TOK_STRING(TOK_SHRI)
		TOK_STRING(TOK_SHLI)
		TOK_STRING(TOK_RORI)
		TOK_STRING(TOK_ROLI)

		TOK_STRING(TOK_MOV)
		TOK_STRING(TOK_ADD)
		TOK_STRING(TOK_AND)
		TOK_STRING(TOK_OR)
		TOK_STRING(TOK_XOR)
		TOK_STRING(TOK_SUB)
		TOK_STRING(TOK_CMP)
		TOK_STRING(TOK_SHR)
		TOK_STRING(TOK_SHL)
		TOK_STRING(TOK_ROR)
		TOK_STRING(TOK_ROL)

		TOK_STRING(TOK_LDB)
		TOK_STRING(TOK_STB)

		TOK_STRING(TOK_JAL)
		TOK_STRING(TOK_JEQ)
		TOK_STRING(TOK_JGT)
		TOK_STRING(TOK_JLT)
		TOK_STRING(TOK_JNE)
		TOK_STRING(TOK_JGE)
		TOK_STRING(TOK_JLE)
		TOK_STRING(TOK_JNV)

		TOK_STRING(TOK_BAL)
		TOK_STRING(TOK_BEQ)
		TOK_STRING(TOK_BGT)
		TOK_STRING(TOK_BLT)
		TOK_STRING(TOK_BNE)
		TOK_STRING(TOK_BGE)
		TOK_STRING(TOK_BLE)
		TOK_STRING(TOK_BNV)

		TOK_STRING(TOK_ADD_LABEL)
		TOK_STRING(TOK_LABEL)
		TOK_STRING(TOK_R0)
		TOK_STRING(TOK_R1)
		TOK_STRING(TOK_R2)
		TOK_STRING(TOK_R3)
		TOK_STRING(TOK_R4)
		TOK_STRING(TOK_FLAGS)
		TOK_STRING(TOK_PCH)
		TOK_STRING(TOK_PCL)
		TOK_STRING(TOK_VAL)
		TOK_STRING(TOK_ADDRESS)
		TOK_STRING(TOK_COMMA)
		TOK_STRING(TOK_INVAL)

		default: return "unknown token";
	}
}

static uint32_t get_label_address(struct parse_state *st, const char *label)
{
	char l[MAX_BUF_SIZE];
	const char *type = "";
	int i;

	strcpy(l, label);
	for (i = 0; i < MAX_BUF_SIZE; i++) {
		if (l[i] == 0) {
			break;
		}
		if (l[i] == '@') {
			l[i] = 0;
			type = l + i + 1;
			break;
		}
	}

	for (i = 0; i < st->numlabels; i++) {
		if (strcmp(st->labels[i].label, l) == 0) {
			if (strcmp(type, "ha") == 0) {
				return (st->labels[i].address >> 9) & 0xFF;
			} else if (strcmp(type, "la") == 0) {
				return (st->labels[i].address >> 1) & 0xFF;
			} else if (strcmp(type, "hi") == 0) {
				return (st->labels[i].address >> 8) & 0xFF;
			} else if (strcmp(type, "lo") == 0) {
				return (st->labels[i].address >> 0) & 0xFF;
			} else if (type[0] == 0) {
				return st->labels[i].address;
			} else {
				fprintf(stderr, "Error: Label %s has invalid type %s at line %u col %u.\n",
					label, type, st->lineno, st->tokens_col[st->tok_pos]);
				return -1;
			}
		}
	}
	return -1;
}

static int add_label(struct parse_state *st, const char *label)
{
	uint32_t u;

#ifdef VERBOSE
	printf("# Add label '%s' at 0x%04X\n", label, st->address);
#endif

	u = get_label_address(st, label);
	if ((u != (uint32_t)-1) && (u != st->address)) {
		fprintf(stderr, "Error: Label '%s' already added at 0x%04X (0x%04X) line %u col %u\n", label, u, st->address, st->lineno, st->tokens_col[st->tok_pos]);
		return 2;
	}
	st->labels[st->numlabels].address = st->address;
	strcpy(st->labels[st->numlabels].label, label);

	if (st->numlabels < LABEL_SIZE) {
		st->numlabels++;
		return 0;
	}
	fprintf(stderr, "Error: Label '%s' too many labels line %u col %u\n",
		label, st->lineno, st->col);
	return 1;
}

static int parse_token(struct parse_state *st)
{
	int n;
	char *text = st->buffer;

	if (strcmp(text, "NOP") == 0) {
		return TOK_NOP;
	}
	if (strcmp(text, "LI") == 0) {
		return TOK_LI;
	}
	if (strcmp(text, "ADDI") == 0) {
		return TOK_ADDI;
	}
	if (strcmp(text, "ANDI") == 0) {
		return TOK_ANDI;
	}
	if (strcmp(text, "ORI") == 0) {
		return TOK_ORI;
	}
	if (strcmp(text, "XORI") == 0) {
		return TOK_XORI;
	}
	if (strcmp(text, "SUBI") == 0) {
		return TOK_SUBI;
	}
	if (strcmp(text, "CMPI") == 0) {
		return TOK_CMPI;
	}
	if (strcmp(text, "SHRI") == 0) {
		return TOK_SHRI;
	}
	if (strcmp(text, "SHLI") == 0) {
		return TOK_SHLI;
	}
	if (strcmp(text, "RORI") == 0) {
		return TOK_RORI;
	}
	if (strcmp(text, "ROLI") == 0) {
		return TOK_ROLI;
	}
	if (strcmp(text, "ADD") == 0) {
		return TOK_ADD;
	}
	if (strcmp(text, "AND") == 0) {
		return TOK_AND;
	}
	if (strcmp(text, "OR") == 0) {
		return TOK_OR;
	}
	if (strcmp(text, "XOR") == 0) {
		return TOK_XOR;
	}
	if (strcmp(text, "SUB") == 0) {
		return TOK_SUB;
	}
	if (strcmp(text, "CMP") == 0) {
		return TOK_CMP;
	}
	if (strcmp(text, "SHR") == 0) {
		return TOK_SHR;
	}
	if (strcmp(text, "SHL") == 0) {
		return TOK_SHL;
	}
	if (strcmp(text, "ROR") == 0) {
		return TOK_ROR;
	}
	if (strcmp(text, "ROL") == 0) {
		return TOK_ROL;
	}
	if (strcmp(text, "LDB") == 0) {
		return TOK_LDB;
	}
	if (strcmp(text, "STB") == 0) {
		return TOK_STB;
	}
	if (strcmp(text, "MOV") == 0) {
		return TOK_MOV;
	}

	if (strcmp(text, "J") == 0) {
		return TOK_JAL;
	}
	if (strcmp(text, "JAL") == 0) {
		return TOK_JAL;
	}
	if (strcmp(text, "JAL") == 0) {
		return TOK_JAL;
	}
	if (strcmp(text, "JEQ") == 0) {
		return TOK_JEQ;
	}
	if (strcmp(text, "JGT") == 0) {
		return TOK_JGT;
	}
	if (strcmp(text, "JLT") == 0) {
		return TOK_JLT;
	}
	if (strcmp(text, "JNE") == 0) {
		return TOK_JNE;
	}
	if (strcmp(text, "JGE") == 0) {
		return TOK_JGE;
	}
	if (strcmp(text, "JLE") == 0) {
		return TOK_JLE;
	}

	if (strcmp(text, "B") == 0) {
		return TOK_BAL;
	}
	if (strcmp(text, "BAL") == 0) {
		return TOK_BAL;
	}
	if (strcmp(text, "BAL") == 0) {
		return TOK_BAL;
	}
	if (strcmp(text, "BEQ") == 0) {
		return TOK_BEQ;
	}
	if (strcmp(text, "BGT") == 0) {
		return TOK_BGT;
	}
	if (strcmp(text, "BLT") == 0) {
		return TOK_BLT;
	}
	if (strcmp(text, "BNE") == 0) {
		return TOK_BNE;
	}
	if (strcmp(text, "BGE") == 0) {
		return TOK_BGE;
	}
	if (strcmp(text, "BLE") == 0) {
		return TOK_BLE;
	}
	if (text[0] == '#') {
		char *l = text + 1;
		uint32_t addr;

		if (text[1] == '$') {
			st->values[st->tok_pos] = strtoul(st->buffer + 2, NULL, 16);
			return TOK_VAL;
		}
		addr = get_label_address(st, l);
		if (addr == (uint32_t)-1) {
			if (st->pass != 0) {
				fprintf(stderr, "Error: Label %s is not defined at line %u col %u.\n",
					l, st->lineno, st->tokens_col[st->tok_pos]);
				return TOK_INVAL;
			}
			addr = 0;
		}
		st->values[st->tok_pos] = addr;
		return TOK_VAL;
	}
	if (text[0] == '$') {
		st->values[st->tok_pos] = strtoul(st->buffer + 1, NULL, 16);
		return TOK_ADDRESS;
	}
	if (strcmp(text, "R0") == 0) {
		return TOK_R0;
	}
	if (strcmp(text, "R1") == 0) {
		return TOK_R1;
	}
	if (strcmp(text, "R2") == 0) {
		return TOK_R2;
	}
	if (strcmp(text, "R3") == 0) {
		return TOK_R3;
	}
	if (strcmp(text, "R4") == 0) {
		return TOK_R4;
	}
	if (strcmp(text, "FLAGS") == 0) {
		return TOK_FLAGS;
	}
	if (strcmp(text, "PCH") == 0) {
		return TOK_PCH;
	}
	if (strcmp(text, "PCL") == 0) {
		return TOK_PCL;
	}

	n = strlen(text);
	if (text[n - 1] == ':') {
		text[n - 1] = 0;
		if (add_label(st, text) != 0) {
			return TOK_INVAL;
		}
		return TOK_ADD_LABEL;
	}
	strcpy(st->label, st->buffer);
	return TOK_LABEL;
}

static void next_insn(struct parse_state *st)
{
	st->address += 2;
}

static int parse_token_nop(struct parse_state *st)
{
	if (st->tok_pos != 1) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", OP_NOP << 12);

	next_insn(st);
	return 0;
}

static int parse_token_reg(int token)
{
	switch(token) {
		case TOK_R0:
			return REG_R0;
		case TOK_R1:
			return REG_R1;
		case TOK_R2:
			return REG_R2;
		case TOK_R3:
			return REG_R3;
		case TOK_R4:
			return REG_R4;

		case TOK_FLAGS:
			return REG_FLAGS;

		case TOK_PCH:
			return REG_PCH;
		case TOK_PCL:
			return REG_PCL;
		default:
			fprintf(stderr, "Error: Register expected.\n");
			return -1;
	}
}

static int parse_token_alu(uint8_t opcode, struct parse_state *st)
{
	int rd;

	if (st->tok_pos != 3) {
		return 1;
	}

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	if (st->tokens[2] != TOK_VAL) {
		return 1;
	}
	if (st->values[2] > 0xFF) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | st->values[2]);

	next_insn(st);
	return 0;
}

static int parse_token_shift(uint8_t opcode, struct parse_state *st)
{
	int rd;
	int rs;
	int off;

	if ((st->tok_pos != 4) && (st->tok_pos != 3)) {
		return 1;
	}

	if ((st->tokens[0] == TOK_RORI) || (st->tokens[0] == TOK_ROLI)) {
		if (st->tok_pos != 3) {
			return 1;
		}
	}

	off = st->tok_pos - 2;

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	rs = parse_token_reg(st->tokens[off]);
	if (rs < 0) {
		return 1;
	}
	if (st->tokens[off + 1] != TOK_VAL) {
		return 1;
	}

	if ((st->tok_pos == 4) || (st->tokens[0] == TOK_RORI) || (st->tokens[0] == TOK_ROLI)) {
		if (st->values[off + 1] > 15) {
			return 1;
		}
	} else {
		if (st->values[off + 1] > 7) {
			return 1;
		}
		st->values[off + 1] += 8;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | (rs << 5) | st->values[off + 1]);

	next_insn(st);
	return 0;
}

static int parse_token_shiftreg(uint8_t opcode, struct parse_state *st)
{
	int rd;
	int rs;
	int rt;
	int off;

	if ((st->tokens[0] == TOK_ROR) || (st->tokens[0] == TOK_ROL)) {
		if (st->tok_pos != 3) {
			return 1;
		}
	} else {
		if (st->tok_pos != 4) {
			return 1;
		}
	}
	off = st->tok_pos - 2;

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	rs = parse_token_reg(st->tokens[off]);
	if (rs < 0) {
		return 1;
	}
	rt = parse_token_reg(st->tokens[off + 1]);
	if (rt < 0) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | (rs << 5) | (rt << 2));

	next_insn(st);
	return 0;
}

static int parse_token_addr(uint8_t opcode, struct parse_state *st)
{
	int rd;

	if (st->tok_pos != 3) {
		return 1;
	}

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	if (st->tokens[2] != TOK_ADDRESS) {
		return 1;
	}
	if (st->values[2] > 0xFF) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | st->values[2]);

	next_insn(st);
	return 0;
}

static int parse_token_mov(uint8_t opcode, struct parse_state *st)
{
	int rd;
	int rs;

	if (st->tok_pos != 3) {
		return 1;
	}

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	rs = parse_token_reg(st->tokens[2]);
	if (rs < 0) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | (rs << 5));

	next_insn(st);
	return 0;
}

static int parse_token_alucode(uint8_t opcode, struct parse_state *st)
{
	int rd;
	int rs;

	if (st->tok_pos != 3) {
		return 1;
	}

	rd = parse_token_reg(st->tokens[1]);
	if (rd < 0) {
		return 1;
	}
	rs = parse_token_reg(st->tokens[2]);
	if (rs < 0) {
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (rd << 8) | (rs << 5));

	next_insn(st);
	return 0;
}

static int parse_cond(int token)
{
	switch(token) {
		case TOK_JAL:
			return COND_AL;
		case TOK_JEQ:
			return COND_EQ;
		case TOK_JGT:
			return COND_GT;
		case TOK_JLT:
			return COND_LT;
		case TOK_JNE:
			return COND_NE;
		case TOK_JGE:
			return COND_GE;
		case TOK_JLE:
			return COND_LE;
		case TOK_JNV:
			return COND_NV;

		case TOK_BAL:
			return COND_AL;
		case TOK_BEQ:
			return COND_EQ;
		case TOK_BGT:
			return COND_GT;
		case TOK_BLT:
			return COND_LT;
		case TOK_BNE:
			return COND_NE;
		case TOK_BGE:
			return COND_GE;
		case TOK_BLE:
			return COND_LE;
		case TOK_BNV:
			return COND_NV;
		default:
			return -1;
	}
}

static int parse_token_jump(uint8_t opcode, struct parse_state *st)
{
	int cond;
	int rs;
	int rt;

	if (st->tok_pos != 3) {
		fprintf(stderr, "Error: Jump tokens wrong (%u).\n", st->tok_pos);
		return 1;
	}

	cond = parse_cond(st->tokens[0]);
	if (cond < 0) {
		fprintf(stderr, "Error: Branch condition wrong.\n");
		return 1;
	}

	rs = parse_token_reg(st->tokens[1]);
	if (rs < 0) {
		return 1;
	}
	rt = parse_token_reg(st->tokens[2]);
	if (rt < 0) {
		return 1;
	}

	PRINTOPCODE(st, "%x\n", (opcode << 11) | (cond << 8) | (rs << 5) | (rt << 2));

	next_insn(st);
	return 0;
}

static int parse_token_branch(uint8_t opcode, struct parse_state *st)
{
	int cond;
	uint16_t offset;
	uint32_t addr;

	if (st->tok_pos != 2) {
		fprintf(stderr, "Error: Branch tokens wrong (%u).\n", st->tok_pos);
		return 1;
	}

	cond = parse_cond(st->tokens[0]);
	if (cond < 0) {
		fprintf(stderr, "Error: Branch condition wrong.\n");
		return 1;
	}

	if (st->tokens[1] == TOK_LABEL) {

		addr = get_label_address(st, st->label);
		if (addr > 0xFFFF) {
			if (st->pass == 0) {
				addr = 0;
			} else {
				fprintf(stderr, "Error: Invalid label %s, line %u col %u\n", st->label, st->lineno, st->col);
				return 1;
			}
		}
	} else {
		if (st->tokens[1] != TOK_ADDRESS) {
			return 1;
		}
		addr = st->values[1];
	}
	offset = addr - (st->address + 2);
	if (((offset & 0xFF00) != 0xFF00) && ((offset & 0xFF00) != 0x0000)) {
		fprintf(stderr, "Error: Branch offset larger than 8 bit (offset 0x%04x, pc 0x%04x, target 0x%04x)\n", offset, st->address, addr);
		return 1;
	}
	PRINTOPCODE(st, "%x\n", (opcode << 11) | (cond << 8) | ((offset >> 1) & 0xFF));

	next_insn(st);
	return 0;
}

static int parse_token_list(struct parse_state *st)
{
	if (st->tok_pos == 0) {
		return 0;
	}
	switch(st->tokens[0]) {
		case TOK_NOP:
			return parse_token_nop(st);
		case TOK_LI:
			return parse_token_alu(OP_LI, st);
		case TOK_ADDI:
			return parse_token_alu(OP_ADDI, st);
		case TOK_ANDI:
			return parse_token_alu(OP_ANDI, st);
		case TOK_ORI:
			return parse_token_alu(OP_ORI, st);
		case TOK_XORI:
			return parse_token_alu(OP_XORI, st);
		case TOK_SUBI:
			return parse_token_alu(OP_SUBI, st);
		case TOK_CMPI:
			return parse_token_alu(OP_CMPI, st);
		case TOK_SHRI:
		case TOK_RORI:
			return parse_token_shift(OP_SHRI, st);
		case TOK_SHLI:
		case TOK_ROLI:
			return parse_token_shift(OP_SHLI, st);
		case TOK_LDB:
			return parse_token_addr(OP_LDB, st);
		case TOK_STB:
			return parse_token_addr(OP_STB, st);
		case TOK_MOV:
			return parse_token_mov(OP_MOV, st);
		case TOK_ADD:
			return parse_token_alucode(OP_ADD, st);
		case TOK_AND:
			return parse_token_alucode(OP_AND, st);
		case TOK_OR:
			return parse_token_alucode(OP_OR, st);
		case TOK_XOR:
			return parse_token_alucode(OP_XOR, st);
		case TOK_SUB:
			return parse_token_alucode(OP_SUB, st);
		case TOK_CMP:
			return parse_token_alucode(OP_CMP, st);
		case TOK_SHR:
		case TOK_ROR:
			return parse_token_shiftreg(OP_SHR, st);
		case TOK_SHL:
		case TOK_ROL:
			return parse_token_shiftreg(OP_SHL, st);

		case TOK_JAL:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JEQ:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JGT:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JLT:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JNE:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JGE:
			return parse_token_jump(OP_JUMP, st);
		case TOK_JLE:
			return parse_token_jump(OP_JUMP, st);

		case TOK_BAL:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BEQ:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BGT:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BLT:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BNE:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BGE:
			return parse_token_branch(OP_BRANCH, st);
		case TOK_BLE:
			return parse_token_branch(OP_BRANCH, st);
		default:
			fprintf(stderr, "Error: Syntax error (%u) col %u\n", st->tokens[0], st->tokens_col[0]);
			return 1;
	}
	return 0;
}

static void parse_reset(struct parse_state *st)
{
	st->pos = 0;
	st->lineno = 1;
	st->col = 1;
	st->address = 0;
	st->tok_pos = 0;
	st->lineskip = 0;
	st->buffer[0] = 0;
	st->label[0] = 0;
}

static int parse_char(struct parse_state *st, char c)
{
	// printf("%c", c);

	if (!st->lineskip) {
		if ((c > ' ') && (c <= '~') && (c != ',') && (c != ';')) {
			/* non white space */
			st->buffer[st->pos] = c;
			if (st->pos == 0) {
				st->tokens_col[st->tok_pos] = st->col;
			}
			st->pos++;

			if (st->pos >= MAX_BUF_SIZE) {
				fprintf(stderr, "Error: String too long at line %u col %u.\n", st->lineno, st->col);
				return 1;
			}
		} else {
			st->buffer[st->pos] = 0;
			if (st->pos > 0) {
				int token;

#ifdef VERBOSE
				printf("# line %u col %u token: '%s'\n", st->lineno, st->col, st->buffer);
#endif
				token = parse_token(st);
				if (token == TOK_INVAL) {
					fprintf(stderr, "Error: Invalid token at line %u col %u.\n", st->lineno, st->tokens_col[st->tok_pos]);
					return 1;
				}
				if (token == TOK_ADD_LABEL) {
					st->tok_pos = 0;
					st->label[0] = 0;
				} else {
					if (st->tok_pos >= TOK_SIZE) {
						fprintf(stderr, "Error: Too many tokens at line %u col %u.\n", st->lineno, st->col);
						return 1;
					}
					st->tokens[st->tok_pos] = token;
					st->tok_pos++;

					if (st->tok_pos < TOK_SIZE) {
						st->tokens_col[st->tok_pos] = st->col;
					}
				}
			}
			st->pos = 0;
		}
	}
	if (c == ';') {
		st->lineskip = 1;
	}

	if (c == '\t') {
		st->col += 8;
	} else {
		st->col++;
	}
	if (c == '\n') {
		int rv;

		st->lineskip = 0;

		rv = parse_token_list(st);
		if (rv != 0) {
			int i;

			fprintf(stderr, "Error: Failed to parse tokens at line %u col %u to %u.\n", st->lineno, st->tokens_col[0], st->col);

			for (i = 0; i < st->tok_pos; i++) {
				fprintf(stderr, "Token %u: %u %s", i, st->tokens[i], get_token_name(st->tokens[i]));
				if (st->tokens[i] == TOK_VAL) {
					fprintf(stderr, " 0x%04x", st->values[i]);
				}
				if (st->tokens[i] == TOK_LABEL) {
					fprintf(stderr, " %s", st->label);
				}
				fprintf(stderr, "\n");
			}
			return 1;
		}

		st->tok_pos = 0;
		st->lineno++;
		st->col = 1;
		st->label[0] = 0;
		st->tokens_col[st->tok_pos] = st->col;
	}

	if (c == '\r') {
		st->col = 1;
		st->tokens_col[st->tok_pos] = st->col;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	const char *filename;
	FILE *fin;
	char c;
	struct parse_state st;

	if (argc > 1) {
		filename = argv[1];
	} else {
		printf("lotec-ass [asm file]\n");
		printf("Assembler for LoTec 8-Bit CPU\n");
		return 1;
	}
	fin = fopen(filename, "r");
	if (fin == NULL) {
		fprintf(stderr, "Error: Failed to open file '%s'.\n", filename);
		return 2;
	}

	printf("v2.0 raw\n");

	parse_reset(&st);
	st.pass = 0;
	st.numlabels = 0;
	while(fread(&c, sizeof(c), 1, fin) == 1) {
		if (parse_char(&st, c) != 0) {
			fprintf(stderr, "Error: Failed to parse file '%s'.\n", filename);
			return 3;
		}
	}

	rewind(fin);
	parse_reset(&st);
	st.pass = 1;
	while(fread(&c, sizeof(c), 1, fin) == 1) {
		if (parse_char(&st, c) != 0) {
			fprintf(stderr, "Error: Failed to parse file '%s'.\n", filename);
			return 3;
		}
	}
	fclose(fin);
	return 0;
}
