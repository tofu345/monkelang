#include "parser.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_parser_errors(FILE* out, Parser* p, const char* monkey_face) {
    // fprintf(out, "%s", monkey_face);
    // fprintf(out, "Woops! We ran into some monkey business here!\n");
    fprintf(out, "parser errors:\n");
    for (size_t i = 0; i < p->errors_len; i++) {
        fprintf(out, "%s\n", p->errors[i]);
        // fprintf(out, "\t%s\n", p->errors[i]);
    }
}

char* load_monkey_face(void) {
    const char* filename = "monkey_face.txt";
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "could not open file: %s", filename);
    }
    fseek(fp, 0, SEEK_END);
    long bufsize = ftell(fp);
    rewind(fp);

    char* buf = malloc((bufsize + 1) * sizeof(char));
    size_t len = fread(buf, sizeof(char), bufsize, fp);
    if (ferror(fp) != 0) {
        fprintf(stderr, "error reading file: %s", filename);
    } else {
        buf[len++] = '\0';
    }
    fclose(fp);
    return buf;
}

void start(FILE* in, FILE* out) {
    const char* monkey_face = load_monkey_face();
    while (1) {
        fprintf(out, ">> ");
        char* input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            return;
        }

        Lexer l = lexer_new(input, strlen(input));
        Parser* p = parser_new(&l);
        Program prog = parse_program(p);
        if (p->errors_len != 0) {
            print_parser_errors(out, p, monkey_face);
            continue;
        }

        program_fprint(&prog, out);
        fprintf(out, "\n");

        program_destroy(&prog);
        parser_destroy(p);
        free(input);
    }
    free((char*)monkey_face);
}
