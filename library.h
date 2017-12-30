#pragma once

#include <vector>
#include <unordered_map>
#include <experimental/string_view>
#include <experimental/filesystem>

#include "coff.h"

namespace fs = std::experimental::filesystem;

class library {
public:
    using external_symbols = std::vector<std::experimental::string_view>;
    using external_symbol_map = std::unordered_map<std::string, uintptr_t>;

    explicit library(fs::path path);

    void read_coffs_exports();

    external_symbols get_unresolved_external_symbols() const;
    void resolve_external_symbols(external_symbol_map const &esm);

    std::unordered_map<std::string, std::pair<coff&, uintptr_t>> const& get_coffs_exports() const { return coffs_exports_; }
private:
    fs::path path_;

    std::vector<coff> coffs_;
    std::unordered_map<std::string, std::pair<coff&, uintptr_t>> coffs_exports_;
};
