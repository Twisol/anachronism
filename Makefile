SHELL = sh

CC     = gcc
FLAGS  = -c -fPIC -Iinclude/
CFLAGS = --pedantic -Wall -Wextra -march=native -std=gnu99
INCLUDE = include/anachronism

VERSION_MAJOR = 0
VERSION = $(VERSION_MAJOR).1.0

SO = libanachronism.so
SOFILE = $(SO).$(VERSION)
SONAME = $(SO).$(VERSION_MAJOR)


all: static shared
shared: build/$(SOFILE)
static: build/libanachronism.a


build/$(SOFILE): build/anachronism.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o build/$(SOFILE) build/anachronism.o

build/libanachronism.a: build/anachronism.o
	ar rcs build/libanachronism.a build/anachronism.o

build/anachronism.o: src/anachronism.c $(INCLUDE)/anachronism.h
	$(CC) $(FLAGS) $(CFLAGS) src/anachronism.c -o build/anachronism.o

src/anachronism.c: src/anachronism.rl src/parser_common.rl
	ragel -C -G2 src/anachronism.rl -o src/anachronism.c


install: all
	install -D -d /usr/local/include/anachronism/ /usr/local/lib
	install -D include/anachronism/* /usr/local/include/anachronism/
	install -D build/$(SOFILE) /usr/local/lib/$(SOFILE)
	install -D build/libanachronism.a /usr/local/lib/libanachronism.a
	ln -s /usr/local/lib/$(SOFILE) /usr/local/lib/$(SONAME)
	ln -s /usr/local/lib/$(SOFILE) /usr/local/lib/$(SO)

uninstall:
	-rm -rf /usr/local/include/anachronism
	-rm /usr/local/lib/libanachronism.a
	-rm /usr/local/lib/$(SOFILE)
	-rm /usr/local/lib/$(SONAME)
	-rm /usr/local/lib/$(SO)

clean:
	-rm -f build/anachronism.o

distclean: clean
	-rm -f build/libanachronism.a build/$(SOFILE)

.PHONY: all static shared clean distclean install uninstall
