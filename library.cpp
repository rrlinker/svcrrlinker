#include "library.hpp"

char const *Library::ENTRY_POINT_NAME = "_rrl_main@4";

Library::Library(std::filesystem::path path)
    : path_(std::move(path))
{
    for (auto &p : fs::directory_iterator(path_)) {
        if (fs::is_regular_file(p))
            coffs_.emplace_back(COFF(p));
    }
}

void Library::reserve_memory_spaces(std::function<uint64_t(uint64_t const)> const &applier) {
    for (auto &coff : coffs_) {
        for (auto &section : coff.sections()) {
            if (section.header.SizeOfRawData && !section.discardable()) {
                section.reserved_address = applier(section.header.SizeOfRawData);
            }
        }
    }
}

void Library::resolve_symbols_addresses() {
    for (auto &coff : coffs_) {
        coff.resolve_symbols_addresses();
    }
}

void Library::resolve_internal_symbols() {
    fill_export_symbol_map();
    unresolved_external_symbols_.clear();
    for (auto &import_coff : coffs_) {
        for (auto const& [import_name, import_symbol] : import_coff.import_symbols()) {
            auto exported = export_symbol_map_.find(import_name);
            if (exported != export_symbol_map_.end()) {
                // Possible to resolve within library
                auto &exporter_coff = exported->second;
                import_coff.resolve_external_symbol(import_name, exporter_coff);
            } else {
                // Unresolved external symbol (for ex. Win32API)
                unresolved_external_symbols_.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(import_name),
                        std::forward_as_tuple(import_coff)
                        );
            }
        }
    }
}

void Library::fill_export_symbol_map() {
    if (export_symbol_map_.size() > 0)
        return;
    for (auto &coff : coffs_) {
        for (auto const& [name, symbol] : coff.export_symbols()) {
            auto [it, inserted] = export_symbol_map_.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(name),
                    std::forward_as_tuple(coff)
                    );
            if (!inserted) {
                throw std::logic_error("re-defined symbol");
            }
        }
    }
}

void Library::resolve_external_symbols(std::function<uint64_t(std::string_view const)> const &resolver) {
    for (auto& [symbol_name, coff] : unresolved_external_symbols_) {
        coff.resolve_external_symbol(symbol_name, resolver(symbol_name));
    }
}

void Library::perform_relocations() {
    for (auto &coff : coffs_) {
        coff.perform_relocations();
    }
}

void Library::export_symbols(std::function<void(std::string_view symbol_name, uint64_t address)> exporter) {
    for (auto &coff : coffs_) {
        for (auto const& [name, symbol] : coff.export_symbols()) {
            exporter(name, coff.get_symbol_address_by_name(name));
        }
    }
}

void Library::commit_memory_spaces(std::function<void(uint64_t address, DWORD protection, std::vector<std::byte> const data)> performer) {
    for (auto &coff : coffs_) {
        for (auto &section : coff.sections()) {
            if (section.reserved_address && section.raw_data.size() && !section.discardable()) {
                performer(section.reserved_address, section.protection(), section.raw_data);
            }
        }
    }
}

uint64_t Library::get_entry_point() {
    fill_export_symbol_map();
    auto it = export_symbol_map_.find(ENTRY_POINT_NAME);
    if (it != export_symbol_map_.end()) {
        auto &symbol_name = it->first;
        auto &coff = it->second;
        return coff.get_symbol_address_by_name(symbol_name);
    } else {
        return 0;
    }
}

