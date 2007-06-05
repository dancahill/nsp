# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = math.o
TARGET  = ../../../lib/libneslamath.a

include ../../Rules.mak

all: _libneslamath

_libneslamath: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
