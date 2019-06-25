#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

#include <unistd.h>
#include <fcntl.h>

#include <librlcom/raw_courier.hpp>
#include "unix_connection.hpp"
#include "posix_connection.hpp"
#include "librarian.hpp"
#include "library.hpp"
#include "coff.hpp"

using namespace rrl;

int main(int argc, const char *argv[]) {
    if (argc != 4) {
        const char *program = argc > 0 ? argv[0] : "svclinker";
        std::cerr << "usage: " << program << " <file descriptor of socket> <symbol resolver unix server> <library path>\n"
            << "\tLinks (sends) specified library to the socket endpoint." << std::endl;
        return 1;
    }

    bool arguments_are_valid = true;
    int fd = atoi(argv[1]);
    auto resolver_path = std::filesystem::path(argv[2]);
    auto library_path = std::filesystem::path(argv[3]);

    if (!fd) {
        std::cerr << '`' << argv[1] << '`' << " is not a valid number of a file descriptor.\n";
        arguments_are_valid = false;
    }

    // Check if file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1) {
        std::cerr << '(' << fd << ')' << " is not a valid file descriptor.\n";
        arguments_are_valid = false;
    }

    if (!std::filesystem::is_socket(resolver_path)) {
        std::cerr << '`' << resolver_path << '`' << " is not a valid path to unix socket.\n";
        arguments_are_valid = false;
    }

    if (!std::filesystem::is_directory(library_path)) {
        std::cerr << '`' << library_path << '`' << " is not a valid path to a library directory.\n";
        arguments_are_valid = false;
    }

    if (!arguments_are_valid)
        return 2;

    try {
        PosixConnection conn(fd);

        UnixConnection resolver_conn;
        resolver_conn.connect(resolver_path);

        RawCourier courier(conn);
        RawCourier resolver_courier(resolver_conn);

        msg::OK msg_ok;
        courier.send(msg_ok);

        Library library(library_path);

        Librarian librarian(resolver_courier);

        librarian.link(courier, library);
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}

