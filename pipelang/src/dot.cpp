#include "pipelang/dot.hpp"

#include <vector>

namespace pipelang {
namespace {

// Escapes text for a double-quoted DOT string.
std::string dot_escape(const std::string& text) {
    std::string escaped;
    for (const char character : text) {
        if (character == '"' || character == '\\') escaped += '\\';
        escaped += character;
    }
    return escaped;
}

const char* fill_colour(IROpcode opcode) {
    switch (opcode) {
        case IROpcode::Input:
        case IROpcode::Output: return "#dbeafe";
        case IROpcode::Clean: return "#dcfce7";
        case IROpcode::Filter: return "#fef3c7";
        case IROpcode::Transform: return "#ede9fe";
        case IROpcode::Sort:
        case IROpcode::Group: return "#fce7f3";
        case IROpcode::Aggregate: return "#ffedd5";
    }
    return "#ffffff";
}

}  // namespace

void write_dot(
    const IRProgram& program,
    const PipelineDag& dag,
    const std::string& pipeline_name,
    std::ostream& output
) {
    std::vector<std::string> datasets;
    for (const IRInstruction& instruction : program) {
        bool known = false;
        for (const std::string& dataset : datasets) known = known || dataset == instruction.dataset;
        if (!known) datasets.push_back(instruction.dataset);
    }

    output << "digraph \"" << dot_escape(pipeline_name) << "\" {\n"
           << "  label=\"" << dot_escape(pipeline_name) << "\";\n"
           << "  labelloc=t;\n"
           << "  fontname=\"Helvetica\";\n"
           << "  rankdir=TB;\n"
           << "  node [shape=box, style=\"rounded,filled\", fontname=\"Helvetica\", "
              "fontsize=11, color=\"#64748b\"];\n"
           << "  edge [color=\"#64748b\", arrowsize=0.7];\n";

    for (std::size_t lane = 0; lane < datasets.size(); ++lane) {
        output << "  subgraph \"cluster_" << lane << "\" {\n"
               << "    label=\"" << dot_escape(datasets[lane]) << "\";\n"
               << "    style=\"rounded,dashed\";\n"
               << "    color=\"#94a3b8\";\n";
        for (const IRInstruction& instruction : program) {
            if (instruction.dataset != datasets[lane]) continue;
            std::string label = "[" + std::to_string(instruction.id) + "] " +
                                ir_opcode_name(instruction.opcode);
            const std::string detail = instruction_detail(instruction);
            output << "    n" << instruction.id << " [label=\"" << dot_escape(label);
            if (!detail.empty()) output << "\\n" << dot_escape(detail);
            output << "\", fillcolor=\"" << fill_colour(instruction.opcode) << "\"];\n";
        }
        output << "  }\n";
    }

    for (const DependencyEdge& edge : dag.edges()) {
        output << "  n" << edge.from << " -> n" << edge.to << ";\n";
    }
    output << "}\n";
}

}  // namespace pipelang
