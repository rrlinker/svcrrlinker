#include <iostream>

#include "unix_connection.hpp"
#include "unix_exception.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace rrl;

UnixConnection::~UnixConnection() {
    disconnect();
}

void UnixConnection::connect(rrl::Address const& address) {
    auto const &path = address.path();

    if (path.string().length() > sizeof(sockaddr_un::sun_path)) {
        throw std::logic_error("unix socket path is too long");
    }

    socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_ < 0) {
        throw UnixException(errno);
    }

    sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));

    if (::connect(socket_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        throw UnixException(errno);
    }
}

void UnixConnection::disconnect() {
    if (socket_ != -1) {
        if (close(socket_) != 0)
            throw UnixException(errno);
        socket_ = -1;
    }
}

void UnixConnection::send(std::byte const *data, uint64_t length) {
    int res;
    do {
        res = ::send(socket_, reinterpret_cast<const char*>(data), length, 0);
        if (res == -1)
            throw UnixException(errno);
        data += res;
        length -= res;
    } while (length > 0);
}

void UnixConnection::recv(std::byte *data, uint64_t length) {
    int res;
    res = ::recv(socket_, reinterpret_cast<char*>(data), length, MSG_WAITALL);
    if (res == -1 || res == 0)
        throw UnixException(errno);
}

