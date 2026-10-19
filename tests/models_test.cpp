#include "Movie.h"
#include "Rating.h"
#include "User.h"
#include <cassert>
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
    std::cout << "Model validation and all dataset round trips passed.\n";
}
