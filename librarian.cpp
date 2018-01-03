#include <iostream>
#include <regex>

#include "librarian.h"

using namespace rrl;

void Librarian::link(Courier &courier, Library &library) {
    handle_external_symbols(courier, library);
    handle_memory_spaces(courier, library);
    commit_memory_spaces(courier, library);

    library.for_each_coff([&courier](COFF &coff) {
        for (auto const &section : coff.sections()) {
            msg::Execute msg_execute;
            msg_execute.body().value = section.actual_address;
            courier.send(msg_execute);
        }
    });

    msg::OK msg_ok;
    courier.send(msg_ok);
}

void Librarian::handle_external_symbols(Courier &courier, Library &library) {
    library.resolve_internal_symbols();

    library.for_each_coff([&courier](COFF &coff) {
        auto external_symbols = coff.import_symbols();
        msg::ResolveExternalSymbols msg_resolve_symbols;
        for (auto const& [symbol, _] : external_symbols) {
            msg_resolve_symbols.body().emplace_back(std::make_pair("kernel32.dll", sanitize_symbol(symbol)));
        }
        courier.send(msg_resolve_symbols);

        auto resolved_symbols = courier.receive().cast<msg::ResolvedSymbols>();
        if (resolved_symbols.body().size() != external_symbols.size()) {
            throw std::logic_error("resolved_symbols.body().size() != external_symbols.size()");
        }
        auto resolved_symbol = resolved_symbols.body().begin();
        for (auto const& [symbol, _] : external_symbols) {
            coff.resolve_external_symbol(symbol, *resolved_symbol++);
        }
    });
}

void Librarian::handle_memory_spaces(Courier &courier, Library &library) {
    library.for_each_coff([&courier](COFF &coff) {
        msg::ReserveMemorySpaces msg_reserve_memory_spaces;
        for (auto &section : coff.sections()) {
            msg_reserve_memory_spaces.body().emplace_back(0, section.header.SizeOfRawData);
        }
        courier.send(msg_reserve_memory_spaces);

        auto reserved_memory = courier.receive().cast<msg::ReservedMemory>();
        auto actual_address = reserved_memory.body().begin();
        for (auto &section : coff.sections()) {
            coff.perform_relocations(section, *actual_address++);
        }
        if (actual_address != reserved_memory.body().end()) {
            throw std::logic_error("actual_address != reserved_memory.body().end()");
        }
    });
}

void Librarian::commit_memory_spaces(rrl::Courier &courier, Library &library) {
    library.for_each_coff([&courier](COFF &coff) {
        for (auto const &section : coff.sections()) {
            msg::CommitMemory msg_commit_memory;
            msg_commit_memory.body().address = section.actual_address;
            msg_commit_memory.body().memory = section.raw_data;
            msg_commit_memory.body().protection = 0x40;
            courier.send(msg_commit_memory);
        }
    });
}

std::string Librarian::sanitize_symbol(std::string symbol) {
    std::regex sanitizer("^(__imp__|_)?(\\w+)(@\\d+)?$");
    std::smatch match;
    if (std::regex_search(symbol, match, sanitizer))
        return match[2];
    return symbol;
}

