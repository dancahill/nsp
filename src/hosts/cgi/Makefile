# MAKEFILE FOR NESLA CGI
CC=gcc
CFLAGS = -Wall -g -O2 -I../../../include
LDFLAGS = -s -L../../../lib -lnesla -lneslaext -lneslatcp -lneslamath -lm
#The following LDFLAGS might be needed for Solaris
#LDFLAGS = -lsocket -lnsl
OBJECTS = cgi.o config.o main.o
TARGET = ../../../bin/nesla.cgi

all: nesla.cgi

nesla.cgi: $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

clean:
	@rm -f $(TARGET) $(OBJECTS) *~
