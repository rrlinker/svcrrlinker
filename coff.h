#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstddef>
#include <experimental/filesystem>

#include "vendor/wine/windef.h"

namespace fs = std::experimental::filesystem;

class COFF {
    public:
        struct Section {
            IMAGE_SECTION_HEADER header;
            std::vector<std::byte> raw_data;
            std::vector<IMAGE_RELOCATION> relocations;
            uint64_t actual_address;
        };

        explicit COFF(fs::path path);

        IMAGE_FILE_HEADER const& file_header() const { return file_header_; }
        std::vector<Section>& sections() { return sections_; }
        std::vector<IMAGE_SYMBOL> const& symbols() const { return symbols_; }
        std::unordered_map<std::string, IMAGE_SYMBOL const*> const& export_symbols() const { return export_symbols_; }
        std::unordered_map<std::string, IMAGE_SYMBOL const*> const& import_symbols() const { return import_symbols_; }
        std::vector<char> const& string_table() const { return string_table_; }

        std::string get_symbol_name(IMAGE_SYMBOL const &symbol) const;
        bool symbol_is_exported(IMAGE_SYMBOL const &symbol) const;

        void resolve_external_symbol(std::string const &symbol, uint64_t address);
        void perform_relocations(Section &section, uint64_t address);


    private:
        void read_file_header();
        void read_sections();
        void read_symbols();
        void read_string_table();

        void read_section_raw_data(Section &section);
        void read_section_relocations(Section &section);

        fs::path path_;
        std::ifstream file_;

        IMAGE_FILE_HEADER file_header_;
        std::vector<Section> sections_;
        std::vector<IMAGE_SYMBOL> symbols_;
        std::vector<uint64_t> symbols_addresses_;
        std::unordered_map<std::string, size_t> symbol_map_;
        std::unordered_map<std::string, IMAGE_SYMBOL const*> export_symbols_;
        std::unordered_map<std::string, IMAGE_SYMBOL const*> import_symbols_;
        std::vector<char> string_table_;
};

