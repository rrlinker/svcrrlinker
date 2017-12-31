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
    read_sections_headers();
    read_symbols();
    read_string_table();
}

void COFF::read_file_header() {
    file_.seekg(0, file_.beg);
    binary_read(file_, file_header_);
}

void COFF::read_sections_headers() {
    file_.seekg(sizeof(IMAGE_FILE_HEADER), file_.beg);
    sections_.resize(file_header_.NumberOfSections);
    for (auto &section : sections_) {
        binary_read(file_, section);
    }
}
void COFF::read_symbols() {
    if (file_header_.NumberOfSymbols) {
        symbols_.resize(file_header_.NumberOfSymbols);
        file_.seekg(file_header_.PointerToSymbolTable, file_.beg);
        for (auto &symbol : symbols_) {
            binary_read(file_, symbol);
        }
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

std::unordered_map<std::string, uintptr_t> COFF::get_exports() const {
    std::unordered_map<std::string, uintptr_t> exs;
    for (int i = 0; i < symbols_.size(); i++) {
        IMAGE_SYMBOL const &symbol = symbols_[i];
        if (!symbol_is_exported(symbol))
            continue;

        auto name = get_symbol_name(symbol);
        exs.emplace(name, symbol.Value);

        // No Aux Symbols are supported yet
        if (symbol.NumberOfAuxSymbols) {
            std::cerr << "COFF(" << path_ << ") Ignoring " << symbol.NumberOfAuxSymbols << " AUX symbols @ " << i << '\n';
            i += symbol.NumberOfAuxSymbols;
        }
    }
    return exs;
}

std::vector<std::string> COFF::get_imports() const {
    std::vector<std::string> ims;
    for (int i = 0; i < symbols_.size(); i++) {
        IMAGE_SYMBOL const &symbol = symbols_[i];
        if (symbol_is_exported(symbol))
            continue;

        auto name = get_symbol_name(symbol);
        ims.emplace_back(name);

        // No Aux Symbols are supported yet
        if (symbol.NumberOfAuxSymbols) {
            std::cerr << "COFF(" << path_ << ") Ignoring " << symbol.NumberOfAuxSymbols << " AUX symbols @ " << i << '\n';
            i += symbol.NumberOfAuxSymbols;
        }
    }
    return ims;
}

std::vector<char> COFF::read_section_data(IMAGE_SECTION_HEADER const &section) {
    std::vector<char> data;
    data.resize(section.SizeOfRawData);
    file_.seekg(section.PointerToRawData, file_.beg);
    file_.read(data.data(), section.SizeOfRawData);
    return data;
}


void COFF::get_relocations() {
    for (auto const& section : sections_) {
        std::vector<IMAGE_RELOCATION> relocations(section.NumberOfRelocations);
        file_.seekg(section.PointerToRelocations, file_.beg);
        file_.read(reinterpret_cast<char*>(relocations.data()), section.NumberOfRelocations * sizeof(IMAGE_RELOCATION));
    }
}

void COFF::resolve_external_symbol(std::string const &symbol, uint64_t address) {
}

