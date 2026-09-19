// ============================================================
// PipeLang AST — implementation of factories and pretty-printer
// ============================================================
#include "pipelang/ast.hpp"

namespace pipelang {

const char* opName(Op o) {
    switch (o) {
        case Op::Input:     return "input";
        case Op::Clean:     return "clean";
        case Op::Filter:    return "filter";
        case Op::Transform: return "transform";
        case Op::Sort:      return "sort";
        case Op::Group:     return "group";
        case Op::Aggregate: return "aggregate";
        case Op::Output:    return "output";
    }
    return "?";
}

// ---------- Expr factories ----------
ExprPtr Expr::ident(std::string s) {
    auto e = std::make_unique<Expr>();
    e->kind = Kind::Ident;
    e->sval = std::move(s);
    return e;
}
ExprPtr Expr::intLit(long long v) {
    auto e = std::make_unique<Expr>();
    e->kind = Kind::Int;
    e->ival = v;
    return e;
}
ExprPtr Expr::floatLit(double v) {
    auto e = std::make_unique<Expr>();
    e->kind = Kind::Float;
    e->fval = v;
    return e;
}
ExprPtr Expr::strLit(std::string s) {
    auto e = std::make_unique<Expr>();
    e->kind = Kind::String;
    e->sval = std::move(s);
    return e;
}
ExprPtr Expr::bin(std::string op, ExprPtr l, ExprPtr r) {
    auto e = std::make_unique<Expr>();
    e->kind = Kind::Binary;
    e->sval = std::move(op);
    e->lhs  = std::move(l);
    e->rhs  = std::move(r);
    return e;
}

// ---------- Pretty printers ----------
static void indent_of(std::ostream& os, int n) {
    for (int i = 0; i < n; ++i) os.put(' ');
}

void Expr::print(std::ostream& os) const {
    switch (kind) {
        case Kind::Ident:  os << sval; break;
        case Kind::Int:    os << ival; break;
        case Kind::Float:  os << fval; break;
        case Kind::String: os << '"' << sval << '"'; break;
        case Kind::Binary:
            os << "(";
            lhs->print(os);
            os << " " << sval << " ";
            rhs->print(os);
            os << ")";
            break;
    }
}

void Stmt::print(std::ostream& os, int indent) const {
    indent_of(os, indent);
    os << opName(op) << " " << dataset;
    if (op == Op::Filter && predicate) {
        os << " where ";
        predicate->print(os);
    } else if (!argument.empty()) {
        if (op == Op::Sort || op == Op::Group) os << " by ";
        else                                   os << " ";
        os << argument;
    }
    os << "\n";
}

void Pipeline::print(std::ostream& os) const {
    os << "pipeline " << name << " {\n";
    for (const auto& s : stmts) s->print(os, 2);
    os << "}\n";
}

} // namespace pipelang
