# Makefile for NullLogic Embedded Scripting Language

SRCDIR  = /home/null/gw/nullgroupware
CC      = gcc
MAKE    = make -s
CFLAGS  = -Wall -O2 -fPIC -I$(SRCDIR)/include -I../../../include
LDFLAGS = -s -shared -lm
OBJECTS = mod_nesla.o
TARGET  = $(SRCDIR)/distrib/lib/ghttpd/mod_nesla.so

all: mod_nesla

mod_nesla: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) ../../../lib/libnesla.a

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
