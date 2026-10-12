# CSON : compilation de la bibliothèque, de l'exemple et des tests
#
#   make            -> libcson.a (bibliothèque statique)
#   make exemple    -> ./exemple (lit test.json, voir exemple.c)
#   make test       -> lance les tests
#   make valgrind   -> lance les tests sous valgrind (fuites et erreurs mémoire)
#   make clean      -> supprime tout ce qui a été compilé

CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2
AR     = ar

LIB    = libcson.a

all: $(LIB)

cson.o: cson.c cson.h
	$(CC) $(CFLAGS) -c cson.c -o cson.o

$(LIB): cson.o
	$(AR) rcs $(LIB) cson.o

exemple: exemple.c cson.h $(LIB)
	$(CC) $(CFLAGS) -I. exemple.c -L. -lcson -o exemple

tests/test_cson: tests/test_cson.c cson.h $(LIB)
	$(CC) $(CFLAGS) -g -I. tests/test_cson.c -L. -lcson -o tests/test_cson

test: tests/test_cson
	./tests/test_cson

valgrind: tests/test_cson
	valgrind --leak-check=full --error-exitcode=1 ./tests/test_cson

clean:
	rm -f cson.o $(LIB) exemple sortie.json tests/test_cson tests/tmp_*.json

.PHONY: all test valgrind clean
