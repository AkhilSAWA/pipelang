#include "pipelang/optimizer.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>

#include "pipelang/functions.hpp"
#include "pipelang/value.hpp"

namespace pipelang {
namespace {

// The operations of one dataset, in execution order. Dependencies only link
// operations on the same dataset, so each pass works on one chain at a time.
using Chain = std::vector<IRInstruction>;

const char* operation_name(IROpcode opcode) {
    switch (opcode) {
        case IROpcode::Input: return "input";
        case IROpcode::Clean: return "clean";
        case IROpcode::Filter: return "filter";
        case IROpcode::Transform: return "transform";
        case IROpcode::Sort: return "sort";
        case IROpcode::Group: return "group";
        case IROpcode::Aggregate: return "aggregate";
        case IROpcode::Output: return "output";
    }
    return "operation";
}

std::string describe(const IRInstruction& instruction) {
    return std::string(operation_name(instruction.opcode)) + " (line " +
           std::to_string(instruction.line) + ")";
}

// ---------------------------------------------------------------- folding

bool is_number_literal(const Expr& expression) {
    return expression.kind == ExprKind::Integer || expression.kind == ExprKind::Float;
}

double literal_number(const Expr& expression) {
    return std::strtod(expression.value.c_str(), nullptr);
}

// The literal reads back as exactly the same double at run time.
ExprPtr number_literal(double value) {
    const std::string text = format_number(value);
    const bool integral = text.find_first_not_of("-0123456789") == std::string::npos;
    return integral ? make_integer(text) : make_float(text);
}

bool is_boolean(const Expr& expression, bool value) {
    return expression.kind == ExprKind::Boolean &&
           expression.value == (value ? "true" : "false");
}

bool compare_literals(const std::string& op, int order) {
    if (op == "==") return order == 0;
    if (op == "!=") return order != 0;
    if (op == "<") return order < 0;
    if (op == ">") return order > 0;
    if (op == "<=") return order <= 0;
    return order >= 0;  // ">="
}

// Folds constant subexpressions. The run-time evaluator computes in doubles
// and treats `x / 0` as null, so division by zero is never folded. Removing
// one side of `and`/`or` is safe because conditions have no side effects
// and never fail.
ExprPtr fold(const Expr& expression) {
    if (expression.kind == ExprKind::Unary) {
        ExprPtr operand = fold(*expression.left);
        if (is_number_literal(*operand)) {
            return number_literal(-literal_number(*operand));
        }
        return make_unary(expression.value, std::move(operand));
    }
    if (expression.kind != ExprKind::Binary) {
        return clone_expr(expression);
    }

    ExprPtr left = fold(*expression.left);
    ExprPtr right = fold(*expression.right);
    const std::string& op = expression.value;

    if (op == "and" || op == "or") {
        const bool is_and = op == "and";
        // and: true is neutral, false absorbs. or: false is neutral, true absorbs.
        if (is_boolean(*left, !is_and)) return make_boolean(!is_and);
        if (is_boolean(*right, !is_and)) return make_boolean(!is_and);
        if (is_boolean(*left, is_and)) return right;
        if (is_boolean(*right, is_and)) return left;
        return make_binary(op, std::move(left), std::move(right));
    }

    if (is_number_literal(*left) && is_number_literal(*right)) {
        const double a = literal_number(*left);
        const double b = literal_number(*right);
        if (op == "+") return number_literal(a + b);
        if (op == "-") return number_literal(a - b);
        if (op == "*") return number_literal(a * b);
        if (op == "/") {
            if (b != 0.0) return number_literal(a / b);
        } else {
            return make_boolean(compare_literals(op, a < b ? -1 : (a > b ? 1 : 0)));
        }
    }

    if (left->kind == ExprKind::String && right->kind == ExprKind::String) {
        const int order = left->value.compare(right->value);
        return make_boolean(compare_literals(op, order < 0 ? -1 : (order > 0 ? 1 : 0)));
    }

    return make_binary(op, std::move(left), std::move(right));
}

void collect_fields(const Expr& expression, std::set<std::string>& fields) {
    if (expression.kind == ExprKind::Identifier) {
        fields.insert(expression.value);
    }
    if (expression.left) collect_fields(*expression.left, fields);
    if (expression.right) collect_fields(*expression.right, fields);
}

// ------------------------------------------------------------------ passes

class Optimizer {
public:
    explicit Optimizer(const IRProgram& program) {
        for (IRInstruction& instruction : clone_program(program)) {
            if (chains_.find(instruction.dataset) == chains_.end()) {
                dataset_order_.push_back(instruction.dataset);
            }
            chains_[instruction.dataset].push_back(std::move(instruction));
        }
    }

    OptimizationResult run() {
        fold_constants();
        remove_dead_code();
        // Each pass can expose work for another; repeat until nothing changes.
        // Every change removes an instruction or moves a filter earlier, so this ends.
        bool changed = true;
        while (changed) {
            changed = false;
            changed |= push_filters_down();
            changed |= merge_filters();
            changed |= remove_redundant_cleans();
            changed |= remove_redundant_sorts();
        }
        return {assemble(), std::move(steps_)};
    }

private:
    void log(const char* pass, int line, std::string message) {
        steps_.push_back({pass, line, std::move(message)});
    }

    void fold_constants() {
        for (const std::string& dataset : dataset_order_) {
            for (IRInstruction& instruction : chains_[dataset]) {
                if (instruction.opcode != IROpcode::Filter) {
                    continue;
                }
                ExprPtr folded = fold(*instruction.condition);
                const std::string before = expr_to_string(*instruction.condition);
                const std::string after = expr_to_string(*folded);
                if (before != after) {
                    log("constant-folding", instruction.line,
                        "`" + before + "` -> `" + after + "`");
                    instruction.condition = std::move(folded);
                }
            }
        }
    }

    void remove_dead_code() {
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            const bool output = std::any_of(chain.begin(), chain.end(), [](const auto& i) {
                return i.opcode == IROpcode::Output;
            });
            if (!output) {
                log("dead-code", chain.front().line,
                    "removed all " + std::to_string(chain.size()) +
                    " operations on dataset '" + dataset + "', which is never output");
                chain.clear();
                continue;
            }
            for (std::size_t index = 0; index < chain.size();) {
                const IRInstruction& instruction = chain[index];
                if (instruction.opcode == IROpcode::Filter &&
                    is_boolean(*instruction.condition, true)) {
                    log("dead-code", instruction.line,
                        "removed filter on '" + dataset + "' whose condition is always true");
                    chain.erase(chain.begin() + static_cast<std::ptrdiff_t>(index));
                } else {
                    if (instruction.opcode == IROpcode::Filter &&
                        is_boolean(*instruction.condition, false)) {
                        log("dead-code", instruction.line,
                            "note: filter on '" + dataset +
                            "' is always false, so the dataset will be empty");
                    }
                    ++index;
                }
            }
        }
    }

    // A filter can run before an operation when both orders give the same
    // rows in the same order:
    //   clean      removes null rows and duplicate rows; the filter keeps or
    //              drops identical rows alike, so the result is the same
    //   sort/group stable reorderings; filtering keeps the relative order
    //   transform  only if it works row by row, ignores row order and does
    //              not write a field the filter reads
    static bool filter_can_move_before(
        const IRInstruction& operation,
        const std::set<std::string>& fields
    ) {
        switch (operation.opcode) {
            case IROpcode::Clean:
            case IROpcode::Sort:
            case IROpcode::Group:
                return true;
            case IROpcode::Transform: {
                const TransformInfo* function = find_transform(operation.function);
                return function != nullptr && !function->order_sensitive &&
                       !function->modifies_string_columns &&
                       (function->added_column.empty() ||
                        fields.count(function->added_column) == 0);
            }
            default:
                return false;
        }
    }

    bool push_filters_down() {
        bool changed = false;
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            for (std::size_t index = 1; index < chain.size(); ++index) {
                if (chain[index].opcode != IROpcode::Filter) {
                    continue;
                }
                std::set<std::string> fields;
                collect_fields(*chain[index].condition, fields);
                std::string passed;
                std::size_t position = index;
                while (position > 0 &&
                       filter_can_move_before(chain[position - 1], fields)) {
                    passed += (passed.empty() ? "" : ", ") + describe(chain[position - 1]);
                    std::swap(chain[position - 1], chain[position]);
                    --position;
                }
                if (position != index) {
                    log("filter-pushdown", chain[position].line,
                        "moved filter on '" + dataset + "' before " + passed +
                        " so later operations see fewer rows");
                    changed = true;
                }
            }
        }
        return changed;
    }

    // filter a; filter b keeps exactly the rows where both hold, and `and`
    // does not evaluate b where a is false, matching the original order.
    bool merge_filters() {
        bool changed = false;
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            for (std::size_t index = 0; index + 1 < chain.size();) {
                if (chain[index].opcode == IROpcode::Filter &&
                    chain[index + 1].opcode == IROpcode::Filter) {
                    log("filter-merge", chain[index + 1].line,
                        "merged filter (line " + std::to_string(chain[index + 1].line) +
                        ") into filter (line " + std::to_string(chain[index].line) +
                        ") on '" + dataset + "'");
                    // Folding again simplifies e.g. `... and false`.
                    chain[index].condition = fold(*make_binary(
                        "and",
                        std::move(chain[index].condition),
                        std::move(chain[index + 1].condition)
                    ));
                    chain.erase(chain.begin() + static_cast<std::ptrdiff_t>(index) + 1);
                    changed = true;
                } else {
                    ++index;
                }
            }
        }
        return changed;
    }

    // After a clean there are no null cells and no duplicate rows. Filter,
    // sort and group only drop or reorder rows, so they keep that true and a
    // second clean would change nothing.
    bool remove_redundant_cleans() {
        bool changed = false;
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            bool rows_clean = false;
            int last_clean_line = 0;
            for (std::size_t index = 0; index < chain.size();) {
                const IROpcode opcode = chain[index].opcode;
                if (opcode == IROpcode::Clean && rows_clean) {
                    log("redundant-clean", chain[index].line,
                        "removed clean on '" + dataset + "'; the rows are already "
                        "clean since line " + std::to_string(last_clean_line));
                    chain.erase(chain.begin() + static_cast<std::ptrdiff_t>(index));
                    changed = true;
                    continue;
                }
                if (opcode == IROpcode::Clean) {
                    rows_clean = true;
                    last_clean_line = chain[index].line;
                } else if (opcode != IROpcode::Filter && opcode != IROpcode::Sort &&
                           opcode != IROpcode::Group) {
                    rows_clean = false;
                }
                ++index;
            }
        }
        return changed;
    }

    // A stable sort on field f only changes the relative order of rows whose
    // f values differ. The sort is unnecessary when, before anything observes
    // row order (output or an order-sensitive transform):
    //   - another sort or a group on f decides that order again, or
    //   - an aggregate runs, whose result does not depend on row order.
    // Operations in between (filter, clean, other transforms, sorts and
    // groups on other fields) keep the relative order of rows they keep.
    static std::string sort_made_redundant_by(const Chain& chain, std::size_t sort_index) {
        const std::string& field = chain[sort_index].field;
        for (std::size_t index = sort_index + 1; index < chain.size(); ++index) {
            const IRInstruction& next = chain[index];
            switch (next.opcode) {
                case IROpcode::Sort:
                case IROpcode::Group:
                    if (next.field == field) {
                        return describe(next) + " orders by '" + field + "' again";
                    }
                    break;
                case IROpcode::Aggregate:
                    return describe(next) + " does not depend on row order";
                case IROpcode::Output:
                    return "";
                case IROpcode::Transform: {
                    const TransformInfo* function = find_transform(next.function);
                    if (function == nullptr || function->order_sensitive) {
                        return "";
                    }
                    break;
                }
                case IROpcode::Input:
                case IROpcode::Clean:
                case IROpcode::Filter:
                    break;
            }
        }
        return "";
    }

    bool remove_redundant_sorts() {
        bool changed = false;
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            for (std::size_t index = 0; index < chain.size();) {
                if (chain[index].opcode == IROpcode::Sort) {
                    const std::string reason = sort_made_redundant_by(chain, index);
                    if (!reason.empty()) {
                        log("redundant-sort", chain[index].line,
                            "removed sort of '" + dataset + "' by '" +
                            chain[index].field + "': " + reason);
                        chain.erase(chain.begin() + static_cast<std::ptrdiff_t>(index));
                        changed = true;
                        continue;
                    }
                }
                ++index;
            }
        }
        return changed;
    }

    // Interleaves the chains back into one program. Each chain keeps its
    // order; its k-th instruction takes the k-th smallest original position
    // still used by that chain, so the layout stays close to the source.
    IRProgram assemble() {
        std::vector<std::pair<std::size_t, IRInstruction>> placed;
        for (const std::string& dataset : dataset_order_) {
            Chain& chain = chains_[dataset];
            std::vector<std::size_t> slots;
            slots.reserve(chain.size());
            for (const IRInstruction& instruction : chain) {
                slots.push_back(instruction.id);
            }
            std::sort(slots.begin(), slots.end());
            for (std::size_t index = 0; index < chain.size(); ++index) {
                placed.emplace_back(slots[index], std::move(chain[index]));
            }
        }
        std::sort(placed.begin(), placed.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        IRProgram program;
        program.reserve(placed.size());
        for (auto& entry : placed) {
            entry.second.id = program.size();
            program.push_back(std::move(entry.second));
        }
        return program;
    }

    std::vector<std::string> dataset_order_;
    std::map<std::string, Chain> chains_;
    std::vector<OptimizationStep> steps_;
};

}  // namespace

OptimizationResult optimize(const IRProgram& program) {
    return Optimizer(program).run();
}

void print_optimization(
    const OptimizationResult& result,
    std::size_t original_size,
    std::ostream& output
) {
    output << "Optimization\n";
    if (result.steps.empty()) {
        output << "No optimizations applied.\n";
    }
    for (const OptimizationStep& step : result.steps) {
        output << '[' << step.pass << "] line " << step.line << ": "
               << step.message << '\n';
    }
    output << "Instructions: " << original_size << " -> "
           << result.program.size() << '\n';
    output << "Optimized ";
    print_ir(result.program, output);
}

}  // namespace pipelang
