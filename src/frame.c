#include "frame.h"

void frame_init(Frame *f, CompiledFunction fn, int base_pointer) {
    *f = (Frame) {
        .fn = fn,
        .ip = -1,
        .base_pointer = base_pointer,
    };
}

Instructions instructions(Frame *f) {
    return f->fn.instructions;
}
