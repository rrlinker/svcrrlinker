#include "unix_exception.hpp"

#include <errno.h>
#include <string.h>

UnixException::UnixException(int ec)
    : std::system_error(ec, std::generic_category())
{}
