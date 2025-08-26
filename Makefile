CFLAGS := -g -Wall -Wextra

repl: .FORCE
	@ gcc ${CFLAGS} -o build/repl.out \
		main.c repl.c lexer.c token.c parser.c grow_array.c parser_tracing.c ast.c

test_lexer: .FORCE
	@ gcc ${CFLAGS} -o build/test_lexer.out \
		unity/* test_lexer.c lexer.c token.c

test_parser: .FORCE
	@ gcc ${CFLAGS} -o build/test_parser.out \
		unity/* test_parser.c grow_array.c parser.c parser_tracing.c ast.c lexer.c token.c

test_ast: .FORCE
	@ gcc ${CFLAGS} -o build/test_ast.out \
		unity/* test_ast.c ast.c token.c

.FORCE:
