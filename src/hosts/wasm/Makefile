# Makefile for NullLogic Embedded Scripting Language
CC      = emcc
CFLAGS  = -I ../../../include -s WASM=1 -s EXPORT_ALL=1 -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -O1
#LDFLAGS = -rdynamic -L../../../lib -lnsp -ldl -lm
#CFLAGS  = -Wall -pg -g -O2 -I../../../include
#LDFLAGS = -L../../../lib -lnsp -pg
OBJECTS = main.c ../../libnsp/block.c ../../libnsp/compile.c ../../libnsp/debug.c ../../libnsp/exec.c ../../libnsp/libc.c ../../libnsp/libn.c ../../libnsp/objects.c ../../libnsp/opcodes.c ../../libnsp/parser.c
TARGETDIR = ../../../bin/wasm/
TARGET  = $(TARGETDIR)/index.html

-include ../../preconf.mak

all: _wasm

_wasm:
	mkdir =p $(TARGETDIR)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS)

clean:
	rm -f $(TARGET) $(TARGET)_d $(TARGET)_s *~
