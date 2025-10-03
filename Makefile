CC = gcc
CFLAGS = -g -Wall -Wextra

OBJS := $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))
DEPS := src/buffer.h

main: $(OBJS)
	@ $(CC) $(CFLAGS) -o build/$@ $^ main.c src/hash-table/ht.c

build/%.o: src/%.c src/%.h $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(wildcard tests/*.c): $(OBJS) tests/compiler.h .FORCE
	@ $(CC) $(CFLAGS) -o $(patsubst tests/%.c,build/test_%,$@) $@ $(OBJS) \
		tests/unity/unity.c src/hash-table/ht.c

.FORCE:
