#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include "pipelang/dependency.hpp"
#include "pipelang/ir.hpp"

namespace pipelang {

class PipelineDag {
public:
    PipelineDag(std::size_t node_count, std::vector<DependencyEdge> edges);

    std::size_t node_count() const { return node_count_; }
    const std::vector<DependencyEdge>& edges() const { return edges_; }
    std::vector<std::vector<std::size_t>> execution_levels() const;

private:
    std::size_t node_count_;
    std::vector<DependencyEdge> edges_;
};

PipelineDag build_dag(const IRProgram& program);
void print_dag(
    const PipelineDag& dag,
    const IRProgram& program,
    std::ostream& output
);

}  // namespace pipelang
