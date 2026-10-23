#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"

namespace pipelang {

enum class IROpcode {
    Input,
    Clean,
    Filter,
    Transform,
    Sort,
    Group,
    Aggregate,
    Output
};

// One instruction per pipeline operation. The filter condition is kept as an
// expression tree (a copy of the AST's) so later passes can evaluate or
// rewrite it.
struct IRInstruction {
    std::size_t id = 0;
    int line = 0;
    IROpcode opcode = IROpcode::Input;
    std::string dataset;
    std::string field;
    std::string function;
    ExprPtr condition;
    SortOrder order = SortOrder::Ascending;
};

using IRProgram = std::vector<IRInstruction>;

// Expects a pipeline that passed semantic analysis.
IRProgram generate_ir(const Pipeline& pipeline);
IRProgram clone_program(const IRProgram& program);
const char* ir_opcode_name(IROpcode opcode);
// Short description of what an instruction does, e.g. "by revenue descending".
std::string instruction_detail(const IRInstruction& instruction);
void print_ir(const IRProgram& program, std::ostream& output);

}  // namespace pipelang
