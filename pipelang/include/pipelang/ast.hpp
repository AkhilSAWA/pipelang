#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace pipelang {

enum class ExprKind {
    Identifier,
    Integer,
    Float,
    String,
    Boolean,  // "true"/"false"; produced only by constant folding
    Unary,
    Binary
};

enum class StmtKind {
    Input,
    Clean,
    Filter,
    Transform,
    Sort,
    Group,
    Aggregate,
    Output
};

enum class SortOrder {
    Ascending,
    Descending
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// Literals and identifiers store their text in `value`. Operators store the
// operator in `value`: unary nodes use `left` only, binary nodes use both.
struct Expr {
    ExprKind kind;
    std::string value;
    ExprPtr left;
    ExprPtr right;

    Expr(ExprKind kind, std::string value);
    Expr(ExprKind kind, std::string value, ExprPtr left, ExprPtr right);
};

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

// One pipeline operation. Which optional members are set depends on `kind`:
//   Filter    -> condition
//   Transform -> function
//   Sort      -> field, order
//   Group     -> field
//   Aggregate -> field, function
struct Stmt {
    StmtKind kind;
    std::string dataset;
    std::string field;
    std::string function;
    SortOrder order = SortOrder::Ascending;
    ExprPtr condition;
    int line = 0;  // source line of the statement keyword

    Stmt(StmtKind kind, std::string dataset, int line);
};

struct Pipeline {
    std::string name;
    std::vector<StmtPtr> statements;
    int line = 0;  // source line of the `pipeline` keyword
};

using PipelinePtr = std::unique_ptr<Pipeline>;

ExprPtr make_identifier(std::string value);
ExprPtr make_integer(std::string value);
ExprPtr make_float(std::string value);
ExprPtr make_string(std::string value);
ExprPtr make_boolean(bool value);
ExprPtr make_unary(std::string op, ExprPtr operand);
ExprPtr make_binary(std::string op, ExprPtr left, ExprPtr right);
ExprPtr clone_expr(const Expr& expression);

// Renders an expression on one line, e.g. `((a + (b * 2)) > 10)`.
// Binary expressions are fully parenthesised; strings keep their quotes.
std::string expr_to_string(const Expr& expression);

StmtPtr make_input(std::string dataset, int line);
StmtPtr make_clean(std::string dataset, int line);
StmtPtr make_filter(std::string dataset, ExprPtr condition, int line);
StmtPtr make_transform(std::string dataset, std::string function, int line);
StmtPtr make_sort(std::string dataset, std::string field, SortOrder order, int line);
StmtPtr make_group(std::string dataset, std::string field, int line);
StmtPtr make_aggregate(
    std::string dataset,
    std::string field,
    std::string function,
    int line
);
StmtPtr make_output(std::string dataset, int line);

PipelinePtr make_pipeline(
    std::string name,
    std::vector<StmtPtr> statements,
    int line
);

const char* sort_order_name(SortOrder order);
void print_ast(const Pipeline& pipeline, std::ostream& output);

}  // namespace pipelang
