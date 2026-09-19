#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/ast.hpp"

namespace pipelang {

struct TokenRecord {
    std::string kind;
    std::string lexeme;
    int line;
};

class Driver {
public:
    bool parse_file(const std::string& path);
    bool parse_stream(std::istream& input, const std::string& source_name);

    void set_program(PipelinePtr program);
    const Pipeline* program() const;

    void set_dump_tokens(bool enabled);
    const std::vector<TokenRecord>& tokens() const;
    void note_token(std::string kind, std::string lexeme, int line);

    void error(int line, const std::string& message);
    const std::vector<std::string>& errors() const;
    void print_tokens(std::ostream& output) const;

private:
    PipelinePtr program_;
    bool dump_tokens_ = false;
    std::vector<TokenRecord> tokens_;
    std::vector<std::string> errors_;
};

}  // namespace pipelang