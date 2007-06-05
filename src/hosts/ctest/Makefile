# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
LDFLAGS = -L../../../lib -lnesla -lneslaext -lneslatcp

# if OSTYPE==cygwin
# LDFLAGS += -lws2_32

OBJECTS = main.o
TARGET  = ../../../bin/ctest

include ../../Rules.mak

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LDFLAGS)
	$(CC) -o $(TARGET)_d $(OBJECTS) $(LDFLAGS)
#	$(CC) -o $(TARGET)_s -s -static $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET)_d $(TARGET)_s $(OBJECTS) *~
