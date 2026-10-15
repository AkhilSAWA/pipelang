#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>

namespace pipelang {

struct Symbol {
    std::string name;
    std::size_t declaration_index = 0;
};

class SymbolTable {
public:
    bool insert(std::string name, std::size_t declaration_index);
    const Symbol* find(const std::string& name) const;
    bool contains(const std::string& name) const;
    void print(std::ostream& output) const;

private:
    std::unordered_map<std::string, Symbol> symbols_;
};

}  // namespace pipelang
