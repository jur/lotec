/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdint.h>

#include "lotec-opcodes.h"

const char *decode_reg(uint8_t reg)
{
	switch(reg) {
		case REG_R0:
			return "R0";
		case REG_R1:
			return "R1";
		case REG_R2:
			return "R2";
		case REG_R3:
			return "R3";
		case REG_R4:
			return "R4";

		case REG_FLAGS:
			return "FLAGS";
		case REG_PCL:
			return "PCL";
		case REG_PCH:
			return "PCH";

		default:
			return "R?";
	}
}

const char *decode_cond(uint8_t lotec_cond)
{
	switch(lotec_cond) {
		case COND_AL:
			return "";
		case COND_EQ:
			return "EQ";
		case COND_GT:
			return "GT";
		case COND_LT:
			return "LT";
		case COND_NE:
			return "NE";
		case COND_GE:
			return "GE";
		case COND_LE:
			return "LE";
		case COND_NV:
			return "NV";
		default:
			return "??";
	}
}

void decode_insn(uint16_t address, uint16_t insn)
{
	uint8_t opcode;
	uint8_t imm8;
	uint8_t imm5;
	uint8_t rd;
	uint8_t rs;
	uint8_t rt;
	uint8_t cond;
	static uint32_t last_pch;

	printf("%04X: %02X %02X ", address, (insn >> 8) & 0xFF, (insn >> 0) & 0xFF);

	opcode = (insn >> 11) & 0x1F;

	rd = (insn >> 8) & 0x07;
	rs = (insn >> 5) & 0x07;
	rt = (insn >> 2) & 0x07;
	imm5 = (insn >> 0) & 0x1F;

	cond = (insn >> 8) & 0x07;
	imm8 = (insn >> 0) & 0xFF;

	switch (opcode) {
		case OP_NOP:
			printf("NOP");
			break;

		case OP_LI:
			printf("LI %s, #$%02X", decode_reg(rd), imm8);
			if (rd == REG_PCH) {
				last_pch = imm8 << 9;
				printf("; PC=$%04x", last_pch);
			}
			if (rd == REG_PCL) {
				printf("; PC=$%04x", last_pch | (imm8 << 1));
			}
			break;

		case OP_ADDI:
			printf("ADDI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_ANDI:
			printf("ANDI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_ORI:
			printf("ORI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_XORI:
			printf("XORI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_SUBI:
			printf("SUBI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_CMPI:
			printf("CMPI %s, #$%02X", decode_reg(rd), imm8);
			break;

		case OP_SHRI:
			if (decode_reg(rd) == decode_reg(rs)) {
				if (imm5 >= 8)  {
					printf("SHRI %s, #$%02X", decode_reg(rd), imm5 - 8);
				} else {
					printf("RORI %s, #$%02X", decode_reg(rd), imm5);
				}
			} else {
				printf("SHRI %s, %s, #$%02X", decode_reg(rd), decode_reg(rs), imm5);
			}
			break;

		case OP_SHLI:
			if (decode_reg(rd) == decode_reg(rs))  {
				if (imm5 >= 8)  {
					printf("SHLI %s, #$%02X", decode_reg(rd), imm5 - 8);
				} else {
					printf("ROLI %s, #$%02X", decode_reg(rd), imm5);
				}
			} else {
				printf("SHLI %s, %s, #$%02X", decode_reg(rd), decode_reg(rs), imm5);
			}
			break;

		case OP_LDB:
			printf("LDB %s, $%04X", decode_reg(rd), imm8);
			break;

		case OP_STB:
			printf("STB %s, $%04X", decode_reg(rd), imm8);
			break;

		case OP_MOV:
			printf("MOV %s, %s", decode_reg(rd), decode_reg(rs));
			break;
		case OP_ADD:
			printf("ADD %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_AND:
			printf("AND %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_OR:
			printf("OR %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_XOR:
			printf("XOR %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_SUB:
			printf("SUB %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_CMP:
			printf("CMP %s, %s", decode_reg(rd), decode_reg(rs));
			break;

		case OP_SHR:
			if (decode_reg(rd) == decode_reg(rs)) {
				printf("ROR %s, %s", decode_reg(rd), decode_reg(rt));
			} else {
				printf("SHR %s, %s, %s", decode_reg(rd), decode_reg(rs), decode_reg(rt));
			}
			break;

		case OP_SHL:
			if (decode_reg(rd) == decode_reg(rs)) {
				printf("ROL %s, %s", decode_reg(rd), decode_reg(rt));
			} else {
				printf("SHL %s, %s, %s", decode_reg(rd), decode_reg(rs), decode_reg(rt));
			}
			break;

		case OP_JUMP:
			printf("J%s %s, %s", decode_cond(cond), decode_reg(rs), decode_reg(rt));
			break;

		case OP_BRANCH:
			printf("B%s $%04X", decode_cond(cond), address + ((((int8_t)imm8) + 1) * 2));
			break;

		default:
			printf("illegal opcode %u", opcode);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	const char *filename;
	FILE *fin;
	uint8_t buf[2];
	uint16_t insn;
	uint16_t address;

	if (argc > 1) {
		filename = argv[1];
	} else {
		printf("lotec-dis [bin file]\n");
		printf("Dissassembler for LoTec 8-Bit CPU\n");
		return 1;
	}
	fin = fopen(filename, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Error: Failed to open file '%s'.\n", filename);
		return 2;
	}

	address = 0;
	while(fread(&buf, sizeof(buf), 1, fin) == 1) {
		insn = (buf[0] << 8) | (buf[1] << 0);
		decode_insn(address, insn);
		address += 2;
	}
	fclose(fin);
	return 0;
}
