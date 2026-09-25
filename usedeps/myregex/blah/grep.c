
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../mrx.h"

    enum {
        matchMaxTokens = 128*4,
        matchMaxCaptures = 128,
        maxLineBuf = 1024*8,
    };

char* nn_util_filegetshandle(char* s, int size, FILE *f, size_t* lendest)
{
    int c;
    char *p;
    p = s;
    (*lendest) = 0;
    if (size > 0)
    {
        while (--size > 0) 
        {
            if ((c = getc(f)) == -1)
            {
                if (ferror(f) == EINTR)
                {
                    continue;
                }
                break;
            }
            *p++ = c & 0xff;
            (*lendest) += 1;
            if(c == '\n')
            {
                break;
            }
        }
        *p = '\0';
    }
    if(p > s)
    {
        return s;
    }
    return NULL;
}


int nn_util_filegetlinehandle(char **lineptr, size_t *destlen, FILE* hnd)
{
    enum { kInitialStrBufSize = 256 };
    static char stackbuf[kInitialStrBufSize];
    char *heapbuf;
    size_t getlen;
    unsigned int linelen;
    getlen = 0;
    if(lineptr == NULL || destlen == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    if(ferror(hnd))
    {
        return -1;
    }
    if (feof(hnd))
    {
        return -1;     
    }
    nn_util_filegetshandle(stackbuf,kInitialStrBufSize,hnd, &getlen);
    heapbuf = strchr(stackbuf,'\n');   
    if(heapbuf)
    {
        *heapbuf = '\0';
    }
    linelen = getlen;
    if((linelen+1) < kInitialStrBufSize)
    {
        heapbuf = (char*)realloc(*lineptr, kInitialStrBufSize);
        if(heapbuf == NULL)
        {
            return -1;
        }
        *lineptr = heapbuf;
        *destlen = kInitialStrBufSize;
    }
    strcpy(*lineptr,stackbuf);
    *destlen = linelen;
    return linelen;
}

void printhilite(const char* line, size_t len, size_t mtstart, size_t mtlength)
{
    size_t i;
    for(i=0; i<len; i++)
    {
        if(i == mtstart)
        {
            printf("\e[31m");
        }
        printf("%c", line[i]);
        if(i == mtlength)
        {
            printf("\e[0m");
        }
    }
    if(line[mtlength - 1] != '\n')
    {
        printf("\n");
    }
}

REMIMU_INLINE int mrx_regex_parse(RegexContext* ctx, const char* pattern, int32_t flags);
REMIMU_INLINE int64_t mrx_regex_match(RegexContext* ctx, const char* text, size_t starti, uint16_t capslots, int64_t* cappos, int64_t* capspan);


void dogrep(RegexContext* ctx, FILE* fh)
{
    int rc;
    int64_t i;
    int64_t cpres;
    int64_t actualmaxcaptures;
    int64_t mtstart;
    int64_t mtlength;
    size_t linelen;
    char* lineptr;
    int64_t capstarts[matchMaxCaptures + 1] = {0};
    int64_t caplengths[matchMaxCaptures + 1] = {0};
    actualmaxcaptures = matchMaxCaptures;
    while(true)
    {
        lineptr = NULL;
        rc  = nn_util_filegetlinehandle(&lineptr, &linelen, fh);
        if(rc == -1)
        {
            return;
        }
        //fprintf(stderr, "line=<<<%.*s>>>\n", linelen, lineptr);
        cpres = mrx_regex_match(ctx, lineptr, 0, actualmaxcaptures, capstarts, caplengths);
        if(cpres > 0)
        {
            //fprintf(stderr, "cpres=<<<%d>>>\n", cpres);
            for(i=0; i<cpres; i++)
            {
                mtstart = capstarts[i];
                mtlength = caplengths[i];
                if(mtlength > 0)
                {
                    printhilite(lineptr, linelen, mtstart, mtlength);
                }
            }
        }
        //prx->
        free(lineptr);
    }
}

int main(int argc, char* argv[])
{

    int prc;
    int fi;
    RegexToken tokens[matchMaxTokens + 1] = {};
    const char* pattern;
    FILE* fh;
    RegexContext ctx; 
    RegexContext* pctx;
    pattern = argv[1];
    fprintf(stderr, "pattern=%s\n", pattern);
    pctx = mrx_init(&ctx, tokens, matchMaxTokens);
    prc = mrx_regex_parse(pctx, pattern, REMIMU_FLAG_DOT_NO_NEWLINES);
    mrx_regex_printtokens(pctx->tokens);
    if(prc == 0)
    {
        for(fi=2; fi<argc; fi++)
        {
            fh = fopen(argv[fi], "rb");
            if(fh == NULL)
            {
                fprintf(stderr, "cannot open '%s' for reading\n", argv[fi]);
            }
            else
            {
                dogrep(pctx, fh);
                fclose(fh);
            }
        }
    }
    else
    {
        fprintf(stderr, "failed to compile regular expression: %s\n", pctx->errorbuf);
    }
    mrx_destroy(pctx);

}
