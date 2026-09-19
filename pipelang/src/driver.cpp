// ============================================================
// PipeLang driver — glues Flex + Bison together
// ============================================================
#include "pipelang/driver.hpp"
#include "parser.hpp"

#include <cstdio>
#include <iostream>

// Flex-generated globals
extern FILE* yyin;
extern int   yylineno;

namespace pipelang {

int Driver::parse_file(const std::string& path) {
    source_path = path;
    yyin = std::fopen(path.c_str(), "r");
    if (!yyin) {
        std::fprintf(stderr, "error: cannot open '%s'\n", path.c_str());
        return 1;
    }
    yylineno = 1;

    yy::parser parser(*this);
    int status = parser.parse();

    std::fclose(yyin);
    return status;
}

} // namespace pipelang
