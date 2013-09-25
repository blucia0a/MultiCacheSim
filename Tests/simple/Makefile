CC=gcc
CFLAGS += -pthread -g -O3 
LDFLAGS= -pthread 

all:
	$(CC) -c $(CFLAGS) -o simple.o simple.c
	$(CC) $(LDFLAGS) -o simple ./simple.o

clean:
	-rm *.o
	-rm simple 
