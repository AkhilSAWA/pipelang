#pragma once

#include <cstddef>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "pipelang/dag.hpp"
#include "pipelang/diagnostic.hpp"
#include "pipelang/ir.hpp"
#include "pipelang/table.hpp"

namespace pipelang {

struct InputSummary {
    std::string file;
    std::size_t rows = 0;
    Schema columns;
    bool loaded = false;
};

struct LoadedInputs {
    std::map<std::string, Table> tables;  // by dataset name; moved into execute()
    std::map<std::string, InputSummary> summaries;
    std::vector<Diagnostic> diagnostics;
    double load_ms = 0;

    bool ok() const { return !has_errors(diagnostics); }
};

// Reads <data_dir>/<dataset>.csv for every input instruction, in parallel
// when jobs > 1.
LoadedInputs load_inputs(const IRProgram& program, const std::string& data_dir, unsigned jobs);

struct ExecutionOptions {
    std::string out_dir;  // write each output dataset as <out_dir>/<dataset>.csv; empty = don't
    unsigned jobs = 1;    // worker threads
};

// How one DAG node ran.
struct NodeRun {
    std::size_t instruction = 0;
    unsigned worker = 0;
    double start_ms = 0;
    double end_ms = 0;
    std::size_t rows_in = 0;
    std::size_t rows_out = 0;
};

struct DatasetOutput {
    std::string dataset;
    int line = 0;
    Table table;
    std::string file;  // empty when not written
};

struct ExecutionResult {
    std::vector<Diagnostic> diagnostics;
    std::vector<NodeRun> runs;            // indexed by instruction id
    std::vector<DatasetOutput> outputs;   // in program order
    unsigned workers = 1;
    double total_ms = 0;

    bool ok() const { return !has_errors(diagnostics); }
};

// Runs the program on the loaded inputs. With one job, nodes run in program
// order; with more, a thread pool runs every node as soon as the node it
// depends on has finished, so independent datasets are processed in
// parallel. Results are identical either way.
ExecutionResult execute(
    const IRProgram& program,
    const PipelineDag& dag,
    std::map<std::string, Table> inputs,
    const ExecutionOptions& options
);

void print_outputs(const ExecutionResult& result, std::ostream& output);
void print_timings(
    const ExecutionResult& result,
    const IRProgram& program,
    double load_ms,
    std::ostream& output
);

}  // namespace pipelang
