#include "MovieDatabase.h"
#include "MovieDatabaseMap.h"
#include "MovieDatabaseTreap.h"
#include <cassert>
#include <iostream>
void compare(const IMovieDatabase& a, const IMovieDatabase& b)
{
    for (int id = 0; id <= 3953; ++id) {
        const auto x = a.findMovieById(id), y = b.findMovieById(id);
        assert(bool(x) == bool(y));
        if (x) assert(x->toDatLine() == y->toDatLine());
        assert(a.averageRating(id) == b.averageRating(id));
    }
    auto same = [](const auto& x, const auto& y) {
        assert(x.size() == y.size());
        for (std::size_t i = 0; i < x.size(); ++i)
            assert(x[i]->getMovieId() == y[i]->getMovieId());
    };
    std::vector<std::vector<std::string>> queries;
    for (const auto& genre : Movie::availableGenres()) queries.push_back({genre});
    queries.push_back({"Comedy", "Romance"});
    queries.push_back({"Action", "Adventure", "Sci-Fi"});
    for (const auto& genres : queries) same(a.findMoviesByGenres(genres), b.findMoviesByGenres(genres));
    for (const auto bounds : {std::pair<double,double>{1,5}, {1,2}, {2,3}, {3,4}, {4,5}, {3.5,4}, {5,5}})
        same(a.findMoviesByAverageRating(bounds.first,bounds.second), b.findMoviesByAverageRating(bounds.first,bounds.second));
}
int main(int argc, char** argv)
{
    assert(argc == 2);
    MovieDatabase a(argv[1]); MovieDatabaseMap b(argv[1]); MovieDatabaseTreap c(argv[1]);
    compare(a,b); compare(a,c);
    std::cout << "All workload results agree exactly.\n";
}
