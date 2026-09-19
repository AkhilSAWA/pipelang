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

struct Stmt {
    StmtKind kind;
    std::string dataset;
    std::string argument;
    SortOrder order = SortOrder::Ascending;
    ExprPtr expression;

    explicit Stmt(StmtKind kind, std::string dataset);
};

struct Pipeline {
    std::string name;
    std::vector<StmtPtr> statements;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

ExprPtr make_identifier(std::string value);
ExprPtr make_integer(std::string value);
ExprPtr make_float(std::string value);
ExprPtr make_string(std::string value);
ExprPtr make_binary(std::string op, ExprPtr left, ExprPtr right);

StmtPtr make_input(std::string dataset);
StmtPtr make_clean(std::string dataset);
StmtPtr make_filter(std::string dataset, ExprPtr condition);
StmtPtr make_transform(std::string dataset, std::string function);
StmtPtr make_sort(std::string dataset, std::string field, SortOrder order);
StmtPtr make_group(std::string dataset, std::string field);
StmtPtr make_aggregate(
    std::string dataset,
    std::string field,
    std::string function
);
StmtPtr make_output(std::string dataset);

PipelinePtr make_pipeline(
    std::string name,
    std::vector<StmtPtr> statements
);

void print_ast(const Pipeline& pipeline, std::ostream& output);

}  // namespace pipelang