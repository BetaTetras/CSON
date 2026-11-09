# CSON : compilation de la bibliothèque et des tests
#
#   make            -> libcson.a (bibliothèque statique)
#   make test       -> lance les tests (test_windows/main.c, marche aussi sous Linux)
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

test_windows/test_cson: test_windows/main.c cson.h $(LIB)
	$(CC) $(CFLAGS) -g -I. test_windows/main.c -L. -lcson -o test_windows/test_cson

# Les tests lisent test.json dans leur dossier : on les lance depuis test_windows/
test: test_windows/test_cson
	cd test_windows && ./test_cson

valgrind: test_windows/test_cson
	cd test_windows && valgrind --leak-check=full --error-exitcode=1 ./test_cson

clean:
	rm -f cson.o $(LIB) test_windows/test_cson test_windows/sortie_test.json

.PHONY: all test valgrind clean
