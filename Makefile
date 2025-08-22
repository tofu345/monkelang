CFLAGS := -g -Wall -Wextra

main: .FORCE
	@ gcc ${CFLAGS} -o main.out \
		main.c repl.c lexer.c token.c parser.c grow_array.c parser_tracing.c ast.c

test_lexer: .FORCE
	@ gcc ${CFLAGS} -o test_lexer.out \
		unity/* test_lexer.c lexer.c token.c

test_parser: .FORCE
	@ gcc ${CFLAGS} -o test_parser.out \
		unity/* test_parser.c grow_array.c parser.c parser_tracing.c ast.c lexer.c token.c

test_ast: .FORCE
	@ gcc ${CFLAGS} -o test_ast.out \
		unity/* test_ast.c ast.c token.c

.FORCE:
