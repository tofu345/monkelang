#include "token.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start(FILE* in, FILE* out) {
    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            return;
        }

        Lexer* l = lexer_new(input, strlen(input));

        for (Token tok = lexer_next_token(l);
                tok.type != Eof; tok = lexer_next_token(l)) {
            fprintf(out, "{token_type = %s; literal = \"%s\"}\n",
                    show_token_type(tok.type), tok.literal);
            free(tok.literal);
        }
        free(input);
    }
}
