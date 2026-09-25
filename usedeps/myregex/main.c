
/*
// tests for regex engine; not actually part of BBEL
// testing requires PCRE2
// msys2: pacman -S mingw-w64-<flavor>-pcre2
// linker flag is usually -lpcre2-8
*/

#include <stdarg.h>
#include <stdbool.h>
#if 0
    #define REGEX_VERBOSE
#endif
#include <time.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include "mrx.h"

#define PCRE2_SPTR8(...) ((const unsigned char*)(__VA_ARGS__))

#define BE_QUIET

int myprintf(const char* fmt, ...)
{
    int rc;
    va_list va;
    va_start(va, fmt);
    rc = vfprintf(stdout, fmt, va);
    va_end(va);
    fflush(stdout);
    return rc;
}

void testify(void)
{
    PCRE2_SIZE erroroffset;
    PCRE2_SIZE* ovector;
    RegexContext ctx;
    bool has_possessive;
    clock_t start;
    const char* slowest_my_regex;
    const char* slowest_pcre2_regex;
    const char* text;
    const char* regex;
    const char* text_str;
    double slowest_my_regex_time;
    double slowest_pcre2_regex_time;
    double t;
    double total_my_regex_time;
    double total_pcre2_regex_time;
    int e;
    int errorcode;
    int submatch_count;
    int x;
    int16_t token_count;
    int32_t n;
    int64_t cap_pos[16];
    int64_t cap_span[16];
    int64_t match_len;
    int64_t matchlen;
    pcre2_code* re;
    pcre2_match_data* match_data;
    size_t i;
    size_t j;
    size_t offs;
    size_t pcre2_len;
    size_t size;
    size_t where;
    volatile int64_t a;

    static const char* regexes[] =
    {
        "(b|a|as|q|)*?X",
        "",
        "(|b|a|as|q)*X",
        "(b|a|as|q|)*X",
        "(b|a|as|q|)+X",
        "(b|a|as|q|)+?X",
        "((b|a|as|q|))*X",
        "((b|a|as|q|))*?X",
        "(b|a|as|q)*X",
        "(b|a|as|q)*?X",
        "(b|a|as|q)+X",

        "((a)|(b))+",
        "((a)|(b))++",
        "((a)|(b))+?",
        "((a)|(b))*",
        "((a)|(b))*+",
        "((a)|(b))*?",
        "((a)|((b)q))*",
        "((a)|((b)q))*+",

        "(|a?)+?a{10}",
        "(a?)*a{10}",
        "(a?)*?a{10}",
        "(a?)+?a{100}",
        "(a?)+?a{10}",
        "(a?)+a{10}",
        "(a)+a{9}",
        "(a)+?a{9}",
        "(|a)+a{9}",
        "(|a)+a{10}",
        "(|a)+a{11}",
        "(|a)+?a{11}",
        "^a(bc+|b[eh])g|.h$",
        "(bc+d$|ef*g.|h?i(j|k))",

        "(b|a|as|q)*?X",
#if 0
        /*
    (?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])
       */
#endif
        "(?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|\"(?:[\\x01-\\x08\\x0b\\x0c\\x0e-\\x1f\\x21\\x23-\\x5b\\x5d-\\x7f]|\\\\[\\x01-\\x09\\x0b\\x0c\\x0e-\\x7f])*\")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\\x01-\\x08\\x0b\\x0c\\x0e-\\x1f\\x21-\\x5a\\x53-\\x7f]|\\\\[\\x01-\\x09\\x0b\\x0c\\x0e-\\x7f])+)\\])",
#if 0
            "(?:\\w+(?:\\.\\w+)*)@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?)",
#endif
        "(?:\\w+(?:\\.\\w+)*)@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?)",
        "(\\w\\w*\\.)+",
        "(\\w+\\.)+",
        "(?:\\w+(?:\\.\\w+)*)@(?:\\w+(?:\\.\\w+)*)",
        "[a-z0-9\\._%+!$&*=^|~#%'`?{}/\\-]+@([a-z0-9\\-]+\\.){1,}([a-z]{2,16})",
        "^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$",

        "(ab?)b",
        "(ab?)*b",
        "(ab?)*?b",

        "([0a-z][a-z0-9]*,)+",
        "([a-z][a-z0-9]*,)+",

        "asdf\\b",
        "asdf\\B",
        "\\basdf",

        "(\\ba?)*",
        "(\\ba?)*?",
        "(\\b)+?",
        "(\\b)+",
        "(\\ba?)+",
        "(\\ba?)+?",
        "(\\ba?)*a",
        "(\\ba?)*?a",
        "(\\ba?)+a",
        "(\\ba?)+?a",
        "a(\\b)*",
        "a(\\b)*?",
        "(\\b)*a",
        "(\\b)*?a",
        "a(\\b)+",
        "a(\\b)+?",
        "(\\b)+a",
        "(\\b)+?a",

        "^asdf$",
        "^asdf",
        "asdf$",
        ".*asdf",
        ".*asdf$",

        "(^(asdf)?)*",
        "(^(asdf)?)*(asdf)?",
        "((asdf)?$)*",
        "((asdf)?)*((asdf)?$)*",
        "(^(asdf)?)*?",
        "(^(asdf)?)*?(asdf)?",
        "((asdf)?$)*?",
        "((asdf)?)*?((asdf)?$)*?",

        "(a?)*a{10}",
        "(a?)*?a{10}",
        "()",
        "(a|)*b",
        "(z?)*a{10}",

        /* possessive */
        "(b|a|)*+",
        "(a|)*+b",
        "(?>(b|a|)*)",
        "(b|a|)*+b",
        "(b|a|as|q)*+",
        "(b|a|as|q)*+X",
        "(b|a|as|q)*",
        "a++ab",

        "[0-9]+\\.[0-9]+",
        "[0-9]+0\\.[0-9]+",

        "(a|a|ab)bc",
        "(ab|ab|a)bc",
        "[0-9]\\.[0-9]",

        "\\d\\.\\d",
        "\\d*\\.\\d*",
        "\\w+",
        "\\s+",
        "\\s(\\w+)",
        "\\w+\\s",

        "(\\d)*?\\.(\\d)+",
        "([0-9])*?\\.([0-9])+",
        "([0-9]){3,5}?\\.([0-9])+",
        "[0-9]{3,5}?\\.[0-9]+",
        "([0-9]){3,5}\\.([0-9])+",
        "[0-9]{3,5}\\.[0-9]+",
        "(a|ab)*b",
        "(ab?)*?b",
        "(ab?\?)*b",
        "(a)?\?(b|a)",
        "(a)*a{10}",
        "(a)*?a{10}",
        "a()a",
        "a(|)a",
        "a(|){1}?a",
        "a(|b)+a",
        "a(|b)+?a",
        "(a|b)*?b",
        "a*a*?",
        "a*?a*",
        "(b|a)*b",
        "(b|a)*?b",
        "(b|a|)*",
        "(b|a|)*bb",
        "(b|a|)*?bb",
        "(|a)+",
        "(|a)+?",
        "()+",
        "()+?",
        "(|)+?",
        "a(|)*a",
        "a(|)*?a",
        "(a|(((()))))*b",
        "((\\w+,?)*:)*",
        "((\\w+,?)*+:)*",
        "((\\w+,?)*+:)*+",

        /* pathological */
        "((a?b|a)b?)*",
        "(.*,){11}P",
        "(.*?,){11}P",

        "mistaken bogus regex",
    };
    static const char* texts[] = 
    {
        "asqbX",

        "aaaaaaaaaa",
        "asqb",
        "abh",

        "effgz",
        "ij",
        "effg",
        "bcdd",
        "reffgz",

        "testacc@example.com",

        "aa.bb.cc.dd",
        "a5,b7,c9",
        "a5,b7,c9,",
        "a5,b7,c9,,",
        "a5,b7,c9,1",
        "a5,b7,c9,a",
        "",
        " ",
        "  ",
        "a",
        "aa",
        "aba) ",
        "aaaaaaaaa",
        "aaaaaaaaaaaaaa",
        "aaaaaaaaaaaaaab",
        "aaaaaaaaaaaaaaba",

        "testacc@example.com",
        "test+acc@example.com",
        "test.acc@example.com",
        "test.acc.acc@sub.example.com",
        "loooooooo10235699ng.1g.g.g.210g01.longie.acc@sub.example.com.co.co.uk.jp.fakedomain.loooooooooooooooooooonger.com......",
        "test.acc@sub.example.com",
        "test@sub.example.com",
        "@example.com",
        "example.com",
        "a@",
        "#@%^%#$@#$@#.com",
        "Joe Smith <email@example.com>",
        "_______@example.com",
        "“email”@example.com",
        "email@[123.123.123.123]",
        "email@123.123.123.123",

        "abc) ",
        "abba) ",
        "abbc) ",
        "012.53) ",
        ".53) ",
        "5.5",
        "022134.53) ",
        "02234.53) ",
        "1131.53) ",
        "131.53) ",
        "11.53) ",
        "1.53) ",
        "aa",
        "aaaaaaaaabababab",
        "aaaaaaaaababababb",
        "aaaaabbbbbbbx",
        "bbbbbbb",
        "1,2,3,4,5,6,7,8,9,10,11,12",
        "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16",

        "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22",
        /*
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25   P",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26   P",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34",
        //"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35",
        */
        "aaaaaababababababaabx",

        "       ",
        "afd1gkage919953bd       ",
        "   x    ",
        "   ,\\1264ga0b a    ",
        "asdf ",
        "asdfg",
        "   asdf",
        "asdf   ",
        "   asdf   ",
        "XXXasdf",
        "asdfXXX",
        "XXXasdfXXX",
        "000asdf",
        "asdf000",
        "000asdf000",
        "a,b,easbe_1:a,:a",

        "uh-uh",
        "words, yeah",
        "mistaken bogus regex",

        "aaaaaabbbabaqa",
    };
    slowest_my_regex = "";
    slowest_my_regex_time = 0.0;
    total_my_regex_time = 0.0;
    slowest_pcre2_regex = "";
    slowest_pcre2_regex_time = 0.0;
    total_pcre2_regex_time = 0.0;
    mrx_context_initctx(&ctx, true);
    for(i = 0; i < sizeof(regexes) / sizeof(regexes[0]); i++)
    {
        regex = regexes[i];
        token_count = ctx.maxtokens;
        start = clock();
        e = mrx_regex_parse(&ctx, regex, 0);
        t = (clock() - start) / 1000000.0;
        assert(!e);
        total_my_regex_time += t;
        if(t > slowest_my_regex_time)
        {
            slowest_my_regex_time = t;
            slowest_my_regex = regex;
        }
        has_possessive = false;
        for(n = 0; n < token_count; n++)
        {
            if(ctx.tokens[n].mode & MRX_MODE_POSSESSIVE)
            {
                has_possessive = true;
                break;
            }
        }
    #if 0
        #ifndef BE_QUIET
            myprintf("token count: %d\n", token_count);
            mrx_regex_printtokens(ctx.tokens);
            myprintf("Took %f seconds for my regex engine to parse the regex\n", t);
        #endif
    #endif
        start = clock();
        re = pcre2_compile(PCRE2_SPTR8(regex), PCRE2_ZERO_TERMINATED,
                           PCRE2_ANCHORED | PCRE2_NO_UTF_CHECK | PCRE2_DOTALL | PCRE2_NO_AUTO_POSSESS | PCRE2_NO_DOTSTAR_ANCHOR | PCRE2_NO_START_OPTIMIZE,
                           &errorcode, &erroroffset, NULL);
        t = (clock() - start) / 1000000.0;
    #if 0
        #ifndef BE_QUIET
            myprintf("Took %f seconds for my PCRE2 to compile the regex\n", t);
        #endif
    #endif
        if(t > slowest_pcre2_regex_time)
        {
            slowest_pcre2_regex_time = t;
            slowest_pcre2_regex = regex;
        }
        total_pcre2_regex_time += t;
        for(j = 0; j < sizeof(texts) / sizeof(texts[0]); j++)
        {
            text = texts[j];
            text_str = text;
            pcre2_len = -1;
            myprintf("testing PCRE2 regex `%s` on string `%s`...\n", regex, text);
            start = clock();
            match_data = pcre2_match_data_create_from_pattern(re, 0);
            submatch_count = pcre2_match(re, PCRE2_SPTR8(text), strlen(text_str), 0, PCRE2_ANCHORED | PCRE2_NO_UTF_CHECK, match_data, 0);
            t = (clock() - start) / 1000000.0;
            /*
            //myprintf("submatch count: %d\n", submatch_count);
            //myprintf("ovector count: %d\n", pcre2_get_ovector_count(match_data));
            */
            ovector = 0;
            if(submatch_count > 0)
            {
                ovector = pcre2_get_ovector_pointer(match_data);
                offs = ovector[0];
                if(offs == 0)
                {
                    pcre2_len = ovector[1] - offs;
                }
                myprintf("pcre2 regex found match at %zd with len %zd after %f seconds\n", offs, pcre2_len, t);
            }
            else
            {
                myprintf("pcre2 regex found no match after %f seconds\n", t);
            }
            myprintf("testing my regex `%s` on string `%s`...\n", regex, text);
            start = clock();
            memset(cap_pos, 0xFF, sizeof(cap_pos));
            memset(cap_span, 0xFF, sizeof(cap_span));

            match_len = mrx_regex_match(&ctx, text, 0, 16, cap_pos, cap_span);

            assert(match_len != -3);
                t = (clock() - start) / 1000000.0;
                if(match_len >= 0)
                {
                    myprintf("my regex found match with len %zd after %f seconds\n", match_len, t);
                }
                else if(match_len == -2)
                {
                    myprintf("my regex ran out of memory after %f seconds (note: `%s`)\n", t, regex);
                }
                else
                {
                    myprintf("my regex found no match after %f seconds\n", t);
                }

            /* we define captures differently than PCRE2 for possessives, so skip them */
            if(!has_possessive && submatch_count > 0)
            {
            #if 1
                //myprintf("comparing %zd to %zd...\n", match_len, pcre2_len);
                myprintf("regex `%s`, string `%s`\n", regex, text);
            #endif
                assert((size_t)match_len == pcre2_len);
    #ifndef BE_QUIET
                puts("comparing captures...");
    #endif
                if(match_len >= 0)
                {
                    for(x = 0; x < submatch_count && x < 16; x++)
                    {
                        where = ovector[x * 2];
                        if(where == 0)
                        {
                            pcre2_len = ovector[x * 2 + 1] - where;
/* probably a situation of std capturing a zero-length group repetition */
    #ifndef BE_QUIET
                            myprintf("Capture %d: std (%zd,%zd)  mine (%zd,%zd)\n", x, where, pcre2_len, cap_pos[x], cap_span[x]);
    #endif
                            if(!(cap_pos[x] == -1 && cap_span[x] == -1 && where == 0 && pcre2_len == 0))
                            {
                                assert(where == (size_t)cap_pos[x]);
                                assert(pcre2_len == (size_t)cap_span[x]);
                            }
                        }
                    }
                }
            }
            pcre2_match_data_free(match_data);
        }
        pcre2_code_free(re);
    }
    myprintf("Slowest regex for me to parse at %f seconds:\n%s\n", slowest_my_regex_time, slowest_my_regex);
    myprintf("Slowest regex for pcre2 to parse at %f seconds:\n%s\n", slowest_pcre2_regex_time, slowest_pcre2_regex);
    myprintf("Total parse time for me: %f\n", total_my_regex_time);
    myprintf("Total parse time for pcre2: %f\n", total_pcre2_regex_time);
    token_count = ctx.maxtokens;
    e = mrx_regex_parse(&ctx, "((a)|(b))++", 0);
    assert(!e);
    memset(cap_pos, 0xFF, sizeof(cap_pos));
    memset(cap_span, 0xFF, sizeof(cap_span));
    matchlen = mrx_regex_match(&ctx, "aaaaaabbbabaqa", 0, 5, cap_pos, cap_span);
    myprintf("Match length: %zd\n", matchlen);
    for(i = 0; i < 5; i++)
        myprintf("Capture %d: %zd plus %zd\n", i, cap_pos[i], cap_span[i]);
    mrx_regex_printtokens(ctx.tokens);
    puts("All regex tests passed!");
    if(1)
    {
        puts("Microbenchmark: matching `\\.\\d+|\\d+\\.\\d*` against 3.1415926535 one million times...");
        token_count = ctx.maxtokens;
        e = mrx_regex_parse(&ctx, "\\.\\d+|\\d+\\.\\d*", 0);
        assert(!e);
        start = clock();
        for(i = 0; i < 1000000; i++)
        {
            matchlen = mrx_regex_match(&ctx, "3.1415926535", 0, 0, 0, 0);
            assert(matchlen == 12);
            a = 0;
            /* force the loop to not be optimized away */
            matchlen = a;
        }
        t = (clock() - start) / 1000000.0;
        myprintf("Match time for me: %f\n", t);
        re = pcre2_compile(PCRE2_SPTR8("\\.\\d+|\\d+\\.\\d*"), PCRE2_ZERO_TERMINATED,
                           PCRE2_ANCHORED | PCRE2_NO_UTF_CHECK | PCRE2_DOTALL | PCRE2_NO_AUTO_POSSESS | PCRE2_NO_DOTSTAR_ANCHOR | PCRE2_NO_START_OPTIMIZE,
                           &errorcode, &erroroffset, NULL);
        match_data = pcre2_match_data_create_from_pattern(re, 0);
        ovector = pcre2_get_ovector_pointer(match_data);
        start = clock();
        size = strlen("3.1415926535");
        for(i = 0; i < 1000000; i++)
        {
            submatch_count = pcre2_match(re, PCRE2_SPTR8("3.1415926535"), size, 0, PCRE2_ANCHORED | PCRE2_NO_UTF_CHECK, match_data, 0);
            matchlen = ovector[1] - ovector[0];
            assert(submatch_count == 1 && matchlen == 12);
            a = 0;
            /* force the loop to not be optimized away */
            matchlen = a;
        }
        t = (clock() - start) / 1000000.0;
        myprintf("Match time for pcre2: %f\n", t);
    }
}

int main(void)
{
    myprintf("running tests, please be patient...\n");
    testify();
}
