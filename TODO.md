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
| Logging: missing include file warning | |
| Logging: section nesting skip warning | |
| Logging: unclosed block warning | |

---

## What Remains

### Syntax highlighting

Source blocks currently emit plain `<code>` tags.  A future pass could
integrate a C++ highlighting library or emit the `data-lang` attributes
needed by a client-side JS highlighter such as highlight.js.

### PDF output

See the PDF section below.

---

## Out of Scope

| Feature | Reason |
|---|---|
| Extensions API (`register`, `preprocessor`, etc.) | Requires plugin ABI or embedded scripting; too tightly coupled to Ruby object model |
| Markdown-style headings (`#`, `##`, …) | Conflicts with AsciiDoc `#` line-comment; document users should use `=` headings |
| Structured sourcemap logging (`:sourcemap:`) | Complex feature with limited practical value |

---

## PDF Output

`struetype.h`, `utf8/`, `TextFlow.hpp`, and a stripped-down copy of `libharu`
are included in the repository as a starting point for a native PDF backend.
The goal is basic, decent-looking output; polished HTML-to-PDF via a web
browser remains the recommended path for production use.

Work needed:
- Identify the minimal libharu API surface needed and rework into a single
  C++17 header-only file (preserving the original copyright / license header).
- Generalise `TextFlow.hpp` using the utf8 and struetype helpers as appropriate.
- Implement a `pdf.hpp` backend analogous to `html5.hpp`.

---

## Performance Notes

Benchmark: 1000 in-process iterations on `benchmark/sample-data/mdbasics.adoc`
(335 lines, ~9 KB), 10-iteration warm-up, `-O2`.

| Implementation | Avg / iter | Conv / sec |
|---|---|---|
| Ruby Asciidoctor 2.1.0 (Ruby 3.2.3) | ~2.3 ms | ~440 |
| asciiquack C++17 / GCC 13 -O2 | ~2.9 ms | ~345 |

The C++ port is currently ~25–35 % slower than Ruby.  Root causes:

- **`std::regex`** – GCC's implementation is slower than RE2/PCRE2.
  `sub_quotes`, `sub_macros`, and `sub_replacements` dominate profiling.
- **`shared_ptr` overhead** – atomic ref-count increments on every AST node.
- **`ostringstream` allocations** – pre-reserving a `std::string` buffer
  in the converters would help.

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
