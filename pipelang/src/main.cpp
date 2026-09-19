#include <iostream>
#include <string>

#include "pipelang/ast.hpp"
#include "pipelang/driver.hpp"

namespace {

void print_usage(const char* program) {
    std::cout
        << "Usage: "
        << program
        << " [--dump-tokens] [--dump-ast] <source.pipe>\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool dump_tokens = false;
    bool dump_ast = false;
    std::string input_file;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--dump-tokens") {
            dump_tokens = true;
        } else if (argument == "--dump-ast") {
            dump_ast = true;
        } else if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (!argument.empty() && argument.front() == '-') {
            std::cerr << "Unknown option: " << argument << '\n';
            print_usage(argv[0]);
            return 2;
        } else if (input_file.empty()) {
            input_file = argument;
        } else {
            std::cerr << "Only one source file is supported.\n";
            return 2;
        }
    }

    if (input_file.empty()) {
        print_usage(argv[0]);
        return 2;
    }

    pipelang::Driver driver;
    driver.set_dump_tokens(dump_tokens);

    if (!driver.parse_file(input_file)) {
        for (const auto& error : driver.errors()) {
            std::cerr << "error: " << error << '\n';
        }
        return 1;
    }

    if (dump_tokens) {
        driver.print_tokens(std::cout);
    }

    if (dump_ast && driver.program()) {
        pipelang::print_ast(*driver.program(), std::cout);
    }

    if (!dump_tokens && !dump_ast) {
        std::cout
            << "Parsed pipeline: "
            << driver.program()->name
            << '\n';
    }

    return 0;
}