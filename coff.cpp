#include <iostream>

#include "coff.h"

template<typename T>
std::istream& binary_read(std::istream &is, T &out) {
    return is.read(reinterpret_cast<char*>(&out), sizeof(T));
}

COFF::COFF(fs::path const &path)
{
    std::cerr << "file_ exceptions" << std::endl;
    file_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::cerr << "file_ open " << path << std::endl;
    file_.open(path);

    std::cerr << "read_file_header" << std::endl;
    read_file_header();
    std::cerr << "read_sections" << std::endl;
    read_sections();
    std::cerr << "read_string_table" << std::endl;
    read_string_table();
    std::cerr << "read_symbols" << std::endl;
    read_symbols();
}

void COFF::read_file_header() {
    file_.seekg(0, file_.beg);
    binary_read(file_, file_header_);
}

void COFF::read_sections() {
    file_.seekg(sizeof(IMAGE_FILE_HEADER), file_.beg);
    sections_.resize(file_header_.NumberOfSections);
    for (auto &section : sections_) {
        binary_read(file_, section.header);
        read_section_raw_data(section);
        read_section_relocations(section);
    }
}

void COFF::read_section_raw_data(COFF::Section &section) {
    section.raw_data.resize(section.header.SizeOfRawData);
    file_.seekg(section.header.PointerToRawData, file_.beg);
    file_.read(reinterpret_cast<char*>(section.raw_data.data()), section.raw_data.size());
}

void COFF::read_section_relocations(COFF::Section &section) {
    if (section.header.NumberOfRelocations) {
        section.relocations.resize(section.header.NumberOfRelocations);
        file_.seekg(section.header.PointerToRelocations, file_.beg);
        file_.read(reinterpret_cast<char*>(section.relocations.data()), section.relocations.size() * sizeof(IMAGE_RELOCATION));
    }
}

void COFF::read_string_table() {
    std::streamoff string_table_offset =
        file_header_.PointerToSymbolTable
        + file_header_.NumberOfSymbols * sizeof(IMAGE_SYMBOL);

    DWORD sizeof_table;

    file_.seekg(string_table_offset, file_.beg);
    binary_read(file_, sizeof_table);

    string_table_.resize(sizeof_table);

    file_.seekg(string_table_offset, file_.beg);
    file_.read(string_table_.data(), sizeof_table);
}

void COFF::read_symbols() {
    if (file_header_.NumberOfSymbols) {
        symbols_.resize(file_header_.NumberOfSymbols);
        file_.seekg(file_header_.PointerToSymbolTable, file_.beg);
        file_.read(reinterpret_cast<char*>(symbols_.data()), symbols_.size() * sizeof(IMAGE_SYMBOL));
    }
    for (size_t symid = 0; symid < symbols_.size(); symid++) {
        auto &symbol = symbols_[symid];
        auto name = get_symbol_name(symbol);
        auto pair = std::make_pair(name, symbol);
        if (symbol_is_exported(symbol))
            export_symbols_.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(symbol));
        else
            import_symbols_.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(symbol));
        symbol_map_.emplace(std::make_pair(name, symid));
    }
    symbols_addresses_.resize(symbols_.size());
}

std::string_view COFF::get_symbol_name(IMAGE_SYMBOL const &symbol) const {
    if (symbol.N.Name.Short) {
        char const *name = reinterpret_cast<char const*>(symbol.N.ShortName);
        size_t const len = strnlen(name, sizeof(IMAGE_SYMBOL::N));
        return std::string_view(name, len);
    } else {
        return &string_table_[symbol.N.Name.Long];
    }
}

bool COFF::symbol_is_exported(IMAGE_SYMBOL const &symbol) const {
    return symbol.SectionNumber > 0;
}

IMAGE_SYMBOL& COFF::get_symbol_by_name(std::string_view name) {
    return symbols_[symbol_map_[name]];
}

uint64_t& COFF::get_symbol_address_by_name(std::string_view name) {
    return symbols_addresses_[symbol_map_[name]];
}

void COFF::resolve_external_symbol(std::string_view symbol, COFF const &exporter_coff) {
    // TODO
    // get_symbol_address_by_name(symbol) = ...;
}

void COFF::resolve_external_symbol(std::string_view symbol_name, uint64_t address) {
    get_symbol_address_by_name(symbol_name) = address;
}

void COFF::resolve_export_symbols_addresses() {
    for (auto &[symbol_name, symbol] : export_symbols_) {
        // SectionNumber is 1-based index
        get_symbol_address_by_name(symbol_name) = sections_[symbol.SectionNumber - 1].reserved_address + symbol.Value;
    }
}

void COFF::perform_relocations() {
    for (auto &section : sections_) {
        section.perform_relocations(symbols_addresses_);
    }
}

void COFF::Section::perform_relocations(std::vector<uint64_t> const &symbols_addresses) {
    for (auto const &relocation : relocations) {
        auto const symbol_address = symbols_addresses[relocation.SymbolTableIndex];
        switch (relocation.Type) {
            case IMAGE_REL_I386_ABSOLUTE:
                std::cout << "ABSOLUTE" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(raw_data.data() + relocation.VirtualAddress) = reserved_address;
                break;
            case IMAGE_REL_I386_DIR32:
                std::cout << "DIR32" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(raw_data.data() + relocation.VirtualAddress) += symbol_address;
                break;
            case IMAGE_REL_I386_REL32:
                std::cout << "REL32" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(raw_data.data() + relocation.VirtualAddress) += symbol_address - (reserved_address + relocation.VirtualAddress + sizeof(DWORD));
                break;

            case IMAGE_REL_I386_DIR16:
            case IMAGE_REL_I386_REL16:
            case IMAGE_REL_I386_DIR32NB:
            case IMAGE_REL_I386_SEG12:
            case IMAGE_REL_I386_SECTION:
            case IMAGE_REL_I386_SECREL:
            case IMAGE_REL_I386_TOKEN:
            case IMAGE_REL_I386_SECREL7:
                throw std::logic_error("unsupported relocation type");
                break;
        }
    }
}
