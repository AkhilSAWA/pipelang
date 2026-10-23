#include "pipelang/ir.hpp"

#include <utility>

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

}  // namespace

IRProgram generate_ir(const Pipeline& pipeline) {
    IRProgram program;
    program.reserve(pipeline.statements.size());

    for (const StmtPtr& statement : pipeline.statements) {
        IRInstruction instruction;
        instruction.id = program.size();
        instruction.line = statement->line;
        instruction.opcode = to_ir_opcode(statement->kind);
        instruction.dataset = statement->dataset;
        instruction.field = statement->field;
        instruction.function = statement->function;
        instruction.order = statement->order;
        if (statement->condition) {
            instruction.condition = clone_expr(*statement->condition);
        }
        program.push_back(std::move(instruction));
    }

    return program;
}

IRProgram clone_program(const IRProgram& program) {
    IRProgram copy;
    copy.reserve(program.size());
    for (const IRInstruction& instruction : program) {
        IRInstruction duplicate;
        duplicate.id = instruction.id;
        duplicate.line = instruction.line;
        duplicate.opcode = instruction.opcode;
        duplicate.dataset = instruction.dataset;
        duplicate.field = instruction.field;
        duplicate.function = instruction.function;
        duplicate.order = instruction.order;
        if (instruction.condition) {
            duplicate.condition = clone_expr(*instruction.condition);
        }
        copy.push_back(std::move(duplicate));
    }
    return copy;
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

std::string instruction_detail(const IRInstruction& instruction) {
    switch (instruction.opcode) {
        case IROpcode::Filter:
            return expr_to_string(*instruction.condition);
        case IROpcode::Transform:
            return instruction.function;
        case IROpcode::Sort:
            return "by " + instruction.field + " " + sort_order_name(instruction.order);
        case IROpcode::Group:
            return "by " + instruction.field;
        case IROpcode::Aggregate:
            return instruction.function + "(" + instruction.field + ")";
        case IROpcode::Input:
        case IROpcode::Clean:
        case IROpcode::Output:
            break;
    }
    return "";
}

void print_ir(const IRProgram& program, std::ostream& output) {
    output << "Intermediate Representation\n";
    for (const IRInstruction& instruction : program) {
        output << '[' << instruction.id << "] "
               << ir_opcode_name(instruction.opcode)
               << " dataset=" << instruction.dataset;
        if (instruction.condition) {
            output << " condition=" << expr_to_string(*instruction.condition);
        }
        if (!instruction.field.empty()) {
            output << " field=" << instruction.field;
        }
        if (!instruction.function.empty()) {
            output << " function=" << instruction.function;
        }
        if (instruction.opcode == IROpcode::Sort) {
            output << " order=" << sort_order_name(instruction.order);
        }
        output << '\n';
    }
}

}  // namespace pipelang
