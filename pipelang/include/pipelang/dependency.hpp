#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include "pipelang/ir.hpp"

namespace pipelang {

struct DependencyEdge {
    std::size_t from = 0;
    std::size_t to = 0;
};

std::vector<DependencyEdge> analyze_dependencies(const IRProgram& program);
void print_dependencies(
    const std::vector<DependencyEdge>& edges,
    std::ostream& output
);

}  // namespace pipelang
