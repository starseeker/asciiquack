// @file block_scanner.re
// @brief re2c source for the AsciiDoc block-level line scanner.
//
// This file is compiled by re2c to produce block_scanner_gen.h.
// Command: re2c --no-generation-date -W -o block_scanner_gen.h block_scanner.re
//
// A pre-generated copy is committed as block_scanner_gen.h so that re2c is
// NOT required at build time.  When USE_RE2C=ON and re2c is found by CMake,
// the file is regenerated automatically.
// Design notes:
// -------------
// * The input is a single AsciiDoc line passed as (const char*, size_t len)
//   where line[len] == '\0'.  std::string::c_str()/size() is always correct.
// * Rules are ordered so more specific patterns appear first; re2c uses the
//   FIRST matching rule on ties (same length).
// * Comment lines ("//...") appear before description-list rules, providing
//   the same guard as the (?!//[^/]) negative lookahead in the PCRE2 pattern.
// * Thematic-break patterns that require backreferences (e.g., the "---" /
//   "* * *" / "_ _ _" family using ([-*_])( *)\1\2\1) are NOT handled here.
//   The wrapper in block_scanner.c detects them with a fast C function BEFORE
//   calling aq_classify_line().  This keeps the re2c DFA free of patterns
//   that are inherently non-regular.
// * Delimited-block fence rules cover 2 characters ("--") through 4+.
//   The three-character fence ("---", "...") is intentionally absent so that
//   it falls through to the AQ_BT_TEXT default, allowing the wrapper's
//   thematic-break pre-check to reclaim it.

#include "block_scanner.h"
#include <stddef.h>

/* ── Internal DFA classifier ─────────────────────────────────────────────────
 *
 * Called by aq_scan_block_line() AFTER the thematic-break pre-check.
 * Returns the broad token type; capture extraction is done by the wrapper.
 */
static AqBlockToken aq_classify_line(const char *p, size_t len)
{
    const char *pe       = p + len;  /* NUL sentinel is at *pe  */
    const char *YYMARKER = p;        /* backtrack marker         */
    (void)YYMARKER;
    (void)pe;  /* YYLIMIT alias; unused when yyfill:enable = 0 */

    /*!re2c
    re2c:define:YYCTYPE   = "unsigned char";
    re2c:define:YYCURSOR  = p;
    re2c:define:YYLIMIT   = pe;
    re2c:define:YYMARKER  = YYMARKER;
    re2c:yyfill:enable    = 0;

    WS   = [ \t];
    NWSE = [^ \t\x00];         /* non-whitespace, non-NUL  */
    ANY  = [^\x00];            /* any byte except NUL      */
    EOL  = "\x00";             /* end-of-line sentinel     */
    DIG  = [0-9];
    AZ   = [A-Za-z];
    WORD = [A-Za-z0-9_];
    NOTBR = [^\x5b\x00];      /* not '[', not NUL         */

    /* ── Blank ──────────────────────────────────────────────────────────── */
    WS* EOL { return AQ_BT_BLANK; }

    /* ── Page break: <<< ────────────────────────────────────────────────── */
    "<<<" "<"* EOL { return AQ_BT_PAGE_BREAK; }

    /* ── Block-comment delimiter: exactly //// ──────────────────────────── */
    "////" EOL { return AQ_BT_BLOCK_COMMENT; }

    /* ── Single-line comment: // followed by anything except a third '/'
     *    (placing this before description-list rules satisfies the original
     *    (?!//[^/]) negative lookahead guard) ────────────────────────────── */
    "//" [^\x2f\x00] ANY* EOL { return AQ_BT_COMMENT; }
    "//"                  EOL { return AQ_BT_COMMENT; }

    /* ── Attribute entry: :name: [value]
     *
     *    Equivalent to PCRE2: ^:(!?[\w][\w\-' ]*):(?:[ \t]+(.*))?$
     *    name = optional '!' then word-char then word/hyphen/space/apostrophe*
     * ──────────────────────────────────────────────────────────────────── */
    ":" [!]? WORD [A-Za-z0-9_\-' ]* ":" (WS+ ANY*)? EOL {
        return AQ_BT_ATTR_ENTRY;
    }

    /* ── ATX section title: 1-6 '=' followed by whitespace and text ─────── */
    "="{1,6} WS+ NWSE ANY* EOL { return AQ_BT_SECTION_TITLE; }

    /* ── Block image macro: image::target[attrs] ────────────────────────── */
    "image::" NWSE NOTBR* "[" ANY* "]" EOL { return AQ_BT_BLOCK_IMAGE; }

    /* ── Block media macros: video:: or audio:: ─────────────────────────── */
    "video::" NWSE NOTBR* "[" ANY* "]" EOL { return AQ_BT_BLOCK_MEDIA; }
    "audio::" NWSE NOTBR* "[" ANY* "]" EOL { return AQ_BT_BLOCK_MEDIA; }

    /* ── Conditional directives ─────────────────────────────────────────── */
    "ifdef::"  WORD ANY* "[" ANY* "]" EOL { return AQ_BT_IFDEF;  }
    "ifndef::" WORD ANY* "[" ANY* "]" EOL { return AQ_BT_IFNDEF; }
    "ifeval::[" ANY* "]"              EOL { return AQ_BT_IFEVAL; }
    "endif::"  ANY*                   EOL { return AQ_BT_ENDIF;  }

    /* ── Include directive ───────────────────────────────────────────────── */
    "include::" NWSE NOTBR* "[" ANY* "]" EOL { return AQ_BT_INCLUDE; }

    /* ── Delimited-block fences ──────────────────────────────────────────── */
    /* Open block: exactly "--" */
    "--" EOL { return AQ_BT_DELIMITER; }
    /* Standard four-character fences */
    "----" EOL { return AQ_BT_DELIMITER; }
    "...." EOL { return AQ_BT_DELIMITER; }
    "====" EOL { return AQ_BT_DELIMITER; }
    "____" EOL { return AQ_BT_DELIMITER; }
    "****" EOL { return AQ_BT_DELIMITER; }
    "++++" EOL { return AQ_BT_DELIMITER; }
    /* Extended fences: five or more repeated fence characters
     * (four-char fences handled above; three-char fences are excluded
     *  so that "---" / "..." fall through to AQ_BT_TEXT and can be
     *  reclaimed as thematic breaks by the wrapper's pre-check) */
    "-"{5,} EOL { return AQ_BT_DELIMITER; }
    "."{5,} EOL { return AQ_BT_DELIMITER; }
    "="{5,} EOL { return AQ_BT_DELIMITER; }
    "_"{5,} EOL { return AQ_BT_DELIMITER; }
    "*"{5,} EOL { return AQ_BT_DELIMITER; }
    "+"{5,} EOL { return AQ_BT_DELIMITER; }
    "~"{4,} EOL { return AQ_BT_DELIMITER; }

    /* ── Block anchor: [[id]] or [[id, reftext]] ────────────────────────── */
    "[[" WORD ANY* "]]" EOL { return AQ_BT_BLOCK_ANCHOR; }

    /* ── Block-attribute line: [content]
     *    Must start with '[' but not '[[' (handled above), and must not be
     *    empty '[]'. ────────────────────────────────────────────────────── */
    "[" [^\x5b\x5d\x00] ANY* "]" EOL { return AQ_BT_BLOCK_ATTR; }

    /* ── Block title: .Title
     *    Must not start with '. ' (literal paragraph with leading '.') or
     *    '..' (list continuation / literal block delimiter).
     *    Equivalent to: line[0]=='.' && line[1]!=' ' && line[1]!='.'
     * ──────────────────────────────────────────────────────────────────── */
    "." [^ \x2e\x00] ANY* EOL { return AQ_BT_BLOCK_TITLE; }

    /* ── Callout list item: <N> text or <.> text ────────────────────────── */
    "<" (DIG+ | ".") ">" WS+ NWSE ANY* EOL { return AQ_BT_LIST_CALLOUT; }

    /* ── Ordered list items ─────────────────────────────────────────────── */
    /* Auto-numbered dots: one or more dots (PCRE2 uses \.*\. = 1+ dots) */
    WS* "."+  WS+ NWSE ANY* EOL { return AQ_BT_LIST_ORD; }
    /* Numeric: 1. text */
    WS* DIG+ "." WS+ NWSE ANY* EOL { return AQ_BT_LIST_ORD; }
    /* Alpha with period: a. text */
    WS* AZ "." WS+ NWSE ANY* EOL { return AQ_BT_LIST_ORD; }
    /* Roman numeral with paren: i) I) xiv) etc. */
    WS* [IVXivx]+ ")" WS+ NWSE ANY* EOL { return AQ_BT_LIST_ORD; }
    /* ── Unordered list items ────────────────────────────────────────────── */
    WS* "-"      WS+ NWSE ANY* EOL { return AQ_BT_LIST_UNORD; }
    WS* "*"{1,5} WS+ NWSE ANY* EOL { return AQ_BT_LIST_UNORD; }

    /* ── Description list: term:: body  or  term;; body
     *
     *    Comment lines have already been matched above, satisfying the
     *    (?!//[^/]) guard from the original PCRE2 pattern.
     *    Separator is "::" through "::::" (i.e., :: or ::: or ::::) or ";;".
     *    Empty-term forms (":: body") are matched first.
     *
     *    Note: re2c uses sentinel 0 (\x00) for end-of-input, so we must not
     *    include \x00 inside optional-suffix character classes.  The patterns
     *    below arrange the EOL to always be the last matched character.
     * ──────────────────────────────────────────────────────────────────── */
    /* Empty-term forms */
    "::" ":"{0,2} WS+ ANY+ EOL { return AQ_BT_LIST_DESCRIPT; }
    "::" ":"{0,2} EOL          { return AQ_BT_LIST_DESCRIPT; }
    ";;" WS+ ANY+ EOL          { return AQ_BT_LIST_DESCRIPT; }
    ";;" EOL                   { return AQ_BT_LIST_DESCRIPT; }
    /* Non-empty-term forms: at least one non-WS char before the separator */
    /* Allow optional leading whitespace (PCRE2 uses ^([ \t]*)([^ \t].*?)...) */
    WS* NWSE ANY* "::" ":"{0,2} (WS+ ANY+)? EOL { return AQ_BT_LIST_DESCRIPT; }
    WS* NWSE ANY* ";;"              (WS+ ANY+)? EOL { return AQ_BT_LIST_DESCRIPT; }

    /* ── Default: ordinary text ─────────────────────────────────────────── */
    * { return AQ_BT_TEXT; }

    */
}
