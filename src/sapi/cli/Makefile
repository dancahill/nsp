# Makefile for NullLogic Embedded Scripting Language
CC      = gcc
#CFLAGS  = -pedantic -Wall -g -O2 -I../../../include -I../../libnesla -I../../libneslaext
CFLAGS  = -Wall -g -O2 -I../../../include -I../../libnesla -I../../libneslaext
LDFLAGS = -L../../../lib -lnesla -lneslaext -lm
OBJECTS = main.o
TARGET  = ../../../bin/nesla

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LDFLAGS)
	$(CC) -o $(TARGET)_d $(OBJECTS) $(LDFLAGS)
	$(CC) -o $(TARGET)_s -s -static $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET)_d $(TARGET)_s $(OBJECTS) *~
