CFLAGS=-Wall -pedantic -std=c99 -g

all: runtest critbit.o nutrient_ffa.o nutrient_util.o nutrient_patch nutrient_dump nutrient_query

nutrient_ffa.o: nutrient_ffa.c nutrient_ffa.h

nutrient_util.o: nutrient_util.c nutrient_util.h

critbit.o: critbit.c critbit.h 

test: test.c critbit.o nutrient_ffa.o nutrient_util.o

nutrient_patch: nutrient_patch.c critbit.o nutrient_ffa.o nutrient_util.o

nutrient_dump: nutrient_dump.c critbit.o nutrient_ffa.o nutrient_util.o

nutrient_query: nutrient_query.c critbit.o nutrient_ffa.o nutrient_util.o

runtest: test
	./test 

clean:
	rm -f test nutrient_query nutrient_patch nutrient_dump *.o
