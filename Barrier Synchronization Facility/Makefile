TARGET=mybarrier.o mypipe.o 
CFLAGS=-Wall -pthread -D PIPE_BUF_MAX=1024
CC=gcc

all: $(TARGET)
	
mybarrier.o: mybarrier.c mybarrier.h
	$(CC) $(CFLAGS) -c $*.c
	

clean:
	rm -rf *.o *.a $(TARGET)

