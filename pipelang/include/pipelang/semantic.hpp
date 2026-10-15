#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/symbol_table.hpp"

namespace pipelang {

struct SemanticError {
    std::size_t line = 0;
    std::string message;
};

struct SemanticResult {
    SymbolTable symbols;
    std::vector<SemanticError> errors;

    bool ok() const { return errors.empty(); }
};

SemanticResult analyze_semantics(const Pipeline& pipeline);
void print_semantic_errors(const SemanticResult& result, std::ostream& output);

}  // namespace pipelang
