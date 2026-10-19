#ifndef DATA_RECORD_H
#define DATA_RECORD_H

#include <charconv>
#include <stdexcept>
#include <string>
#include <vector>

// Shared parsing for MovieLens records; no whitespace or partial numbers accepted.
namespace data_record {
inline std::vector<std::string> split(std::string line, const std::string& delimiter)
{
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> fields;
    std::size_t start = 0;
    std::size_t end;
    while ((end = line.find(delimiter, start)) != std::string::npos) {
        fields.push_back(line.substr(start, end - start));
        start = end + delimiter.size();
    }
    fields.push_back(line.substr(start));
    return fields;
}

inline std::vector<std::string> fields(const std::string& line, std::size_t count)
{
    auto result = split(line, "::");
    if (result.size() != count) throw std::invalid_argument("Incorrect MovieLens field count");
    return result;
}

template <typename T>
T number(const std::string& value)
{
    T result{};
    auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument("Invalid MovieLens integer: " + value);
    return result;
}
}

#endif
