#include "cli.h"
#include "code.h"
#include "compiler.h"
#include "errors.h"
#include "lexer.h"
#include "object.h"
#include "parser.h"
#include "shared.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

// It works, but it requires a module to explicitly `return` a value for it to
// be exposed. not a bad idea tbh. will polish this tmr.

typedef struct {
    char *source;
    Program program;
} Input;

BUFFER(Input, Input)
DEFINE_BUFFER(Input, Input)

// getline() but if there is no more data to read or the first line ends with
// '{' or '('.  Then read and append multiple lines to [input] until a blank
// line is encountered and there is no data left in [in_stream].
static int multigetline(char **input, size_t *input_cap, FILE *in, FILE *out);

void repl(FILE* in, FILE* out) {
    Lexer l;
    lexer_init(&l, NULL, 0);

    Compiler c;
    compiler_init(&c);
    enter_scope(&c);

    VM vm;
    vm_init(&vm, &c);

    InputBuffer inputs = {0};
    Program program = {0};
    ParseErrorBuffer p_errors = {0};

    error err = NULL;
    char *input = NULL;
    int len;
    size_t cap = 0;
    while (1) {
        len = multigetline(&input, &cap, in, out);
        if (len == -1) {
            free(input);
            break;

        } else if (len == 1) {
            // only '\n' was entered.
            continue;
        }

        lexer_with(&l, input, len);
        p_errors = parse(parser(), &l, &program);
        if (p_errors.length > 0) {
            fputs(parser_error_msg, out);
            print_parse_errors(&p_errors, out);
            goto cleanup;

        } else if (program.stmts.length == 0) {
            goto cleanup;
        }

        err = compile(&c, &program);
        if (err) {
            fputs(compiler_error_msg, out);
            print_error(err, out);
            goto cleanup;
        }

        InputBufferPush(&inputs, (Input){ input, program });
        input = NULL;
        program = (Program){0};

        err = vm_run(&vm, bytecode(&c));
        if (err) {
            fputs(vm_error_msg, out);
            print_vm_stack_trace(&vm, out);
            print_error(err, out);
            goto cleanup;
        }

        Object stack_elem = vm_last_popped(&vm);
        if (stack_elem.type != o_Nothing) {
            object_fprint(stack_elem, out);
            fputc('\n', out);
        }

cleanup:
        free_error(err);
        err = NULL;

        for (int i = 0; i < program.stmts.length; i++) {
            node_free(program.stmts.data[i]);
        }
        program.stmts.length = 0;

        compiler_reset(&c);
        c.cur_instructions->length = 0;
        c.cur_mappings->length = 0;

        vm.sp = 0;
        vm.frames_index = 0;
        vm.stack[0] = (Object){0}; // remove stack_elem
    }

    vm_free(&vm);
    compiler_free(&c);
    free_parse_errors(&p_errors);
    for (int i = 0; i < inputs.length; ++i) {
        free(inputs.data[i].source);
        program_free(&inputs.data[i].program);
    }
    free(inputs.data);
}

int run(char* filename) {
    bool success = false;

    char *source;
    error err = load_file(filename, &source);
    if (err) {
        print_error(err, stdout);
        free_error(err);
        return -1;
    }

    Lexer l;
    lexer_init(&l, source, strlen(source));

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, &c);

    Program program = {0};
    ParseErrorBuffer errors = parse(parser(), &l, &program);
    if (errors.length > 0) {
        fputs(parser_error_msg, stdout);
        print_parse_errors(&errors, stdout);
        goto cleanup;
    }

    err = compile(&c, &program);
    if (err) {
        fputs(compiler_error_msg, stdout);
        print_error(err, stdout);
        free_error(err);
        goto cleanup;
    }

    err = vm_run(&vm, bytecode(&c));
    if (err) {
        fputs(vm_error_msg, stdout);
        print_vm_stack_trace(&vm, stdout);
        print_error(err, stdout);
        free_error(err);
    }

    success = true;

cleanup:
    vm_free(&vm);
    compiler_free(&c);
    program_free(&program);
    free_parse_errors(&errors);
    free(source);
    return success ? 0 : 1;
}

static int
multigetline(char **input, size_t *input_cap, FILE *in, FILE *out) {
    fprintf(out, ">> ");

    int len = getline(input, input_cap, in);
    if (len == -1) { return len; }

    struct pollfd fds;
    fds.fd = fileno(in);
    fds.events = POLLIN;
    int ret = poll(&fds, 1, 0);

    // ret is 0 if there is no data to read.
    if (ret == 0) {
        // only '\n' was entered
        if (len == 1) {
            return len;
        }

        // last character before '\n'
        char last_ch = (*input)[len - 2];
        if (last_ch != '{' && last_ch != '(') {
            return len;
        }
    }

    char *line = NULL;
    size_t line_cap = 0,
           capacity = *input_cap;

    while (1) {
        // Skip printing '.. ' when a multiline string is pasted.
        ret = poll(&fds, 1, 0);
        if (ret == 0) {
            // print '.. ' on a newline
            if ((*input)[len - 1] != '\n') {
                putc('\n', out);
            }
            fprintf(out, ".. ");
        }

        int line_len = getline(&line, &line_cap, in);
        if (line_len == -1) { // EOF or error
            putc('\n', out);
            break;
        }

        // blank line and no data to read.
        if (line_len == 1 && ret == 0) {
            (*input)[len] = '\0';
            break;
        }

        // realloc [input] if necessary
        if ((size_t)(len + line_len) > capacity) {
            capacity *= 2;
            char *ptr = realloc(*input, capacity);
            if (ptr == NULL) {
                free(line);
                return -1;
            }
            *input = ptr;
            *input_cap = capacity;
        }

        // append new line to [input]
        strncpy((*input) + len, line, line_len);
        len += line_len;
    }

    free(line);
    return len;
}
