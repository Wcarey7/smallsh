#CFLAGS = -Wall
# -fpic -coverage -lm
# GCOV = -ftest-coverage -fprofile-arcs

smallsh: smallsh.c
	gcc -o smallsh smallsh.c -g $(CFLAGS)

all: smallsh

clean:
	rm -f *.o smallsh *.gcov *.gcda *.gcno *.so