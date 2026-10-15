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
                    index,
                    "duplicate input declaration for dataset '" +
                        statement.dataset + "'; first declared at statement " +
                        std::to_string(existing->declaration_index + 1)
                });
            } else {
                result.symbols.insert(statement.dataset, index);
            }
            continue;
        }

        if (!result.symbols.contains(statement.dataset)) {
            result.errors.push_back({
                index,
                "dataset '" + statement.dataset + "' is used by an operation "
                "before it is declared with input"
            });
        }
    }

    return result;
}

void print_semantic_errors(const SemanticResult& result, std::ostream& output) {
    for (const SemanticError& error : result.errors) {
        output << "semantic error at statement " << error.statement_index + 1
               << ": " << error.message << '\n';
    }
}

}  // namespace pipelang
