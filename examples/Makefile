CFLAGS=-I . -I .. -O0 --std=gnu99  $(CDEBUGFLAGS)
LDFLAGS=-O0  $(CDEBUGFLAGS)
CC=gcc
LD=gcc

all: example1

lineedit:
	$(CC) $(CFLAGS) -c ../lineedit.c

example1: lineedit
	$(CC) $(CFLAGS) -c example1.c
	$(LD) $(LDFLAGS) lineedit.o example1.o -o example1


