# Overview
This is an implementation of the LoTec 8 Bit CPU. The features are:

* Implemented using discrete logic elements
* Each instruction takes 2 clock cycles
	* Fetch instruction
	* Execute instruction
* 8 Register R0, R1, R2, R3, R4, FLAGS, PCH and PCL (program counter).
* ALU with ADD, SUB, AND, OR, XOR, CMP and bit shift (SHR, SHL).
* Move registers (MOV).
* Conditional branch/jumps (B, J).
	* always (AL)
	* equal (EQ)
	* greater then (GT)
	* less than (LT)
	* not equal (NE)
	* greater or equal (GE)
	* less or equal (LE)
	* never (NV)
* RAM access load and store (LDB, STB).
* Not implemented instructions are executed as NOP.
* Instructions are in ROM (Harvard architecture).
* Toolchain with assembler and disassembler.

# Usage
Get the program Digital and install it as described here:
[Digital at github](https://github.com/hneemann/Digital)

* Load file dig/lotec.dig
* Start simulator
* Zoom in to see what is happening.

The ROM hex files can be loaded into the ROM.
