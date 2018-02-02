#include <iostream>

#include "posix_connection.h"
#include "unix_exception.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

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
            throw UnixException(errno);
        socket_ = -1;
    }
}

void PosixConnection::send(std::byte const *data, uint64_t length) {
    int res;
    do {
        res = ::send(socket_, reinterpret_cast<const char*>(data), length, 0);
        if (res == -1)
            throw UnixException(errno);
        data += res;
        length -= res;
    } while (length > 0);
}

void PosixConnection::recv(std::byte *data, uint64_t length) {
    int res;
    res = ::recv(socket_, reinterpret_cast<char*>(data), length, MSG_WAITALL);
    if (res == -1 || res == 0)
        throw UnixException(errno);
}

