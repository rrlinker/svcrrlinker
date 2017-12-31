#pragma once

#include <stdexcept>

class UnixException : public std::runtime_error
{
public:
    UnixException(int errnum);

    int error() const { return errnum_; }

private:
    int errnum_;
};

