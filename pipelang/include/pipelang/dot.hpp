#pragma once

#include <ostream>
#include <string>

#include "pipelang/dag.hpp"
#include "pipelang/ir.hpp"

namespace pipelang {

// Writes the DAG in Graphviz DOT format: one cluster per dataset, one node
// per instruction, coloured by operation. Render with e.g.
//   dot -Tsvg pipeline.dot -o pipeline.svg
void write_dot(
    const IRProgram& program,
    const PipelineDag& dag,
    const std::string& pipeline_name,
    std::ostream& output
);

}  // namespace pipelang
