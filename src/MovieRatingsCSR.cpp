#include "MovieRatingsCSR.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

MovieRatingsCSR::MovieRatingsCSR(const std::vector<Rating>& ratings)
    : rowOffsets_(movieCount + 1, 0), rowNorms_(movieCount, 0.0)
{
    // Sort a temporary copy so each CSR row has ascending user columns.
    auto sorted = ratings;
    std::sort(sorted.begin(), sorted.end(), [](const Rating& a, const Rating& b) {
        if (a.getMovieId() != b.getMovieId()) return a.getMovieId() < b.getMovieId();
        return a.getUserId() < b.getUserId();
    });
    columnIndices_.reserve(sorted.size());
    values_.reserve(sorted.size());
    int previousMovie = 0;
    int previousUser = 0;
    for (const auto& rating : sorted) {
        const int movie = rating.getMovieId();
        const int user = rating.getUserId();
        if (movie == previousMovie && user == previousUser)
            throw std::invalid_argument("Duplicate rating for a movie/user pair");
        ++rowOffsets_[movie];
        columnIndices_.push_back(user - 1);
        const double value = rating.getRating();
        values_.push_back(value);
        rowNorms_[movie - 1] += value * value;
        previousMovie = movie;
        previousUser = user;
    }
    for (int i = 1; i <= movieCount; ++i) rowOffsets_[i] += rowOffsets_[i - 1];
    for (double& norm : rowNorms_) norm = std::sqrt(norm);
}

double MovieRatingsCSR::cosineSimilarity(int firstMovieId, int secondMovieId) const
{
    if (firstMovieId < 1 || firstMovieId > movieCount ||
        secondMovieId < 1 || secondMovieId > movieCount)
        throw std::out_of_range("Movie ID must be 1–3952");
    const int a = firstMovieId - 1;
    const int b = secondMovieId - 1;
    if (rowNorms_[a] == 0 || rowNorms_[b] == 0) return 0;

    double dot = 0;
    std::size_t i = rowOffsets_[a];
    std::size_t j = rowOffsets_[b];
    while (i < rowOffsets_[a + 1] && j < rowOffsets_[b + 1]) {
        if (columnIndices_[i] == columnIndices_[j]) {
            dot += values_[i++] * values_[j++];
        } else if (columnIndices_[i] < columnIndices_[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return std::clamp(dot / (rowNorms_[a] * rowNorms_[b]), 0.0, 1.0);
}
