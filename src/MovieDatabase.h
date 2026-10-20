#ifndef MOVIE_DATABASE_H
#define MOVIE_DATABASE_H

#include "Movie.h"
#include "MovieRatingsCSR.h"
#include "Rating.h"
#include "User.h"

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

class MovieDatabase {
public:
    // Loads all three data files; throws std::runtime_error on a loading error.
    explicit MovieDatabase(const std::string& dataDirectory = "data");

    const std::vector<User>& getUsers() const { return users_; }
    const std::vector<Movie>& getMovies() const { return movies_; }
    const std::vector<Rating>& getRatings() const { return ratings_; }

    // Returns nullptr when absent. The pointer refers to this database's storage
    // and remains valid until the database is destroyed, assigned, or moved from.
    const Movie* findMovieById(int movieId) const;

    // Precomputed arithmetic mean, with expected constant-time lookup.
    // Returns no value for an unknown movie or a movie with no ratings.
    std::optional<double> averageRating(int movieId) const;

    // Highest cosine similarity of movie rating vectors; excludes the input movie.
    // Ties choose the smaller movie ID. Returns nullptr for an unknown ID or
    // when no other movie shares any ratings. Pointer lifetime matches findMovieById.
    const Movie* recommendMovie(int movieId) const;

private:
    std::vector<User> users_;
    std::vector<Movie> movies_;
    std::vector<Rating> ratings_;
    MovieRatingsCSR ratingMatrix_;
    std::unordered_map<int, std::optional<double>> averageRatings_;
};

#endif
