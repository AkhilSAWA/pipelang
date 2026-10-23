#include "pipelang/diagnostic.hpp"

#include <algorithm>

namespace pipelang {
namespace {

const char* diagnostic_label(DiagnosticKind kind) {
    switch (kind) {
        case DiagnosticKind::General: return "error";
        case DiagnosticKind::Lexical: return "lexical error";
        case DiagnosticKind::Syntax: return "syntax error";
        case DiagnosticKind::Semantic: return "semantic error";
        case DiagnosticKind::Runtime: return "runtime error";
        case DiagnosticKind::Warning: return "warning";
    }
    return "error";
}

}  // namespace

bool is_error(const Diagnostic& diagnostic) {
    return diagnostic.kind != DiagnosticKind::Warning;
}

bool has_errors(const std::vector<Diagnostic>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), is_error);
}

void print_diagnostic(const Diagnostic& diagnostic, std::ostream& output) {
    output << diagnostic_label(diagnostic.kind);
    if (diagnostic.line > 0) {
        output << " at line " << diagnostic.line;
    }
    output << ": " << diagnostic.message << '\n';
}

void print_diagnostics(
    const std::vector<Diagnostic>& diagnostics,
    std::ostream& output
) {
    for (const Diagnostic& diagnostic : diagnostics) {
        print_diagnostic(diagnostic, output);
    }
}

}  // namespace pipelang
