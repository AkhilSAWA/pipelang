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

struct IRInstruction {
    std::size_t id = 0;
    IROpcode opcode = IROpcode::Input;
    std::string dataset;
    std::string field;
    std::string function;
    std::string condition;
    SortOrder order = SortOrder::Ascending;
};

using IRProgram = std::vector<IRInstruction>;

IRProgram generate_ir(const Pipeline& pipeline);
const char* ir_opcode_name(IROpcode opcode);
void print_ir(const IRProgram& program, std::ostream& output);

}  // namespace pipelang
