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
(335 lines, ~9 KB), 10-iteration warm-up, GCC 13 `-O2`.

| Implementation | Avg / iter | Conv / sec | Notes |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 (Ruby 3.2.3) | ~2.3 ms | ~440 | reference |
| asciiquack / `std::regex` (GCC 13) | ~3.1 ms | ~321 | baseline |
| asciiquack / embedded PCRE2 (no JIT) | ~0.77 ms | ~1 291 | **~4× vs std::regex** – zero external dep |
| asciiquack / system PCRE2 (JIT) | ~0.65 ms | ~1 541 | **~4.8× vs std::regex** |

### What was done

- **`std::regex` → PCRE2** – Replaced GCC's slow `std::regex` with PCRE2
  via a thin `aqregex.hpp` adapter.  CMake selects:
  1. System `libpcre2-8` (JIT enabled, fastest) when `libpcre2-dev` is present.
  2. Embedded vendor subset (`vendor/pcre2/`, no JIT) when the system library
     is absent or when `-DUSE_SYSTEM_PCRE2=OFF` is passed — zero external
     dependency, still ~4× faster than `std::regex`.
  3. `std::regex` fallback via `-DUSE_PCRE2=OFF`.

- **Embedded PCRE2 amalgamation** – `vendor/pcre2_embed.h` is a
  single-header amalgamation (~54 000 lines) of PCRE2 10.42 generated by
  `vendor/tools/amalgamate_pcre2.py`.  Instantiated by `vendor/pcre2_impl.c`
  (one line: `#define PCRE2_EMBED_IMPLEMENTATION` + `#include`).  No JIT,
  no Unicode property tables (`\p{}`), no DFA, no tools.  The non-Unicode
  stubs in the inlined `pcre2_ucd.c` section satisfy any `\w`/`\s`/`\d`
  reference via the portable character tables in `pcre2_chartables.c`.

- **`OutputBuffer` instead of `std::ostringstream`** – The HTML5, DocBook5,
  and man-page converters now use a pre-reserved `std::string` sink
  (`outbuf.hpp`) instead of `std::ostringstream`.  This eliminates virtual
  dispatch on every `<<` call and avoids the repeated buffer doublings that
  ostringstream incurs for large documents.

### Why not RE2?

RE2 was evaluated but cannot serve as a drop-in backend because several
patterns require features RE2 intentionally omits:

- **Backreferences in patterns** – e.g. `([-*_])…\1` (thematic-break)
- **Lookahead assertions** – e.g. `(?=[^*\w]|$)` (constrained quotes)
- **Negative lookahead** – e.g. `(?!//[^/])` (description-list guard)

PCRE2 is equally fast and supports the full pattern set.

### Why not BRL-CAD/regex (Henry Spencer's POSIX regex)?

[BRL-CAD/regex](https://github.com/BRL-CAD/regex) is Henry Spencer's classic
POSIX ERE/BRE implementation.  It was evaluated as a potential zero-dependency
replacement for the bundled PCRE2 amalgamation.  It cannot serve as a drop-in
backend because it lacks several features that asciiquack's patterns require:

1. **Lookahead assertions** – Neither positive `(?=…)` nor negative `(?!…)`
   exist in POSIX ERE/BRE, and Spencer's engine does not implement them.
   Asciiquack uses lookaheads in more than a dozen patterns (e.g. every
   constrained-quote boundary check uses `(?=[^*a-zA-Z0-9]|$)` and the
   description-list guard uses `(?!//[^/])`).

2. **Non-greedy quantifiers** – `*?`, `+?`, and `??` are Perl/PCRE
   extensions absent from POSIX.  All constrained inline-markup patterns
   (bold, italic, monospace, etc.) rely on non-greedy matching.

3. **Non-capturing groups** – `(?:…)` is not part of POSIX ERE.  Every
   optional-suffix pattern in the codebase (`(?:…)?`) uses this syntax.

4. **Shorthand character classes** – `\w`, `\d`, `\s` and their negations
   (`\W`, `\D`, `\S`) are Perl extensions.  POSIX requires bracket
   expressions such as `[[:alnum:]]` instead.  A mechanical translation
   would be possible but would require rewriting every pattern.

5. **No built-in substitution function** – The POSIX API (`regcomp`,
   `regexec`, `regfree`) provides only match-position information.  There
   is no equivalent of PCRE2's `pcre2_substitute` or `std::regex_replace`.
   A replacement loop would need to be written from scratch.

6. **Performance** – Spencer's engine is an NFA-based backtracking
   interpreter with no JIT tier.  Benchmarks show it at roughly the same
   speed as `std::regex` (GCC's libstdc++ implementation, also NFA-based),
   well below PCRE2's ~4–5× advantage.  Adopting it would therefore provide
   no performance benefit over the existing `std::regex` fallback.

**Conclusion:** BRL-CAD/regex cannot replace PCRE2 for asciiquack.  The
`std::regex` fallback (`-DUSE_PCRE2=OFF`) remains the appropriate
dependency-free option, and the embedded PCRE2 amalgamation
(`vendor/pcre2_embed.h`) remains the default zero-external-dependency fast
path.


### re2c + lemon: block-level scanner and attribute-list parser

AsciiDoc's **block-level grammar is regular / context-free** and therefore
fully amenable to re2c and lemon:

| Layer | Tool | File(s) | What it replaces |
|---|---|---|---|
| Block-line lexer | [re2c](https://skvadrik.github.io/re2c/) | `block_scanner.re` → `block_scanner_gen.h` | `aqrx::regex` for every block-level classification pattern in `parser.cpp` |
| Attr-list parser | [lemon](https://github.com/BRL-CAD/lemon) | `attr_list.lemon` → `attr_list_gen.c` | hand-written `parse_attribute_list()` in `parser.cpp` |

**Key findings:**

- All block-level patterns are regular and can be expressed as re2c rules
  without backreferences or lookaheads, with one exception: the
  Markdown-style thematic-break pattern `([-*_])( *)\1\2\1`.  That
  backreference is trivially resolved by a 10-line C helper
  (`is_thematic_break` in `block_scanner.c`) that runs before the DFA; the
  re2c scanner itself has no non-regular patterns.

- The comment-line guard `(?!//[^/])` on description lists is satisfied by
  placing the comment rules *before* the description-list rule; re2c's
  longest-match / first-match semantics then ensure comment lines never
  reach the description-list rule.

- The AsciiDoc block-attribute list `[positional, key=value, "quoted"]` is
  an LALR(1) language and is parsed correctly by the lemon grammar.

**Build integration:**

- Pre-generated files (`block_scanner_gen.h`, `attr_list_gen.c`) are
  committed so that neither re2c nor lemon is required at build time.

- When re2c / lemon are found (`USE_RE2C=ON`, `USE_LEMON=ON`, both default),
  CMake adds custom commands to regenerate the pre-generated files from their
  sources whenever the sources change.

- The scanner is compiled into every build unconditionally.  The
  `ASCIIQUACK_USE_SCANNER` definition gates the test helpers and the
  `#include` of the C headers from C++ code.

- **`USE_SCANNER_PARSER=ON`** wires the scanner and lemon parser into the hot
  path: `try_parse_attribute_entry`, `match_list_item`, `match_block_image`,
  `match_block_media`, `is_thematic_break`, `parse_attribute_list`, and
  `section_title_text` all delegate to the scanner instead of PCRE2.
  Default is OFF so existing PCRE2 behavior is unchanged.

**Corpus validation against BRL-CAD documentation (532 .adoc files):**

```
scripts/compare_brlcad.sh
  Identical:     532 / 532
  Differing:     0
```

Scanner-parser output is **bit-for-bit identical** to PCRE2 output across
the full BRL-CAD documentation corpus (both HTML5 and man-page output).

**Benchmark results** (`build_regex/bench_asciiquack` vs
`build_scanner/bench_asciiquack`, 500 iterations, Release build, system
PCRE2 10.42 with JIT):

| Input | PCRE2 (regex) | re2c/lemon (scanner) | Speedup |
|---|---|---|---|
| `mdbasics.adoc` (335 lines) | 0.82 ms/iter | 0.43 ms/iter | **1.9×** |
| BRL-CAD corpus (532 files) | 733 µs/file | 427 µs/file | **1.7×** |

The scanner eliminates all PCRE2 calls for block-level classification; only
inline markup patterns remain on the PCRE2 path.  To build and run:

```bash
# Scanner-parser variant:
mkdir build_scanner && cd build_scanner
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SCANNER_PARSER=ON
cmake --build . -j4
./bench_asciiquack /path/to/brlcad/doc/asciidoc 20

# Corpus correctness check:
bash scripts/compare_brlcad.sh
```

**Inline markup:** The patterns in `substitutors.hpp` use lookaheads
(`(?=[^*\w]|$)`) and backreferences.  These are not regular and must remain
on the PCRE2 path.  A future avenue is to replace them with a hand-written
multi-pass inline scanner that handles markup as a single DFA traversal.

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
