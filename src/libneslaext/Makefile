# Makefile for NullLogic Embedded Scripting Language
include ../Rules.mak

CC      = gcc
#CFLAGS  = -pedantic -Wall -g -O2 -I../../include -I../libnesla
CFLAGS  = -Wall -g -O2 -I../../include -I../libnesla
OBJECTS = lib.o
TARGET  = ../../lib/libneslaext.a

all: _libneslaext

_libneslaext: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) *~
