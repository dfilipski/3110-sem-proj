#include "IMovieDatabase.h"
#include "MovieDatabase.h"
#include "MovieDatabaseMap.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

// Register future implementations here; all measured queries use IMovieDatabase.
std::unique_ptr<IMovieDatabase> createDatabase(const std::string& implementation,
                                             const std::string& dataDirectory)
{
    if (implementation == "MovieDatabase") {
        return std::make_unique<MovieDatabase>(dataDirectory);
    }
    if (implementation == "MovieDatabaseMap") {
        return std::make_unique<MovieDatabaseMap>(dataDirectory);
    }
    throw std::invalid_argument("Unknown implementation: " + implementation);
}

int parseRounds(const std::string& text)
{
    int rounds = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), rounds);
    if (result.ec != std::errc() || result.ptr != text.data() + text.size() || rounds <= 0) {
        throw std::invalid_argument("Rounds must be a positive integer");
    }
    return rounds;
}

template <typename Query>
void benchmark(const char* name, int rounds, std::size_t queriesPerRound, Query query)
{
    // Warm each workload once before timing. Consume and print results so the
    // compiler cannot discard the queries. No terminal output occurs while timed.
    std::uint64_t checksum = query();
    const auto start = Clock::now();
    for (int round = 0; round < rounds; ++round) {
        checksum += query();
    }
    const double milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    const auto calls = static_cast<std::uint64_t>(rounds) * queriesPerRound;
    std::cout << name << ": " << calls << " calls, " << milliseconds << " ms total, "
              << milliseconds * 1000000.0 / calls << " ns/call, checksum=" << checksum << '\n';
}
}

int main(int argc, char* argv[])
{
    try {
        if (argc > 4) {
            throw std::invalid_argument("Too many arguments");
        }
        const std::string implementation = argc > 1 ? argv[1] : "MovieDatabase";
        const int rounds = argc > 2 ? parseRounds(argv[2]) : 10;
        const std::string dataDirectory = argc > 3 ? argv[3] : "data";
        const auto start = Clock::now();
        const auto storage = createDatabase(implementation, dataDirectory);
        const double loadMilliseconds =
            std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        const IMovieDatabase& database = *storage;
        std::cout << std::fixed << std::setprecision(3)
                  << "Implementation: " << implementation << ", rounds: " << rounds
                  << ", data: " << dataDirectory << '\n'
                  << "Construction/loading: " << loadMilliseconds << " ms\n";

        // Include every MovieLens ID (including gaps) and two out-of-range IDs.
        constexpr int lastMovieId = 3952;
        benchmark("findMovieById", rounds, lastMovieId + 2, [&] {
            std::uint64_t checksum = 0;
            for (int id = 0; id <= lastMovieId + 1; ++id) {
                const Movie* movie = database.findMovieById(id);
                if (movie) checksum += movie->getMovieId();
            }
            return checksum;
        });
        benchmark("averageRating", rounds, lastMovieId + 2, [&] {
            std::uint64_t checksum = 0;
            for (int id = 0; id <= lastMovieId + 1; ++id) {
                const auto average = database.averageRating(id);
                if (average) checksum += static_cast<std::uint64_t>(*average * 1000000.0);
            }
            return checksum;
        });

        std::vector<std::vector<std::string>> genreQueries;
        for (const auto& genre : Movie::availableGenres()) {
            genreQueries.push_back({genre});
        }
        genreQueries.push_back({"Comedy", "Romance"});
        genreQueries.push_back({"Action", "Adventure", "Sci-Fi"});
        benchmark("findMoviesByGenres", rounds, genreQueries.size(), [&] {
            std::uint64_t checksum = 0;
            for (const auto& genres : genreQueries) {
                const auto results = database.findMoviesByGenres(genres);
                checksum += results.size();
                for (const Movie* movie : results) checksum += movie->getMovieId();
            }
            return checksum;
        });

        const std::vector<std::pair<double, double>> ratingQueries = {
            {1, 5}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {3.5, 4.0}, {5, 5}
        };
        benchmark("findMoviesByAverageRating", rounds, ratingQueries.size(), [&] {
            std::uint64_t checksum = 0;
            for (const auto& bounds : ratingQueries) {
                const auto results = database.findMoviesByAverageRating(bounds.first, bounds.second);
                checksum += results.size();
                for (const Movie* movie : results) checksum += movie->getMovieId();
            }
            return checksum;
        });
    } catch (const std::exception& error) {
        std::cerr << "Benchmark error: " << error.what() << '\n'
                  << "Usage: " << argv[0] << " [implementation [rounds [data-directory]]]\n"
                  << "Available implementations: MovieDatabase, MovieDatabaseMap\n";
        return 1;
    }
    return 0;
}
