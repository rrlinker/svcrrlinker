#include <librlcom/connection.hpp>

#include <filesystem>

#include <sys/socket.h>

class PosixConnection : public rrl::Connection {
public:
    PosixConnection(int fd);
    virtual ~PosixConnection();

    virtual void connect(rrl::Address const &address) override;
    virtual void disconnect() override;
    virtual void send(std::byte const *data, size_t length) override;
    virtual void recv(std::byte *data, size_t length) override;
};

