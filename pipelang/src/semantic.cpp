#include "pipelang/semantic.hpp"

#include <utility>

namespace pipelang {

SemanticResult analyze_semantics(const Pipeline& pipeline) {
    SemanticResult result;

    for (std::size_t index = 0; index < pipeline.statements.size(); ++index) {
        const Stmt& statement = *pipeline.statements[index];

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
                result.symbols.insert(statement.dataset, index, statement.line);
            }
            continue;
        }

        if (!result.symbols.contains(statement.dataset)) {
            result.errors.push_back({
                statement.line,
                "dataset '" + statement.dataset + "' is used by an operation " +
                    "before it is declared with input"
            });
        }
    }

    return result;
}

void print_semantic_errors(const SemanticResult& result, std::ostream& output) {
    for (const SemanticError& error : result.errors) {
        if (error.line > 0) {
            output << "semantic error at line " << error.line << ": "
                   << error.message << '\n';
        } else {
            output << "semantic error: " << error.message << '\n';
        }
    }
}

}  // namespace pipelang
