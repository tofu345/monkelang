#pragma once

#include "code.h"
#include "object.h"

typedef struct {
    Closure *cl;

    int ip; // instruction pointer

    // points to bottom of stack for current function
    // (normally the first argument).
    int base_pointer;

    // tracks arguments since they do not stay on the stack while the
    // function is run.
    ObjectBuffer args;
} Frame;

void frame_init(Frame *f, Closure *cl, int base_pointer);
Instructions instructions(Frame *f);
