#include "ast.h"
#include "object.h"
#include "parser.h"
#include "lexer.h"
#include "evaluator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_parser_errors(FILE* out, Parser* p) {
    fprintf(out, "Woops! We ran into some monke business here!\n");
    // fprintf(out, "parser errors:\n");
    for (size_t i = 0; i < p->errors_len; i++) {
        fprintf(out, "%s\n", p->errors[i]);
    }
}

void start(FILE* in, FILE* out) {
    printf("Hello %s! This is the Monke Programming Language!\n",
            getlogin());

    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;

        if (getline(&input, &len, in) == -1) {
            break;
        }

        Lexer l = lexer_new(input);
        Parser p;
        parser_init(&p, &l);
        Program prog = parse_program(&p);
        if (p.errors_len != 0) {
            print_parser_errors(out, &p);
            continue;
        }

        Object evaluated = eval_program(&prog);
        if (evaluated.typ != o_Null) {
            object_fprint(evaluated, out);
            fprintf(out, "\n");

            object_destroy(&evaluated);
        }

        program_destroy(&prog);
        parser_destroy(&p);
        free(input);
    }
}
