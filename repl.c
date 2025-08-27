#include "parser.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_parser_errors(FILE* out, Parser* p) {
    fprintf(out, "Woops! We ran into some monkey business here!\n");
    // fprintf(out, "parser errors:\n");
    for (size_t i = 0; i < p->errors_len; i++) {
        fprintf(out, "%s\n", p->errors[i]);
    }
}

void start(FILE* in, FILE* out) {
    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            break;
        }

        Lexer l = lexer_new(input, strlen(input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        if (p->errors_len != 0) {
            print_parser_errors(out, p);
            continue;
        }

        program_fprint(&prog, out);
        fprintf(out, "\n");

        program_destroy(&prog);
        parser_destroy(p);
        free(input);
    }
}
