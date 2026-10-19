#ifndef RATING_H
#define RATING_H

#include <cstdint>
#include <string>

class Rating {
public:
    // Timestamp is Unix time in seconds.
    Rating(int userId, int movieId, int rating, std::int64_t timestamp);
    static Rating fromDatLine(const std::string& line);
    std::string toDatLine() const;

    int getUserId() const { return userId_; }
    int getMovieId() const { return movieId_; }
    int getRating() const { return rating_; }
    std::int64_t getTimestamp() const { return timestamp_; }

private:
    int userId_;
    int movieId_;
    int rating_;
    std::int64_t timestamp_;
};

#endif
