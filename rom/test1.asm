; SPDX-License-Identifier: GPL-3.0-or-later
start:
	NOP
	LI R1, #$23
	LI FLAGS, #$00
	ADDI R1, #$01
	ANDI R1, #$20
	ORI R1, #$01
	XORI R1, #$FF
	LI R1, #$40
	LI FLAGS, #$00
	SUBI R1, #$03
	LI R1, #$C0
	LI FLAGS, #$00
	ADDI R1, #$80
	MOV R2, R1
loop:
	LDB R3, $0000
	LI FLAGS, #$00
	SUBI R3, #$01
	STB R3, $0000
	CMPI R3, #$00
	BNE loop
	LI R0, #$10
	B start
