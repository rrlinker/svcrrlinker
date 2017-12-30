#include "unix_exception.h"

#include <errno.h>
#include <string.h>

UnixException::UnixException(int errnum)
    : std::runtime_error(strerror(errnum))
    , errnum_(errnum)
{}
