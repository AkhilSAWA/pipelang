#pragma once
// ============================================================
// PipeLang AST — minimal node definitions for Phase 2 scaffold
// ============================================================
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <ostream>

namespace pipelang {

// ---------- Operation kinds ----------
enum class Op {
    Input, Clean, Filter, Transform,
    Sort,  Group, Aggregate, Output
};

const char* opName(Op o);

// ---------- Expressions (used by filter WHERE …) ----------
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Expr {
    enum class Kind { Ident, Int, Float, String, Binary };
    Kind kind;
    std::string sval;        // Ident, String, or operator symbol for Binary
    long long   ival = 0;
    double      fval = 0.0;
    ExprPtr     lhs, rhs;    // Binary

    static ExprPtr ident(std::string s);
    static ExprPtr intLit(long long v);
    static ExprPtr floatLit(double v);
    static ExprPtr strLit(std::string s);
    static ExprPtr bin(std::string op, ExprPtr l, ExprPtr r);

    void print(std::ostream& os) const;
};

// ---------- Statements ----------
struct Stmt {
    Op          op;
    std::string dataset;     // e.g. "students"
    std::string argument;    // second identifier (transform fn, sort/group key)
    ExprPtr     predicate;   // for FILTER … WHERE …

    Stmt(Op o, std::string ds) : op(o), dataset(std::move(ds)) {}
    void print(std::ostream& os, int indent = 0) const;
};
using StmtPtr = std::unique_ptr<Stmt>;

// ---------- Pipeline ----------
struct Pipeline {
    std::string          name;
    std::vector<StmtPtr> stmts;

    Pipeline(std::string n, std::vector<StmtPtr> s)
        : name(std::move(n)), stmts(std::move(s)) {}

    void print(std::ostream& os) const;
};
using PipelinePtr = std::unique_ptr<Pipeline>;

} // namespace pipelang
