# Movie catalog report and reproducibility

The report PDF is `output/pdf/movie-catalog-report.pdf` in the repository.
`report/report.md` is the source of truth. Edit it directly, then run
`python3 report/scripts/write_report.py` to regenerate only the PDF. The generator
never rewrites the Markdown and does not require benchmark results to render it.

Supported formatting: title (`#`), section headings (`##`), paragraphs, bold,
italics, inline code, links, bullet/numbered lists, and pipe tables. Use
`<!-- page break -->` on its own line to start a new page. Content before the first
page break is centered on the title page. Longer edits can increase the page count;
check the resulting PDF before submission.

## Reproduce

Run from the repository root, with clang++ and make installed:

```sh
make
make test
python3 report/scripts/collect.py
python3 report/scripts/write_report.py
```

Collection uses Python's standard library. PDF generation requires `reportlab`.
The bundled Codex Python used for this run was:
`/Users/danfilipski/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3`.

Download MovieLens 1M from https://grouplens.org/datasets/movielens/1m/ and place
`movies.dat`, `ratings.dat`, and `users.dat` in `data/`. Source data is not included
in the report package. The supplied local data was preserved. Cite Harper and
Konstan (2015), DOI https://doi.org/10.1145/2827872, and follow GroupLens terms.

The collection script builds the existing optimized benchmark and an untimed exact
result verifier. It creates temporary nested samples (seed 20261020), retains every
rating for selected movies, and verifies all benchmark queries on each size before
running five trials per implementation with 100 repetitions per workload.
Execution order rotates. Only operation timing is compared; loading is logged
separately. Full workload definitions are in `src/benchmark.cpp` and the report.

`report/raw/measurements.csv` contains all 180 method measurements (45 process
runs, four operations each). `summary.json` contains medians and ranges;
`environment.json` contains fixture counts, source hashes, compiler, and OS.
The 45 text logs retain loading times and checksums. Three verification logs
record successful exact equivalence checks. Rerunning collection updates the raw
results only. If you collect new measurements, manually update the tables and
numeric discussion in `report/report.md` from `summary.json`, then render the PDF.
Wording-only edits do not require rerunning collection.

## Verification and review

- `make` and `make test` passed, with assertions enabled and no compiler warnings.
- Complete benchmark result sequences and optional averages agreed at all sizes.
- CLI smoke checks covered ID, genre, rating, invalid menu input, exit, and EOF.
- The PDF was rendered and visually checked; it has a title page, three content
  pages, and a references page.
- The cover's course code/title was inferred from the repository context; confirm
  it before submission. Team names were taken from the proposal.
- AI assistance is disclosed. Each team member should review and be able to
  explain the project. The report makes no claim that this human review occurred.

The package includes application sources, tests, Makefile, repository README,
report, reproduction scripts, and raw results. Generated executables and source
datasets are excluded. Build/run details for the CLI remain in the root README.
