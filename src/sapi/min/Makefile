# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
CFLAGS  = -Wall -g -O2 -I../../../include -I../../libnesla
LDFLAGS = -L../../../lib -lnesla -lm
OBJECTS = main.o
TARGET  = ../../../bin/nesla_m

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LDFLAGS)
#	$(CC) -o $(TARGET)_d $(OBJECTS) $(LDFLAGS)
#	$(CC) -o $(TARGET)_s -s -static $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET)_d $(TARGET)_s $(OBJECTS) *~
