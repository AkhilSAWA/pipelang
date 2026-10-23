#include "pipelang/semantic.hpp"

#include <initializer_list>
#include <optional>
#include <string_view>
#include <utility>

#include "pipelang/functions.hpp"

namespace pipelang {
namespace {

// The static type of an expression. Without a schema a field reference
// (`Field`) may hold a number or a string and is compatible with both; it is
// never a boolean.
enum class ValueType {
    Field,
    Number,
    String,
    Boolean
};

const char* value_type_name(ValueType type) {
    switch (type) {
        case ValueType::Field: return "field";
        case ValueType::Number: return "number";
        case ValueType::String: return "string";
        case ValueType::Boolean: return "comparison";
    }
    return "value";
}

bool is_numeric(ValueType type) {
    return type == ValueType::Number || type == ValueType::Field;
}

bool is_literal_zero(const Expr& expression) {
    if (expression.kind != ExprKind::Integer && expression.kind != ExprKind::Float) {
        return false;
    }
    // Source literals are unsigned digit strings, so zero means only '0' and '.'.
    return expression.value.find_first_not_of("0.") == std::string::npos;
}

bool is_one_of(const std::string& op, std::initializer_list<std::string_view> ops) {
    for (const std::string_view candidate : ops) {
        if (op == candidate) {
            return true;
        }
    }
    return false;
}

// Type-checks the condition of one filter statement. Each error is reported
// once: a subexpression that already failed yields no type, and its parents
// stay silent instead of reporting follow-on errors.
class ConditionChecker {
public:
    ConditionChecker(
        std::vector<Diagnostic>& diagnostics,
        int line,
        const std::string& dataset,
        const Schema* schema
    )
        : diagnostics_(diagnostics), line_(line), dataset_(dataset), schema_(schema) {}

    void check_condition(const Expr& condition) {
        const std::optional<ValueType> type = check(condition);
        if (type && *type != ValueType::Boolean) {
            error(
                "filter condition must be a comparison such as `marks > 75`, "
                "but `" + expr_to_string(condition) + "` is a " +
                value_type_name(*type)
            );
        }
    }

private:
    std::optional<ValueType> check(const Expr& expression) {
        switch (expression.kind) {
            case ExprKind::Identifier: return check_field(expression.value);
            case ExprKind::Integer:
            case ExprKind::Float: return ValueType::Number;
            case ExprKind::String: return ValueType::String;
            case ExprKind::Boolean: return ValueType::Boolean;
            case ExprKind::Unary: return check_unary(expression);
            case ExprKind::Binary: return check_binary(expression);
        }
        return std::nullopt;
    }

    std::optional<ValueType> check_field(const std::string& name) {
        if (schema_ == nullptr) {
            return ValueType::Field;
        }
        const std::optional<std::size_t> column = find_column(*schema_, name);
        if (!column) {
            error(
                "field '" + name + "' is not a column of dataset '" + dataset_ +
                "' (columns: " + column_list(*schema_) + ")"
            );
            return std::nullopt;
        }
        return (*schema_)[*column].type == ColumnType::Number
            ? ValueType::Number
            : ValueType::String;
    }

    std::optional<ValueType> check_unary(const Expr& expression) {
        const std::optional<ValueType> operand = check(*expression.left);
        if (!operand) {
            return std::nullopt;
        }
        if (!is_numeric(*operand)) {
            error(
                "unary '" + expression.value + "' needs a numeric operand, got a " +
                value_type_name(*operand)
            );
            return std::nullopt;
        }
        return ValueType::Number;
    }

    std::optional<ValueType> check_binary(const Expr& expression) {
        const std::optional<ValueType> left = check(*expression.left);
        const std::optional<ValueType> right = check(*expression.right);
        if (!left || !right) {
            return std::nullopt;
        }

        const std::string& op = expression.value;
        const std::string operands = std::string(value_type_name(*left)) +
            " and " + value_type_name(*right);

        if (is_one_of(op, {"and", "or"})) {
            if (*left != ValueType::Boolean || *right != ValueType::Boolean) {
                error("'" + op + "' combines two comparisons, got " + operands);
                return std::nullopt;
            }
            return ValueType::Boolean;
        }

        if (is_one_of(op, {"+", "-", "*", "/"})) {
            if (!is_numeric(*left) || !is_numeric(*right)) {
                error(
                    "arithmetic operator '" + op + "' needs numeric operands, got " +
                    operands
                );
                return std::nullopt;
            }
            if (op == "/" && is_literal_zero(*expression.right)) {
                error("division by zero in `" + expr_to_string(expression) + "`");
                return std::nullopt;
            }
            return ValueType::Number;
        }

        // Comparison operators: ==, !=, <, >, <=, >=.
        if (*left == ValueType::Boolean || *right == ValueType::Boolean) {
            error(
                "cannot compare the result of a comparison with '" + op +
                "'; combine conditions with 'and' or 'or'"
            );
            return std::nullopt;
        }
        const bool compatible = *left == ValueType::Field ||
            *right == ValueType::Field || *left == *right;
        if (!compatible) {
            error("cannot compare " + operands + " with '" + op + "'");
            return std::nullopt;
        }
        return ValueType::Boolean;
    }

    void error(std::string message) {
        diagnostics_.push_back({DiagnosticKind::Semantic, line_, std::move(message)});
    }

    std::vector<Diagnostic>& diagnostics_;
    int line_;
    const std::string& dataset_;
    const Schema* schema_;
};

}  // namespace

void check_condition(
    const Expr& condition,
    int line,
    const std::string& dataset,
    const Schema* schema,
    std::vector<Diagnostic>& diagnostics
) {
    ConditionChecker(diagnostics, line, dataset, schema).check_condition(condition);
}

SemanticResult analyze_semantics(const Pipeline& pipeline) {
    SemanticResult result;
    auto report = [&result](DiagnosticKind kind, int line, std::string message) {
        result.diagnostics.push_back({kind, line, std::move(message)});
    };

    for (const StmtPtr& pointer : pipeline.statements) {
        const Stmt& statement = *pointer;
        const std::string quoted = "'" + statement.dataset + "'";

        if (statement.kind == StmtKind::Input) {
            const Symbol* existing = result.symbols.find(statement.dataset);
            if (existing != nullptr) {
                report(
                    DiagnosticKind::Semantic,
                    statement.line,
                    "duplicate input declaration for dataset " + quoted +
                        "; first declared at line " +
                        std::to_string(existing->declaration_line)
                );
            } else {
                result.symbols.declare(statement.dataset, statement.line);
            }
            continue;
        }

        Symbol* symbol = result.symbols.find(statement.dataset);
        if (symbol == nullptr) {
            report(
                DiagnosticKind::Semantic,
                statement.line,
                "dataset " + quoted + " is used by an operation before it is "
                    "declared with input"
            );
        } else if (symbol->output_line > 0) {
            const std::string first_output = std::to_string(symbol->output_line);
            report(
                DiagnosticKind::Semantic,
                statement.line,
                statement.kind == StmtKind::Output
                    ? "dataset " + quoted + " is output more than once; first "
                          "output at line " + first_output
                    : "dataset " + quoted + " is used after its output at line " +
                          first_output
            );
        } else if (statement.kind == StmtKind::Output) {
            symbol->output_line = statement.line;
        }

        if (statement.kind == StmtKind::Filter && statement.condition) {
            check_condition(
                *statement.condition,
                statement.line,
                statement.dataset,
                nullptr,
                result.diagnostics
            );
        }

        if (statement.kind == StmtKind::Aggregate &&
            find_aggregate(statement.function) == nullptr) {
            report(
                DiagnosticKind::Semantic,
                statement.line,
                "unknown aggregate function '" + statement.function +
                    "'; expected one of: " + aggregate_function_list()
            );
        }

        if (statement.kind == StmtKind::Transform &&
            find_transform(statement.function) == nullptr) {
            report(
                DiagnosticKind::Semantic,
                statement.line,
                "unknown transform function '" + statement.function +
                    "'; expected one of: " + transform_function_list()
            );
        }
    }

    // Whole-pipeline checks. They are skipped after statement errors, which
    // would otherwise cause misleading follow-on messages.
    if (pipeline.statements.empty()) {
        report(
            DiagnosticKind::Semantic,
            pipeline.line,
            "pipeline '" + pipeline.name + "' has no statements"
        );
        return result;
    }
    if (!result.ok()) {
        return result;
    }

    bool any_output = false;
    for (const Symbol& symbol : result.symbols.symbols()) {
        any_output = any_output || symbol.output_line > 0;
    }
    if (!any_output) {
        report(
            DiagnosticKind::Semantic,
            pipeline.line,
            "pipeline '" + pipeline.name + "' has no output statement"
        );
        return result;
    }

    for (const Symbol& symbol : result.symbols.symbols()) {
        if (symbol.output_line == 0) {
            report(
                DiagnosticKind::Warning,
                symbol.declaration_line,
                "dataset '" + symbol.name + "' is never output, so its "
                    "operations have no effect"
            );
        }
    }

    return result;
}

}  // namespace pipelang
