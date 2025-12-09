#include "repl.h"
#include "compiler.h"
#include "errors.h"
#include "object.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

// Successfully compiled and run input.
typedef struct {
    char *input;
    Program program;
} Eval;

BUFFER(Done, Eval)
DEFINE_BUFFER(Done, Eval)

static void print_parser_errors(FILE* out, Parser *p);

// getline() but if there is no more data to read or the first line ends with
// '{' or '('.  Then read and append multiple lines to [input] until a blank
// line is encountered and there is no data left in [in_stream].
static int multigetline(char **input, size_t *input_cap,
                        FILE *in_stream, FILE *out_stream);

void repl(FILE* in_stream, FILE* out_stream) {
    Parser p;
    parser_init(&p);

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    DoneBuffer done = {0};

    char *input = NULL;
    int len;
    size_t cap = 0;
    Program prog;
    while (1) {
        len = multigetline(&input, &cap, in_stream, out_stream);
        if (len == -1) {
            free(input);
            break;

        // only '\n' was entered.
        } else if (len == 1) {
            continue;
        }

        prog = parse_(&p, input, len);
        if (p.errors.length > 0) {
            print_parser_errors(out_stream, &p);
            goto cleanup;
        } else if (prog.stmts.length == 0) {
            goto cleanup;
        }

        Error *e = compile(&c, &prog);
        if (e) {
            fprintf(out_stream, "Woops! Compilation failed!\n");
            print_error(e);
            free_error(e);
            goto cleanup;
        }

        error err = vm_run(&vm, bytecode(&c));
        if (err) {
            fprintf(out_stream, "Woops! Executing bytecode failed!\n");
            print_vm_stack_trace(&vm);
            puts(err);
            free(err);
            goto cleanup;
        }

        Object stack_elem = vm_last_popped(&vm);
        if (stack_elem.type != o_Null) {
            object_fprint(stack_elem, out_stream);
            fputc('\n', out_stream);
        }

        DoneBufferPush(&done, (Eval){
            .input = input,
            .program = prog,
        });
        goto cleanup_;

cleanup:
        free(input);
        program_free(&prog);

cleanup_:
        input = NULL;
        prog = (Program){0};

        // reset compiler
        for (; c.cur_scope_index > 0; --c.cur_scope_index) {
            // free unsuccessfully [CompiledFunctions]
            free_function(c.scopes.data[c.cur_scope_index].function);
        }
        c.scopes.length = 1;
        c.current_instructions->length = 0;
        c.cur_mapping->length = 0;

        // reset VM
        vm.sp = 0;
        vm.frames_index = 0;
        vm.stack[0] = (Object){0}; // remove stack_elem
    }

    vm_free(&vm);
    compiler_free(&c);
    parser_free(&p);
    for (int i = 0; i < done.length; i++) {
        free(done.data[i].input);
        program_free(&done.data[i].program);
    }
    free(done.data);
}

void run(char* program) {
    Parser p;
    Program prog;
    Compiler c;
    VM vm;

    parser_init(&p);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    prog = parse(&p, program);
    if (p.errors.length > 0) {
        print_parser_errors(stdout, &p);
        goto cleanup;
    } else if (prog.stmts.length == 0) {
        goto cleanup;
    }

    Error *e = compile(&c, &prog);
    if (e) {
        fprintf(stdout, "Woops! Compilation failed!\n");
        print_error(e);
        free_error(e);
        goto cleanup;
    }

    error err = vm_run(&vm, bytecode(&c));
    if (err) {
        fprintf(stdout, "Woops! Executing bytecode failed!\n");
        print_vm_stack_trace(&vm);
        puts(err);
        free(err);
        goto cleanup;
    }

cleanup:
    vm_free(&vm);
    compiler_free(&c);
    program_free(&prog);
    parser_free(&p);
}

static void
print_parser_errors(FILE* out_stream, Parser *p) {
    fprintf(out_stream, "Woops! We ran into some monkey business here!\n");
    for (int i = 0; i < p->errors.length; i++) {
        Error *err = &p->errors.data[i];
        print_error(err);
        free(err->message);
    }
    p->errors.length = 0;
}

static int
multigetline(char **input, size_t *input_cap,
             FILE *in_stream, FILE *out_stream) {

    fprintf(out_stream, ">> ");

    int len = getline(input, input_cap, in_stream);
    if (len == -1) { return len; }

    struct pollfd fds;
    fds.fd = fileno(in_stream);
    fds.events = POLLIN;
    int ret = poll(&fds, 1, 0);

    // ret is 0 if there is no data to read.
    if (ret == 0) {
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
                putc('\n', out_stream);
            }
            fprintf(out_stream, ".. ");
        }

        int line_len = getline(&line, &line_cap, in_stream);
        if (line_len == -1) {
            free(line);
            return -1;
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
