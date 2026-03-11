```
     __
   <(o )___
    ( ._> /
     `---'
```

**asciiquack** is a fast, self-contained C++17 AsciiDoc processor compatible
with [Asciidoctor](https://asciidoctor.org/).  It converts `.adoc` source files
to HTML5, PDF, DocBook 5, and troff/groff man pages — with no Ruby runtime, no
gem dependencies, and no internet connection required at build or run time.


## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./asciiquack_tests                          # run the test suite
./bench_asciiquack [file] [iterations]      # single-file benchmark
./bench_asciiquack [directory] [iterations] # corpus benchmark (all *.adoc)
```

No system libraries are required.  All dependencies are vendored.


## Usage

```
asciiquack [OPTIONS] FILE...
```

Read one or more AsciiDoc source files (or stdin when no file is given) and
write the converted output to a file beside the input (or to stdout with `-o -`).

### Common options

| Option | Description |
|---|---|
| `-b, --backend FORMAT` | Output format: `html5` (default), `pdf`, `docbook5`, `manpage` |
| `-d, --doctype TYPE` | Document type: `article` (default), `book`, `manpage`, `inline` |
| `-a, --attribute NAME[=VALUE]` | Set or override a document attribute |
| `-o, --out-file PATH` | Output file (use `-` for stdout) |
| `-D, --destination-dir DIR` | Output directory when converting multiple files |
| `-B, --base-dir DIR` | Base directory for the document and relative resources |
| `-e, --embedded` | Suppress document wrapper (`<html>`/`<head>`/`<body>`) |
| `-S, --safe-mode MODE` | Safe mode: `unsafe`, `safe`, `server`, `secure` (default: `secure`) |
| `-q, --quiet` | Suppress informational messages |
| `-v, --verbose` | Increase verbosity |
| `-V, --version` | Print version |
| `-h, --help` | Print help |

### Examples

```bash
# HTML5 (default)
asciiquack doc.adoc

# PDF output
asciiquack -b pdf doc.adoc

# PDF with a custom body font
asciiquack -b pdf -a pdf-font=/path/to/MyFont.ttf doc.adoc

# DocBook 5
asciiquack -b docbook5 doc.adoc

# Man page
asciiquack -b manpage doc.adoc

# Embedded fragment (no <html> wrapper)
asciiquack -e doc.adoc

# Read from stdin, write to stdout
echo "Hello _world_." | asciiquack -o -
```


## Output formats

| Backend | Flag | Output |
|---|---|---|
| HTML5 | `-b html5` | Self-contained HTML with inline CSS |
| PDF | `-b pdf` | Portable Document Format |
| DocBook 5 | `-b docbook5` | DocBook 5 XML |
| Man page | `-b manpage` | troff/groff source (`.1`) |


## PDF output

The PDF backend (`-b pdf`) is implemented in `pdf.hpp` using `minipdf.hpp`,
a self-contained C++17 PDF writer.

### Page size

```bash
asciiquack -b pdf -a pdf-page-size=A4 doc.adoc   # A4 (default: Letter)
```

### Fonts

By default the PDF uses the [Noto Sans](https://fonts.google.com/noto) fonts
from the repository's `fonts/` directory (path resolved at build time).
You can override any weight/style individually:

| Attribute | Font role |
|---|---|
| `-a pdf-font=PATH` | Body text (regular) |
| `-a pdf-font-bold=PATH` | Bold body text |
| `-a pdf-font-italic=PATH` | Italic body text |
| `-a pdf-font-bold-italic=PATH` | Bold-italic body text |
| `-a pdf-font-mono=PATH` | Monospace (code blocks) |
| `-a pdf-font-mono-bold=PATH` | Bold monospace |

All values accept an absolute or relative path to a TrueType (`.ttf`) font
file.  Each font is embedded as a `/FontFile2` stream; metrics and the
PostScript name are read directly from the font's OS/2 and name tables.

### PDF features

- Letter (8.5″×11″) and A4 page sizes
- Section headings H1–H4, paragraphs, ordered/unordered lists
- Code/listing blocks, admonition blocks, horizontal rules
- Inline bold, italic, monospace markup
- Multi-page layout with automatic page breaks
- Block and inline images (JPEG and PNG)


## Supported AsciiDoc features

| Feature | Notes |
|---|---|
| Document header (`=` title, author, revision) | |
| Attribute entries (`:name: value`, `:!name:`, multi-line `\`) | |
| Section titles (`==` – `======`), setext-style titles | |
| Section IDs (`idprefix`, `idseparator`) | |
| Section numbering (`:sectnums:`, `:sectnumlevels:`) | |
| Table of contents (`:toc:`, `:toclevels:`, `:toc-placement:`) | |
| Floating titles (`[discrete]`) | |
| Special sections (`[preface]`, `[appendix]`, etc.) | |
| Paragraphs (multi-line joined with space), literal paragraphs | |
| Listing / source blocks (`----`) | |
| Literal blocks (`....`) | |
| Example blocks (`====`) | |
| Sidebar blocks (`****`) | |
| Quote / verse blocks (`____`) | |
| Passthrough blocks (`++++`) | |
| Open blocks (`--`) | |
| Admonition paragraphs and blocks (`NOTE`, `TIP`, `IMPORTANT`, `WARNING`, `CAUTION`) | |
| Admonition captions from locale attributes | |
| Unordered lists (`*`, `-`, up to 5 levels), compound items (`+`) | |
| Ordered lists (`.`, `1.`, `a.`, roman numerals, `[loweralpha]`, `[start=N]`) | |
| Description lists (`term::`), compound body blocks | |
| Callout lists (`<N>`) and source callout markers | |
| Basic tables (`\|===`): column spec, colspan, rowspan, alignment, style | |
| Block title (`.Title`), block anchor (`[[id]]`), block attributes | |
| Thematic break (`'''`), page break (`<<<`) | |
| Block and inline images (`image::`, `image:`) | |
| Video and audio block macros | |
| Inline bold, italic, monospace, highlight (constrained + unconstrained) | |
| Superscript, subscript | |
| Attribute references (`{name}`), `attribute-missing` policy | |
| `counter:` / `counter2:` inline macros | |
| Typographic replacements (`--`, `...`, `(C)`, etc.) | |
| Inline and block anchors, cross-references (`<<id>>`, `xref:id[]`) | |
| Explicit link macro (`link:url[text]`), bare URL auto-linking | |
| Inline image macro (`image:path[alt]`) | |
| `kbd:[]`, `btn:[]`, `menu:[]` inline macros | |
| Hard line-break (` +`) | |
| Footnotes (`footnote:[text]`, `footnoteref:[]`) | |
| Stem / math macros (`stem:[]`, `latexmath:[]`, `asciimath:[]`) and MathJax loader | |
| Inline passthrough (`pass:[]`, `pass:q[]`, `pass:c[]`) | |
| `include::` directive (safe-mode–aware) | |
| Conditional preprocessing (`ifdef::`, `ifndef::`, `ifeval::`) | |
| Stylesheet linking (`:linkcss:`, `:stylesheet:`) | |
| `docinfo.html` / `docinfo-footer.html` injection (unsafe mode) | |
| Embedded mode (`--embedded`) | |
| Safe-mode levels (Unsafe / Safe / Server / Secure) | |
| DocBook 5 backend (`-b docbook5`) | |
| Man page backend (`-b manpage`, troff/groff) | |
| PDF backend (`-b pdf`) | |
| Syntax highlighting in HTML5 output (C++23 build, via µlight) | |


## Out of scope

The following Asciidoctor features are intentionally not implemented:

| Feature | Reason |
|---|---|
| Extensions API (`register`, `preprocessor`, etc.) | Requires plugin ABI or embedded scripting; tightly coupled to Ruby's object model |
| Markdown-style headings (`#`, `##`, …) | Conflicts with AsciiDoc `#` line-comment; use `=` headings instead |
| Structured sourcemap logging (`:sourcemap:`) | Complex feature with limited practical value |


## Performance

The parser uses a hand-written single-pass block scanner (`block_scanner_hand.c`)
and a hand-written inline scanner (`inline_scanner.hpp`).  No external libraries
or generated code are required on the hot path.

### BRL-CAD corpus (532 files, 50 rounds, GCC 13 `-O3`)

| Processor | µs / file | Files / sec | vs Asciidoctor |
|---|---|---|---|
| Ruby Asciidoctor 2.0.26 | ~2 200 µs | ~450 | 1× (reference) |
| asciiquack | ~357 µs | ~2 800 | **~6.2×** faster |

### Single file (`benchmark/sample-data/mdbasics.adoc`, 334 lines, ~9 KB)

| Processor | Avg / iter | Conv / sec | vs Asciidoctor |
|---|---|---|---|
| Ruby Asciidoctor 2.0.26 | ~2.3 ms | ~440 | 1× (reference) |
| asciiquack | ~0.24 ms | ~4 150 | **~9.5×** faster |

Run the corpus benchmark yourself:

```bash
./bench_asciiquack /path/to/brlcad/doc/asciidoc 50
```


## BRL-CAD corpus compatibility

asciiquack was validated against the full BRL-CAD AsciiDoc corpus (199 files),
comparing output against Asciidoctor 2.0.26.

| Category | Files | Result |
|---|---|---|
| No differences (man pages, most HTML) | 181 / 199 | ✓ identical |
| Block image alt text | 18 / 199 | ✓ improvement — matches Asciidoctor |
| Inline image URL wrapped in `<a href>` | 1 / 199 | ✓ bug fix |
| Malformed asterisks (`*******text*****`) | 3 / 199 | ⚠ benign — all processors diverge on degenerate input |


## Dependencies

All dependencies are vendored; no system packages are required.

| Dependency | Purpose |
|---|---|
| [cxxopts](https://github.com/jarro2783/cxxopts) | Command-line option parsing |
| [minipdf](pdf.hpp) / libharu concepts | Self-contained PDF writer |
| [struetype](https://github.com/starseeker/struetype) | TrueType font parsing (stb_truetype fork) |
| [LodePNG](https://github.com/lvandeve/lodepng) | PNG image decoding |
| [Noto fonts](https://fonts.google.com/noto) | Default PDF fonts (in `fonts/`, path set at build time) |
| [µlight](https://github.com/eisenwave/ulight) | Syntax highlighting (C++23 builds only) |
| [PCRE2](https://www.pcre.org/) (embedded subset) | Regex backend (5× faster than `std::regex`) |


## References

- AsciiDoc language reference: <https://docs.asciidoctor.org/asciidoc/latest/>
