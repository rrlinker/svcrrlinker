#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <experimental/filesystem>

#include <unistd.h>
#include <fcntl.h>

#include <courier.h>
#include "unix_connection.h"
#include "posix_connection.h"
#include "librarian.h"
#include "library.h"
#include "coff.h"

using namespace rrl;
namespace fs = std::experimental::filesystem;

int main(int argc, const char *argv[]) {
    std::cout << "PRESS ENTER TO CONTINUE ...\n";
    std::string tmp;
    std::cin >> tmp;

    if (argc != 4) {
        const char *program = argc > 0 ? argv[0] : "svclinker";
        std::cerr << "usage: " << program << " <file descriptor of socket> <symbol resolver unix server> <library path>\n"
            << "\tLinks (sends) specified library to the socket endpoint." << std::endl;
        return 1;
    }

    bool arguments_are_valid = true;
    int fd = atoi(argv[1]);
    auto resolver_path = fs::path(argv[2]);
    auto library_path = fs::path(argv[3]);

    if (!fd) {
        std::cerr << '`' << argv[1] << '`' << " is not a valid number of a file descriptor.\n";
        arguments_are_valid = false;
    }

    // Check if file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1) {
        std::cerr << '(' << fd << ')' << " is not a valid file descriptor.\n";
        arguments_are_valid = false;
    }

    if (!fs::is_socket(resolver_path)) {
        std::cerr << '`' << resolver_path << '`' << " is not a valid path to unix socket.\n";
        arguments_are_valid = false;
    }

    if (!fs::is_directory(library_path)) {
        std::cerr << '`' << library_path << '`' << " is not a valid path to a library directory.\n";
        arguments_are_valid = false;
    }

    if (!arguments_are_valid)
        return 2;

    std::cerr << "svclinker started" << std::endl;

    try {
        std::cerr << "creating connection" << std::endl;
        PosixConnection conn(fd);

        std::cerr << "creating resolver connection" << std::endl;
        UnixConnection resolver_conn;
        resolver_conn.connect(resolver_path);

        std::cerr << "creating courier" << std::endl;
        Courier courier(conn);
        Courier resolver_courier(resolver_conn);

        std::cerr << "sending OK..." << std::endl;
        msg::OK msg_ok;
        courier.send(msg_ok);

        std::cerr << "creating library" << std::endl;
        Library library(library_path);

        Librarian librarian(resolver_courier);

        std::cerr << "linking..." << std::endl;
        librarian.link(courier, library);
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}

