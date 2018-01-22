VGFLAGS ?= --quiet --leak-check=full --show-leak-kinds=all --track-origins=yes

example: example.c test/test.h
	gcc -g -o $@ $<

test: example
	valgrind $(VGFLAGS) ./example
