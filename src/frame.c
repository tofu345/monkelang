#include "frame.h"

void frame_init(Frame *f, Closure *cl, int base_pointer) {
    *f = (Frame) {
        .cl = cl,
        .ip = -1,
        .base_pointer = base_pointer,
    };
}

Instructions instructions(Frame *f) {
    return f->cl->func->instructions;
}
