# MAKEFILE FOR NESLA CGI

include ../../config.mak

#CC      = gcc
#CFLAGS  = -Wall -g -O2 -I../../../include
CFLAGS  += -I../../../include
LD_FLAGS = -L../../../lib -lnesla -lneslaext -lneslatcp $(LDFLAGS)
#The following LDFLAGS might be needed for Solaris
#LDFLAGS = -lsocket -lnsl
OBJECTS = cgi.o config.o main.o
TARGET = ../../../bin/nesla.cgi

all: nesla.cgi

nesla.cgi: $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LD_FLAGS)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
