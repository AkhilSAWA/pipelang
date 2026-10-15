#include "pipelang/symbol_table.hpp"

#include <algorithm>
#include <iomanip>
#include <utility>
#include <vector>

namespace pipelang {

bool SymbolTable::insert(std::string name, std::size_t declaration_index) {
    // Copy the key first: argument evaluation order is unspecified, so using
    // `name` as the key and `std::move(name)` in the same call can leave the
    // key empty (GCC evaluates right-to-left).
    std::string key = name;
    const auto result = symbols_.emplace(
        std::move(key),
        Symbol{std::move(name), declaration_index}
    );
    return result.second;
}

const Symbol* SymbolTable::find(const std::string& name) const {
    const auto iterator = symbols_.find(name);
    return iterator == symbols_.end() ? nullptr : &iterator->second;
}

bool SymbolTable::contains(const std::string& name) const {
    return find(name) != nullptr;
}

void SymbolTable::print(std::ostream& output) const {
    std::vector<const Symbol*> ordered;
    ordered.reserve(symbols_.size());
    for (const auto& entry : symbols_) {
        ordered.push_back(&entry.second);
    }
    std::sort(ordered.begin(), ordered.end(), [](const Symbol* left, const Symbol* right) {
        return left->declaration_index < right->declaration_index;
    });

    output << "Symbol Table\n"
           << std::left << std::setw(20) << "Dataset"
           << "Declaration statement\n";
    for (const Symbol* symbol : ordered) {
        output << std::left << std::setw(20) << symbol->name
               << symbol->declaration_index + 1 << '\n';
    }
}

}  // namespace pipelang
