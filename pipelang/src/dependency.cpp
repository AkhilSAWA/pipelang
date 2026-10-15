#include "pipelang/dependency.hpp"

#include <unordered_map>

namespace pipelang {

std::vector<DependencyEdge> analyze_dependencies(const IRProgram& program) {
    std::unordered_map<std::string, std::size_t> latest_operation;
    std::vector<DependencyEdge> edges;

    for (const IRInstruction& instruction : program) {
        const auto previous = latest_operation.find(instruction.dataset);
        if (previous != latest_operation.end()) {
            edges.push_back({previous->second, instruction.id});
        }
        latest_operation[instruction.dataset] = instruction.id;
    }

    return edges;
}

void print_dependencies(
    const std::vector<DependencyEdge>& edges,
    std::ostream& output
) {
    output << "Dependency Edges\n";
    for (const DependencyEdge& edge : edges) {
        output << edge.from << " -> " << edge.to << '\n';
    }
}

}  // namespace pipelang
