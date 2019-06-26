#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <memory>

#include <unistd.h>
#include <fcntl.h>

#include <librlcom/raw_courier.hpp>
#include <librlcrypto/crypto_courier.hpp>
#include "unix_connection.hpp"
#include "posix_connection.hpp"
#include "librarian.hpp"
#include "library.hpp"
#include "coff.hpp"

using namespace rrl;
using namespace rrl::rlc;

int main(int argc, const char *argv[]) {
    if (argc < 4 || argc > 5) {
        const char *program = argc > 0 ? argv[0] : "svclinker";
        std::cerr << "usage: " << program << " <file descriptor of socket> <symbol resolver unix server> <library path> [key for crypto courier]\n"
            << "\tLinks (sends) specified library to the socket endpoint." << std::endl;
        return 1;
    }

    bool arguments_are_valid = true;
    int fd = atoi(argv[1]);
    auto resolver_path = std::filesystem::path(argv[2]);
    auto library_path = std::filesystem::path(argv[3]);
    std::string key;
    if (argc >= 5) {
        key = argv[4];
    }

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
        resolver_conn.connect(Address{resolver_path});

        std::unique_ptr<Courier> courier;
        if (key.empty()) {
            courier = std::make_unique<RawCourier>(conn);
        } else {
            auto crypto_courier = std::make_unique<CryptoCourier>(conn);
            crypto_courier->init_with_key(bytes_from_hex_string(key));
            courier = std::move(crypto_courier);
        }
        RawCourier resolver_courier(resolver_conn);

        msg::OK msg_ok;
        courier->send(msg_ok);

        Library library(library_path);

        Librarian librarian(resolver_courier);

        librarian.link(*courier, library);
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}

