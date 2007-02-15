# Makefile for NullLogic Embedded Scripting Language
include ../Rules.mak

CC      = gcc
#CFLAGS  = -pedantic -Wall -g -O2 -I../../include
CFLAGS  = -Wall -g -O2 -I../../include
OBJECTS = exec.o lib.o obj.o parse.o
TARGET  = ../../lib/libnesla.a

all: _lib

_lib: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) *~
