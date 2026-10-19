#include "Movie.h"
#include "DataRecord.h"
#include <algorithm>
#include <utility>

Movie::Movie(int movieId, std::string title, std::vector<std::string> genres)
    : movieId_(movieId), title_(std::move(title)), genres_(std::move(genres))
{
    if (movieId < 1 || movieId > 3952) throw std::invalid_argument("Movie ID must be 1–3952");
    if (title_.empty() || title_.find("::") != std::string::npos ||
        title_.find_first_of("\r\n") != std::string::npos)
        throw std::invalid_argument("Invalid movie title");
    static const std::vector<std::string> allowed = {
        "Action", "Adventure", "Animation", "Children's", "Comedy", "Crime",
        "Documentary", "Drama", "Fantasy", "Film-Noir", "Horror", "Musical",
        "Mystery", "Romance", "Sci-Fi", "Thriller", "War", "Western"
    };
    if (genres_.empty()) throw std::invalid_argument("Movie must have at least one genre");
    for (const auto& genre : genres_)
        if (std::find(allowed.begin(), allowed.end(), genre) == allowed.end())
            throw std::invalid_argument("Unknown movie genre: " + genre);
}

Movie Movie::fromDatLine(const std::string& line)
{
    const auto f = data_record::fields(line, 3);
    return Movie(data_record::number<int>(f[0]), f[1], data_record::split(f[2], "|"));
}

std::string Movie::toDatLine() const
{
    std::string result = std::to_string(movieId_) + "::" + title_ + "::";
    for (std::size_t i = 0; i < genres_.size(); ++i) {
        if (i != 0) result += '|';
        result += genres_[i];
    }
    return result;
}

bool Movie::hasGenre(const std::string& genre) const
{
    return std::find(genres_.begin(), genres_.end(), genre) != genres_.end();
}
