/// @file block_scanner.c
/// @brief AsciiDoc block-line scanner: wrapper around re2c DFA + capture
///        extraction + thematic-break pre-check.
///
/// This translation unit provides the single public function:
///   AqBlockScanResult aq_scan_block_line(const char *line, size_t len);
///
/// It works in two steps:
///   1. A fast C pre-check detects the patterns that require backreferences
///      (thematic break) or simple character inspection (''').
///   2. The re2c-generated DFA (included from block_scanner_gen.h) classifies
///      every other line into its AqBlockToken type.
///   3. Capture extraction: once the type is known, simple pointer arithmetic
///      locates the key sub-strings (name, value, marker, text, etc.) without
///      a second regex pass.

#include "block_scanner.h"
#include <string.h>
#include <stddef.h>
#include <ctype.h>

/* Pull in the re2c-generated DFA (defines aq_classify_line as a static). */
#include "block_scanner_gen.h"

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/** Build an AqSpan from two pointers within [line, line+len]. */
static AqSpan make_span(const char *base,
                        const char *s, const char *e)
{
    AqSpan sp;
    sp.off = (unsigned short)(s - base);
    sp.len = (e > s) ? (unsigned short)(e - s) : 0u;
    return sp;
}

/** Skip leading ASCII whitespace; return pointer to first non-WS byte. */
static const char *skip_ws(const char *p, const char *pe)
{
    while (p < pe && (*p == ' ' || *p == '\t')) { ++p; }
    return p;
}

/** Return pointer to first byte past trailing ASCII whitespace. */
static const char *rskip_ws(const char *p, const char *pe)
{
    while (pe > p && (*(pe - 1) == ' ' || *(pe - 1) == '\t')) { --pe; }
    return pe;
}

/* ── Thematic-break pre-check ────────────────────────────────────────────── */
/*
 * Detects:
 *   a) Three or more single-quote characters: '''  '''''  etc.
 *   b) The Markdown-style pattern ^{0,3}([-*_])( *)\1\2\1$
 *      i.e. exactly three of the same character with equal inter-char spacing.
 *
 * This is called BEFORE the re2c DFA so that "---" is not mis-classified as
 * AQ_BT_TEXT (the re2c DFA intentionally leaves three-char dash/dot sequences
 * unclaimed to avoid shadowing the delimiter rules for "----", "----+" etc.).
 */
static int is_thematic_break(const char *line, size_t len)
{
    if (len < 3) { return 0; }

    /* a) Three or more single-quotes */
    {
        size_t i = 0;
        while (i < len && line[i] == '\'') { ++i; }
        if (i == len && i >= 3) { return 1; }
    }

    /* b) Optional 0-3 leading spaces */
    size_t start = 0;
    while (start < 3 && start < len && line[start] == ' ') { ++start; }

    if (start >= len) { return 0; }
    char ch = line[start];
    if (ch != '-' && ch != '*' && ch != '_') { return 0; }

    /* The pattern is: ch [sp*] ch [sp*] ch EOL
     * where both [sp*] runs must be the same length. */
    const char *p = line + start;
    const char *pe = line + len;

    if (*p != ch) { return 0; }
    ++p;

    /* Count inter-char spaces after first character */
    const char *sp1_start = p;
    while (p < pe && *p == ' ') { ++p; }
    size_t sp1 = (size_t)(p - sp1_start);

    if (p >= pe || *p != ch) { return 0; }
    ++p;

    /* Count inter-char spaces after second character */
    const char *sp2_start = p;
    while (p < pe && *p == ' ') { ++p; }
    size_t sp2 = (size_t)(p - sp2_start);

    if (sp1 != sp2) { return 0; }
    if (p >= pe || *p != ch) { return 0; }
    ++p;

    /* There must be nothing after the third character (except trailing
     * spaces that belong to the same "group" – but the original pattern has
     * no trailing-space group, so the line must end here). */
    return (p == pe);
}

/* ── Capture extraction ──────────────────────────────────────────────────── */

/* AQ_BT_ATTR_ENTRY: ":name: value"
 *   caps[0] = name (between first ':' and second ':')
 *   caps[1] = value (everything after the second ':' + WS, trimmed right) */
static AqBlockScanResult extract_attr_entry(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_ATTR_ENTRY;

    const char *p  = line;
    const char *pe = line + len;

    if (p >= pe || *p != ':') { return r; }
    ++p;  /* skip leading ':' */

    /* name starts here */
    const char *name_start = p;
    /* advance to the closing ':' */
    while (p < pe && *p != ':') { ++p; }
    const char *name_end = p;
    if (p < pe) { ++p; }  /* skip closing ':' */

    r.caps[0] = make_span(line, name_start, name_end);

    /* value: skip leading whitespace */
    p = skip_ws(p, pe);
    const char *val_start = p;
    const char *val_end   = rskip_ws(p, pe);
    if (val_end > val_start) {
        r.caps[1] = make_span(line, val_start, val_end);
    }
    return r;
}

/* AQ_BT_SECTION_TITLE: "=+ text"
 *   caps[0] = the leading '=' run
 *   caps[1] = the title text (trimmed) */
static AqBlockScanResult extract_section_title(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_SECTION_TITLE;

    const char *p  = line;
    const char *pe = line + len;

    const char *eq_start = p;
    while (p < pe && *p == '=') { ++p; }
    r.caps[0] = make_span(line, eq_start, p);

    p = skip_ws(p, pe);
    const char *txt_start = p;
    /* Strip trailing "=+" setext marker if present */
    const char *txt_end = rskip_ws(p, pe);
    /* Peel off trailing '=' run */
    const char *q = txt_end;
    while (q > txt_start && *(q - 1) == '=') { --q; }
    if (q < txt_end) {
        /* Make sure there's a space between the text and the trailing '=' */
        const char *after_text = rskip_ws(txt_start, q);
        txt_end = after_text;
    }
    r.caps[1] = make_span(line, txt_start, txt_end);
    return r;
}

/* AQ_BT_BLOCK_ATTR: "[content]"
 *   caps[0] = the content inside the brackets */
static AqBlockScanResult extract_block_attr(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_BLOCK_ATTR;

    const char *p  = line;
    const char *pe = line + len;
    if (p >= pe || *p != '[') { return r; }
    ++p;
    /* Skip second '[' for [[anchor]] */
    int double_open = (p < pe && *p == '[') ? 1 : 0;
    if (double_open) { ++p; }

    const char *inner_start = p;
    /* Find closing ']' (or ']]' for double) */
    const char *inner_end = pe;
    if (double_open) {
        /* Search backwards for ']]' */
        if (len >= 4 && pe[-1] == ']' && pe[-2] == ']') {
            inner_end = pe - 2;
        }
    } else {
        /* Search backwards for ']' */
        if (len >= 2 && pe[-1] == ']') {
            inner_end = pe - 1;
        }
    }
    r.caps[0] = make_span(line, inner_start, inner_end);
    return r;
}

/* AQ_BT_BLOCK_TITLE: ".Title"
 *   caps[0] = the title text (after the leading '.') */
static AqBlockScanResult extract_block_title(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_BLOCK_TITLE;

    const char *p  = line + 1;  /* skip leading '.' */
    const char *pe = line + len;
    r.caps[0] = make_span(line, p, pe);
    return r;
}

/* AQ_BT_BLOCK_ANCHOR: "[[id]]" or "[[id, reftext]]"
 *   caps[0] = the anchor id
 *   caps[1] = optional reftext */
static AqBlockScanResult extract_block_anchor(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_BLOCK_ANCHOR;

    const char *p  = line + 2;  /* skip "[[" */
    const char *pe = line + len;
    /* Strip the closing "]]".  The re2c DFA guarantees len >= 6 and that the
     * last two bytes before the NUL sentinel are "]]", so both checks succeed
     * in practice.  The pe > p guard prevents underflow on pathologically
     * short inputs. */
    if (len >= 2 && pe[-1] == ']') { --pe; }  /* strip outer ']' */
    if (pe > p  && pe[-1] == ']') { --pe; }   /* strip inner ']' */

    /* Split on first ',' */
    const char *comma = (const char*)memchr(p, ',', (size_t)(pe - p));
    if (comma) {
        r.caps[0] = make_span(line, p, comma);
        const char *ref_start = skip_ws(comma + 1, pe);
        const char *ref_end   = rskip_ws(ref_start, pe);
        if (ref_end > ref_start) {
            r.caps[1] = make_span(line, ref_start, ref_end);
        }
    } else {
        r.caps[0] = make_span(line, p, pe);
    }
    return r;
}

/* AQ_BT_DELIMITER: the fence string itself
 *   caps[0] = the entire delimiter string */
static AqBlockScanResult extract_delimiter(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_DELIMITER;
    r.caps[0] = make_span(line, line, line + len);
    return r;
}

/* AQ_BT_LIST_UNORD: "  - text"  or  "  *** text"
 *   caps[0] = the marker ('-' or '*+')
 *   caps[1] = the item text */
static AqBlockScanResult extract_list_unord(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_LIST_UNORD;

    const char *p  = line;
    const char *pe = line + len;

    /* Skip leading whitespace */
    p = skip_ws(p, pe);

    const char *mk_start = p;
    char lead = (p < pe) ? *p : '\0';
    if (lead == '-') {
        ++p;
    } else {
        while (p < pe && *p == '*') { ++p; }
    }
    r.caps[0] = make_span(line, mk_start, p);

    p = skip_ws(p, pe);
    r.caps[1] = make_span(line, p, pe);
    return r;
}

/* AQ_BT_LIST_ORD: "  1. text"  "  a. text"  "  i) text"
 *   caps[0] = the marker (e.g. "1.", "a.", "i)")
 *   caps[1] = the item text */
static AqBlockScanResult extract_list_ord(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_LIST_ORD;

    const char *p  = line;
    const char *pe = line + len;

    p = skip_ws(p, pe);

    const char *mk_start = p;
    /* Skip over the marker: dots, digits, letters, or roman-numeral letters */
    while (p < pe && *p != ' ' && *p != '\t') { ++p; }
    r.caps[0] = make_span(line, mk_start, p);

    p = skip_ws(p, pe);
    r.caps[1] = make_span(line, p, pe);
    return r;
}

/* AQ_BT_LIST_DESCRIPT: "term:: body"  or  ":: body"
 *   caps[0] = the term (may be empty for "::" forms)
 *   caps[1] = the separator ("::" / ":::" / "::::" / ";;")
 *   caps[2] = the body text */
static AqBlockScanResult extract_list_descript(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_LIST_DESCRIPT;

    const char *p  = line;
    const char *pe = line + len;

    /* Skip leading whitespace – description list items may be indented.
     * The PCRE2 pattern uses ^([ \t]*)([^ \t].*?)... so the captured term
     * starts at the first non-whitespace character. */
    p = skip_ws(p, pe);
    const char *sep = NULL;
    size_t sep_len  = 0;

    for (const char *q = p; q + 1 < pe; ++q) {
        if (q[0] == ';' && q[1] == ';') {
            sep = q; sep_len = 2;
            break;
        }
        if (q[0] == ':' && q[1] == ':') {
            size_t extra = 0;
            while (q + 2 + extra < pe && q[2 + extra] == ':' && extra < 2) {
                ++extra;
            }
            /* Only accept if the char AFTER the separator is WS or EOL */
            const char *after = q + 2 + extra;
            if (after >= pe || *after == ' ' || *after == '\t') {
                sep = q; sep_len = 2 + extra;
                break;
            }
        }
    }

    if (!sep) {
        /* Fallback: whole line is the term with no separator */
        r.caps[0] = make_span(line, p, pe);
        return r;
    }

    r.caps[0] = make_span(line, p, sep);
    r.caps[1] = make_span(line, sep, sep + sep_len);

    const char *body = skip_ws(sep + sep_len, pe);
    r.caps[2] = make_span(line, body, pe);
    return r;
}

/* AQ_BT_LIST_CALLOUT: "<1> text"  or  "<.> text"
 *   caps[0] = the callout number/dot
 *   caps[1] = the item text */
static AqBlockScanResult extract_list_callout(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_LIST_CALLOUT;

    const char *p  = line + 1;  /* skip '<' */
    const char *pe = line + len;

    const char *num_start = p;
    while (p < pe && *p != '>') { ++p; }
    r.caps[0] = make_span(line, num_start, p);
    if (p < pe) { ++p; }  /* skip '>' */

    p = skip_ws(p, pe);
    r.caps[1] = make_span(line, p, pe);
    return r;
}

/* AQ_BT_BLOCK_IMAGE / AQ_BT_BLOCK_MEDIA:
 *   "image::target[attrs]"  /  "video::target[attrs]"  /  "audio::..."
 *
 * For IMAGE:
 *   caps[0] = target (between "::" and '[')
 *   caps[1] = attr content (inside '[' ... ']')
 *
 * For MEDIA:
 *   caps[0] = macro type ("video" or "audio")
 *   caps[1] = target
 *   caps[2] = attr content
 */
static AqBlockScanResult extract_block_macro(const char *line, size_t len,
                                             AqBlockToken type)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = type;

    const char *p  = line;
    const char *pe = line + len;

    /* Find "::" */
    const char *sep = NULL;
    for (const char *q = p; q + 1 < pe; ++q) {
        if (q[0] == ':' && q[1] == ':') { sep = q; break; }
    }
    if (!sep) { return r; }

    if (type == AQ_BT_BLOCK_MEDIA) {
        r.caps[0] = make_span(line, p, sep);  /* "video" or "audio" */
    }

    const char *tgt_start = sep + 2;
    /* Target ends at the '[' */
    const char *bracket = NULL;
    for (const char *q = tgt_start; q < pe; ++q) {
        if (*q == '[') { bracket = q; break; }
    }
    if (!bracket) { return r; }

    const char *tgt_end = bracket;

    if (type == AQ_BT_BLOCK_IMAGE) {
        r.caps[0] = make_span(line, tgt_start, tgt_end);
        /* attr content: between '[' and ']' */
        const char *attr_start = bracket + 1;
        const char *attr_end   = (pe > attr_start && pe[-1] == ']') ? pe - 1 : pe;
        r.caps[1] = make_span(line, attr_start, attr_end);
    } else {
        r.caps[1] = make_span(line, tgt_start, tgt_end);
        const char *attr_start = bracket + 1;
        const char *attr_end   = (pe > attr_start && pe[-1] == ']') ? pe - 1 : pe;
        r.caps[2] = make_span(line, attr_start, attr_end);
    }
    return r;
}

/* AQ_BT_IFDEF / AQ_BT_IFNDEF: "ifdef::attr[]" / "ifndef::attr[]"
 *   caps[0] = the attribute name (between "::" and '[')
 *
 * AQ_BT_ENDIF: "endif::attr[]"
 *   caps[0] = the attribute name (may be empty) */
static AqBlockScanResult extract_conditional(const char *line, size_t len,
                                             AqBlockToken type)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = type;

    const char *p  = line;
    const char *pe = line + len;

    /* Skip the directive keyword ("ifdef::", "ifndef::", "endif::",
     * "ifeval::[") */
    const char *sep = NULL;
    for (const char *q = p + 1; q + 1 < pe; ++q) {
        if (q[0] == ':' && q[1] == ':') { sep = q; break; }
    }
    if (!sep) { return r; }

    const char *attr_start = sep + 2;

    if (type == AQ_BT_IFEVAL) {
        /* ifeval::[expr] – attr field is the expression */
        /* skip '[' */
        if (attr_start < pe && *attr_start == '[') { ++attr_start; }
        const char *attr_end = pe;
        if (attr_end > attr_start && *(attr_end - 1) == ']') { --attr_end; }
        r.caps[0] = make_span(line, attr_start, attr_end);
        return r;
    }

    /* Find '[' to delimit attribute name */
    const char *bracket = (const char *)memchr(attr_start, '[',
                                               (size_t)(pe - attr_start));
    const char *attr_end = bracket ? bracket : pe;
    r.caps[0] = make_span(line, attr_start, attr_end);
    return r;
}

/* AQ_BT_INCLUDE: "include::path[attrs]"
 *   caps[0] = the file path
 *   caps[1] = the attr content */
static AqBlockScanResult extract_include(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));
    r.type = AQ_BT_INCLUDE;

    /* Reuse block-macro extraction (same structure as image::) */
    AqBlockScanResult tmp = extract_block_macro(line, len, AQ_BT_BLOCK_IMAGE);
    r.type   = AQ_BT_INCLUDE;
    r.caps[0] = tmp.caps[0];  /* path */
    r.caps[1] = tmp.caps[1];  /* attrs */
    return r;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

AqBlockScanResult aq_scan_block_line(const char *line, size_t len)
{
    AqBlockScanResult r;
    memset(&r, 0, sizeof(r));

    /* 1. Thematic-break pre-check (requires backreference-style logic that
     *    cannot be expressed in a regular language / re2c DFA). */
    if (is_thematic_break(line, len)) {
        r.type = AQ_BT_THEMATIC_BREAK;
        return r;
    }

    /* 2. DFA classification (re2c-generated). */
    AqBlockToken type = aq_classify_line(line, len);

    /* 3. Capture extraction. */
    switch (type) {
        case AQ_BT_BLANK:
        case AQ_BT_COMMENT:
        case AQ_BT_BLOCK_COMMENT:
        case AQ_BT_PAGE_BREAK:
        case AQ_BT_THEMATIC_BREAK:
            r.type = type;
            return r;

        case AQ_BT_ATTR_ENTRY:
            return extract_attr_entry(line, len);

        case AQ_BT_SECTION_TITLE:
            return extract_section_title(line, len);

        case AQ_BT_BLOCK_ATTR:
            return extract_block_attr(line, len);

        case AQ_BT_BLOCK_ANCHOR:
            return extract_block_anchor(line, len);

        case AQ_BT_BLOCK_TITLE:
            return extract_block_title(line, len);

        case AQ_BT_DELIMITER:
            return extract_delimiter(line, len);

        case AQ_BT_LIST_UNORD:
            return extract_list_unord(line, len);

        case AQ_BT_LIST_ORD:
            return extract_list_ord(line, len);

        case AQ_BT_LIST_DESCRIPT:
            return extract_list_descript(line, len);

        case AQ_BT_LIST_CALLOUT:
            return extract_list_callout(line, len);

        case AQ_BT_BLOCK_IMAGE:
            return extract_block_macro(line, len, AQ_BT_BLOCK_IMAGE);

        case AQ_BT_BLOCK_MEDIA:
            return extract_block_macro(line, len, AQ_BT_BLOCK_MEDIA);

        case AQ_BT_IFDEF:
        case AQ_BT_IFNDEF:
        case AQ_BT_IFEVAL:
        case AQ_BT_ENDIF:
            return extract_conditional(line, len, type);

        case AQ_BT_INCLUDE:
            return extract_include(line, len);

        default:
            r.type = type;
            return r;
    }
}
