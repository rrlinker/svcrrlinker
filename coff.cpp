#include <iostream>

#include "coff.h"

template<typename T>
std::istream& binary_read(std::istream &is, T &out) {
    return is.read((char*)&out, sizeof(T));
}

COFF::COFF(fs::path path) :
    path_(std::move(path))
{
    file_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file_.open(path_);

    read_file_header();
    read_sections();
    read_string_table();
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
    section.relocations.resize(section.header.NumberOfRelocations);
    file_.seekg(section.header.PointerToRelocations, file_.beg);
    file_.read(reinterpret_cast<char*>(section.relocations.data()), section.relocations.size() * sizeof(IMAGE_RELOCATION));
}

void COFF::read_symbols() {
    if (file_header_.NumberOfSymbols) {
        symbols_.resize(file_header_.NumberOfSymbols);
        file_.seekg(file_header_.PointerToSymbolTable, file_.beg);
        file_.read(reinterpret_cast<char*>(symbols_.data()), symbols_.size() * sizeof(IMAGE_SYMBOL));
    }
    for (auto const &symbol : symbols_) {
        auto const &name = get_symbol_name(symbol);
        auto pair = std::make_pair(name, &symbol);
        if (symbol_is_exported(symbol))
            export_symbols_.insert(pair);
        else
            import_symbols_.insert(pair);
        symbol_map_.emplace(std::make_pair(name, symbol_map_.size()));
    }
    symbols_addresses_.resize(symbols_.size());
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

std::string COFF::get_symbol_name(IMAGE_SYMBOL const &symbol) const {
    if (symbol.N.Name.Short) {
        const char *name = (const char*)symbol.N.ShortName;
        size_t len = strnlen(name, sizeof(IMAGE_SYMBOL::N));
        return std::string(name, len);
    } else {
        return &string_table_[symbol.N.Name.Long];
    }
}

bool COFF::symbol_is_exported(IMAGE_SYMBOL const &symbol) const {
    return symbol.SectionNumber > 0;
}

void COFF::resolve_external_symbol(std::string const &symbol_name, uint64_t address) {
    auto symbol_id = symbol_map_[symbol_name];
    symbols_addresses_[symbol_id] = address;
}

void COFF::perform_relocations(COFF::Section &section, uint64_t address) {
    section.actual_address = address;
    for (auto const &relocation : section.relocations) {
        auto const symbol_address = symbols_addresses_[relocation.SymbolTableIndex];
        switch (relocation.Type) {
            case IMAGE_REL_I386_ABSOLUTE:
                std::cout << "ABSOLUTE" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(section.raw_data.data() + relocation.VirtualAddress) = address;
                break;
            case IMAGE_REL_I386_DIR32:
                std::cout << "DIR32" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(section.raw_data.data() + relocation.VirtualAddress) = symbol_address;
                break;
            case IMAGE_REL_I386_REL32:
                std::cout << "REL32" << relocation.VirtualAddress << std::endl;
                *reinterpret_cast<DWORD*>(section.raw_data.data() + relocation.VirtualAddress) += symbol_address - address - 10;
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

