#include "library.h"

Library::Library(fs::path path)
    : path_(std::move(path))
{
    for (auto &p : fs::directory_iterator(path_)) {
        if (fs::is_regular_file(p))
            coffs_.emplace_back(COFF(p));
    }
}

void Library::read_coffs_exports() {
    for (auto &c : coffs_) {
        for (auto const& [name, value] : c.get_exports()) {
            coffs_exports_.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(name),
                std::forward_as_tuple(c, value)
            );
        }
    }
}

std::vector<std::string> Library::get_unresolved_external_symbols() const {
    return coffs_[0].get_imports();
}

void Library::resolve_external_symbol(std::string const &symbol, uint64_t address) {
    coffs_[0].resolve_external_symbol(symbol, address);
}

