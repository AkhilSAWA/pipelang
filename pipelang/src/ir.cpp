#include "pipelang/ir.hpp"

#include <sstream>

namespace pipelang {
namespace {

IROpcode to_ir_opcode(StmtKind kind) {
    switch (kind) {
        case StmtKind::Input: return IROpcode::Input;
        case StmtKind::Clean: return IROpcode::Clean;
        case StmtKind::Filter: return IROpcode::Filter;
        case StmtKind::Transform: return IROpcode::Transform;
        case StmtKind::Sort: return IROpcode::Sort;
        case StmtKind::Group: return IROpcode::Group;
        case StmtKind::Aggregate: return IROpcode::Aggregate;
        case StmtKind::Output: return IROpcode::Output;
    }
    return IROpcode::Input;
}

std::string expression_text(const Expr& expression) {
    if (expression.kind != ExprKind::Binary) {
        return expression.value;
    }

    return "(" + expression_text(*expression.left) + " " + expression.value +
           " " + expression_text(*expression.right) + ")";
}

}  // namespace

IRProgram generate_ir(const Pipeline& pipeline) {
    IRProgram program;
    program.reserve(pipeline.statements.size());

    for (std::size_t index = 0; index < pipeline.statements.size(); ++index) {
        const Stmt& statement = *pipeline.statements[index];
        IRInstruction instruction;
        instruction.id = index;
        instruction.opcode = to_ir_opcode(statement.kind);
        instruction.dataset = statement.dataset;
        instruction.order = statement.order;

        if (statement.kind == StmtKind::Filter && statement.expression) {
            instruction.condition = expression_text(*statement.expression);
        } else if (statement.kind == StmtKind::Aggregate) {
            const std::string marker = " using ";
            const std::size_t position = statement.argument.find(marker);
            if (position == std::string::npos) {
                instruction.field = statement.argument;
            } else {
                instruction.field = statement.argument.substr(0, position);
                instruction.function = statement.argument.substr(position + marker.size());
            }
        } else if (statement.kind == StmtKind::Transform) {
            instruction.function = statement.argument;
        } else {
            instruction.field = statement.argument;
        }

        program.push_back(std::move(instruction));
    }

    return program;
}

const char* ir_opcode_name(IROpcode opcode) {
    switch (opcode) {
        case IROpcode::Input: return "INPUT";
        case IROpcode::Clean: return "CLEAN";
        case IROpcode::Filter: return "FILTER";
        case IROpcode::Transform: return "TRANSFORM";
        case IROpcode::Sort: return "SORT";
        case IROpcode::Group: return "GROUP";
        case IROpcode::Aggregate: return "AGGREGATE";
        case IROpcode::Output: return "OUTPUT";
    }
    return "UNKNOWN";
}

void print_ir(const IRProgram& program, std::ostream& output) {
    output << "Intermediate Representation\n";
    for (const IRInstruction& instruction : program) {
        output << '[' << instruction.id << "] "
               << ir_opcode_name(instruction.opcode)
               << " dataset=" << instruction.dataset;
        if (!instruction.condition.empty()) {
            output << " condition=" << instruction.condition;
        }
        if (!instruction.field.empty()) {
            output << " field=" << instruction.field;
        }
        if (!instruction.function.empty()) {
            output << " function=" << instruction.function;
        }
        if (instruction.opcode == IROpcode::Sort) {
            output << " order="
                   << (instruction.order == SortOrder::Ascending
                       ? "ascending" : "descending");
        }
        output << '\n';
    }
}

}  // namespace pipelang
