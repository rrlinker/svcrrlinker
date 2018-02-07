#include <iostream>
#include <regex>

#include "librarian.h"

using namespace rrl;

Librarian::Librarian(rrl::Courier &resolver_courier) 
    : resolver_courier(resolver_courier)
{}

void Librarian::link(Courier &courier, Library &library) {
    reserve_memory_spaces(courier, library);
    resolve_symbols_addresses(library);
    resolve_internal_symbols(library);
    resolve_external_symbols(courier, library);
    perform_relocations(library);
    commit_memory_spaces(courier, library);
    export_symbols(courier, library);
    execute_entry_point(courier, library);

    msg::OK msg_ok;
    courier.send(msg_ok);
}

void Librarian::resolve_symbols_addresses(Library &library) {
    library.resolve_symbols_addresses();
}

void Librarian::resolve_internal_symbols(Library &library) {
    library.resolve_internal_symbols();
}

void Librarian::resolve_external_symbols(Courier &courier, Library &library) {
    library.resolve_external_symbols([this, &courier](std::string_view symbol_name) -> uint64_t {
        auto library_name = get_symbol_library(symbol_name);
        msg::ResolveExternalSymbol msg_resolve_symbol;
        msg_resolve_symbol.body().library = library_name;
        msg_resolve_symbol.body().symbol = sanitize_symbol(std::string(symbol_name));
        courier.send(msg_resolve_symbol);
        auto resolved_symbol = courier.receive().cast<msg::ResolvedSymbol>();
        return resolved_symbol.body().value;
    });
}

void Librarian::reserve_memory_spaces(Courier &courier, Library &library) {
    library.reserve_memory_spaces([&courier](uint64_t size) -> uint64_t {
        msg::ReserveMemorySpace msg_reserve_memory_space;
        msg_reserve_memory_space.body().first = 0;
        msg_reserve_memory_space.body().second = size;
        courier.send(msg_reserve_memory_space);
        auto reserved_memory = courier.receive().cast<msg::ReservedMemory>();
        return reserved_memory.body().value;
    });
}

void Librarian::perform_relocations(Library &library) {
    library.perform_relocations();
}

void Librarian::commit_memory_spaces(rrl::Courier &courier, Library &library) {
    library.commit_memory_spaces([&courier](uint64_t address, DWORD protection, std::vector<std::byte> const data) {
        msg::CommitMemory msg_commit_memory;
        msg_commit_memory.body().address = address;
        msg_commit_memory.body().protection = protection;
        msg_commit_memory.body().memory = data;
        courier.send(msg_commit_memory);
        courier.receive().cast<msg::OK>();
    });
}

void Librarian::export_symbols(rrl::Courier &courier, Library &library) {
    library.export_symbols([&courier](std::string_view symbol_name, uint64_t address) {
        msg::ExportSymbol msg_export_symbol;
        msg_export_symbol.body().symbol = sanitize_symbol(std::string(symbol_name));
        msg_export_symbol.body().address = address;
        courier.send(msg_export_symbol);
    });
}

void Librarian::execute_entry_point(rrl::Courier &courier, Library &library) {
    auto entry_point = library.get_entry_point();
    if (entry_point) {
        msg::Execute msg_execute;
        msg_execute.body().value = entry_point;
        courier.send(msg_execute);
    }
}

std::string Librarian::get_symbol_library(std::string_view symbol_name) {
    msg::GetSymbolLibrary msg_get_symbol_library;
    msg_get_symbol_library.body() = symbol_name;
    resolver_courier.send(msg_get_symbol_library);
    return resolver_courier.receive().cast<msg::ResolvedSymbolLibrary>().body() + ".dll";
}

std::string Librarian::sanitize_symbol(std::string const &symbol_name) {
    std::regex sanitizer("^(__imp__|_)?(\\w+)(@\\d+)?$");
    std::smatch match;
    if (std::regex_search(symbol_name, match, sanitizer))
        return match[2];
    return symbol_name;
}

