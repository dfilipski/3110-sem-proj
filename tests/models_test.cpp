#include "Movie.h"
#include "MovieDatabase.h"
#include "Rating.h"
#include "User.h"
#include <cassert>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <limits>
#include <fstream>
#include <iostream>
#include <stdexcept>

template <typename F>
void rejects(F action)
{
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}

template <typename T>
std::size_t checkFile(const char* path)
{
    std::ifstream input(path);
    assert(input.is_open());
    std::size_t count = 0;
    std::string line;
    while (std::getline(input, line)) {
        auto record = T::fromDatLine(line);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        assert(record.toDatLine() == line);
        ++count;
    }
    assert(!input.bad());
    return count;
}

int main()
{
    // A separate dataset gives exact averages and explicit unrated/missing cases.
    struct TemporaryDataset {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("movie-average-test-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        TemporaryDataset() { std::filesystem::create_directory(path); }
        ~TemporaryDataset() { std::filesystem::remove_all(path); }
    };
    {
        const TemporaryDataset fixture;
        std::ofstream(fixture.path / "users.dat") << "1::F::1::10::01234\n2::M::25::0::12345\n";
        std::ofstream(fixture.path / "movies.dat") <<
            "3952::Last::Drama|Comedy\n3::Unrated::Comedy\n1::First::Comedy\n";
        std::ofstream(fixture.path / "ratings.dat") <<
            "1::1::2::0\n2::1::5::0\n1::3952::1::0\n";
        const MovieDatabase small(fixture.path.string());
        assert(small.averageRating(1) == 3.5);
        assert(small.averageRating(3952) == 1.0);
        assert(!small.averageRating(3));
        assert(!small.averageRating(2));
        assert(!small.averageRating(-1));
        assert(!small.averageRating(0));
        assert(!small.averageRating(3953));
        const auto comedy = small.findMoviesByGenres({"Comedy"});
        assert(comedy.size() == 3 && comedy[0]->getMovieId() == 1 &&
               comedy[1]->getMovieId() == 3 && comedy[2]->getMovieId() == 3952);
        assert(comedy[0] == small.findMovieById(1));
        const auto both = small.findMoviesByGenres({"Drama", "Comedy"});
        assert(both.size() == 1 && both[0]->getMovieId() == 3952);
        assert(small.findMoviesByGenres({"Action"}).empty());
        assert(small.findMoviesByGenres({"Comedy", "Comedy"}).size() == 3);
        rejects([&] { small.findMoviesByGenres({}); });
        rejects([&] { small.findMoviesByGenres({"Unknown"}); });
        const auto rated = small.findMoviesByAverageRating(1, 5);
        assert(rated.size() == 2 && rated[0]->getMovieId() == 1 &&
               rated[1]->getMovieId() == 3952);
        assert(small.findMoviesByAverageRating(3.5, 3.5).front() == comedy[0]);
        assert(small.findMoviesByAverageRating(3.5001, 5).empty());
        assert(small.findMoviesByAverageRating(1, 1).size() == 1);
        rejects([&] { small.findMoviesByAverageRating(0, 5); });
        rejects([&] { small.findMoviesByAverageRating(1, 6); });
        rejects([&] { small.findMoviesByAverageRating(4, 3); });
        rejects([&] { small.findMoviesByAverageRating(std::numeric_limits<double>::quiet_NaN(), 5); });
        rejects([&] { small.findMoviesByAverageRating(1, std::numeric_limits<double>::infinity()); });
        // Empty ratings must leave every movie without an average.
        std::ofstream(fixture.path / "ratings.dat", std::ios::trunc).close();
        const MovieDatabase empty(fixture.path.string());
        assert(empty.findMoviesByAverageRating(1, 5).empty());
        assert(!empty.averageRating(1));
        assert(!empty.averageRating(3952));
    }
    const auto user = User::fromDatLine("1::F::1::10::01234\r");
    assert(user.getUserId() == 1 && user.getGender() == 'F');
    assert(user.getAge() == 1 && user.getAgeDescription() == "Under 18");
    assert(user.getOccupation() == 10 && user.getOccupationDescription() == "K-12 student");
    assert(user.getZipCode() == "01234");
    const auto movie = Movie::fromDatLine("1::Toy Story (1995)::Animation|Children's|Comedy");
    assert(movie.getMovieId() == 1 && movie.getTitle() == "Toy Story (1995)");
    assert(movie.getGenres().size() == 3 && movie.hasGenre("Children's"));
    assert(!movie.hasGenre("Horror"));
    const auto rating = Rating::fromDatLine("6040::3952::5::4102444800");
    assert(rating.getUserId() == 6040 && rating.getMovieId() == 3952);
    assert(rating.getRating() == 5 && rating.getTimestamp() == 4102444800LL);
    rejects([] { User::fromDatLine("0::F::1::10::48067"); });
    rejects([] { User::fromDatLine("1::Female::1::10::48067"); });
    rejects([] { User(1, 'M', 24, 0, "48067"); });
    rejects([] { User(1, 'M', 25, 21, "48067"); });
    rejects([] { Rating::fromDatLine("1::1::3.5::978300760"); });
    rejects([] { Rating(1, 1, 6, 0); });
    rejects([] { Rating(1, 1, 1, -1); });
    rejects([] { Rating::fromDatLine("1::1::1::9999999999999999999999"); });
    rejects([] { Movie::fromDatLine("1::Title::Unknown"); });
    rejects([] { Movie::fromDatLine("1::Title::Comedy|"); });
    rejects([] { Movie::fromDatLine("1::Title"); });
    rejects([] { Movie::fromDatLine("1x::Title::Comedy"); });
    assert(checkFile<User>("data/users.dat") == 6040);
    assert(checkFile<Movie>("data/movies.dat") == 3883);
    assert(checkFile<Rating>("data/ratings.dat") == 1000209);
    // Unsorted input, a gap in movie IDs, proportional rows, and disjoint users.
    const MovieRatingsCSR matrix({Rating(2, 3, 2, 0), Rating(1, 1, 3, 0),
                                  Rating(3, 4, 5, 0), Rating(2, 1, 3, 0),
                                  Rating(1, 3, 2, 0), Rating(1, 5, 5, 0)});
    assert(matrix.nonzeroCount() == 6);
    assert(std::abs(matrix.cosineSimilarity(1, 3) - 1.0) < 1e-12);
    assert(std::abs(matrix.cosineSimilarity(1, 5) - 1.0 / std::sqrt(2.0)) < 1e-12);
    assert(matrix.cosineSimilarity(1, 4) == 0);
    assert(matrix.cosineSimilarity(1, 2) == 0);
    assert(matrix.cosineSimilarity(2, 2) == 0);
    assert(MovieRatingsCSR({}).cosineSimilarity(1, 1) == 0);
    rejects([] { MovieRatingsCSR duplicate({Rating(1, 1, 3, 0), Rating(1, 1, 4, 1)}); });

    const MovieDatabase database;
    assert(database.getUsers().size() == 6040);
    assert(database.getMovies().size() == 3883);
    assert(database.getRatings().size() == 1000209);
    // Independently compute online means for every movie in the full dataset.
    std::vector<double> means(3953, 0);
    std::vector<std::size_t> counts(3953, 0);
    for (const auto& r : database.getRatings()) {
        const int id = r.getMovieId();
        means[id] += (r.getRating() - means[id]) / ++counts[id];
    }
    for (const auto& m : database.getMovies()) {
        const int id = m.getMovieId();
        const auto average = database.averageRating(id);
        assert(average.has_value() == (counts[id] != 0));
        if (average) assert(std::abs(*average - means[id]) < 1e-12);
    }
    assert(!database.averageRating(91));
    const Movie* found = database.findMovieById(1);
    assert(found && found->getTitle() == "Toy Story (1995)");
    assert(database.findMovieById(3952));
    assert(!database.findMovieById(0));
    assert(!database.findMovieById(3953));
    assert(!database.findMovieById(91)); // A gap in the MovieLens IDs.
    assert(!database.recommendMovie(91));
    assert(!database.recommendMovie(0));
    const Movie* recommendation = database.recommendMovie(1);
    assert(recommendation && recommendation->getMovieId() != 1);

    // Independent calculation from ratings, without CSR row traversal.
    std::vector<double> target(6041, 0), dot(3953, 0), norm(3953, 0);
    for (const auto& r : database.getRatings())
        if (r.getMovieId() == 1) target[r.getUserId()] = r.getRating();
    for (const auto& r : database.getRatings()) {
        dot[r.getMovieId()] += target[r.getUserId()] * r.getRating();
        norm[r.getMovieId()] += r.getRating() * r.getRating();
    }
    double bestScore = 0;
    int bestId = std::numeric_limits<int>::max();
    for (const auto& m : database.getMovies()) {
        int id = m.getMovieId();
        if (id == 1 || norm[id] == 0) continue;
        double score = dot[id] / (std::sqrt(norm[1]) * std::sqrt(norm[id]));
        if (score > bestScore || (score == bestScore && score > 0 && id < bestId)) {
            bestScore = score;
            bestId = id;
        }
    }
    assert(recommendation->getMovieId() == bestId);
    bool missingDirectory = false;
    try {
        MovieDatabase missing("data/missing-directory");
    } catch (const std::runtime_error& error) {
        missingDirectory = std::string(error.what()).find("users.dat") != std::string::npos;
    }
    assert(missingDirectory);
    std::cout << "Model validation and all dataset round trips passed.\n";
}
