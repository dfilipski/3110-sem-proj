#ifndef MOVIE_DATABASE_H
#define MOVIE_DATABASE_H

#include "Movie.h"
#include "Rating.h"
#include "User.h"

#include <string>
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

private:
    std::vector<User> users_;
    std::vector<Movie> movies_;
    std::vector<Rating> ratings_;
};

#endif
