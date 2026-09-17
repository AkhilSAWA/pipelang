// ============================================================
// PipeLang compiler — CLI entry point
//   pipelangc <file.pipe> [--dump-tokens] [--dump-ast]
// ============================================================
#include "pipelang/driver.hpp"

#include <iostream>
#include <string>
#include <vector>

static void usage(const char* prog) {
    std::cerr <<
        "PipeLang compiler (prototype)\n"
        "Usage: " << prog << " <file.pipe> [--dump-tokens] [--dump-ast]\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 2; }

    pipelang::Driver drv;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--dump-tokens") drv.dump_tokens = true;
        else if (a == "--dump-ast")    drv.dump_ast    = true;
        else if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else                                   path = a;
    }
    if (path.empty()) { usage(argv[0]); return 2; }

    int rc = drv.parse_file(path);
    if (rc != 0) {
        std::cerr << "compilation failed (parse rc=" << rc << ")\n";
        return rc;
    }

    if (drv.dump_ast && drv.pipeline()) {
        std::cout << "=== AST ===\n";
        drv.pipeline()->print(std::cout);
    }

    std::cout << "OK: parsed '" << path << "'\n";
    return 0;
}
