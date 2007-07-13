# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
OBJECTS = sqlite3.o
TARGET  = ../../../lib/libneslasqlite3.a

all: _libneslasqlite3

_libneslasqlite3: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
