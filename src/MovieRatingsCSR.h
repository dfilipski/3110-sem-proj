#ifndef MOVIE_RATINGS_CSR_H
#define MOVIE_RATINGS_CSR_H

#include "Rating.h"
#include <cstddef>
#include <vector>

// Movie-by-user matrix: row = movie ID - 1, column = user ID - 1.
// Missing ratings occupy no storage and count as zero in cosine similarity.
class MovieRatingsCSR {
public:
    explicit MovieRatingsCSR(const std::vector<Rating>& ratings);
    double cosineSimilarity(int firstMovieId, int secondMovieId) const;
    std::size_t nonzeroCount() const { return values_.size(); }

private:
    static constexpr int movieCount = 3952;
    std::vector<std::size_t> rowOffsets_;
    std::vector<int> columnIndices_;
    std::vector<double> values_;
    std::vector<double> rowNorms_;
};

#endif
