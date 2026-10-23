#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/dag.hpp"
#include "pipelang/diagnostic.hpp"
#include "pipelang/executor.hpp"
#include "pipelang/ir.hpp"
#include "pipelang/optimizer.hpp"

namespace pipelang {

// Everything the HTML report shows. Pointers are optional sections: no
// optimization -> no before/after comparison, no execution -> no timeline
// or results.
struct ReportInput {
    std::string source_path;
    std::string source_text;
    const Pipeline* pipeline = nullptr;
    std::vector<Diagnostic> warnings;
    const IRProgram* original = nullptr;
    const PipelineDag* original_dag = nullptr;
    const OptimizationResult* optimization = nullptr;
    const PipelineDag* optimized_dag = nullptr;
    const LoadedInputs* inputs = nullptr;
    const ExecutionResult* execution = nullptr;
};

// Writes a self-contained HTML page (inline CSS and SVG, no scripts or
// external files) that works offline and in light or dark mode.
void write_report(const ReportInput& input, std::ostream& output);

}  // namespace pipelang
