; SPDX-License-Identifier: GPL-3.0-or-later
start:
	LI R0, #$08
	LI R1, #$80
	SHLI R0, #$01
	SHRI R0, #$01
	ROLI R0, #$01
	RORI R0, #$01
	SHR R0, R1, R2
	ROR R0, R2
	SHL R0, R1, R2
	ROL R0, R2

	LI PCH, #loadjump@ha
	LI PCL, #loadjump@la
	NOP
	NOP
	NOP
	NOP
wait:
	B wait

loadjump:
	LI R0, #movjump@ha
	LI R1, #movjump@la
	MOV PCH, R0
	MOV PCL, R1

	NOP
	NOP
	NOP
	NOP
wait2:
	B wait2

movjump:
	LI R0, #longjump@ha
	LI R1, #longjump@la
	J R0, R1
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
wait3:
	B wait3
longjump:
	LI FLAGS, #$FF
	MOV R0, FLAGS
	LI FLAGS, #$08
	MOV R0, FLAGS
	LI FLAGS, #$04
	MOV R0, FLAGS
	LI FLAGS, #$02
	MOV R0, FLAGS
	LI FLAGS, #$01
	MOV R0, FLAGS
	LI FLAGS, #$00
	MOV R0, FLAGS

	LI FLAGS, #$FF
	LI R1, #$80
	LI R2, #$01
	LI R3, #$01
	SHL R1, R2, R3

	LI R0, #$00
	LI R1, #$01
	LI R2, #$00
loop2:
	LI FLAGS, #$00
	ADD R0, R1
	LI FLAGS, #$00
	ADDI R2, #$01
	CMPI R2, #$05
	BLT loop2

	LI R0, #$40
	LI R1, #$41
	LI R2, #$42
	LI R3, #$43
	LI R4, #$44

	MOV R0, R3
	MOV R1, R2

	LI R0, #$42
	LI R1, #$52
	LI R2, #$43
	LI R3, #$53

	MOV R2, R0
	MOV R3, R1

	LI R0, #$20
	LI R1, #$21
	LI R2, #$22
	LI R3, #$23
	LI R4, #$23
loop:
	B loop
