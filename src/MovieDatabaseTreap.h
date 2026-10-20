#ifndef MOVIE_DATABASE_TREAP_H
#define MOVIE_DATABASE_TREAP_H

#include "IMovieDatabase.h"
#include "Rating.h"
#include "User.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

// Immutable movie index ordered by (movieId, unrounded average, canonical genre).
// One node per genre; movies are owned once, and returned pointers follow
// IMovieDatabase's lifetime contract. Copying is disabled; moving is supported.
class MovieDatabaseTreap : public IMovieDatabase {
public:
    explicit MovieDatabaseTreap(const std::string& dataDirectory = "data");
    ~MovieDatabaseTreap();
    MovieDatabaseTreap(const MovieDatabaseTreap&) = delete;
    MovieDatabaseTreap& operator=(const MovieDatabaseTreap&) = delete;
    MovieDatabaseTreap(MovieDatabaseTreap&&) noexcept;
    MovieDatabaseTreap& operator=(MovieDatabaseTreap&&) noexcept;

    const std::vector<User>& getUsers() const { return users_; }
    const std::vector<Movie>& getMovies() const { return movies_; }
    const std::vector<Rating>& getRatings() const { return ratings_; }
    // Expected O(log N), where N is the number of movie/genre entries.
    const Movie* findMovieById(int movieId) const override;
    std::optional<double> averageRating(int movieId) const override;
    // Subtree metadata prunes searches; worst-case O(N). Results are unique
    // and ordered by movie ID. Validation follows IMovieDatabase.
    std::vector<const Movie*> findMoviesByGenres(const std::vector<std::string>& genres) const override;
    std::vector<const Movie*> findMoviesByAverageRating(double minimum, double maximum) const override;

private:
    friend struct MovieDatabaseTreapTestAccess;
    using Key = std::tuple<int, std::optional<double>, std::string>;
    struct Node {
        Key key;
        std::size_t movieIndex;
        std::uint64_t priority;
        std::uint32_t movieGenres;
        std::uint32_t genres;
        double minimum;
        double maximum;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        Node(Key key, std::size_t index, std::uint64_t priority, std::uint32_t movieGenres);
    };
    static void update(Node& node);
    static void insert(std::unique_ptr<Node>& root, std::unique_ptr<Node> node);
    void search(const Node* node, std::uint32_t genres, double minimum, double maximum,
                bool byRating, std::vector<const Movie*>& results) const;
    std::vector<User> users_;
    std::vector<Movie> movies_;
    std::vector<Rating> ratings_;
    std::unordered_map<int, std::optional<double>> averageRatings_;
    std::unique_ptr<Node> root_;
};

#endif
