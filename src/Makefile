CFLAGS=-Wall -pedantic -std=c99 -Werror -g

all: runtest nutrient.o nutrient_ffa.o nutrient_util.o nutrient_patch nutrient_dump nutrient_query nutrient_prefix

nutrient_ffa.o: nutrient_ffa.c nutrient_ffa.h

nutrient_util.o: nutrient_util.c nutrient_util.h

nutrient.o: nutrient.c nutrient.h 

test: test.c nutrient.o nutrient_ffa.o nutrient_util.o

nutrient_patch: nutrient_patch.c nutrient.o nutrient_ffa.o nutrient_util.o

nutrient_dump: nutrient_dump.c nutrient.o nutrient_ffa.o nutrient_util.o

nutrient_query: nutrient_query.c nutrient.o nutrient_ffa.o nutrient_util.o

nutrient_prefix: nutrient_prefix.c nutrient.o nutrient_ffa.o nutrient_util.o

runtest: test
	./test 

clean:
	rm -f test nutrient_query nutrient_patch nutrient_dump *.o
