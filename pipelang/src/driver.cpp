#include "pipelang/driver.hpp"

#include <fstream>
#include <sstream>
#include <utility>

struct yy_buffer_state;
using YY_BUFFER_STATE = yy_buffer_state*;

extern int yyparse(pipelang::Driver& driver);
extern YY_BUFFER_STATE yy_scan_bytes(const char* bytes, int length);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern int yylineno;

namespace pipelang {

bool Driver::parse_file(const std::string& path) {
    std::ifstream input(path);

    if (!input) {
        error(0, "cannot open file: " + path);
        return false;
    }

    return parse_stream(input, path);
}

bool Driver::parse_stream(
    std::istream& input,
    const std::string& source_name
) {
    program_.reset();
    tokens_.clear();
    errors_.clear();

    std::ostringstream buffer;
    buffer << input.rdbuf();

    const std::string source = buffer.str();

    yylineno = 1;

    YY_BUFFER_STATE scanner_buffer =
        yy_scan_bytes(source.c_str(), static_cast<int>(source.size()));

    const int result = yyparse(*this);

    yy_delete_buffer(scanner_buffer);

    if (result != 0 && errors_.empty()) {
        error(0, "parser failed for " + source_name);
    }

    return result == 0 && errors_.empty() && program_ != nullptr;
}

void Driver::set_program(PipelinePtr program) {
    program_ = std::move(program);
}

const Pipeline* Driver::program() const {
    return program_.get();
}

void Driver::set_dump_tokens(bool enabled) {
    dump_tokens_ = enabled;
}

const std::vector<TokenRecord>& Driver::tokens() const {
    return tokens_;
}

void Driver::note_token(
    std::string kind,
    std::string lexeme,
    int line
) {
    if (dump_tokens_) {
        tokens_.push_back({
            std::move(kind),
            std::move(lexeme),
            line
        });
    }
}

void Driver::error(int line, const std::string& message) {
    const std::string prefix =
        line > 0 ? "line " + std::to_string(line) + ": " : "";

    errors_.push_back(prefix + message);
}

const std::vector<std::string>& Driver::errors() const {
    return errors_;
}

void Driver::print_tokens(std::ostream& output) const {
    for (const auto& token : tokens_) {
        output << token.line
               << "  "
               << token.kind
               << "  "
               << token.lexeme
               << '\n';
    }
}

}  // namespace pipelang