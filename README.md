```
= asciiquack
:doctype: duck
:quack: true

// parsing...

     __
   <(o )___
    ( ._> /
     `---'

----
quack:: true
duck:: ascii
----
```

A C++17 translation of (most of) asciidoctor.

This is intending to be a minimalist, self-contained tool that can be used to
produce output along the lines of asciidoctor, but without some of its most
complex features - for example, our PDF output is quite basic since most full
featured solutions to that problem are also extremely heavy.


## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./asciiquack_tests        # run test suite
./bench_asciiquack [file] [iterations]
./bench_asciiquack [directory] [iterations]   # corpus mode: all *.adoc files
```

## Performance

The default build uses a hand-written single-pass block scanner
(`block_scanner_hand.c`) and a hand-written inline scanner
(`inline_scanner.hpp`) that together replace all regex use on the hot path.
No external libraries are needed.

Benchmark: in-process iterations with warm-up, GCC 13 `-O2`.
Full BRL-CAD corpus (199 files, ~67 000 lines total).

### BRL-CAD corpus benchmark (199 files)

All timing measured against the complete BRL-CAD documentation corpus from
[starseeker/brlcad_quickiterate](https://github.com/starseeker/brlcad_quickiterate/tree/asciidoc_only/brlcad/doc/asciidoc)
(199 `.adoc` files: articles, books, man pages, specs, 50 benchmark rounds):

| Processor | µs / file | Files / sec | vs asciidoctor |
|---|---|---|---|
| Ruby Asciidoctor 2.0.26 | ~3 460 µs | ~290 | 1× (reference) |
| asciiquack PCRE2 (main branch) | ~1 127 µs | ~890 | **~3.1×** faster |
| asciiquack hand-written scanner | ~421 µs | ~2 375 | **~8.2×** faster |

Run the corpus benchmark yourself:

```bash
./bench_asciiquack /path/to/brlcad/doc/asciidoc 50
```

### Single-file (`benchmark/sample-data/mdbasics.adoc`, 334 lines, ~9 KB)

| Processor | Avg / iter | Conv / sec | vs Asciidoctor |
|---|---|---|---|
| Ruby Asciidoctor 2.0.26 | ~3.5 ms | ~290 | 1× (reference) |
| asciiquack (hand-written scanner) | ~0.24 ms | ~4 200 | **~14× faster** |

## BRL-CAD compatibility

asciiquack was validated against the full BRL-CAD AsciiDoc corpus (199 files).
All AsciiDoc features used by the documentation set are supported.  Output was
compared against both the PCRE2-based main-branch asciiquack and Asciidoctor
2.0.26.

### Regression results (199 files)

| Category | Files affected | Result |
|---|---|---|
| No differences (manpages, most HTML) | 181 / 199 | ✓ identical |
| Block image alt text (stem, not full path) | 18 / 199 | ✓ **improvement** – matches asciidoctor |
| Inline image URL wrapped in `<a href>` | 1 / 199 | ✓ **bug fix** – old version produced broken HTML |
| Malformed asterisks (`*******text*****`) | 3 / 199 | ⚠ benign – valid HTML, differs from PCRE2; all processors diverge on degenerate input |

### Features exercised

| Feature | Example | Status |
|---|---|---|
| Articles with TOC | `:doctype: article`, `:toc:` | ✓ |
| Books | `:doctype: book`, chapters, parts | ✓ |
| Man pages | `:doctype: manpage`, `:mansource:`, `:manmanual:` | ✓ |
| Definition lists | `*-o*::` , `*term*::` | ✓ |
| Tables with `cols=` | `[cols="2*"]` | ✓ |
| `[%noheader]` tables | stacked attribute lists | ✓ |
| Verbatim blocks | `....` delimited blocks | ✓ |
| Source / listing blocks | `[source,bash]` + `----` | ✓ |
| Admonitions | `[NOTE]`, `[CAUTION]` | ✓ |
| Example blocks | `[example]` + `====` | ✓ |
| Block and inline images | `image::path[]`, `image:path[]` | ✓ |
| Cross-references | `<<anchor>>`, `[[anchor]]` | ✓ |
| Nested inline markup | `*bold _italic_ text*`, `` `cmd _arg_` `` | ✓ |

### Other dependencies

- Command line options: cxxopts (https://github.com/jarro2783/cxxopts)
- PDF writing: minimal subset of libharu (https://github.com/libharu/libharu)
- Font support: struetype fork of stb_truetype (https://github.com/starseeker/struetype)
- PNG support: LodePNG (https://github.com/lvandeve/lodepng)
- fonts: Noto (https://fonts.google.com/noto) 
- Syntax highlighting (if C++23 available): µlight (https://github.com/eisenwave/ulight)
