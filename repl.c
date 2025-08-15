#include "token.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int start(FILE* in, FILE* out) {
    char* input = NULL;
    size_t len = 0;
    while (1) {
        fprintf(out, ">> ");
        getline(&input, &len, in);
        if (input == NULL) {
            printf("getline fail\n");
            return 1;
        }

        Lexer* l = lexer_new(input, strlen(input));

        for (Token tok = lexer_next_token(l);
                tok.type != Eof; tok = lexer_next_token(l)) {
            fprintf(out, "{token_type = %s; literal = \"%s\"}\n",
                    show_token_type(tok.type), tok.literal);
            free(tok.literal);
        }
        fflush(out);
    }

    free(input);
    return 0;
}
