/* config.h – Minimal compile-time configuration for the embedded PCRE2 subset.
 *
 * Generated for the asciiquack vendor build (PCRE2 10.42, 8-bit mode only).
 * No JIT, no Unicode properties (\p{}), no DFA, no POSIX API, no conversion.
 *
 * This file is read by every PCRE2 .c source file before pcre2_internal.h.
 */

#ifndef PCRE2_EMBED_CONFIG_H
#define PCRE2_EMBED_CONFIG_H

/* ── 8-bit code units only ───────────────────────────────────────────── */
#define SUPPORT_PCRE2_8 1

/* ── No JIT (platform-specific machine code generation) ─────────────── */
/* #undef SUPPORT_JIT */

/* ── No Unicode property support (\p{}, \P{}, \X, \R script-runs) ───── */
/* Without this, \w/\d/\s still work via the character tables. */
/* #undef SUPPORT_UNICODE */

/* ── Standard C library functions available ─────────────────────────── */
#define HAVE_BCOPY 1
#define HAVE_MEMMOVE 1
#define HAVE_STRERROR 1

/* ── Internal limits (PCRE2 defaults) ───────────────────────────────── */
#define LINK_SIZE            2          /* size of internal link in compiled code */
#define HEAP_LIMIT           20000000   /* bytes of heap used by matching */
#define MATCH_LIMIT          10000000   /* recursion/backtrack limit */
#define MATCH_LIMIT_DEPTH    MATCH_LIMIT
#define NEWLINE_DEFAULT      2          /* 1=CR, 2=LF, 3=CRLF, 4=any, 5=anycrlf */
#define PARENS_NEST_LIMIT    250
#define MAX_NAME_SIZE        32
#define MAX_NAME_COUNT       10000

#endif /* PCRE2_EMBED_CONFIG_H */
