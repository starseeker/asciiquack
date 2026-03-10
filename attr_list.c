/// @file attr_list.c
/// @brief AsciiDoc attribute-list parser: hand-written lexer that feeds the
///        lemon-generated LALR(1) parser (attr_list_gen.c).
///
/// Public function:
///   int aq_parse_attr_list(const char *content, size_t len,
///                          AqAttrCallback cb, void *userdata);
///
/// Build: compile this file AND attr_list_gen.c into the same link unit.
/// They share types via attr_list_impl.h (internal header).

#include "attr_list_impl.h"
#include <stdlib.h>

/* ── Hand-written lexer ──────────────────────────────────────────────────── */
/*
 * Tokenises the content inside a block-attribute line's square brackets,
 * e.g. "source,java" (brackets already stripped), "id=myid,title=Foo Bar".
 *
 * Token classes:
 *   COMMA:  ','
 *   EQ:     '='
 *   QUOTED: a double-quoted string, including the surrounding '"' characters
 *   WORD:   a run of non-special bytes
 *
 * Two lexer modes:
 *   key mode   (in_value == 0): WORD ends at ',', '=', '"', or whitespace.
 *   value mode (in_value == 1): WORD ends at ',' or '"' only.
 *              This matches the existing parse_attribute_list() behaviour
 *              where unquoted values such as "title=The Title" are valid.
 *
 * Whitespace is silently consumed between tokens (but included in WORD tokens
 * when in value mode, since leading/trailing spaces are trimmed below).
 */
typedef struct {
    AqAttrSpan tok;  /* .s and .len of the current token */
    int        type; /* AQ_TOK_* or 0 = end of input     */
} AqAttrLex;

static AqAttrLex aq_attr_lex(const char **pp, const char *pe, int in_value)
{
    AqAttrLex r;
    r.tok.s   = NULL;
    r.tok.len = 0;
    r.type    = 0;

    /* Skip leading whitespace */
    while (*pp < pe && (**pp == ' ' || **pp == '\t')) { ++(*pp); }
    if (*pp >= pe) { return r; }

    const char *p = *pp;

    if (*p == ',') {
        r.type    = AQ_TOK_COMMA;
        r.tok.s   = p;
        r.tok.len = 1;
        *pp = p + 1;
        return r;
    }
    if (*p == '=') {
        r.type    = AQ_TOK_EQ;
        r.tok.s   = p;
        r.tok.len = 1;
        *pp = p + 1;
        return r;
    }
    if (*p == '"') {
        /* Quoted string: scan to the closing '"', respecting '\"' escapes. */
        const char *start = p;
        ++p;
        while (p < pe && *p != '"') {
            if (*p == '\\' && p + 1 < pe) { ++p; }  /* skip escaped char */
            ++p;
        }
        if (p < pe) { ++p; }  /* consume closing '"' */
        r.type    = AQ_TOK_QUOTED;
        r.tok.s   = start;
        r.tok.len = (int)(p - start);
        *pp = p;
        return r;
    }

    if (in_value) {
        /* Value mode: consume everything up to the next ',' or '"'.
         * Trailing whitespace is trimmed so "title=The Title , next" works. */
        const char *start = p;
        while (p < pe && *p != ',' && *p != '"') { ++p; }
        /* Trim trailing whitespace */
        const char *end = p;
        while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t')) {
            --end;
        }
        r.type    = AQ_TOK_WORD;
        r.tok.s   = start;
        r.tok.len = (int)(end - start);
        *pp = p;
    } else {
        /* Key mode: word ends at ',', '=', or '"'.
         * Spaces are intentionally NOT treated as delimiters here —
         * unquoted positional values may contain spaces, e.g. [A photo] or
         * [quote, Mike Muuss].  This matches the PCRE2 behaviour where
         * commas are the only separator.
         * Trailing whitespace is trimmed to match "key = value" handling. */
        const char *start = p;
        while (p < pe && *p != ',' && *p != '=' && *p != '"') { ++p; }
        /* Trim trailing whitespace */
        const char *end = p;
        while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t')) {
            --end;
        }
        r.type    = AQ_TOK_WORD;
        r.tok.s   = start;
        r.tok.len = (int)(end - start);
        *pp = p;
    }
    return r;
}

/* ── Public function ─────────────────────────────────────────────────────── */

int aq_parse_attr_list(const char    *content,
                       size_t         len,
                       AqAttrCallback cb,
                       void          *userdata)
{
    if (!content || !cb) { return -1; }

    struct AqAttrCtx ctx;
    ctx.pos      = 1;
    ctx.cb       = cb;
    ctx.userdata = userdata;
    ctx.error    = 0;

    void *pParser = AqAttrAlloc(malloc);
    if (!pParser) { return -1; }

    const char *p   = content;
    const char *pe  = content + len;
    int in_value    = 0;   /* lexer mode: 0 = key, 1 = value */

    for (;;) {
        AqAttrLex lr = aq_attr_lex(&p, pe, in_value);
        if (lr.type == 0) { break; }

        /* Track whether the next token should be read as a value. */
        if (lr.type == AQ_TOK_EQ) {
            in_value = 1;
        } else if (lr.type == AQ_TOK_COMMA) {
            in_value = 0;
        } else {
            in_value = 0;  /* after reading a value, go back to key mode */
        }

        /* Skip empty WORD tokens (can arise from leading/trailing spaces). */
        if (lr.type == AQ_TOK_WORD && lr.tok.len == 0) { continue; }

        AqAttr(pParser, lr.type, lr.tok, &ctx);
        if (ctx.error) { break; }
    }

    /* Signal end of input with token 0 (lemon convention). */
    if (!ctx.error) {
        AqAttrSpan empty;
        empty.s   = NULL;
        empty.len = 0;
        AqAttr(pParser, 0, empty, &ctx);
    }

    AqAttrFree(pParser, free);
    return ctx.error ? -1 : 0;
}
