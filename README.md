```
= asciiquack
:doctype: duck
:quack: true

// parsing...

    ==(o )===
      ( ._> ::
      `---'//

----
quack:: true
duck:: ascii
----
```

A C++17 translation of asciidoctor.

Currently a work in progress.  The desired end state is a clean,
self-contained, strictly C++17 compliant codebase that can handle
most of what real-world asciidoc use would entail - we'll see if
we get there.

Command line option via https://github.com/jarro2783/cxxopts

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./asciiquack_tests        # run test suite
./bench_asciiquack [file] [iterations]
```

To build without the PCRE2 dependency (falls back to `std::regex`):

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PCRE2=OFF
```

## Performance

Benchmark: 1 000 in-process iterations on `benchmark/sample-data/mdbasics.adoc`
(335 lines, ~9 KB), 10-iteration warm-up, GCC 13 `-O2`.

| Implementation | Avg / iter | Conv / sec | Notes |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 (Ruby 3.2.3) | ~2.3 ms | ~440 | reference |
| asciiquack / std::regex | ~3.2 ms | ~310 | baseline C++ |
| asciiquack / PCRE2 | ~0.65 ms | ~1 530 | **~5× faster than Ruby** |

PCRE2 is selected automatically by CMake when `libpcre2-dev` is installed
(enabled by default via `-DUSE_PCRE2=ON`).  All 479 tests pass with both
backends.

### Why not RE2?

RE2 was evaluated but cannot serve as a drop-in backend because several
patterns in asciiquack require features RE2 intentionally omits:

- **Backreferences in patterns** – e.g. `([-*_])…\1` (thematic-break detection)
- **Lookahead assertions** – e.g. `(?=[^*\w]|$)` (constrained inline quotes)
- **Negative lookahead** – e.g. `(?!//[^/])` (description-list guard)

Rewriting those patterns for RE2 would risk subtle behaviour changes.
PCRE2 is equally fast in practice and supports the full pattern set.

### Remaining performance opportunities

- **`shared_ptr` → `unique_ptr`** – The AST is a strict ownership tree;
  converting to `unique_ptr` would eliminate atomic ref-count traffic on
  every node.  Significant API refactoring required.
