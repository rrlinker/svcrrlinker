#pragma once

#include <vector>
#include <unordered_map>
#include <cstddef>
#include <experimental/filesystem>
#include <functional>

#include "coff.h"

namespace fs = std::experimental::filesystem;

class Library {
public:
    explicit Library(fs::path path);

    void reserve_memory_spaces(std::function<uint64_t(uint64_t)> const &applier);
    void resolve_symbols_addresses();
    void resolve_internal_symbols();
    void resolve_external_symbols(std::function<uint64_t(std::string_view const)> const &resolver);
    void perform_relocations();
    void export_symbols(std::function<void(std::string_view symbol_name, uint64_t address)> exporter);
    void commit_memory_spaces(std::function<void(uint64_t address, DWORD protection, std::vector<std::byte> const data)> performer);
    uint64_t get_entry_point();

private:
    void fill_export_symbol_map();

    fs::path path_;

    std::vector<COFF> coffs_;
    std::unordered_map<std::string_view, COFF&> export_symbol_map_;
    std::unordered_map<std::string_view, COFF&> unresolved_external_symbols_;

    static char const *ENTRY_POINT_NAME;
};

