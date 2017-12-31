#include <connection.h>

#include <sys/socket.h>

class UXConnection : public rrl::Connection {
public:
    UXConnection(int fd);
    virtual ~UXConnection();

    virtual void connect(rrl::Address const &address) override;
    virtual void disconnect() override;
    virtual void send(std::byte const *data, size_t length) override;
    virtual void recv(std::byte *data, size_t length) override;
};

