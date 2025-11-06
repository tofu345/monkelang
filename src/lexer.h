#pragma once

// The Lexer, converts the source code into Tokens.

#include "token.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *input;
    uint64_t input_len;

    char ch; // current char
    uint64_t position;      // index of current char
    uint64_t read_position; // after current char

    // 1-based line number of current char
    int line;
} Lexer;

// Initialise Lexer and call [lexer_with(input)] if input not NULL.
void lexer_init(Lexer *l, const char *input);

// Change source code under inspection to input.
// Reset [l.position] and [l.read_position], [l.line] is not modified.
void lexer_with(Lexer *l, const char *input, uint64_t length);

Token lexer_next_token(Lexer *l);
