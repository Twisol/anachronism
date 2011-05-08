CC     = gcc
FLAGS  = -c -fPIC -I.
CFLAGS = --pedantic -Wall -Wextra -march=native -std=gnu99


all: static shared
shared: src/libanachronism.so.1.0.0
static: src/libanachronism.a


src/libanachronism.so.1.0.0: src/anachronism.o
	$(CC) -shared -Wl,-soname,libanachronism.so.1 -o src/libanachronism.so.1.0.0 src/anachronism.o

src/libanachronism.a: src/anachronism.o
	ar rcs src/libanachronism.a src/anachronism.o

src/anachronism.o: src/anachronism.c src/anachronism.h
	$(CC) $(FLAGS) $(CFLAGS) src/anachronism.c -o src/anachronism.o

src/anachronism.c: src/anachronism.rl
	ragel -C -G2 src/anachronism.rl -o src/anachronism.c

src/anachronism.rl: src/parser_common.rl


install:
	install -D src/libanachronism.so.1.0.0 /usr/local/lib/libanachronism.so.1.0.0
	install -D src/libanachronism.a /usr/local/lib/libanachronism.a

uninstall:
	-rm /usr/local/lib/libanachronism.a
	-rm /usr/local/lib/libanachronism.so.1.0.0

clean:
	-rm -f src/anachronism.o src/anachronism.c

distclean: clean
	-rm -f src/libanachronism.a src/libanachronism.so.1.0.0

.PHONY: all static shared clean distclean install uninstall
