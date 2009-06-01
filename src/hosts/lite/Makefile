# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include
LDFLAGS = -L../../../lib -lnesla
#CFLAGS  = -Wall -g -O2 -I../../../include -pg
#LDFLAGS = -L../../../lib -lnesla -pg
OBJECTS = main.o
TARGET  = ../../../bin/neslite

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LDFLAGS)
	$(CC) -o $(TARGET)_d $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) *~
