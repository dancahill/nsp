# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = odbc.o
TARGET  = ../../../lib/libneslaodbc.a

include ../../Rules.mak

all: _libneslaodbc

_libneslaodbc: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
