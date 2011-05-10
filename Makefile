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


build/$(SOFILE): build/nvt.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o build/$(SOFILE) build/nvt.o

build/libanachronism.a: build/nvt.o
	ar rcs build/libanachronism.a build/nvt.o

build/nvt.o: src/nvt.c $(INCLUDE)/nvt.h
	$(CC) $(FLAGS) $(CFLAGS) src/nvt.c -o build/nvt.o

src/nvt.c: src/nvt.rl src/parser_common.rl
	ragel -C -G2 src/nvt.rl -o src/nvt.c


install: all
	install -D -d /usr/local/include/anachronism/ /usr/local/lib
	install -D include/anachronism/* /usr/local/include/anachronism/
	install -D build/$(SOFILE) /usr/local/lib/$(SOFILE)
	install -D build/libanachronism.a /usr/local/lib/libanachronism.a
	ln -s -f /usr/local/lib/$(SOFILE) /usr/local/lib/$(SONAME)
	ln -s -f /usr/local/lib/$(SOFILE) /usr/local/lib/$(SO)

uninstall:
	-rm -rf /usr/local/include/anachronism
	-rm /usr/local/lib/libanachronism.a
	-rm /usr/local/lib/$(SOFILE)
	-rm /usr/local/lib/$(SONAME)
	-rm /usr/local/lib/$(SO)

clean:
	-rm -f build/nvt.o

distclean: clean
	-rm -f build/libanachronism.a build/$(SOFILE)

.PHONY: all static shared clean distclean install uninstall
