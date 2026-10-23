#pragma once

#include <optional>
#include <string>
#include <vector>

#include "pipelang/value.hpp"

namespace pipelang {

enum class AggregateFunction {
    Sum,
    Avg,
    Count,
    Min,
    Max
};

struct AggregateInfo {
    AggregateFunction id;
    std::string name;
    bool needs_number;  // sum and avg only accept number columns
};

const std::vector<AggregateInfo>& aggregate_functions();
const AggregateInfo* find_aggregate(const std::string& name);

enum class TransformFunction {
    CalculateGrade,
    Uppercase,
    AddRowNumber
};

// What a built-in transform does, as far as type checking and the
// optimizer need to know.
struct TransformInfo {
    TransformFunction id;
    std::string name;
    std::string description;
    std::string required_number_column;  // empty if none
    std::string added_column;            // created or overwritten; empty if none
    ColumnType added_type = ColumnType::String;
    bool modifies_string_columns = false;
    bool order_sensitive = false;        // the result depends on row order
};

const std::vector<TransformInfo>& transform_functions();
const TransformInfo* find_transform(const std::string& name);

// "sum, avg, count, min, max"
std::string aggregate_function_list();
std::string transform_function_list();

// Name of the column an aggregate produces, e.g. "sum_revenue".
std::string aggregate_column_name(const AggregateInfo& function, const std::string& field);

// Letter grade used by calculate_grade: A >= 90, B >= 80, C >= 70, D >= 60, else F.
const char* letter_grade(double marks);

}  // namespace pipelang
