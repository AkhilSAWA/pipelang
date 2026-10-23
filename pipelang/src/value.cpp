#include "pipelang/value.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

namespace pipelang {

const char* column_type_name(ColumnType type) {
    return type == ColumnType::Number ? "number" : "string";
}

std::optional<std::size_t> find_column(const Schema& schema, const std::string& name) {
    for (std::size_t index = 0; index < schema.size(); ++index) {
        if (schema[index].name == name) {
            return index;
        }
    }
    return std::nullopt;
}

std::string column_list(const Schema& schema) {
    std::string list;
    for (const Column& column : schema) {
        if (!list.empty()) {
            list += ", ";
        }
        list += column.name;
    }
    return list;
}

Value Value::number(double value) {
    Value result;
    result.data_ = value;
    return result;
}

Value Value::string(std::string value) {
    Value result;
    result.data_ = std::move(value);
    return result;
}

std::string format_number(double value) {
    if (value == 0.0) {
        return "0";  // also turns -0 into 0
    }
    char buffer[64];
    if (std::isfinite(value) && std::fabs(value) < 1e15 && value == std::trunc(value)) {
        const auto result = std::to_chars(
            buffer, buffer + sizeof buffer, static_cast<long long>(value)
        );
        return std::string(buffer, result.ptr);
    }
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value);
    return std::string(buffer, result.ptr);
}

std::string format_value(const Value& value) {
    if (value.is_number()) {
        return format_number(value.as_number());
    }
    if (value.is_string()) {
        return value.as_string();
    }
    return "";
}

int compare_values(const Value& left, const Value& right) {
    if (left.is_number() && right.is_number()) {
        const double a = left.as_number();
        const double b = right.as_number();
        return a < b ? -1 : (a > b ? 1 : 0);
    }
    if (left.is_string() && right.is_string()) {
        const int result = left.as_string().compare(right.as_string());
        return result < 0 ? -1 : (result > 0 ? 1 : 0);
    }
    return left.is_number() ? -1 : 1;
}

bool null_last_less(const Value& left, const Value& right, bool descending) {
    if (left.is_null() || right.is_null()) {
        return !left.is_null() && right.is_null();
    }
    const int order = compare_values(left, right);
    return descending ? order > 0 : order < 0;
}

bool same_value(const Value& left, const Value& right) {
    if (left.is_null() || right.is_null()) {
        return left.is_null() && right.is_null();
    }
    return compare_values(left, right) == 0;
}

void append_value_key(const Value& value, std::string& key) {
    if (value.is_null()) {
        key += 'N';
    } else if (value.is_number()) {
        key += 'D';
        double number = value.as_number();
        if (number == 0.0) {
            number = 0.0;  // -0 and 0 are the same value
        }
        char bytes[sizeof number];
        std::memcpy(bytes, &number, sizeof number);
        key.append(bytes, sizeof bytes);
    } else {
        key += 'S';
        const std::uint64_t length = value.as_string().size();
        char bytes[sizeof length];
        std::memcpy(bytes, &length, sizeof length);
        key.append(bytes, sizeof bytes);
        key += value.as_string();
    }
}

}  // namespace pipelang
