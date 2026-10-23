#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include "pipelang/ast.hpp"
#include "pipelang/dag.hpp"
#include "pipelang/diagnostic.hpp"
#include "pipelang/dot.hpp"
#include "pipelang/driver.hpp"
#include "pipelang/executor.hpp"
#include "pipelang/ir.hpp"
#include "pipelang/optimizer.hpp"
#include "pipelang/report.hpp"
#include "pipelang/schema.hpp"
#include "pipelang/semantic.hpp"

namespace {

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program << " [options] <source.pipe>\n"
           "\n"
           "Compiler stages:\n"
           "  --dump-tokens        print the token stream\n"
           "  --dump-ast           print the syntax tree\n"
           "  --dump-symbols       print the dataset symbol table\n"
           "  --dump-ir            print the intermediate representation\n"
           "  --dump-dag           print the dependency DAG and execution levels\n"
           "\n"
           "Optimization:\n"
           "  -O, --optimize       apply the optimization passes\n"
           "  --dump-opt           print the optimization log and optimized IR (implies -O)\n"
           "\n"
           "Execution:\n"
           "  --run                execute the pipeline on CSV data and print the outputs\n"
           "  --data <dir>         directory with <dataset>.csv files\n"
           "                       (default: the 'data' folder next to the source file)\n"
           "  --out <dir>          also write each output dataset to <dir>/<dataset>.csv\n"
           "  --jobs <n>           worker threads; 0 = one per CPU core (default 1)\n"
           "  --timings            print per-node execution times\n"
           "\n"
           "Visualization:\n"
           "  --emit-dot <file>    write the DAG as Graphviz DOT ('-' = stdout)\n"
           "  --report <file>      write a self-contained HTML report\n";
}

struct Options {
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_symbols = false;
    bool dump_ir = false;
    bool dump_dag = false;
    bool dump_opt = false;
    bool optimize = false;
    bool run = false;
    bool timings = false;
    std::optional<std::string> data_dir;
    std::string out_dir;
    unsigned jobs = 1;
    std::string dot_file;
    std::string report_file;
    std::string input_file;
};

// Returns 0 when the arguments are valid, otherwise the exit code to use.
int parse_arguments(int argc, char** argv, Options& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto value = [&](std::string& target) -> bool {
            if (index + 1 >= argc) {
                std::cerr << "Option " << argument << " needs a value.\n";
                return false;
            }
            target = argv[++index];
            return true;
        };

        if (argument == "--dump-tokens") options.dump_tokens = true;
        else if (argument == "--dump-ast") options.dump_ast = true;
        else if (argument == "--dump-symbols") options.dump_symbols = true;
        else if (argument == "--dump-ir") options.dump_ir = true;
        else if (argument == "--dump-dag") options.dump_dag = true;
        else if (argument == "--dump-opt") options.dump_opt = options.optimize = true;
        else if (argument == "-O" || argument == "--optimize") options.optimize = true;
        else if (argument == "--run") options.run = true;
        else if (argument == "--timings") options.timings = true;
        else if (argument == "--data") {
            std::string directory;
            if (!value(directory)) return 2;
            options.data_dir = directory;
        } else if (argument == "--out") {
            if (!value(options.out_dir)) return 2;
        } else if (argument == "--jobs") {
            std::string text;
            if (!value(text)) return 2;
            const bool digits = !text.empty() &&
                text.find_first_not_of("0123456789") == std::string::npos && text.size() <= 4;
            if (!digits) {
                std::cerr << "--jobs needs a whole number, got '" << text << "'.\n";
                return 2;
            }
            options.jobs = static_cast<unsigned>(std::stoul(text));
            if (options.jobs == 0) {
                options.jobs = std::max(1u, std::thread::hardware_concurrency());
            }
        } else if (argument == "--emit-dot") {
            if (!value(options.dot_file)) return 2;
        } else if (argument == "--report") {
            if (!value(options.report_file)) return 2;
        } else if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return -1;
        } else if (!argument.empty() && argument.front() == '-' && argument != "-") {
            std::cerr << "Unknown option: " << argument << '\n';
            print_usage(argv[0]);
            return 2;
        } else if (options.input_file.empty()) {
            options.input_file = argument;
        } else {
            std::cerr << "Only one source file is supported.\n";
            return 2;
        }
    }

    if (options.input_file.empty()) {
        print_usage(argv[0]);
        return 2;
    }
    const bool needs_run = options.timings || !options.out_dir.empty() || options.data_dir;
    if (needs_run && !options.run) {
        std::cerr << "--data, --out and --timings need --run.\n";
        return 2;
    }
    return 0;
}

std::string default_data_dir(const std::string& source) {
    const std::filesystem::path parent = std::filesystem::path(source).parent_path();
    return parent.empty() ? "data" : (parent / "data").generic_string();
}

bool write_text_file(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    file << text;
    file.flush();
    if (!file) {
        std::cerr << "error: cannot write file '" << path << "'\n";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    const int argument_status = parse_arguments(argc, argv, options);
    if (argument_status != 0) {
        return argument_status < 0 ? 0 : argument_status;
    }

    // ------------------------------------------------------------- front end
    pipelang::Driver driver;
    driver.set_dump_tokens(options.dump_tokens);
    const bool parsed = driver.parse_file(options.input_file);

    // Tokens are printed even when parsing fails; they help find the error.
    if (options.dump_tokens) driver.print_tokens(std::cout);

    if (!parsed) {
        pipelang::print_diagnostics(driver.diagnostics(), std::cerr);
        return 1;
    }

    const pipelang::Pipeline& pipeline = *driver.program();
    const pipelang::SemanticResult semantics = pipelang::analyze_semantics(pipeline);
    pipelang::print_diagnostics(semantics.diagnostics, std::cerr);
    if (!semantics.ok()) {
        return 1;
    }

    const pipelang::IRProgram ir = pipelang::generate_ir(pipeline);
    const pipelang::PipelineDag dag = pipelang::build_dag(ir);

    // ------------------------------------------------- data and schema check
    // Inputs are loaded before optimizing so that field errors are reported
    // against the program as written.
    std::optional<pipelang::LoadedInputs> inputs;
    if (options.run) {
        const std::string data_dir = options.data_dir ? *options.data_dir
                                                      : default_data_dir(options.input_file);
        inputs = pipelang::load_inputs(ir, data_dir, options.jobs);
        if (!inputs->ok()) {
            pipelang::print_diagnostics(inputs->diagnostics, std::cerr);
            return 1;
        }
        std::map<std::string, pipelang::Schema> schemas;
        for (const auto& [dataset, table] : inputs->tables) {
            schemas[dataset] = table.columns;
        }
        const pipelang::SchemaResult schema = pipelang::analyze_schemas(ir, schemas);
        if (!schema.ok()) {
            pipelang::print_diagnostics(schema.diagnostics, std::cerr);
            return 1;
        }
    }

    // ------------------------------------------------------------ optimizer
    std::optional<pipelang::OptimizationResult> optimization;
    std::optional<pipelang::PipelineDag> optimized_dag;
    if (options.optimize) {
        optimization = pipelang::optimize(ir);
        optimized_dag = pipelang::build_dag(optimization->program);
    }
    const pipelang::IRProgram& final_ir = optimization ? optimization->program : ir;
    const pipelang::PipelineDag& final_dag = optimized_dag ? *optimized_dag : dag;

    if (options.dump_ast) pipelang::print_ast(pipeline, std::cout);
    if (options.dump_symbols) semantics.symbols.print(std::cout);
    if (options.dump_ir) pipelang::print_ir(ir, std::cout);
    if (options.dump_opt) pipelang::print_optimization(*optimization, ir.size(), std::cout);
    if (options.dump_dag) pipelang::print_dag(final_dag, final_ir, std::cout);

    if (!options.dot_file.empty()) {
        std::ostringstream dot;
        pipelang::write_dot(final_ir, final_dag, pipeline.name, dot);
        if (options.dot_file == "-") {
            std::cout << dot.str();
        } else if (!write_text_file(options.dot_file, dot.str())) {
            return 1;
        }
    }

    // ------------------------------------------------------------ execution
    std::optional<pipelang::ExecutionResult> execution;
    if (options.run) {
        pipelang::ExecutionOptions run_options;
        run_options.out_dir = options.out_dir;
        run_options.jobs = options.jobs;
        if (!run_options.out_dir.empty()) {
            std::error_code error;
            std::filesystem::create_directories(run_options.out_dir, error);
            if (error) {
                std::cerr << "error: cannot create output directory '" << run_options.out_dir
                          << "': " << error.message() << '\n';
                return 1;
            }
        }
        execution = pipelang::execute(final_ir, final_dag, std::move(inputs->tables), run_options);
        if (!execution->ok()) {
            pipelang::print_diagnostics(execution->diagnostics, std::cerr);
            return 1;
        }
        pipelang::print_outputs(*execution, std::cout);
        if (options.timings) {
            std::cout << '\n';
            pipelang::print_timings(*execution, final_ir, inputs->load_ms, std::cout);
        }
    }

    // --------------------------------------------------------------- report
    if (!options.report_file.empty()) {
        std::ifstream source_file(options.input_file, std::ios::binary);
        std::ostringstream source;
        source << source_file.rdbuf();

        pipelang::ReportInput report;
        report.source_path = options.input_file;
        report.source_text = source.str();
        report.pipeline = &pipeline;
        for (const pipelang::Diagnostic& diagnostic : semantics.diagnostics) {
            if (!pipelang::is_error(diagnostic)) report.warnings.push_back(diagnostic);
        }
        report.original = &ir;
        report.original_dag = &dag;
        report.optimization = optimization ? &*optimization : nullptr;
        report.optimized_dag = optimized_dag ? &*optimized_dag : nullptr;
        report.inputs = inputs ? &*inputs : nullptr;
        report.execution = execution ? &*execution : nullptr;

        std::ostringstream html;
        pipelang::write_report(report, html);
        if (!write_text_file(options.report_file, html.str())) {
            return 1;
        }
    }

    const bool printed = options.dump_tokens || options.dump_ast || options.dump_symbols ||
        options.dump_ir || options.dump_opt || options.dump_dag || options.run ||
        options.dot_file == "-";
    if (!printed) {
        std::cout << "Compiled pipeline: " << pipeline.name << '\n'
                  << "Semantic analysis: PASSED\n"
                  << "IR instructions: " << ir.size() << '\n';
        if (optimization) {
            std::cout << "Optimized instructions: " << final_ir.size() << " ("
                      << optimization->steps.size() << " optimization steps)\n";
        }
        std::cout << "DAG edges: " << final_dag.edges().size() << '\n'
                  << "Execution levels: " << final_dag.execution_levels().size() << '\n';
        if (!options.dot_file.empty()) std::cout << "DOT graph: " << options.dot_file << '\n';
        if (!options.report_file.empty()) std::cout << "HTML report: " << options.report_file << '\n';
    }

    return 0;
}
