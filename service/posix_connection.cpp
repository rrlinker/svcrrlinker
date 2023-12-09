#include <iostream>

#include <rrlinker/service/posix_connection.hpp>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <system_error>

using namespace rrl;

PosixConnection::PosixConnection(int fd)
    : Connection(fd)
{}

PosixConnection::~PosixConnection() {
    disconnect();
}

void PosixConnection::connect(Address const&) {
    throw std::logic_error("`connect` member function of PosixConnection class is not implemented");
}

void PosixConnection::disconnect() {
    if (socket_ != -1) {
        if (close(socket_) != 0)
            throw std::system_error(errno, std::generic_category());
        socket_ = -1;
    }
}

void PosixConnection::send(std::byte const *data, uint64_t length) {
    int res;
    do {
        res = ::send(socket_, reinterpret_cast<const char*>(data), length, 0);
        if (res == -1)
            throw std::system_error(errno, std::generic_category());
        data += res;
        length -= res;
    } while (length > 0);
}

void PosixConnection::recv(std::byte *data, uint64_t length) {
    while (length > 0) {
        int res = ::recv(socket_, reinterpret_cast<char*>(data), static_cast<int>(length), 0);
        if (res == -1 || res == 0)
            throw std::system_error(errno, std::generic_category());
        data += res;
        length -= res;
    }
}

