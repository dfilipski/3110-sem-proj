#include "MovieDatabase.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

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

} // namespace

MovieDatabase::MovieDatabase(const std::string& dataDirectory)
    : users_(loadRecords<User>((std::filesystem::path(dataDirectory) / "users.dat").string())),
      movies_(loadRecords<Movie>((std::filesystem::path(dataDirectory) / "movies.dat").string())),
      ratings_(loadRecords<Rating>((std::filesystem::path(dataDirectory) / "ratings.dat").string())),
      ratingMatrix_(ratings_)
{
}

const Movie* MovieDatabase::findMovieById(int movieId) const
{
    // MovieLens IDs have gaps; they are not vector indexes.
    const auto movie = std::find_if(movies_.begin(), movies_.end(),
        [movieId](const Movie& entry) { return entry.getMovieId() == movieId; });
    return movie == movies_.end() ? nullptr : &*movie;
}

const Movie* MovieDatabase::recommendMovie(int movieId) const
{
    if (!findMovieById(movieId)) return nullptr;
    const Movie* best = nullptr;
    double bestSimilarity = 0;
    for (const auto& movie : movies_) {
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
