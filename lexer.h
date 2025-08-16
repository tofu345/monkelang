#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#include <stddef.h>

typedef struct {
    // TODO: maybe read from a FILE directly?
    const char* input;
    size_t input_len;

    size_t position;      // current position in input (points to current char)
    size_t read_position; // current reading position in input (after current char)
    char ch;
} Lexer;

// create new lexer object, the input is not copied so it must not be
// freed while in use.
Lexer lexer_new(const char* input, size_t len);

// Returns the next token parsed from input, this token contains a
// literal that is copied from the input and must be freed by the
// caller.
Token lexer_next_token(Lexer* l);

#endif
