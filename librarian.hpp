#pragma once

#include <vector>
#include <string>
#include <cstddef>

#include <librlcom/courier.hpp>
#include "library.hpp"

class Librarian {
public:
    Librarian(rrl::Courier &resolver_courier);

    void link(rrl::Courier &courier, Library &library);

    std::string get_symbol_library(std::string_view symbol_name);

    static std::string sanitize_symbol(std::string const &symbol_name);

private:
    void reserve_memory_spaces(rrl::Courier &courier, Library &library);
    void resolve_symbols_addresses(Library &library);
    void resolve_internal_symbols(Library &library);
    void resolve_external_symbols(rrl::Courier &courier, Library &library);
    void perform_relocations(Library &library);
    void commit_memory_spaces(rrl::Courier &courier, Library &library);
    void export_symbols(rrl::Courier &courier, Library &library);
    void execute_entry_point(rrl::Courier &courier, Library &library);

    rrl::Courier &resolver_courier;
};

