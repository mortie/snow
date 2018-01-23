VGFLAGS ?= --quiet --leak-check=full --show-leak-kinds=all --track-origins=yes

example: example.c test/test.h
	$(CC) $(CFLAGS) -g -Wall -o $@ $<

test: example
	valgrind $(VGFLAGS) ./example

clean:
	rm -f example

.PHONY: test clean
