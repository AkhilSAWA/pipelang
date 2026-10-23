#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace pipelang {

// The compiler stage that produced a diagnostic. Warnings never stop
// compilation; every other kind does.
enum class DiagnosticKind {
    General,
    Lexical,
    Syntax,
    Semantic,
    Runtime,
    Warning
};

struct Diagnostic {
    DiagnosticKind kind = DiagnosticKind::General;
    int line = 0;  // 0 when the diagnostic has no source position
    std::string message;
};

bool is_error(const Diagnostic& diagnostic);
bool has_errors(const std::vector<Diagnostic>& diagnostics);

// Prints "<kind> at line N: message", e.g. "semantic error at line 3: ...".
void print_diagnostic(const Diagnostic& diagnostic, std::ostream& output);
void print_diagnostics(
    const std::vector<Diagnostic>& diagnostics,
    std::ostream& output
);

}  // namespace pipelang
