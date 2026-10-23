#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pipelang/value.hpp"

namespace pipelang {

using Row = std::vector<Value>;

// An in-memory dataset: typed columns and rows of values.
struct Table {
    Schema columns;
    std::vector<Row> rows;
};

// Thrown for unreadable or malformed CSV input. The message already names
// the file and, when known, the CSV line.
class CsvError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Reads CSV with a header row. Fields are separated by commas; double quotes
// allow commas, quotes ("") and line breaks inside a field. Unquoted fields
// are trimmed of spaces and tabs; an empty field is null; blank lines are
// skipped. Column types are inferred (see ColumnType).
Table read_csv(std::istream& input, const std::string& source_name);
Table read_csv_file(const std::string& path);

// Writes a header row and one line per row. Fields are quoted only when
// they contain a comma, quote, line break or edge whitespace. A row that is
// a single null cell is written as "" so it is not mistaken for a blank line.
void write_csv(const Table& table, std::ostream& output);
void write_csv_file(const Table& table, const std::string& path);

}  // namespace pipelang
