# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
OBJECTS = mysql.o
TARGET  = ../../../lib/libneslamysql.a

all: _libneslamysql

_libneslamysql: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
