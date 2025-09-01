#include "repl.h"

#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("Hello %s! This is the Monke Programming Language!\n",
            getlogin());
    start(stdin, stdout);
    return EXIT_SUCCESS;
}
