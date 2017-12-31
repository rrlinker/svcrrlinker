#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <experimental/filesystem>

#include "vendor/wine/windef.h"

namespace fs = std::experimental::filesystem;

class COFF {
    public:
        explicit COFF(fs::path path);

        IMAGE_FILE_HEADER const& file_header() const { return file_header_; }
        std::vector<IMAGE_SECTION_HEADER> const& sections_headers() const { return sections_; }
        std::vector<IMAGE_SYMBOL> const& symbols() const { return symbols_; }
        std::vector<char> const& string_table() const { return string_table_; }

        std::unordered_map<std::string, uintptr_t> get_exports() const;
        std::vector<std::string> get_imports() const;

        void get_relocations();

        std::vector<char> read_section_data(IMAGE_SECTION_HEADER const &section);

        void resolve_external_symbol(std::string const &symbol, uint64_t address);

    private:
        void read_file_header();
        void read_sections_headers();
        void read_symbols();
        void read_string_table();

        bool symbol_is_exported(IMAGE_SYMBOL const &symbol) const;
        std::string get_symbol_name(IMAGE_SYMBOL const &symbol) const;

        fs::path path_;
        std::ifstream file_;

        IMAGE_FILE_HEADER file_header_;
        std::vector<IMAGE_SECTION_HEADER> sections_;
        std::vector<IMAGE_SYMBOL> symbols_;
        std::vector<char> string_table_;
};

