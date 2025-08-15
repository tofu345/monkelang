#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#include <stddef.h>

typedef struct Lexer Lexer;

// create new lexer object, the input is not copied so it must not be
// freed while in use.
Lexer* lexer_new(char* input, size_t len);

void lexer_destroy(Lexer* l);

Token lexer_next_token(Lexer* l);

#endif
