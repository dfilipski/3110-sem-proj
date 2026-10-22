# Guidance for coding agents

## Project and scope

This is a C++17 command-line movie search program using the MovieLens
1M dataset. It loads users, movies, and ratings and supports movie lookup,
matching all selected genres, and inclusive average-rating searches. Vector,
unordered-map, and treap implementations share the IMovieDatabase query interface.

Keep changes focused on the requested feature. Follow the existing small-class,
standard-library approach; avoid adding frameworks, dependencies, or broad
refactors unless the feature needs them. Check the working tree before editing
and preserve unrelated user changes.

## Build, run, and verify

Run commands from the repository root; data paths are relative to that directory.

```sh
make                 # Build build/main with clang++, C++17, -Wall -Wextra -g
make test            # Build and run build/models_test
make run             # Run the interactive program
printf '1\n1\n0\n' | ./build/main  # Smoke-test a movie lookup and exit
```

- For C++ changes, run `make` and `make test`; resolve new compiler warnings.
- For CLI changes, also exercise the affected input/output paths, including
  invalid input and EOF where relevant.
- The tests use `assert`; do not disable assertions with `-DNDEBUG` during testing.
- `make clean` removes `build/`. A clean rebuild is useful when changing compiler
  flags, since make does not track flag changes.
- Add new implementation files to `MODEL_SOURCES` in `Makefile` so both the
  application and tests link them. The current test target has one test main in
  `tests/models_test.cpp`; adding another test executable requires a new target.
- Report what was verified and any checks that could not run.

## Where features belong

- `src/main.cpp`: interactive prompts, complete-line input validation, display,
  and top-level exception handling. Keep search and data logic outside it.
- `src/Movie.h/.cpp`, `src/User.h/.cpp`, `src/Rating.h/.cpp`: validated record
  types, MovieLens parsing/serialization, and record-specific helpers.
- `src/DataRecord.h`: shared delimiter splitting and strict integer parsing.
- `src/IMovieDatabase.h`: shared query, result ordering, and pointer lifetime contracts.
- `src/MovieDatabase.h/.cpp`: vector storage, dataset loading, cached averages,
  and searches. Its constructor accepts a data directory, defaulting to `data`.
- `src/MovieDatabaseMap.h/.cpp`: unordered-map storage implementing the same queries.
- `src/MovieDatabaseTreap.h/.cpp`: composite-key treap with subtree search metadata.
- `src/benchmark.cpp`: comparable query workloads across all three implementations.
- `tests/models_test.cpp`: model validation, full-dataset round trips, search
  edge cases, implementation comparisons, and treap invariants.
- `data/*.dat`: source datasets. Preserve them; use small synthetic records or
  separate temporary fixtures for tests instead of editing the supplied data.
- `build/`, object files, and `.dSYM/` are generated output; do not commit them.

## Data contracts to preserve

- Records use `::` between fields and `|` between movie genres. Parsing accepts
  CRLF line endings but rejects incorrect field counts and partial or
  whitespace-padded integers. Keep parsing and serialization round trips intact.
- Movie IDs range from 1 to 3952, but the dataset contains 3883 movies with gaps.
  Never treat a position in the movies vector as the movie's ID.
- User IDs range from 1 to 6040. Ratings are whole stars from 1 to 5, with a
  nonnegative 64-bit Unix timestamp in seconds.
- Preserve complete movie titles, including release years. ZIP codes are strings
  so leading zeroes survive. User ages are categorical codes, not exact ages.
- Constructors enforce model validity. Invalid records throw
  `std::invalid_argument`; file loading reports `std::runtime_error` with path
  and, for malformed records, line number.
- Existing dataset tests expect 6040 users, 3883 movies, and 1000209 ratings.
  If a requested feature generalizes the dataset, update the coupled validation,
  benchmark ID ranges, CLI ranges, and tests consistently.

## Search and ownership contracts

- All implementations reject duplicate movie IDs, user IDs, and movie/user
  rating pairs with file path and line number diagnostics.
- Average ratings are precomputed arithmetic means of submitted ratings only.
  Unknown or unrated movies return `std::nullopt`.
- Genre searches require at least one canonical genre and match all selected
  genres. Empty or unknown genres throw `std::invalid_argument`.
- Rating searches use inclusive, unrounded averages and exclude unrated movies.
  Nonfinite bounds or ranges outside 1 <= minimum <= maximum <= 5 throw
  `std::invalid_argument`.
- Search results contain unique movies in ascending movie-ID order. Unknown
  movie lookups return `nullptr`.
- Returned pointers refer to database-owned storage and remain valid until the
  database is destroyed, assigned, or moved from, as documented in IMovieDatabase.
- The dataset has about one million ratings. Cache averages during construction
  rather than rescanning ratings for each query. Preserve the distinct storage
  implementations so their benchmark comparisons remain meaningful.

## Implementation and testing style

Use existing naming and formatting: PascalCase classes, camelCase functions,
trailing underscores for instance members, four-space indentation, include guards,
and braces on a new line for functions. Prefer const references for read-only
access and standard containers for ownership. Document public behavior and edge
cases in headers when adding APIs.

For behavior changes, add focused regression cases to the existing tests. Prefer
small synthetic inputs for algorithm edge cases and an independent expected
result for average-rating math. Cover relevant empty inputs, missing IDs,
malformed records, duplicate IDs, and boundary values without duplicating implementation
logic in tests. Retain the existing full-dataset integration checks.
