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

Stmt::Stmt(StmtKind kind, std::string dataset)
    : kind(kind), dataset(std::move(dataset)) {}

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

ExprPtr make_binary(std::string op, ExprPtr left, ExprPtr right) {
    return std::make_unique<Expr>(
        ExprKind::Binary,
        std::move(op),
        std::move(left),
        std::move(right)
    );
}

StmtPtr make_input(std::string dataset) {
    return std::make_unique<Stmt>(StmtKind::Input, std::move(dataset));
}

StmtPtr make_clean(std::string dataset) {
    return std::make_unique<Stmt>(StmtKind::Clean, std::move(dataset));
}

StmtPtr make_filter(std::string dataset, ExprPtr condition) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Filter,
        std::move(dataset)
    );
    statement->expression = std::move(condition);
    return statement;
}

StmtPtr make_transform(std::string dataset, std::string function) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Transform,
        std::move(dataset)
    );
    statement->argument = std::move(function);
    return statement;
}

StmtPtr make_sort(
    std::string dataset,
    std::string field,
    SortOrder order
) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Sort,
        std::move(dataset)
    );
    statement->argument = std::move(field);
    statement->order = order;
    return statement;
}

StmtPtr make_group(std::string dataset, std::string field) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Group,
        std::move(dataset)
    );
    statement->argument = std::move(field);
    return statement;
}

StmtPtr make_aggregate(
    std::string dataset,
    std::string field,
    std::string function
) {
    auto statement = std::make_unique<Stmt>(
        StmtKind::Aggregate,
        std::move(dataset)
    );
    statement->argument = std::move(field) + " using " + std::move(function);
    return statement;
}

StmtPtr make_output(std::string dataset) {
    return std::make_unique<Stmt>(StmtKind::Output, std::move(dataset));
}

PipelinePtr make_pipeline(
    std::string name,
    std::vector<StmtPtr> statements
) {
    auto pipeline = std::make_unique<Pipeline>();
    pipeline->name = std::move(name);
    pipeline->statements = std::move(statements);
    return pipeline;
}

namespace {

const char* expr_kind_name(ExprKind kind) {
    switch (kind) {
        case ExprKind::Identifier: return "Identifier";
        case ExprKind::Integer: return "Integer";
        case ExprKind::Float: return "Float";
        case ExprKind::String: return "String";
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

    output << padding
           << expr_kind_name(expression.kind)
           << ": "
           << expression.value
           << '\n';

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

        if (!statement->argument.empty()) {
            output << " argument=" << statement->argument;
        }

        if (statement->kind == StmtKind::Sort) {
            output << " order="
                   << (statement->order == SortOrder::Ascending
                       ? "ascending"
                       : "descending");
        }

        output << '\n';

        if (statement->expression) {
            print_expr(*statement->expression, output, 4);
        }
    }
}

}  // namespace pipelang