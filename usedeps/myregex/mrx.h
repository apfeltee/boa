
#pragma once

/************

    REMIMU: SINGLE HEADER C/C++ REGEX LIBRARY

    Compatible with C99 and C++11 and later standards. Uses backtracking and relatively standard regex syntax.

    #include "myregex.h"

FUNCTIONS

    //  Returns 0 on success, or -1 on invalid or unsupported regex, or -2 on not enough tokens given to parse regex.
    MRX_INLINE int mrx_regex_parse(
        MRXContext* ctx,
        //  Regex pattern to parse. Must be null-terminated.
        const char * pattern,
        //  Output buffer of tokencount regex tokens
        MRXToken * tokens,
        //  Maximum allowed number of tokens to write
        int16_t * tokencount,
        // Optional bitflags.
        int32_t flags

    )

    // Returns match length, or -1 on no match, or -2 on out of memory, or -3 ifthe regex is invalid.
    MRX_INLINE int64_t mrx_regex_match(
        MRXContext* ctx,
        // Parsed regex to match against textstr.
        const MRXToken * tokens,
        // Text to match against tokens.
        const char * textstr,
        // index value to match at.
        size_t starti,
        // Number of allowed capture info output slots.
        uint16_t capslots,
        // Capture position info output buffer.
        int64_t* cappos,
        // Capture length info output buffer.
        int64_t* capspan
    )

    MRX_INLINE void mrx_regex_printtokens(
        // Regex tokens to spew to stdout, fordebugging.
        MRXToken* tokens
    )

PERFORMANCE

    On simple cases, Remimu's match speed is similar to PCRE2. Regex parsing/compilation is also much faster (around 4x to 10x), so single-shot regexes are often faster than PCRE2.

    HOWEVER: Remimu is a pure backtracking engine, and has `O(2^x)` complexity on regexes with catastrophic backtracking. It can be much, much, MUCH slower than PCRE2. Beware!

    Remimu uses length-checked fixed memory buffers with no recursion, so memory usage is statically known.

FEATURES

    - Lowest-common-denominator common regex syntax
    - Based on backtracking (slow in the worst case, but fast in the best case)
    - 8-bit only, no utf-16 or utf-32
    - Statically known memory usage (no heap allocation or recursion)
    - Groups with or without capture, and with or without quantifiers
    - Supported escapes:
    - - 2-digit hex: e.g. \x00, \xFF, or lowercase, or mixed case
    - - \r, \n, \t, \v, \f (whitespace characters)
    - - \d, \s, \w, \D, \S, \W (digit, space, and word character classes)
    - - \b, \B word boundary and non-word-boundary anchors (not fully supported in zero-size quantified groups, but even then, usually supported)
    - - Escaped literal characters: {}[]-()|^$*+?:./\
    - - - Escapes work in character classes, except for'b'
    - Character classes, including disjoint ranges, proper handling of bare [ and trailing -, etc
    - - Dot (.) matches all characters, including newlines, unless MRX_FLAG_DOTNONEWLINES is passed as a flag to mrx_regex_parse
    - - Dot (.) only matches at most one byte at a time, so matching \r\n requires two dots (and not using MRX_FLAG_DOTNONEWLINES)
    - Anchors (^ and $)
    - - Same support caveats as \b, \B apply
    - Basic quantifiers (*, +, ?)
    - - Quantifiers are greedy by default.
    - Explicit quantifiers ({2}, {5}, {5,}, {5,7})
    - Alternation e.g. (asdf|foo)
    - Lazy quantifiers e.g. (asdf)*? or \w+?
    - Possessive greedy quantifiers e.g. (asdf)*+ or \w++
    - - NOTE: Capture groups forand inside of possessive groups return no capture information.
    - Atomic groups e.g. (?>(asdf))
    - - NOTE: Capture groups inside of atomic groups return no capture information.

NOT SUPPORTED

    - Strings with non-terminal null characters
    - Unicode character classes (matching single utf-8 characters works regardless)
    - Exact POSIX regex semantics (posix-style greediness etc)
    - Backreferences
    - Lookbehind/Lookahead
    - Named groups
    - Most other weird flavor-specific regex stuff
    - Capture of or inside of possessive-quantified groups (still take up a capture slot, but no data is returned)

USAGE

    // minimal:

    MRXContext ctx;
    MRXToken tokens[1024];
    int16_t tokencount = 1024;
    mrx_context_init(&ctx);
    int e = mrx_regex_parse(&ctx, "[0-9]+\\.[0-9]+", tokens, &tokencount, 0);
    assert(!e);

    int64_t matchlen = mrx_regex_match(&ctx, tokens, "23.53) ", 0, 0, 0, 0);
    printf("########### return: %d\n", matchlen);

    // with captures:
    MRXContext ctx;
    MRXToken tokens[256];
    int16_t tokencount = sizeof(tokens)/sizeof(tokens[0]);
    mrx_context_init(&xtx);
    int e = mrx_regex_parse(&ctx, "((a)|(b))++", tokens, &tokencount, 0);
    assert(!e);

    int64_t cappos[5];
    int64_t capspan[5];
    memset(cappos, 0xFF, sizeof(cappos));
    memset(capspan, 0xFF, sizeof(capspan));

    int64_t matchlen = mrx_regex_match(&ctx, tokens, "aaaaaabbbabaqa", 0, 5, cappos, capspan);
    printf("Match length: %d\n", matchlen);
    for(int i = 0; i < 5; i++)
        printf("Capture %d: %d plus %d\n", i, cappos[i], capspan[i]);
    mrx_regex_printtokens(tokens);

LICENSE

    Creative Commons Zero, public domain.

*/

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#if defined(__STRICT_ANSI__)
    #define MRX_INLINE static
#else
    #define MRX_INLINE static inline
#endif

#define MRX_VERBOSE 0

enum
{
    MRX_FLAG_DOTNONEWLINES = 1
};

enum
{
    MRX_KIND_NORMAL = 0,
    MRX_KIND_OPEN = 1,
    MRX_KIND_NCOPEN = 2,
    MRX_KIND_CLOSE = 3,
    MRX_KIND_OR = 4,
    MRX_KIND_CARET = 5,
    MRX_KIND_DOLLAR = 6,
    MRX_KIND_BOUND = 7,
    MRX_KIND_NBOUND = 8,
    MRX_KIND_END = 9
};

enum
{
    MRX_MODE_POSSESSIVE = 1,
    MRX_MODE_LAZY = 2,
    /*  temporary; gets cleared later */
    MRX_MODE_INVERTED = 128
};

enum
{
    /*
    0: init
    1: normal
    2: in char class, initial state
    3: in char class, but possibly looking fora range marker
    4: in char class, but just saw a range marker
    5: immediately after quantifiable token
    6: immediately after quantifier
    */
    MRX_STATE_NORMAL = 1,
    MRX_STATE_QUANT = 2,
    MRX_STATE_MODE = 3,
    MRX_STATE_CCINIT = 4,
    MRX_STATE_CCNORMAL = 5,
    MRX_STATE_CCRANGE = 6
};

typedef struct MRXMatchState MRXMatchState;
typedef struct MRXContext MRXContext;
typedef struct MRXToken MRXToken;

struct MRXMatchState
{
    uint32_t k;
    uint32_t groupstate; /*  quantified group temp state (e.g. number of repetitions) */
    uint32_t prev; /*  for)s, stack index of corresponding previous quantified state */
    uint64_t i;
    uint64_t rangemin;
    uint64_t rangemax;
};

struct MRXToken
{
    uint8_t kind;
    uint8_t mode;
    uint16_t countlow;
    /*  0 means no limit */
    uint16_t counthigh;
    /*  forgroups: mask 0 stores group-with-quantifier number (quantifiers are +, *, ?, {n}, {n,}, or {n,m}) */
    uint16_t mask[16];
    /*  from ( or ), offset in token list to matching paren. TODO: move into mask maybe */
    int16_t pairoffset;
};

struct MRXContext
{
    bool haderror;
    bool isallocated;
    size_t maxtokens;
    size_t tokencount;
    char errorbuf[1024];
    MRXToken tokens[256];
};

MRX_INLINE int mrx_regex_parse(MRXContext* ctx, const char* pattern, int32_t flags);
MRX_INLINE int64_t mrx_regex_match(MRXContext* ctx, const char* textstr, size_t textlen, size_t starti, uint16_t capslots, int64_t* cappos, int64_t* capspan);
MRX_INLINE void mrx_regex_printtokens(MRXToken* tokens);

MRX_INLINE void mrx_guts_doinvert(int* macn, MRXToken* token)
{
    for(*macn = 0; (*macn) < 16; (*macn)++)
    {
        token->mask[*macn] = ~token->mask[*macn];
    }
    token->mode &= ~MRX_MODE_INVERTED;
}

MRX_INLINE void mrx_guts_cleartoken(MRXToken* token)
{
    memset(token, 0, sizeof(MRXToken));
    token->countlow = 1;
    token->counthigh = 2;
}

MRX_INLINE bool mrx_guts_pushtoken(MRXContext* ctx, MRXToken* token, int64_t tokenslen, int16_t* k, int* macn)
{
    if((*k) == 0 || ctx->tokens[(*k) - 1].kind != token->kind || (token->kind != MRX_KIND_BOUND && token->kind != MRX_KIND_NBOUND))
    {
        if(token->mode & MRX_MODE_INVERTED)
        {
            mrx_guts_doinvert(macn, token);
        }
        if((*k) >= tokenslen)
        {
            puts("buffer overflow");
            return false;
        }
        ctx->tokens[(*k)++] = *token;
        mrx_guts_cleartoken(token);
    }
    return true;
}

MRX_INLINE void mrx_guts_setmasktoken(MRXToken* token, int bc)
{
    token->mask[((uint8_t)(bc)) >> 4] |= 1 << ((uint8_t)(bc) & 0xF);
}

MRX_INLINE void mrx_guts_setmaskall(MRXToken* token, int* macn)
{
    for((*macn) = 0; (*macn) < 16; (*macn)++)
    {
        token->mask[(*macn)] = 0xFFFF;
    }
}

MRX_INLINE void mrx_context_initctx(MRXContext* ctx, bool onstack)
{
    ctx->isallocated = (onstack ? false : true);
    ctx->haderror = false;
    ctx->maxtokens = sizeof(ctx->tokens) / sizeof(ctx->tokens[0]);
    memset(ctx->tokens, 0xFF, sizeof(ctx->tokens));
}

#if 0
MRX_INLINE MRXContext* mrx_context_init(MRXToken* tokens, size_t maxtokens)
{
    MRXContext* ctx;
    ctx = (MRXContext*)boa_sysmem_malloc(sizeof(MRXContext));
    if(ctx == NULL)
    {
        return NULL;
    }
    mrx_context_initctx(ctx, tokens, maxtokens, false);
    return ctx;
}
#endif

void mrx_context_destroy(MRXContext* ctx)
{
    if(!ctx->isallocated)
    {
        return;
    }
#if 0
    boa_sysmem_free(ctx);
#endif
}

void mrx_context_seterror(MRXContext* ctx, const char* fmt, ...)
{
    va_list va;
    ctx->haderror = true;
    fprintf(stderr, "ERROR: ");
    va_start(va, fmt);
    vsprintf(ctx->errorbuf, fmt, va);
    va_end(va);
}

MRX_INLINE int mrx_util_isquantchar(int c)
{
    return (c == '{' || c == '}' || c == '[' || c == ']' || c == '-' || c == '(' || c == ')' || c == '|' || c == '^' || c == '$' || c == '*' || c == '+' || c == '?' || c == ':' || c == '.' || c == '/' || c == '\\');
}

/*
 Returns a negative number on failure:
 -1: Regex string is invalid or using unsupported features or too long.
 -2: Provided buffer not long enough. Give up, or reallocate with more length and retry.
  Returns 0 on success.
  On call, tokencount pointer must point to the number of tokens that can be written to the tokens buffer.
  On successful return, the number of actually used tokens is written to tokencount.
  Sets tokencount to zero ifa regex is not created but no error happened (e.g. empty pattern).
  Flags: Not yet used.
  SAFETY: Pattern must be null-terminated.
  SAFETY: tokens buffer must have at least the input tokencount number of MRXToken objects. They are allowed to be uninitialized.
*/
MRX_INLINE int mrx_regex_parse(MRXContext* ctx, const char* pattern, int32_t flags)
{
    int escstate;
    int state;
    int charclassmem;
    int parencount;
    int16_t k;
    int64_t tokenslen;
    uint64_t i;
    uint64_t mi;
    uint64_t patternlen;
    uint8_t clsi;
    char c;
    ptrdiff_t l;
    int macn;
    uint32_t val;
    uint32_t val2;
    uint8_t escc;
    uint8_t n0;
    uint8_t n1;
    uint8_t isupper;
    uint64_t n;
    int16_t k3;
    int16_t k2;
    ptrdiff_t diff;
    int balance;
    ptrdiff_t found;
    uint16_t m[16];
    MRXToken token;
    tokenslen = ctx->maxtokens;
    patternlen = strlen(pattern);
    if(ctx->maxtokens == 0)
    {
        return -2;
    }
    /*
    0: normal
    1: just saw a backslash
    */
    escstate = 0;
    state = MRX_STATE_NORMAL;
    charclassmem = -1;
    mrx_guts_cleartoken(&token);
    k = 0;
    /*
    start with an invisible group specifier
    (this allows the matcher to not need to have a special root-level alternation operator case)
    */
    token.kind = MRX_KIND_OPEN;
    token.countlow = 0;
    token.counthigh = 0;
    parencount = 0;
    for(i = 0; i < patternlen; i++)
    {
        c = pattern[i];
        if(state == MRX_STATE_QUANT)
        {
            state = MRX_STATE_MODE;
            if(c == '?')
            {
                /* first non-allowed amount */
                token.countlow = 0;
                token.counthigh = 2;
                continue;
            }
            else if(c == '+')
            {
                /* unlimited */
                token.countlow = 1;
                token.counthigh = 0;
                continue;
            }
            else if(c == '*')
            {
                /* unlimited */
                token.countlow = 0;
                token.counthigh = 0;
                continue;
            }
            else if(c == '{')
            {
                if(pattern[i + 1] == 0 || pattern[i + 1] < '0' || pattern[i + 1] > '9')
                {
                    state = MRX_STATE_NORMAL;
                }
                else
                {
                    i += 1;
                    val = 0;
                    while(pattern[i] >= '0' && pattern[i] <= '9')
                    {
                        val *= 10;
                        val += (uint32_t)(pattern[i] - '0');
                        if(val > 0xFFFF)
                        {
                            /*  unsupported length */
                            mrx_context_seterror(ctx, "quantifier range too long");
                            return -1;
                        }
                        i += 1;
                    }
                    token.countlow = val;
                    token.counthigh = val + 1;
                    if(pattern[i] == ',')
                    {
                        token.counthigh = 0; /*  unlimited */
                        i += 1;

                        if(pattern[i] >= '0' && pattern[i] <= '9')
                        {
                            val2 = 0;
                            while(pattern[i] >= '0' && pattern[i] <= '9')
                            {
                                val2 *= 10;
                                val2 += (uint32_t)(pattern[i] - '0');
                                if(val2 > 0xFFFF)
                                {
                                    /*  unsupported length */
                                    mrx_context_seterror(ctx, "quantifier range too long");
                                    return -1;
                                }
                                i += 1;
                            }
                            if(val2 < val)
                            {
                                mrx_context_seterror(ctx, "quantifier range is backwards");
                                return -1; /*  unsupported length */
                            }
                            token.counthigh = val2 + 1;
                        }
                    }
                    if(pattern[i] == '}')
                    {
                        /*  quantifier range parsed successfully */
                        continue;
                    }
                    else
                    {
                        mrx_context_seterror(ctx, "quantifier range syntax broken (no terminator)");
                        return -1;
                    }
                }
            }
        }
        if(state == MRX_STATE_MODE)
        {
            state = MRX_STATE_NORMAL;
            if(c == '?')
            {
                token.mode |= MRX_MODE_LAZY;
                continue;
            }
            else if(c == '+')
            {
                token.mode |= MRX_MODE_POSSESSIVE;
                continue;
            }
        }
        if(state == MRX_STATE_NORMAL)
        {
            if(escstate == 1)
            {
                escstate = 0;
                if(c == 'n')
                {
                    mrx_guts_setmasktoken(&token, '\n');
                }
                else if(c == 'r')
                {
                    mrx_guts_setmasktoken(&token, '\r');
                }
                else if(c == 't')
                {
                    mrx_guts_setmasktoken(&token, '\t');
                }
                else if(c == 'v')
                {
                    mrx_guts_setmasktoken(&token, '\v');
                }
                else if(c == 'f')
                {
                    mrx_guts_setmasktoken(&token, '\f');
                }
                else if(c == 'x')
                {
                    if(pattern[i + 1] == 0 || pattern[i + 2] == 0)
                    {
                        return -1; /*  too-short hex pattern */
                    }
                    n0 = pattern[i + 1];
                    n1 = pattern[i + 1];
                    if(n0 < '0' || n0 > 'f' || n1 < '0' || n1 > 'f' || (n0 > '9' && n0 < 'A') || (n1 > '9' && n1 < 'A'))
                    {
                        mrx_context_seterror(ctx, "invalid hex digit");
                        return -1; /*  invalid hex digit */
                    }
                    if(n0 > 'F')
                    {
                        n0 -= 0x20;
                    }
                    if(n1 > 'F')
                    {
                        n1 -= 0x20;
                    }
                    if(n0 >= 'A')
                    {
                        n0 -= 'A' - 10;
                    }
                    if(n1 >= 'A')
                    {
                        n1 -= 'A' - 10;
                    }
                    n0 -= '0';
                    n1 -= '0';
                    mrx_guts_setmasktoken(&token, (n1 << 4) | n0);
                    i += 2;
                }
                else if(mrx_util_isquantchar(c))
                {
                    mrx_guts_setmasktoken(&token, c);
                    state = MRX_STATE_QUANT;
                }
                else if(c == 'd' || c == 's' || c == 'w' || c == 'D' || c == 'S' || c == 'W')
                {
                    isupper = c <= 'Z';
                    memset(m, 0, sizeof(m));
                    if(isupper)
                    {
                        c += 0x20;
                    }
                    if(c == 'd' || c == 'w')
                    {
                        m[3] |= 0x03FF; /*  0~7 */
                    }
                    if(c == 's')
                    {
                        m[0] |= 0x3E00; /*  \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.) */
                        m[2] |= 1; /*  ' ' */
                    }
                    if(c == 'w')
                    {
                        m[4] |= 0xFFFE; /*  A-O */
                        m[5] |= 0x87FF; /*  P-Z_ */
                        m[6] |= 0xFFFE; /*  a-o */
                        m[7] |= 0x07FF; /*  p-z */
                    }
                    for(mi = 0; mi < 16; mi++)
                    {
                        token.mask[mi] |= isupper ? ~m[mi] : m[mi];
                    }
                    token.kind = MRX_KIND_NORMAL;
                    state = MRX_STATE_QUANT;
                }
                else if(c == 'b')
                {
                    token.kind = MRX_KIND_BOUND;
                    state = MRX_STATE_NORMAL;
                }
                else if(c == 'B')
                {
                    token.kind = MRX_KIND_NBOUND;
                    state = MRX_STATE_NORMAL;
                }
                else
                {
                    mrx_context_seterror(ctx, "unsupported escape sequence");
                    return -1; /*  unknown/unsupported escape sequence */
                }
            }
            else
            {
                if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
                {
                    return -2;
                }
                if(c == '\\')
                {
                    escstate = 1;
                }
                else if(c == '[')
                {
                    state = MRX_STATE_CCINIT;
                    charclassmem = -1;
                    token.kind = MRX_KIND_NORMAL;
                    if(pattern[i + 1] == '^')
                    {
                        token.mode |= MRX_MODE_INVERTED;
                        i += 1;
                    }
                }
                else if(c == '(')
                {
                    parencount += 1;
                    state = MRX_STATE_NORMAL;
                    token.kind = MRX_KIND_OPEN;
                    token.countlow = 0;
                    token.counthigh = 1;
                    if(pattern[i + 1] == '?' && pattern[i + 2] == ':')
                    {
                        token.kind = MRX_KIND_NCOPEN;
                        i += 2;
                    }
                    else if(pattern[i + 1] == '?' && pattern[i + 2] == '>')
                    {
                        token.kind = MRX_KIND_NCOPEN;
                        if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
                        {
                            return -2;
                        }
                        state = MRX_STATE_NORMAL;
                        token.kind = MRX_KIND_NCOPEN;
                        token.mode = MRX_MODE_POSSESSIVE;
                        token.countlow = 1;
                        token.counthigh = 2;
                        i += 2;
                    }
                }
                else if(c == ')')
                {
                    parencount -= 1;
                    if(parencount < 0 || k == 0)
                    {
                        mrx_context_seterror(ctx, "unbalanced parentheses");
                        return -1; /*  unbalanced parens */
                    }
                    token.kind = MRX_KIND_CLOSE;
                    state = MRX_STATE_QUANT;
                    balance = 0;
                    found = -1;
                    for(l = k - 1; l >= 0; l--)
                    {
                        if(ctx->tokens[l].kind == MRX_KIND_NCOPEN || ctx->tokens[l].kind == MRX_KIND_OPEN)
                        {
                            if(balance == 0)
                            {
                                found = l;
                                break;
                            }
                            else
                            {
                                balance -= 1;
                            }
                        }
                        else if(ctx->tokens[l].kind == MRX_KIND_CLOSE)
                        {
                            balance += 1;
                        }
                    }
                    if(found == -1)
                    {
                        mrx_context_seterror(ctx, "unbalanced parentheses");
                        return -1; /*  unbalanced parens */
                    }
                    diff = k - found;
                    if(diff > 32767)
                    {
                        mrx_context_seterror(ctx, "difference too large");
                        return -1; /*  too long */
                    }
                    token.pairoffset = -diff;
                    ctx->tokens[found].pairoffset = diff;
                    /*  phantom group foratomic group emulation */
                    if(ctx->tokens[found].mode == MRX_MODE_POSSESSIVE)
                    {
                        if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
                        {
                            return -2;
                        }
                        token.kind = MRX_KIND_CLOSE;
                        token.mode = MRX_MODE_POSSESSIVE;
                        token.pairoffset = -diff - 2;
                        ctx->tokens[found - 1].pairoffset = diff + 2;
                    }
                }
                else if(c == '?' || c == '+' || c == '*' || c == '{')
                {
                    mrx_context_seterror(ctx, "quantifier in non-quantifier context");
                    return -1; /*  quantifier in non-quantifier context */
                }
                else if(c == '.')
                {
                    /* puts("setting ALL of mask..."); */
                    mrx_guts_setmaskall(&token, &macn);
                    if(flags & MRX_FLAG_DOTNONEWLINES)
                    {
                        token.mask[1] ^= 0x04; /*  \n */
                        token.mask[1] ^= 0x20; /*  \r */
                    }
                    state = MRX_STATE_QUANT;
                }
                else if(c == '^')
                {
                    token.kind = MRX_KIND_CARET;
                    state = MRX_STATE_NORMAL;
                }
                else if(c == '$')
                {
                    token.kind = MRX_KIND_DOLLAR;
                    state = MRX_STATE_NORMAL;
                }
                else if(c == '|')
                {
                    token.kind = MRX_KIND_OR;
                    state = MRX_STATE_NORMAL;
                }
                else
                {
                    mrx_guts_setmasktoken(&token, c);
                    state = MRX_STATE_QUANT;
                }
            }
        }
        else if(state == MRX_STATE_CCINIT || state == MRX_STATE_CCNORMAL || state == MRX_STATE_CCRANGE)
        {
            if(c == '\\' && escstate == 0)
            {
                escstate = 1;
                continue;
            }
            escc = 0;
            if(escstate == 1)
            {
                escstate = 0;
                if(c == 'n')
                {
                    escc = '\n';
                }
                else if(c == 'r')
                {
                    escc = '\r';
                }
                else if(c == 't')
                {
                    escc = '\t';
                }
                else if(c == 'v')
                {
                    escc = '\v';
                }
                else if(c == 'f')
                {
                    escc = '\f';
                }
                else if(c == 'x')
                {
                    if(pattern[i + 1] == 0 || pattern[i + 2] == 0)
                    {
                        mrx_context_seterror(ctx, "hex pattern too short");
                        return -1; /*  too-short hex pattern */
                    }
                    n0 = pattern[i + 1];
                    n1 = pattern[i + 1];
                    if(n0 < '0' || n0 > 'f' || n1 < '0' || n1 > 'f' || (n0 > '9' && n0 < 'A') || (n1 > '9' && n1 < 'A'))
                    {
                        mrx_context_seterror(ctx, "invalid hex digit");
                        return -1; /*  invalid hex digit */
                    }
                    if(n0 > 'F')
                    {
                        n0 -= 0x20;
                    }
                    if(n1 > 'F')
                    {
                        n1 -= 0x20;
                    }
                    if(n0 >= 'A')
                    {
                        n0 -= 'A' - 10;
                    }
                    if(n1 >= 'A')
                    {
                        n1 -= 'A' - 10;
                    }
                    n0 -= '0';
                    n1 -= '0';
                    escc = (n1 << 4) | n0;
                    i += 2;
                }
                else if(c == '{' || c == '}' || c == '[' || c == ']' || c == '-' || c == '(' || c == ')' || c == '|' || c == '^' || c == '$' || c == '*' || c == '+' || c == '?' || c == ':' || c == '.' || c == '/' || c == '\\')
                {
                    escc = c;
                }
                else if(c == 'd' || c == 's' || c == 'w' || c == 'D' || c == 'S' || c == 'W')
                {
                    if(state == MRX_STATE_CCRANGE)
                    {
                        mrx_context_seterror(ctx, "tried to use a shorthand as part of a range");
                        return -1; /*  range shorthands can't be part of a range */
                    }
                    isupper = c <= 'Z';
                    memset(m, 0, sizeof(m));
                    if(isupper)
                    {
                        c += 0x20;
                    }
                    if(c == 'd' || c == 'w')
                    {
                        m[3] |= 0x03FF; /*  0~7 */
                    }
                    if(c == 's')
                    {
                        m[0] |= 0x3E00; /*  \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.) */
                        m[2] |= 1; /*  ' ' */
                    }
                    if(c == 'w')
                    {
                        m[4] |= 0xFFFE; /*  A-O */
                        m[5] |= 0x87FF; /*  P-Z_ */
                        m[6] |= 0xFFFE; /*  a-o */
                        m[7] |= 0x07FF; /*  p-z */
                    }
                    for(mi = 0; mi < 16; mi++)
                    {
                        token.mask[mi] |= isupper ? ~m[mi] : m[mi];
                    }
                    charclassmem = -1; /*  range shorthands can't be part of a range */
                    continue;
                }
                else
                {
                    printf("unknown/unsupported escape sequence in character class (\\%c)\n", c);
                    return -1; /*  unknown/unsupported escape sequence */
                }
            }
            if(state == MRX_STATE_CCINIT)
            {
                charclassmem = c;
                mrx_guts_setmasktoken(&token, c);
                state = MRX_STATE_CCNORMAL;
            }
            else if(state == MRX_STATE_CCNORMAL)
            {
                if(c == ']' && escc == 0)
                {
                    charclassmem = -1;
                    state = MRX_STATE_QUANT;
                    continue;
                }
                else if(c == '-' && escc == 0 && charclassmem >= 0)
                {
                    state = MRX_STATE_CCRANGE;
                    continue;
                }
                else
                {
                    charclassmem = c;
                    mrx_guts_setmasktoken(&token, c);
                    state = MRX_STATE_CCNORMAL;
                }
            }
            else if(state == MRX_STATE_CCRANGE)
            {
                if(c == ']' && escc == 0)
                {
                    charclassmem = -1;
                    mrx_guts_setmasktoken(&token, '-');
                    state = MRX_STATE_QUANT;
                    continue;
                }
                else
                {
                    if(charclassmem == -1)
                    {
                        mrx_context_seterror(ctx, "character class range is broken");
                        return -1; /*  probably tried to use a character class shorthand as part of a range */
                    }
                    if((uint8_t)c < charclassmem)
                    {
                        mrx_context_seterror(ctx, "character class range is misordered");
                        return -1; /*  range is in wrong order */
                    }
                    /* printf("enabling char class from %d to %d...\n", charclassmem, c); */
                    for(clsi = c; clsi > charclassmem; clsi--)
                    {
                        mrx_guts_setmasktoken(&token, clsi);
                    }
                    state = MRX_STATE_CCNORMAL;
                    charclassmem = -1;
                }
            }
        }
        else
        {
            assert(0);
        }
    }
    if(parencount > 0)
    {
        mrx_context_seterror(ctx, "(parencount > 0)");
        return -1; /*  unbalanced parens */
    }
    if(escstate != 0)
    {
        mrx_context_seterror(ctx, "(escstate != 0)");
        return -1; /*  open escape sequence */
    }
    if(state >= MRX_STATE_CCINIT)
    {
        mrx_context_seterror(ctx, "(state >= MRX_STATE_CCINIT)");
        return -1; /*  open character class */
    }
    if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
    {
        return -2;
    }
    /*  add invisible non-capturing group specifier */
    token.kind = MRX_KIND_CLOSE;
    token.countlow = 1;
    token.counthigh = 2;
    if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
    {
        return -2;
    }
    /*  add end token (tells matcher that it's done) */
    token.kind = MRX_KIND_END;
    if(!mrx_guts_pushtoken(ctx, &token, tokenslen, &k, &macn))
    {
        return -2;
    }
    ctx->tokens[0].pairoffset = k - 2;
    ctx->tokens[k - 2].pairoffset = -(k - 2);
    ctx->tokencount = k;
    /*  copy quantifiers from )s to (s (so (s know whether they're optional) */
    /*  also take the opportunity to smuggle "quantified group index" into the mask field forthe ) */
    n = 0;
    for(k2 = 0; k2 < k; k2++)
    {
        if(ctx->tokens[k2].kind == MRX_KIND_CLOSE)
        {
            ctx->tokens[k2].mask[0] = n++;
            k3 = k2 + ctx->tokens[k2].pairoffset;
            ctx->tokens[k3].countlow = ctx->tokens[k2].countlow;
            ctx->tokens[k3].counthigh = ctx->tokens[k2].counthigh;
            ctx->tokens[k3].mask[0] = n++;
            ctx->tokens[k3].mode = ctx->tokens[k2].mode;
            /* if(n > 65535) */
            if(n > 1024)
            {
                return -1; /*  too many quantified groups */
            }
        }
        else if(ctx->tokens[k2].kind == MRX_KIND_OR || ctx->tokens[k2].kind == MRX_KIND_OPEN || ctx->tokens[k2].kind == MRX_KIND_NCOPEN)
        {
            /*  find next | or ) and how far away it is. store in token */
            balance = 0;
            found = -1;
            for(l = k2 + 1; l < tokenslen; l++)
            {
                if(ctx->tokens[l].kind == MRX_KIND_OR && balance == 0)
                {
                    found = l;
                    break;
                }
                else if(ctx->tokens[l].kind == MRX_KIND_CLOSE)
                {
                    if(balance == 0)
                    {
                        found = l;
                        break;
                    }
                    else
                    {
                        balance -= 1;
                    }
                }
                else if(ctx->tokens[l].kind == MRX_KIND_NCOPEN || ctx->tokens[l].kind == MRX_KIND_OPEN)
                {
                    balance += 1;
                }
            }
            if(found == -1)
            {
                mrx_context_seterror(ctx, "unbalanced parens...");
                return -1; /*  unbalanced parens */
            }
            diff = found - k2;
            if(diff > 32767)
            {
                mrx_context_seterror(ctx, "too long...");
                return -1; /*  too long */
            }
            if(ctx->tokens[k2].kind == MRX_KIND_OR)
            {
                ctx->tokens[k2].pairoffset = diff;
            }
            else
            {
                ctx->tokens[k2].mask[15] = diff;
            }
        }
    }
    return 0;
}

/*  NOTE: undef'd later */
MRX_INLINE bool mrx_guts_checkmask(MRXToken* tokens, int k, int byte)
{
    return (!!(tokens[k].mask[((uint8_t)byte) >> 4] & (1 << ((uint8_t)byte & 0xF))));
}

MRX_INLINE bool mrx_guts_rwnddosave(MRXContext* ctx, int i, int k, int isdummy, uint16_t* stn, int stszm, uint64_t rngmin, uint64_t rngmax, MRXMatchState* rws, uint32_t* qgs, const uint32_t* qgstate)
{
    MRXMatchState s;
    if((*stn) >= stszm)
    {
        mrx_context_seterror(ctx, "out of backtracking room. returning");
        return false;
    }
    memset(&s, 0, sizeof(MRXMatchState));
    s.i = i;
    s.k = k;
    s.rangemin = rngmin;
    s.rangemax = rngmax;
    s.prev = 0;
    if(isdummy)
    {
        s.prev = 0xFAC7;
    }
    else if(ctx->tokens[s.k].kind == MRX_KIND_CLOSE)
    {
        s.groupstate = qgstate[ctx->tokens[s.k].mask[0]];
        s.prev = qgs[ctx->tokens[s.k].mask[0]];
        qgs[ctx->tokens[s.k].mask[0]] = (*stn);
    }
    rws[(*stn)++] = s;
    return true;
}

#define mrx_guts_macrwnddosavedummy(k)                                                                                         \
    if(!mrx_guts_rwnddosave(ctx, i, k, 1, &stackn, stacksizemax, rngmin, rngmax, rewindstack, qgroupstack, qgroupstate)) \
    {                                                                                                                          \
        return -2;                                                                                                             \
    }

#define mrx_guts_macrwnddosave(k)                                                                                              \
    if(!mrx_guts_rwnddosave(ctx, i, k, 0, &stackn, stacksizemax, rngmin, rngmax, rewindstack, qgroupstack, qgroupstate)) \
    {                                                                                                                          \
        return -2;                                                                                                             \
    }

/*  Returns 0 if a rewind state was popped (the match should continue from it).
 * Returns -1 if the rewind stack was empty (the caller should return -1 for no match). */
MRX_INLINE int mrx_guts_rwndorabort(MRXContext* ctx, uint16_t* stn, uint64_t* rmin, uint64_t* rmax, uint64_t* i, uint32_t* k, uint8_t* jrwnd, MRXMatchState* rws, uint32_t* qgs, uint32_t* qgstate)
{
    if((*stn) == 0)
    {
        return -1;
    }
    (*stn) -= 1;
    while((*stn) > 0 && rws[(*stn)].prev == 0xFAC7)
    {
        (*stn) -= 1;
    }
    (*jrwnd) = 1;
    (*rmin) = rws[(*stn)].rangemin;
    (*rmax) = rws[(*stn)].rangemax;
    assert(rws[(*stn)].i <= (*i));
    (*i) = rws[(*stn)].i;
    (*k) = rws[(*stn)].k;
    if(ctx->tokens[(*k)].kind == MRX_KIND_CLOSE)
    {
        qgstate[ctx->tokens[(*k)].mask[0]] = rws[(*stn)].groupstate;
        qgs[ctx->tokens[(*k)].mask[0]] = rws[(*stn)].prev;
    }
    /*  the -= 1 is because of the k++ in the forloop */
    (*k) -= 1;
    return 0;
}

MRX_INLINE bool _REGEX_CHECK_IS_W(const uint64_t* wmask, int byte)
{
    return (!!(wmask[((uint8_t)byte) >> 4] & (1 << ((uint8_t)byte & 0xF))));
}

/*  Returns match length iftext starts with a regex match.
 * Returns -1 ifthe textstr doesn't start with a regex match.
 * Returns -2 ifthe matcher ran out of memory or the regex is too complex.
 * Returns -3 ifthe regex is somehow invalid.
 * The first capslots capture positions and spans (lengths) will be written to cappos and capspan. If zero, will not be written to.
 * SAFETY: The textstr variable must be null-terminated, and starti must be the index of a character within the string or its null terminator.
 * SAFETY: Tokens array must be terminated by a MRX_KIND_END token (done by default by mrx_regex_parse).
 * SAFETY: Partial capture data may be written even ifthe match fails.
 */

MRX_INLINE int64_t mrx_regex_match(MRXContext* ctx, const char* textstr, size_t textlen, size_t starti, uint16_t capslots, int64_t* cappos, int64_t* capspan)
{
    enum
    {
        stacksizemax = 1024,
        auxstatssize = 1024
    };
    int kind;
    size_t n;
    uint64_t tokenslen;
    uint32_t k;
    uint16_t caps;
    uint16_t stackn;
    uint64_t i;
    uint64_t rngmin;
    uint64_t rngmax;
    uint8_t justrewinded;
    size_t iterlimit;
    uint64_t origk;
    ptrdiff_t kdiff;
    uint32_t prev;
    uint8_t forcezero;
    uint32_t k2;
    uint64_t ntcnt;
    uint64_t oldi;
    uint64_t hiclimit;
    uint64_t rangelimit;
    uint16_t capindex;
    uint64_t wmask[16];
    /* quantified group state */
    uint8_t qgroupacceptszero[auxstatssize];
    /* number of repetitions */
    uint32_t qgroupstate[auxstatssize];
    /* location of most recent corresponding ) on stack. 0 means nowhere */
    uint32_t qgroupstack[auxstatssize];
    uint16_t qgroupcapindex[auxstatssize];
    MRXMatchState rewindstack[stacksizemax];

    if(capslots > auxstatssize)
    {
        capslots = auxstatssize;
    }
    memset(qgroupcapindex, 0xFF, sizeof(qgroupcapindex));
    tokenslen = 0;
    k = 0;
    caps = 0;
    while(ctx->tokens[k].kind != MRX_KIND_END)
    {
        if(ctx->tokens[k].kind == MRX_KIND_OPEN && caps < capslots)
        {
            qgroupcapindex[ctx->tokens[k].mask[0]] = caps;
            qgroupcapindex[ctx->tokens[k + ctx->tokens[k].pairoffset].mask[0]] = caps;
            cappos[caps] = -1;
            capspan[caps] = -1;
            caps += 1;
        }
        k += 1;
        if(ctx->tokens[k].kind == MRX_KIND_CLOSE || ctx->tokens[k].kind == MRX_KIND_OPEN || ctx->tokens[k].kind == MRX_KIND_NCOPEN)
        {
            if(ctx->tokens[k].mask[0] >= auxstatssize)
            {
                mrx_context_seterror(ctx, "too many qualified groups. returning");
                /* OOM: too many quantified groups */
                return -2;
            }
            qgroupstate[ctx->tokens[k].mask[0]] = 0;
            qgroupstack[ctx->tokens[k].mask[0]] = 0;
            qgroupacceptszero[ctx->tokens[k].mask[0]] = 0;
        }
    }
    tokenslen = k;
    stackn = 0;
    i = starti;
    rngmin = 0;
    rngmax = 0;
    justrewinded = 0;
    /* used in boundary anchor checker */
    memset(wmask, 0, sizeof(wmask));
    wmask[3] = 0x03FF;
    wmask[4] = 0xFFFE;
    wmask[5] = 0x87FF;
    wmask[6] = 0xFFFE;
    wmask[7] = 0x07FF;
    iterlimit = 10000;
    for(k = 0; k < tokenslen; k++)
    {
        /* iterlimit--; */
        if(iterlimit == 0)
        {
            mrx_context_seterror(ctx, "iteration limit exceeded. returning");
            return -2;
        }
        if(ctx->tokens[k].kind == MRX_KIND_CARET)
        {
            if(i != 0)
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            continue;
        }
        else if(ctx->tokens[k].kind == MRX_KIND_DOLLAR)
        {
            if(i < textlen)
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            continue;
        }
        else if(ctx->tokens[k].kind == MRX_KIND_BOUND)
        {
            if(i == 0 && !_REGEX_CHECK_IS_W(wmask, textstr[i]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            else if(i != 0 && (i >= textlen) && !_REGEX_CHECK_IS_W(wmask, textstr[i - 1]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            else if(i != 0 && (i < textlen) && _REGEX_CHECK_IS_W(wmask, textstr[i - 1]) == _REGEX_CHECK_IS_W(wmask, textstr[i]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
        }
        else if(ctx->tokens[k].kind == MRX_KIND_NBOUND)
        {
            if(i == 0 && _REGEX_CHECK_IS_W(wmask, textstr[i]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            else if(i != 0 && (i >= textlen) && _REGEX_CHECK_IS_W(wmask, textstr[i - 1]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
            else if(i != 0 && (i < textlen) && _REGEX_CHECK_IS_W(wmask, textstr[i - 1]) != _REGEX_CHECK_IS_W(wmask, textstr[i]))
            {
                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                {
                    return -1;
                }
            }
        }
        else
        {
            /* deliberately unmatchable token (e.g. a{0}, a{0,0}) */
            if(ctx->tokens[k].counthigh == 1)
            {
                if(ctx->tokens[k].kind == MRX_KIND_OPEN || ctx->tokens[k].kind == MRX_KIND_NCOPEN)
                {
                    k += ctx->tokens[k].pairoffset;
                }
                else
                {
                    k += 1;
                }
                continue;
            }
            if(ctx->tokens[k].kind == MRX_KIND_OPEN || ctx->tokens[k].kind == MRX_KIND_NCOPEN)
            {
                if(!justrewinded)
                {
                    /*  need this to be able to detect and reject zero-size matches */
                    /* qgroupstate[ctx->tokens[k].mask[0]] = i; */

                    /*  ifwe're lazy and the min length is 0, we need to try the non-group case first */
                    if((ctx->tokens[k].mode & MRX_MODE_LAZY) && (ctx->tokens[k].countlow == 0 || qgroupacceptszero[ctx->tokens[k + ctx->tokens[k].pairoffset].mask[0]]))
                    {
                        rngmin = 0;
                        rngmax = 0;
                        mrx_guts_macrwnddosave(k);
                        k += ctx->tokens[k].pairoffset; /*  automatic += 1 will put us past the matching ) */
                    }
                    else
                    {
                        rngmin = 1;
                        rngmax = 0;
                        mrx_guts_macrwnddosave(k);
                    }
                }
                else
                {
                    justrewinded = 0;
                    origk = k;
                    if(rngmin != 0)
                    {
                        k += rngmin;
                        if(ctx->tokens[k - 1].kind == MRX_KIND_OR)
                        {
                            k += ctx->tokens[k - 1].pairoffset - 1;
                        }
                        else if(ctx->tokens[k - 1].kind == MRX_KIND_OPEN || ctx->tokens[k - 1].kind == MRX_KIND_NCOPEN)
                        {
                            k += ctx->tokens[k - 1].mask[15] - 1;
                        }
                        if(ctx->tokens[k].kind == MRX_KIND_END) /*  unbalanced parens */
                        {
                            return -3;
                        }
                        if(ctx->tokens[k].kind == MRX_KIND_CLOSE)
                        {
                            /*  do nothing and continue on ifwe don't need this group */
                            if(ctx->tokens[k].countlow == 0 || qgroupacceptszero[ctx->tokens[k].mask[0]])
                            {
                                qgroupstate[ctx->tokens[k].mask[0]] = 0;
                                if(!(ctx->tokens[k].mode & MRX_MODE_LAZY))
                                {
                                    qgroupstack[ctx->tokens[k].mask[0]] = 0;
                                }
                                continue;
                            }
                            /*  otherwise go to the last point before the group */
                            else
                            {
                                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                                {
                                    return -1;
                                }
                                continue;
                            }
                        }

                        assert(ctx->tokens[k].kind == MRX_KIND_OR);
                    }
                    kdiff = k - origk;
                    rngmin = kdiff + 1;
                    mrx_guts_macrwnddosave(k - kdiff);
                }
            }
            else if(ctx->tokens[k].kind == MRX_KIND_CLOSE)
            {
                /*  unquantified */
                if(ctx->tokens[k].countlow == 1 && ctx->tokens[k].counthigh == 2)
                {
                    /*  forcaptures */
                    capindex = qgroupcapindex[ctx->tokens[k].mask[0]];
                    if(capindex != 0xFFFF)
                    {
                        mrx_guts_macrwnddosavedummy(k);
                    }
                }
                /*  quantified */
                else
                {
                    if(!justrewinded)
                    {
                        prev = qgroupstack[ctx->tokens[k].mask[0]];
                        rngmax = ctx->tokens[k].counthigh;
                        rngmax -= 1;
                        rngmin = qgroupacceptszero[ctx->tokens[k].mask[0]] ? 0 : ctx->tokens[k].countlow;
                        /* assert(qgroupstate[ctx->tokens[k + ctx->tokens[k].pairoffset].mask[0]] <= i); */
                        /* if(prev) assert(rewindstack[prev].i <= i); */
                        /*  minimum requirement not yet met */
                        if(qgroupstate[ctx->tokens[k].mask[0]] + 1 < rngmin)
                        {
                            qgroupstate[ctx->tokens[k].mask[0]] += 1;
                            mrx_guts_macrwnddosave(k);
                            k += ctx->tokens[k].pairoffset; /*  back to start of group */
                            k -= 1; /*  ensure we actually hit the group node next and not the node after it */
                            continue;
                        }
                        /*  maximum allowance exceeded */
                        else if(ctx->tokens[k].counthigh != 0 && qgroupstate[ctx->tokens[k].mask[0]] + 1 > rngmax)
                        {
                            rngmax -= 1;
                            if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                            {
                                return -1;
                            }
                            continue;
                        }

                        /*  fallback case to detect zero-length matches when we backtracked into the inside of this group */
                        /*  after an attempted parse of a second copy of itself */
                        forcezero = 0;
                        if(prev != 0 && (uint32_t)rewindstack[prev].i > (uint32_t)i)
                        {
                            /*  find matching open paren */
                            n = stackn - 1;
                            while(n > 0 && rewindstack[n].k != k + ctx->tokens[k].pairoffset)
                            {
                                n -= 1;
                            }
                            assert(n > 0);
                            if(rewindstack[n].i == i)
                            {
                                forcezero = 1;
                            }
                        }

                        /*  reject zero-length matches */
                        if((forcezero || (prev != 0 && (uint32_t)rewindstack[prev].i == (uint32_t)i))) /*   && qgroupstate[ctx->tokens[k].mask[0]] > 0 */
                        {
                            qgroupacceptszero[ctx->tokens[k].mask[0]] = 1;
                            if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                            {
                                return -1;
                            }
                            /* rngmax = qgroupstate[ctx->tokens[k].mask[0]]; */
                            /* rngmin = 0; */
                        }
                        else if(ctx->tokens[k].mode & MRX_MODE_LAZY) /*  lazy */
                        {
                            /*  continue on to past the group; group retry is in rewind state */
                            qgroupstate[ctx->tokens[k].mask[0]] += 1;
                            mrx_guts_macrwnddosave(k);
                            qgroupstate[ctx->tokens[k].mask[0]] = 0;
                        }
                        else /*  greedy */
                        {
                            /*  clear unwanted memory ifpossessive */
                            if((ctx->tokens[k].mode & MRX_MODE_POSSESSIVE))
                            {
                                k2 = k;
                                /*  special case forfirst, only rewind to (, not to ) */
                                if(qgroupstate[ctx->tokens[k].mask[0]] == 0)
                                {
                                    k2 = k + ctx->tokens[k].pairoffset;
                                }
                                if(stackn == 0)
                                {
                                    return -1;
                                }
                                stackn -= 1;
                                while(stackn > 0 && rewindstack[stackn].k != k2)
                                {
                                    stackn -= 1;
                                }
                                if(stackn == 0)
                                {
                                    return -1;
                                }
                            }
                            /*  continue to next match ifsane */
                            if((uint32_t)qgroupstate[ctx->tokens[k + ctx->tokens[k].pairoffset].mask[0]] < (uint32_t)i)
                            {
                                qgroupstate[ctx->tokens[k].mask[0]] += 1;
                                mrx_guts_macrwnddosave(k);
                                k += ctx->tokens[k].pairoffset; /*  back to start of group */
                                k -= 1; /*  ensure we actually hit the group node next and not the node after it */
                            }
                        }
                    }
                    else
                    {
                        justrewinded = 0;
                        if(ctx->tokens[k].mode & MRX_MODE_LAZY)
                        {
                            /*  lazy rewind: need to try matching the group again */
                            mrx_guts_macrwnddosavedummy(k);
                            qgroupstack[ctx->tokens[k].mask[0]] = stackn;
                            k += ctx->tokens[k].pairoffset; /*  back to start of group */
                            k -= 1; /*  ensure we actually hit the group node next and not the node after it */
                        }
                        else
                        {
                            /*  greedy. ifwe're going to go outside the acceptable range, rewind */
                            /* uint64_t oldi = i; */
                            if(qgroupstate[ctx->tokens[k].mask[0]] < rngmin && !qgroupacceptszero[ctx->tokens[k].mask[0]])
                            {
                                /* i = oldi; */
                                if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                                {
                                    return -1;
                                }
                            }
                            /*  otherwise continue on to past the group */
                            else
                            {
                                qgroupstate[ctx->tokens[k].mask[0]] = 0;
                                /*  forcaptures */
                                capindex = qgroupcapindex[ctx->tokens[k].mask[0]];
                                if(capindex != 0xFFFF)
                                {
                                    mrx_guts_macrwnddosavedummy(k);
                                }
                            }
                        }
                    }
                }
            }
            else if(ctx->tokens[k].kind == MRX_KIND_OR)
            {
                k += ctx->tokens[k].pairoffset;
                k -= 1;
            }
            else if(ctx->tokens[k].kind == MRX_KIND_NORMAL)
            {
                if(!justrewinded)
                {
                    ntcnt = 0;
                    /*  do whatever the obligatory minimum amount of matching is */
                    oldi = i;
                    while(ntcnt < ctx->tokens[k].countlow && (i < textlen) && mrx_guts_checkmask(ctx->tokens, k, textstr[i]))
                    {
                        i += 1;
                        ntcnt += 1;
                    }
                    if(ntcnt < ctx->tokens[k].countlow)
                    {
                        i = oldi;
                        if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                        {
                            return -1;
                        }
                        continue;
                    }
                    if(ctx->tokens[k].mode & MRX_MODE_LAZY)
                    {
                        rngmin = ntcnt;
                        rngmax = ctx->tokens[k].counthigh - 1;
                        mrx_guts_macrwnddosave(k);
                    }
                    else
                    {
                        hiclimit = ctx->tokens[k].counthigh;
                        if(hiclimit == 0)
                        {
                            hiclimit = ~hiclimit;
                        }
                        rngmin = ntcnt;
                        while((i < textlen) && mrx_guts_checkmask(ctx->tokens, k, textstr[i]) && ntcnt + 1 < hiclimit)
                        {
                            i += 1;
                            ntcnt += 1;
                        }
                        rngmax = ntcnt;
                        if(!(ctx->tokens[k].mode & MRX_MODE_POSSESSIVE))
                        {
                            mrx_guts_macrwnddosave(k);
                        }
                    }
                }
                else
                {
                    justrewinded = 0;
                    if(ctx->tokens[k].mode & MRX_MODE_LAZY)
                    {
                        rangelimit = rngmax;
                        if(rangelimit == 0)
                        {
                            rangelimit = ~rangelimit;
                        }
                        if(mrx_guts_checkmask(ctx->tokens, k, textstr[i]) && (i < textlen) && rngmin < rangelimit)
                        {
                            i += 1;
                            rngmin += 1;
                            mrx_guts_macrwnddosave(k);
                        }
                        else
                        {
                            if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                    else
                    {
                        if(rngmax > rngmin)
                        {
                            i -= 1;
                            rngmax -= 1;
                            mrx_guts_macrwnddosave(k);
                        }
                        else
                        {
                            if(mrx_guts_rwndorabort(ctx, &stackn, &rngmin, &rngmax, &i, &k, &justrewinded, rewindstack, qgroupstate, qgroupstack) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
            }
            else
            {
                fprintf(stderr, "unimplemented token kind %d\n", ctx->tokens[k].kind);
                assert(0);
            }
        }
        /*
        printf("k... %d\n", k);
        */
    }
    if(caps != 0)
    {
        /*
        printf("stackn: %d\n", stackn);
        */
        fflush(stdout);
        for(n = 0; n < stackn; n++)
        {
            MRXMatchState s = rewindstack[n];
            kind = ctx->tokens[s.k].kind;
            if(kind == MRX_KIND_OPEN || kind == MRX_KIND_CLOSE)
            {
                capindex = qgroupcapindex[ctx->tokens[s.k].mask[0]];
                if(capindex == 0xFFFF)
                {
                    continue;
                }
                if(ctx->tokens[s.k].kind == MRX_KIND_OPEN)
                {
                    cappos[capindex] = s.i;
                }
                else if(cappos[capindex] >= 0)
                {
                    capspan[capindex] = s.i - cappos[capindex];
                }
            }
        }
        /*  re-deinitialize capture positions that have no associated capture span */
        for(n = 0; n < caps; n++)
        {
            if(capspan[n] == -1)
            {
                cappos[n] = -1;
            }
        }
    }
    return i;
}

MRX_INLINE void mrx_guts_printcsmart(int c)
{
    if(c >= 0x20 && c <= 0x7E)
    {
        printf("%c", c);
    }
    else
    {
        printf("\\x%02x", c);
    }
}

MRX_INLINE void mrx_regex_printtokens(MRXToken* tokens)
{
    int c;
    int k;
    int cold;
    static const char* kindtostr[] = {
        "NORMAL", "OPEN", "NCOPEN", "CLOSE", "OR", "CARET", "DOLLAR", "BOUND", "NBOUND", "END",
    };
    static const char* modetostr[] = {
        "GREEDY",
        "POSSESS",
        "LAZY",
    };
    for(k = 0;; k++)
    {
        printf("%s\t%s\t", kindtostr[tokens[k].kind], modetostr[tokens[k].mode]);
        cold = -1;
        for(c = 0; c < (tokens[k].kind ? 0 : 256); c++)
        {
            if(mrx_guts_checkmask(tokens, k, c))
            {
                if(cold == -1)
                {
                    cold = c;
                }
            }
            else if(cold != -1)
            {
                if(c - 1 == cold)
                {
                    mrx_guts_printcsmart(cold);
                    cold = -1;
                }
                else if(c - 2 == cold)
                {
                    mrx_guts_printcsmart(cold);
                    mrx_guts_printcsmart(cold + 1);
                    cold = -1;
                }
                else
                {
                    mrx_guts_printcsmart(cold);
                    printf("-");
                    mrx_guts_printcsmart(c - 1);
                    cold = -1;
                }
            }
        }
        /*
        printf("\t");
        for(int i = 0; i < 16; i++)
            printf("%04x", tokens[k].mask[i]);
        */
        printf("\t{%d,%d}\t(%d)\n", tokens[k].countlow, tokens[k].counthigh - 1, tokens[k].pairoffset);
        if(tokens[k].kind == MRX_KIND_END)
        {
            break;
        }
    }
}
