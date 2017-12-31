#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <experimental/filesystem>

#include <unistd.h>
#include <fcntl.h>

#include <courier.h>
#include "ux_connection.h"
#include "librarian.h"
#include "library.h"
#include "coff.h"

using namespace rrl;
namespace fs = std::experimental::filesystem;

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        const char *program = argc > 0 ? argv[0] : "svclinker";
        std::cerr << "usage: " << program << " <file descriptor of socket> <library path>\n"
            << "\tLinks (sends) specified library to the socket endpoint." << std::endl;
        return 1;
    }

    bool arguments_are_valid = true;
    int fd = atoi(argv[1]);
    auto library_path = fs::path(argv[2]);

    if (!fd) {
        std::cerr << '`' << argv[1] << '`' << " is not a valid number of a file descriptor.\n";
        arguments_are_valid = false;
    }

    // Check if file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1) {
        std::cerr << '(' << fd << ')' << " is not a valid file descriptor.\n";
        arguments_are_valid = false;
    }

    if (!fs::is_directory(library_path)) {
        std::cerr << '`' << library_path << '`' << " is not a valid path to a library directory.\n";
        arguments_are_valid = false;
    }

    if (!arguments_are_valid)
        return 2;

    auto number_of_files = std::count_if(
        fs::directory_iterator(library_path),
        fs::directory_iterator(),
        static_cast<bool(*)(fs::path const&)>(fs::is_regular_file)
    );

    if (number_of_files > 1) {
        std::cerr << "Multi-file libraries are not supported yet.\n";
        return -1;
    }

    try {
        UXConnection conn(fd);
        Courier courier(conn);

        msg::OK msg_ok;
        courier.send(msg_ok);

        Library library(library_path);
        Librarian librarian;
        librarian.link(courier, library);
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}

