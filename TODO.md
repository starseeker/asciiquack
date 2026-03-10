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
(334 lines, ~8 KB), 10-iteration warm-up, GCC 13 `-O2`.

| Implementation | Average / iter | Conv / sec | Notes |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 (Ruby 3.2.3) | ~2.3 ms | ~440 | reference |
| asciiquack / `std::regex` (GCC 13) | ~3.1 ms | ~321 | baseline |
| asciiquack / embedded PCRE2 (no JIT) | ~0.77 ms | ~1 291 | **~4× vs std::regex** – zero external dep |
| asciiquack / system PCRE2 (JIT) | ~0.89 ms | ~1 120 | PCRE2-only baseline |
| asciiquack / system PCRE2 (JIT) + inline scanner | ~0.79 ms | ~1 265 | **1.13×** vs PCRE2-only |
| asciiquack / system PCRE2 (JIT) + block scanner | ~0.45 ms | ~2 210 | **1.97×** vs PCRE2-only |
| asciiquack / system PCRE2 (JIT) + block + inline scanners | ~0.35 ms | ~2 820 | **2.53×** vs PCRE2-only |

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

**Benchmark results** (1 000 iterations, Release build, system PCRE2 10.42
with JIT, GCC 13 `-O2`):

| Backend | `mdbasics.adoc` (sparse markup) | `inline_heavy.adoc` (dense markup) | Speedup |
|---|---|---|---|
| system PCRE2 (JIT) | 0.89 ms/iter | 1.30 ms/iter | baseline |
| + block scanner | 0.45 ms/iter | 0.51 ms/iter | **1.97–2.55×** |
| + inline scanner | 0.79 ms/iter | 1.13 ms/iter | **1.13–1.15×** |
| + block + inline scanners | 0.35 ms/iter | 0.35 ms/iter | **2.53–3.73×** |

`benchmark/sample-data/inline_heavy.adoc` (136 lines, ~6 KB) has dense
bold, italic, monospace, super/subscript, and highlight spans throughout
every paragraph, making it the worst case for the PCRE2 inline-markup path.

The scanner eliminates all PCRE2 calls for block-level classification; the
inline scanner (`inline_scanner.hpp`) additionally eliminates PCRE2 for
inline quote markup.  When both are enabled, every PCRE2 call in the
parse+convert pipeline is removed.  To build and run:

```bash
# Block scanner only:
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SCANNER_PARSER=ON

# Inline scanner only:
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_INLINE_SCANNER=ON

# Both (maximum performance, zero PCRE2 in hot path):
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SCANNER_PARSER=ON -DUSE_INLINE_SCANNER=ON
cmake --build . -j4
./bench_asciiquack benchmark/sample-data/mdbasics.adoc 1000
./bench_asciiquack benchmark/sample-data/inline_heavy.adoc 1000

# Corpus correctness check:
bash scripts/compare_brlcad.sh
```

### Inline markup scanner (`inline_scanner.hpp`)

The lookahead and boundary requirements that make inline-markup patterns
unsuitable for re2c / lemon **are straightforwardly expressible as
character-level boundary checks** in a hand-written scanner.

**Why re2c / lemon are poor matches for inline markup:**

| Requirement | Why not re2c | Why not lemon |
|---|---|---|
| Preceding-character boundary (`[^*\w]` before `*`) | re2c DFA has no state for the previously-consumed token | Parser grammar can, but each token's validity depends on the surrounding characters — context the LALR(1) automaton would need to thread through as attributes |
| Following-character lookahead (`(?=[^*\w]\|$)`) | re2c supports fixed-length lookahead, but "not followed by alphanumeric or same marker" at an unknown offset requires variable lookahead | Lookahead is a parser-level concept; it would require rewriting the grammar in terms of token pairs |
| Greedy / non-greedy content span (`\S.*?\S`) | re2c always takes the longest match; non-greedy semantics need extra states or a second scanner pass | Not a lexical concept |

**What the hand-written scanner does:**

`inline_scanner.hpp` implements `scan_inline_quotes()`, a single left-to-right
pass over the input string that is a drop-in replacement for the 13-regex
chain in `sub_quotes()`.

- **"Preceding character"** is tracked via `out.back()` — the last byte
  written to the output buffer.  This naturally includes boundary changes
  caused by previously-emitted HTML tags (e.g. `>` after `</strong>`).
- **"Following character" lookahead** is satisfied by inspecting `text[close+1]`
  after finding the candidate closing marker.
- **Non-greedy matching** is achieved by returning the *first* closing position
  that passes all constraints.
- The scanner handles all 13 patterns (6 unconstrained `**`, `__`, `\`\``,
  `##`, `^^`, `~~`; 7 constrained `*`, `_`, `` ` ``, `+`, `#`, `^`, `~`) in
  one pass, giving O(n) throughput vs. the O(13n) of the regex chain.

**Measured performance** (1 000 iterations, Release build, system PCRE2 10.42
with JIT, GCC 13 `-O2`):

| Input | PCRE2 baseline | + inline scanner | Speedup | `sub_quotes` share |
|---|---|---|---|---|
| `mdbasics.adoc` (334 lines, sparse inline) | 0.89 ms | 0.79 ms | **1.13×** | ~12% of pipeline |
| `inline_heavy.adoc` (136 lines, dense inline) | 1.30 ms | 1.13 ms | **1.15×** | ~13% of pipeline |

The "sub_quotes share" column is derived from `(baseline − scanner) / baseline`,
which is the fraction of total pipeline time that was spent in `sub_quotes()`.
On typical documentation the inline-substitution step accounts for 12–13% of
total parse+convert time; it is not the dominant cost.

**Why the end-to-end gain is smaller than the theoretical 13×:**

The inline scanner makes a single O(n) pass; the PCRE2 chain makes 13 passes.
In pure inline-text micro-benchmarks the scanner is ~10–12× faster.  In full
end-to-end benchmarks the gain is smaller because:

1. `sub_quotes()` accounts for only 12–13% of total pipeline time.
2. Other stages (parsing, attribute resolution, macro substitution, output
   serialisation, shared-ptr overhead) dominate.
3. PCRE2 with JIT is itself very fast; the single PCRE2 call overhead for
   each of the 13 patterns is low when the input string is short (one
   paragraph or one list-item body).

The inline scanner is most valuable when combined with the block-level scanner
(`USE_SCANNER_PARSER=ON`), which removes the larger PCRE2 cost at block level.
Together they yield a **2.53–3.73× speedup** and eliminate every PCRE2 call
in the hot path.

**Build integration:**

```bash
# Enable the inline scanner:
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_INLINE_SCANNER=ON
```

This adds `ASCIIQUACK_USE_INLINE_SCANNER` to all targets.  The `sub_quotes()`
function in `substitutors.hpp` then delegates to `scan_inline_quotes()`
instead of running the PCRE2 regexes.  When `OFF` (default), the PCRE2 chain
runs unchanged.

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
