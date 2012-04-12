CFLAGS=-Wall -pedantic -std=c99

all: runtest critbit.o ffa.o

ffa.o: ffa.c ffa.h

critbit.o: critbit.c critbit.h

test: test.c critbit.o ffa.o nutrient.h

runtest: test
	./test 

clean:
	rm -f test *.o
