CASESRC = $(shell find cases -name "*.c")
CASES = $(patsubst %.c,%,$(CASESRC))

CFLAGS ?= -g -Wall -Wextra -std=c11
FLAGS = $(CFLAGS) -DSNOW_ENABLED
VGFLAGS ?= --quiet --leak-check=full --show-leak-kinds=all --track-origins=yes

test: test.c $(CASES) snow/snow.h
	$(CC) -I. $(FLAGS)  -o $@ $<

cases/%: cases/%.c ../snow/snow.h
	$(CC) -I.. $(FLAGS) -o $@ $<

run-test: test
	valgrind $(VGFLAGS) ./test

clean:
	rm -f test $(CASES)

.PHONY: run-test clean
