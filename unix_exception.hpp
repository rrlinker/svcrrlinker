#pragma once

#include <stdexcept>
#include <system_error>
#include <cstring>

class UnixException : public std::system_error
{
public:
    UnixException(int ec);
    virtual const char* what() const noexcept override { return strerror(code().value()); }
};
