# SPDX-License-Identifier: GPL-3.0-or-later
DISELF = lotec-dis
ASSELF = lotec-ass

CPPFLAGS += -W -Wall

.PHONY: all clean

all: bin/$(DISELF) bin/$(ASSELF)

clean:
	rm -f bin/$(DISELF) bin/$(ASSELF)

bin/$(DISELF): src/$(DISELF).c
	mkdir -p bin
	$(CC) $(CPPFLAGS) -o $@ $^

bin/$(ASSELF): src/$(ASSELF).c
	mkdir -p bin
	$(CC) $(CPPFLAGS) -o $@ $^
