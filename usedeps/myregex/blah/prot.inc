/* grep.c */
static inline RegexContext *mrx_init(RegexContext *ctx, RegexToken *tokens, size_t maxtokens);
void mrx_destroy(RegexContext *ctx);
void mrx_seterror(RegexContext *ctx, const char *fmt, ...);
static inline int isquantchar(int c);
static inline int mrx_regex_parse(RegexContext *ctx, const char *pattern, int32_t flags);
static inline int64_t mrx_regex_match(RegexContext *ctx, const char *text, size_t starti, uint16_t capslots, int64_t *cappos, int64_t *capspan);
static inline void mrx_regex_printtokens(RegexToken *tokens);
char *nn_util_filegetshandle(char *s, int size, FILE *f, size_t *lendest);
int nn_util_filegetlinehandle(char **lineptr, size_t *destlen, FILE *hnd);
void printhilite(const char *line, size_t len, size_t mtstart, size_t mtlength);
void dogrep(RegexContext *ctx, FILE *fh);
int main(int argc, char *argv[]);
