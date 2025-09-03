#pragma once

#include "token.h"

#include <stddef.h>

typedef struct {
    // TODO: maybe read from a FILE directly?
    const char* input;
    size_t input_len;

    char ch;
    short col;
    int line;
    size_t position;      // current position in input (points to current char)
    size_t read_position; // current reading position in input (after current char)
} Lexer;

// create new lexer object, the input (NULL-terminated) is not copied.
Lexer lexer_new(const char* input);

// initialize lexer object, the input (NULL-terminated) is not copied.
void lexer_init(Lexer* l, const char* input);

// Returns the next token parsed from input, this token contains a
// literal that is copied from the input and must be freed by the
// caller.
Token lexer_next_token(Lexer* l);
