#include "MovieDatabaseMap.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {
// Parse directly into the owning map and report duplicate keys at their source line.
template <typename Map, typename KeyFunction>
Map loadRecords(const std::string& path, KeyFunction keyFor)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open " + path + ". Run the program from the project directory.");
    }
    Map records;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            auto record = Map::mapped_type::fromDatLine(line);
            const auto key = keyFor(record);
            if (!records.emplace(key, std::move(record)).second) {
                throw std::invalid_argument("Duplicate record key");
            }
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) throw std::runtime_error("Error reading " + path);
    return records;
}

// CSR accepts a vector; this temporary is released once construction completes.
std::vector<Rating> matrixRatings(const MovieDatabaseMap::RatingsMap& ratings)
{
    std::vector<Rating> result;
    result.reserve(ratings.size());
    for (const auto& entry : ratings) result.push_back(entry.second);
    return result;
}

} // namespace

MovieDatabaseMap::MovieDatabaseMap(const std::string& dataDirectory)
    : users_(loadRecords<std::unordered_map<int, User>>(
          (std::filesystem::path(dataDirectory) / "users.dat").string(),
          [](const User& user) { return user.getUserId(); })),
      movies_(loadRecords<std::unordered_map<int, Movie>>(
          (std::filesystem::path(dataDirectory) / "movies.dat").string(),
          [](const Movie& movie) { return movie.getMovieId(); })),
      ratings_(loadRecords<RatingsMap>(
          (std::filesystem::path(dataDirectory) / "ratings.dat").string(),
          [](const Rating& rating) { return RatingKey{rating.getMovieId(), rating.getUserId()}; })),
      ratingMatrix_(matrixRatings(ratings_))
{
    struct RatingTotals {
        double sum = 0;
        std::size_t count = 0;
    };
    std::unordered_map<int, RatingTotals> totals;
    for (const auto& entry : ratings_) {
        const Rating& rating = entry.second;
        auto& total = totals[rating.getMovieId()];
        total.sum += rating.getRating();
        ++total.count;
    }
    for (const auto& entry : movies_) {
        const Movie& movie = entry.second;
        const int id = movie.getMovieId();
        const auto total = totals.find(id);
        averageRatings_[id] = total == totals.end()
            ? std::nullopt
            : std::optional<double>(total->second.sum / total->second.count);
    }
}

std::optional<double> MovieDatabaseMap::averageRating(int movieId) const
{
    const auto average = averageRatings_.find(movieId);
    return average == averageRatings_.end() ? std::nullopt : average->second;
}

const Movie* MovieDatabaseMap::findMovieById(int movieId) const
{
    const auto movie = movies_.find(movieId);
    return movie == movies_.end() ? nullptr : &movie->second;
}

const Movie* MovieDatabaseMap::recommendMovie(int movieId) const
{
    if (!findMovieById(movieId)) return nullptr;
    const Movie* best = nullptr;
    double bestSimilarity = 0;
    for (const auto& entry : movies_) {
        const Movie& movie = entry.second;
        if (movie.getMovieId() == movieId) continue;
        const double similarity = ratingMatrix_.cosineSimilarity(movieId, movie.getMovieId());
        if (similarity > bestSimilarity ||
            (best && similarity == bestSimilarity && movie.getMovieId() < best->getMovieId())) {
            best = &movie;
            bestSimilarity = similarity;
        }
    }
    return best;
}

std::vector<const Movie*> MovieDatabaseMap::findMoviesByGenres(const std::vector<std::string>& genres) const
{
    if (genres.empty()) throw std::invalid_argument("Select at least one genre");
    const auto& available = Movie::availableGenres();
    for (const auto& genre : genres) {
        if (std::find(available.begin(), available.end(), genre) == available.end())
            throw std::invalid_argument("Unknown movie genre: " + genre);
    }
    std::vector<const Movie*> results;
    for (const auto& entry : movies_) {
        const Movie& movie = entry.second;
        if (std::all_of(genres.begin(), genres.end(),
                        [&movie](const std::string& genre) { return movie.hasGenre(genre); }))
            results.push_back(&movie);
    }
    std::sort(results.begin(), results.end(),
              [](const Movie* a, const Movie* b) { return a->getMovieId() < b->getMovieId(); });
    return results;
}

std::vector<const Movie*> MovieDatabaseMap::findMoviesByAverageRating(double minimum, double maximum) const
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
        minimum < 1 || maximum > 5 || minimum > maximum)
        throw std::invalid_argument("Rating range must satisfy 1 <= minimum <= maximum <= 5");
    std::vector<const Movie*> results;
    for (const auto& entry : movies_) {
        const Movie& movie = entry.second;
        const auto average = averageRating(movie.getMovieId());
        if (average && *average >= minimum && *average <= maximum) results.push_back(&movie);
    }
    std::sort(results.begin(), results.end(),
              [](const Movie* a, const Movie* b) { return a->getMovieId() < b->getMovieId(); });
    return results;
}
