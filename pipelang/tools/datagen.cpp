// Generates large, deterministic CSV datasets for benchmarking PipeLang:
//   pipelang_datagen <out-dir> <rows> [seed]
// writes orders.csv, customers.csv, events.csv and payments.csv with <rows>
// rows each. About 2% of rows have a missing value and about 2% are exact
// duplicates, so `clean` has real work to do.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// splitmix64: small, fast and identical on every platform.
class Random {
public:
    explicit Random(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        std::uint64_t value = (state_ += 0x9E3779B97F4A7C15ULL);
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    std::uint64_t below(std::uint64_t limit) { return next() % limit; }

    template <typename T>
    const T& pick(const std::vector<T>& items) { return items[below(items.size())]; }

private:
    std::uint64_t state_;
};

std::string money(std::uint64_t cents) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%llu.%02llu",
                  static_cast<unsigned long long>(cents / 100),
                  static_cast<unsigned long long>(cents % 100));
    return buffer;
}

// Writes `rows` lines produced by `make_row`, blanking one field in ~2% of
// rows and repeating the previous row in another ~2%.
template <typename MakeRow>
bool write_dataset(const std::filesystem::path& path, const std::string& header,
                   std::size_t rows, Random& random, MakeRow make_row) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "cannot write " << path.string() << '\n';
        return false;
    }
    file << header << '\n';
    std::string previous;
    for (std::size_t index = 0; index < rows; ++index) {
        const std::uint64_t roll = random.below(100);
        if (roll < 2 && !previous.empty()) {
            file << previous << '\n';
            continue;
        }
        std::vector<std::string> fields = make_row(index);
        if (roll < 4) {
            fields[1 + random.below(fields.size() - 1)].clear();
        }
        std::string line;
        for (std::size_t field = 0; field < fields.size(); ++field) {
            if (field > 0) line += ',';
            line += fields[field];
        }
        file << line << '\n';
        previous = line;
    }
    return static_cast<bool>(file);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <out-dir> <rows> [seed]\n";
        return 2;
    }
    const std::filesystem::path directory = argv[1];
    char* end = nullptr;
    const unsigned long long rows = std::strtoull(argv[2], &end, 10);
    if (*end != '\0' || rows == 0) {
        std::cerr << "rows must be a positive whole number\n";
        return 2;
    }
    const std::uint64_t seed = argc == 4 ? std::strtoull(argv[3], nullptr, 10) : 42;

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "cannot create " << directory.string() << ": " << error.message() << '\n';
        return 1;
    }

    const std::vector<std::string> regions = {"North", "South", "East", "West", "Central"};
    const std::vector<std::string> statuses = {"delivered", "delivered", "delivered",
                                               "shipped", "cancelled"};
    const std::vector<std::string> first = {"Asha", "Ben", "Chen", "Divya", "Elif", "Farhan",
                                            "Grace", "Hugo", "Isha", "Jonas", "Kavya", "Liam"};
    const std::vector<std::string> last = {"Rao", "Okafor", "Li", "Nair", "Kaya", "Ali",
                                           "Kim", "Silva", "Patel", "Berg", "Iyer", "Moreau"};
    const std::vector<std::string> cities = {"Pune", "Lagos", "Oslo", "Seoul", "Porto",
                                             "Kochi", "Izmir", "Lima"};
    const std::vector<std::string> kinds = {"click", "view", "search", "purchase", "share"};
    const std::vector<std::string> methods = {"card", "upi", "cash", "wallet"};

    Random random(seed);
    const std::size_t count = static_cast<std::size_t>(rows);
    const bool ok =
        write_dataset(directory / "orders.csv", "order_id,customer_id,region,amount,status",
            count, random, [&](std::size_t index) {
                return std::vector<std::string>{
                    std::to_string(100000 + index), std::to_string(1 + random.below(50000)),
                    random.pick(regions), money(random.below(100000)), random.pick(statuses)};
            }) &&
        write_dataset(directory / "customers.csv", "customer_id,name,city,age",
            count, random, [&](std::size_t index) {
                return std::vector<std::string>{
                    std::to_string(1 + index), random.pick(first) + " " + random.pick(last),
                    random.pick(cities), std::to_string(16 + random.below(60))};
            }) &&
        write_dataset(directory / "events.csv", "event_id,user_id,kind,duration",
            count, random, [&](std::size_t index) {
                return std::vector<std::string>{
                    std::to_string(1 + index), std::to_string(1 + random.below(50000)),
                    random.pick(kinds), std::to_string(random.below(5000))};
            }) &&
        write_dataset(directory / "payments.csv", "payment_id,method,amount",
            count, random, [&](std::size_t index) {
                return std::vector<std::string>{
                    std::to_string(1 + index), random.pick(methods), money(random.below(50000))};
            });

    if (!ok) {
        return 1;
    }
    std::cout << "Wrote 4 datasets with " << rows << " rows each to " << directory.string() << '\n';
    return 0;
}
