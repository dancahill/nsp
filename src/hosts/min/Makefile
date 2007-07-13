# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
LDFLAGS = -L../../../lib -lnesla
OBJECTS = main.o
TARGET  = ../../../bin/nesla_m

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) *~
