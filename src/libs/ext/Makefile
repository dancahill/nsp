# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
OBJECTS = base64.o dir.o md5.o misc.o rot13.o sort.o xml.o
TARGET  = ../../../lib/libneslaext.a

all: _libneslaext

_libneslaext: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
