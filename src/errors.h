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

// print [err.message] if not NULL and [highlight_token(err.token)]
void print_error(Error *err);

// print line of [tok].
// highlight [tok.start] to [tok.start + tok.length].
void highlight_token(Token tok);

void free_error(Error *err);
