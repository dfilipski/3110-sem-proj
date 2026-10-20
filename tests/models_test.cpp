#include "Movie.h"
#include "MovieDatabase.h"
#include "MovieDatabaseMap.h"
#include "MovieDatabaseTreap.h"
#include <algorithm>
#include <utility>
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
#include <type_traits>

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

static_assert(std::is_abstract<IMovieDatabase>::value, "The query interface must be abstract");
static_assert(std::has_virtual_destructor<IMovieDatabase>::value,
              "The interface must support polymorphic destruction");
static_assert(!std::is_abstract<MovieDatabase>::value,
              "MovieDatabase must implement every interface method");

static_assert(!std::is_abstract<MovieDatabaseMap>::value,
              "MovieDatabaseMap must implement every interface method");

// Inspect structural invariants independently of insertion and metadata updates.
struct MovieDatabaseTreapTestAccess {
    using Node = MovieDatabaseTreap::Node;
    using Key = MovieDatabaseTreap::Key;
    struct Summary {
        std::uint32_t genres = 0;
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = -std::numeric_limits<double>::infinity();
        std::size_t count = 0;
    };
    static Summary inspect(const Node* node, const Key* lower, const Key* upper,
                           const MovieDatabaseTreap& database)
    {
        if (!node) return {};
        assert(!lower || *lower < node->key);
        assert(!upper || node->key < *upper);
        assert(!node->left || node->priority >= node->left->priority);
        assert(!node->right || node->priority >= node->right->priority);
        const auto& movie = database.movies_.at(node->movieIndex);
        assert(movie.getMovieId() == std::get<0>(node->key));
        assert(movie.hasGenre(std::get<2>(node->key)));
        assert(database.averageRating(movie.getMovieId()) == std::get<1>(node->key));
        Summary result;
        const auto& genres = Movie::availableGenres();
        for (std::size_t i = 0; i < genres.size(); ++i)
            if (movie.hasGenre(genres[i])) result.genres |= std::uint32_t{1} << i;
        assert(node->movieGenres == result.genres);
        if (std::get<1>(node->key)) result.minimum = result.maximum = *std::get<1>(node->key);
        result.count = 1;
        for (auto child : {inspect(node->left.get(), lower, &node->key, database),
                           inspect(node->right.get(), &node->key, upper, database)}) {
            result.genres |= child.genres;
            result.minimum = std::min(result.minimum, child.minimum);
            result.maximum = std::max(result.maximum, child.maximum);
            result.count += child.count;
        }
        assert(node->genres == result.genres);
        assert(node->minimum == result.minimum && node->maximum == result.maximum);
        return result;
    }
    static void check(const MovieDatabaseTreap& database)
    {
        std::size_t expected = 0;
        for (const auto& movie : database.movies_) expected += movie.getGenres().size();
        assert(inspect(database.root_.get(), nullptr, nullptr, database).count == expected);
    }
};

static_assert(!std::is_abstract<MovieDatabaseTreap>::value, "Treap implements every query");
static_assert(!std::is_copy_constructible<MovieDatabaseTreap>::value, "Treap is move-only");
static_assert(std::is_nothrow_move_constructible<MovieDatabaseTreap>::value, "Treap supports moving");

void sameResults(const std::vector<const Movie*>& expected, const std::vector<const Movie*>& actual,
                 const IMovieDatabase& database)
{
    assert(expected.size() == actual.size());
    int previous = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(expected[i]->toDatLine() == actual[i]->toDatLine());
        assert(actual[i] == database.findMovieById(actual[i]->getMovieId()));
        assert(actual[i]->getMovieId() > previous);
        previous = actual[i]->getMovieId();
    }
}

void compareQueries(const IMovieDatabase& expected, const IMovieDatabase& actual)
{
    for (int id = -1; id <= 3953; ++id) {
        const auto a = expected.findMovieById(id);
        const auto b = actual.findMovieById(id);
        assert(bool(a) == bool(b));
        if (a) assert(a->toDatLine() == b->toDatLine());
        assert(expected.averageRating(id) == actual.averageRating(id));
    }
    for (const auto& first : Movie::availableGenres()) {
        sameResults(expected.findMoviesByGenres({first}), actual.findMoviesByGenres({first}), actual);
        for (const auto& second : Movie::availableGenres())
            sameResults(expected.findMoviesByGenres({first, second}),
                        actual.findMoviesByGenres({first, second}), actual);
    }
    for (const auto& bounds : std::vector<std::pair<double, double>>{
             {1, 5}, {1, 1}, {5, 5}, {3.5, 3.5}, {3.5001, 5}, {1, 2}, {2, 3}, {3, 4}, {4, 5}})
        sameResults(expected.findMoviesByAverageRating(bounds.first, bounds.second),
                    actual.findMoviesByAverageRating(bounds.first, bounds.second), actual);
    rejects([&] { actual.findMoviesByGenres({}); });
    rejects([&] { actual.findMoviesByGenres({"Unknown"}); });
    rejects([&] { actual.findMoviesByAverageRating(0, 5); });
    rejects([&] { actual.findMoviesByAverageRating(1, 6); });
    rejects([&] { actual.findMoviesByAverageRating(4, 3); });
    rejects([&] { actual.findMoviesByAverageRating(1, std::numeric_limits<double>::infinity()); });
    rejects([&] { actual.findMoviesByAverageRating(std::numeric_limits<double>::quiet_NaN(), 5); });
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
        const MovieDatabase storage(fixture.path.string());
        const MovieDatabaseMap mapStorage(fixture.path.string());
        MovieDatabaseTreap treap(fixture.path.string());
        MovieDatabaseTreapTestAccess::check(treap);
        compareQueries(storage, treap);
        compareQueries(storage, mapStorage);
        const auto* original = treap.findMovieById(3952);
        MovieDatabaseTreap moved(std::move(treap));
        assert(moved.findMovieById(3952) == original);
        MovieDatabaseTreap assigned(fixture.path.string());
        assigned = std::move(moved);
        assert(assigned.findMovieById(3952) == original);
        MovieDatabaseTreapTestAccess::check(assigned);
        compareQueries(storage, assigned);
        const IMovieDatabase& mapped = mapStorage;
        assert(mapStorage.getMovies().size() == 3);
        assert(mapStorage.getUsers().at(1).getZipCode() == "01234");
        assert(mapStorage.getRatings().at({1, 1}).getRating() == 2);
        assert(mapStorage.getRatings().at({1, 2}).getRating() == 5);
        assert(mapStorage.getRatings().at({3952, 1}).getRating() == 1);
        assert(mapped.findMovieById(3952)->getTitle() == "Last");
        assert(!mapped.findMovieById(2));
        assert(!mapped.findMovieById(0));
        assert(!mapped.findMovieById(3953));
        assert(mapped.averageRating(1) == 3.5);
        assert(!mapped.averageRating(3));
        assert(!mapped.averageRating(2));
        const auto mapComedy = mapped.findMoviesByGenres({"Comedy"});
        assert(mapComedy.size() == 3 && mapComedy[0]->getMovieId() == 1 &&
               mapComedy[1]->getMovieId() == 3 && mapComedy[2]->getMovieId() == 3952);
        assert(mapComedy[0] == mapped.findMovieById(1));
        assert(mapped.findMoviesByGenres({"Comedy", "Drama"}).size() == 1);
        assert(mapped.findMoviesByGenres({"Action"}).empty());
        const auto mapRated = mapped.findMoviesByAverageRating(1, 5);
        assert(mapRated.size() == 2 && mapRated[0]->getMovieId() == 1 &&
               mapRated[1]->getMovieId() == 3952);
        rejects([&] { mapped.findMoviesByGenres({}); });
        rejects([&] { mapped.findMoviesByGenres({"Unknown"}); });
        rejects([&] { mapped.findMoviesByAverageRating(4, 3); });
        rejects([&] { mapped.findMoviesByAverageRating(0, 5); });
        rejects([&] { mapped.findMoviesByAverageRating(1, 6); });
        rejects([&] { mapped.findMoviesByAverageRating(1, std::numeric_limits<double>::infinity()); });
        rejects([&] { mapped.findMoviesByAverageRating(std::numeric_limits<double>::quiet_NaN(), 5); });
        const IMovieDatabase& small = storage;
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
        const MovieDatabaseTreap unrated(fixture.path.string());
        MovieDatabaseTreapTestAccess::check(unrated);
        compareQueries(empty, unrated);
        const MovieDatabaseMap emptyRatings(fixture.path.string());
        assert(!emptyRatings.averageRating(1));
        assert(emptyRatings.findMoviesByAverageRating(1, 5).empty());

        std::ofstream(fixture.path / "movies.dat") << "1::First::Comedy\n1::Duplicate::Drama\n";
        bool duplicateRejected = false;
        try { MovieDatabaseMap duplicate(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            duplicateRejected = std::string(error.what()).find("movies.dat:2:") != std::string::npos;
        }
        assert(duplicateRejected);
        bool treapduplicateRejected = false;
        try { MovieDatabaseTreap invalid(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            treapduplicateRejected = std::string(error.what()).find("movies.dat:2:") != std::string::npos;
        }
        assert(treapduplicateRejected);
        std::ofstream(fixture.path / "movies.dat") << "1::First::Comedy\ninvalid\n";
        bool malformedRejected = false;
        try { MovieDatabaseMap malformed(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            malformedRejected = std::string(error.what()).find("movies.dat:2:") != std::string::npos;
        }
        assert(malformedRejected);
        bool treapmalformedRejected = false;
        try { MovieDatabaseTreap invalid(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            treapmalformedRejected = std::string(error.what()).find("movies.dat:2:") != std::string::npos;
        }
        assert(treapmalformedRejected);
        std::ofstream(fixture.path / "movies.dat", std::ios::trunc).close();
        std::ofstream(fixture.path / "users.dat") <<
            "1::F::1::10::01234\n1::M::25::0::12345\n";
        bool duplicateUserRejected = false;
        try { MovieDatabaseMap duplicateUser(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            duplicateUserRejected = std::string(error.what()).find("users.dat:2:") != std::string::npos;
        }
        assert(duplicateUserRejected);
        bool treapduplicateUserRejected = false;
        try { MovieDatabaseTreap invalid(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            treapduplicateUserRejected = std::string(error.what()).find("users.dat:2:") != std::string::npos;
        }
        assert(treapduplicateUserRejected);
        std::ofstream(fixture.path / "users.dat", std::ios::trunc).close();
        std::ofstream(fixture.path / "ratings.dat") << "1::1::3::0\n1::1::4::1\n";
        bool duplicateRatingRejected = false;
        try { MovieDatabaseMap duplicateRating(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            duplicateRatingRejected = std::string(error.what()).find("ratings.dat:2:") != std::string::npos;
        }
        assert(duplicateRatingRejected);
        bool treapduplicateRatingRejected = false;
        try { MovieDatabaseTreap invalid(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            treapduplicateRatingRejected = std::string(error.what()).find("ratings.dat:2:") != std::string::npos;
        }
        assert(treapduplicateRatingRejected);
        bool duplicateVectorRatingRejected = false;
        try { MovieDatabase duplicateRating(fixture.path.string()); }
        catch (const std::runtime_error& error) {
            duplicateVectorRatingRejected = std::string(error.what()).find("ratings.dat:2:") != std::string::npos;
        }
        assert(duplicateVectorRatingRejected);
        std::ofstream(fixture.path / "ratings.dat", std::ios::trunc).close();
        const MovieDatabaseTreap emptyTreap(fixture.path.string());
        MovieDatabaseTreapTestAccess::check(emptyTreap);
        const MovieDatabaseMap emptyMap(fixture.path.string());
        compareQueries(emptyMap, emptyTreap);
        // Equal, nonterminating averages and monotonic insertion exercise rotations.
        std::ofstream movies(fixture.path / "movies.dat");
        std::ofstream ratings(fixture.path / "ratings.dat");
        for (int id = 1; id <= 128; ++id) {
            movies << id << "::Synthetic::Comedy|Drama\n";
            ratings << "1::" << id << "::2::0\n2::" << id << "::4::0\n3::" << id << "::4::0\n";
        }
        movies.close();
        ratings.close();
        const MovieDatabaseTreap sorted(fixture.path.string());
        const MovieDatabase reference(fixture.path.string());
        MovieDatabaseTreapTestAccess::check(sorted);
        compareQueries(reference, sorted);
        assert(sorted.findMoviesByAverageRating(10.0 / 3, 10.0 / 3).size() == 128);
        assert(sorted.findMoviesByGenres({"Comedy", "Drama"}).size() == 128);
        assert(emptyMap.getUsers().empty());
        assert(emptyMap.getRatings().empty());
        assert(emptyMap.getMovies().empty());
        assert(!emptyMap.findMovieById(1));
        assert(emptyMap.findMoviesByGenres({"Comedy"}).empty());
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
    const MovieDatabaseMap mapDatabase;
    assert(mapDatabase.getMovies().size() == 3883);
    assert(mapDatabase.getUsers().size() == 6040);
    assert(mapDatabase.getRatings().size() == 1000209);
    assert(!mapDatabase.findMovieById(91));
    const MovieDatabaseTreap treapDatabase;
    MovieDatabaseTreapTestAccess::check(treapDatabase);
    assert(treapDatabase.getMovies().size() == 3883);
    assert(treapDatabase.getUsers().size() == 6040);
    assert(treapDatabase.getRatings().size() == 1000209);
    const MovieDatabase database;
    compareQueries(database, treapDatabase);
    assert(database.getUsers().size() == 6040);
    assert(database.getMovies().size() == 3883);
    assert(database.getRatings().size() == 1000209);
    for (const auto& user : database.getUsers())
        assert(mapDatabase.getUsers().at(user.getUserId()).toDatLine() == user.toDatLine());
    for (const auto& rating : database.getRatings())
        assert(mapDatabase.getRatings().at({rating.getMovieId(), rating.getUserId()}).toDatLine() ==
               rating.toDatLine());
    // Independently compute online means for every movie in the full dataset.
    std::vector<double> means(3953, 0);
    std::vector<std::size_t> counts(3953, 0);
    for (const auto& r : database.getRatings()) {
        const int id = r.getMovieId();
        means[id] += (r.getRating() - means[id]) / ++counts[id];
    }
    for (const auto& m : database.getMovies()) {
        const int id = m.getMovieId();
        assert(mapDatabase.findMovieById(id)->toDatLine() == m.toDatLine());
        assert(mapDatabase.averageRating(id) == database.averageRating(id));
        assert(treapDatabase.averageRating(id) == database.averageRating(id));
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
    bool missingDirectory = false;
    try {
        MovieDatabase missing("data/missing-directory");
    } catch (const std::runtime_error& error) {
        missingDirectory = std::string(error.what()).find("users.dat") != std::string::npos;
    }
    assert(missingDirectory);
    bool missingTreap = false;
    try { MovieDatabaseTreap missing("data/missing-directory"); }
    catch (const std::runtime_error& error) {
        missingTreap = std::string(error.what()).find("users.dat") != std::string::npos;
    }
    assert(missingTreap);
    std::cout << "Model validation and all dataset round trips passed.\n";
}
