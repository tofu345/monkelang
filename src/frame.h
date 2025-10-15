#pragma once

#include "code.h"
#include "object.h"

typedef struct {
    CompiledFunction fn;
    int ip;
    int base_pointer;
} Frame;

void frame_init(Frame *f, CompiledFunction fn, int base_pointer);
Instructions instructions(Frame *f);
