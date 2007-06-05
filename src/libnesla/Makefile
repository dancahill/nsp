# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../include
OBJECTS = api.o exec.o lib.o libc.o loop.o obj.o parse.o
TARGET  = ../../lib/libnesla.a

all: _libnesla

_libnesla: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
