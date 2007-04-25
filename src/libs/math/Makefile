# Makefile for NullLogic Embedded Scripting Language
include ../../Rules.mak

CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = math.o
TARGET  = ../../../lib/libneslamath.a

all: _libneslamath

_libneslamath: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
