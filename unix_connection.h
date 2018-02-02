#include <connection.h>

#include <experimental/filesystem>

#include <sys/socket.h>

namespace fs = std::experimental::filesystem;

class UnixConnection : public rrl::Connection {
public:
    virtual ~UnixConnection();

    void connect(fs::path const &path);
    virtual void connect(rrl::Address const &address) override;
    virtual void disconnect() override;
    virtual void send(std::byte const *data, size_t length) override;
    virtual void recv(std::byte *data, size_t length) override;
};

