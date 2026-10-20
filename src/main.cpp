#include "MovieDatabase.h"

#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

int main()
{
    try {
        const MovieDatabase database;

        std::cout << "Loaded " << database.getUsers().size() << " users, "
                  << database.getMovies().size() << " movies, and "
                  << database.getRatings().size() << " ratings.\n";

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

        const Movie* movie = database.findMovieById(movieId);
        if (!movie) {
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

        const auto average = database.averageRating(movieId);
        if (average) {
            std::cout << "Average user rating: " << std::fixed << std::setprecision(2)
                      << *average << " / 5\n";
        } else {
            std::cout << "No ratings available.\n";
        }

        const Movie* recommendation = database.recommendMovie(movieId);
        if (recommendation) {
            std::cout << "Recommended similar movie: " << recommendation->getTitle()
                      << " (ID: " << recommendation->getMovieId() << ")\n";
        } else {
            std::cout << "No similar movie with overlapping ratings was found.\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
