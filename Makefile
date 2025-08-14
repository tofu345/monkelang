FILES = test_lexer.c
OUTFILE = test_lexer
CFLAGS = -g -Wall -Wextra -Werror -o ${OUTFILE}

test_lexer: ${FILES} Makefile
	@ gcc ${CFLAGS} ${FILES}
