#include <iostream>

#include "ux_connection.h"
#include "unix_exception.h"

#include <unistd.h>

using namespace rrl;

UXConnection::UXConnection(int fd)
    : Connection(fd)
{}

void UXConnection::connect(Address const &address) {
}

void UXConnection::disconnect() {
    int err;

    if (close(socket_) != 0)
        throw UnixException(errno);

    socket_ = -1;
}

void UXConnection::send(const std::byte *data, uint64_t length) {
    int res;
    do {
        res = ::send(socket_, reinterpret_cast<const char*>(data), length, 0);
        if (res == -1)
            throw UnixException(errno);
        data += res;
        length -= res;
    } while (length > 0);
}

void UXConnection::recv(std::byte *data, uint64_t length) {
    int res;
    res = ::recv(socket_, reinterpret_cast<char*>(data), length, MSG_WAITALL);
    if (res == -1 || res == 0)
        throw UnixException(errno);
}
