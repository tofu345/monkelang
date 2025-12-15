#pragma once

#include "token.h"
#include "utils.h"

#include <stdarg.h>

struct Error {
    const char *message;

    // Optional pointer to where the error occurred in the source code
    Token *token;
};
typedef struct Error *error;
BUFFER(Error, error)

error errorf(const char* format, ...);
error verrorf(const char* format, va_list args);

error error_num_args(const char *name, int expected, int actual);
error error_num_args_(Token *tok, int expected, int actual);

// Reallocate error to store Token
void error_with(error *err, Token *tok);
void free_error(error err);

void print_error(error err, FILE *stream);

void print_token(Token *tok, int leftpad, FILE *stream);
void highlight_token(Token *tok, int leftpad, FILE *stream);
