# asciiquack – Translation Status & TODO

## Summary

The initial C++ translation covers the **core document model and the most
common block/inline constructs**.  All existing tests pass, and the
implementation produces correct HTML5 output for typical prose documents.

However, the Ruby Asciidoctor source is roughly **18,000 lines** of highly
optimised Ruby, while the C++ port is currently around **5,000 lines** (not
counting the bundled third-party headers).  A significant portion of the
Ruby feature set is still missing.

---

## What Is Working

| Feature | Status |
|---|---|
| Reader (line-by-line, CRLF, push-back, blank-skip) | ✅ |
| Document header (ATX `=` and setext `====` titles) | ✅ |
| Author line and multiple authors (`;` separated) | ✅ |
| Revision line (`vN.N, date: remark`) | ✅ |
| Attribute entries (`:name: value`, `:!name:`) | ✅ |
| Paragraphs (multi-line, joined with space) | ✅ |
| Literal paragraphs (leading whitespace) | ✅ |
| Section titles (`==` through `======`) | ✅ |
| Setext-style body section titles | ✅ |
| ATX section ID generation (`idprefix`, `idseparator`) | ✅ |
| `idprefix` empty string (IDs without leading `_`) | ✅ |
| Listing / source blocks (`----`) | ✅ |
| Literal blocks (`....`) | ✅ |
| Example blocks (`====`) | ✅ |
| Sidebar blocks (`****`) | ✅ |
| Quote / verse blocks (`____`) | ✅ |
| Passthrough blocks (`++++`) | ✅ |
| Inline passthrough (`pass:[]`, `pass:q[]`, `pass:c[]`) | ✅ |
| Open blocks (`--`) | ✅ |
| Admonition paragraphs (`NOTE:`, `TIP:`, etc.) | ✅ |
| Admonition blocks (`[NOTE]\n====`) | ✅ |
| Unordered lists (`*`, `-`, up to 5 levels) | ✅ |
| Compound list items (list continuation `+`) | ✅ |
| Ordered lists (`.`, `1.`, `a.`, roman numerals) | ✅ |
| Ordered list style from block attr (`[loweralpha]`, etc.) | ✅ |
| Ordered list start value (`[start=N]`) | ✅ |
| Description lists (`term::`) | ✅ |
| Description list compound body blocks | ✅ |
| Callout lists (`<N>`) | ✅ |
| Block images (`image::target[alt]`) | ✅ |
| Video block macro (`video::url[opts]`) | ✅ |
| Audio block macro (`audio::url[opts]`) | ✅ |
| Basic tables (`\|===`) | ✅ |
| Block title (`.Title`) | ✅ |
| Block anchor (`[[id]]`) | ✅ |
| Block attribute lines (`[source,lang]`, etc.) | ✅ |
| Special section names (`[preface]`, `[appendix]`, etc.) | ✅ |
| Floating titles (`[discrete]` on a section-title line) | ✅ |
| Thematic break (`'''`) | ✅ |
| Page break (`<<<`) | ✅ |
| Single-line comments (`// …`) | ✅ |
| Block comments (`////`) | ✅ |
| Special-character escaping (`&`, `<`, `>`) | ✅ |
| Inline bold / italic / monospace / highlight | ✅ |
| Constrained and unconstrained inline markers | ✅ |
| Superscript / subscript | ✅ |
| Attribute references (`{name}`) | ✅ |
| `counter:` / `counter2:` inline macros | ✅ |
| Typographic replacements (`--`, `...`, `(C)`, etc.) | ✅ |
| Inline anchors (`[[id]]`) | ✅ |
| Cross-references (`<<id>>`, `xref:id[]`) | ✅ |
| Explicit link macro (`link:url[text]`, absolute and relative) | ✅ |
| Bare URL auto-linking | ✅ |
| Inline image macro (`image:path[alt]`) | ✅ |
| `kbd:[]`, `btn:[]`, `menu:[]` inline macros | ✅ |
| Hard line-break (` +`) | ✅ |
| Footnotes (`footnote:[text]`, `footnoteref:[id,text]`) | ✅ |
| Inline stem/math macros (`stem:[]`, `latexmath:[]`, `asciimath:[]`) | ✅ |
| Block stem (`[stem]` on a pass block → display math) | ✅ |
| Source callout markers (`<N>` in listings) paired with colist | ✅ |
| ID generation helper (`generate_id`) | ✅ |
| HTML5 converter (all above block / inline types) | ✅ |
| Embedded mode (`--no-header-footer`) | ✅ |
| Safe-mode levels (Unsafe / Safe / Server / Secure) | ✅ |
| CLI (backend, doctype, attributes, safe-mode, dest-dir) | ✅ |
| Section numbering (`:sectnums:`, `:sectnumlevels:`) | ✅ |
| Table of Contents (`:toc:`, `:toclevels:`, `:toc-placement:`) | ✅ |
| Multi-line attribute values (trailing `\` continuation) | ✅ |
| `include::` directive (safe-mode–aware) | ✅ |
| Conditional preprocessing (`ifdef::`, `ifndef::`, `ifeval::`) | ✅ |
| Admonition captions from locale attributes (`note-caption`, etc.) | ✅ |
| Preamble `<div>` only when sections follow | ✅ |
| Stylesheet linking (`:linkcss:`, `:stylesheet:` attrs) | ✅ |
| `docinfo.html` / `docinfo-footer.html` injection (unsafe mode) | ✅ |
| MathJax CDN loader when `:stem:` attribute is set | ✅ |
| `doctype: manpage` title parsing (`manname`, `manvolnum`) | ✅ |
| Man page backend (`-b manpage`, troff/groff output) | ✅ |
| DocBook 5 backend (`-b docbook5`, XML output) | ✅ |
| Table column spec: proportional, alignment, repeat, style | ✅ |
| Logging: missing include file warning | ✅ |
| Section nesting validation warning | ✅ |

---

## What Is Missing

The items below are grouped by priority.  Items are **P3** (advanced or rarely used).

### P3 – Advanced / Optional

22. ~~**Man page backend** (`-b manpage`)~~ **Implemented** (`manpage.hpp`)
    - Generates troff/groff output for sections, paragraphs, lists,
      description lists, listing blocks (`.nf`/`.fi`), admonitions,
      and compound blocks.  Verified with `man -l` and `groff -man`.

23. ~~**Full table-column spec parsing**~~ **Implemented** (`parser.cpp`, `html5.hpp`)
    - Parses proportional widths (`1*,2*`), alignment prefixes (`<^>`),
      repeat notation (`3*`), header/style suffixes (`h`, `e`, etc.),
      and auto-width columns (`~`).
    - Column alignment is applied in the HTML `<colgroup>` and cell CSS classes.
    - AsciiDoc-style cell content (the `a|` cell type for nested AsciiDoc)
      remains unimplemented (niche feature; consider for a future pass).

24. **Markdown-compatible section titles** (`#`, `##`, …) — **Out of scope**
    - Ruby Asciidoctor historically supported Markdown-style headings
      via `Asciidoctor::Compliance.markdown_syntax`.  This mode is rarely
      needed and conflicts with AsciiDoc's `#` line-comment intent.
    - **Decision:** out of scope; document users should use `=` headings.

25. ~~**Logging / diagnostics**~~ **Partially implemented** (`parser.cpp`)
    - Missing include files now emit a `WARNING` to `stderr`.
    - Section nesting violations now emit a `WARNING` to `stderr`.
    - Remaining gaps: unknown attribute warnings, unclosed-block detection,
      and structured sourcemap logging (`:sourcemap:` attribute).

26. ~~**Section nesting validation**~~ **Implemented** (`parser.cpp`)
    - When a section is nested more than one level deeper than its parent,
      a warning is emitted to `stderr`.

21. **DocBook 5 backend** (`-b docbook5`)
    - The CLI already falls back to html5 for unknown backends.
    - Ruby source: `lib/asciidoctor/converter/docbook5.rb` (837 lines).

---

## TODO - PDF Output

This is a bit of a hail mary, but see what you can do - I've added strutype, utf8, TextFlow.hpp and a
stripped down copy of libharu to the repository.  See what you can put
together to produce PDFs using logic extracted from those components - you don't have
to use any of them verbatim - for example, perhaps you can generalize TextFlow using
utf8 or struetype info....
(struetype is header only, and utf8 is also small, so you can probably leave
them as is, but please identify what you need out of libharu and rework it into
a single C++17 header-only version, keeping their copyright and license at the
top of the file.)
The idea here will be user can do html5 to pdf via web browser themselves,
but we want at least a basic, decent looking capability

---

## Probably pass - items too complex to be worthwhile without clear need

19. **Extensions API** (`register`, `preprocessor`, `block_processor`, etc.)
    - The Ruby extensions model (`lib/asciidoctor/extensions.rb`, 1551 lines)
      is very large and tightly coupled to the Ruby object model.  A C++
      equivalent would require a plugin ABI or embedded scripting engine.
    - **Recommendation:** defer; document extension point interfaces instead.

20. **Syntax highlighting** (coderay, highlight.js, pygments, rouge)
    - Currently source blocks emit plain `<code>` tags without highlighting.
    - Could integrate a C++ highlighting library (e.g. `highlight`) or emit
      the data attributes that a client-side JS library expects.
    - Ruby source: `lib/asciidoctor/syntax_highlighter/`.

21. ~~**DocBook 5 backend** (`-b docbook5`)~~ **Implemented** (`docbook5.hpp`)
    - Produces valid DocBook 5.0 XML with `<article>` / `<book>` root.
    - Covers: `<info>` (title, authors, date), `<section>` / `<chapter>`,
      paragraphs, listing/literal blocks (`<programlisting>`,
      `<literallayout>`), admonitions, `<itemizedlist>`, `<orderedlist>`,
      `<variablelist>`, `<calloutlist>`, CALS tables, block images
      (`<mediaobject>`), inline bold/italic/mono, and special-char escaping.
    - Ruby source: `lib/asciidoctor/converter/docbook5.rb` (837 lines).

## Bugs

| 5 | `std::regex` is compiled on every call to `sub_quotes` etc. (static local works, but GCC's `<regex>` is slow – see performance section) | `substitutors.hpp` | Medium |
| 8 | Section nesting deeper than one level beyond parent is silently accepted | `parser.cpp` | Low |

---

## Performance Comparison

Benchmark methodology: in-process loop (no process-spawn overhead), 1000
iterations of parse + HTML5 conversion on `benchmark/sample-data/mdbasics.adoc`
(335 lines, ~9 KB), with a 10-iteration warm-up, `-O2` optimisation.

| Implementation | Language | Avg time / iter | Conversions / sec |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 | Ruby 3.2.3 | ~2.3 ms | ~440 |
| asciiquack (this repo) | C++17 / GCC 13 -O2 | ~2.9 ms | ~345 |

**The C++ implementation is currently ~25–35% slower than the Ruby version**
on this benchmark.  This is counter-intuitive and worth investigating.

### Root Causes

1. **`std::regex` performance** – GCC's `<regex>` implementation is well-known
   to be slower than most competing regular-expression engines.  The
   substitution pipeline calls many regex operations on every paragraph.
   Profiling consistently shows `sub_quotes`, `sub_macros`, and
   `sub_replacements` dominating runtime.

2. **`shared_ptr` reference counting** – Every AST node is heap-allocated and
   managed through `std::shared_ptr`.  The atomic reference-count increments
   and decrements add up across large documents.

3. **`std::ostringstream` for output building** – Each `<<` operation on an
   `ostringstream` can involve heap reallocation.  A pre-reserved
   `std::string` buffer would be faster.

4. **Feature disparity** – The Ruby version processes the full document
   (including TOC, includes, etc.) while the C++ version silently skips
   those features, so the Ruby run does more real work.

### Suggested Optimisations

- Replace `std::regex` with a faster library such as **RE2** or **PCRE2**,
  or hand-roll the subset of patterns actually needed.
- Use `std::string` with `reserve()` instead of `ostringstream` in the HTML5
  converter.
- Consider arena allocation for AST nodes to eliminate per-node `shared_ptr`
  overhead.
- Profile with `perf` or `gprof` to confirm which functions dominate.

---

## Build Instructions

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run tests
ctest  # or: ./asciiquack_tests

# Run in-process benchmark
./bench_asciiquack [file] [iterations]
```

---

## References

- Upstream Ruby implementation: `lib/asciidoctor/`
- AsciiDoc specification: <https://docs.asciidoctor.org/asciidoc/latest/>
- cxxopts (CLI parsing): <https://github.com/jarro2783/cxxopts>


---

## What Is Working

| Feature | Status |
|---|---|
| Reader (line-by-line, CRLF, push-back, blank-skip) | ✅ |
| Document header (ATX `=` and setext `====` titles) | ✅ |
| Author line and multiple authors (`;` separated) | ✅ |
| Revision line (`vN.N, date: remark`) | ✅ |
| Attribute entries (`:name: value`, `:!name:`) | ✅ |
| Paragraphs (multi-line, joined with space) | ✅ |
| Literal paragraphs (leading whitespace) | ✅ |
| Section titles (`==` through `======`) | ✅ |
| Setext-style body section titles | ✅ |
| ATX section ID generation (`idprefix`, `idseparator`) | ✅ |
| Listing / source blocks (`----`) | ✅ |
| Literal blocks (`....`) | ✅ |
| Example blocks (`====`) | ✅ |
| Sidebar blocks (`****`) | ✅ |
| Quote / verse blocks (`____`) | ✅ |
| Passthrough blocks (`++++`) | ✅ |
| Open blocks (`--`) | ✅ |
| Admonition paragraphs (`NOTE:`, `TIP:`, etc.) | ✅ |
| Admonition blocks (`[NOTE]\n====`) | ✅ |
| Unordered lists (`*`, `-`, up to 5 levels) | ✅ |
| Ordered lists (`.`, `1.`, `a.`, roman numerals) | ✅ |
| Description lists (`term::`) | ✅ |
| Callout lists (`<N>`) | ✅ |
| Block images (`image::target[alt]`) | ✅ |
| Basic tables (`\|===`) | ✅ |
| Block title (`.Title`) | ✅ |
| Block anchor (`[[id]]`) | ✅ |
| Block attribute lines (`[source,lang]`, etc.) | ✅ |
| Thematic break (`'''`) | ✅ |
| Page break (`<<<`) | ✅ |
| Single-line comments (`// …`) | ✅ |
| Block comments (`////`) | ✅ |
| Special-character escaping (`&`, `<`, `>`) | ✅ |
| Inline bold / italic / monospace / highlight | ✅ |
| Constrained and unconstrained inline markers | ✅ |
| Superscript / subscript | ✅ |
| Attribute references (`{name}`) | ✅ |
| Typographic replacements (`--`, `...`, `(C)`, etc.) | ✅ |
| Inline anchors (`[[id]]`) | ✅ |
| Cross-references (`<<id>>`, `xref:id[]`) | ✅ |
| Explicit link macro (`link:url[text]`, absolute and relative) | ✅ |
| Bare URL auto-linking | ✅ |
| Inline image macro (`image:path[alt]`) | ✅ |
| Hard line-break (` +`) | ✅ |
| ID generation helper (`generate_id`) | ✅ |
| HTML5 converter (all above block / inline types) | ✅ |
| Embedded mode (`--no-header-footer`) | ✅ |
| Safe-mode levels (Unsafe / Safe / Server / Secure) | ✅ |
| CLI (backend, doctype, attributes, safe-mode, dest-dir) | ✅ |

---

## What Is Missing

The items below are grouped by priority.  Items marked **P1** are needed for
real-world documents; **P2** are common but not universal; **P3** are advanced
or rarely used.

### P1 – Critical for Real-World Use

1. **`include::` directive** (`include::path[opts]`)
   - Recursive file inclusion is the most frequently used AsciiDoc feature
     in large documentation sets.
   - Needs: safe-mode–aware file resolution, depth limiting, `tag::` /
     `lines::` / `leveloffset::` attribute support, and cycle detection.
   - Ruby source: `lib/asciidoctor/reader.rb` (`PreprocessorReader`,
     `IncludeProcessor`).

2. **Conditional preprocessing** (`ifdef::`, `ifndef::`, `ifeval::`)
   - `ifdef::attr[...]` / `ifdef::attr[]` … `endif::attr[]`
   - `ifeval::[expr]` … `endif::[]`
   - Required by virtually every non-trivial documentation project to
     conditionally include/exclude content based on attributes.
   - Ruby source: `lib/asciidoctor/reader.rb` (PreprocessorReader).

3. **Table of Contents** (`:toc:` attribute, `toc::[]` macro)
   - Needs: title registry during parsing, depth control (`:toclevels:`),
     placement (`:toc-placement:`) and rendering in the HTML header.
   - Ruby source: `lib/asciidoctor/document.rb`,
     `lib/asciidoctor/converter/html5.rb` (`convert_outline`).

4. **Section numbering** (`:sectnums:`, `:sectnumlevels:`)
   - Incremental counters per level, applied during conversion.
   - Ruby source: `lib/asciidoctor/section.rb`.

5. **Multi-line attribute values** (trailing `\` continuation)
   - `:attr: first line \` / `second line`
   - Common in build-system–generated attribute files.

### P2 – Commonly Needed

6. **Footnotes** (`footnote:[text]`, `footnoteref:[id]`)
   - Collect during parsing, render at document end.
   - Ruby source: `lib/asciidoctor/substitutors.rb` (`sub_inline_passthrough`),
     `lib/asciidoctor/converter/html5.rb` (`convert_floating_title`).

7. **Compound list items** (list continuation `+`)
   - A list item body separated by a `+` line may contain multiple blocks
     (paragraphs, listings, etc.).  Currently only inline text continuation
     is collected.

8. **Description list – full compound items**
   - Description list items may contain entire block structures.  The current
     implementation captures only a single line of body text.

9. **Inline passthrough** (`pass:[raw]`, `+mono+` passthrough fence,
   `` `raw HTML` `` pass)
   - `pass:q[text]` – apply only the `q` (quotes) substitution.
   - `pass:c[<b>raw</b>]` – pass through raw HTML.
   - Ruby source: `lib/asciidoctor/substitutors.rb` (`sub_inline_passthrough`).

10. **Video and audio block macros** (`video::url[opts]`, `audio::url[opts]`)
    - Ruby source: `lib/asciidoctor/converter/html5.rb`.

11. **Numbered ordered-list styles** from block attribute
    (`[loweralpha]`, `[upperroman]`, etc.) and custom start value
    (`[start=3]`).

12. **`doctype: manpage`** special processing
    - Synopsis, Name section, and man-page–specific section handling.
    - Ruby source: `lib/asciidoctor/document.rb` (doctype: manpage path).

13. **Preamble / abstract as standalone section**
    - When a document starts directly with body content and no header, the
      Ruby version wraps it in a preamble `<div id="preamble">` only if there
      are subsequent sections.  Complex edge cases exist.

14. **Special section names** (`[preface]`, `[appendix]`, `[abstract]`,
    `[colophon]`, `[glossary]`, `[bibliography]`, `[index]`)
    - These trigger different CSS classes, numbering behaviour, and sometimes
      different header levels.

15. **Floating titles** (`[discrete]` on a section-title line)
    - Renders as an `<hN>` element outside the normal section structure.
    - Ruby source: `lib/asciidoctor/converter/html5.rb`
      (`convert_floating_title`).

16. **`idprefix` / `idseparator` defaults**
    - When `idprefix` is set to empty string and `idseparator` to `-`
      (common for compatibility), IDs are generated as `section-title`
      instead of `_section_title`.  This is already supported but needs a
      test for the empty-prefix case.

17. **`counter:` and `counter2:` inline macros**
    - Document-level incrementing counters used in attribute references.

18. **`btn:[]`, `kbd:[]`, `menu:[]` inline macros**
    - Experimental/common UI macro extensions.

### P3 – Advanced / Optional

19. **Extensions API** (`register`, `preprocessor`, `block_processor`, etc.)
    - The Ruby extensions model (`lib/asciidoctor/extensions.rb`, 1551 lines)
      is very large and tightly coupled to the Ruby object model.  A C++
      equivalent would require a plugin ABI or embedded scripting engine.
    - **Recommendation:** defer; document extension point interfaces instead.

20. **Syntax highlighting** (coderay, highlight.js, pygments, rouge)
    - Currently source blocks emit plain `<code>` tags without highlighting.
    - Could integrate a C++ highlighting library (e.g. `highlight`) or emit
      the data attributes that a client-side JS library expects.
    - Ruby source: `lib/asciidoctor/syntax_highlighter/`.

21. ~~**DocBook 5 backend** (`-b docbook5`)~~ **Implemented** (`docbook5.hpp`)
    - See the P3 section above for full implementation notes.
    - Ruby source: `lib/asciidoctor/converter/docbook5.rb` (837 lines).

22. **Man page backend** (`-b manpage`)
    - Ruby source: `lib/asciidoctor/converter/manpage.rb` (757 lines).

23. **Source callout rendering** (`<N>` markers in source blocks)
    - Callout numbers in source blocks are paired with `<N>` items in the
      following callout list.  Currently the markers are passed through as
      literal text.

24. **Full table-column spec parsing**
    - The `cols` attribute supports rich specifiers: `cols="1,2,3"`,
      `cols="1*,2*"` (proportional), `cols=">1,^2,<3"` (alignment),
      `cols="1h,2,3"` (header column style), etc.
    - AsciiDoc-style cell content (the `a|` cell type for nested AsciiDoc).

25. **Markdown-compatible section titles** (`#`, `##`, …)
    - Ruby Asciidoctor historically supported Markdown-style headings
      via `Asciidoctor::Compliance.markdown_syntax`.  This mode is rarely
      needed; document it as out-of-scope or add as an option.

26. **`stem:[]` / `latexmath:[]` inline and block macros**
    - LaTeX / MathML math rendering.
    - Typically delegates to MathJax or KaTeX at runtime.

27. **docinfo files** (header/footer HTML fragments injected from disk)
    - `docinfo.html`, `<docname>-docinfo.html`, etc.
    - Requires safe-mode–aware file I/O.

28. **Stylesheet linking** (`:linkcss:` attribute, `:stylesheet:` attribute)
    - Currently the converter always inlines a minimal CSS block.
    - Should support linking to an external file or the full Asciidoctor
      default stylesheet.

29. **`toc-title:` and other localisation attributes**
    - Built-in strings ("Note", "Tip", "Figure", "Table", etc.) should use
      the locale attributes from `data/locale/attributes-<lang>.adoc`.

30. **Logging / diagnostics** (`:sourcemap:` attribute, warning messages)
    - The Ruby implementation emits structured log messages for missing
      includes, unknown attributes, unclosed blocks, etc.
    - Ruby source: `lib/asciidoctor/logging.rb`.

---

## Known Bugs / Limitations (Pre-Existing Before This Analysis)

| # | Description | File | Severity |
|---|---|---|---|
| 1 | ~~Compound delimited blocks (====, ****, ____) ran to EOF~~ | `parser.cpp` | **Fixed** |
| 2 | ~~Setext title detection matched block-attribute lines (`[...]`)~~ | `parser.cpp` | **Fixed** |
| 3 | ~~`link:` macro only matched `http(s)://` URLs~~ | `substitutors.hpp` | **Fixed** |
| 4 | Constrained inline-bold regex can produce false positives near URLs | `substitutors.hpp` | Medium |
| 5 | `std::regex` is compiled on every call to `sub_quotes` etc. (static local works, but GCC's `<regex>` is slow – see performance section) | `substitutors.hpp` | Medium |
| 6 | Attribute entries with multi-line values (trailing `\`) silently discard the continuation | `parser.cpp` | Low |
| 7 | Description list term regex can match table separator rows | `parser.cpp` | Low |
| 8 | ~~Section nesting deeper than one level beyond parent is silently accepted~~ | `parser.cpp` | **Fixed** (now emits WARNING) |

---

## Performance Comparison

Benchmark methodology: in-process loop (no process-spawn overhead), 1000
iterations of parse + HTML5 conversion on `benchmark/sample-data/mdbasics.adoc`
(335 lines, ~9 KB), with a 10-iteration warm-up, `-O2` optimisation.

| Implementation | Language | Avg time / iter | Conversions / sec |
|---|---|---|---|
| Ruby Asciidoctor 2.1.0 | Ruby 3.2.3 | ~2.3 ms | ~440 |
| asciiquack (this repo) | C++17 / GCC 13 -O2 | ~2.9 ms | ~345 |

**The C++ implementation is currently ~25–35% slower than the Ruby version**
on this benchmark.  This is counter-intuitive and worth investigating.

### Root Causes

1. **`std::regex` performance** – GCC's `<regex>` implementation is well-known
   to be slower than most competing regular-expression engines.  The
   substitution pipeline calls many regex operations on every paragraph.
   Profiling consistently shows `sub_quotes`, `sub_macros`, and
   `sub_replacements` dominating runtime.

2. **`shared_ptr` reference counting** – Every AST node is heap-allocated and
   managed through `std::shared_ptr`.  The atomic reference-count increments
   and decrements add up across large documents.

3. **`std::ostringstream` for output building** – Each `<<` operation on an
   `ostringstream` can involve heap reallocation.  A pre-reserved
   `std::string` buffer would be faster.

4. **Feature disparity** – The Ruby version processes the full document
   (including TOC, includes, etc.) while the C++ version silently skips
   those features, so the Ruby run does more real work.

### Suggested Optimisations

- Replace `std::regex` with a faster library such as **RE2** or **PCRE2**,
  or hand-roll the subset of patterns actually needed.
- Use `std::string` with `reserve()` instead of `ostringstream` in the HTML5
  converter.
- Consider arena allocation for AST nodes to eliminate per-node `shared_ptr`
  overhead.
- Profile with `perf` or `gprof` to confirm which functions dominate.

---

## Build Instructions

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run tests
ctest  # or: ./asciiquack_tests

# Run in-process benchmark
./bench_asciiquack [file] [iterations]
```

---

## References

- Upstream Ruby implementation: `lib/asciidoctor/`
- AsciiDoc specification: <https://docs.asciidoctor.org/asciidoc/latest/>
- cxxopts (CLI parsing): <https://github.com/jarro2783/cxxopts>
