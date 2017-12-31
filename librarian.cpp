#include "librarian.h"

using namespace rrl;

void Librarian::link(Courier &courier, Library &library) {
    auto external_symbols = library.get_unresolved_external_symbols();
    msg::ResolveExternalSymbols msg_resolve_symbols;
    for (auto symbol : external_symbols) {
        msg_resolve_symbols.body().emplace_back(std::make_pair("user32.dll", symbol));
    }
    courier.send(msg_resolve_symbols);

    auto resolved_symbols = courier.receive().cast<msg::ResolvedSymbols>();
    if (resolved_symbols.body().size() != external_symbols.size()) {}
    for (int i = 0; i < external_symbols.size(); i++) {
        auto const &symbol = external_symbols[i];
        auto address = resolved_symbols[i];
        library.resolve_external_symbol(symbol, address);
    }
}

