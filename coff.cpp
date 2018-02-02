#include <iostream>

#include "coff.h"

template<typename T>
std::istream& binary_read(std::istream &is, T &out) {
    return is.read(reinterpret_cast<char*>(&out), sizeof(T));
}

COFF::COFF(fs::path const &path)
{
    file_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file_.open(path);

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
    sections_.resize(file_header_.NumberOfSections);
    for (size_t i = 0; i < sections_.size(); i++) {
        auto &section = sections_[i];
        file_.seekg(IMAGE_SIZEOF_FILE_HEADER + i * IMAGE_SIZEOF_SECTION_HEADER, file_.beg);
        binary_read(file_, section.header);
        if (section.discardable()) {
            // Don't read section contents
            continue;
        }
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
        file_.read(reinterpret_cast<char*>(section.relocations.data()), section.relocations.size() * IMAGE_SIZEOF_RELOCATION);
    }
}

void COFF::read_string_table() {
    std::streamoff string_table_offset =
        file_header_.PointerToSymbolTable
        + file_header_.NumberOfSymbols * IMAGE_SIZEOF_SYMBOL;

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
        file_.read(reinterpret_cast<char*>(symbols_.data()), symbols_.size() * IMAGE_SIZEOF_SYMBOL);
    }
    for (size_t symid = 0; symid < symbols_.size(); symid++) {
        auto &symbol = symbols_[symid];
        auto name = get_symbol_name(symbol);
        auto pair = std::make_pair(name, symbol);
        if (symbol_is_exported(symbol))
            export_symbols_.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(symbol));
        else if (symbol_is_imported(symbol))
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
    return (symbol.StorageClass == IMAGE_SYM_CLASS_EXTERNAL && symbol.SectionNumber > 0)
        || (symbol.StorageClass == IMAGE_SYM_CLASS_STATIC && symbol.SectionNumber > 0 && symbol.Value != 0);
}

bool COFF::symbol_is_imported(IMAGE_SYMBOL const &symbol) const {
    return symbol.StorageClass == IMAGE_SYM_CLASS_EXTERNAL
        && symbol.SectionNumber == IMAGE_SYM_UNDEFINED;
}

IMAGE_SYMBOL& COFF::get_symbol_by_name(std::string_view name) {
    return symbols_[symbol_map_[name]];
}

uint64_t& COFF::get_symbol_address_by_name(std::string_view name) {
    return symbols_addresses_.at(symbol_map_.at(name));
}

uint64_t COFF::get_symbol_address_by_name(std::string_view name) const {
    return symbols_addresses_.at(symbol_map_.at(name));
}

void COFF::resolve_external_symbol(std::string_view symbol, COFF const &exporter_coff) {
    get_symbol_address_by_name(symbol) = exporter_coff.get_symbol_address_by_name(symbol);
}

void COFF::resolve_external_symbol(std::string_view symbol_name, uint64_t address) {
    get_symbol_address_by_name(symbol_name) = address;
}

void COFF::resolve_symbols_addresses() {
    for (size_t i = 0; i < symbols_.size(); i++) {
        auto &symbol = symbols_[i];
        auto &symbol_address = symbols_addresses_[i];
        // Import symbols are external, their addresses are resolved using resolve_external_symbol
        if (symbol_is_imported(symbol))
            continue;
        // SectionNumber is 1-based index
        symbol_address = sections_[symbol.SectionNumber - 1].reserved_address + symbol.Value;
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
                *reinterpret_cast<DWORD*>(raw_data.data() + relocation.VirtualAddress) = reserved_address;
                break;
            case IMAGE_REL_I386_DIR32:
                *reinterpret_cast<DWORD*>(raw_data.data() + relocation.VirtualAddress) += symbol_address;
                break;
            case IMAGE_REL_I386_REL32:
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

DWORD const COFF::Section::section2page_protection[2][2][2] = {
    // !IMAGE_SCN_MEM_EXECUTE
    {
        // !IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE
        {
            // !IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE && !IMAGE_SCN_MEM_READ
            PAGE_NOACCESS,
            // !IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE && IMAGE_SCN_MEM_READ
            PAGE_READONLY,
        },
        // !IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE
        {
            // !IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE && !IMAGE_SCN_MEM_READ
            PAGE_READWRITE,
            // !IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE && IMAGE_SCN_MEM_READ
            PAGE_READWRITE,
        },
    },
    // IMAGE_SCN_MEM_EXECUTE
    {
        // IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE
        {
            // IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE && !IMAGE_SCN_MEM_READ
            PAGE_EXECUTE,
            // IMAGE_SCN_MEM_EXECUTE && !IMAGE_SCN_MEM_WRITE && IMAGE_SCN_MEM_READ
            PAGE_EXECUTE_READ,
        },
        // IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE
        {
            // IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE && !IMAGE_SCN_MEM_READ
            PAGE_EXECUTE_READWRITE,
            // IMAGE_SCN_MEM_EXECUTE && IMAGE_SCN_MEM_WRITE && IMAGE_SCN_MEM_READ
            PAGE_EXECUTE_READWRITE,
        },
    },
};

bool COFF::Section::discardable() {
    return (header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        || (header.Characteristics & IMAGE_SCN_LNK_REMOVE);
}

DWORD COFF::Section::protection() {
    return section2page_protection
        [(header.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0]
        [(header.Characteristics & IMAGE_SCN_MEM_WRITE) != 0]
        [(header.Characteristics & IMAGE_SCN_MEM_READ) != 0];
}

