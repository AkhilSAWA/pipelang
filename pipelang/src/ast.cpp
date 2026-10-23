#include "pipelang/ast.hpp"

#include <utility>

namespace pipelang {

Expr::Expr(ExprKind kind, std::string value)
    : kind(kind), value(std::move(value)) {}

Expr::Expr(
    ExprKind kind,
    std::string value,
    ExprPtr left,
    ExprPtr right
)
    : kind(kind),
      value(std::move(value)),
      left(std::move(left)),
      right(std::move(right)) {}

Stmt::Stmt(StmtKind kind, std::string dataset, int line)
    : kind(kind), dataset(std::move(dataset)), line(line) {}

ExprPtr make_identifier(std::string value) {
    return std::make_unique<Expr>(ExprKind::Identifier, std::move(value));
}

ExprPtr make_integer(std::string value) {
    return std::make_unique<Expr>(ExprKind::Integer, std::move(value));
}

ExprPtr make_float(std::string value) {
    return std::make_unique<Expr>(ExprKind::Float, std::move(value));
}

ExprPtr make_string(std::string value) {
    return std::make_unique<Expr>(ExprKind::String, std::move(value));
}

ExprPtr make_boolean(bool value) {
    return std::make_unique<Expr>(ExprKind::Boolean, value ? "true" : "false");
}

ExprPtr make_unary(std::string op, ExprPtr operand) {
    return std::make_unique<Expr>(
        ExprKind::Unary,
        std::move(op),
        std::move(operand),
        nullptr
    );
}

ExprPtr make_binary(std::string op, ExprPtr left, ExprPtr right) {
    return std::make_unique<Expr>(
        ExprKind::Binary,
        std::move(op),
        std::move(left),
        std::move(right)
    );
}

ExprPtr clone_expr(const Expr& expression) {
    return std::make_unique<Expr>(
        expression.kind,
        expression.value,
        expression.left ? clone_expr(*expression.left) : nullptr,
        expression.right ? clone_expr(*expression.right) : nullptr
    );
}

std::string expr_to_string(const Expr& expression) {
    switch (expression.kind) {
        case ExprKind::String:
            return '"' + expression.value + '"';
        case ExprKind::Unary:
            return expression.value + expr_to_string(*expression.left);
        case ExprKind::Binary:
            return "(" + expr_to_string(*expression.left) + " " +
                   expression.value + " " +
                   expr_to_string(*expression.right) + ")";
        case ExprKind::Identifier:
        case ExprKind::Integer:
        case ExprKind::Float:
        case ExprKind::Boolean:
            break;
    }
    return expression.value;
}

StmtPtr make_input(std::string dataset, int line) {
    return std::make_unique<Stmt>(StmtKind::Input, std::move(dataset), line);
}

StmtPtr make_clean(std::string dataset, int line) {
    return std::make_unique<Stmt>(StmtKind::Clean, std::move(dataset), line);
}

StmtPtr make_filter(std::string dataset, ExprPtr condition, int line) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Filter,
        std::move(dataset),
        line
    );
    statement->condition = std::move(condition);
    return statement;
}

StmtPtr make_transform(std::string dataset, std::string function, int line) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Transform,
        std::move(dataset),
        line
    );
    statement->function = std::move(function);
    return statement;
}

StmtPtr make_sort(
    std::string dataset,
    std::string field,
    SortOrder order,
    int line
) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Sort,
        std::move(dataset),
        line
    );
    statement->field = std::move(field);
    statement->order = order;
    return statement;
}

StmtPtr make_group(std::string dataset, std::string field, int line) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Group,
        std::move(dataset),
        line
    );
    statement->field = std::move(field);
    return statement;
}

StmtPtr make_aggregate(
    std::string dataset,
    std::string field,
    std::string function,
    int line
) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Aggregate,
        std::move(dataset),
        line
    );
    statement->field = std::move(field);
    statement->function = std::move(function);
    return statement;
}

StmtPtr make_output(std::string dataset, int line) {
    return std::make_unique<Stmt>(StmtKind::Output, std::move(dataset), line);
}

PipelinePtr make_pipeline(
    std::string name,
    std::vector<StmtPtr> statements,
    int line
) {
    auto pipeline = std::make_unique<Pipeline>();
    pipeline->name = std::move(name);
    pipeline->statements = std::move(statements);
    pipeline->line = line;
    return pipeline;
}

const char* sort_order_name(SortOrder order) {
    return order == SortOrder::Ascending ? "ascending" : "descending";
}

namespace {

const char* expr_kind_name(ExprKind kind) {
    switch (kind) {
        case ExprKind::Identifier: return "Identifier";
        case ExprKind::Integer: return "Integer";
        case ExprKind::Float: return "Float";
        case ExprKind::String: return "String";
        case ExprKind::Boolean: return "Boolean";
        case ExprKind::Unary: return "Unary";
        case ExprKind::Binary: return "Binary";
    }
    return "Unknown";
}

const char* stmt_kind_name(StmtKind kind) {
    switch (kind) {
        case StmtKind::Input: return "Input";
        case StmtKind::Clean: return "Clean";
        case StmtKind::Filter: return "Filter";
        case StmtKind::Transform: return "Transform";
        case StmtKind::Sort: return "Sort";
        case StmtKind::Group: return "Group";
        case StmtKind::Aggregate: return "Aggregate";
        case StmtKind::Output: return "Output";
    }
    return "Unknown";
}

void print_expr(const Expr& expression, std::ostream& output, int indent) {
    const std::string padding(static_cast<std::size_t>(indent), ' ');
    const std::string value = expression.kind == ExprKind::String
        ? '"' + expression.value + '"'
        : expression.value;

    output << padding << expr_kind_name(expression.kind) << ": " << value << '\n';

    if (expression.left) {
        print_expr(*expression.left, output, indent + 2);
    }

    if (expression.right) {
        print_expr(*expression.right, output, indent + 2);
    }
}

}  // namespace

void print_ast(const Pipeline& pipeline, std::ostream& output) {
    output << "Pipeline: " << pipeline.name << '\n';

    for (const auto& statement : pipeline.statements) {
        output << "  "
               << stmt_kind_name(statement->kind)
               << " dataset="
               << statement->dataset;

        if (!statement->field.empty()) {
            output << " field=" << statement->field;
        }

        if (!statement->function.empty()) {
            output << " function=" << statement->function;
        }

        if (statement->kind == StmtKind::Sort) {
            output << " order=" << sort_order_name(statement->order);
        }

        output << " line=" << statement->line << '\n';

        if (statement->condition) {
            print_expr(*statement->condition, output, 4);
        }
    }
}

}  // namespace pipelang
