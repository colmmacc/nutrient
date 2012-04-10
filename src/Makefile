CFLAGS=-Wall -pedantic -std=c99

all: runtest critbit.o

critbit.o: critbit.c critbit.h

test: test.c critbit.o

runtest: test
	./test 

clean:
	rm -f test *.o
