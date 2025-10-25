#pragma once

#include "token.h"
#include "utils.h"

typedef char *error;

error new_error(const char* format, ...);
error error_num_args(const char *name, int expected, int actual);


// TODO
// typedef enum {
//     SyntaxError = 1,
// } ErrorType;

typedef struct {
    Token token; // the token the error occurred at.
    char *message;
} Error;

BUFFER(Error, Error)

// print line of [err.tok], highlight [err.tok] and print [err.message]
void print_error(const char *input, Error *err);
