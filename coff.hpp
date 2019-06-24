#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstddef>
#include <filesystem>

#include "vendor/wine/windef.h"

class COFF {
    public:
        struct Section {
            IMAGE_SECTION_HEADER header;
            std::vector<std::byte> raw_data;
            std::vector<IMAGE_RELOCATION> relocations;
            uint64_t reserved_address;

            bool discardable();
            DWORD protection();
            void perform_relocations(std::vector<uint64_t> const &symbols_addresses);

            static DWORD const section2page_protection[2][2][2];
        };

        explicit COFF(std::filesystem::path const &path);

        IMAGE_FILE_HEADER const& file_header() const { return file_header_; }
        std::vector<Section>& sections() { return sections_; }
        std::vector<IMAGE_SYMBOL> const& symbols() const { return symbols_; }
        std::unordered_map<std::string_view, IMAGE_SYMBOL&> const& export_symbols() const { return export_symbols_; }
        std::unordered_map<std::string_view, IMAGE_SYMBOL&> const& import_symbols() const { return import_symbols_; }
        std::vector<char> const& string_table() const { return string_table_; }

        std::string_view get_symbol_name(IMAGE_SYMBOL const &symbol) const;
        bool symbol_is_exported(IMAGE_SYMBOL const &symbol) const;
        bool symbol_is_imported(IMAGE_SYMBOL const &symbol) const;

        void resolve_external_symbol(std::string_view symbol, COFF const &exporter_coff);
        void resolve_external_symbol(std::string_view symbol, uint64_t address);
        void resolve_symbols_addresses();
        void perform_relocations();

        IMAGE_SYMBOL& get_symbol_by_name(std::string_view name);
        uint64_t& get_symbol_address_by_name(std::string_view name);
        uint64_t get_symbol_address_by_name(std::string_view name) const;

    private:
        void read_file_header();
        void read_sections();
        void read_string_table();
        void read_symbols();

        void read_section_raw_data(Section &section);
        void read_section_relocations(Section &section);

        std::ifstream file_;

        IMAGE_FILE_HEADER file_header_;
        std::vector<Section> sections_;
        std::vector<IMAGE_SYMBOL> symbols_;
        std::vector<uint64_t> symbols_addresses_;
        std::unordered_map<std::string_view, size_t> symbol_map_;
        std::unordered_map<std::string_view, IMAGE_SYMBOL&> export_symbols_;
        std::unordered_map<std::string_view, IMAGE_SYMBOL&> import_symbols_;
        std::vector<char> string_table_;
};

