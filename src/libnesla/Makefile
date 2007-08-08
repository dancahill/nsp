# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../include
OBJECTS = block.o compile.o debug.o exec.o libc.o libn.o objects.o opcodes.o parser.o
TARGET  = ../../lib/libnesla.a

all: _libnesla

_libnesla: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
