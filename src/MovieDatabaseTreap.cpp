#include "MovieDatabaseTreap.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace {
// Each model parses its own MovieLens record format.
template <typename T>
std::vector<T> loadRecords(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open " + path + ". Run the program from the project directory.");
    }

    std::vector<T> records;
    std::unordered_set<std::uint64_t> ratingKeys;
    std::unordered_set<int> ids;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        try {
            auto record = T::fromDatLine(line);
            if constexpr (std::is_same_v<T, Rating>) {
                const auto key = (static_cast<std::uint64_t>(record.getMovieId()) << 32) |
                                 static_cast<std::uint32_t>(record.getUserId());
                if (!ratingKeys.insert(key).second)
                    throw std::invalid_argument("Duplicate movie/user rating pair");
            }
            if constexpr (std::is_same_v<T, Movie>) {
                if (!ids.insert(record.getMovieId()).second)
                    throw std::invalid_argument("Duplicate movie ID");
            } else if constexpr (std::is_same_v<T, User>) {
                if (!ids.insert(record.getUserId()).second)
                    throw std::invalid_argument("Duplicate user ID");
            }
            records.push_back(std::move(record));
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    if (input.bad()) {
        throw std::runtime_error("Error reading " + path);
    }
    return records;
}

std::uint32_t genreBit(const std::string& genre)
{
    const auto& available = Movie::availableGenres();
    const auto found = std::find(available.begin(), available.end(), genre);
    if (found == available.end()) throw std::invalid_argument("Unknown movie genre: " + genre);
    return std::uint32_t{1} << std::distance(available.begin(), found);
}
} // namespace

MovieDatabaseTreap::MovieDatabaseTreap(const std::string& dataDirectory)
    : users_(loadRecords<User>((std::filesystem::path(dataDirectory) / "users.dat").string())),
      movies_(loadRecords<Movie>((std::filesystem::path(dataDirectory) / "movies.dat").string())),
      ratings_(loadRecords<Rating>((std::filesystem::path(dataDirectory) / "ratings.dat").string()))
{
    struct RatingTotals {
        double sum = 0;
        std::size_t count = 0;
    };
    std::unordered_map<int, RatingTotals> totals;
    for (const auto& rating : ratings_) {
        auto& total = totals[rating.getMovieId()];
        total.sum += rating.getRating();
        ++total.count;
    }
    for (const auto& movie : movies_) {
        const int id = movie.getMovieId();
        const auto total = totals.find(id);
        averageRatings_[id] = total == totals.end()
            ? std::nullopt
            : std::optional<double>(total->second.sum / total->second.count);
    }
    std::mt19937_64 priorities(0x4d6f766965ULL);
    for (std::size_t i = 0; i < movies_.size(); ++i) {
        const auto& movie = movies_[i];
        std::uint32_t mask = 0;
        for (const auto& genre : movie.getGenres()) mask |= genreBit(genre);
        for (const auto& genre : movie.getGenres())
            insert(root_, std::make_unique<Node>(
                Key{movie.getMovieId(), averageRating(movie.getMovieId()), genre}, i, priorities(), mask));
    }
}

std::optional<double> MovieDatabaseTreap::averageRating(int movieId) const
{
    const auto average = averageRatings_.find(movieId);
    return average == averageRatings_.end() ? std::nullopt : average->second;
}

MovieDatabaseTreap::~MovieDatabaseTreap() = default;
MovieDatabaseTreap::MovieDatabaseTreap(MovieDatabaseTreap&&) noexcept = default;
MovieDatabaseTreap& MovieDatabaseTreap::operator=(MovieDatabaseTreap&&) noexcept = default;

MovieDatabaseTreap::Node::Node(Key value, std::size_t index, std::uint64_t rank, std::uint32_t mask)
    : key(std::move(value)), movieIndex(index), priority(rank),
      movieGenres(mask), genres(mask),
      minimum(std::get<1>(key).value_or(std::numeric_limits<double>::infinity())),
      maximum(std::get<1>(key).value_or(-std::numeric_limits<double>::infinity()))
{
}

void MovieDatabaseTreap::update(Node& node)
{
    node.genres = node.movieGenres;
    node.minimum = std::get<1>(node.key).value_or(std::numeric_limits<double>::infinity());
    node.maximum = std::get<1>(node.key).value_or(-std::numeric_limits<double>::infinity());
    for (const Node* child : {node.left.get(), node.right.get()}) {
        if (!child) continue;
        node.genres |= child->genres;
        node.minimum = std::min(node.minimum, child->minimum);
        node.maximum = std::max(node.maximum, child->maximum);
    }
}

void MovieDatabaseTreap::insert(std::unique_ptr<Node>& root, std::unique_ptr<Node> node)
{
    if (!root) {
        root = std::move(node);
        return;
    }
    if (node->key == root->key) return;
    const bool left = node->key < root->key;
    auto& child = left ? root->left : root->right;
    insert(child, std::move(node));
    if (child->priority > root->priority) {
        auto promoted = std::move(child);
        child = left ? std::move(promoted->right) : std::move(promoted->left);
        update(*root);
        if (left) promoted->right = std::move(root);
        else promoted->left = std::move(root);
        root = std::move(promoted);
    }
    update(*root);
}

const Movie* MovieDatabaseTreap::findMovieById(int movieId) const
{
    const Node* node = root_.get();
    while (node) {
        const int id = std::get<0>(node->key);
        if (id == movieId) return &movies_[node->movieIndex];
        node = movieId < id ? node->left.get() : node->right.get();
    }
    return nullptr;
}

void MovieDatabaseTreap::search(const Node* node, std::uint32_t genres,
                               double minimum, double maximum, bool byRating,
                               std::vector<const Movie*>& results) const
{
    if (!node || (node->genres & genres) != genres) return;
    if (byRating && (node->minimum > maximum || node->maximum < minimum)) return;
    search(node->left.get(), genres, minimum, maximum, byRating, results);
    const auto& movie = movies_[node->movieIndex];
    const auto average = std::get<1>(node->key);
    bool matches = !byRating || (average && *average >= minimum && *average <= maximum);
    if (!byRating) {
        matches = (node->movieGenres & genres) == genres;
    }
    // In-order traversal groups entries for the same movie, so deduplication
    // preserves ID order without a separate sort or hash set.
    if (matches && (results.empty() || results.back() != &movie)) results.push_back(&movie);
    search(node->right.get(), genres, minimum, maximum, byRating, results);
}

std::vector<const Movie*> MovieDatabaseTreap::findMoviesByGenres(const std::vector<std::string>& genres) const
{
    if (genres.empty()) throw std::invalid_argument("Select at least one genre");
    std::uint32_t mask = 0;
    for (const auto& genre : genres) mask |= genreBit(genre);
    std::vector<const Movie*> results;
    search(root_.get(), mask, 1, 5, false, results);
    return results;
}

std::vector<const Movie*> MovieDatabaseTreap::findMoviesByAverageRating(double minimum, double maximum) const
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
        minimum < 1 || maximum > 5 || minimum > maximum)
        throw std::invalid_argument("Rating range must satisfy 1 <= minimum <= maximum <= 5");
    std::vector<const Movie*> results;
    search(root_.get(), 0, minimum, maximum, true, results);
    return results;
}
