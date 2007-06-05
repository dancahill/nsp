# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = base64.o dir.o misc.o rot13.o xml.o
TARGET  = ../../../lib/libneslaext.a

include ../../Rules.mak

all: _libneslaext

_libneslaext: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
