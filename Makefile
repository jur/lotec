# SPDX-License-Identifier: GPL-3.0-or-later
.PHONY: all clean

all:
	$(MAKE) -C toolchain all
	$(MAKE) -C rom all

clean:
	$(MAKE) -C rom clean
	$(MAKE) -C toolchain clean
