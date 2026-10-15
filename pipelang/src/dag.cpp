#include "pipelang/dag.hpp"

#include <queue>
#include <stdexcept>
#include <utility>

namespace pipelang {

PipelineDag::PipelineDag(
    std::size_t node_count,
    std::vector<DependencyEdge> edges
)
    : node_count_(node_count), edges_(std::move(edges)) {}

std::vector<std::vector<std::size_t>> PipelineDag::execution_levels() const {
    std::vector<std::vector<std::size_t>> adjacency(node_count_);
    std::vector<std::size_t> indegree(node_count_, 0);

    for (const DependencyEdge& edge : edges_) {
        adjacency[edge.from].push_back(edge.to);
        ++indegree[edge.to];
    }

    std::queue<std::size_t> ready;
    for (std::size_t node = 0; node < node_count_; ++node) {
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

    if (visited != node_count_) {
        throw std::logic_error("pipeline dependency graph contains a cycle");
    }
    return levels;
}

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
    const auto levels = dag.execution_levels();
    for (std::size_t index = 0; index < levels.size(); ++index) {
        output << "Level " << index << ':';
        for (const std::size_t node : levels[index]) {
            output << " [" << node << ']';
        }
        output << '\n';
    }
}

}  // namespace pipelang
