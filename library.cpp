#include "library.h"

Library::Library(fs::path path)
    : path_(std::move(path))
{
    for (auto &p : fs::directory_iterator(path_)) {
        if (fs::is_regular_file(p))
            coffs_.emplace_back(COFF(p));
    }
}

void Library::resolve_internal_symbols() {
    // TODO
}

void Library::for_each_coff(std::function<void(COFF&)> callback) {
    for (auto &coff : coffs_)
        callback(coff);
}

