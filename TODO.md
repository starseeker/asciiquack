# asciiquack – Status & TODO

## What Is Done

| Feature | Notes |
|---|---|
| Reader (line-by-line, CRLF, push-back, blank-skip) | |
| Document header (ATX `=` and setext `====` titles) | |
| Author line and multiple authors (`;` separated) | |
| Revision line (`vN.N, date: remark`) | |
| Attribute entries (`:name: value`, `:!name:`) | |
| Multi-line attribute values (trailing `\` continuation) | |
| Paragraphs (multi-line, joined with space) | |
| Literal paragraphs (leading whitespace) | |
| Section titles (`==` through `======`) | |
| Setext-style body section titles | |
| ATX section ID generation (`idprefix`, `idseparator`) | |
| `idprefix` empty string (IDs without leading `_`) | |
| Section numbering (`:sectnums:`, `:sectnumlevels:`) | |
| Table of Contents (`:toc:`, `:toclevels:`, `:toc-placement:`) | |
| Floating titles (`[discrete]`) | |
| Special section names (`[preface]`, `[appendix]`, etc.) | |
| Listing / source blocks (`----`) | |
| Literal blocks (`....`) | |
| Example blocks (`====`) | |
| Sidebar blocks (`****`) | |
| Quote / verse blocks (`____`) | |
| Passthrough blocks (`++++`) | |
| Open blocks (`--`) | |
| Admonition paragraphs (`NOTE:`, `TIP:`, etc.) | |
| Admonition blocks (`[NOTE]\n====`) | |
| Admonition captions from locale attributes (`note-caption`, etc.) | |
| Unordered lists (`*`, `-`, up to 5 levels) | |
| Compound list items (list continuation `+`) | |
| Ordered lists (`.`, `1.`, `a.`, roman numerals) | |
| Ordered list style from block attr (`[loweralpha]`, etc.) | |
| Ordered list start value (`[start=N]`) | |
| Description lists (`term::`) | |
| Description list compound body blocks | |
| Callout lists (`<N>`) | |
| Source callout markers in listing blocks | |
| Block images (`image::target[alt]`) | |
| Video block macro (`video::url[opts]`) | |
| Audio block macro (`audio::url[opts]`) | |
| Basic tables (`\|===`) | |
| Table column spec: proportional, alignment, repeat, style | |
| Table cell spec: colspan (`N+\|`), rowspan (`.N+\|`), combined (`N.M+\|`) | HTML5 `colspan=` and `rowspan=` emitted |
| Block title (`.Title`) | |
| Block anchor (`[[id]]`) | |
| Block attribute lines (`[source,lang]`, etc.) | |
| Thematic break (`'''`) | |
| Page break (`<<<`) | |
| Single-line comments (`// …`) | |
| Block comments (`////`) | |
| Special-character escaping (`&`, `<`, `>`) | |
| Inline bold / italic / monospace / highlight | |
| Constrained and unconstrained inline markers | |
| Superscript / subscript | |
| Attribute references (`{name}`) | |
| `attribute-missing` policy (`skip`/`warn`/`drop`) | |
| `counter:` / `counter2:` inline macros | |
| Typographic replacements (`--`, `...`, `(C)`, etc.) | |
| Inline anchors (`[[id]]`) | |
| Cross-references (`<<id>>`, `xref:id[]`) | |
| Explicit link macro (`link:url[text]`) | |
| Bare URL auto-linking | |
| Inline image macro (`image:path[alt]`) | |
| `kbd:[]`, `btn:[]`, `menu:[]` inline macros | |
| Hard line-break (` +`) | |
| Footnotes (`footnote:[text]`, `footnoteref:[id,text]`) | |
| Inline stem/math macros (`stem:[]`, `latexmath:[]`, `asciimath:[]`) | |
| Block stem (`[stem]` on a pass block → display math) | |
| MathJax CDN loader when `:stem:` is set | |
| Inline passthrough (`pass:[]`, `pass:q[]`, `pass:c[]`) | |
| ID generation helper (`generate_id`) | |
| HTML5 converter | |
| Man page backend (`-b manpage`, troff/groff output) | |
| DocBook 5 backend (`-b docbook5`, XML output) | |
| `doctype: manpage` title parsing | |
| Embedded mode (`--no-header-footer`) | |
| Safe-mode levels (Unsafe / Safe / Server / Secure) | |
| CLI (backend, doctype, attributes, safe-mode, dest-dir) | |
| `include::` directive (safe-mode–aware) | |
| Conditional preprocessing (`ifdef::`, `ifndef::`, `ifeval::`) | |
| Stylesheet linking (`:linkcss:`, `:stylesheet:`) | |
| `docinfo.html` / `docinfo-footer.html` injection (unsafe mode) | |
| Preamble `<div>` only when sections follow | |
| PDF backend (`-b pdf`, Letter/A4, headings, lists, code, admonitions, inline markup) | |
| PDF font embedding (`-a pdf-font=/path/to/font.ttf`; PostScript name and OS/2 metrics read from font) | |
| Logging: missing include file warning | |
| Logging: section nesting skip warning | |
| Logging: unclosed block warning | |

---

## What Remains

### Syntax highlighting

Source blocks currently emit plain `<code>` tags.  A future pass could
integrate a C++ highlighting library or emit the `data-lang` attributes
needed by a client-side JS highlighter such as highlight.js.

---

## Out of Scope

| Feature | Reason |
|---|---|
| Extensions API (`register`, `preprocessor`, etc.) | Requires plugin ABI or embedded scripting; too tightly coupled to Ruby object model |
| Markdown-style headings (`#`, `##`, …) | Conflicts with AsciiDoc `#` line-comment; document users should use `=` headings |
| Structured sourcemap logging (`:sourcemap:`) | Complex feature with limited practical value |

---

## PDF Output

The PDF backend (`-b pdf`) is implemented in `pdf.hpp` using `minipdf.hpp`,
a self-contained C++17 PDF writer derived from libharu concepts.  It uses
`struetype.h` (an stb-style TrueType parser) for optional font embedding.

Features:
- Letter (8.5"×11") and A4 page sizes (`-a pdf-page-size=A4`)
- Section headings (H1–H4), paragraphs, ordered/unordered lists
- Code/listing blocks (Courier font), admonition blocks, horizontal rules
- Inline bold, italic, monospace markup
- Multi-page layout with automatic page breaks

### Embedded TrueType fonts

By default the PDF uses the PDF base-14 fonts (Helvetica family + Courier).
To embed a custom TrueType body font:

```bash
asciiquack -b pdf -a pdf-font=/path/to/MyFont.ttf input.adoc
```

The font file is read once and the raw bytes are stored as a `/FontFile2`
stream.  A `FontDescriptor` object is generated with:
- **Metrics** from the OS/2 typographic table (`/Ascent`, `/Descent`,
  `/CapHeight`) — more accurate for PDF viewers than the hhea table values.
- **PostScript name** (`/FontName`, `/BaseFont`) extracted from the font's
  own name table (nameID=6), falling back to the filename stem.
- **`/Widths` array** for code points 32–255, derived from the font's
  horizontal metrics.

Bold, italic, and monospace text continue to use the PDF base-14 fonts
(Helvetica-Bold, Helvetica-Oblique, Courier).

---

## Performance Notes

Benchmark: 1 000 in-process iterations on `benchmark/sample-data/mdbasics.adoc`
(334 lines, ~8 KB), 10-iteration warm-up, GCC 13 `-O3`.

| Implementation | Average / iter | Conv / sec | Notes |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 (Ruby 3.2.3) | ~2.3 ms | ~440 | reference |
| asciiquack (hand-written scanner) | ~0.32 ms | ~3 100 | **~7.2×** faster |

The parser uses a hand-written single-pass block scanner (`block_scanner_hand.c`)
and a hand-written inline scanner (`inline_scanner.hpp`).  No external libraries
or generated code are required; the only runtime dependency is the C standard
library (`memchr`, `memcmp`, `memset`, `strlen`).

**Key techniques:**

- `memchr()` for description-list detection — glibc uses SSE2/AVX2,
  processing 16–32 bytes per cycle for the TEXT-line fast path.
- `memcmp()` prefix matching for keyword macros (`image::`, `ifdef::`, etc.)
  — compiled to word-sized comparisons by the C compiler.
- Combined classify+extract in one pass (no second scan).
- Single left-to-right pass for all 13 inline-markup patterns.

### Remaining opportunity

- **`shared_ptr` → `unique_ptr`** – The AST is a strict ownership tree;
  converting to `unique_ptr` would eliminate atomic ref-count traffic on
  every node.  Significant API refactoring required across parser, document
  model, and all converters.

---

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./asciiquack_tests        # run test suite
./bench_asciiquack [file] [iterations]
```

---

## References

- AsciiDoc specification: <https://docs.asciidoctor.org/asciidoc/latest/>
- cxxopts (CLI parsing): <https://github.com/jarro2783/cxxopts>
