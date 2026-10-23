#include "pipelang/dag.hpp"

#include <queue>
#include <stdexcept>
#include <utility>

namespace pipelang {
namespace {

// Kahn's algorithm, processed one level at a time.
std::vector<std::vector<std::size_t>> compute_levels(
    std::size_t node_count,
    const std::vector<DependencyEdge>& edges
) {
    std::vector<std::vector<std::size_t>> adjacency(node_count);
    std::vector<std::size_t> indegree(node_count, 0);

    for (const DependencyEdge& edge : edges) {
        adjacency[edge.from].push_back(edge.to);
        ++indegree[edge.to];
    }

    std::queue<std::size_t> ready;
    for (std::size_t node = 0; node < node_count; ++node) {
        if (indegree[node] == 0) {
            ready.push(node);
        }
    }

    std::vector<std::vector<std::size_t>> levels;
    std::size_t visited = 0;
    while (!ready.empty()) {
        const std::size_t count = ready.size();
        std::vector<std::size_t> level;
        level.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            const std::size_t node = ready.front();
            ready.pop();
            level.push_back(node);
            ++visited;
            for (const std::size_t successor : adjacency[node]) {
                if (--indegree[successor] == 0) {
                    ready.push(successor);
                }
            }
        }
        levels.push_back(std::move(level));
    }

    // Dependency analysis only creates forward edges, so this is an internal
    // invariant rather than a user error.
    if (visited != node_count) {
        throw std::logic_error("pipeline dependency graph contains a cycle");
    }
    return levels;
}

}  // namespace

PipelineDag::PipelineDag(
    std::size_t node_count,
    std::vector<DependencyEdge> edges
)
    : node_count_(node_count),
      edges_(std::move(edges)),
      levels_(compute_levels(node_count_, edges_)) {}

PipelineDag build_dag(const IRProgram& program) {
    return PipelineDag(program.size(), analyze_dependencies(program));
}

void print_dag(
    const PipelineDag& dag,
    const IRProgram& program,
    std::ostream& output
) {
    output << "Pipeline DAG\nNodes:\n";
    for (const IRInstruction& instruction : program) {
        output << '[' << instruction.id << "] "
               << ir_opcode_name(instruction.opcode)
               << " dataset=" << instruction.dataset << '\n';
    }

    output << "Edges:\n";
    for (const DependencyEdge& edge : dag.edges()) {
        output << edge.from << " -> " << edge.to << '\n';
    }

    output << "Execution levels:\n";
    const auto& levels = dag.execution_levels();
    for (std::size_t index = 0; index < levels.size(); ++index) {
        output << "Level " << index << ':';
        for (const std::size_t node : levels[index]) {
            output << " [" << node << ']';
        }
        output << '\n';
    }
}

}  // namespace pipelang
