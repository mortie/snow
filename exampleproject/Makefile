TARGET = exampleproject

SRC = src/main.c src/vector.c
OBJ = $(patsubst src/%.c,obj/%.o,$(SRC))
OBJ-TEST = $(patsubst src/%.c,obj-test/%.o,$(SRC))
HDR = src/vector.h ../snow/snow.h

CFLAGS ?= -Wall -Wextra -Wpedantic -Werror
VGFLAGS ?= \
	--quiet --leak-check=full --show-leak-kinds=all \
	--track-origins=yes --error-exitcode=1

INCLUDE = -I..

$(TARGET): $(OBJ) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
test-$(TARGET): $(OBJ-TEST)
	$(CC) -g $(CFLAGS) -o $@ $(OBJ-TEST)

obj/%.o: src/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(CC) $(INCLUDE) $(CFLAGS) -c -o $@ $<
obj-test/%.o: src/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(CC) -DSNOW_ENABLED -g $(INCLUDE) $(CFLAGS) -c -o $@ $<

test: test-$(TARGET)
	valgrind $(VGFLAGS) ./test-$(TARGET) $(ARGS)

clean:
	rm -rf obj obj-test
	rm -f $(TARGET) test-$(TARGET)

.PHONY: test clean
