#include "src/cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("Hello %s! This is the Monke Programming Language!\n",
                getlogin());
        repl(stdin, stdout);
        return EXIT_SUCCESS;

    } else if (argc > 2) {
        fprintf(stderr, "error: expect only optional path to program\n");
        return EXIT_FAILURE;
    }

    return run(argv[1]);
}
