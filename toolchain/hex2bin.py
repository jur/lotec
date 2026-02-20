#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import struct

instream = sys.stdin
outstream = sys.stdout

if len(sys.argv) > 1:
	instream = open(sys.argv[1], "r")
if len(sys.argv) > 2:
	outstream = open(sys.argv[2], "wb")

line = instream.readline()

line = instream.readline()
while line:
	data = int(line, 16)
	outstream.write(struct.pack(">H", data))
	#print("0x%04x" % data)
	line = instream.readline()
