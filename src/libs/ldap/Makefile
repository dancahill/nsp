# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
OBJECTS = ldap.o
TARGET  = ../../../lib/libneslaldap.a

all: _libneslaldap

_libneslaldap: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
