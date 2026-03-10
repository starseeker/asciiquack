/* attr_list_impl.h – Internal types shared between the lexer (attr_list.c)
 * and the lemon-generated parser (attr_list_gen.c).
 * Not part of the public API.
 */

#pragma once

#include "attr_list.h"
#include <stddef.h>
#include <stdio.h>   /* snprintf */

/* Token value type: a (pointer, length) span into the original input.
 * Must match %token_type in attr_list.lemon. */
typedef struct { const char *s; int len; } AqAttrSpan;

/* Parser context threaded through every grammar action via %extra_argument. */
struct AqAttrCtx {
    int             pos;      /* 1-based positional index */
    AqAttrCallback  cb;
    void           *userdata;
    int             error;    /* set to 1 on parse error */
};

/* Token type constants – must match the terminal numbering in attr_list.lemon.
 * Lemon assigns numbers starting from 1 in declaration order.
 * The order here matches the %token_prefix declarations. */
#define AQ_TOK_COMMA   1
#define AQ_TOK_EQ      2
#define AQ_TOK_WORD    3
#define AQ_TOK_QUOTED  4

/* ── Lemon-generated parser API ─────────────────────────────────────────── */
void *AqAttrAlloc(void *(*mallocProc)(size_t));
void  AqAttrFree(void *p, void (*freeProc)(void *));
void  AqAttr(void *pParser, int token, AqAttrSpan val,
             struct AqAttrCtx *ctx);
