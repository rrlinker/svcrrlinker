#include <iostream>
#include <regex>

#include "librarian.h"

using namespace rrl;

void Librarian::link(Courier &courier, Library &library) {
    handle_external_symbols(courier, library);
    handle_memory_spaces(courier, library);

    msg::OK msg_ok;
    courier.send(msg_ok);
}

void Librarian::handle_external_symbols(Courier &courier, Library &library) {
    // TODO: library.resolve_internal_symbols();

    auto external_symbols = library.get_unresolved_external_symbols();
    sanitize_symbols(external_symbols);
    msg::ResolveExternalSymbols msg_resolve_symbols;
    for (auto &symbol : external_symbols) {
        msg_resolve_symbols.body().emplace_back(std::make_pair("kernel32.dll", symbol));
    }
    courier.send(msg_resolve_symbols);

    auto resolved_symbols = courier.receive().cast<msg::ResolvedSymbols>();
    if (resolved_symbols.body().size() != external_symbols.size()) {
        throw std::logic_error("resolved_symbols.body().size() != external_symbols.size()");
    }
    for (int i = 0; i < external_symbols.size(); i++) {
        auto const &symbol = external_symbols[i];
        auto address = resolved_symbols[i];
        library.resolve_external_symbol(symbol, address);
    }

}

void Librarian::handle_memory_spaces(Courier &courier, Library &library) {
    auto memory_spaces = library.get_memory_spaces();
    msg::ReserveMemorySpaces msg_reserve_memory_spaces;
    msg_reserve_memory_spaces.body() = std::move(memory_spaces);
    courier.send(msg_reserve_memory_spaces);

    auto reserved_memory = courier.receive().cast<msg::ReservedMemory>();
    if (reserved_memory.body().size() != memory_spaces.size()) {
        throw std::logic_error("reserved_memory.body().size() != memory_spaces.size()");
    }
    for (int i = 0; i < memory_spaces.size(); i++) {
        auto original_address = memory_spaces[i].first;
        // auto size = memory_spaces[i].second;
        auto reserved_address = reserved_memory[i];
        library.assign_memory_space(original_address, reserved_address);
    }
}

void Librarian::sanitize_symbols(std::vector<std::string> &symbols) {
    for (auto &symbol : symbols)
        sanitize_symbol(symbol);
}

void Librarian::sanitize_symbol(std::string &symbol) {
    std::regex sanitizer("^(__imp__|_)?(\\w+)(@\\d+)?$");
    std::smatch match;
    if (std::regex_search(symbol, match, sanitizer))
        symbol = match[2];
}

