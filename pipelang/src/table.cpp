#include "pipelang/table.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace pipelang {
namespace {

struct Field {
    std::string text;
    bool quoted = false;
};

struct Record {
    std::vector<Field> fields;
    int line = 0;  // line where the record starts
};

bool is_blank(char character) {
    return character == ' ' || character == '\t';
}

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && is_blank(text[begin])) ++begin;
    while (end > begin && is_blank(text[end - 1])) --end;
    return text.substr(begin, end - begin);
}

// Splits the whole input into records, handling quoted fields.
std::vector<Record> parse_records(const std::string& text, const std::string& source) {
    std::vector<Record> records;
    std::size_t position = 0;
    int line = 1;
    const std::size_t size = text.size();

    auto fail = [&source](int at_line, const std::string& message) -> CsvError {
        return CsvError(source + ":" + std::to_string(at_line) + ": " + message);
    };

    while (position < size) {
        Record record;
        record.line = line;
        bool end_of_record = false;

        while (!end_of_record) {
            Field field;
            // Leading blanks before an opening quote are ignored.
            std::size_t start = position;
            while (start < size && is_blank(text[start])) ++start;

            if (start < size && text[start] == '"') {
                field.quoted = true;
                position = start + 1;
                const int quote_line = line;
                bool closed = false;
                while (position < size) {
                    const char character = text[position];
                    if (character == '"') {
                        if (position + 1 < size && text[position + 1] == '"') {
                            field.text += '"';
                            position += 2;
                            continue;
                        }
                        ++position;
                        closed = true;
                        break;
                    }
                    if (character == '\n') ++line;
                    field.text += character;
                    ++position;
                }
                if (!closed) {
                    throw fail(quote_line, "unterminated quoted field");
                }
                while (position < size && is_blank(text[position])) ++position;
                if (position < size && text[position] != ',' &&
                    text[position] != '\n' && text[position] != '\r') {
                    throw fail(line, "unexpected text after a closing quote");
                }
            } else {
                while (position < size && text[position] != ',' &&
                       text[position] != '\n' && text[position] != '\r') {
                    if (text[position] == '"') {
                        throw fail(line, "quote inside an unquoted field");
                    }
                    field.text += text[position];
                    ++position;
                }
                field.text = trim(field.text);
            }

            record.fields.push_back(std::move(field));

            if (position >= size) {
                end_of_record = true;
            } else if (text[position] == ',') {
                ++position;
            } else {
                // "\r\n", "\n" or a lone "\r" ends the record.
                if (text[position] == '\r') ++position;
                if (position < size && text[position] == '\n') ++position;
                ++line;
                end_of_record = true;
            }
        }

        const bool blank = record.fields.size() == 1 &&
            !record.fields[0].quoted && record.fields[0].text.empty();
        if (!blank) {
            records.push_back(std::move(record));
        }
    }

    return records;
}

// Optional sign, digits with an optional fraction, optional exponent. Rejects
// forms strtod would accept but CSV data should not contain (inf, nan, hex).
bool looks_numeric(const std::string& text) {
    std::size_t index = 0;
    const std::size_t size = text.size();
    if (index < size && (text[index] == '+' || text[index] == '-')) ++index;
    std::size_t digits = 0;
    while (index < size && std::isdigit(static_cast<unsigned char>(text[index]))) {
        ++index;
        ++digits;
    }
    if (index < size && text[index] == '.') {
        ++index;
        while (index < size && std::isdigit(static_cast<unsigned char>(text[index]))) {
            ++index;
            ++digits;
        }
    }
    if (digits == 0) {
        return false;
    }
    if (index < size && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        if (index < size && (text[index] == '+' || text[index] == '-')) ++index;
        std::size_t exponent_digits = 0;
        while (index < size && std::isdigit(static_cast<unsigned char>(text[index]))) {
            ++index;
            ++exponent_digits;
        }
        if (exponent_digits == 0) {
            return false;
        }
    }
    return index == size;
}

bool needs_quotes(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    if (is_blank(text.front()) || is_blank(text.back())) {
        return true;
    }
    return text.find_first_of(",\"\r\n") != std::string::npos;
}

void write_field(const std::string& text, std::ostream& output) {
    if (!needs_quotes(text)) {
        output << text;
        return;
    }
    output << '"';
    for (const char character : text) {
        if (character == '"') output << '"';
        output << character;
    }
    output << '"';
}

}  // namespace

Table read_csv(std::istream& input, const std::string& source_name) {
    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string text = buffer.str();
    if (text.rfind("\xEF\xBB\xBF", 0) == 0) {
        text.erase(0, 3);  // UTF-8 byte order mark written by some editors
    }

    const std::vector<Record> records = parse_records(text, source_name);
    if (records.empty()) {
        throw CsvError(source_name + ": file is empty; expected a header row");
    }

    Table table;
    std::unordered_set<std::string> seen;
    for (const Field& field : records[0].fields) {
        const std::string name = trim(field.text);
        if (name.empty()) {
            throw CsvError(source_name + ":" + std::to_string(records[0].line) +
                           ": header has an empty column name");
        }
        if (!seen.insert(name).second) {
            throw CsvError(source_name + ":" + std::to_string(records[0].line) +
                           ": duplicate column name '" + name + "'");
        }
        table.columns.push_back({name, ColumnType::String});
    }

    const std::size_t width = table.columns.size();
    for (std::size_t index = 1; index < records.size(); ++index) {
        const Record& record = records[index];
        if (record.fields.size() != width) {
            throw CsvError(
                source_name + ":" + std::to_string(record.line) + ": expected " +
                std::to_string(width) + " fields, found " +
                std::to_string(record.fields.size())
            );
        }
    }

    // A column is numeric when it has at least one value and all values parse.
    std::vector<bool> numeric(width, true);
    std::vector<bool> has_value(width, false);
    for (std::size_t index = 1; index < records.size(); ++index) {
        for (std::size_t column = 0; column < width; ++column) {
            const std::string& cell = records[index].fields[column].text;
            if (cell.empty()) {
                continue;
            }
            has_value[column] = true;
            if (numeric[column] && !looks_numeric(cell)) {
                numeric[column] = false;
            }
        }
    }
    for (std::size_t column = 0; column < width; ++column) {
        if (numeric[column] && has_value[column]) {
            table.columns[column].type = ColumnType::Number;
        }
    }

    table.rows.reserve(records.size() - 1);
    for (std::size_t index = 1; index < records.size(); ++index) {
        Row row;
        row.reserve(width);
        for (std::size_t column = 0; column < width; ++column) {
            const std::string& cell = records[index].fields[column].text;
            if (cell.empty()) {
                row.emplace_back();
            } else if (table.columns[column].type == ColumnType::Number) {
                row.push_back(Value::number(std::strtod(cell.c_str(), nullptr)));
            } else {
                row.push_back(Value::string(cell));
            }
        }
        table.rows.push_back(std::move(row));
    }

    return table;
}

Table read_csv_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw CsvError("cannot open input file '" + path + "'");
    }
    return read_csv(input, path);
}

void write_csv(const Table& table, std::ostream& output) {
    for (std::size_t column = 0; column < table.columns.size(); ++column) {
        if (column > 0) output << ',';
        write_field(table.columns[column].name, output);
    }
    output << '\n';
    for (const Row& row : table.rows) {
        // A lone null would be a blank line, which readers skip; write "".
        if (row.size() == 1 && row[0].is_null()) {
            output << "\"\"\n";
            continue;
        }
        for (std::size_t column = 0; column < row.size(); ++column) {
            if (column > 0) output << ',';
            write_field(format_value(row[column]), output);
        }
        output << '\n';
    }
}

void write_csv_file(const Table& table, const std::string& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw CsvError("cannot write output file '" + path + "'");
    }
    write_csv(table, output);
    output.flush();
    if (!output) {
        throw CsvError("failed while writing output file '" + path + "'");
    }
}

}  // namespace pipelang
