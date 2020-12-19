CASESRC = $(shell find cases -name "*.c")
CASES = $(patsubst %.c,%,$(CASESRC))

FLAGS = -g $(WFLAGS) -DSNOW_ENABLED \
	-Werror -Wall -Wextra -Wpedantic $(CFLAGS)
VGFLAGS ?= \
	--quiet --leak-check=full --show-leak-kinds=all \
	--track-origins=yes --error-exitcode=1

test-snow: test.c $(CASES) snow/.git
	$(CC) -Isnow $(FLAGS)  -o $@ $<

cases/%: cases/%.c ../snow/snow.h
	$(CC) -DSNOW_DUMMY_TIMER -I.. $(FLAGS) -o $@ $<

snow/.git:
	git clone -b experimentation https://github.com/mortie/snow.git snow
	cd snow && git checkout v2.3.2

.PHONY: test
test: test-snow
	valgrind $(VGFLAGS) ./test-snow $(ARGS)

.PHONY: check
check: GCC ?= gcc
check: CLANG ?= clang
check:
	@printf "\n*** Compiling and running with GCC...\n"
	$(MAKE) clean
	$(MAKE) CC=$(GCC) test-snow
	valgrind $(VGFLAGS) ./test-snow -q
	@printf "\n*** Compiling and running with Clang...\n"
	$(MAKE) clean
	$(MAKE) CC=$(CLANG) CFLAGS=" \
		-Wincompatible-pointer-types-discards-qualifiers -Wmissing-prototypes -Wwrite-strings" \
		test-snow
	valgrind $(VGFLAGS) ./test-snow -q

.PHONY: clean
clean:
	rm -f test-snow $(CASES)

.PHONY: distclean
distclean: clean
	rm -rf snow
