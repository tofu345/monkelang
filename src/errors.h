#pragma once

#include "token.h"
#include "utils.h"

typedef char *error;

error new_error(const char* format, ...);
error error_num_args(const char *name, int expected, int actual);

// TODO
// typedef enum {
//     SyntaxError,
//     CompilationError,
// } ErrorType;

typedef struct {
    Token token; // where the error occurred
    error message;
} Error;

BUFFER(Error, Error)

// print [err.message], print line of [err.token] and highlight [err.token]
void print_error(const char *input, Error *err);

void free_error(Error *err);
