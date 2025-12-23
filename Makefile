CFLAGS = -g -Wall -Werror -Wextra -pedantic-errors

OBJS = $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))
DEPS = src/hash-table/ht.c

.PHONY: main
main: $(OBJS)
	@ $(CC) $(CFLAGS) $^ $(DEPS) main.c -o build/$@

build/allocation.o: src/vm.h # recompile when changing DEBUG

build/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -c $< -o $@

TEST_DEPS = tests/helpers.c tests/unity/unity.c src/hash-table/ht.c

test_%: tests/%.c .FORCE
	@ $(CC) $(CFLAGS) $< $(OBJS) $(TEST_DEPS) -o build/$@

.FORCE:
