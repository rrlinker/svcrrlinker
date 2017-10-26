#include <iostream>
#include <vector>
#include <string>

#include "coff.h"

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        // usage
    }

    int fd = atoi(argv[1]);
    if (!fd) {
        // no socket fd
    }
    return 0;
}

