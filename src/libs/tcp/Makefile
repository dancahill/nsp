# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
OBJECTS = http.o ssl.o tcp.o
TARGET  = ../../../lib/libneslatcp.a

all: _libneslatcp

_libneslatcp: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
