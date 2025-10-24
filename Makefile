CC = gcc
CFLAGS = -g -Wall -Werror -Wextra -pedantic-errors

OBJS := $(patsubst src/%.c,build/%.o,$(wildcard src/*.c))

main: $(OBJS)
	@ $(CC) $(CFLAGS) -o build/$@ $^ main.c src/hash-table/ht.c

build/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(wildcard tests/*.c): $(OBJS) tests/*.h .FORCE
	@ $(CC) $(CFLAGS) -o $(patsubst tests/%.c,build/test_%,$@) $@ \
		$(OBJS) tests/helpers.c tests/unity/unity.c src/hash-table/ht.c

.FORCE:
