#pragma once

#include <vector>
#include <unordered_map>
#include <cstddef>
#include <experimental/filesystem>
#include <functional>

#include "coff.h"

namespace fs = std::experimental::filesystem;

class Library {
public:
    explicit Library(fs::path path);

    void resolve_internal_symbols();

    void for_each_coff(std::function<void(COFF&)> callback);

private:
    fs::path path_;

    std::vector<COFF> coffs_;
};

