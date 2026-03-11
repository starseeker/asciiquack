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

Benchmark: 1 000 in-process iterations, 10-iteration warm-up, GCC 13 `-O2`.

### Single-file (`benchmark/sample-data/mdbasics.adoc`, 334 lines, ~9 KB)

| Processor | Avg / iter | Conv / sec | vs Asciidoctor |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 | ~2.3 ms | ~440 | 1× (reference) |
| asciiquack (hand-written scanner) | ~0.27 ms | ~3 700 | **~8.5× faster** |

### BRL-CAD corpus (`benchmark/sample-data/brlcad/`, 9 files, ~3 600 lines)

The sample corpus mirrors real documentation from
[BRL-CAD](https://github.com/starseeker/brlcad_quickiterate/tree/asciidoc_only/brlcad/doc/asciidoc)
and exercises articles, man pages, tables, admonitions, code blocks, and
definition lists.

| Metric | Value |
|---|---|
| Files | 9 (4 articles + 5 man pages) |
| Total lines | ~3 600 |
| Average per file | ~760 µs |
| Throughput | ~1 300 files/sec |

Run the corpus benchmark yourself:

```bash
./bench_asciiquack benchmark/sample-data/brlcad 100
```

## BRL-CAD compatibility

All AsciiDoc features used by the BRL-CAD documentation set are supported:

| Feature | Example | Status |
|---|---|---|
| Articles with TOC | `:doctype: article`, `:toc:` | ✓ |
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
| Inline markup | `*bold*`, `_italic_`, `` `mono` `` | ✓ |

### Other dependencies

- Command line options: cxxopts (https://github.com/jarro2783/cxxopts)
- PDF writing: minimal subset of libharu (https://github.com/libharu/libharu)
- Font support: struetype fork of stb_truetype (https://github.com/starseeker/struetype)
- PNG support: LodePNG (https://github.com/lvandeve/lodepng)
- fonts: Noto (https://fonts.google.com/noto) 
- Syntax highlighting (if C++23 available): µlight (https://github.com/eisenwave/ulight)
