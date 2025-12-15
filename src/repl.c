#include "repl.h"
#include "compiler.h"
#include "constants.h"
#include "errors.h"
#include "object.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

static const char *parser_error = "Woops! We ran into some monkey business here!";
static const char *compiler_error = "Woops! Compilation failed!";
static const char *vm_error = "Woops! Executing bytecode failed!";

// Successfully compiled and run input.
typedef struct {
    char *input;
    Program program;
} Eval;

BUFFER(Done, Eval)
DEFINE_BUFFER(Done, Eval)

// getline() but if there is no more data to read or the first line ends with
// '{' or '('.  Then read and append multiple lines to [input] until a blank
// line is encountered and there is no data left in [in_stream].
static int multigetline(char **input, size_t *input_cap,
                        FILE *in_stream, FILE *out_stream);

void repl(FILE* in, FILE* out) {
    Parser p;
    parser_init(&p);

    Compiler c;
    compiler_init(&c);

    VM vm;
    vm_init(&vm, NULL, NULL, NULL);

    DoneBuffer done = {0};

    error err = NULL;
    char *input = NULL;
    int len;
    size_t cap = 0;
    Program prog;
    while (1) {
        len = multigetline(&input, &cap, in, out);
        if (len == -1) {
            free(input);
            break;

        // only '\n' was entered.
        } else if (len == 1) {
            continue;
        }

        prog = parse_(&p, input, len);
        if (p.errors.length > 0) {
            fputs(parser_error, out);
            print_parser_errors(&p);
            goto cleanup;
        } else if (prog.stmts.length == 0) {
            goto cleanup;
        }

        err = compile(&c, &prog);
        if (err) {
            fputs(compiler_error, out);
            print_error(err, out);
            goto cleanup;
        }

        err = vm_run(&vm, bytecode(&c));
        if (err) {
            fputs(vm_error, out);
            print_vm_stack_trace(&vm, out);
            print_error(err, out);
            goto cleanup;
        }

        Object stack_elem = vm_last_popped(&vm);
        if (stack_elem.type != o_Null) {
            object_fprint(stack_elem, out);
            fputc('\n', out);
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
        free_error(err);
        err = NULL;

        input = NULL;
        prog = (Program){0};

        if (c.cur_scope_index > 0) {
            for (; c.cur_scope_index > 0; --c.cur_scope_index) {
                // free unsuccessfully [CompiledFunctions]
                free_function(c.scopes.data[c.cur_scope_index].function);
            }
            c.cur_scope = &c.scopes.data[0];
            c.scopes.length = 1;

            CompiledFunction *main_fn = c.cur_scope->function;
            c.current_instructions = &main_fn->instructions;
            c.cur_mappings = &main_fn->mappings;
        } else {
            c.current_instructions->length = 0;
            c.cur_mappings->length = 0;
        }

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
    error err = NULL;

    parser_init(&p);
    compiler_init(&c);
    vm_init(&vm, NULL, NULL, NULL);

    prog = parse(&p, program);
    if (p.errors.length > 0) {
        fputs(parser_error, stdout);
        print_parser_errors(&p);
        goto cleanup;
    } else if (prog.stmts.length == 0) {
        goto cleanup;
    }

    err = compile(&c, &prog);
    if (err) {
        fputs(compiler_error, stdout);
        print_error(err, stdout);
        goto cleanup;
    }

    err = vm_run(&vm, bytecode(&c));
    if (err) {
        fputs(vm_error, stdout);
        print_vm_stack_trace(&vm, stdout);
        print_error(err, stdout);
        goto cleanup;
    }

cleanup:
    free_error(err);
    vm_free(&vm);
    compiler_free(&c);
    program_free(&prog);
    parser_free(&p);
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
