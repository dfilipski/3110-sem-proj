#include "Movie.h"
#include "Rating.h"
#include "User.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

int main()
{
    try {
        const std::vector<User> users = loadRecords<User>("data/users.dat");
        const std::vector<Movie> movies = loadRecords<Movie>("data/movies.dat");
        const std::vector<Rating> ratings = loadRecords<Rating>("data/ratings.dat");

        std::cout << "Loaded " << users.size() << " users, " << movies.size()
                  << " movies, and " << ratings.size() << " ratings.\n";

        // Read a whole line so input such as "1abc" is rejected.
        std::string line;
        int movieId;
        while (true) {
            std::cout << "Enter a movie ID (1-3952): ";
            if (!std::getline(std::cin, line)) {
                std::cout << "\nNo movie ID entered.\n";
                return 0;
            }
            std::istringstream input(line);
            if (input >> movieId && (input >> std::ws).eof() && movieId >= 1 && movieId <= 3952) {
                break;
            }
            std::cout << "Please enter a whole-number movie ID between 1 and 3952.\n";
        }

        // IDs have gaps, so search by ID instead of using it as a vector index.
        const auto movie = std::find_if(movies.begin(), movies.end(),
            [movieId](const Movie& entry) { return entry.getMovieId() == movieId; });
        if (movie == movies.end()) {
            std::cout << "No movie found with ID " << movieId << ".\n";
            return 0;
        }

        std::cout << "\nMovie ID: " << movie->getMovieId()
                  << "\nTitle: " << movie->getTitle() << "\nGenres: ";
        const auto& genres = movie->getGenres();
        for (std::size_t i = 0; i < genres.size(); ++i) {
            if (i != 0) std::cout << ", ";
            std::cout << genres[i];
        }
        std::cout << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
