#include <iostream>
#include <string>

#include "pipelang/ast.hpp"
#include "pipelang/dag.hpp"
#include "pipelang/driver.hpp"
#include "pipelang/ir.hpp"
#include "pipelang/semantic.hpp"

namespace {

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program
        << " [--dump-tokens] [--dump-ast] [--dump-symbols]"
           " [--dump-ir] [--dump-dag] <source.pipe>\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_symbols = false;
    bool dump_ir = false;
    bool dump_dag = false;
    std::string input_file;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--dump-tokens") dump_tokens = true;
        else if (argument == "--dump-ast") dump_ast = true;
        else if (argument == "--dump-symbols") dump_symbols = true;
        else if (argument == "--dump-ir") dump_ir = true;
        else if (argument == "--dump-dag") dump_dag = true;
        else if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (!argument.empty() && argument.front() == '-') {
            std::cerr << "Unknown option: " << argument << '\n';
            print_usage(argv[0]);
            return 2;
        } else if (input_file.empty()) input_file = argument;
        else {
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

    const pipelang::Pipeline& pipeline = *driver.program();
    const pipelang::SemanticResult semantics = pipelang::analyze_semantics(pipeline);
    if (!semantics.ok()) {
        pipelang::print_semantic_errors(semantics, std::cerr);
        return 1;
    }

    const pipelang::IRProgram ir = pipelang::generate_ir(pipeline);
    const pipelang::PipelineDag dag = pipelang::build_dag(ir);

    if (dump_tokens) driver.print_tokens(std::cout);
    if (dump_ast) pipelang::print_ast(pipeline, std::cout);
    if (dump_symbols) semantics.symbols.print(std::cout);
    if (dump_ir) pipelang::print_ir(ir, std::cout);
    if (dump_dag) pipelang::print_dag(dag, ir, std::cout);

    if (!dump_tokens && !dump_ast && !dump_symbols && !dump_ir && !dump_dag) {
        std::cout << "Compiled pipeline: " << pipeline.name << '\n'
                  << "Semantic analysis: PASSED\n"
                  << "IR instructions: " << ir.size() << '\n'
                  << "DAG edges: " << dag.edges().size() << '\n';
    }

    return 0;
}
