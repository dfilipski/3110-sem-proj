#ifndef MOVIE_H
#define MOVIE_H

#include <string>
#include <vector>

class Movie {
public:
    // Preserve the complete title, including its release year.
    Movie(int movieId, std::string title, std::vector<std::string> genres);
    static Movie fromDatLine(const std::string& line);
    std::string toDatLine() const;

    int getMovieId() const { return movieId_; }
    const std::string& getTitle() const { return title_; }
    const std::vector<std::string>& getGenres() const { return genres_; }
    bool hasGenre(const std::string& genre) const;

private:
    int movieId_;
    std::string title_;
    std::vector<std::string> genres_;
};

#endif
