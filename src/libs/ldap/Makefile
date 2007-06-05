# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = ldap.o
TARGET  = ../../../lib/libneslaldap.a

include ../../Rules.mak

all: _libneslaldap

_libneslaldap: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
