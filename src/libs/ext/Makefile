# Makefile for NullLogic Embedded Scripting Language
include ../../Rules.mak

CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = base64.o dir.o misc.o rot13.o sys.o xml.o
TARGET  = ../../../lib/libneslaext.a

all: _libneslaext

_libneslaext: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
