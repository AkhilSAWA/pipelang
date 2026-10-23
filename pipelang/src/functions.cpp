#include "pipelang/functions.hpp"

namespace pipelang {

const std::vector<AggregateInfo>& aggregate_functions() {
    static const std::vector<AggregateInfo> functions = {
        {AggregateFunction::Sum, "sum", true},
        {AggregateFunction::Avg, "avg", true},
        {AggregateFunction::Count, "count", false},
        {AggregateFunction::Min, "min", false},
        {AggregateFunction::Max, "max", false},
    };
    return functions;
}

const AggregateInfo* find_aggregate(const std::string& name) {
    for (const AggregateInfo& function : aggregate_functions()) {
        if (function.name == name) {
            return &function;
        }
    }
    return nullptr;
}

const std::vector<TransformInfo>& transform_functions() {
    static const std::vector<TransformInfo> functions = {
        {
            TransformFunction::CalculateGrade,
            "calculate_grade",
            "adds a 'grade' column (A-F) from the number column 'marks'",
            "marks",
            "grade",
            ColumnType::String,
            false,
            false,
        },
        {
            TransformFunction::Uppercase,
            "uppercase",
            "converts every string column to upper case",
            "",
            "",
            ColumnType::String,
            true,
            false,
        },
        {
            TransformFunction::AddRowNumber,
            "add_row_number",
            "adds a 'row_number' column numbering rows from 1 in their current order",
            "",
            "row_number",
            ColumnType::Number,
            false,
            true,
        },
    };
    return functions;
}

const TransformInfo* find_transform(const std::string& name) {
    for (const TransformInfo& function : transform_functions()) {
        if (function.name == name) {
            return &function;
        }
    }
    return nullptr;
}

std::string aggregate_function_list() {
    std::string list;
    for (const AggregateInfo& function : aggregate_functions()) {
        if (!list.empty()) list += ", ";
        list += function.name;
    }
    return list;
}

std::string transform_function_list() {
    std::string list;
    for (const TransformInfo& function : transform_functions()) {
        if (!list.empty()) list += ", ";
        list += function.name;
    }
    return list;
}

std::string aggregate_column_name(const AggregateInfo& function, const std::string& field) {
    return function.name + "_" + field;
}

const char* letter_grade(double marks) {
    if (marks >= 90) return "A";
    if (marks >= 80) return "B";
    if (marks >= 70) return "C";
    if (marks >= 60) return "D";
    return "F";
}

}  // namespace pipelang
