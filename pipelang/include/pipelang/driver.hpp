#pragma once
// ============================================================
// PipeLang compiler driver — owns lexer state and AST result
// ============================================================
#include <string>
#include <memory>
#include "pipelang/ast.hpp"

namespace pipelang {

class Driver {
public:
    Driver() = default;

    // Parse a file; returns 0 on success.
    int parse_file(const std::string& path);

    // AST hand-off from Bison actions
    void set_pipeline(PipelinePtr p) { pipeline_ = std::move(p); }
    const Pipeline* pipeline() const { return pipeline_.get(); }

    // Flags controlled from main()
    bool dump_tokens = false;
    bool dump_ast    = false;

    std::string source_path;

private:
    PipelinePtr pipeline_;
};

} // namespace pipelang
