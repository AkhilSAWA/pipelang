#include "pipelang/executor.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <future>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "pipelang/functions.hpp"

namespace pipelang {
namespace {

using Clock = std::chrono::steady_clock;

double milliseconds_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// ------------------------------------------------------ condition evaluation

enum class NodeKind { Literal, Column, Negate, Arithmetic, Compare, And, Or, Boolean };
enum class Operator { Add, Subtract, Multiply, Divide, Equal, NotEqual, Less, Greater,
                      LessEqual, GreaterEqual, None };

// A filter condition with columns resolved to indexes and literals parsed
// once, so evaluating a row does no lookups or parsing.
struct CompiledExpr {
    NodeKind kind = NodeKind::Literal;
    Operator op = Operator::None;
    Value literal;
    bool boolean = false;
    std::size_t column = 0;
    std::unique_ptr<CompiledExpr> left;
    std::unique_ptr<CompiledExpr> right;
};

Operator parse_operator(const std::string& op) {
    if (op == "+") return Operator::Add;
    if (op == "-") return Operator::Subtract;
    if (op == "*") return Operator::Multiply;
    if (op == "/") return Operator::Divide;
    if (op == "==") return Operator::Equal;
    if (op == "!=") return Operator::NotEqual;
    if (op == "<") return Operator::Less;
    if (op == ">") return Operator::Greater;
    if (op == "<=") return Operator::LessEqual;
    if (op == ">=") return Operator::GreaterEqual;
    return Operator::None;
}

std::unique_ptr<CompiledExpr> compile(const Expr& expression, const Schema& schema) {
    auto node = std::make_unique<CompiledExpr>();
    switch (expression.kind) {
        case ExprKind::Integer:
        case ExprKind::Float:
            node->kind = NodeKind::Literal;
            node->literal = Value::number(std::strtod(expression.value.c_str(), nullptr));
            break;
        case ExprKind::String:
            node->kind = NodeKind::Literal;
            node->literal = Value::string(expression.value);
            break;
        case ExprKind::Boolean:
            node->kind = NodeKind::Boolean;
            node->boolean = expression.value == "true";
            break;
        case ExprKind::Identifier: {
            const std::optional<std::size_t> column = find_column(schema, expression.value);
            if (!column) {
                // Schema analysis rules this out before execution starts.
                throw std::logic_error("unknown column '" + expression.value + "'");
            }
            node->kind = NodeKind::Column;
            node->column = *column;
            break;
        }
        case ExprKind::Unary:
            node->kind = NodeKind::Negate;
            node->left = compile(*expression.left, schema);
            break;
        case ExprKind::Binary:
            node->left = compile(*expression.left, schema);
            node->right = compile(*expression.right, schema);
            if (expression.value == "and") {
                node->kind = NodeKind::And;
            } else if (expression.value == "or") {
                node->kind = NodeKind::Or;
            } else {
                node->op = parse_operator(expression.value);
                node->kind = node->op <= Operator::Divide ? NodeKind::Arithmetic
                                                          : NodeKind::Compare;
            }
            break;
    }
    return node;
}

// Arithmetic on a null or non-number gives null, and so does x / 0.
const Value& evaluate_value(const CompiledExpr& node, const Row& row, Value& scratch) {
    switch (node.kind) {
        case NodeKind::Literal:
            return node.literal;
        case NodeKind::Column:
            return row[node.column];
        case NodeKind::Negate: {
            Value temporary;
            const Value& operand = evaluate_value(*node.left, row, temporary);
            scratch = operand.is_number() ? Value::number(-operand.as_number()) : Value();
            return scratch;
        }
        case NodeKind::Arithmetic: {
            Value left_scratch;
            Value right_scratch;
            const Value& left = evaluate_value(*node.left, row, left_scratch);
            const Value& right = evaluate_value(*node.right, row, right_scratch);
            if (!left.is_number() || !right.is_number()) {
                scratch = Value();
                return scratch;
            }
            const double a = left.as_number();
            const double b = right.as_number();
            switch (node.op) {
                case Operator::Add: scratch = Value::number(a + b); break;
                case Operator::Subtract: scratch = Value::number(a - b); break;
                case Operator::Multiply: scratch = Value::number(a * b); break;
                default: scratch = b == 0.0 ? Value() : Value::number(a / b); break;
            }
            return scratch;
        }
        default:
            throw std::logic_error("boolean expression used as a value");
    }
}

// A comparison involving null is false (also for !=).
bool evaluate_condition(const CompiledExpr& node, const Row& row) {
    switch (node.kind) {
        case NodeKind::Boolean:
            return node.boolean;
        case NodeKind::And:
            return evaluate_condition(*node.left, row) && evaluate_condition(*node.right, row);
        case NodeKind::Or:
            return evaluate_condition(*node.left, row) || evaluate_condition(*node.right, row);
        case NodeKind::Compare: {
            Value left_scratch;
            Value right_scratch;
            const Value& left = evaluate_value(*node.left, row, left_scratch);
            const Value& right = evaluate_value(*node.right, row, right_scratch);
            const bool comparable = (left.is_number() && right.is_number()) ||
                                    (left.is_string() && right.is_string());
            if (!comparable) {
                return false;
            }
            const int order = compare_values(left, right);
            switch (node.op) {
                case Operator::Equal: return order == 0;
                case Operator::NotEqual: return order != 0;
                case Operator::Less: return order < 0;
                case Operator::Greater: return order > 0;
                case Operator::LessEqual: return order <= 0;
                default: return order >= 0;
            }
        }
        default:
            throw std::logic_error("value expression used as a condition");
    }
}

// --------------------------------------------------------------- operations

struct DatasetState {
    Table table;
    std::string group_key;  // empty when not grouped
};

std::size_t require_column(const Table& table, const std::string& name) {
    const std::optional<std::size_t> column = find_column(table.columns, name);
    if (!column) {
        throw std::logic_error("unknown column '" + name + "'");
    }
    return *column;
}

// Returns the index of `name`, appending it (null in every row) if missing.
std::size_t ensure_column(Table& table, const std::string& name, ColumnType type) {
    if (const std::optional<std::size_t> column = find_column(table.columns, name)) {
        table.columns[*column].type = type;
        return *column;
    }
    table.columns.push_back({name, type});
    for (Row& row : table.rows) {
        row.emplace_back();
    }
    return table.columns.size() - 1;
}

// Removes rows that contain a null cell, then duplicate rows (keeping the
// first occurrence).
void run_clean(Table& table) {
    std::unordered_set<std::string> seen;
    seen.reserve(table.rows.size());
    std::vector<Row> kept;
    kept.reserve(table.rows.size());
    std::string key;
    for (Row& row : table.rows) {
        const bool has_null = std::any_of(row.begin(), row.end(), [](const Value& value) {
            return value.is_null();
        });
        if (has_null) {
            continue;
        }
        key.clear();
        for (const Value& value : row) {
            append_value_key(value, key);
        }
        if (seen.insert(key).second) {
            kept.push_back(std::move(row));
        }
    }
    table.rows = std::move(kept);
}

void run_filter(Table& table, const Expr& condition) {
    const std::unique_ptr<CompiledExpr> compiled = compile(condition, table.columns);
    std::vector<Row> kept;
    kept.reserve(table.rows.size());
    for (Row& row : table.rows) {
        if (evaluate_condition(*compiled, row)) {
            kept.push_back(std::move(row));
        }
    }
    table.rows = std::move(kept);
}

void run_transform(Table& table, const std::string& name) {
    const TransformInfo* function = find_transform(name);
    if (function == nullptr) {
        throw std::logic_error("unknown transform '" + name + "'");
    }
    switch (function->id) {
        case TransformFunction::CalculateGrade: {
            const std::size_t marks = require_column(table, "marks");
            const std::size_t grade = ensure_column(table, "grade", ColumnType::String);
            for (Row& row : table.rows) {
                row[grade] = row[marks].is_number()
                    ? Value::string(letter_grade(row[marks].as_number()))
                    : Value();
            }
            break;
        }
        case TransformFunction::Uppercase:
            for (std::size_t column = 0; column < table.columns.size(); ++column) {
                if (table.columns[column].type != ColumnType::String) {
                    continue;
                }
                for (Row& row : table.rows) {
                    if (row[column].is_string()) {
                        std::string text = row[column].as_string();
                        for (char& character : text) {
                            character = static_cast<char>(
                                std::toupper(static_cast<unsigned char>(character))
                            );
                        }
                        row[column] = Value::string(std::move(text));
                    }
                }
            }
            break;
        case TransformFunction::AddRowNumber: {
            const std::size_t column = ensure_column(table, "row_number", ColumnType::Number);
            for (std::size_t index = 0; index < table.rows.size(); ++index) {
                table.rows[index][column] = Value::number(static_cast<double>(index + 1));
            }
            break;
        }
    }
}

// Stable, nulls last.
void run_sort(Table& table, const std::string& field, bool descending) {
    const std::size_t column = require_column(table, field);
    std::stable_sort(table.rows.begin(), table.rows.end(),
        [column, descending](const Row& a, const Row& b) {
            return null_last_less(a[column], b[column], descending);
        });
}

Value aggregate_values(const AggregateInfo& function, const std::vector<const Value*>& values) {
    std::vector<const Value*> present;
    for (const Value* value : values) {
        if (!value->is_null()) present.push_back(value);
    }
    switch (function.id) {
        case AggregateFunction::Count:
            return Value::number(static_cast<double>(present.size()));
        case AggregateFunction::Sum:
        case AggregateFunction::Avg: {
            // Adding in sorted order makes the sum independent of row order,
            // so reordering operations never changes an aggregate.
            std::vector<double> numbers;
            numbers.reserve(present.size());
            for (const Value* value : present) numbers.push_back(value->as_number());
            std::sort(numbers.begin(), numbers.end());
            double sum = 0;
            for (const double number : numbers) sum += number;
            if (function.id == AggregateFunction::Sum) {
                return Value::number(sum);
            }
            return numbers.empty() ? Value()
                                   : Value::number(sum / static_cast<double>(numbers.size()));
        }
        case AggregateFunction::Min:
        case AggregateFunction::Max: {
            const Value* best = nullptr;
            for (const Value* value : present) {
                const bool better = best == nullptr ||
                    (function.id == AggregateFunction::Min
                        ? compare_values(*value, *best) < 0
                        : compare_values(*value, *best) > 0);
                if (better) best = value;
            }
            return best == nullptr ? Value() : *best;
        }
    }
    return Value();
}

// One row per group (ordered by group key, nulls last), or a single row for
// the whole table when it is not grouped.
void run_aggregate(DatasetState& state, const std::string& field, const std::string& name) {
    Table& table = state.table;
    const AggregateInfo* function = find_aggregate(name);
    if (function == nullptr) {
        throw std::logic_error("unknown aggregate '" + name + "'");
    }
    const std::size_t value_column = require_column(table, field);
    const Column source = table.columns[value_column];
    const bool numeric = function->id == AggregateFunction::Count || function->needs_number;
    const Column produced{
        aggregate_column_name(*function, source.name),
        numeric ? ColumnType::Number : source.type
    };

    Table result;
    if (state.group_key.empty()) {
        std::vector<const Value*> values;
        values.reserve(table.rows.size());
        for (const Row& row : table.rows) values.push_back(&row[value_column]);
        result.columns = {produced};
        result.rows.push_back({aggregate_values(*function, values)});
    } else {
        const std::size_t key_column = require_column(table, state.group_key);
        std::vector<std::size_t> order(table.rows.size());
        for (std::size_t index = 0; index < order.size(); ++index) order[index] = index;
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            return null_last_less(table.rows[a][key_column], table.rows[b][key_column], false);
        });
        result.columns = {table.columns[key_column], produced};
        std::size_t begin = 0;
        while (begin < order.size()) {
            const Value& key = table.rows[order[begin]][key_column];
            std::size_t end = begin;
            std::vector<const Value*> values;
            while (end < order.size() && same_value(table.rows[order[end]][key_column], key)) {
                values.push_back(&table.rows[order[end]][value_column]);
                ++end;
            }
            result.rows.push_back({key, aggregate_values(*function, values)});
            begin = end;
        }
    }
    state.table = std::move(result);
    state.group_key.clear();
}

// ---------------------------------------------------------------- scheduler

class Runner {
public:
    Runner(const IRProgram& program, const PipelineDag& dag,
           std::map<std::string, Table> inputs, const ExecutionOptions& options)
        : program_(program), dag_(dag), options_(options) {
        for (const IRInstruction& instruction : program_) {
            if (dataset_index_.emplace(instruction.dataset, dataset_index_.size()).second) {
                states_.emplace_back();
                loaded_.emplace_back();
            }
        }
        for (auto& [name, table] : inputs) {
            const auto found = dataset_index_.find(name);
            if (found != dataset_index_.end()) {
                loaded_[found->second] = std::move(table);
            }
        }
        result_.runs.resize(program_.size());
        for (std::size_t node = 0; node < program_.size(); ++node) {
            result_.runs[node].instruction = node;
        }
        output_slot_.assign(program_.size(), 0);
        for (const IRInstruction& instruction : program_) {
            if (instruction.opcode == IROpcode::Output) {
                output_slot_[instruction.id] = result_.outputs.size();
                result_.outputs.push_back({instruction.dataset, instruction.line, {}, {}});
            }
        }
    }

    ExecutionResult run() {
        const std::size_t count = program_.size();
        const unsigned workers = static_cast<unsigned>(
            std::max<std::size_t>(1, std::min<std::size_t>(options_.jobs, std::max<std::size_t>(count, 1)))
        );
        result_.workers = workers;
        start_ = Clock::now();

        if (workers == 1) {
            // Instruction ids are a topological order: edges point forward.
            for (std::size_t node = 0; node < count; ++node) {
                if (!run_node(node, 0)) break;
            }
        } else {
            run_parallel(workers);
        }

        result_.total_ms = milliseconds_since(start_);
        std::sort(result_.diagnostics.begin(), result_.diagnostics.end(),
            [](const Diagnostic& a, const Diagnostic& b) { return a.line < b.line; });
        return std::move(result_);
    }

private:
    // Returns false when the node failed.
    bool run_node(std::size_t node, unsigned worker) {
        const IRInstruction& instruction = program_[node];
        DatasetState& state = states_[dataset_index_.at(instruction.dataset)];
        NodeRun& record = result_.runs[node];
        record.instruction = node;
        record.worker = worker;
        record.start_ms = milliseconds_since(start_);
        // An input node's "rows in" are the rows read from its file.
        record.rows_in = instruction.opcode == IROpcode::Input
            ? loaded_[dataset_index_.at(instruction.dataset)].rows.size()
            : state.table.rows.size();

        std::optional<std::string> failure;
        try {
            execute_instruction(instruction, state);
        } catch (const std::exception& error) {
            failure = error.what();
        }

        record.rows_out = instruction.opcode == IROpcode::Output
            ? result_.outputs[output_slot_[node]].table.rows.size()
            : state.table.rows.size();
        record.end_ms = milliseconds_since(start_);

        if (failure) {
            std::lock_guard<std::mutex> lock(diagnostics_mutex_);
            result_.diagnostics.push_back({DiagnosticKind::Runtime, instruction.line, *failure});
            return false;
        }
        return true;
    }

    void execute_instruction(const IRInstruction& instruction, DatasetState& state) {
        switch (instruction.opcode) {
            case IROpcode::Input:
                state.table = std::move(loaded_[dataset_index_.at(instruction.dataset)]);
                state.group_key.clear();
                break;
            case IROpcode::Clean:
                run_clean(state.table);
                break;
            case IROpcode::Filter:
                run_filter(state.table, *instruction.condition);
                break;
            case IROpcode::Transform:
                run_transform(state.table, instruction.function);
                break;
            case IROpcode::Sort:
                run_sort(state.table, instruction.field,
                         instruction.order == SortOrder::Descending);
                break;
            case IROpcode::Group:
                run_sort(state.table, instruction.field, false);
                state.group_key = instruction.field;
                break;
            case IROpcode::Aggregate:
                run_aggregate(state, instruction.field, instruction.function);
                break;
            case IROpcode::Output: {
                // Output is the dataset's last operation, so its table can move.
                DatasetOutput& output = result_.outputs[output_slot_[instruction.id]];
                output.table = std::move(state.table);
                if (!options_.out_dir.empty()) {
                    output.file = options_.out_dir + "/" + instruction.dataset + ".csv";
                    write_csv_file(output.table, output.file);
                }
                break;
            }
        }
    }

    // Dependency-driven thread pool: a node becomes ready when the node it
    // depends on finishes. Nodes that touch the same dataset are always
    // linked by an edge, so no two running nodes share a table.
    void run_parallel(unsigned workers) {
        const std::size_t count = program_.size();
        std::vector<std::vector<std::size_t>> successors(count);
        std::vector<std::size_t> waiting(count, 0);
        for (const DependencyEdge& edge : dag_.edges()) {
            successors[edge.from].push_back(edge.to);
            ++waiting[edge.to];
        }

        std::mutex mutex;
        std::condition_variable wake;
        std::deque<std::size_t> ready;
        std::size_t remaining = count;
        bool failed = false;
        for (std::size_t node = 0; node < count; ++node) {
            if (waiting[node] == 0) ready.push_back(node);
        }

        auto work = [&](unsigned worker) {
            std::unique_lock<std::mutex> lock(mutex);
            while (true) {
                wake.wait(lock, [&] { return failed || remaining == 0 || !ready.empty(); });
                if (failed || remaining == 0) {
                    return;
                }
                const std::size_t node = ready.front();
                ready.pop_front();
                lock.unlock();
                const bool ok = run_node(node, worker);
                lock.lock();
                if (!ok) {
                    failed = true;
                } else {
                    --remaining;
                    for (const std::size_t next : successors[node]) {
                        if (--waiting[next] == 0) ready.push_back(next);
                    }
                }
                wake.notify_all();
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(workers);
        for (unsigned worker = 0; worker < workers; ++worker) {
            threads.emplace_back(work, worker);
        }
        for (std::thread& thread : threads) {
            thread.join();
        }
    }

    const IRProgram& program_;
    const PipelineDag& dag_;
    const ExecutionOptions& options_;
    std::unordered_map<std::string, std::size_t> dataset_index_;
    std::vector<DatasetState> states_;
    std::vector<Table> loaded_;
    std::vector<std::size_t> output_slot_;
    std::mutex diagnostics_mutex_;
    ExecutionResult result_;
    Clock::time_point start_;
};

std::string format_ms(double value) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(3) << value;
    return text.str();
}

}  // namespace

LoadedInputs load_inputs(const IRProgram& program, const std::string& data_dir, unsigned jobs) {
    LoadedInputs loaded;
    struct Request {
        std::string dataset;
        int line;
        std::string file;
    };
    std::vector<Request> requests;
    for (const IRInstruction& instruction : program) {
        if (instruction.opcode == IROpcode::Input) {
            const std::string directory = data_dir.empty() ? "." : data_dir;
            requests.push_back({instruction.dataset, instruction.line,
                                directory + "/" + instruction.dataset + ".csv"});
        }
    }

    const Clock::time_point start = Clock::now();
    std::vector<std::optional<Table>> tables(requests.size());
    std::vector<std::string> errors(requests.size());
    auto load = [&](std::size_t index) {
        try {
            tables[index] = read_csv_file(requests[index].file);
        } catch (const std::exception& error) {
            errors[index] = error.what();
        }
    };
    if (jobs > 1 && requests.size() > 1) {
        std::vector<std::future<void>> pending;
        for (std::size_t index = 0; index < requests.size(); ++index) {
            pending.push_back(std::async(std::launch::async, load, index));
        }
        for (std::future<void>& future : pending) future.get();
    } else {
        for (std::size_t index = 0; index < requests.size(); ++index) load(index);
    }
    loaded.load_ms = milliseconds_since(start);

    for (std::size_t index = 0; index < requests.size(); ++index) {
        const Request& request = requests[index];
        InputSummary& summary = loaded.summaries[request.dataset];
        summary.file = request.file;
        if (tables[index]) {
            summary.rows = tables[index]->rows.size();
            summary.columns = tables[index]->columns;
            summary.loaded = true;
            loaded.tables[request.dataset] = std::move(*tables[index]);
        } else {
            loaded.diagnostics.push_back({
                DiagnosticKind::Runtime,
                request.line,
                "cannot load dataset '" + request.dataset + "': " + errors[index]
            });
        }
    }
    return loaded;
}

ExecutionResult execute(
    const IRProgram& program,
    const PipelineDag& dag,
    std::map<std::string, Table> inputs,
    const ExecutionOptions& options
) {
    return Runner(program, dag, std::move(inputs), options).run();
}

void print_outputs(const ExecutionResult& result, std::ostream& output) {
    for (std::size_t index = 0; index < result.outputs.size(); ++index) {
        const DatasetOutput& dataset = result.outputs[index];
        if (index > 0) output << '\n';
        output << "Output " << dataset.dataset << " (" << dataset.table.rows.size()
               << (dataset.table.rows.size() == 1 ? " row" : " rows") << ")";
        if (!dataset.file.empty()) {
            output << " -> " << dataset.file;
        }
        output << '\n';
        write_csv(dataset.table, output);
    }
}

void print_timings(
    const ExecutionResult& result,
    const IRProgram& program,
    double load_ms,
    std::ostream& output
) {
    output << "Execution timings\n"
           << "Workers: " << result.workers << '\n'
           << "Load inputs: " << format_ms(load_ms) << " ms\n"
           << "Run pipeline: " << format_ms(result.total_ms) << " ms\n"
           << std::left << std::setw(6) << "Node" << std::setw(11) << "Operation"
           << std::setw(14) << "Dataset" << std::setw(20) << "Rows in -> out"
           << std::setw(12) << "Start ms" << std::setw(12) << "Time ms" << "Worker\n";
    for (const NodeRun& run : result.runs) {
        const IRInstruction& instruction = program[run.instruction];
        output << std::left << std::setw(6) << ("[" + std::to_string(run.instruction) + "]")
               << std::setw(11) << ir_opcode_name(instruction.opcode)
               << std::setw(14) << instruction.dataset
               << std::setw(20) << (std::to_string(run.rows_in) + " -> " +
                                    std::to_string(run.rows_out))
               << std::setw(12) << format_ms(run.start_ms)
               << std::setw(12) << format_ms(run.end_ms - run.start_ms)
               << run.worker << '\n';
    }
}

}  // namespace pipelang
