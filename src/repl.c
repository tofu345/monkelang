#include "compiler.h"
#include "object.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_errors(FILE* out, ErrorBuffer *buf) {
    for (int i = 0; i < buf->length; i++) {
        fprintf(out, "%s\n", buf->data[i]);
        free(buf->data[i]);
    }
    if (buf->length >= MAX_ERRORS)
        fprintf(out, "too many errors, stopping now\n");
}

void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);
    Program prog;

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    char *input;
    int err;
    size_t len;
    Object stack_elem;
    while (1) {
        fprintf(out, ">> ");
        input = NULL;
        len = 0;
        if (getline(&input, &len, in) == -1) {
            free(input);
            break;
        }

        prog = parse(&p, input);
        if (p.errors.length > 0) {
            fprintf(out, "Woops! We ran into some monkey business here!\n");
            print_errors(out, &p.errors);
            goto cleanup;
        }

        err = compile(&c, &prog);
        if (err != 0) {
            fprintf(out, "Woops! Compilation failed:\n");
            print_errors(out, &c.errors);
            goto cleanup;
        }

        vm_with(&vm, bytecode(&c));
        err = vm_run(&vm);
        if (err != 0) {
            fprintf(out, "Woops! Executing bytecode failed:\n");
            print_errors(out, &vm.errors);
            goto cleanup;
        }

        stack_elem = vm_last_popped(&vm);
        object_fprint(stack_elem, out);
        fputc('\n', out);
        vm.stack[0] = (Object){}; // remove stack_elem

cleanup:
        free(input);
        program_free(&prog);
        free(p.peek_token.literal);
        p.peek_token.literal = NULL;
        p.errors.length = 0;

        c.errors.length = 0;
        c.instructions.length = 0;
        c.constants.length = 0;

        vm.errors.length = 0;
    }

    parser_free(&p);
    compiler_free(&c);
    vm_free(&vm);
}
