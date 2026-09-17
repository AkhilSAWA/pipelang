// ============================================================
// PipeLang driver — glues Flex + Bison together
// ============================================================
#include "pipelang/driver.hpp"
#include "parser.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>

// Flex-generated globals
extern FILE* yyin;
extern int   yylex();
extern int   yylineno;

namespace pipelang {

int Driver::parse_file(const std::string& path) {
    source_path = path;
    yyin = std::fopen(path.c_str(), "r");
    if (!yyin) {
        std::fprintf(stderr, "error: cannot open '%s'\n", path.c_str());
        return 1;
    }

    // TODO(phase 2): switch to full C++ lexer/parser bridge.
    // For the initial prototype we invoke Bison's generated parser via a
    // classic yyparse() shim. When you regenerate with `%define
    // api.token.constructor`, replace this with a proper symbol_type pump.
    yy::parser parser(*this);
    int status = parser.parse();

    std::fclose(yyin);
    return status;
}

} // namespace pipelang
