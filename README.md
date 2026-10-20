# Movie Search and Recommendation

A C++17 command-line program for exploring the MovieLens 1M dataset. Look up a
movie and get a similar recommendation, find movies that share selected genres,
or search by average user rating.

## Requirements

- A C++17 compiler with standard-library filesystem support. The Makefile uses
  `clang++` by default.
- `make`.
- The MovieLens data files in `data/`: `users.dat`, `movies.dat`, and `ratings.dat`.

The program uses the C++ standard library and requires no additional libraries.

## Build and run

Run these commands from the repository root, since dataset paths are relative to
that directory:

```sh
make
make run
```

You can also launch the compiled executable directly:

```sh
./build/main
```

To use another compiler:

```sh
make clean
make CXX=g++
```

The default build enables C++17, compiler warnings (`-Wall -Wextra`), and debugging
information (`-g`). `make clean` removes the generated `build/` directory. Rebuild
from clean output after changing compiler flags or compilers.

## Using the program

The dataset loads once at startup. The program displays the loaded record counts
and presents this menu:

```text
1. Find a movie by ID and recommend another
2. Search by genre
3. Search by average rating
0. Exit
```

After each search, the menu appears again. Choose `0` to quit. End-of-file also
exits cleanly (Ctrl-D on macOS/Linux). Invalid input prompts you to try again.

### 1. Movie lookup and recommendation

Enter a whole-number movie ID between **1 and 3952**. Movie IDs contain gaps, so
some IDs within that range have no matching movie.

The program displays the movie's ID, full title, genres, and average user rating,
then recommends a similar movie when one is available. Movies without ratings
show a message instead of an average.

For example, to look up movie `1` and then exit:

```sh
printf '1\n1\n0\n' | ./build/main
```

### 2. Genre search

Enter one or more genres separated by commas:

```text
Comedy, Romance
```

A movie must contain **all** selected genres to match. Capitalization and spaces
around each genre are ignored, so ` comedy , ROMANCE ` works too. Unknown genres
and empty entries, such as `Comedy,`, are rejected.

Supported genres:

```text
Action, Adventure, Animation, Children's, Comedy, Crime, Documentary,
Drama, Fantasy, Film-Noir, Horror, Musical, Mystery, Romance, Sci-Fi,
Thriller, War, Western
```

### 3. Average-rating search

Enter a minimum and maximum average rating. Both bounds are inclusive, must be
between **1 and 5**, and the maximum must be at least the minimum. Decimal values
are accepted; for example, a minimum of `3.5` and maximum of `4.0`.

Averages use only ratings actually submitted for a movie. Unrated movies are
excluded. Filtering uses the unrounded average, while results display averages
to two decimal places. Setting both bounds to the same value searches for that
exact numeric average, rather than its rounded display value.

Both genre and rating searches return every match in ascending movie-ID order,
including the ID, full title, genres, and average rating when available. A message
indicates when there are no matches. Large searches can produce substantial
terminal output.

## How recommendations work

Each movie is represented by a sparse vector of user ratings. The program uses
cosine similarity to compare the selected movie with other movies:

```text
similarity(A, B) = dot(A, B) / (norm(A) * norm(B))
```

Missing ratings count as zero for this calculation. Norms use each movie's full
rating vector, rather than only users who rated both movies. The sparse matrix
uses compressed sparse row (CSR) storage to avoid a dense movie-by-user matrix.

The recommendation excludes the selected movie and requires positive similarity.
If two candidates have equal similarity, the smaller movie ID wins. Unknown
movies or movies with no positive candidate produce no recommendation. This is
movie-to-movie similarity based on raw ratings, rather than a personalized
recommendation for an individual user.

## Dataset

The supplied MovieLens 1M files contain:

| File | Records | Format |
| --- | ---: | --- |
| `data/users.dat` | 6,040 | `UserID::Gender::Age::Occupation::Zip-code` |
| `data/movies.dat` | 3,883 | `MovieID::Title::Genres` |
| `data/ratings.dat` | 1,000,209 | `UserID::MovieID::Rating::Timestamp` |

Genres within a movie record are separated by `|`. Ratings are whole stars from
1 through 5; timestamps are nonnegative Unix seconds. Movie titles retain their
release years, ZIP codes retain leading zeroes, and user ages are categorical
codes. The models validate records, and malformed dataset errors include the
file path and line number.

The current implementation is tailored to MovieLens 1M: movie IDs range from
1–3952 and user IDs from 1–6040. Replacing the files with another dataset may
require changes to validation, matrix dimensions, prompts, and tests. Preserve
the supplied data files when developing; use separate fixtures for test cases.

## Project structure

| Path | Responsibility |
| --- | --- |
| `src/main.cpp` | Menu, input validation, and terminal output |
| `src/Movie.h`, `src/Movie.cpp` | Movie records, genre names, and genre membership |
| `src/User.h`, `src/User.cpp` | User records and categorical descriptions |
| `src/Rating.h`, `src/Rating.cpp` | Rating records |
| `src/DataRecord.h` | Shared delimiter splitting and strict integer parsing |
| `src/MovieDatabase.h`, `src/MovieDatabase.cpp` | Dataset loading, cached averages, searches, and recommendations |
| `src/MovieRatingsCSR.h`, `src/MovieRatingsCSR.cpp` | Sparse ratings matrix and cosine similarity |
| `tests/models_test.cpp` | Model, search, recommendation, and dataset integration tests |
| `Makefile` | Application and test build targets |
| `data/` | MovieLens source data |
| `build/` | Generated executables, object files, and dependency files |

## Tests

```sh
make test
```

This builds and runs `build/models_test`. Tests cover record validation,
serialization round trips across the full dataset, rating averages, genre and
rating searches, sparse similarity calculations, and recommendations. Small
synthetic datasets cover edge cases without editing the supplied files.

Tests use `assert`; do not compile them with `-DNDEBUG`. For CLI changes, also
exercise interactive input, invalid entries, repeated searches, and EOF.

## Troubleshooting

- **Cannot open `data/...`:** Run the executable from the repository root and
  check that all three `.dat` files are present.
- **A movie ID is in range but missing:** The dataset has gaps in movie IDs;
  this is expected.
- **No genre matches:** Multiple genres require a movie to contain every
  selected genre.
- **A displayed rating seems to match but is excluded:** Searches compare
  unrounded averages; displayed ratings are rounded to two decimal places.
- **Generated `.dSYM` folders:** These contain macOS debugging symbols. Like
  `build/` and object files, they are generated output and should not be committed.

## Benchmarks

Run `make benchmark` to build and run the optimized (`-O2`) benchmark executable.
It uses separate objects from the application's debug build. To customize a run:

```sh
make build/benchmark
./build/benchmark MovieDatabase 100 data
```

Arguments are implementation name, positive repetition count (default 10), and
data directory (default `data`). Run from the repository root. Each repetition
looks up IDs 0 through 3953, searches each canonical genre plus two genre
combinations, and searches seven rating ranges including the full range and an
exact bound. Unknown IDs and gaps are included; invalid search arguments are
covered by the model tests rather than timed.

Construction/loading is reported separately. Each method gets one untimed warmup
and reports timed call count, total milliseconds, average nanoseconds per call,
and a result checksum (including the warmup). Search timings include result
allocation, traversal, and destruction. These are warm-cache workload averages;
repeat runs on the same machine, dataset, and compiler settings for comparisons.
Checksums help compare results but do not replace correctness tests.

To add a storage implementation, implement `IMovieDatabase` and register its name
and constructor in `createDatabase` in `src/benchmark.cpp`. Add any implementation
sources to `MODEL_SOURCES` in the Makefile. All timed queries go through the
interface; the workloads do not depend on the concrete database type.
