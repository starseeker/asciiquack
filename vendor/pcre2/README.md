# Embedded PCRE2 subset (10.42)

A minimal subset of PCRE2 10.42 vendored in this repository so that
asciiquack can use the PCRE2 regex engine without an external
`libpcre2-dev` dependency.

## What is included

| File | Role |
|---|---|
| `config.h` | Minimal build configuration (no JIT, no Unicode props) |
| `pcre2.h` | Public API header (version 10.42) |
| `pcre2_internal.h` | Private internal types and macros |
| `pcre2_intmodedep.h` | Mode-dependent types (8-bit) |
| `pcre2_ucp.h` | Unicode category constants |
| `pcre2_chartables.c` | Character-type lookup tables (locale-independent) |
| `pcre2_compile.c` | Pattern compiler |
| `pcre2_auto_possess.c` | Compile-time optimisation pass |
| `pcre2_find_bracket.c` | Bracket scanning helper |
| `pcre2_match.c` | NFA backtracking match engine |
| `pcre2_match_data.c` | Match-data allocation |
| `pcre2_substitute.c` | Global substitution (`regex_replace`) |
| `pcre2_pattern_info.c` | Query compiled-pattern metadata |
| `pcre2_study.c` | Optional post-compile optimisation |
| `pcre2_context.c` | General/match/convert context management |
| `pcre2_error.c` | Error message strings |
| `pcre2_newline.c` | Newline convention helpers |
| `pcre2_string_utils.c` | Internal string utilities |
| `pcre2_maketables.c` | Build character tables at startup |
| `pcre2_tables.c` | Static character-class tables |
| `pcre2_xclass.c` | Extended character classes |
| `pcre2_ord2utf.c` | Ordinal → UTF-8 encoder |
| `pcre2_valid_utf.c` | UTF-8 validity checker |
| `pcre2_jit_compile.c` | JIT entry-point (compiles to stubs – no `SUPPORT_JIT`) |
| `pcre2_jit_match.c` | JIT match stub |
| `pcre2_jit_misc.c` | JIT misc stubs |

## What is NOT included

| Component | Reason |
|---|---|
| JIT (`SUPPORT_JIT`) | Platform-specific machine code; 14 500-line SLJIT compiler |
| Unicode properties (`SUPPORT_UNICODE`) | `\p{}`, `\P{}`, script-run support; 5 000-line `pcre2_ucd.c` |
| DFA matching (`pcre2_dfa_match.c`) | Alternative matching engine; not used by asciiquack |
| Serialization (`pcre2_serialize.c`) | Save/restore compiled patterns; not used |
| POSIX wrapper (`pcre2posix.c`) | POSIX-compatible API; not used |
| Pattern conversion (`pcre2_convert.c`) | Glob/POSIX→PCRE2 conversion; not used |
| Tools (`pcre2test.c`, `pcre2grep.c`) | Standalone programs |

## Performance

Without JIT the embedded engine still significantly outperforms GCC's
`std::regex` (~3× speedup on the asciiquack benchmark).  When
`libpcre2-dev` is found on the system, CMake automatically uses the
system library (which includes JIT) instead, giving ~5× speedup.

## Updating

To update to a newer PCRE2 release, re-run the source copy from a fresh
`git clone https://github.com/PCRE2Project/pcre2` at the desired tag,
regenerate `pcre2_chartables.c` via `cmake` in the PCRE2 build tree, and
replace all files in this directory.

## Licence

PCRE2 is distributed under the BSD licence.  See the copyright notices
at the top of each `.c` / `.h` file.
