CFLAGS=-Wall -pedantic -std=c99

all: runtest critbit.o ffa.o util.o

ffa.o: ffa.c ffa.h

util.o: util.c util.h

critbit.o: critbit.c critbit.h 

test: test.c critbit.o ffa.o util.o

runtest: test
	./test 

clean:
	rm -f test *.o
