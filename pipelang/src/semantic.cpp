#include "pipelang/semantic.hpp"

namespace pipelang {

SemanticResult analyze_semantics(const Pipeline& pipeline) {
    SemanticResult result;

    for (const auto& statement_ptr : pipeline.statements) {
        const Stmt& statement = *statement_ptr;

        if (statement.kind == StmtKind::Input) {
            const Symbol* existing = result.symbols.find(statement.dataset);
            if (existing != nullptr) {
                result.errors.push_back({
                    statement.line,
                    "duplicate input declaration for dataset '" +
                        statement.dataset + "'; first declared at line " +
                        std::to_string(existing->declaration_line)
                });
            } else {
                result.symbols.insert(statement.dataset, statement.line);
            }
            continue;
        }

        if (!result.symbols.contains(statement.dataset)) {
            result.errors.push_back({
                statement.line,
                "dataset '" + statement.dataset +
                    "' is used by an operation before it is declared with input"
            });
        }
    }

    return result;
}

void print_semantic_errors(const SemanticResult& result, std::ostream& output) {
    for (const SemanticError& error : result.errors) {
        output << "semantic error at line " << error.line
               << ": " << error.message << '\n';
    }
}

}  // namespace pipelang
