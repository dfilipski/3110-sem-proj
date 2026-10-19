#include "Rating.h"
#include "DataRecord.h"

Rating::Rating(int userId, int movieId, int rating, std::int64_t timestamp)
    : userId_(userId), movieId_(movieId), rating_(rating), timestamp_(timestamp)
{
    if (userId < 1 || userId > 6040) throw std::invalid_argument("User ID must be 1–6040");
    if (movieId < 1 || movieId > 3952) throw std::invalid_argument("Movie ID must be 1–3952");
    if (rating < 1 || rating > 5) throw std::invalid_argument("Rating must be 1–5 whole stars");
    if (timestamp < 0) throw std::invalid_argument("Timestamp must be nonnegative");
}

Rating Rating::fromDatLine(const std::string& line)
{
    const auto f = data_record::fields(line, 4);
    return Rating(data_record::number<int>(f[0]), data_record::number<int>(f[1]),
                  data_record::number<int>(f[2]), data_record::number<std::int64_t>(f[3]));
}

std::string Rating::toDatLine() const
{
    return std::to_string(userId_) + "::" + std::to_string(movieId_) + "::" +
           std::to_string(rating_) + "::" + std::to_string(timestamp_);
}
