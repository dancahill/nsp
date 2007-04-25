# Makefile for NullLogic Embedded Scripting Language
include ../../Rules.mak

CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
OBJECTS = http.o misc.o ssl.o tcp.o
TARGET  = ../../../lib/libneslatcp.a

all: _libneslatcp

_libneslatcp: $(OBJECTS)
	@ar cru $(TARGET) $(OBJECTS)
	@ranlib $(TARGET)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
