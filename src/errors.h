#pragma once

#include "token.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

static const char parser_error_msg[] = "Woops! We ran into some monkey business here!\n";
static const char compiler_error_msg[] = "Woops! Compilation failed!\n";
static const char vm_error_msg[] = "Woops! Executing bytecode failed!\n";

struct Error {
    const char *message;

    // Optional pointer to where the error occurred in the source code
    Token *token;
};
typedef struct Error *error;

error create_error(const char *message);
error errorf(const char* format, ...);
error verrorf(const char* format, va_list args);

error error_num_args(const char *name, int expected, int actual);
error error_num_args_(Token *tok, int expected, int actual);

void free_error(error err);

void print_error(error err, FILE *stream);

void print_token(Token *tok, int leftpad, FILE *stream);
void highlight_token(Token *tok, int leftpad, FILE *stream);
