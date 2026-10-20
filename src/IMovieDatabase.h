#ifndef I_MOVIE_DATABASE_H
#define I_MOVIE_DATABASE_H

#include "Movie.h"

#include <optional>
#include <string>
#include <vector>

// Common query contract for databases with different storage implementations.
class IMovieDatabase {
public:
    virtual ~IMovieDatabase() = default;

    // Returns nullptr when absent. The pointer refers to this database's storage
    // and remains valid until the database is destroyed, assigned, or moved from.
    virtual const Movie* findMovieById(int movieId) const = 0;

    // Precomputed arithmetic mean, with expected constant-time lookup.
    // Returns no value for an unknown movie or a movie with no ratings.
    virtual std::optional<double> averageRating(int movieId) const = 0;

    // Search results are sorted by movie ID; pointer lifetime matches findMovieById.
    // Requires at least one canonical genre and matches all supplied genres.
    // Empty or unknown genres throw std::invalid_argument.
    virtual std::vector<const Movie*> findMoviesByGenres(const std::vector<std::string>& genres) const = 0;

    // Inclusive range using unrounded means; excludes unrated movies.
    // Nonfinite bounds, bounds outside [1, 5], or reversed bounds throw
    // std::invalid_argument. Ordering and pointer lifetime match genre search.
    virtual std::vector<const Movie*> findMoviesByAverageRating(double minimum, double maximum) const = 0;

};

#endif
