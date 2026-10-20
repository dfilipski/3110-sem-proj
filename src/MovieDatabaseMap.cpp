#include "MovieDatabaseMap.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {
// Each model parses its own MovieLens record format.
template <typename T>
std::vector<T> loadRecords(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open " + path + ". Run the program from the project directory.");
    }

    std::vector<T> records;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            records.push_back(T::fromDatLine(line));
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) {
        throw std::runtime_error("Error reading " + path);
    }
    return records;
}

std::unordered_map<int, Movie> loadMovies(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open " + path + ". Run the program from the project directory.");
    }
    std::unordered_map<int, Movie> movies;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            Movie movie = Movie::fromDatLine(line);
            const int id = movie.getMovieId();
            if (!movies.emplace(id, std::move(movie)).second) {
                throw std::invalid_argument("Duplicate movie ID: " + std::to_string(id));
            }
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) throw std::runtime_error("Error reading " + path);
    return movies;
}

} // namespace

MovieDatabaseMap::MovieDatabaseMap(const std::string& dataDirectory)
    : users_(loadRecords<User>((std::filesystem::path(dataDirectory) / "users.dat").string())),
      movies_(loadMovies((std::filesystem::path(dataDirectory) / "movies.dat").string())),
      ratings_(loadRecords<Rating>((std::filesystem::path(dataDirectory) / "ratings.dat").string())),
      ratingMatrix_(ratings_)
{
    struct RatingTotals {
        double sum = 0;
        std::size_t count = 0;
    };
    std::unordered_map<int, RatingTotals> totals;
    for (const auto& rating : ratings_) {
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
