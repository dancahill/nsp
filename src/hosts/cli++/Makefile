# Makefile for NullLogic Embedded Scripting Language

include ../../config.mak

CC       = g++
#CPPFLAGS = -Wall -g -O2 -I../../../include
CPPFLAGS += -I../../../include
LD_FLAGS = -L../../../lib -lnesla -lneslaext -lneslatcp $(LDFLAGS)

# if OSTYPE==cygwin
# LDFLAGS += -lws2_32

OBJECTS = main.o
TARGET  = ../../../bin/nesla++

all: _nesla

_nesla: $(OBJECTS)
	$(CC) -o $(TARGET) -s $(OBJECTS) $(LD_FLAGS)
	$(CC) -o $(TARGET)_d $(OBJECTS) $(LD_FLAGS)
#	$(CC) -o $(TARGET)_s -s -static $(OBJECTS) $(LD_FLAGS)

clean:
	rm -f $(TARGET) $(TARGET)_d $(TARGET)_s $(OBJECTS) *~

test:
	../../../bin/nesla++ ../../../scripts/tests/test1.ns
