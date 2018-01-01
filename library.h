#pragma once

#include <vector>
#include <unordered_map>
#include <experimental/filesystem>

#include "coff.h"

namespace fs = std::experimental::filesystem;

class Library {
public:
    explicit Library(fs::path path);

    void read_coffs_exports();

    std::vector<std::string> get_unresolved_external_symbols() const;
    void resolve_external_symbol(std::string const &symbol, uint64_t address);

    std::unordered_map<std::string, std::pair<COFF&, uint64_t>> const& get_coffs_exports() const { return coffs_exports_; }
private:
    fs::path path_;

    std::vector<COFF> coffs_;
    std::unordered_map<std::string, std::pair<COFF&, uint64_t>> coffs_exports_;
};

