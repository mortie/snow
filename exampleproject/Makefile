TARGET = exampleproject

SRC = src/main.c src/vector.c
OBJ = $(patsubst src/%.c,obj/%.o,$(SRC))
OBJ-TEST = $(patsubst src/%.c,obj-test/%.o,$(SRC))
HDR = src/vector.h ../snow/snow.h

CFLAGS ?= -Wall -Wextra -Wno-unused-parameter -std=c11
VGFLAGS ?= --quiet --leak-check=full --show-leak-kinds=all --track-origins=yes

INCLUDE = -I..

$(TARGET): $(OBJ) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
$(TARGET)-test: $(OBJ-TEST)
	$(CC) -g $(CFLAGS) -o $@ $(OBJ-TEST)

obj/%.o: src/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(CC) -DSNOW_DISABLED=1 $(INCLUDE) $(CFLAGS) -c -o $@ $<
obj-test/%.o: src/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(CC) -g $(INCLUDE) $(CFLAGS) -c -o $@ $<

test: $(TARGET)-test
	valgrind $(VGFLAGS) ./$(TARGET)-test

clean:
	rm -rf obj obj-test
	rm -f $(TARGET) $(TARGET)-test

.PHONY: test clean
