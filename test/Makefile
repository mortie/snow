CASESRC = $(shell find cases -name "*.c")
CASES = $(patsubst %.c,%,$(CASESRC))

CFLAGS ?= -g -Wall -Wextra -Wpedantic -Werror
FLAGS = $(CFLAGS) -DSNOW_ENABLED
VGFLAGS ?= \
	--quiet --leak-check=full --show-leak-kinds=all \
	--track-origins=yes --error-exitcode=1

test-snow: test.c $(CASES) snow/.git
	$(CC) -Isnow $(FLAGS)  -o $@ $<

cases/%: cases/%.c ../snow/snow.h
	$(CC) -DSNOW_DUMMY_TIMER -I.. $(FLAGS) -o $@ $<

snow/.git:
	git clone -b experimentation https://github.com/mortie/snow.git snow
	cd snow && git checkout v2.3.0

test: test-snow
	valgrind $(VGFLAGS) ./test-snow $(ARGS)

clean:
	rm -f test-snow $(CASES)
	rm -rf snow

.PHONY: test clean
