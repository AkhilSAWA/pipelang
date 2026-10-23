#include "pipelang/symbol_table.hpp"

#include <iomanip>

namespace pipelang {

bool SymbolTable::declare(const std::string& name, int declaration_line) {
    const bool inserted = index_.emplace(name, symbols_.size()).second;
    if (inserted) {
        symbols_.push_back({name, declaration_line, 0});
    }
    return inserted;
}

Symbol* SymbolTable::find(const std::string& name) {
    const auto iterator = index_.find(name);
    return iterator == index_.end() ? nullptr : &symbols_[iterator->second];
}

const Symbol* SymbolTable::find(const std::string& name) const {
    const auto iterator = index_.find(name);
    return iterator == index_.end() ? nullptr : &symbols_[iterator->second];
}

void SymbolTable::print(std::ostream& output) const {
    output << "Symbol Table\n"
           << std::left << std::setw(20) << "Dataset"
           << std::setw(14) << "Declared at"
           << "Output at\n";
    for (const Symbol& symbol : symbols_) {
        const std::string output_line = symbol.output_line > 0
            ? "line " + std::to_string(symbol.output_line)
            : "-";
        output << std::left << std::setw(20) << symbol.name
               << std::setw(14) << "line " + std::to_string(symbol.declaration_line)
               << output_line << '\n';
    }
}

}  // namespace pipelang
