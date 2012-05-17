CFLAGS=-Wall -pedantic -std=c99

all: runtest critbit.o ffa.o util.o nutrient_patch nutrient_dump nutrient_query

ffa.o: ffa.c ffa.h

util.o: util.c util.h

critbit.o: critbit.c critbit.h 

test: test.c critbit.o ffa.o util.o

nutrient_patch: nutrient_patch.c critbit.o ffa.o util.o

nutrient_dump: nutrient_dump.c critbit.o ffa.o util.o

nutrient_query: nutrient_query.c critbit.o ffa.o util.o

runtest: test
	./test 

clean:
	rm -f test nutrient_query nutrient_patch nutrient_dump *.o
