CFLAGS := -g -Wall -Wextra

main: .FORCE
	@ gcc ${CFLAGS} -o main.out \
		main.c repl.c lexer.c token.c

test_lexer: .FORCE
	@ gcc ${CFLAGS} -o test_lexer.out \
		unity/* test_lexer.c lexer.c token.c

.FORCE:
