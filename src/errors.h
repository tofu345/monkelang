#pragma once

#include "token.h"
#include "utils.h"

// A simple error message.
typedef char *error;

error new_error(const char* format, ...);
error error_num_args(const char *name, int expected, int actual);

// typedef enum {
//     SyntaxError,
//     CompilationError,
// } ErrorType;

// An error message with a Token which points to where in the source code the
// error occurred.
typedef struct {
    Token token; // where the error occurred
    error message;
} Error;

BUFFER(Error, Error)
void free_error(Error *err);

// Utilities for displaying Errors.

const char *start_of_line(Token *tok, int *distance);
const char *end_of_line(Token *tok, int *distance);

// print [err.message] if not NULL,
// and [highlight_token_with_line_number(err.token)].
void print_error(Error *err);

void print_token(Token *tok, int leftpad);
void print_token_line_number(Token *tok);

void highlight_token(Token *tok, int leftpad);
void highlight_token_with_line_number(Token *tok);

void highlight_line_with_line_number(Token *tok);
