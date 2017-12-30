#include "library.h"

library::library(fs::path path)
    : path_(std::move(path))
{
    for (auto &p : fs::directory_iterator(path_)) {
        if (fs::is_regular_file(p))
            coffs_.emplace_back(p);
    }
}

void library::read_coffs_exports() {
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

library::external_symbols library::get_unresolved_external_symbols() const {
    return coffs_[0].get_imports();
}

void library::resolve_external_symbols(library::external_symbol_map const &esm) {

}
