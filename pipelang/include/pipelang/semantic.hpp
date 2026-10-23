#pragma once

#include <string>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/diagnostic.hpp"
#include "pipelang/symbol_table.hpp"
#include "pipelang/value.hpp"

namespace pipelang {

struct SemanticResult {
    SymbolTable symbols;
    std::vector<Diagnostic> diagnostics;  // semantic errors and warnings

    bool ok() const { return !has_errors(diagnostics); }
};

// Checks dataset usage (declare before use, no duplicates, nothing after
// output, every pipeline produces output), built-in function names, and the
// types of filter conditions.
SemanticResult analyze_semantics(const Pipeline& pipeline);

// Type-checks one filter condition and appends any errors. Without a schema
// (compile time) a field may hold a number or a string; with the dataset's
// schema (before execution) every field must exist and has its column type.
void check_condition(
    const Expr& condition,
    int line,
    const std::string& dataset,
    const Schema* schema,
    std::vector<Diagnostic>& diagnostics
);

}  // namespace pipelang
