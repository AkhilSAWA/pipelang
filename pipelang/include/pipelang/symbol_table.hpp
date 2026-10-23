#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pipelang {

// A dataset introduced by `input`.
struct Symbol {
    std::string name;
    int declaration_line = 0;
    int output_line = 0;  // 0 until the dataset is output
};

// Datasets in declaration order, with constant-time lookup by name.
class SymbolTable {
public:
    // Returns false (and changes nothing) when the name is already declared.
    bool declare(const std::string& name, int declaration_line);

    Symbol* find(const std::string& name);
    const Symbol* find(const std::string& name) const;
    const std::vector<Symbol>& symbols() const { return symbols_; }

    void print(std::ostream& output) const;

private:
    std::vector<Symbol> symbols_;
    std::unordered_map<std::string, std::size_t> index_;
};

}  // namespace pipelang
