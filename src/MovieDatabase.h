#ifndef MOVIE_DATABASE_H
#define MOVIE_DATABASE_H

#include "IMovieDatabase.h"
#include "Rating.h"
#include "User.h"

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

class MovieDatabase : public IMovieDatabase {
public:
    // Loads all three data files; throws std::runtime_error on a loading error.
    explicit MovieDatabase(const std::string& dataDirectory = "data");

    const std::vector<User>& getUsers() const { return users_; }
    const std::vector<Movie>& getMovies() const { return movies_; }
    const std::vector<Rating>& getRatings() const { return ratings_; }

    // Returns nullptr when absent. The pointer refers to this database's storage
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
    std::vector<User> users_;
    std::vector<Movie> movies_;
    std::vector<Rating> ratings_;
    std::unordered_map<int, std::optional<double>> averageRatings_;
};

#endif
