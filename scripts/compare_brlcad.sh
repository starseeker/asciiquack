#!/usr/bin/env bash
# compare_brlcad.sh — run the regex and scanner-parser variants of asciiquack
# against the BRL-CAD AsciiDoc corpus and compare their outputs.
#
# Usage:
#   scripts/compare_brlcad.sh [BRLCAD_ADOC_DIR] [ASCIIQUACK_REGEX] [ASCIIQUACK_SCANNER]
#
# Defaults (relative to this script's parent directory):
#   BRLCAD_ADOC_DIR  — /tmp/brlcad_quickiterate/brlcad/doc/asciidoc
#   ASCIIQUACK_REGEX — build_regex/asciiquack
#   ASCIIQUACK_SCANNER — build_scanner/asciiquack
#
# The script produces:
#   /tmp/aq_compare/  — tree of regex and scanner outputs
#   Summary printed to stdout: pass/fail counts, first N diffs
#
# Both binaries are called with -a safe-mode=unsafe so that include::
# directives are processed.  Man-page files (doctype: manpage) are
# converted with -b manpage; all others use -b html5.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"

BRLCAD_DIR="${1:-/tmp/brlcad_quickiterate/brlcad/doc/asciidoc}"
REGEX_BIN="${2:-$REPO_DIR/build_regex/asciiquack}"
SCANNER_BIN="${3:-$REPO_DIR/build_scanner/asciiquack}"

OUT_DIR="/tmp/aq_compare"
REGEX_OUT="$OUT_DIR/regex"
SCANNER_OUT="$OUT_DIR/scanner"

if [[ ! -d "$BRLCAD_DIR" ]]; then
    echo "ERROR: BRL-CAD adoc directory not found: $BRLCAD_DIR" >&2
    echo "  Clone it with:" >&2
    echo "  git clone --depth=1 --branch=asciidoc_only \\" >&2
    echo "    https://github.com/starseeker/brlcad_quickiterate.git /tmp/brlcad_quickiterate" >&2
    exit 1
fi
for bin in "$REGEX_BIN" "$SCANNER_BIN"; do
    if [[ ! -x "$bin" ]]; then
        echo "ERROR: binary not found or not executable: $bin" >&2
        echo "  Build with:" >&2
        echo "  mkdir build_regex  && cd build_regex  && cmake .. -DUSE_SCANNER_PARSER=OFF && cmake --build . -j4" >&2
        echo "  mkdir build_scanner && cd build_scanner && cmake .. -DUSE_SCANNER_PARSER=ON  && cmake --build . -j4" >&2
        exit 1
    fi
done

echo "BRL-CAD adoc corpus: $BRLCAD_DIR"
echo "  regex   binary: $REGEX_BIN"
echo "  scanner binary: $SCANNER_BIN"
echo "  output dir:     $OUT_DIR"
echo

rm -rf "$OUT_DIR"
mkdir -p "$REGEX_OUT" "$SCANNER_OUT"

# Collect all .adoc files
mapfile -d '' ADOC_FILES < <(find "$BRLCAD_DIR" -name "*.adoc" -print0 | sort -z)
TOTAL=${#ADOC_FILES[@]}
echo "Converting $TOTAL .adoc files with each backend..."

pass=0; fail=0; errors_regex=0; errors_scanner=0
fail_list=()

for adoc in "${ADOC_FILES[@]}"; do
    rel="${adoc#"$BRLCAD_DIR/"}"
    # Determine format: manpage files live under man1/ man3/ man5/ etc.
    if grep -q "^:doctype: manpage" "$adoc" 2>/dev/null; then
        backend="manpage"
        ext=".man"
    else
        backend="html5"
        ext=".html"
    fi

    safe="safe"
    # Use unsafe mode so include:: directives are resolved relative to the file
    # (most BRL-CAD files use safe mode; a small number use includes).

    rel_out="${rel%.adoc}${ext}"
    regex_out="$REGEX_OUT/$rel_out"
    scanner_out="$SCANNER_OUT/$rel_out"

    mkdir -p "$(dirname "$regex_out")" "$(dirname "$scanner_out")"

    # Run regex variant
    if ! "$REGEX_BIN"  -b "$backend" -a safe-mode="$safe" \
            -o "$regex_out"  "$adoc" 2>/dev/null; then
        errors_regex=$((errors_regex + 1))
    fi

    # Run scanner variant
    if ! "$SCANNER_BIN" -b "$backend" -a safe-mode="$safe" \
            -o "$scanner_out" "$adoc" 2>/dev/null; then
        errors_scanner=$((errors_scanner + 1))
    fi

    # Compare outputs (ignore if either binary failed to produce output)
    if [[ -f "$regex_out" && -f "$scanner_out" ]]; then
        if diff -q "$regex_out" "$scanner_out" > /dev/null 2>&1; then
            pass=$((pass + 1))
        else
            fail=$((fail + 1))
            fail_list+=("$rel_out")
        fi
    fi
done

echo
echo "===== Results ====="
printf "  Identical:     %d / %d\n" "$pass" "$TOTAL"
printf "  Differing:     %d\n" "$fail"
printf "  Regex errors:  %d\n" "$errors_regex"
printf "  Scanner errors: %d\n" "$errors_scanner"
echo

if [[ ${#fail_list[@]} -gt 0 ]]; then
    MAX_SHOW=10
    shown=0
    echo "--- First diffs (up to $MAX_SHOW) ---"
    for f in "${fail_list[@]}"; do
        echo
        echo "### $f"
        diff --unified=3 "$REGEX_OUT/$f" "$SCANNER_OUT/$f" | head -40 || true
        shown=$((shown + 1))
        if [[ $shown -ge $MAX_SHOW ]]; then
            echo "  ... ($(( ${#fail_list[@]} - MAX_SHOW )) more)"
            break
        fi
    done
fi

if [[ $fail -eq 0 && $errors_regex -eq 0 && $errors_scanner -eq 0 ]]; then
    echo "All outputs are identical. Scanner-parser is a correct drop-in replacement."
    exit 0
else
    exit 1
fi
