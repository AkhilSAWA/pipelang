#pragma once

#include <map>
#include <string>
#include <vector>

#include "pipelang/diagnostic.hpp"
#include "pipelang/ir.hpp"
#include "pipelang/value.hpp"

namespace pipelang {

struct SchemaResult {
    std::vector<Diagnostic> diagnostics;
    // Columns of each instruction's dataset after the instruction runs
    // (indexed by instruction id; empty when an earlier error stopped the check).
    std::vector<Schema> after;

    bool ok() const { return !has_errors(diagnostics); }
};

// Checks the program against the column schemas of its input files, before
// anything runs: every field must exist with a suitable type where it is
// used. Transforms and aggregates change the schema as they would at run time.
SchemaResult analyze_schemas(
    const IRProgram& program,
    const std::map<std::string, Schema>& inputs
);

}  // namespace pipelang
