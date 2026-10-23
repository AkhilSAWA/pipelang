#include "pipelang/schema.hpp"

#include <optional>
#include <utility>

#include "pipelang/functions.hpp"
#include "pipelang/semantic.hpp"

namespace pipelang {
namespace {

struct DatasetSchema {
    Schema columns;
    std::string group_key;  // empty when not grouped
    bool failed = false;    // stop checking after the first error
};

}  // namespace

SchemaResult analyze_schemas(
    const IRProgram& program,
    const std::map<std::string, Schema>& inputs
) {
    SchemaResult result;
    result.after.resize(program.size());
    std::map<std::string, DatasetSchema> datasets;

    for (const IRInstruction& instruction : program) {
        DatasetSchema& state = datasets[instruction.dataset];
        if (state.failed) {
            continue;
        }

        const std::size_t errors_before = result.diagnostics.size();
        auto error = [&](std::string message) {
            result.diagnostics.push_back(
                {DiagnosticKind::Semantic, instruction.line, std::move(message)}
            );
        };
        auto require_column = [&](const std::string& field) -> std::optional<std::size_t> {
            const std::optional<std::size_t> column = find_column(state.columns, field);
            if (!column) {
                error(
                    "field '" + field + "' is not a column of dataset '" +
                    instruction.dataset + "' (columns: " + column_list(state.columns) + ")"
                );
            }
            return column;
        };

        switch (instruction.opcode) {
            case IROpcode::Input: {
                const auto input = inputs.find(instruction.dataset);
                if (input == inputs.end()) {
                    error("no data was loaded for dataset '" + instruction.dataset + "'");
                } else {
                    state.columns = input->second;
                }
                break;
            }
            case IROpcode::Clean:
            case IROpcode::Output:
                break;
            case IROpcode::Filter:
                check_condition(
                    *instruction.condition,
                    instruction.line,
                    instruction.dataset,
                    &state.columns,
                    result.diagnostics
                );
                break;
            case IROpcode::Transform: {
                const TransformInfo* function = find_transform(instruction.function);
                if (function == nullptr) {
                    error("unknown transform function '" + instruction.function + "'");
                    break;
                }
                const std::string& required = function->required_number_column;
                if (!required.empty()) {
                    const std::optional<std::size_t> column =
                        find_column(state.columns, required);
                    if (!column) {
                        error(
                            "transform '" + function->name + "' needs a number column '" +
                            required + "', but dataset '" + instruction.dataset +
                            "' has none (columns: " + column_list(state.columns) + ")"
                        );
                        break;
                    }
                    if (state.columns[*column].type != ColumnType::Number) {
                        error(
                            "transform '" + function->name + "' needs '" + required +
                            "' to be a number column, but it is a string column"
                        );
                        break;
                    }
                }
                if (!function->added_column.empty()) {
                    const std::optional<std::size_t> existing =
                        find_column(state.columns, function->added_column);
                    if (existing) {
                        state.columns[*existing].type = function->added_type;
                    } else {
                        state.columns.push_back({function->added_column, function->added_type});
                    }
                }
                break;
            }
            case IROpcode::Sort:
                require_column(instruction.field);
                break;
            case IROpcode::Group:
                if (require_column(instruction.field)) {
                    state.group_key = instruction.field;
                }
                break;
            case IROpcode::Aggregate: {
                const AggregateInfo* function = find_aggregate(instruction.function);
                const std::optional<std::size_t> column = require_column(instruction.field);
                if (function == nullptr || !column) {
                    if (function == nullptr) {
                        error("unknown aggregate function '" + instruction.function + "'");
                    }
                    break;
                }
                const Column field = state.columns[*column];
                if (function->needs_number && field.type != ColumnType::Number) {
                    error(
                        "aggregate function '" + function->name +
                        "' needs a number column, but '" + field.name +
                        "' is a string column"
                    );
                    break;
                }
                const bool counts = function->id == AggregateFunction::Count;
                Column produced{
                    aggregate_column_name(*function, field.name),
                    counts || function->needs_number ? ColumnType::Number : field.type
                };
                Schema next;
                if (!state.group_key.empty()) {
                    next.push_back(state.columns[*find_column(state.columns, state.group_key)]);
                }
                if (find_column(next, produced.name)) {
                    error(
                        "aggregate result column '" + produced.name +
                        "' has the same name as the group column"
                    );
                    break;
                }
                next.push_back(std::move(produced));
                state.columns = std::move(next);
                state.group_key.clear();
                break;
            }
        }

        if (result.diagnostics.size() != errors_before) {
            state.failed = true;
        } else {
            result.after[instruction.id] = state.columns;
        }
    }

    return result;
}

}  // namespace pipelang
