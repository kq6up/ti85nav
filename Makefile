PYTHON ?= python3
CC = cc
CFLAGS = -O2 -Wall -Wextra
SOURCES := $(wildcard src/*.ASC)

.PHONY: programs converter clean
programs:
	$(PYTHON) tools/ascii85p.py $(SOURCES) --output-dir programs

converter: build/ascii85p

build/ascii85p: tools/ascii85p.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99 $(LDFLAGS) -o $@ tools/ascii85p.c

clean:
	rm -f build/ascii85p
