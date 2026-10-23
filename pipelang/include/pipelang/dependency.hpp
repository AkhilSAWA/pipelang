#pragma once

#include <cstddef>
#include <vector>

#include "pipelang/ir.hpp"

namespace pipelang {

// `to` must run after `from`. Edges always point from a lower instruction id
// to a higher one.
struct DependencyEdge {
    std::size_t from = 0;
    std::size_t to = 0;
};

// Each operation depends on the previous operation on the same dataset.
std::vector<DependencyEdge> analyze_dependencies(const IRProgram& program);

}  // namespace pipelang
