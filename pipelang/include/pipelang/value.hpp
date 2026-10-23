#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pipelang {

// The type of a dataset column. A column is Number when every non-empty cell
// of the input file is a number, otherwise String.
enum class ColumnType {
    Number,
    String
};

const char* column_type_name(ColumnType type);

struct Column {
    std::string name;
    ColumnType type = ColumnType::String;
};

// The columns of a dataset at one point in the pipeline.
using Schema = std::vector<Column>;

std::optional<std::size_t> find_column(const Schema& schema, const std::string& name);
std::string column_list(const Schema& schema);  // "id, name, marks"

// One cell: null (an empty CSV field), a number, or a string.
class Value {
public:
    Value() = default;
    static Value number(double value);
    static Value string(std::string value);

    bool is_null() const { return std::holds_alternative<std::monostate>(data_); }
    bool is_number() const { return std::holds_alternative<double>(data_); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }

    double as_number() const { return std::get<double>(data_); }
    const std::string& as_string() const { return std::get<std::string>(data_); }

private:
    std::variant<std::monostate, double, std::string> data_;
};

// Shortest text that reads back as the same double; integral values print
// without a decimal point ("1000.5", "42", "0").
std::string format_number(double value);

// CSV/display text of a value; null is the empty string.
std::string format_value(const Value& value);

// Orders two non-null values: numbers numerically, strings by bytes, and
// numbers before strings. Returns <0, 0 or >0.
int compare_values(const Value& left, const Value& right);

// Ordering used by sort and group: nulls always come last.
bool null_last_less(const Value& left, const Value& right, bool descending);

// True when both are null, or both are non-null and compare equal.
bool same_value(const Value& left, const Value& right);

// Appends an unambiguous binary encoding of the value, used to detect
// duplicate rows.
void append_value_key(const Value& value, std::string& key);

}  // namespace pipelang
