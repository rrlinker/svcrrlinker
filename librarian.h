#pragma once

#include <vector>
#include <string>

#include <courier.h>
#include "library.h"

class Librarian {
public:

    void link(rrl::Courier &courier, Library &library);

    void sanitize_symbols(std::vector<std::string> &symbols);
    void sanitize_symbol(std::string &symbol);

private:
    void handle_external_symbols(rrl::Courier &courier, Library &library);
    void handle_memory_spaces(rrl::Courier &courier, Library &library);
};

