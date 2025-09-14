CFLAGS := -g -Wall -Wextra

OBJS := $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))
DEPS := src/buffer.h

main: $(OBJS)
	@ $(CC) $(CFLAGS) -o build/$@ $^ main.c src/hash-table/ht.c

build/%.o: src/%.c src/%.h $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< $(CFLAGS)

tests/$(wildcard tests/*.c): $(OBJS)
	@ $(CC) $(CFLAGS) -o $(patsubst tests/%.c,build/%,$@) $@ $^ \
		tests/unity/unity.c src/hash-table/ht.c
