#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ir.hpp"

namespace pipelang {

// One change made by an optimization pass.
struct OptimizationStep {
    std::string pass;  // e.g. "filter-pushdown"
    int line = 0;      // source line of the affected statement
    std::string message;
};

struct OptimizationResult {
    IRProgram program;  // renumbered; each instruction keeps its source line
    std::vector<OptimizationStep> steps;
};

// Rewrites a validated IR program into an equivalent, cheaper one. Every pass
// preserves the exact output of the pipeline:
//   constant-folding     evaluates literal arithmetic and comparisons
//   dead-code            drops datasets that are never output and filters
//                        that are always true
//   filter-pushdown      moves filters before clean/sort/group and before
//                        transforms that do not affect the filtered fields
//   filter-merge         joins adjacent filters with `and`
//   redundant-clean      drops a clean when the rows are already clean
//   redundant-sort       drops a sort whose order is overwritten or ignored
// The input program is not modified.
OptimizationResult optimize(const IRProgram& program);

void print_optimization(
    const OptimizationResult& result,
    std::size_t original_size,
    std::ostream& output
);

}  // namespace pipelang
