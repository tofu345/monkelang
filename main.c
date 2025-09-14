#include "src/repl.h"

#include <stdlib.h>
#include <unistd.h>

char* read_file(char* filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "error: could not open file %s", filename);
    }
    fseek(fp, 0, SEEK_END);
    long bufsize = ftell(fp);
    rewind(fp);

    char* buf = malloc((bufsize + 1) * sizeof(char));
    size_t new_len = fread(buf, sizeof(char), bufsize, fp);
    if (ferror(fp) != 0) {
        fprintf(stderr, "error: could not read file %s", filename);
    } else {
        buf[new_len++] = '\0';
    }
    fclose(fp);
    return buf;
}

int run_program(char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "error: could not open file %s\n", filename);
        return EXIT_FAILURE;
    }

    char* program = read_file(filename);
    run(program);
    free(program);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("Hello %s! This is the Monke Programming Language!\n",
                getlogin());
        repl(stdin, stdout);
        return EXIT_SUCCESS;

    } else if (argc > 2) {
        fprintf(stderr, "error: expect only optional path to program\n");
        return EXIT_FAILURE;
    }
    return run_program(argv[1]);
}
