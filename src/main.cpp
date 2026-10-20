#include "MovieDatabase.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
template <typename T>
bool readNumber(const std::string& prompt, T minimum, T maximum, T& value)
{
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) return false;
        std::istringstream input(line);
        if (input >> value && (input >> std::ws).eof() &&
            std::isfinite(static_cast<double>(value)) && value >= minimum && value <= maximum)
            return true;
        std::cout << "Please enter a valid number between " << minimum << " and " << maximum << ".\n";
    }
}

void displayMovie(const MovieDatabase& database, const Movie& movie)
{
    std::cout << "\nMovie ID: " << movie.getMovieId()
              << "\nTitle: " << movie.getTitle() << "\nGenres: ";
    const auto& genres = movie.getGenres();
    for (std::size_t i = 0; i < genres.size(); ++i) {
        if (i != 0) std::cout << ", ";
        std::cout << genres[i];
    }
    const auto average = database.averageRating(movie.getMovieId());
    if (average) {
        std::cout << "\nAverage user rating: " << std::fixed << std::setprecision(2)
                  << *average << " / 5\n" << std::defaultfloat;
    } else {
        std::cout << "\nNo ratings available.\n";
    }
}

void displayResults(const MovieDatabase& database, const std::vector<const Movie*>& movies)
{
    std::cout << movies.size() << " matching movies.\n";
    if (movies.empty()) std::cout << "No movies match your search.\n";
    for (const Movie* movie : movies) displayMovie(database, *movie);
}

std::string normalized(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool searchGenres(const MovieDatabase& database)
{
    std::cout << "Available genres: ";
    for (const auto& genre : Movie::availableGenres()) std::cout << genre << "; ";
    std::cout << '\n';
    std::string line;
    while (true) {
        std::cout << "Enter comma-separated genres (movies must match all): ";
        if (!std::getline(std::cin, line)) return false;
        std::vector<std::string> genres;
        bool valid = true;
        std::size_t start = 0;
        do {
            const auto end = line.find(',', start);
            const auto token = normalized(line.substr(start, end - start));
            const auto& available = Movie::availableGenres();
            const auto found = std::find_if(available.begin(), available.end(),
                [&token](const std::string& genre) { return normalized(genre) == token; });
            if (found == available.end()) {
                valid = false;
                break;
            }
            genres.push_back(*found);
            if (end == std::string::npos) break;
            start = end + 1;
        } while (true);
        if (valid) {
            displayResults(database, database.findMoviesByGenres(genres));
            return true;
        }
        std::cout << "Please enter one or more of the available genres, with no empty entries.\n";
    }
}
} // namespace

int main()
{
    try {
        const MovieDatabase database;
        std::cout << "Loaded " << database.getUsers().size() << " users, "
                  << database.getMovies().size() << " movies, and "
                  << database.getRatings().size() << " ratings.\n";
        while (true) {
            std::cout << "\n1. Find a movie by ID and recommend another\n"
                         "2. Search by genre\n3. Search by average rating\n0. Exit\n";
            int choice;
            if (!readNumber("Choose an option: ", 0, 3, choice) || choice == 0) break;
            if (choice == 1) {
                int movieId;
                if (!readNumber("Enter a movie ID (1-3952): ", 1, 3952, movieId)) break;
                const Movie* movie = database.findMovieById(movieId);
                if (!movie) {
                    std::cout << "No movie found with ID " << movieId << ".\n";
                    continue;
                }
                displayMovie(database, *movie);
                const Movie* recommendation = database.recommendMovie(movieId);
                if (recommendation) {
                    std::cout << "Recommended similar movie: " << recommendation->getTitle()
                              << " (ID: " << recommendation->getMovieId() << ")\n";
                } else {
                    std::cout << "No similar movie with overlapping ratings was found.\n";
                }
            } else if (choice == 2) {
                if (!searchGenres(database)) break;
            } else {
                double minimum, maximum;
                if (!readNumber("Minimum average rating (1-5): ", 1.0, 5.0, minimum)) break;
                if (!readNumber("Maximum average rating (minimum-5): ", minimum, 5.0, maximum)) break;
                displayResults(database, database.findMoviesByAverageRating(minimum, maximum));
            }
        }
        std::cout << "\nGoodbye.\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
