CFLAGS := -g -Wall -Wextra

main: .FORCE
	@ gcc ${CFLAGS} -o build/main.out \
		main.c repl.c lexer.c token.c parser.c parser_tracing.c ast.c \
		evaluator.c object.c environment.c hash-table/ht.c utils.c \
		builtin.c

test_lexer: .FORCE
	@ gcc ${CFLAGS} -o build/test_lexer.out \
		unity/* test_lexer.c lexer.c token.c utils.c

test_parser: .FORCE
	@ gcc ${CFLAGS} -o build/test_parser.out \
		unity/* test_parser.c parser.c parser_tracing.c ast.c lexer.c \
		token.c utils.c object.c hash-table/ht.c

test_ast: .FORCE
	@ gcc ${CFLAGS} -o build/test_ast.out \
		unity/* test_ast.c ast.c token.c utils.c object.c hash-table/ht.c

test_evaluator: .FORCE
	@ gcc ${CFLAGS} -o build/test_evaluator.out \
		unity/* test_evaluator.c evaluator.c object.c parser.c \
		parser_tracing.c ast.c lexer.c token.c environment.c hash-table/ht.c \
		utils.c builtin.c

.FORCE:
