#ifndef MOVIE_DATABASE_MAP_H
#define MOVIE_DATABASE_MAP_H

#include "IMovieDatabase.h"
#include "Rating.h"
#include "User.h"

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <functional>
#include <cstdint>

class MovieDatabaseMap : public IMovieDatabase {
public:
    // Rating keys are (movie ID, user ID), identifying one user's rating of a movie.
    using RatingKey = std::pair<int, int>;
    struct RatingKeyHash {
        std::size_t operator()(const RatingKey& key) const
        {
            return std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(key.first) << 32) |
                static_cast<std::uint32_t>(key.second));
        }
    };
    using RatingsMap = std::unordered_map<RatingKey, Rating, RatingKeyHash>;

    // Loads all three data files; throws std::runtime_error on a loading error,
    // including duplicate movie IDs, user IDs, or movie/user rating pairs (reported with file path and line number).
    explicit MovieDatabaseMap(const std::string& dataDirectory = "data");

    // Map iteration order is unspecified; user keys are user IDs.
    const std::unordered_map<int, User>& getUsers() const { return users_; }
    // Iteration order is unspecified; keys are movie IDs.
    const std::unordered_map<int, Movie>& getMovies() const { return movies_; }
    const RatingsMap& getRatings() const { return ratings_; }

    // Expected constant-time lookup. Returns nullptr when absent. The pointer refers to this database's storage
    // and remains valid until the database is destroyed, assigned, or moved from.
    const Movie* findMovieById(int movieId) const override;

    // Precomputed arithmetic mean, with expected constant-time lookup.
    // Returns no value for an unknown movie or a movie with no ratings.
    std::optional<double> averageRating(int movieId) const override;

    // Search results are sorted by movie ID; pointer lifetime matches findMovieById.
    // Requires at least one canonical genre and matches all supplied genres.
    // Empty or unknown genres throw std::invalid_argument.
    std::vector<const Movie*> findMoviesByGenres(const std::vector<std::string>& genres) const override;

    // Inclusive range using unrounded means; excludes unrated movies.
    // Nonfinite bounds, bounds outside [1, 5], or reversed bounds throw
    // std::invalid_argument. Ordering and pointer lifetime match genre search.
    std::vector<const Movie*> findMoviesByAverageRating(double minimum, double maximum) const override;


private:
    std::unordered_map<int, User> users_;
    std::unordered_map<int, Movie> movies_;
    RatingsMap ratings_;
    std::unordered_map<int, std::optional<double>> averageRatings_;
};

#endif
