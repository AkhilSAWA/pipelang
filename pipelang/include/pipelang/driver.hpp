#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"
#include "pipelang/diagnostic.hpp"

namespace pipelang {

struct TokenRecord {
    std::string kind;
    std::string lexeme;
    int line;
};

// Runs the Flex lexer and Bison parser over one source file and collects the
// resulting AST, lexical/syntax diagnostics and (optionally) the token stream.
class Driver {
public:
    bool parse_file(const std::string& path);
    bool parse_stream(std::istream& input, const std::string& source_name);

    void set_program(PipelinePtr program);
    const Pipeline* program() const;

    void set_dump_tokens(bool enabled);
    void note_token(std::string kind, std::string lexeme, int line);
    void print_tokens(std::ostream& output) const;

    void report(DiagnosticKind kind, int line, std::string message);
    const std::vector<Diagnostic>& diagnostics() const;

private:
    PipelinePtr program_;
    bool dump_tokens_ = false;
    std::vector<TokenRecord> tokens_;
    std::vector<Diagnostic> diagnostics_;
};

}  // namespace pipelang
