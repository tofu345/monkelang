#pragma once

#include "token.h"

#include <stddef.h>

// TODO: read directly from a FILE.
// TODO: attach length to Token.

typedef struct {
    const char* input;
    size_t input_len;

    char ch;
    short col;
    int line;
    size_t position;      // current position in input (points to current char)
    size_t read_position; // current reading position in input (after current char)
} Lexer;

// initiliaze lexer object with input (NULL-terminated).
void lexer_init(Lexer *l, const char* input);

// Returns the next token parsed from input, this token contains a
// literal that is copied from the input and must be freed by the
// caller.
Token lexer_next_token(Lexer* l);
