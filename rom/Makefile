# SPDX-License-Identifier: GPL-3.0-or-later
.PHONY: all clean

TOOLCHAINDIR = ../toolchain

DISELF = $(TOOLCHAINDIR)/bin/lotec-dis
ASSELF = $(TOOLCHAINDIR)/bin/lotec-ass

all: test1.hex test2.hex

clean:
	rm -f test1.bin test2.bin test1.hex test2.hex

%.hex: %.asm
	$(ASSELF) $^ >$@

%.bin: %.hex
	$(TOOLCHAINDIR)/hex2bin.py $^ $@
