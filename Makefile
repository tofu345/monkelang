CFLAGS := -g -Wall -Wextra

repl: .FORCE
	@ gcc ${CFLAGS} -o build/repl.out \
		main.c repl.c lexer.c token.c parser.c utils.c \
		parser_tracing.c ast.c evaluator.c object.c

test_lexer: .FORCE
	@ gcc ${CFLAGS} -o build/test_lexer.out \
		unity/* test_lexer.c lexer.c token.c

test_parser: .FORCE
	@ gcc ${CFLAGS} -o build/test_parser.out \
		unity/* test_parser.c utils.c parser.c parser_tracing.c \
		ast.c lexer.c token.c

test_ast: .FORCE
	@ gcc ${CFLAGS} -o build/test_ast.out \
		unity/* test_ast.c ast.c token.c

test_evaluator: .FORCE
	@ gcc ${CFLAGS} -o build/test_evaluator.out \
		unity/* test_evaluator.c evaluator.c object.c utils.c \
		parser.c parser_tracing.c ast.c lexer.c token.c

.FORCE:
