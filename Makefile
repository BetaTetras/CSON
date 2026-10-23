# CSON : compilation de la bibliothèque
#
#   make            -> libcson.a (bibliothèque statique)
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

clean:
	rm -f cson.o $(LIB)

.PHONY: all clean
