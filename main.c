
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <wchar.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <io.h>
#endif

#include "usedeps/myregex/mrx.h"

#define BOA_CONFIG_USELINO 1

#if !defined(BOA_INLINE)
    #if defined(__muccdebug__)
        #define BOA_INLINE
        #define BOA_FORCEINLINE
    #else
        /* gcc defines __STRICT_ANSI__ in C++, because C++ defaults to strict mode */
        #if (defined(__STRICT_ANSI__) && (!defined(__cplusplus))) || defined(__PCC__)
            #define BOA_INLINE
            #define BOA_FORCEINLINE 
            /* prot.inc would still use the inline keyword */
            #undef inline
            #define inline
        #else
            #if defined(__GNUC__) || defined(__TINYC__)
                #define BOA_INLINE static inline
                #define BOA_FORCEINLINE static __attribute__((always_inline)) inline
            #else
                #define BOA_INLINE static inline
                #define BOA_FORCEINLINE static inline
            #endif
        #endif
    #endif
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define BOA_LIKELY(x) (__builtin_expect(!!(x), 1))
    #define BOA_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
    #define BOA_LIKELY(x) (x)
    #define BOA_UNLIKELY(x) (x)
#endif

#if defined(__PCC__)
int __dso_handle = 0;
#endif

#if defined(__STRICT_ANSI__)
char *strdup(const char *s);
char *strndup(const char* s, size_t n);
char *strdupa(const char *s);
char *strndupa(const char* s, size_t n);
void* memccpy(void* dest, const void* src, int c, size_t n);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
#endif

#if defined(__unix__) || defined(__linux__)
    #define BOA_OSPLATFORM_ISUNIXLIKE 1
#endif

#if defined(__unix__) || defined(__linux__)
    #define BOA_OSPLATFORM_ISLINUX 1
#else
    #if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
        #define BOA_OSPLATFORM_ISWINNT 1
    #endif
#endif

#if defined(BOA_OSPLATFORM_ISLINUX)
    #include <unistd.h>
    #include <dirent.h>
#else
    #if defined(BOA_OSPLATFORM_ISWINNT)
        #include <windows.h>
    #endif
#endif

#if !defined(OSLIB_CONF_OSPATHSIZE)
    #define OSLIB_CONF_OSPATHSIZE 1024
#endif

#if defined(BOA_OSPLATFORM_LINUX)
    #undef BOA_CONFIG_PLATFORMNAME
    #define BOA_CONFIG_PLATFORMNAME "linux"
#elif defined(BOA_CONFIG_OSPLATFORM_ISWINNT)
    #undef BOA_CONFIG_PLATFORMNAME
    #define BOA_CONFIG_PLATFORMNAME "windows"
#endif

#if defined(_WIN64) || defined(__x86_64) || defined(__amd64__) || (__LONG_WIDTH__ == 64)
    #undef BOA_CONFIG_ARCHNAME
    #undef BOA_CONFIG_ARCHBITS
    #define BOA_CONFIG_ARCHNAME "x64"
    #define BOA_CONFIG_ARCHBITS 64
#elif (__LONG_WIDTH__ == 32)
    #undef BOA_CONFIG_ARCHNAME
    #undef BOA_CONFIG_ARCHBITS
    #define BOA_CONFIG_ARCHNAME "x86"
    #define BOA_CONFIG_ARCHBITS 32
#endif

#if !defined(BOA_CONFIG_ARCHNAME)
    #define BOA_CONFIG_ARCHNAME "unknown"    
#endif
#if !defined(BOA_CONFIG_PLATFORMNAME)
    #define BOA_CONFIG_PLATFORMNAME "unknown"
#endif
#if !defined(BOA_CONFIG_ARCHBITS)
    #define BOA_CONFIG_ARCHBITS 32
#endif

#ifndef S_IREAD
    #define S_IREAD     (0400)
#endif /* S_IREAD */
#ifndef S_IWRITE
    #define S_IWRITE    (0200)
#endif /* S_IWRITE */
#ifndef S_IEXEC
    #define S_IEXEC     (0100)
#endif /* S_IEXEC */
#if !defined(S_IRUSR)
    #define S_IRUSR (S_IREAD)
#endif
#if !defined(S_IWUSR)
    #define S_IWUSR (S_IWRITE)
#endif
#if !defined(S_IXUSR)
    #define S_IXUSR (S_IEXEC)
#endif
#if !defined(S_IRGRP)
    #define S_IRGRP (S_IRUSR >> 3)
#endif
#if !defined(S_IWGRP)
    #define S_IWGRP (S_IWUSR >> 3)
#endif
#if !defined(S_IXGRP)
    #define S_IXGRP (S_IXUSR >> 3)
#endif
#if !defined(S_IROTH)
    #define S_IROTH (S_IRUSR >> 6)
#endif
#if !defined(S_IWOTH)
    #define S_IWOTH (S_IWUSR >> 6)
#endif
#if !defined(S_IXOTH)
    #define S_IXOTH (S_IXUSR >> 6)
#endif
#if !defined(S_IRWXU)
    #define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#endif
#if !defined(S_IRWXG)
    #define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#endif
#if !defined(S_IRWXO)
    #define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)
#endif
#if !defined(S_IFLNK)
    #define S_IFLNK (0120000)
#endif
#if !defined(S_IFMT)
    #define S_IFMT  (00170000)
#endif
#if !defined(S_IFDIR)
    #define S_IFDIR 0040000 /* directory */
#endif
#if !defined (S_ISDIR)
    #define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)	/* directory */
#endif
#if !defined(S_IFREG)
    #define S_IFREG (0100000) /* regular file */
#endif
#if !defined (S_ISREG)
    #define	S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)	/* file */
#endif
#if !defined(S_ISLNK)
    #define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(DT_DIR)
    #define DT_DIR (4)
#endif
#if !defined(DT_REG)
    #define DT_REG (8)
#endif
#if !defined(PATH_MAX)
    #define PATH_MAX (1024)
#endif

#if !defined(va_copy)
    #if defined(__GNUC__) || defined(__CLANG__)
        #define va_copy(d, s) __builtin_va_copy(d, s)
    #else
        #define va_copy(dest, src) memcpy(dest, src, sizeof(va_list))
    #endif
#endif

#include "optparse.h"
#include "allocator.h"

#if defined(BOA_CONFIG_USELINO) && (BOA_CONFIG_USELINO == 1)
    #include "lino.h"
#endif

#if defined(M_PI)
    #define BOA_CONST_M_PI M_PI
#else
    #define BOA_CONST_M_PI (3.14)
#endif

#define BOA_CONFIG_MAXSHORTSTRLENGTH (16)

#define BOA_CONFIG_DEBUGTRACECHUNK 0
#define BOA_CONFIG_DEBUGLOGGC 0
#define BOA_CONFIG_DEBUGLOGALLOCATION 0
#define BOA_CONFIG_DEBUGLOGMARKING 0
#define BOA_CONFIG_DEBUGLOGBLACKING 0

/* make sure that we did not break anything */
#define BOA_CONFIG_DEBUGSTRESSTESTGC 0
#define BOA_CONFIG_TPLSTRINGNMAXNESTING (4)
#define BOA_CONFIG_GCHEAPGROWFACTOR (2)
#define BOA_CONFIG_INITIALCALLFRAMES (1024)
#define BOA_CONFIG_UINT8COUNT (UINT8_MAX + 1)
#define BOA_CONFIG_UINT16COUNT (UINT16_MAX + 1)

/* do not change these, or old bytecode files will break! */
#define BOA_CONFIG_BCVERSION 0
#define BOA_CONFIG_BCMAGICNUMBER (6932)
#define BOA_CONFIG_BCENDNUMBER (2942)

/* was: 48 -- but it's not actually necessary to encode strings */
#define BOA_CONFIG_BCSTRINGKEY (0)
/* top load of a table before reallocation kicks in */
#define BOA_CONFIG_TABLEMAXLOAD (0.75)
#define BOA_CONFIG_LONGESTOPNAME (13)

/* can't be over 255 */
#define BOA_CONFIG_REGISTERSMAX (255)
#define BOA_CONFIG_OPCODESIZE (0x3f)
#define BOA_CONFIG_ARGSIZEA (0xff)
#define BOA_CONFIG_ARGSIZEB (0x1ff)
#define BOA_CONFIG_ARGSIZEC (0x1ff)
/* 18 bits max */
#define BOA_CONFIG_ARGSIZEBX (0x3ffff)
/* 17 bits max */
#define BOA_CONFIG_ARGSIZESBX (0x1ffff)
#define BOA_CONFIG_ARGPOSA (6)
#define BOA_CONFIG_ARGPOSB (14)
#define BOA_CONFIG_ARGPOSC (23)
#define BOA_CONFIG_ARGPOSBX (14)
#define BOA_CONFIG_ARGPOSSBX (15)
#define BOA_CONFIG_FLAGPOSSBX (14)

#define STRBUF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define STRBUF_MAX(x, y) ((x) > (y) ? (x) : (y))

#define BOA_UTIL_UNREACHABLE() \
    { \
        fprintf(stderr, "unreachable code was reached at %s:%i\n", __FILE__, __LINE__); \
        assert(false); \
    }

#define BOA_BIT_SETBIT(number, n) number |= (1UL << (n))
#define BOA_BIT_ISSET(number, n) ((((number) >> (n)) & 1U) != 0)

#define boa_vmexec_pushgc(state, allow) \
    { \
        state->gcwasallowed = state->gcallowgc;  \
        state->gcallowgc = allow; \
    }

#define boa_vmexec_popgc(state) \
    { \
        state->gcallowgc = state->gcwasallowed; \
    }

#define BOA_REG_FORMABCINST(opcode, a, b, c) \
    ( \
        ((opcode) & BOA_CONFIG_OPCODESIZE) | \
        ((((int64_t)(a)) & BOA_CONFIG_ARGSIZEA) << BOA_CONFIG_ARGPOSA) | \
        ((((int64_t)(b)) & BOA_CONFIG_ARGSIZEB) << BOA_CONFIG_ARGPOSB) | \
        ((((int64_t)(c)) & BOA_CONFIG_ARGSIZEC) << BOA_CONFIG_ARGPOSC) \
    )

#define BOA_REG_FORMABXINST(opcode, a, bx) \
    ( \
        ((opcode) & BOA_CONFIG_OPCODESIZE) | \
        ((((int64_t)(a)) & BOA_CONFIG_ARGSIZEA) << BOA_CONFIG_ARGPOSA) | \
        ((((int64_t)(bx)) & BOA_CONFIG_ARGSIZEBX) << BOA_CONFIG_ARGPOSBX) \
    )

#define BOA_REG_FORMASBXINST(opcode, a, sbx) \
    ( \
        ((opcode) & BOA_CONFIG_OPCODESIZE) | \
        (((a) & BOA_CONFIG_ARGSIZEA) << BOA_CONFIG_ARGPOSA) | \
        ((labs((int64_t)(sbx)) & BOA_CONFIG_ARGSIZESBX) << BOA_CONFIG_ARGPOSSBX)) | \
        ((((((int64_t)(sbx)) < 0) ? 1 : 0) << BOA_CONFIG_FLAGPOSSBX) \
    )

typedef double BoaNumber;

enum BoaBit
{
    BOA_BITFLAG_CONSTANT = 8,
    BOA_BITFLAG_CONSTANT_BX = 12,
    BOA_BITFLAG_REGISTER = 9,
    BOA_BITFLAG_VMCONST = 16
};

enum BoaObjType
{
    BOA_OBJTYPE_STRING,
    BOA_OBJTYPE_FUNCSCRIPT,
    BOA_OBJTYPE_FUNCNATIVE,
    BOA_OBJTYPE_FUNCNATMETHOD,
    BOA_OBJTYPE_FIBER,
    BOA_OBJTYPE_MODULE,
    BOA_OBJTYPE_FUNCCLOSURE,
    BOA_OBJTYPE_CLSPROTOTYPE,
    BOA_OBJTYPE_UPVALUE,
    BOA_OBJTYPE_CLASS,
    BOA_OBJTYPE_INSTANCE,
    BOA_OBJTYPE_FUNCBOUNDMETHOD,
    BOA_OBJTYPE_ARRAY,
    BOA_OBJTYPE_VARARGARRAY,
    BOA_OBJTYPE_MAP,
    BOA_OBJTYPE_USERDATA,
    BOA_OBJTYPE_RANGE,
    BOA_OBJTYPE_FIELD,
    BOA_OBJTYPE_REFERENCE,
    BOA_OBJTYPE_EXCEPTION,
    BOA_OBJTYPE_CALLABLEFUNCTION = (BOA_OBJTYPE_FUNCCLOSURE | BOA_OBJTYPE_FUNCSCRIPT | BOA_OBJTYPE_FUNCNATIVE | BOA_OBJTYPE_FUNCNATMETHOD | BOA_OBJTYPE_FUNCBOUNDMETHOD),
};

enum BoaValType
{
    BOA_VALTYPE_NULL,
    BOA_VALTYPE_BOOL,
    BOA_VALTYPE_NUMBER,
    BOA_VALTYPE_OBJECT
};

enum BoaFuncType
{
    BOA_FUNCTYPE_REGULAR,
    BOA_FUNCTYPE_SCRIPT,
    BOA_FUNCTYPE_METHOD,
    BOA_FUNCTYPE_STATIC_METHOD,
    BOA_FUNCTYPE_CONSTRUCTOR
};

enum BoaStatusCode
{
    BOA_STATUS_OK,
    BOA_STATUS_COMPILEERROR,
    BOA_STATUS_RUNTIMEERROR,
    BOA_STATUS_INVALID
};

enum BoaAstTokType
{
    BOA_ASTTOKTYP_LINEFEED,

    /* single-character tokens */
    BOA_ASTTOKTYP_LEFTPAREN,
    BOA_ASTTOKTYP_RIGHTPAREN,
    BOA_ASTTOKTYP_LEFTBRACE,
    BOA_ASTTOKTYP_RIGHTBRACE,
    BOA_ASTTOKTYP_LEFTBRACKET,
    BOA_ASTTOKTYP_RIGHTBRACKET,
    BOA_ASTTOKTYP_COMMA,
    BOA_ASTTOKTYP_SEMICOLON,
    BOA_ASTTOKTYP_COLON,
    /* one or two character tokens */
    BOA_ASTTOKTYP_BAREQUAL,
    BOA_ASTTOKTYP_BAR,
    BOA_ASTTOKTYP_BARBAR,
    BOA_ASTTOKTYP_AMPERSANDEQUAL,
    BOA_ASTTOKTYP_AMPERSAND,
    BOA_ASTTOKTYP_AMPERSANDAMPERSAND,
    BOA_ASTTOKTYP_BANG,
    BOA_ASTTOKTYP_BANGEQUAL,
    BOA_ASTTOKTYP_EQUAL,
    BOA_ASTTOKTYP_EQUALEQUAL,
    BOA_ASTTOKTYP_GREATERTHAN,
    BOA_ASTTOKTYP_GREATEREQUAL,
    BOA_ASTTOKTYP_GREATERGREATER,
    BOA_ASTTOKTYP_LESSTHAN,
    BOA_ASTTOKTYP_LESSEQUAL,
    BOA_ASTTOKTYP_LESSLESS,
    BOA_ASTTOKTYP_PLUS,
    BOA_ASTTOKTYP_PLUSEQUAL,
    BOA_ASTTOKTYP_PLUSPLUS,
    BOA_ASTTOKTYP_MINUS,
    BOA_ASTTOKTYP_MINUSEQUAL,
    BOA_ASTTOKTYP_MINUSMINUS,
    BOA_ASTTOKTYP_STAR,
    BOA_ASTTOKTYP_STAREQUAL,
    BOA_ASTTOKTYP_STARSTAR,
    BOA_ASTTOKTYP_SLASH,
    BOA_ASTTOKTYP_SLASHEQUAL,
    BOA_ASTTOKTYP_QUESTION,
    BOA_ASTTOKTYP_QUESTIONQUESTION,
    BOA_ASTTOKTYP_PERCENT,
    BOA_ASTTOKTYP_PERCENTEQUAL,
    BOA_ASTTOKTYP_ARROW,
    BOA_ASTTOKTYP_SMALLARROW,
    BOA_ASTTOKTYP_TILDE,
    BOA_ASTTOKTYP_REFSYM,
    BOA_ASTTOKTYP_CARET,
    BOA_ASTTOKTYP_CARETEQUAL,
    BOA_ASTTOKTYP_DOT,
    BOA_ASTTOKTYP_DOTDOT,
    BOA_ASTTOKTYP_DOTDOTDOT,
    BOA_ASTTOKTYP_SHARP,
    BOA_ASTTOKTYP_SHARPEQUAL,
    /* literals */
    BOA_ASTTOKTYP_IDENTIFIER,
    BOA_ASTTOKTYP_STRING,
    BOA_ASTTOKTYP_STRTEMPLATE,
    BOA_ASTTOKTYP_NUMBER,
    /* keywords */
    BOA_ASTTOKTYP_KWEXTENDS,
    BOA_ASTTOKTYP_KWCLASS,
    BOA_ASTTOKTYP_KWELSE,
    BOA_ASTTOKTYP_KWFALSE,
    BOA_ASTTOKTYP_KWFOR,
    BOA_ASTTOKTYP_KWFUNCTION,
    BOA_ASTTOKTYP_KWIF,
    BOA_ASTTOKTYP_KWNULL,
    BOA_ASTTOKTYP_KWRETURN,
    BOA_ASTTOKTYP_KWSUPER,
    BOA_ASTTOKTYP_KWTHIS,
    BOA_ASTTOKTYP_KWTRUE,
    BOA_ASTTOKTYP_KWVAR,
    BOA_ASTTOKTYP_KWWHILE,
    BOA_ASTTOKTYP_KWCONTINUE,
    BOA_ASTTOKTYP_KWBREAK,
    BOA_ASTTOKTYP_KWNEW,
    BOA_ASTTOKTYP_KWEXPORT,
    BOA_ASTTOKTYP_KWIS,
    BOA_ASTTOKTYP_KWSTATIC,
    BOA_ASTTOKTYP_KWOPERATOR,
    BOA_ASTTOKTYP_KWIN,
    BOA_ASTTOKTYP_KWCONST,
    BOA_ASTTOKTYP_KWREF,
    BOA_ASTTOKTYP_KWTRY,
    BOA_ASTTOKTYP_KWCATCH,
    BOA_ASTTOKTYP_KWFINALLY,
    BOA_ASTTOKTYP_KWTHROW,
    BOA_ASTTOKTYP_KWSWITCH,
    BOA_ASTTOKTYP_KWCASE,
    BOA_ASTTOKTYP_KWDEFAULT,
    BOA_ASTTOKTYP_ERROR,
    BOA_ASTTOKTYP_EOF
};

enum BoaPrecedence
{
    BOA_ASTPREC_NONE,
    BOA_ASTPREC_ASSIGNMENT, /* = */
    BOA_ASTPREC_OR, /* || */
    BOA_ASTPREC_AND, /* && */
    BOA_ASTPREC_NULL, /* ?? */
    BOA_ASTPREC_BOR, /* | */
    BOA_ASTPREC_BXOR, /* ^ */
    BOA_ASTPREC_BAND, /* & */
    BOA_ASTPREC_EQUALITY, /* == != */
    BOA_ASTPREC_IS, /* is */
    BOA_ASTPREC_COMPARISON, /* < > <= >= */
    BOA_ASTPREC_RANGE, /* .. */
    BOA_ASTPREC_SHIFT, /* << >> */
    BOA_ASTPREC_TERM, /* + - */
    BOA_ASTPREC_FACTOR, /* * / % # */
    BOA_ASTPREC_COMPOUND, /* += -= *= /= ++ -- */
    BOA_ASTPREC_UNARY, /* ! - ~ */
    BOA_ASTPREC_CALL, /* . () [] */
    BOA_ASTPREC_PRIMARY
};

enum BoaAstExprType
{
    BOA_ASTEXPRTYP_LITERAL,
    BOA_ASTEXPRTYP_BINARY,
    BOA_ASTEXPRTYP_UNARY,
    BOA_ASTEXPRTYP_VARGET,
    BOA_ASTEXPRTYP_ASSIGN,
    BOA_ASTEXPRTYP_CALL,
    BOA_ASTEXPRTYP_INDEXSET,
    BOA_ASTEXPRTYP_INDEXGET,
    BOA_ASTEXPRTYP_FUNCANON,
    BOA_ASTEXPRTYP_ARRAY,
    BOA_ASTEXPRTYP_OBJECT,
    BOA_ASTEXPRTYP_SUBSCRIPT,
    BOA_ASTEXPRTYP_THIS,
    BOA_ASTEXPRTYP_SUPER,
    BOA_ASTEXPRTYP_RANGE,
    BOA_ASTEXPRTYP_TERNARY,
    BOA_ASTEXPRTYP_INTERPOLATION,
    BOA_ASTEXPRTYP_REFERENCE,
    BOA_ASTEXPRTYP_EXPRESSION,
    BOA_ASTEXPRTYP_BLOCK,
    BOA_ASTEXPRTYP_IF,
    BOA_ASTEXPRTYP_WHILE,
    BOA_ASTEXPRTYP_FOR,
    BOA_ASTEXPRTYP_VARDECL,
    BOA_ASTEXPRTYP_CONTINUE,
    BOA_ASTEXPRTYP_BREAK,
    BOA_ASTEXPRTYP_FUNCTION,
    BOA_ASTEXPRTYP_RETURN,
    BOA_ASTEXPRTYP_METHOD,
    BOA_ASTEXPRTYP_CLASS,
    BOA_ASTEXPRTYP_FIELD,
    BOA_ASTEXPRTYP_TRY,
    BOA_ASTEXPRTYP_SWITCH,
    BOA_ASTEXPRTYP_THROW
};

enum BoaInstrucType
{
    BOA_INSTYP_ABC,
    BOA_INSTYP_ABX,
    BOA_INSTYP_ASBX
};

enum BoaOpCode
{
    BOA_OPCODE_MOVE, /* R(A) := RC(B) */
    BOA_OPCODE_LOADNULL, /* R(A) := null */
    BOA_OPCODE_LOADBOOL, /* R(A) := (bool) B */
    BOA_OPCODE_MAKECLOSURE, /* R(A) := PrC[Bx] */
    BOA_OPCODE_MAKEARRAY, /* R(A) := new Array(Bx) */
    BOA_OPCODE_MAKEOBJECT, /* R(A) = new Object() */
    BOA_OPCODE_MAKERANGE, /* R(A) = new Range(RC(B), RC(C)) */
    BOA_OPCODE_RETURN, /* return R(A) */
    BOA_OPCODE_MATHADD, /* R(A) := RC(B) + RC(C) */
    BOA_OPCODE_MATHSUBTRACT, /* R(A) := RC(B) - RC(C) */
    BOA_OPCODE_MATHMULTIPLY, /* R(A) := RC(B) * RC(C) */
    BOA_OPCODE_MATHDIVIDE, /* R(A) := RC(B) / RC(C) */
    BOA_OPCODE_MATHFLOORDIVIDE, /* R(A) := floor(RC(B) / RC(C)) */
    BOA_OPCODE_MATHMOD, /* R(A) := RC(B) % RC(C) */
    BOA_OPCODE_MATHPOWER, /* R(A) := pow(RC(B), RC(C)) */
    BOA_OPCODE_MATHLEFTSHIFT, /* R(A) := RC(B) << RC(C) */
    BOA_OPCODE_MATHRIGHTSHIFT, /* R(A) := RC(B) >> RC(C) */
    BOA_OPCODE_BINXOR, /* R(A) := RC(B) ^ RC(C) */
    BOA_OPCODE_BINAND, /* R(A) := RC(B) & RC(C) */
    BOA_OPCODE_BINOR, /* R(A) := RC(B) | RC(C) */
    BOA_OPCODE_JUMP, /* PC += sBx */
    BOA_OPCODE_JUMPIFTRUE, /* if (R(A)) PC += Bx */
    BOA_OPCODE_JUMPIFFALSE, /* if (not R(A)) PC += Bx */
    BOA_OPCODE_JUMPIFNONNULL, /* if (R(A) != null) PC += Bx */
    BOA_OPCODE_JUMPIFNULL, /* if (R(A) == null) PC += Bx */
    BOA_OPCODE_EQUAL, /* R(A) := RC(B) == RC(C) */
    BOA_OPCODE_LESSTHAN, /* R(A) := RC(B) < RC(C) */
    BOA_OPCODE_LESSEQUAL, /* R(A) := RC(B) <= RC(C) */
    BOA_OPCODE_GREATERTHAN, /* R(A) := RC(B) > RC(C) */
    BOA_OPCODE_GREATEREQUAL, /* R(A) := RC(B) >= RC(C) */
    BOA_OPCODE_NEGATE, /* R(A) := -RC(B) */
    BOA_OPCODE_NOT, /* R(A) := !RC(B) */
    BOA_OPCODE_BINNOT, /* R(A) := ~RC(B) */
    BOA_OPCODE_GLOBALSET, /* G[C(A)] := RC(BX) */
    BOA_OPCODE_GLOBALGET, /* R(A) := G[C(Bx)] */
    BOA_OPCODE_UPVALUESET, /* U[A] := RC(Bx) */
    BOA_OPCODE_UPVALUEGET, /* R(A) := U[Bx] */
    BOA_OPCODE_PRIVATESET, /* P[A] := RC(Bx) */
    BOA_OPCODE_PRIVATEGET, /* R(A) := P[C(Bx)] */
    BOA_OPCODE_CALLCALLABLE, /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
    BOA_OPCODE_UPVALUECLOSE, /* close_upvalue(R(A)) */
    BOA_OPCODE_CLASSMAKE, /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
    BOA_OPCODE_CLASSPUTFIELDSTATIC, /* R(A)[C(B)] = RC(C) */
    BOA_OPCODE_CLASSPUTMETHOD, /* R(A).Methods[C(B)] = RC(C) */
    BOA_OPCODE_FIELDGET, /* R(A) = R(B)[C(C)] */
    BOA_OPCODE_CLASSGETSUPERMETHOD, /* R(A) = R(B).super[C(C)] */
    BOA_OPCODE_FIELDSET, /* R(A)[C(B)] = R(C) */
    BOA_OPCODE_IS, /* R(A) := RC(B) is G[C(C)] */
    BOA_OPCODE_INVOKE, /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
    BOA_OPCODE_INVOKESUPER, /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
    BOA_OPCODE_SUBSCRIPTGET, /* R(A) := R(A)[RC(B)] */
    BOA_OPCODE_SUBSCRIPTSET, /* R(A)[RC(B)] := R(C) */
    BOA_OPCODE_ARRAYPUSH, /* R(A)[R(A).listcount++] = RC(Bx) */
    BOA_OPCODE_OBJECTPUSH, /* R(A)[R(B)] = RC(C) */
    BOA_OPCODE_REFGLOBAL, /* R(A) := ref G(C[Bx]) */
    BOA_OPCODE_REFPRIVATE, /* R(A) := ref P(Bx) */
    BOA_OPCODE_REFLOCAL, /* R(A) := ref R(B) */
    BOA_OPCODE_REFUPVALUE, /* R(A) := ref U(Bx) */
    BOA_OPCODE_REFFIELD, /* R(A) = ref R(B)[C(C)] */
    BOA_OPCODE_REFSET, /* ref R(A) := R(B) */
    BOA_OPCODE_PUSHTRY, /* push try handler at PC + Bx */
    BOA_OPCODE_POPTRY, /* pop try handler */
    BOA_OPCODE_THROW, /* throw R(A) */
    BOA_OPCODE_RETHROW /* rethrow fiber->error */
};

enum BoaStrMode
{
    BOA_IOSTRMODE_UNDEFINED,
    BOA_IOSTRMODE_STRING,
    BOA_IOSTRMODE_FILE
};

typedef enum BoaStrMode BoaStrMode;
typedef enum BoaObjType BoaObjType;
typedef enum BoaFuncType BoaFuncType;
typedef enum BoaStatusCode BoaStatusCode;
typedef enum BoaAstTokType BoaAstTokType;
typedef enum BoaPrecedence BoaPrecedence;
typedef enum BoaValType BoaValType;
typedef enum BoaAstExprType BoaAstExprType;
typedef enum BoaInstrucType BoaInstrucType;
typedef enum BoaOpCode BoaOpCode;
typedef struct BoaAstLexer BoaAstLexer;
typedef struct BoaState BoaState;
typedef struct BoaAstParser BoaAstParser;
typedef struct BoaAstEmitter BoaAstEmitter;
typedef struct BoaResult BoaResult;
typedef struct BoaObject BoaObject;
typedef struct BoaMap BoaMap;
typedef struct BoaString BoaString;
typedef struct BoaModule BoaModule;
typedef struct BoaFiber BoaFiber;
typedef struct BoaUserdata BoaUserdata;
typedef struct BoaAstExpression BoaAstExpression;
typedef struct BoaUpvalue BoaUpvalue;
typedef struct BoaClass BoaClass;
typedef struct BoaAstLiteralValExpr BoaAstLiteralValExpr;
typedef struct BoaAstBinaryExpr BoaAstBinaryExpr;
typedef struct BoaAstUnaryExpr BoaAstUnaryExpr;
typedef struct BoaAstVarGetExpr BoaAstVarGetExpr;
typedef struct BoaAstAssignExpr BoaAstAssignExpr;
typedef struct BoaAstCallExpr BoaAstCallExpr;
typedef struct BoaAstIndexGetExpr BoaAstIndexGetExpr;
typedef struct BoaAstIndexSetExpr BoaAstIndexSetExpr;
typedef struct BoaAstFuncParamExpr BoaAstFuncParamExpr;
typedef struct BoaDynListParam BoaDynListParam;
typedef struct BoaAstLiteralArrayExpr BoaAstLiteralArrayExpr;
typedef struct BoaAstLiteralObjectExpr BoaAstLiteralObjectExpr;
typedef struct BoaAstSubscriptExpr BoaAstSubscriptExpr;
typedef struct BoaAstThisExpr BoaAstThisExpr;
typedef struct BoaAstSuperExpr BoaAstSuperExpr;
typedef struct BoaAstRangeExpr BoaAstRangeExpr;
typedef struct BoaAstTernaryExpr BoaAstTernaryExpr;
typedef struct BoaAstStrTemplateExpr BoaAstStrTemplateExpr;
typedef struct BoaAstRefExpr BoaAstRefExpr;
typedef struct BoaAstExprStmtExpr BoaAstExprStmtExpr;
typedef struct BoaAstBlockExpr BoaAstBlockExpr;
typedef struct BoaAstVarDeclExpr BoaAstVarDeclExpr;
typedef struct BoaAstIfExpr BoaAstIfExpr;
typedef struct BoaAstSwitchExpr BoaAstSwitchExpr;
typedef struct BoaAstWhileExpr BoaAstWhileExpr;
typedef struct BoaAstForExpr BoaAstForExpr;
typedef struct BoaAstContinueExpr BoaAstContinueExpr;
typedef struct BoaBreakStatement BoaBreakStatement;
typedef struct BoaAstFunctionExpr BoaAstFunctionExpr;
typedef struct BoaAstReturnExpr BoaAstReturnExpr;
typedef struct BoaAstMethodExpr BoaAstMethodExpr;
typedef struct BoaAstClassExpr BoaAstClassExpr;
typedef struct BoaAstFieldExpr BoaAstFieldExpr;
typedef struct BoaAstTryExpr BoaAstTryExpr;
typedef struct BoaAstThrowExpr BoaAstThrowExpr;
typedef struct BoaAstPrivate BoaAstPrivate;
typedef struct BoaDynListPriv BoaDynListPriv;
typedef struct BoaAstLocal BoaAstLocal;
typedef struct BoaDynListLoc BoaDynListLoc;
typedef struct BoaAstUpvalue BoaAstUpvalue;
typedef struct BoaAstCompiler BoaAstCompiler;
typedef struct BoaAstRule BoaAstRule;
typedef struct BoaEmulatedFile BoaEmulatedFile;
typedef struct BoaFileData BoaFileData;
typedef struct BoaRegexData BoaRegexData;
typedef struct BoaAstToken BoaAstToken;
typedef struct BoaDynListExpr BoaDynListExpr;
typedef struct BoaStream BoaStream;
typedef struct BoaStrBuffer BoaStrBuffer;
typedef struct BoaValue BoaValue;
typedef struct BoaVMState BoaVMState;
typedef struct BoaHandler BoaHandler;
typedef struct BoaDynListUInt BoaDynListUInt;
typedef struct BoaDynListByte BoaDynListByte;
typedef struct BoaDynListVal BoaDynListVal;
typedef struct BoaChunk BoaChunk;
typedef struct BoaTabEntry BoaTabEntry;
typedef struct BoaTable BoaTable;
typedef struct BoaStringTable BoaStringTable;
typedef struct BoaFuncScript BoaFuncScript;
typedef struct BoaFuncClosure BoaFuncClosure;
typedef struct BoaClsPrototype BoaClsPrototype;
typedef struct BoaFuncNative BoaFuncNative;
typedef struct BoaCallFrame BoaCallFrame;
typedef struct BoaInstance BoaInstance;
typedef struct BoaFuncBound BoaFuncBound;
typedef struct BoaArray BoaArray;
typedef struct BoaVarargArray BoaVarargArray;
typedef struct BoaRange BoaRange;
typedef struct BoaField BoaField;
typedef struct BoaReference BoaReference;
typedef struct BoaAstPrinter BoaAstPrinter;
typedef struct BoaConstStrings BoaConstStrings;
typedef struct BoaConfig BoaConfig;
typedef struct BoaFSStat BoaFSStat;
typedef struct BoaFSDirReader BoaFSDirReader;
typedef struct BoaFSDirItem BoaFSDirItem;
typedef struct BoaException BoaException;
typedef struct BoaUTF8Iterator BoaUTF8Iterator;
typedef struct BoaMemPoolContext BoaMemPoolContext;
typedef struct BoaChecker BoaChecker;

typedef void (*BoaErrorFn)(BoaState* state, const char* message, bool);
typedef BoaAstExpression* (*BoaParsePrefixFn)(BoaAstParser*, bool);
typedef BoaAstExpression* (*BoaParseInfixFn)(BoaAstParser*, BoaAstExpression*, bool);
typedef void (*BoaCleanupFn)(BoaState*, BoaUserdata*, bool);
typedef BoaValue (*BoaOnMapSetFn)(BoaState*, BoaMap*, BoaString*, BoaValue*);
typedef BoaValue (*BoaOnMapGetFn)(BoaState*, BoaMap*, BoaString*, BoaValue*);
typedef BoaValue (*BoaNativeFunctionFn)(BoaState*, BoaValue, size_t, BoaValue*);
typedef bool(*BoaValueIsFN)(BoaValue);
typedef void (*BoaAstCallback)(BoaAstPrinter*, BoaDynListExpr*);

struct BoaFSStat
{
    struct stat rawstbuf;
    int mode;
    int inode;
    int numlinks;
    int owneruid;
    int ownergid;
    const char* modename;
    bool isfile;
    size_t blocksize;
    size_t blockcount;
    size_t filesize;
    const time_t* tmlastchanged;
    const time_t* tmlastaccessed;
    const time_t* tmlastmodified;
};

struct BoaFSDirReader
{
    #if defined(BOA_OSPLATFORM_ISWINNT)
        HANDLE dirhandle;
        WIN32_FIND_DATA fdfile;
    #elif defined(BOA_OSPLATFORM_ISUNIXLIKE)
        DIR* dirhandle;
    #else
        void* unusedfield;
    #endif
};

struct BoaFSDirItem
{
    char name[OSLIB_CONF_OSPATHSIZE + 1];
    bool isdir;
    bool isfile;
};

struct BoaStrBuffer
{
    uint8_t isintern;
    uint8_t isshort;
    uint32_t capacity;
    uint32_t length;
    char* data;
    char sso[BOA_CONFIG_MAXSHORTSTRLENGTH+1];
};

struct BoaStream
{
    /* if file: should be closed when writer is destroyed? */
    uint8_t shouldclose;
    /* if file: should write operations be flushed via fflush()? */
    uint8_t shouldflush;
    /* if string: true if $strbuf was taken via boa_stream_take() */
    uint8_t stringtaken;
    /* was this writer instance created on stack? */
    uint8_t fromstack;
    uint8_t shortenvalues;
    uint8_t jsonmode;
    uint8_t cachedistty;
    uint8_t havecachedtty;
    size_t maxvallength;
    /* the mode that determines what writer actually does */
    BoaStrMode wrmode;
    BoaStrBuffer desthndstring;
    FILE* desthndfile;
};

struct BoaAstPrinter
{
    BoaAstCallback startfunc;
    bool fromcall;
    size_t indentlevel;
    BoaStream* printer;
    BoaState* pstate;
};

struct BoaValue
{
    BoaValType type;
    union
    {
        bool boolval;
        BoaNumber numval;
        BoaObject* obj;
    } as;
};

struct BoaDynListUInt
{
    size_t listcapacity;
    size_t listcount;
    size_t* listitems;
};

struct BoaDynListByte
{
    size_t listcapacity;
    size_t listcount;
    uint8_t* listitems;
};

struct BoaDynListVal
{
    size_t listcapacity;
    size_t listcount;
    BoaValue* listitems;
};

struct BoaDynListParam
{
    size_t listcapacity;
    size_t listcount;
    BoaAstFuncParamExpr* listitems;
};

struct BoaDynListExpr
{
    size_t listcapacity;
    size_t listcount;
    BoaAstExpression** listitems;
};

struct BoaDynListPriv
{
    size_t listcapacity;
    size_t listcount;
    BoaAstPrivate* listitems;
};

struct BoaDynListLoc
{
    size_t listcapacity;
    size_t listcount;
    BoaAstLocal* listitems;
};

struct BoaChunk
{
    uint32_t compiledcodecount;
    uint32_t capacity;
    uint64_t* compiledcodechunk;
    bool haslineinfo;
    uint32_t linecount;
    uint32_t linecapacity;
    uint16_t* lines;
    BoaDynListVal constantlist;
};

struct BoaTabEntry
{
    BoaString* entkey;
    BoaValue entvalue;
};

struct BoaTable
{
    int htcount;
    int htcapacity;
    BoaState* pstate;
    BoaTabEntry* htentries;
};

/* tombstone marker used for deleted slots in BoaStringTable (address 1 can
 * never be a real allocation) */
#define BOA_STRINGTABLE_TOMBSTONE ((BoaString*)(intptr_t)1)

/*
 * dedicated open-addressed hash set used only for interned strings.
 * unlike BoaTable, entries are plain BoaString* pointers, so lookups can
 * reject candidates by comparing the 32-bit hash before touching the data.
 */
struct BoaStringTable
{
    int count;
    int capacity;
    BoaString** entries;
};

struct BoaObject
{
    BoaObjType type;
    BoaState* pstate;
    BoaObject* next;
    bool marked;
};

struct BoaString
{
    BoaObject innerobject;
    uint32_t strhash;
    BoaStrBuffer strbuf;
};

struct BoaFuncScript
{
    BoaObject innerobject;
    BoaChunk chunk;
    BoaString* name;
    uint32_t argcount;
    uint32_t upvaluecount;
    uint64_t maxregisters;
    bool vararg;
    BoaModule* module;
};

struct BoaUpvalue
{
    BoaObject innerobject;
    BoaValue* location;
    BoaValue closed;
    BoaUpvalue* next;
};

struct BoaFuncClosure
{
    BoaObject innerobject;
    BoaFuncScript* function;
    BoaUpvalue** closureupvalueitems;
    uint32_t upvaluecount;
};

struct BoaClsPrototype
{
    BoaObject innerobject;
    BoaFuncScript* function;
    bool* local;
    uint32_t* indexes;
    uint32_t upvaluecount;
};

struct BoaFuncNative
{
    BoaObject innerobject;
    BoaNativeFunctionFn natfuncptr;
    BoaString* name;
};

struct BoaCallFrame
{
    BoaFuncScript* function;
    BoaFuncClosure* closure;
    uint64_t* ip;
    BoaValue* slots;
    BoaValue* returnaddress;
    bool resultignored;
    bool returntoc;
};

struct BoaMap
{
    BoaObject innerobject;
    BoaTable innertable;
};

struct BoaModule
{
    BoaObject innerobject;
    BoaValue returnvalue;
    BoaString* name;
    BoaValue* privatevalues;
    BoaMap* privatenames;
    uint32_t privatecount;
    BoaFuncScript* mainfunction;
    BoaFiber* mainfiber;
    bool ran;
};

struct BoaHandler
{
    uint64_t* handlerip;
    uint32_t registercount;
    uint32_t framecount;
    uint8_t errorreg;
};

struct BoaFiber
{
    BoaObject innerobject;
    BoaFiber* parent;
    BoaValue* registeritems;
    uint32_t registersallocated;
    BoaCallFrame* framevals;
    uint32_t framecapacity;
    uint32_t framecount;
    uint32_t argcount;
    BoaValue* returnaddress;
    BoaUpvalue* openupvalues;
    BoaModule* module;
    BoaValue error;
    bool muststop;
    bool catcher;
    bool caught;
    BoaHandler* handleritems;
    uint32_t handlercount;
    uint32_t handlercapacity;
};

struct BoaClass
{
    BoaObject innerobject;
    BoaString* name;
    BoaObject* mthconstructor;
    BoaTable mthtable;
    BoaTable staticstable;
    BoaClass* super;
};

struct BoaInstance
{
    BoaObject innerobject;
    BoaClass* klass;
    BoaTable fields;
};

struct BoaFuncBound
{
    BoaObject innerobject;
    BoaValue receiver;
    BoaValue method;
};

struct BoaArray
{
    BoaObject innerobject;
    BoaDynListVal innerlist;
};

struct BoaVarargArray
{
    BoaArray innerarray;
};

struct BoaUserdata
{
    BoaObject innerobject;
    void* data;
    uint32_t size;
    BoaCleanupFn oncleanupfn;
};

struct BoaRange
{
    BoaObject innerobject;
    BoaNumber from;
    BoaNumber to;
};

struct BoaField
{
    BoaObject innerobject;
    BoaObject* getter;
    BoaObject* setter;
};

struct BoaReference
{
    BoaObject innerobject;
    BoaValue* slot;
};

struct BoaException
{
    BoaObject innerobject;
    BoaClass* baseclass;
    BoaValue message;
};

struct BoaVMState
{
    BoaObject* objects;
    BoaStringTable storedstrings;
    BoaMap* modules;
    BoaMap* globals;
    BoaFiber* fiber;
    /* For garbage collection */
    uint32_t gcgraycount;
    uint32_t gcgraycapacity;
    BoaObject** gcgraystack;
    /* exec */
    BoaCallFrame* frame;
    BoaChunk* currentchunk;
    BoaValue* vmregisteritems;
    BoaValue* vmconstantvalues;
    BoaValue* vmprivatevalues;
    BoaUpvalue** vmupvalueitems;
    uint64_t* ip;
    uint64_t instruction;
};

/*
* contains pre-allocated strings that are often used, to reduce memory use.
* you should *never* directly modify these objects; clone them instead.
*/
struct BoaConstStrings
{
    BoaString* strempty;
    BoaString* strthis;
    BoaString* strtrue;
    BoaString* strfalse;
    BoaString* strtostring;
    BoaString* strconstructor;
    BoaString* strsuper;
    BoaString* strjoin;
    BoaString* striterator;
    BoaString* stritervalue;
    BoaString* struserdatafield;
    BoaString* strnull;
    BoaString* stropequal;
    BoaString* stropnot;
    BoaString* stropplus;
    BoaString* stropminus;
    BoaString* stropmodulo;
    BoaString* stropdivide;
    BoaString* stropmultiply;
    BoaString* stroplessthan;
    BoaString* stropgreaterthan;
    BoaString* stroplessequal;
    BoaString* stropgreaterequal;
    BoaString* stropindex;
    BoaString* stropfloordiv;
    BoaString* stroppower;  
    BoaString* strcallbackforeach;
    BoaString* strcallbacksort;
    BoaString* strdots;
};

struct BoaConfig
{
    /* use memory pool? defaults to true. can be toggled with '-m' / '--usemalloc' */
    bool mempooldisable;
    bool mempoolforcegeneric;
    /* should the AST be dumped? */
    bool dumpast;
    /* should the interpreter stop after dumping the AST? */
    bool quitafterdump;
    /* should execution be traced? */
    bool traceexecution;
    /* (requires traceexecution) but only print instructions? */
    bool traceinstsonly;
    /* not currently in use; true if the interpreter is in REPL mode */
    bool isreplmode;
    /* (requires traceexecution) has a destination file for trace output been defined? */
    bool havedesttrace;
    /* where text for $traceexecution goes. defaults to stderr. if $havedesttrace, then thats what it'll point to. */
    BoaStream* desttrace;
};

struct BoaState
{
    BoaConfig config;

    BoaVMState vmstate;

    int64_t bytesallocated;
    int64_t gcnextgc;
    bool gcallowgc;
    bool gcwasallowed;
    BoaStream* streamstdout;
    BoaStream* streamstderr;
    BoaErrorFn printerrmessagefn;
    BoaValue* rootvalues;
    size_t rootcount;
    size_t rootcapacity;
    BoaAstLexer* activelexer;
    BoaAstParser* activeparser;
    BoaAstEmitter* activeemitter;
    bool haderror;
    BoaFuncScript* apifunction;
    BoaString* apiname;
    /* Mental note: */
    /* When adding another class here, DO NOT forget to mark it or it will be GC-ed */
    BoaClass* stdclassclass;
    BoaClass* stdobjectclass;
    BoaClass* stdnullclass;
    BoaClass* stdclassnumber;
    BoaClass* stdclassstring;
    BoaClass* stdclassbool;
    BoaClass* stdclassfunction;
    BoaClass* stdclassfiber;
    BoaClass* stdclassmodule;
    BoaClass* stdclassarray;
    BoaClass* stdclassmap;
    BoaClass* stdclassrange;
    BoaClass* stdclassregex;
    struct {
        BoaException* stdexception;
        BoaException* stdioerror;
        BoaException* stdargumenterror;
    } exceptions;
    BoaModule* lastmodule;
    BoaConstStrings strings;
};

struct BoaResult
{
    BoaStatusCode type;
    BoaValue result;
};

struct BoaAstToken
{
    const char* start;
    BoaAstTokType type;
    size_t length;
    size_t line;
    BoaValue tokvalue;
};

struct BoaAstPrivate
{
    bool initialized;
    bool constant;
};

struct BoaAstLocal
{
    const char* name;
    size_t length;
    int depth;
    bool captured;
    bool constant;
    uint32_t reg;
};

struct BoaAstUpvalue
{
    uint32_t index;
    bool islocal;
};

struct BoaAstCompiler
{
    BoaDynListLoc locals;
    int scopedepth;
    BoaFuncScript* function;
    BoaFuncType type;
    BoaAstUpvalue compiledupvalueitems[BOA_CONFIG_UINT8COUNT];
    uint64_t registersused;
    BoaAstCompiler* enclosing;
    bool skipreturn;
    size_t loopdepth;
    size_t switchdepth;
};

struct BoaAstEmitter
{
    BoaState* pstate;
    BoaChunk* chunk;
    BoaAstCompiler* compiler;
    size_t lastline;
    size_t loopstart;
    BoaDynListPriv privlist;
    BoaDynListUInt breaks;
    BoaDynListUInt continues;
    BoaModule* module;
    BoaString* classname;
    uint32_t classregister;
    bool classhassuper;
    int emitreference;
};

struct BoaAstRule
{
    BoaParsePrefixFn prefix;
    BoaParseInfixFn infix;
    BoaPrecedence precedence;
};

struct BoaAstParser
{
    BoaState* pstate;
    bool haderror;
    bool panicmode;
    BoaAstToken previoustoken;
    BoaAstToken currenttoken;
    BoaAstCompiler* compiler;
    uint32_t exprrootcnt;
    uint32_t stmtrootcnt;
};

struct BoaEmulatedFile
{
    const char* source;
    size_t position;
};

struct BoaAstLexer
{
    size_t sourcecurrentline;
    const char* sourcedatastart;
    const char* sourcedatacurrent;
    const char* sourcefilename;
    BoaState* pstate;
    size_t bracevalues[BOA_CONFIG_TPLSTRINGNMAXNESTING];
    size_t bracecount;
    bool haderror;
};

struct BoaAstExpression
{
    BoaAstExprType type;
    size_t line;
};

struct BoaAstLiteralValExpr
{
    BoaAstExpression exprbase;
    BoaValue value;
};

struct BoaAstBinaryExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* left;
    BoaAstExpression* right;
    BoaAstTokType op;
    bool ignoreleft;
};

struct BoaAstUnaryExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* right;
    BoaAstTokType op;
};

struct BoaAstVarGetExpr
{
    BoaAstExpression exprbase;
    const char* name;
    size_t length;
};

struct BoaAstAssignExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* to;
    BoaAstExpression* value;
};

struct BoaAstCallExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* excallee;
    BoaDynListExpr callargs;
    BoaAstExpression* init;
};

struct BoaAstIndexGetExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* where;
    const char* name;
    size_t length;
    int jump;
    bool isdot;
    bool ignoreemit;
    bool ignoreresult;
};

struct BoaAstIndexSetExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* where;
    const char* name;
    size_t length;
    BoaAstExpression* value;
    bool isdot;
};

struct BoaAstFuncParamExpr
{
    const char* name;
    size_t length;
    uint32_t reg;
    BoaAstExpression* defaultval;
};

struct BoaAstLiteralArrayExpr
{
    BoaAstExpression exprbase;
    BoaDynListExpr exvalues;
};

struct BoaAstLiteralObjectExpr
{
    BoaAstExpression exprbase;
    BoaDynListVal objexkeys;
    BoaDynListExpr objexvalues;
};

struct BoaAstSubscriptExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* array;
    BoaAstExpression* index;
};

struct BoaAstThisExpr
{
    BoaAstExpression exprbase;
};

struct BoaAstSuperExpr
{
    BoaAstExpression exprbase;
    BoaString* methodname;
    bool ignoreemit;
    bool ignoreresult;
};

struct BoaAstRangeExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* from;
    BoaAstExpression* to;
};

struct BoaAstTernaryExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* condition;
    BoaAstExpression* branchif;
    BoaAstExpression* branchelse;
};

struct BoaAstStrTemplateExpr
{
    BoaAstExpression exprbase;
    BoaDynListExpr expressions;
};

struct BoaAstRefExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* to;
};

struct BoaAstExprStmtExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* exvalue;
};

struct BoaAstBlockExpr
{
    BoaAstExpression exprbase;
    BoaDynListExpr statements;
};

struct BoaAstVarDeclExpr
{
    BoaAstExpression exprbase;
    const char* name;
    size_t length;
    bool isconstant;
    BoaAstExpression* init;
};

struct BoaAstIfExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* condition;
    BoaAstExpression* branchif;
    BoaAstExpression* branchelse;
    BoaDynListExpr* elseifcondlist;
    BoaDynListExpr* branchelseiflist;
};

struct BoaAstSwitchExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* condition;
    BoaDynListExpr caseconditions;
    BoaDynListExpr casebodies;
};

struct BoaAstWhileExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* condition;
    BoaAstExpression* body;
};

struct BoaAstForExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* init;
    BoaAstExpression* var;
    BoaAstExpression* condition;
    BoaAstExpression* increment;
    BoaAstExpression* body;
    bool iscstyle;
};

struct BoaAstContinueExpr
{
    BoaAstExpression exprbase;
};

struct BoaBreakStatement
{
    BoaAstExpression exprbase;
};

struct BoaAstFunctionExpr
{
    BoaAstExpression exprbase;
    const char* name;
    size_t length;
    BoaDynListParam parameters;
    BoaAstExpression* body;
    bool exported;
};

struct BoaAstReturnExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* exvalue;
};

struct BoaAstMethodExpr
{
    BoaAstExpression exprbase;
    BoaString* name;
    BoaDynListParam parameters;
    BoaAstExpression* body;
    bool isstatic;
};

struct BoaAstClassExpr
{
    BoaAstExpression exprbase;
    BoaString* name;
    BoaString* parent;
    BoaDynListExpr staticfields;
};

struct BoaAstFieldExpr
{
    BoaAstExpression exprbase;
    BoaString* name;
    BoaAstExpression* getter;
    BoaAstExpression* setter;
    bool isstatic;
};

struct BoaAstTryExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* tryblock;
    BoaAstExpression* catchblock;
    BoaAstExpression* finallyblock;
    const char* catchvarstr;
    size_t catchvarlen;
};

struct BoaAstThrowExpr
{
    BoaAstExpression exprbase;
    BoaAstExpression* exvalue;
};

struct BoaFileData
{
    BoaInstance innerobject;
    char* path;
    BoaStream* fdhandle;
};

struct BoaRegexData
{
    BoaInstance innerobject;
    MRXContext rxctx;
};

struct BoaUTF8Iterator
{
    /*input string pointer */
    const char* plainstr;

    /* input string length */
    uint32_t plainlen;

    /* the codepoint, or char */
    uint32_t codepoint;

    /* character size in bytes */
    uint8_t charsize;

    /* current character position */
    uint32_t currpos;

    /* next character position */
    uint32_t nextpos;

    /* number of counter characters currently */
    uint32_t currcount;
};

struct BoaChecker
{
    BoaState* state;
    const char* name;
    size_t argc;
    BoaValue* args;
};

struct BoaMemPoolContext
{
    BoaConfig* config;
    void* mspctx;
};

#include "prot.inc"

#if defined(__GNUC__)
    #define BOA_ATTRIB(...) __attribute__(__VA_ARGS__)
#else
    #define BOA_ATTRIB(...)
#endif

#define BOA_CHECK_INIT(statevar, chk, namestr, argcvar, argsvar) \
    (chk)->state = statevar; \
    (chk)->name = namestr; \
    (chk)->argc = argcvar; \
    (chk)->args = argsvar;

#define BOA_CHECK_REQUIREARGS(chk, cnt) \
    if(BOA_UNLIKELY((chk)->argc < cnt)) \
    { \
        return boa_vm_raiseexception((chk)->state, (chk)->state->exceptions.stdargumenterror, "function %s expected %d arguments, got %d instead", (chk)->name, cnt, (chk)->argc); \
    }

#define BOA_CHECK_CHECKARGTYPE(chk, idx, asfn) \
    if(BOA_LIKELY(((chk)->argc != 0) && ((int)((chk)->argc) >= ((int)(idx))))) \
    { \
        if(BOA_UNLIKELY(!asfn((chk)->args[idx]))) \
        { \
            return boa_vm_raiseexception((chk)->state, (chk)->state->exceptions.stdargumenterror, "function %s expected argument #%d to be a %s, but got %s instead", (chk)->name, idx, boa_value_typenamefromfn(asfn), boa_value_valtypename((chk)->args[idx])); \
        } \
    }

/* if any global variables need to be declared, declare them here. */
jmp_buf g_vmglobaljumpbuf = {};
static BoaMemPoolContext g_mspcontext;
BOA_FORCEINLINE size_t boa_string_getlength(BoaString* string);
BOA_FORCEINLINE char* boa_string_getdata(BoaString* string);

void boa_sysmem_poolinit(BoaConfig* cfg)
{
    g_mspcontext.config = cfg;
    g_mspcontext.mspctx = NULL;
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        g_mspcontext.mspctx = mempool_createpool(g_mspcontext.config->mempoolforcegeneric);
    }
}

void boa_sysmem_pooldestroy()
{
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        mempool_destroypool(g_mspcontext.mspctx);
    }
}

void* boa_sysmem_malloc(size_t sz)
{
    void* p;
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        p = (void*)mempool_usermalloc(g_mspcontext.mspctx, sz);
    }
    else
    {
        p = (void*)malloc(sz);
    }
    return p;
}

void* boa_sysmem_realloc(void* p, size_t nsz)
{
    void* retp;
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        if(p == NULL)
        {
            return boa_sysmem_malloc(nsz);
        }
        retp = (void*)mempool_userrealloc(g_mspcontext.mspctx, p, nsz);
    }
    else
    {
        retp = (void*)realloc(p, nsz);
    }
    return retp;
}

void* boa_sysmem_calloc(size_t count, size_t typsize)
{
    void* p;
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        p = (void*)mempool_usermalloc(g_mspcontext.mspctx, (count * typsize));
        memset(p, 0, (count * typsize));
    }
    else
    {
        p = (void*)calloc(count, typsize);
    }
    return p;
}

void boa_sysmem_free(void* ptr)
{
    if(BOA_LIKELY(!g_mspcontext.config->mempooldisable))
    {
        mempool_userfree(g_mspcontext.mspctx, ptr);
    }
    else
    {
        free(ptr);
    }
}

static bool fslib_diropen(BoaFSDirReader* rd, const char* path)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        if((rd->dirhandle = opendir(path)) == NULL)
        {
            return false;
        }
        return true;
    #else
        /*
        * windows' directory reading api expects a glob pattern.
        * i wish i was making this up!
        */
        enum { kExtra = 5 };
        bool b;
        size_t pslen;
        size_t buflen;
        char* winsillypath;
        b = false;
        pslen = strlen(path);
        buflen = (pslen + kExtra);
        winsillypath = (char*)boa_sysmem_malloc(buflen);
        if(winsillypath == NULL)
        {
            return false;
        }
        memset(winsillypath, 0, buflen);
        strcat(winsillypath, path);
        strcat(winsillypath, "\\*.*");
        fprintf(stderr, "sillypath=%s\n", winsillypath);
        rd->dirhandle = FindFirstFile(winsillypath, &rd->fdfile);
        if(rd->dirhandle != INVALID_HANDLE_VALUE)
        {
            b = true;
        }
        boa_sysmem_free(winsillypath);
        return b;
    #endif
}

static bool fslib_dirread(BoaFSDirReader* rd, BoaFSDirItem* itm)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        struct dirent* ent;
    #endif
    itm->isdir = false;
    itm->isfile = false;
    memset(itm->name, 0, OSLIB_CONF_OSPATHSIZE);
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        if((ent = readdir((DIR*)(rd->dirhandle))) == NULL)
        {
            return false;
        }
        if(ent->d_type == DT_DIR)
        {
            itm->isdir = true;
        }
        if(ent->d_type == DT_REG)
        {
            itm->isfile = true;
        }
        strcpy(itm->name, ent->d_name);
        return true;
    #else
        if(FindNextFile(rd->dirhandle, &rd->fdfile))
        {
            if((rd->fdfile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                itm->isdir = true;
            }
            else
            {
                itm->isfile = true;
            }
            strcpy(itm->name, rd->fdfile.cFileName);
            return true;            
        }
    #endif
    return false;
}

static bool fslib_dirclose(BoaFSDirReader* rd)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        closedir(rd->dirhandle);
    #else
        FindClose(rd->dirhandle);
    #endif
    return false;
}

BoaResult boa_result_make(BoaStatusCode type, BoaValue result)
{
    BoaResult r;
    r.type = type;
    r.result = result;
    return r;
}

/* Bounds check when inserting (pos <= len are valid) */
#define boa_strbuf_boundscheckinsert(sb, pos) boa_strbufutil_callboundscheckinsert(sb, pos, __FILE__, __LINE__)
#define boa_strbuf_boundscheckreadrange(sb, start, len) boa_strbufutil_callboundscheckreadrange(sb, start, len, __FILE__, __LINE__)

size_t boa_strbufutil_rndup2pow64(uint64_t x)
{
    /* long long >=64 bits guaranteed in C99 */
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    ++x;
    return x;
}

/*
 * Replaces `sep` with \0 in str
 * Returns number of occurances of `sep` character in `str`
 * Stores `nptrs` pointers in `ptrs`
 */
size_t boa_strbufutil_splitstr(char* str, char sep, char** ptrs, size_t nptrs)
{
    size_t n;
    n = 1;
    if(*str == '\0')
    {
        return 0;
    }
    if(nptrs > 0)
    {
        ptrs[0] = str;
    }
    while((str = strchr(str, sep)) != NULL)
    {
        *str = '\0';
        str++;
        if(n < nptrs)
        {
            ptrs[n] = str;
        }
        n++;
    }
    return n;
}

/*
 * Replace one char with another in a string. Return number of replacements made
 */
size_t boa_strbufutil_charreplace(char* str, char from, char to)
{
    size_t n;
    n = 0;
    for(; *str; str++)
    {
        if(*str == from)
        {
            n++;
            *str = to;
        }
    }
    return n;
}

/*
 * Reverse a string region
 */
void boa_strbufutil_reverseregion(char* str, size_t length)
{
    char* a;
    char* b;
    char tmp;
    a = str;
    b = str + length - 1;
    while(a < b)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
        a++;
        b--;
    }
}

bool boa_strbufutil_isallspace(const char* s)
{
    int i;
    for(i = 0; s[i] != '\0' && isspace((int)s[i]); i++)
    {
    }
    return (s[i] == '\0');
}

char* boa_strbufutil_nextspace(char* s)
{
    while(*s != '\0' && isspace((int)*s))
    {
        s++;
    }
    return (*s == '\0' ? NULL : s);
}

/*
 * Strip whitespace the the start and end of a string.
 * Strips whitepace from the end of the string with \0, and returns pointer to
 * first non-whitespace character
 */
char* boa_strbufutil_trim(char* str)
{
    /* Work backwards */
    char* end;
    end = str + strlen(str);
    while(end > str && isspace((int)*(end - 1)))
    {
        end--;
    }
    *end = '\0';
    /* Work forwards: don't need start < len because will hit \0 */
    while(isspace((int)*str))
    {
        str++;
    }
    return str;
}

/*
 * Removes \r and \n from the ends of a string and returns the new length
 */
size_t boa_strbufutil_chomp(char* str, size_t len)
{
    while(len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n'))
    {
        len--;
    }
    str[len] = '\0';
    return len;
}

/*
 * Returns count
 */
size_t boa_strbufutil_countchar(const char* str, char c)
{
    size_t count;
    count = 0;
    while((str = strchr(str, c)) != NULL)
    {
        str++;
        count++;
    }
    return count;
}

/*
 * Returns the number of strings resulting from the split
 */
size_t boa_strbufutil_split(const char* splitat, const char* sourcetxt, char*** result)
{
    size_t i;
    size_t slen;
    size_t count;
    size_t splitlen;
    size_t txtlen;
    char** arr;
    const char* find;
    const char* plastpos;
    splitlen = strlen(splitat);
    txtlen = strlen(sourcetxt);
    /* result is temporarily held here */
    if(splitlen == 0)
    {
        /* Special case */
        if(txtlen == 0)
        {
            *result = NULL;
            return 0;
        }
        else
        {
            arr = (char**)boa_sysmem_malloc(txtlen * sizeof(char*));
            for(i = 0; i < txtlen; i++)
            {
                arr[i] = (char*)boa_sysmem_malloc(2 * sizeof(char));
                arr[i][0] = sourcetxt[i];
                arr[i][1] = '\0';
            }
            *result = arr;
            return txtlen;
        }
    }
    find = sourcetxt;
    /* must have at least one item */
    count = 1;
    for(; (find = strstr(find, splitat)) != NULL; count++, find += splitlen)
    {
    }
    /* Create return array */
    arr = (char**)boa_sysmem_malloc(count * sizeof(char*));
    count = 0;
    plastpos = sourcetxt;
    while((find = strstr(plastpos, splitat)) != NULL)
    {
        slen = (size_t)(find - plastpos);
        arr[count] = (char*)boa_sysmem_malloc((slen + 1) * sizeof(char));
        strncpy(arr[count], plastpos, slen);
        arr[count][slen] = '\0';
        count++;
        plastpos = find + splitlen;
    }
    /* Copy last item */
    slen = (size_t)(sourcetxt + txtlen - plastpos);
    arr[count] = (char*)boa_sysmem_malloc((slen + 1) * sizeof(char));
    if(count == 0)
    {
        strcpy(arr[count], sourcetxt);
    }
    else
    {
        strncpy(arr[count], plastpos, slen);
    }
    arr[count][slen] = '\0';
    count++;
    *result = arr;
    return count;
}

void boa_strbufutil_callboundscheckinsert(BoaStrBuffer* sb, size_t pos, const char* srcfile, int srcline)
{
    if(pos > sb->length)
    {
        fprintf(stderr, "%s:%i: - out of bounds error [index: %ld, numofbits: %ld]\n", srcfile, srcline, (long)pos, (long)sb->length);
        errno = EDOM;
        abort();
    }
}

/* Bounds check when reading a range (start+len < strlen is valid) */
void boa_strbufutil_callboundscheckreadrange(BoaStrBuffer* sb, size_t start, size_t len, const char* srcfile, int srcline)
{
    if(start + len > sb->length)
    {
        fprintf(stderr, "%s:%i: - out of bounds error [start: %ld; length: %ld; strlen: %ld; buf:%.*s%s]\n", srcfile, srcline, (long)start, (long)len, (long)sb->length, (int)STRBUF_MIN(5, sb->length), boa_strbuf_data(sb), sb->length > 5 ? "..." : "");
        errno = EDOM;
        abort();
    }
}

/* via: codereview.stackexchange.com/q/274832 */
void boa_strbufutil_faststrncat(char* dest, const char* src, size_t* size)
{
    if(dest && src && size)
    {
        while((dest[*size] = *src++))
        {
            *size += 1;
        }
    }
}

size_t boa_strbufutil_strreplace1(char** str, size_t selflen, const char* findstr, size_t findlen, const char* substr, size_t sublen)
{
    size_t i;
    size_t x;
    size_t oldcount;
    char* buff;
    const char* temp;
    (void)selflen;
    oldcount = 0;
    temp = (const char*)(*str);
    for(i = 0; temp[i] != '\0'; ++i)
    {
        if(strstr((const char*)&temp[i], findstr) == &temp[i])
        {
            oldcount++;
            i += findlen - 1;
        }
    }
    buff = (char*)boa_sysmem_malloc((i + oldcount * (sublen - findlen) + 1) * sizeof(char));
    if(!buff)
    {
        perror("bad allocation\n");
        exit(EXIT_FAILURE);
    }
    i = 0;
    while(*temp)
    {
        if(strstr(temp, findstr) == temp)
        {
            x = 0;
            boa_strbufutil_faststrncat(&buff[i], substr, &x);
            i += sublen;
            temp += findlen;
        }
        else
        {
            buff[i++] = *temp++;
        }
    }
    boa_sysmem_free(*str);
    *str = (char*)boa_sysmem_malloc((i + 1) * sizeof(char));
    if(!(*str))
    {
        perror("bad allocation\n");
        exit(EXIT_FAILURE);
    }
    i = 0;
    boa_strbufutil_faststrncat(*str, (const char*)buff, &i);
    boa_sysmem_free(buff);
    return i;
}

size_t boa_strbufutil_strrepcount(const char* str, size_t slen, const char* findstr, size_t findlen, size_t sublen)
{
    size_t i;
    size_t count;
    size_t total;
    (void)total;
    total = slen;
    count = 0;
    for(i = 0; i < slen; i++)
    {
        if(str[i] == findstr[0])
        {
            if((i + findlen) < slen)
            {
                if(memcmp(&str[i], findstr, findlen) == 0)
                {
                    count++;
                    total += sublen;
                }
            }
        }
    }
    if(count == 0)
    {
        return 0;
    }
    return total + 0;
}

/* via: stackoverflow.com/a/32413923 */
void boa_strbufutil_strreplace2(char* target, size_t tgtlen, const char* findstr, size_t findlen, const char* substr, size_t sublen)
{
    const char* p;
    const char* tmp;
    char* inspoint;
    char buffer[1024];
    (void)tgtlen;
    memset(buffer, 0, sizeof(buffer));
    inspoint = &buffer[0];
    tmp = target;
    while(true)
    {
        p = strstr(tmp, findstr);
        /* walked past last occurrence of findstr; copy remaining part */
        if(p == NULL)
        {
            strcpy(inspoint, tmp);
            break;
        }
        /* copy part before findstr */
        memcpy(inspoint, tmp, p - tmp);
        inspoint += p - tmp;
        /* copy substr string */
        memcpy(inspoint, substr, sublen);
        inspoint += sublen;
        /* adjust pointers, move on */
        tmp = p + findlen;
    }
    /* write altered string back to target */
    strcpy(target, buffer);
}

bool boa_strbufutil_inpreplhelper(char* dest, const char* src, size_t srclen, int findme, const char* substr, size_t sublen, size_t maxlen, size_t* dlen)
{
    /* ch(ar) at pos(ition) */
    int chatpos;
    /* printf("'%p' '%s' %c\n", dest, src, findme); */
    if(*src == findme)
    {
        if(sublen > maxlen)
        {
            return false;
        }
        if(!boa_strbufutil_inpreplhelper(dest + sublen, src + 1, srclen, findme, substr, sublen, maxlen - sublen, dlen))
        {
            return false;
        }
        memcpy(dest, substr, sublen);
        *dlen += sublen;
        return true;
    }
    if(maxlen == 0)
    {
        return false;
    }
    chatpos = *src;
    if(*src)
    {
        *dlen += 1;
        if(!boa_strbufutil_inpreplhelper(dest + 1, src + 1, srclen, findme, substr, sublen, maxlen - 1, dlen))
        {
            return false;
        }
    }
    *dest = chatpos;
    return true;
}

size_t boa_strbufutil_inpreplace(char* target, size_t tgtlen, int findme, const char* substr, size_t sublen, size_t maxlen)
{
    size_t nlen;
    if(findme == 0)
    {
        return 0;
    }
    if(maxlen == 0)
    {
        return 0;
    }
    if(*substr == 0)
    {
        /* Insure target does not shrink. */
        return 0;
    }
    nlen = 0;
    boa_strbufutil_inpreplhelper(target, target, tgtlen, findme, substr, sublen, maxlen - 1, &nlen);
    return nlen;
}

BoaStrBuffer* boa_strbuf_makelongfromptr(BoaStrBuffer* sb, size_t len)
{
    /* fprintf(stderr, "in makelong...\n"); */
    sb->isshort = false;
    sb->isintern = false;
    sb->length = 0;
#if 0
        sb->capacity = boa_strbufutil_rndup2pow64(len + 1);
#else
    sb->capacity = (len + 1);
#endif
    sb->data = (char*)boa_sysmem_malloc(sb->capacity);
    if(!sb->data)
    {
        return NULL;
    }
    sb->data[0] = '\0';
    return sb;
}

bool boa_strbuf_initbasicempty(BoaStrBuffer* sb, size_t len, bool isintern, bool preallocated)
{
    memset(sb, 0, sizeof(BoaStrBuffer));
    sb->isintern = isintern;
    sb->isshort = true;
    sb->capacity = BOA_CONFIG_MAXSHORTSTRLENGTH;
    sb->length = 0;
    sb->data = sb->sso;
    sb->data[0] = '\0';
    if(preallocated)
    {
        if (len > BOA_CONFIG_MAXSHORTSTRLENGTH)
        {
            sb->isshort = false;
            sb->capacity = len;
            sb->data = NULL;
        }
        return true;
    }
    if(len > BOA_CONFIG_MAXSHORTSTRLENGTH)
    {
        boa_strbuf_resize(sb, len);
    }
    return true;
}

bool boa_strbuf_makebasicemptystack(BoaStrBuffer* sb, const char* str, size_t len)
{
    boa_strbuf_initbasicempty(sb, len, false, false);
    boa_strbuf_appendstrn(sb, str, len);
    return true;
}

BoaStrBuffer* boa_strbuf_makebasicempty(const char* str, size_t len)
{
    BoaStrBuffer* sb;
    sb = (BoaStrBuffer*)boa_sysmem_malloc(sizeof(BoaStrBuffer));
    if(!sb)
    {
        return NULL;
    }
    if(!boa_strbuf_initbasicempty(sb, len, false, false))
    {
        return NULL;
    }
    boa_strbuf_appendstrn(sb, str, len);
    return sb;
}

bool boa_strbuf_destroyfromstack(BoaStrBuffer* sb)
{
    if(!sb->isintern && !sb->isshort)
    {
        boa_sysmem_free(sb->data);
    }
    return true;
}

bool boa_strbuf_destroy(BoaStrBuffer* sb)
{
    boa_strbuf_destroyfromstack(sb);
    boa_sysmem_free(sb);
    return true;
}

/* Clear the content of an existing BoaStrBuffer (sets size to 0) */
void boa_strbuf_reset(BoaStrBuffer* sb)
{
    if(sb->data)
    {
        memset(sb->data, 0, sb->length);
    }
    sb->length = 0;
}

/* Ensure capacity for len characters plus '\0' character - exits on FAILURE */
bool boa_strbuf_ensurecapacity(BoaStrBuffer* sb, size_t len)
{
    char* ptr;
    /* for nul byte */
    len++;
    if(sb->capacity < len)
    {
        size_t newcap = boa_strbufutil_rndup2pow64(len);
        if (sb->isshort)
        {
            ptr = (char*)boa_sysmem_malloc(newcap);
            if (ptr == NULL)
            {
                fprintf(stderr, "[%s:%i] out of memory: tried to allocate %ld bytes\n", __FILE__, __LINE__, (long)newcap);
                return false;
            }
            memcpy(ptr, sb->data, sb->length + 1);
            sb->isshort = false;
        }
        else
        {
            ptr = (char*)boa_sysmem_realloc(sb->data, newcap);
            if(ptr == NULL)
            {
                fprintf(stderr, "[%s:%i] out of memory: tried to allocate %ld bytes\n", __FILE__, __LINE__, (long)newcap);
                return false;
            }
        }
        sb->data = ptr;
        sb->capacity = (uint32_t)newcap;
    }
    return true;
}

/*
 * Resize the buffer to have capacity to hold a string of length newlen
 * (+ a null terminating character).  Can also be used to downsize the buffer's
 * memory usage.  Returns 1 on success, 0 on failure.
 */
bool boa_strbuf_resize(BoaStrBuffer* sb, size_t newlen)
{
    return boa_strbuf_ensurecapacity(sb, newlen);
}

bool boa_strbuf_setlength(BoaStrBuffer* sb, size_t len)
{
    sb->length = len;
    return true;
}

bool boa_strbuf_setdata(BoaStrBuffer* sb, char* str)
{
    sb->isshort = false;
    sb->data = str;
    return true;
}

size_t boa_strbuf_length(BoaStrBuffer* sb)
{
    return sb->length;
}

const char* boa_strbuf_data(BoaStrBuffer* sb)
{
    return sb->data;
}

#define boa_strbuf_mutdata(sb) ((sb)->data)

int boa_strbuf_get(BoaStrBuffer* sb, size_t idx)
{
    return sb->data[idx];
}

bool boa_strbuf_containschar(BoaStrBuffer* sb, char ch)
{
    size_t i;
    const char* data;
    data = boa_strbuf_data(sb);
    for(i = 0; i < sb->length; i++)
    {
        if(data[i] == ch)
        {
            return true;
        }
    }
    return false;
}

bool boa_strbuf_fullreplace(BoaStrBuffer* from, BoaStrBuffer* dest, const char* findmestr, size_t findmelen, const char* repwithstr, size_t repwithlen)
{
    size_t i;
    if((from->length == 0 && findmelen == 0) || from->length == 0 || findmelen == 0)
    {
        return false;
    }
    for(i = 0; i < from->length; i++)
    {
        if(memcmp(from->data + i, findmestr, findmelen) == 0)
        {
            if(findmelen > 0)
            {
                boa_strbuf_appendstrn(dest, repwithstr, repwithlen);
            }
            i += findmelen - 1;
        }
        else
        {
            boa_strbuf_appendchar(dest, from->data[i]);
        }
    }
    return true;
}

bool boa_strbuf_charreplace(BoaStrBuffer* sb, int findme, const char* substr, size_t sublen)
{
    size_t i;
    size_t nlen;
    size_t needed;
    char* data;
    needed = sb->capacity;
    data = boa_strbuf_mutdata(sb);
    for(i = 0; i < sb->length; i++)
    {
        if(data[i] == findme)
        {
            needed += sublen;
        }
    }
    if(!boa_strbuf_ensurecapacity(sb, needed + 1))
    {
        return false;
    }
    data = boa_strbuf_mutdata(sb);
    nlen = boa_strbufutil_inpreplace(data, sb->length, findme, substr, sublen, sb->capacity);
    sb->length = nlen;
    return true;
}

/* Set string buffer to contain a given string */
bool boa_strbuf_set(BoaStrBuffer* sb, size_t idx, int b)
{
    char* data;
    boa_strbuf_ensurecapacity(sb, idx);
    data = boa_strbuf_mutdata(sb);
    data[idx] = b;
    return true;
}

/* Add a character to the end of this BoaStrBuffer */
bool boa_strbuf_appendchar(BoaStrBuffer* sb, int c)
{
    char* data;
    boa_strbuf_ensurecapacity(sb, sb->length + 1);
    data = boa_strbuf_mutdata(sb);
    data[sb->length] = c;
    data[sb->length + 1] = '\0';
    sb->length++;
    return true;
}

/*
 * Copy N characters from a character array to the end of this BoaStrBuffer
 * strlen(str) must be >= len
 */
bool boa_strbuf_appendstrn(BoaStrBuffer* sb, const char* str, size_t len)
{
    int epos;
    char* data;
    epos = 0;
    if(len > 0)
    {
        boa_strbuf_ensurecapacity(sb, sb->length + len);
        data = boa_strbuf_mutdata(sb);
        if(sb->length > 0)
        {
            epos = sb->length;
        }
        memcpy(data + epos, str, len);
        sb->length = sb->length + len;
        data[sb->length] = '\0';
    }
    return true;
}

/* Copy a character array to the end of this BoaStrBuffer */
bool boa_strbuf_appendstr(BoaStrBuffer* sb, const char* str)
{
    return boa_strbuf_appendstrn(sb, str, strlen(str));
}

bool boa_strbuf_appendbuff(BoaStrBuffer* sb1, BoaStrBuffer* sb2)
{
    return boa_strbuf_appendstrn(sb1, boa_strbuf_data(sb2), sb2->length);
}

/*
 * Integer to string functions adapted from:
 *   www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 */

#define DYN_STRCONST_P01 10
#define DYN_STRCONST_P02 100
#define DYN_STRCONST_P03 1000
#define DYN_STRCONST_P04 10000
#define DYN_STRCONST_P05 100000
#define DYN_STRCONST_P06 1000000
#define DYN_STRCONST_P07 10000000
#define DYN_STRCONST_P08 100000000
#define DYN_STRCONST_P09 1000000000
#define DYN_STRCONST_P10 10000000000
#define DYN_STRCONST_P11 100000000000
#define DYN_STRCONST_P12 1000000000000

/**
 * Return number of digits required to represent `num` in base 10.
 * Uses binary search to find number.
 * Examples:
 *   boa_strbufutil_numofdigits(0)   = 1
 *   boa_strbufutil_numofdigits(1)   = 1
 *   boa_strbufutil_numofdigits(10)  = 2
 *   boa_strbufutil_numofdigits(123) = 3
 */
size_t boa_strbufutil_numofdigits(unsigned long v)
{
    if(v < DYN_STRCONST_P01)
    {
        return 1;
    }
    if(v < DYN_STRCONST_P02)
    {
        return 2;
    }
    if(v < DYN_STRCONST_P03)
    {
        return 3;
    }
    if(v < DYN_STRCONST_P12)
    {
        if(v < DYN_STRCONST_P08)
        {
            if(v < DYN_STRCONST_P06)
            {
                if(v < DYN_STRCONST_P04)
                {
                    return 4;
                }
                return 5 + (v >= DYN_STRCONST_P05);
            }
            return 7 + (v >= DYN_STRCONST_P07);
        }
        if(v < DYN_STRCONST_P10)
        {
            return 9 + (v >= DYN_STRCONST_P09);
        }
        return 11 + (v >= DYN_STRCONST_P11);
    }
    return 12 + boa_strbufutil_numofdigits(v / DYN_STRCONST_P12);
}

/* Convert integers to string to append */
bool boa_strbuf_appendnumulong(BoaStrBuffer* sb, unsigned long value)
{
    size_t v;
    size_t pos;
    size_t numdigits;
    char* dst;
    char* data;
    /* Append two digits at a time */
    static const char* digits = (
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899"
    );
    numdigits = boa_strbufutil_numofdigits(value);
    pos = numdigits - 1;
    boa_strbuf_ensurecapacity(sb, sb->length + numdigits);
    data = boa_strbuf_mutdata(sb);
    dst = data + sb->length;
    while(value >= 100)
    {
        v = value % 100;
        value /= 100;
        dst[pos] = digits[v * 2 + 1];
        dst[pos - 1] = digits[v * 2];
        pos -= 2;
    }
    /* Handle last 1-2 digits */
    if(value < 10)
    {
        dst[pos] = '0' + value;
    }
    else
    {
        dst[pos] = digits[value * 2 + 1];
        dst[pos - 1] = digits[value * 2];
    }
    sb->length += numdigits;
    data[sb->length] = '\0';
    return true;
}

bool boa_strbuf_appendnumlong(BoaStrBuffer* sb, long value)
{
    /* boa_strbuf_appendformat(sb, "%li", value); */
    if(value < 0)
    {
        boa_strbuf_appendchar(sb, '-');
        value = -value;
    }
    return boa_strbuf_appendnumulong(sb, value);
}

bool boa_strbuf_appendnumint(BoaStrBuffer* sb, int value)
{
    /* boa_strbuf_appendformat(sb, "%i", value); */
    return boa_strbuf_appendnumlong(sb, value);
}

/* Append string converted to lowercase */
bool boa_strbuf_appendstrnlowercase(BoaStrBuffer* sb, const char* str, size_t len)
{
    char* to;
    char* data;
    const char* plength;
    boa_strbuf_ensurecapacity(sb, sb->length + len);
    data = boa_strbuf_mutdata(sb);
    to = data + sb->length;
    plength = str + len;
    for(; str < plength; str++, to++)
    {
        *to = boa_util_chartolower(*str);
    }
    sb->length += len;
    data[sb->length] = '\0';
    return true;
}

/* Append string converted to uppercase */
bool boa_strbuf_appendstrnuppercase(BoaStrBuffer* sb, const char* str, size_t len)
{
    char* to;
    char* data;
    const char* end;
    boa_strbuf_ensurecapacity(sb, sb->length + len);
    data = boa_strbuf_mutdata(sb);
    to = data + sb->length;
    end = str + len;
    for(; str < end; str++, to++)
    {
        *to = boa_util_chartoupper(*str);
    }
    sb->length += len;
    data[sb->length] = '\0';
    return true;
}

void boa_strbuf_shrink(BoaStrBuffer* sb, size_t len)
{
    char* data;
    data = boa_strbuf_mutdata(sb);
    data[len] = 0;
    sb->length = len;
}

/*
 * Remove \r and \n characters from the end of this StringBuffer
 * Returns the number of characters removed
 */
size_t boa_strbuf_chomp(BoaStrBuffer* sb)
{
    size_t oldlen;
    char* data;
    data = boa_strbuf_mutdata(sb);
    oldlen = sb->length;
    sb->length = boa_strbufutil_chomp(data, sb->length);
    return oldlen - sb->length;
}

/* Reverse a string */
void boa_strbuf_reverse(BoaStrBuffer* sb)
{
    char* data;
    data = boa_strbuf_mutdata(sb);
    boa_strbufutil_reverseregion(data, sb->length);
}

/*
 * Get a substring as a new null terminated char array
 * (remember to free the returned char* after you're done with it!)
 */
char* boa_strbuf_substr(BoaStrBuffer* sb, size_t start, size_t len)
{
    char* data;
    char* newstr;
    boa_strbuf_boundscheckreadrange(sb, start, len);
    data = boa_strbuf_mutdata(sb);
    newstr = (char*)boa_sysmem_malloc((len + 1) * sizeof(char));
    strncpy(newstr, data + start, len);
    newstr[len] = '\0';
    return newstr;
}

void boa_strbuf_touppercase(BoaStrBuffer* sb)
{
    char* pos;
    char* end;
    char* data;
    data = boa_strbuf_mutdata(sb);
    end = data + sb->length;
    for(pos = data; pos < end; pos++)
    {
        *pos = (char)boa_util_chartoupper(*pos);
    }
}

void boa_strbuf_tolowercase(BoaStrBuffer* sb)
{
    char* pos;
    char* end;
    char* data;
    data = boa_strbuf_mutdata(sb);
    end = data + sb->length;
    for(pos = data; pos < end; pos++)
    {
        *pos = (char)boa_util_chartolower(*pos);
    }
}

/*
 * Copy a string to this BoaStrBuffer, overwriting any existing characters
 * Note: dstpos + len can be longer the the current sb BoaStrBuffer
 */
void boa_strbuf_copyover(BoaStrBuffer* sb, size_t dstpos, const char* src, size_t len)
{
    size_t newlen;
    char* data;
    if(src == NULL || len == 0)
    {
        return;
    }
    boa_strbuf_boundscheckinsert(sb, dstpos);
    /*
     * Check if sb buffer can handle string
     * src may have pointed to sb, which has now moved
     */
    newlen = STRBUF_MAX(dstpos + len, sb->length);
    boa_strbuf_ensurecapacity(sb, newlen);
    data = boa_strbuf_mutdata(sb);
    /* memmove instead of strncpy, as it can handle overlapping regions */
    memmove(data + dstpos, src, len * sizeof(char));
    if(dstpos + len > sb->length)
    {
        /* Extended string - add '\0' char */
        sb->length = dstpos + len;
        data[sb->length] = '\0';
    }
}

/* Insert: copy to a BoaStrBuffer, shifting any existing characters along */
void boa_strbuf_insert(BoaStrBuffer* sb, size_t dstpos, const char* src, size_t len)
{
    char* data;
    char* insert;
    if(src == NULL || len == 0)
    {
        return;
    }
    boa_strbuf_boundscheckinsert(sb, dstpos);
    /*
     * Check if sb buffer has capacity for inserted string plus \0
     * src may have pointed to sb, which will be moved in realloc when
     * calling ensure capacity
     */
    boa_strbuf_ensurecapacity(sb, sb->length + len);
    data = boa_strbuf_mutdata(sb);
    insert = data + dstpos;
    /* dstpos could be at the end (== sb->length) */
    if(dstpos < sb->length)
    {
        /* Shift some characters up */
        memmove(insert + len, insert, (sb->length - dstpos) * sizeof(char));
        if(src >= data && src < data + sb->capacity)
        {
            /* src/sb strings point to the same string in memory */
            if(src < insert)
            {
                memmove(insert, src, len * sizeof(char));
            }
            else if(src > insert)
            {
                memmove(insert, src + len, len * sizeof(char));
            }
        }
        else
        {
            memmove(insert, src, len * sizeof(char));
        }
    }
    else
    {
        memmove(insert, src, len * sizeof(char));
    }
    /* Update size */
    sb->length += len;
    data[sb->length] = '\0';
}

/*
 * Overwrite dstpos..(dstpos+dstlen-1) with srclen chars from src
 * if dstlen != srclen, content to the right of dstlen is shifted
 * Example:
 * boa_strbuf_set(sb, "aaabbccc");
 * char *mystr = "xxx";
 * boa_strbuf_overwrite(sb,3,2,mystr,strlen(mystr));
 * // sb is now "aaaxxxccc"
 * boa_strbuf_overwrite(sb,3,2,"_",1);
 * // sb is now "aaaccc"
 */
void boa_strbuf_overwrite(BoaStrBuffer* sb, size_t dstpos, size_t dstlen, const char* src, size_t srclen)
{
    size_t len;
    size_t newlen;
    char* tgt;
    char* end;
    char* data;
    boa_strbuf_boundscheckreadrange(sb, dstpos, dstlen);
    if(src == NULL)
    {
        return;
    }
    if(dstlen == srclen)
    {
        boa_strbuf_copyover(sb, dstpos, src, srclen);
    }
    newlen = sb->length + srclen - dstlen;
    boa_strbuf_ensurecapacity(sb, newlen);
    data = boa_strbuf_mutdata(sb);
    if(src >= data && src < data + sb->capacity)
    {
        if(srclen < dstlen)
        {
            /* copy */
            memmove(data + dstpos, src, srclen * sizeof(char));
            /* resize (shrink) */
            memmove(data + dstpos + srclen, data + dstpos + dstlen, (sb->length - dstpos - dstlen) * sizeof(char));
        }
        else
        {
            /*
             * Buffer is going to grow and src points to this buffer
             * resize (grow)
             */
            memmove(data + dstpos + srclen, data + dstpos + dstlen, (sb->length - dstpos - dstlen) * sizeof(char));
            tgt = data + dstpos;
            end = data + dstpos + srclen;
            if(src < tgt + dstlen)
            {
                len = STRBUF_MIN((size_t)(end - src), srclen);
                memmove(tgt, src, len);
                tgt += len;
                src += len;
                srclen -= len;
            }
            if(src >= tgt + dstlen)
            {
                /* shift to account for resizing */
                src += srclen - dstlen;
                memmove(tgt, src, srclen);
            }
        }
    }
    else
    {
        /* resize */
        memmove(data + dstpos + srclen, data + dstpos + dstlen, (sb->length - dstpos - dstlen) * sizeof(char));
        /* copy */
        memcpy(data + dstpos, src, srclen * sizeof(char));
    }
    sb->length = newlen;
    data[sb->length] = '\0';
}

/*
 * Remove characters from the buffer
 * boa_strbuf_set(sb, "aaaBBccc");
 * boa_strbuf_erase(sb, 3, 2);
 * // sb is now "aaaccc"
 */
void boa_strbuf_erase(BoaStrBuffer* sb, size_t pos, size_t len)
{
    char* data;
    boa_strbuf_boundscheckreadrange(sb, pos, len);
    data = boa_strbuf_mutdata(sb);
    memmove(data + pos, data + pos + len, sb->length - pos - len);
    sb->length -= len;
    data[sb->length] = '\0';
}

int boa_strbuf_appendformatposv(BoaStrBuffer* sb, size_t pos, const char* fmt, va_list argptr)
{
    size_t buflen;
    int numchars;
    va_list vacpy;
    char* data;
    boa_strbuf_boundscheckinsert(sb, pos);
    /* Length of remaining buffer */
    buflen = sb->capacity - pos;
    if(buflen == 0 && !boa_strbuf_ensurecapacity(sb, sb->capacity << 1))
    {
        fprintf(stderr, "%s:%i:Error: Out of memory\n", __FILE__, __LINE__);
        abort();
    }
    data = boa_strbuf_mutdata(sb);
    /* Make a copy of the list of args incase we need to resize buff and try again */
    va_copy(vacpy, argptr);
    numchars = vsnprintf(data + pos, buflen, fmt, argptr);
    va_end(argptr);
    /*
     * numchars is the number of chars that would be written (not including '\0')
     * numchars < 0 => failure
     */
    if(numchars < 0)
    {
        fprintf(stderr, "warning: boa_strbuf_appendformatv something went wrong..\n");
        abort();
    }
    /* numchars does not include the null terminating byte */
    if((size_t)numchars + 1 > buflen)
    {
        boa_strbuf_ensurecapacity(sb, pos + (size_t)numchars);
        /*
         * now use the argptr copy we made earlier
         * Don't need to use vsnprintf now, vsprintf will do since we know it'll fit
         */
        data = boa_strbuf_mutdata(sb);
        numchars = vsprintf(data + pos, fmt, vacpy);
        if(numchars < 0)
        {
            fprintf(stderr, "warning: boa_strbuf_appendformatv something went wrong..\n");
            abort();
        }
    }
    va_end(vacpy);
    /*
     * Don't need to NUL terminate, vsprintf/vnsprintf does that for us
     * Update length
     */
    sb->length = pos + (size_t)numchars;
    return numchars;
}

int boa_strbuf_appendformatv(BoaStrBuffer* sb, const char* fmt, va_list argptr)
{
    return boa_strbuf_appendformatposv(sb, sb->length, fmt, argptr);
}

/* sprintf to the end of a BoaStrBuffer (adds string terminator after sprint) */
int boa_strbuf_appendformat(BoaStrBuffer* sb, const char* fmt, ...)
{
    int numchars;
    va_list argptr;
    va_start(argptr, fmt);
    numchars = boa_strbuf_appendformatposv(sb, sb->length, fmt, argptr);
    va_end(argptr);
    return numchars;
}

/* Print at a given position (overwrite chars at positions >= pos) */
int boa_strbuf_appendformatat(BoaStrBuffer* sb, size_t pos, const char* fmt, ...)
{
    int numchars;
    va_list argptr;
    boa_strbuf_boundscheckinsert(sb, pos);
    va_start(argptr, fmt);
    numchars = boa_strbuf_appendformatposv(sb, pos, fmt, argptr);
    va_end(argptr);
    return numchars;
}

/*
 * sprintf without terminating character
 * Does not prematurely end the string if you sprintf within the string
 * (terminates string if sprintf to the end)
 * Does not prematurely end the string if you sprintf within the string
 * (vs at the end)
 */
int boa_strbuf_appendformatnoterm(BoaStrBuffer* sb, size_t pos, const char* fmt, ...)
{
    size_t len;
    int nchars;
    char lastchar;
    va_list argptr;
    char* data;
    boa_strbuf_boundscheckinsert(sb, pos);
    len = sb->length;
    /* Call vsnprintf with NULL, 0 to get resulting string length without writing */
    va_start(argptr, fmt);
    nchars = vsnprintf(NULL, 0, fmt, argptr);
    va_end(argptr);
    if(nchars < 0)
    {
        fprintf(stderr, "warning: boa_strbuf_appendformatv something went wrong..\n");
        abort();
    }
    /* Save overwritten char */
    data = boa_strbuf_mutdata(sb);
    lastchar = (pos + (size_t)nchars < sb->length) ? data[pos + (size_t)nchars] : 0;
    va_start(argptr, fmt);
    nchars = boa_strbuf_appendformatposv(sb, pos, fmt, argptr);
    va_end(argptr);
    if(nchars < 0)
    {
        fprintf(stderr, "warning: boa_strbuf_appendformatv something went wrong..\n");
        abort();
    }
    /* Restore length if shrunk, null terminate if extended */
    if(sb->length < len)
    {
        sb->length = len;
    }
    else
    {
        data[sb->length] = '\0';
    }
    /* Re-instate overwritten character */
    data[pos + (size_t)nchars] = lastchar;
    return nchars;
}

/* Trim whitespace characters from the start and end of a string */
void boa_strbuf_triminplace(BoaStrBuffer* sb, const char* list)
{
    boa_strbuf_trimleftinplace(sb, list);
    boa_strbuf_trimrightinplace(sb, list);
    /*
    size_t start;
    char* data;
    if(sb->length == 0)
    {
        return;
    }
    data = boa_strbuf_mutdata(sb);
    while(sb->length > 0 && isspace((int)data[sb->length - 1]))
    {
        sb->length--;
    }
    data[sb->length] = '\0';
    if(sb->length == 0)
    {
        return;
    }
    start = 0;
    while(start < sb->length && isspace((int)data[start]))
    {
        start++;
    }
    if(start != 0)
    {
        sb->length -= start;
        memmove(data, data + start, sb->length * sizeof(char));
        data[sb->length] = '\0';
    }
    */
}

/*
 * Trim the characters listed in `list` from the left of `sb`
 * `list` is a null-terminated string of characters
 */
void boa_strbuf_trimleftinplace(BoaStrBuffer* sb, const char* list)
{
    size_t start;
    char* data;
    start = 0;
    data = boa_strbuf_mutdata(sb);
    while(start < sb->length && (strchr(list, data[start]) != NULL))
    {
        start++;
    }
    if(start != 0)
    {
        sb->length -= start;
        memmove(data, data + start, sb->length * sizeof(char));
        data[sb->length] = '\0';
    }
}

/*
 * Trim the characters listed in `list` from the right of `sb`
 * `list` is a null-terminated string of characters
 */
void boa_strbuf_trimrightinplace(BoaStrBuffer* sb, const char* list)
{
    char* data;
    if(sb->length == 0)
    {
        return;
    }
    data = boa_strbuf_mutdata(sb);
    while(sb->length > 0 && strchr(list, data[sb->length - 1]) != NULL)
    {
        sb->length--;
    }
    data[sb->length] = '\0';
}

size_t boa_util_grownextcapacity(size_t capacity)
{
    if(capacity < 8)
    {
        return 8;
    }
    return (capacity * 2);
}

bool boa_util_charisdigit(char c)
{
    return c >= '0' && c <= '9';
}

bool boa_util_charisalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

/* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float */
int boa_util_closestpoweroftwo(int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

/* allows you to set a custom length. */
void boa_utf8iter_init(BoaUTF8Iterator* iter, const char* ptr, uint32_t length)
{
    iter->plainstr = ptr;
    iter->plainlen = length;
    iter->codepoint = 0;
    iter->currpos = 0;
    iter->nextpos = 0;
    iter->currcount = 0;
}

/* calculate the number of bytes a UTF8 character occupies in a string. */
uint8_t boa_utf8iter_charsize(const char* character)
{
    if(character == NULL)
    {
        return 0;
    }
    if(character[0] == 0)
    {
        return 0;
    }
    if((character[0] & 0x80) == 0)
    {
        return 1;
    }
    else if((character[0] & 0xE0) == 0xC0)
    {
        return 2;
    }
    else if((character[0] & 0xF0) == 0xE0)
    {
        return 3;
    }
    else if((character[0] & 0xF8) == 0xF0)
    {
        return 4;
    }
    else if((character[0] & 0xFC) == 0xF8)
    {
        return 5;
    }
    else if((character[0] & 0xFE) == 0xFC)
    {
        return 6;
    }
    return 0;
}

uint32_t boa_utf8iter_converter(const char* character, uint8_t size)
{
    uint8_t i;
    static uint32_t codepoint = 0;
    static const uint8_t g_utf8iter_table_unicode[] = { 0, 0, 0x1F, 0xF, 0x7, 0x3, 0x1 };
    if(size == 0)
    {
        return 0;
    }
    if(character == NULL)
    {
        return 0;
    }
    if(character[0] == 0)
    {
        return 0;
    }
    if(size == 1)
    {
        return character[0];
    }
    codepoint = g_utf8iter_table_unicode[size] & character[0];
    for(i = 1; i < size; i++)
    {
        codepoint = codepoint << 6;
        codepoint = codepoint | (character[i] & 0x3F);
    }
    return codepoint;
}

/* returns 1 if there is a character in the next position. If there is not, return 0. */
uint8_t boa_utf8iter_next(BoaUTF8Iterator* iter)
{
    const char* pointer;
    if(iter == NULL)
    {
        return 0;
    }
    if(iter->plainstr == NULL)
    {
        return 0;
    }
    if(iter->nextpos < iter->plainlen)
    {
        iter->currpos = iter->nextpos;
        /* Set Current Pointer */
        pointer = iter->plainstr + iter->nextpos;
        iter->charsize = boa_utf8iter_charsize(pointer);
        if(iter->charsize == 0)
        {
            return 0;
        }
        iter->nextpos = iter->nextpos + iter->charsize;
        iter->codepoint = boa_utf8iter_converter(pointer, iter->charsize);
        if(iter->codepoint == 0)
        {
            return 0;
        }
        iter->currcount++;
        return 1;
    }
    iter->currpos = iter->nextpos;
    return 0;
}

/* return current character in UFT8 - no same that iter.codepoint (not codepoint/unicode) */
const char* boa_utf8iter_getchar(BoaUTF8Iterator* iter)
{
    uint8_t i;
    const char* pointer;
    static char str[16];
    str[0] = '\0';
    if(iter == NULL)
    {
        return str;
    }
    if(iter->plainstr == NULL)
    {
        return str;
    }
    if(iter->charsize == 0)
    {
        return str;
    }
    if(iter->charsize == 1)
    {
        str[0] = iter->plainstr[iter->currpos];
        str[1] = '\0';
        return str;
    }
    pointer = iter->plainstr + iter->currpos;
    for(i = 0; i < iter->charsize; i++)
    {
        str[i] = pointer[i];
    }
    str[iter->charsize] = '\0';
    return str;
}

int boa_util_utfchargetcountdecode(uint8_t byte)
{
    if((byte & 0xc0) == 0x80)
    {
        return 0;
    }
    if((byte & 0xf8) == 0xf0)
    {
        return 4;
    }
    if((byte & 0xf0) == 0xe0)
    {
        return 3;
    }
    if((byte & 0xe0) == 0xc0)
    {
        return 2;
    }
    return 1;
}

int boa_util_utfchargetcountencode(int value)
{
    if(value <= 0x7f)
    {
        return 1;
    }
    if(value <= 0x7ff)
    {
        return 2;
    }
    if(value <= 0xffff)
    {
        return 3;
    }
    if(value <= 0x10ffff)
    {
        return 4;
    }
    return 0;
}

int boa_util_utfcharwritebytes(int value, uint8_t* destbytes)
{
    if(value <= 0x7f)
    {
        *destbytes = value & 0x7f;
        return 1;
    }
    else if(value <= 0x7ff)
    {
        *destbytes = 0xc0 | ((value & 0x7c0) >> 6);
        destbytes++;
        *destbytes = 0x80 | (value & 0x3f);
        return 2;
    }
    else if(value <= 0xffff)
    {
        *destbytes = 0xe0 | ((value & 0xf000) >> 12);
        destbytes++;
        *destbytes = 0x80 | ((value & 0xfc0) >> 6);
        destbytes++;
        *destbytes = 0x80 | (value & 0x3f);
        return 3;
    }
    else if(value <= 0x10ffff)
    {
        *destbytes = 0xf0 | ((value & 0x1c0000) >> 18);
        destbytes++;
        *destbytes = 0x80 | ((value & 0x3f000) >> 12);
        destbytes++;
        *destbytes = 0x80 | ((value & 0xfc0) >> 6);
        destbytes++;
        *destbytes = 0x80 | (value & 0x3f);
        return 4;
    }
    BOA_UTIL_UNREACHABLE();
    return 0;
}

int boa_util_utfstrdecode(const uint8_t* bytes, uint32_t length)
{
    int value;
    uint32_t remainingbytes;
    if(*bytes <= 0x7f)
    {
        return *bytes;
    }
    if((*bytes & 0xe0) == 0xc0)
    {
        value = *bytes & 0x1f;
        remainingbytes = 1;
    }
    else if((*bytes & 0xf0) == 0xe0)
    {
        value = *bytes & 0x0f;
        remainingbytes = 2;
    }
    else if((*bytes & 0xf8) == 0xf0)
    {
        value = *bytes & 0x07;
        remainingbytes = 3;
    }
    else
    {
        return -1;
    }
    if(remainingbytes > length - 1)
    {
        return -1;
    }
    while(remainingbytes > 0)
    {
        bytes++;
        remainingbytes--;
        if((*bytes & 0xc0) != 0x80)
        {
            return -1;
        }
        value = value << 6 | (*bytes & 0x3f);
    }
    return value;
}

int boa_util_utfcharisutfbyte(int c)
{
    return (((c) & 0xC0) != 0x80);
}

int boa_util_utfstrfindoffset(const char* str, int index)
{
    int offset;
    offset = 0;
    while(index > 0 && str[offset])
    {
        if(!boa_util_utfcharisutfbyte(str[++offset]))
        {
            if(!boa_util_utfcharisutfbyte(str[++offset]))
            {
                if(!boa_util_utfcharisutfbyte(str[++offset]))
                {
                    ++offset;
                }
            }
        }
        index--;
    }
    return offset;
}

/* returns the number of bytes contained in a unicode character */
int boa_util_utfcharbytelength(int value)
{
    if(value < 0)
    {
        return -1;
    }
    if(value <= 0x7f)
    {
        return 1;
    }
    if(value <= 0x7ff)
    {
        return 2;
    }
    if(value <= 0xffff)
    {
        return 3;
    }
    if(value <= 0x10ffff)
    {
        return 4;
    }
    return 0;
}

char* boa_util_utfstrencode(unsigned int code, size_t* dlen)
{
    int count;
    char* chars;
    *dlen = 0;
    count = boa_util_utfcharbytelength((int)code);
    if(BOA_LIKELY(count > 0))
    {
        *dlen = count;
        chars = (char*)boa_sysmem_malloc(sizeof(char) * ((size_t)count + 1));
        if(chars != NULL)
        {
            if(code <= 0x7F)
            {
                chars[0] = (char)(code & 0x7F);
                chars[1] = '\0';
            }
            else if(code <= 0x7FF)
            {
                /* one continuation byte */
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xC0 | (code & 0x1F));
            }
            else if(code <= 0xFFFF)
            {
                /* two continuation bytes */
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xE0 | (code & 0xF));
            }
            else if(code <= 0x10FFFF)
            {
                /* three continuation bytes */
                chars[3] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xF0 | (code & 0x7));
            }
            else
            {
                /* unicode replacement character */
                chars[2] = (char)0xEF;
                chars[1] = (char)0xBF;
                chars[0] = (char)0xBD;
            }
            return chars;
        }
    }
    return NULL;
}

int boa_util_chartolower(int c)
{
    return tolower(c);
}

int boa_util_chartoupper(int c)
{
    return toupper(c);
}

void boa_util_strchangecase(const char* instr, size_t inlen, char* deststr, int (*func)(int))
{
    size_t i;
    for(i = 0; i < inlen; i++)
    {
        deststr[i] = func(instr[i]);
    }
}

static int boa_util_strfindfirstchar(const char* str, size_t len, int ch)
{
    size_t i;
    for(i=0; i<len; i++)
    {
        if(str[i] == ch)
        {
            return i;
        }
    }
    return -1;
}

static int boa_util_strcasecmp(const char* s1, const char* s2)
{
    const char *cm;
    static const char chrmap[] = {
        '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
        '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
        '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
        '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
        '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
        '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
        '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
        '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
        '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
        '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
        '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
        '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
        '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
        '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
        '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
    };
    cm = chrmap;
	while (cm[(int)*s1] == cm[(int)*s2++])
    {
		if (*s1++ == '\0')
        {
            return(0);
        }
	}
    return(cm[(int)*s1] - cm[(int)*--s2]);
}

bool boa_util_strcaseequal(const char* s1, const char* s2)
{
    return (boa_util_strcasecmp(s1, s2) == 0);
}

char* boa_util_readhandle(FILE* hnd, size_t* dlen)
{
    long rawtold;
    /*
    * the value returned by ftell() may not necessarily be the same as
    * the amount that can be read.
    * since we only ever read a maximum of $toldlen, there will
    * be no memory trashing.
    */
    size_t toldlen;
    size_t actuallen;
    char* buf;
    if(fseek(hnd, 0, SEEK_END) == -1)
    {
        return NULL;
    }
    if((rawtold = ftell(hnd)) == -1)
    {
        return NULL;
    }
    toldlen = rawtold;
    if(fseek(hnd, 0, SEEK_SET) == -1)
    {
        return NULL;
    }
    buf = (char*)boa_sysmem_calloc(toldlen + 1, sizeof(char));
    memset(buf, 0, toldlen+1);
    if(buf != NULL)
    {
        actuallen = fread(buf, sizeof(char), toldlen, hnd);
        /*
        // optionally, read remainder:
        size_t tmplen;
        if(actuallen < toldlen)
        {
            tmplen = actuallen;
            actuallen += fread(buf+tmplen, sizeof(char), actuallen-toldlen, hnd);
            ...
        }
        // unlikely to be necessary, so not implemented.
        */
        if(dlen != NULL)
        {
            *dlen = actuallen;
        }
        return buf;
    }
    return NULL;
}

char* boa_util_readfile(const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    if((fh = fopen(filename, "rb")) == NULL)
    {
        return NULL;
    }
    b = boa_util_readhandle(fh, dlen);
    fclose(fh);
    return b;
}

char* boa_util_dupstring(const char* string)
{
    size_t length;
    char* newstring;
    length = strlen(string) + 1;
    newstring = (char*)boa_sysmem_malloc(length);
    memcpy(newstring, string, length);
    return newstring;
}

char* boa_util_dirname(const char *fname, size_t* lendest)
{
    size_t dirlen;
    char * dirpart;
    const char *p;
    const char *slash;
    p = fname;
    slash = NULL;
    *lendest = 0;
    if(fname)
    {
        if(*fname && fname[1] == ':')
        {
            slash = fname + 1;
            p += 2;
        }
        /* Find the rightmost slash.  */
        while (*p)
        {
            if (*p == '/' || *p == '\\')
            {
                slash = p;
            }
            p++;
        }
        if(slash == NULL)
        {
            fname = ".";
            dirlen = 1;
        }
        else
        {
            /* Remove any trailing slashes.  */
            while(slash > fname && (slash[-1] == '/' || slash[-1] == '\\'))
            {
                slash--;
            }
            /* How long is the directory we will return?  */
            dirlen = slash - fname + (slash == fname || slash[-1] == ':');
            if (*slash == ':' && dirlen == 1)
            {
                dirlen += 2;
            }
        }
        dirpart = (char *)boa_sysmem_malloc(dirlen + 1);
        if(dirpart != NULL)
        {
            strncpy(dirpart, fname, dirlen);
            if (slash && *slash == ':' && dirlen == 3)
            {
                dirpart[2] = '.';	/* for "x:foo" return "x:." */
            }
            dirpart[dirlen] = '\0';
        }
        *lendest = dirlen;
        return dirpart;
    }
    return NULL;
}

const char* boa_util_fsgetbasename(const char* opath)
{
    char* strbeg;
    char* strend;
    char* cpath;
    strend = cpath = (char*)opath;
    while(*strend)
    {
        strend++;
    }
    while(strend > cpath && strend[-1] == '/')
    {
        strend--;
    }
    strbeg = strend;
    while(strbeg > cpath && strbeg[-1] != '/')
    {
        strbeg--;
    }
    /* len = (strend - strbeg) */
    cpath = strbeg;
    cpath[(strend - strbeg)] = 0;
    return strbeg;
}

int boa_util_isatty(int fd)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return isatty(fd);
    #else
        return 0;
    #endif
}

char* boa_util_getcwd(char* buf, size_t size)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return getcwd(buf, size);
    #else
        #if defined(BOA_OSPLATFORM_ISWINNT)
            GetCurrentDirectory(size, buf);
            return buf;
        #endif
    #endif
    return NULL;
}

unsigned int boa_util_sleep(unsigned int seconds)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return sleep(seconds);
    #else
        return 0;
    #endif
}

int boa_util_mkdir(const char* path, size_t mode)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return mkdir(path, mode);
    #else
        return -1;
    #endif
}

int boa_util_rmdir(const char* path)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return rmdir(path);
    #else
        return -1;
    #endif
}

int boa_util_unlink(const char* path)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return unlink(path);
    #else
        return -1;
    #endif
}

const char* boa_util_getenv(const char* key)
{
    return getenv(key);
}

bool boa_util_setenv(const char* key, const char* value, bool replace)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return (setenv(key, value, replace) == 0);
    #else
        int errcode;
        size_t envsize;
        errcode = 0;
        if(!replace)
        {
            envsize = 0;
            errcode = getenv_s(&envsize, NULL, 0, key);
            if(errcode || envsize) return errcode;
        }
        if(_putenv_s(key, value) == -1)
        {
            return false;
        }
        return true;
    #endif
}

int boa_util_chdir(const char* path)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return chdir(path);
    #else
        return -1;
    #endif
}

int boa_util_getpid()
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return getpid();
    #endif
    return -1;
}

int boa_util_kill(int pid, int code)
{
    #if defined(BOA_OSPLATFORM_ISUNIXLIKE)
        return kill(pid, code);
    #else
        return -1;
    #endif
}

bool boa_util_fsfileistype(const char* filepath, int typ)
{
    struct stat sti;
    (void)filepath;
    if(stat(filepath, &sti) == -1)
    {
        return false;
    }
    if(typ == 'f')
    {
        return S_ISREG(sti.st_mode);
    }
    else if(typ == 'd')
    {
        return S_ISDIR(sti.st_mode);
    }
    return false;
}

bool boa_util_fsfileisfile(const char* filepath)
{
    return boa_util_fsfileistype(filepath, 'f');
}

bool boa_util_fsfileisdirectory(const char* filepath)
{
    return boa_util_fsfileistype(filepath, 'd');
}

bool boa_util_fsfileexists(const char* filepath)
{
    return boa_util_fsfileistype(filepath, 'f');
}

/* endutils */

BOA_FORCEINLINE BoaObject* boa_value_asobject(BoaValue v)
{
    return (v.as.obj);
}

BOA_FORCEINLINE BoaObjType boa_value_objtype(BoaValue value)
{
    return boa_value_asobject(value)->type;
}

BOA_INLINE bool boa_value_istype(BoaValue v, BoaValType t)
{
    return v.type == t;
}

BOA_INLINE bool boa_value_isbool(BoaValue v)
{
    return boa_value_istype(v, BOA_VALTYPE_BOOL);
}

BOA_INLINE bool boa_value_isnull(BoaValue v)
{
    return boa_value_istype(v, BOA_VALTYPE_NULL);
}

BOA_INLINE bool boa_value_isnumber(BoaValue v)
{
    return boa_value_istype(v, BOA_VALTYPE_NUMBER);
}

BOA_INLINE bool boa_value_isobject(BoaValue v)
{
    return boa_value_istype(v, BOA_VALTYPE_OBJECT);
}

BOA_INLINE bool boa_value_isobjtype(BoaValue value, BoaObjType t)
{
    if(boa_value_isobject(value))
    {
        return (boa_value_asobject(value)->type == t);
    }
    return false;
}

BOA_INLINE bool boa_value_ismap(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_MAP);
}

BOA_INLINE bool boa_value_isstring(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_STRING);
}

BOA_INLINE bool boa_value_isfuncscript(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_FUNCSCRIPT);
}

BOA_INLINE bool boa_value_isfuncmethod(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_FUNCNATMETHOD);
}

BOA_INLINE bool boa_value_ismodule(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_MODULE);
}

BOA_INLINE bool boa_value_isclass(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_CLASS);
}

BOA_INLINE bool boa_value_isinstance(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_INSTANCE);
}

BOA_INLINE bool boa_value_isvargarray(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_VARARGARRAY);
}

BOA_INLINE bool boa_value_isarray(BoaValue value)
{
    return (boa_value_isobjtype(value, BOA_OBJTYPE_ARRAY) || boa_value_isvargarray(value));
}

BOA_INLINE bool boa_value_isrange(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_RANGE);
}

BOA_INLINE bool boa_value_isfield(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_FIELD);
}

BOA_INLINE bool boa_value_isreference(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_REFERENCE);
}

BOA_INLINE bool boa_value_isexception(BoaValue value)
{
    return boa_value_isobjtype(value, BOA_OBJTYPE_EXCEPTION);
}

BOA_INLINE bool boa_value_iscallablefunction(BoaValue value)
{
    BoaObjType type;
    if(boa_value_isobject(value))
    {
        type = boa_value_objtype(value);
        return ((type == BOA_OBJTYPE_FUNCCLOSURE) || (type == BOA_OBJTYPE_FUNCSCRIPT) || (type == BOA_OBJTYPE_FUNCNATIVE) || (type == BOA_OBJTYPE_FUNCNATMETHOD) || (type == BOA_OBJTYPE_FUNCBOUNDMETHOD));
    }
    return false;
}

BOA_FORCEINLINE BoaNumber boa_value_asnumber(BoaValue value)
{
    return value.as.numval;
}

BOA_FORCEINLINE bool boa_value_asbool(BoaValue v)
{
    return (v.as.boolval);
}

BOA_FORCEINLINE BoaString* boa_value_asstring(BoaValue value)
{
    return ((BoaString*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFuncScript* boa_value_asfuncscript(BoaValue value)
{
    return ((BoaFuncScript*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFuncNative* boa_value_asfuncnative(BoaValue value)
{
    return ((BoaFuncNative*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFuncNative* boa_value_asfuncmethod(BoaValue value)
{
    return ((BoaFuncNative*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaModule* boa_value_asmodule(BoaValue value)
{
    return ((BoaModule*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFuncClosure* boa_value_asfuncclosure(BoaValue value)
{
    return ((BoaFuncClosure*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaClsPrototype* boa_value_asclsproto(BoaValue value)
{
    return ((BoaClsPrototype*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaUpvalue* boa_value_asupvalue(BoaValue value)
{
    return ((BoaUpvalue*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaClass* boa_value_asclass(BoaValue value)
{
    return ((BoaClass*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaInstance* boa_value_asinstance(BoaValue value)
{
    return ((BoaInstance*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaArray* boa_value_asarray(BoaValue value)
{
    return ((BoaArray*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaMap* boa_value_asmap(BoaValue value)
{
    return ((BoaMap*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFuncBound* boa_value_asfuncboundmethod(BoaValue value)
{
    return ((BoaFuncBound*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaUserdata* boa_value_asuserdata(BoaValue value)
{
    return ((BoaUserdata*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaRange* boa_value_asrange(BoaValue value)
{
    return ((BoaRange*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaField* boa_value_asfield(BoaValue value)
{
    return ((BoaField*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaFiber* boa_value_asfiber(BoaValue value)
{
    return ((BoaFiber*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaReference* boa_value_asreference(BoaValue value)
{
    return ((BoaReference*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaException* boa_value_asexception(BoaValue value)
{
    return ((BoaException*)boa_value_asobject(value));
}

BOA_FORCEINLINE BoaValue boa_value_makenull()
{
    BoaValue rt;
    rt.type = BOA_VALTYPE_NULL;
    rt.as.numval = 0;
    return rt;
}

BOA_FORCEINLINE BoaValue boa_value_makebool(bool b)
{
    BoaValue rt;
    rt.type = BOA_VALTYPE_BOOL;
    rt.as.boolval = b;
    return rt;
}

BOA_FORCEINLINE BoaValue boa_value_makenumber(BoaNumber num)
{
    BoaValue rt;
    rt.type = BOA_VALTYPE_NUMBER;
    rt.as.numval = num;
    return rt;
}

#define boa_value_fromobject(obj) boa_value_fromobject_actual((BoaObject*)(obj))

BOA_FORCEINLINE BoaValue boa_value_fromobject_actual(BoaObject* obj)
{
    BoaValue rt;
    rt.type = BOA_VALTYPE_OBJECT;
    rt.as.obj = obj;
    return rt;
}

BOA_INLINE bool boa_value_isfalsy(BoaValue value)
{
    BoaString* str;
    if(boa_value_isbool(value))
    {
        return (boa_value_asbool(value) == false);
    }
    else if(boa_value_isnull(value))
    {
        return true;
    }
    else if(boa_value_isnumber(value))
    {
        return (boa_value_asnumber(value) == 0);
    }
    else
    {
        if(boa_value_isstring(value))
        {
            str = boa_value_asstring(value);
            if(boa_string_getlength(str) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

BOA_FORCEINLINE bool boa_value_compare(BoaState* state, BoaValue a, BoaValue b)
{
    BoaValue tmpargs[2];
    BoaString* as;
    BoaString* bs;

    if(a.type == b.type)
    {
        if(boa_value_isnumber(a))
        {
            return boa_value_asnumber(a) == boa_value_asnumber(b);
        }
        if(boa_value_isnull(a))
        {
            return true;
        }
        if(boa_value_isbool(a))
        {
            return boa_value_asbool(a) == boa_value_asbool(b);
        }
        if(boa_value_asobject(a) == boa_value_asobject(b))
        {
            return true;
        }
        if(boa_value_isstring(a) && boa_value_isstring(b))
        {
            as = boa_value_asstring(a);
            bs = boa_value_asstring(b);
            return boa_string_getlength(as) == boa_string_getlength(bs) && memcmp(boa_string_getdata(as), boa_string_getdata(bs), boa_string_getlength(as)) == 0;
        }
    }
    else
    {
        if(boa_value_isbool(a) && boa_value_isnumber(b))
        {
            return boa_value_asbool(a) == boa_value_asnumber(b);
        }
        if(boa_value_isnumber(a) && boa_value_isbool(b))
        {
            return boa_value_asnumber(a) == boa_value_asbool(b);
        }
        if(boa_value_isstring(a) || boa_value_isstring(b))
        {
            return false;
        }
    }
    tmpargs[0] = b;
    return !boa_value_isfalsy(boa_state_findandcallmethod(state, a, state->strings.stropequal, tmpargs, 1).result);
}

const char* boa_value_objtypename(int t)
{
    switch(t)
    {
        case BOA_OBJTYPE_STRING:
            return "string";
        case BOA_OBJTYPE_FUNCSCRIPT:
        case BOA_OBJTYPE_FUNCNATIVE:
        case BOA_OBJTYPE_FUNCNATMETHOD:
        case BOA_OBJTYPE_FUNCCLOSURE:
        case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            return "function";
        case BOA_OBJTYPE_FIBER:
            return "fiber";
        case BOA_OBJTYPE_MODULE:
            return "module";

        case BOA_OBJTYPE_CLSPROTOTYPE:
            return "prototype";
        case BOA_OBJTYPE_UPVALUE:
            return "upvalue";
        case BOA_OBJTYPE_CLASS:
            return "class";
        case BOA_OBJTYPE_INSTANCE:
            return "instance";
        case BOA_OBJTYPE_ARRAY:
            return "array";
        case BOA_OBJTYPE_VARARGARRAY:
            return "varargarray";
        case BOA_OBJTYPE_MAP:
            return "map";
        case BOA_OBJTYPE_USERDATA:
            return "userdata";
        case BOA_OBJTYPE_RANGE:
            return "range";
        case BOA_OBJTYPE_FIELD:
            return "field";
        case BOA_OBJTYPE_REFERENCE:
            return "reference";
    }
    return "?unknown?";
}

const char* boa_value_valtypefromtype(int t)
{
    switch(t)
    {
        case BOA_VALTYPE_NULL:
            return "null";
        case BOA_VALTYPE_BOOL:
            return "bool";
        case BOA_VALTYPE_NUMBER:
            return "number";
        /* technically never reached */
        case BOA_VALTYPE_OBJECT:
            return "object";
    }
    return "?unknown?";
}

const char* boa_value_valtypename(BoaValue val)
{
    if(boa_value_isobject(val))
    {
        return boa_value_objtypename(boa_value_asobject(val)->type);
    }
    return boa_value_valtypefromtype(val.type);
}

const char* boa_value_typenamefromfn(BoaValueIsFN fn)
{
    #define iftypefn(name, vtyp, callthat) \
        { \
            static const BoaValueIsFN cache[] = {&name}; \
            if((fn) == cache[0]) \
            { \
                return callthat(vtyp); \
            } \
        }
    #define iftypefnval(name, vtyp) iftypefn(name, vtyp, boa_value_valtypefromtype)
    #define iftypefnobj(name, vtyp) iftypefn(name, vtyp, boa_value_objtypename)
        iftypefnval(boa_value_isfalsy, BOA_VALTYPE_BOOL);
        iftypefnval(boa_value_isbool, BOA_VALTYPE_BOOL);
        iftypefnval(boa_value_isnull, BOA_VALTYPE_NULL);
        iftypefnval(boa_value_isnumber, BOA_VALTYPE_NUMBER);
        iftypefnval(boa_value_isobject, BOA_VALTYPE_OBJECT);
        iftypefnobj(boa_value_ismap, BOA_OBJTYPE_MAP);
        iftypefnobj(boa_value_isstring, BOA_OBJTYPE_STRING);
        iftypefnobj(boa_value_isfuncscript, BOA_OBJTYPE_FUNCSCRIPT);
        iftypefnobj(boa_value_isfuncmethod, BOA_OBJTYPE_FUNCNATMETHOD);
        iftypefnobj(boa_value_ismodule, BOA_OBJTYPE_MODULE);
        iftypefnobj(boa_value_isclass, BOA_OBJTYPE_CLASS);
        iftypefnobj(boa_value_isinstance, BOA_OBJTYPE_INSTANCE);
        iftypefnobj(boa_value_isvargarray, BOA_OBJTYPE_VARARGARRAY);
        iftypefnobj(boa_value_isarray, BOA_OBJTYPE_ARRAY);
        iftypefnobj(boa_value_isrange, BOA_OBJTYPE_RANGE);
        iftypefnobj(boa_value_isfield, BOA_OBJTYPE_FIELD);
        iftypefnobj(boa_value_isreference, BOA_OBJTYPE_REFERENCE);
        iftypefnobj(boa_value_isexception, BOA_OBJTYPE_EXCEPTION);
        iftypefnobj(boa_value_iscallablefunction, BOA_OBJTYPE_CALLABLEFUNCTION);
    #undef iftypefn
    #undef iftypefnval
    #undef iftypefnobj
    return "unknown";
}

void boa_dynlistval_init(BoaDynListVal* list)
{
    boa_dynlistval_reset(list);
}

void boa_dynlistval_reset(BoaDynListVal* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistval_destroy(BoaDynListVal* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistval_reset(list);
}

void boa_dynlistval_push(BoaDynListVal* list, BoaValue val)
{
    size_t i;
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (BoaValue*)boa_sysmem_realloc(list->listitems, sizeof(BoaValue) * (list->listcapacity));
        for(i = oldcapacity; i < list->listcapacity; i++)
        {
            list->listitems[i] = boa_value_makenull();
        }
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}

BoaValue boa_dynlistval_get(BoaDynListVal* list, size_t idx)
{
    return list->listitems[idx];
}

BoaValue boa_dynlistval_set(BoaDynListVal* list, size_t idx, BoaValue val)
{
    list->listitems[idx] = val;
    return list->listitems[idx];
}

void boa_dynlistval_ensuresize(BoaDynListVal* list, size_t size)
{
    boa_dynlistval_ensureactualsize(list, size);
    if(list->listcount < size)
    {
        list->listcount = size;
    }
}

void boa_dynlistval_ensureactualsize(BoaDynListVal* list, size_t size)
{
    size_t i;
    size_t oldcapacity;
    if(list->listcapacity < size)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = size;
        if(list->listitems == NULL)
        {
            list->listitems = (BoaValue*)boa_sysmem_malloc(sizeof(BoaValue) * (size));            
        }
        else
        {
            list->listitems = (BoaValue*)boa_sysmem_realloc(list->listitems, sizeof(BoaValue) * (size));
        }
        for(i = oldcapacity; i < size; i++)
        {
            list->listitems[i] = boa_value_makenull();
        }
    }
}

void boa_dynlistpriv_init(BoaDynListPriv* list)
{
    boa_dynlistpriv_reset(list);
}

void boa_dynlistpriv_reset(BoaDynListPriv* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistpriv_destroy(BoaDynListPriv* list)
{
    if(list->listitems != NULL && ((list->listcount > 0) && (list->listcapacity > 0)))
    {
        boa_sysmem_free(list->listitems);
    }
    boa_dynlistpriv_reset(list);
}

void boa_dynlistpriv_push(BoaDynListPriv* list, BoaAstPrivate priv)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (BoaAstPrivate*)boa_sysmem_realloc(list->listitems, sizeof(BoaAstPrivate) * (list->listcapacity));
    }
    list->listitems[list->listcount] = priv;
    list->listcount++;
}

void boa_dynlistloc_init(BoaDynListLoc* list)
{
    boa_dynlistloc_reset(list);
}

void boa_dynlistloc_reset(BoaDynListLoc* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistloc_destroy(BoaDynListLoc* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistloc_reset(list);
}

void boa_dynlistloc_push(BoaDynListLoc* list, BoaAstLocal loc)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (BoaAstLocal*)boa_sysmem_realloc(list->listitems, sizeof(BoaAstLocal) * (list->listcapacity));
    }
    list->listitems[list->listcount] = loc;
    list->listcount++;
}

void boa_dynlistexpr_init(BoaDynListExpr* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistexpr_destroy(BoaDynListExpr* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistexpr_init(list);
}

void boa_dynlistexpr_push(BoaDynListExpr* list, BoaAstExpression* value)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (BoaAstExpression**)boa_sysmem_realloc(list->listitems, sizeof(BoaAstExpression*) * (list->listcapacity));
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}

void boa_dynlistparam_init(BoaDynListParam* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistparam_destroy(BoaDynListParam* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistparam_init(list);
}

void boa_dynlistparam_push(BoaDynListParam* list, BoaAstFuncParamExpr value)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (BoaAstFuncParamExpr*)boa_sysmem_realloc(list->listitems, sizeof(BoaAstFuncParamExpr) * (list->listcapacity));
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}

void boa_dynlistuint_init(BoaDynListUInt* list)
{
    boa_dynlistuint_reset(list);
}

void boa_dynlistuint_reset(BoaDynListUInt* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistuint_destroy(BoaDynListUInt* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistuint_reset(list);
}

void boa_dynlistuint_push(BoaDynListUInt* list, size_t val)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (size_t*)boa_sysmem_realloc(list->listitems, sizeof(size_t) * (list->listcapacity));
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}

void boa_dynlistbyte_init(BoaDynListByte* list)
{
    boa_dynlistbyte_reset(list);
}

void boa_dynlistbyte_reset(BoaDynListByte* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void boa_dynlistbyte_destroy(BoaDynListByte* list)
{
    boa_sysmem_free(list->listitems);
    boa_dynlistbyte_reset(list);
}

void boa_dynlistbyte_push(BoaDynListByte* list, uint8_t val)
{
    size_t oldcapacity;
    if(list->listcapacity < list->listcount + 1)
    {
        oldcapacity = list->listcapacity;
        list->listcapacity = boa_util_grownextcapacity(oldcapacity);
        list->listitems = (uint8_t*)boa_sysmem_realloc(list->listitems, sizeof(uint8_t) * (list->listcapacity));
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}

void boa_table_init(BoaState* state, BoaTable* table)
{
    table->pstate = state;
    boa_table_resetvars(table);
}

void boa_table_resetvars(BoaTable* table)
{
    memset(table, 0, sizeof(BoaTable));
    table->htcapacity = -1;
    table->htcount = 0;
    table->htentries = NULL;
}

void boa_free_table(BoaTable* table)
{
    if(table->htcapacity > 0)
    {
        boa_sysmem_free(table->htentries);
    }
    boa_table_resetvars(table);
}

BoaTabEntry* boa_table_findentry(BoaTabEntry* entries, int capacity, BoaString* key)
{
    uint32_t index;
    BoaTabEntry* tombstone;
    BoaTabEntry* entry;
    index = key->strhash % capacity;
    tombstone = NULL;
    while(true)
    {
        entry = &entries[index];
        if(entry->entkey == NULL)
        {
            if(boa_value_isnull(entry->entvalue))
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else if(tombstone == NULL)
            {
                tombstone = entry;
            }
        }
        if(entry->entkey == key)
        {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

void boa_table_adjustcapacity(BoaTable* table, int capacity)
{
    int i;
    BoaTabEntry* entry;
    BoaTabEntry* entries;
    BoaTabEntry* destination;
    entries = (BoaTabEntry*)boa_sysmem_malloc((capacity + 1) * sizeof(BoaTabEntry));
    for(i = 0; i <= capacity; i++)
    {
        entries[i].entkey = NULL;
        entries[i].entvalue = boa_value_makenull();
    }
    table->htcount = 0;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->entkey == NULL)
        {
            continue;
        }
        destination = boa_table_findentry(entries, capacity, entry->entkey);
        destination->entkey = entry->entkey;
        destination->entvalue = entry->entvalue;
        table->htcount++;
    }
    boa_sysmem_free(table->htentries);
    table->htcapacity = capacity;
    table->htentries = entries;
}

bool boa_table_set(BoaTable* table, BoaString* key, BoaValue value)
{
    int capacity;
    BoaTabEntry* entry;
    bool isnew;
    if(table->htcount + 1 > (table->htcapacity + 1) * BOA_CONFIG_TABLEMAXLOAD)
    {
        capacity = boa_util_grownextcapacity(table->htcapacity + 1) - 1;
        boa_table_adjustcapacity(table, capacity);
    }
    entry = boa_table_findentry(table->htentries, table->htcapacity, key);
    isnew = entry->entkey == NULL;
    if(isnew && boa_value_isnull(entry->entvalue))
    {
        table->htcount++;
    }
    entry->entkey = key;
    entry->entvalue = value;
    return isnew;
}

bool boa_table_getentry(BoaTable* table, BoaString* key, BoaValue* valuedest)
{
    BoaTabEntry* entry;
    if(table->htcount == 0)
    {
        return false;
    }
    entry = boa_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    if(valuedest != NULL)
    {
        *valuedest = entry->entvalue;
    }
    return true;
}

bool boa_table_getslot(BoaTable* table, BoaString* key, BoaValue** value)
{
    BoaTabEntry* entry;
    if(table->htcount == 0)
    {
        return false;
    }
    entry = boa_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    *value = &entry->entvalue;
    return true;
}

bool boa_table_delete(BoaTable* table, BoaString* key)
{
    BoaTabEntry* entry;
    if(table->htcount == 0)
    {
        return false;
    }
    entry = boa_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    entry->entkey = NULL;
    entry->entvalue = boa_value_makebool(true);
    return true;
}

BoaString* boa_table_findstring(BoaTable* table, const char* chars, size_t length, uint32_t hash)
{
    size_t entlen;
    const char* entstr;
    uint32_t enthash;
    uint32_t index;
    BoaTabEntry* entry;
    (void)enthash;
    if(chars == NULL)
    {
        return NULL;
    }
    if(length == 0)
    {
        return NULL;
    }
    if(table->htcount == 0)
    {
        return NULL;
    }
    index = hash % table->htcapacity;
    while(true)
    {
        entry = &table->htentries[index];
        if(entry->entkey == NULL)
        {
            if(boa_value_isnull(entry->entvalue))
            {
                return NULL;
            }
        }
        else
        {
            entlen = boa_string_getlength(entry->entkey);
            enthash = entry->entkey->strhash;
            entstr = boa_string_getdata(entry->entkey);
            if(entlen == length && /* enthash == hash && */ memcmp(entstr, chars, length) == 0)
            {
                return entry->entkey;
            }
        }
        index = (index + 1) % table->htcapacity;
    }
}

void boa_strtable_init(BoaStringTable* table)
{
    table->count = 0;
    table->capacity = -1;
    table->entries = NULL;
}

void boa_strtable_free(BoaStringTable* table)
{
    if(table->capacity > 0)
    {
        boa_sysmem_free(table->entries);
    }
    boa_strtable_init(table);
}

/*
 * probes for a free slot. hash must already be applied to the caller's string.
 * returns the first NULL (empty) or tombstone slot in the probe chain.
 */
BoaString** boa_strtable_findslot(BoaStringTable* table, const char* chars, size_t length, uint32_t hash)
{
    uint32_t index;
    BoaString** slot;
    BoaString* string;
    index = hash % table->capacity;
    while(true)
    {
        slot = &table->entries[index];
        string = *slot;
        if(string == NULL || string == BOA_STRINGTABLE_TOMBSTONE)
        {
            return slot;
        }
        if(string->strhash == hash
            && boa_string_getlength(string) == length
            && (length == 0 || memcmp(boa_string_getdata(string), chars, length) == 0))
        {
            return slot;
        }
        index = (index + 1) % table->capacity;
    }
}

BoaString* boa_strtable_find(BoaStringTable* table, const char* chars, size_t length, uint32_t hash)
{
    uint32_t index;
    BoaString* string;
    if(table->count == 0)
    {
        return NULL;
    }
    index = hash % table->capacity;
    while(true)
    {
        string = table->entries[index];
        if(string == NULL)
        {
            return NULL;
        }
        if(string != BOA_STRINGTABLE_TOMBSTONE
            && string->strhash == hash
            && boa_string_getlength(string) == length
            && (length == 0 || memcmp(boa_string_getdata(string), chars, length) == 0))
        {
            return string;
        }
        index = (index + 1) % table->capacity;
    }
}

void boa_strtable_adjustcapacity(BoaStringTable* table, int capacity)
{
    int i;
    BoaString** entries;
    BoaString* string;
    entries = (BoaString**)boa_sysmem_malloc((capacity + 1) * sizeof(BoaString*));
    for(i = 0; i <= capacity; i++)
    {
        entries[i] = NULL;
    }
    table->count = 0;
    for(i = 0; i <= table->capacity; i++)
    {
        string = table->entries[i];
        if(string == NULL || string == BOA_STRINGTABLE_TOMBSTONE)
        {
            continue;
        }
        {
            uint32_t index;
            index = string->strhash % capacity;
            while(entries[index] != NULL)
            {
                index = (index + 1) % capacity;
            }
            entries[index] = string;
        }
        table->count++;
    }
    boa_sysmem_free(table->entries);
    table->capacity = capacity;
    table->entries = entries;
}

bool boa_strtable_set(BoaStringTable* table, BoaString* string)
{
    int capacity;
    BoaString** slot;
    bool isnew;
    if(table->count + 1 > (table->capacity + 1) * BOA_CONFIG_TABLEMAXLOAD)
    {
        capacity = boa_util_grownextcapacity(table->capacity + 1) - 1;
        boa_strtable_adjustcapacity(table, capacity);
    }
    slot = boa_strtable_findslot(table, boa_string_getdata(string), boa_string_getlength(string), string->strhash);
    isnew = (*slot == NULL);
    if(isnew)
    {
        table->count++;
    }
    *slot = string;
    return isnew;
}

void boa_strtable_removewhite(BoaStringTable* table)
{
    int i;
    BoaObject* obj;
    for(i = 0; i <= table->capacity; i++)
    {
        if(table->entries[i] == NULL || table->entries[i] == BOA_STRINGTABLE_TOMBSTONE)
        {
            continue;
        }
        obj = (BoaObject*)table->entries[i];
        if(!obj->marked)
        {
            table->entries[i] = BOA_STRINGTABLE_TOMBSTONE;
            table->count--;
        }
    }
}

void boa_table_addall(BoaTable* from, BoaTable* to)
{
    int i;
    BoaTabEntry* entry;
    for(i = 0; i <= from->htcapacity; i++)
    {
        entry = &from->htentries[i];
        if(entry->entkey != NULL)
        {
            boa_table_set(to, entry->entkey, entry->entvalue);
        }
    }
}

void boa_table_addallignoring(BoaTable* from, BoaTable* to)
{
    int i;
    BoaValue fake;
    BoaTabEntry* entry;
    for(i = 0; i <= from->htcapacity; i++)
    {
        entry = &from->htentries[i];
        if(entry->entkey != NULL && !boa_table_getentry(to, entry->entkey, &fake))
        {
            boa_table_set(to, entry->entkey, entry->entvalue);
        }
    }
}

void boa_table_removewhite(BoaTable* table)
{
    int i;
    BoaObject* obj;
    BoaTabEntry* entry;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->entkey != NULL)
        {
            obj = (BoaObject*)entry->entkey;
            if(!obj->marked)
            {
                boa_table_delete(table, entry->entkey);
            }
        }
    }
}

void boa_table_markentries(BoaTable* table)
{
    int i;
    BoaState* state;
    BoaTabEntry* entry;
    state = table->pstate;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        boa_gcmem_markobject(state, (BoaObject*)entry->entkey);
        boa_gcmem_markvalue(state, entry->entvalue);
    }
}

void boa_chunk_init(BoaChunk* chunk)
{
    boa_chunk_reset(chunk);
    boa_dynlistval_init(&chunk->constantlist);
}

void boa_chunk_reset(BoaChunk* chunk)
{
    chunk->compiledcodecount = 0;
    chunk->capacity = 0;
    chunk->compiledcodechunk = NULL;
    chunk->haslineinfo = true;
    chunk->linecount = 0;
    chunk->linecapacity = 0;
    chunk->lines = NULL;
}

void boa_chunk_destroy(BoaChunk* chunk)
{
    boa_sysmem_free(chunk->compiledcodechunk);
    boa_sysmem_free(chunk->lines);
    boa_dynlistval_destroy(&chunk->constantlist);
    boa_chunk_reset(chunk);
}

void boa_chunk_push(BoaChunk* chunk, uint64_t word, uint16_t scriptsrcline)
{
    size_t oldcapacity;
    size_t lineindex;
    size_t value;

    if(chunk->capacity < chunk->compiledcodecount + 1)
    {
        oldcapacity = chunk->capacity;
        chunk->capacity = boa_util_grownextcapacity(oldcapacity);
        chunk->compiledcodechunk = (uint64_t*)boa_sysmem_realloc(chunk->compiledcodechunk, sizeof(uint64_t) * (chunk->capacity));
    }
    chunk->compiledcodechunk[chunk->compiledcodecount] = word;
    chunk->compiledcodecount++;
    if(!chunk->haslineinfo)
    {
        return;
    }
    if(chunk->linecapacity < chunk->linecount + 4)
    {
        oldcapacity = chunk->linecapacity;
        chunk->linecapacity = boa_util_grownextcapacity(chunk->linecapacity);
        chunk->lines = (uint16_t*)boa_sysmem_realloc(chunk->lines, sizeof(uint16_t) * (chunk->linecapacity));
        if(oldcapacity == 0)
        {
            chunk->lines[0] = 0;
            chunk->lines[1] = 0;
        }
    }
    lineindex = chunk->linecount;
    value = chunk->lines[lineindex];
    if(value != 0 && value != scriptsrcline)
    {
        chunk->linecount += 2;
        lineindex = chunk->linecount;
        chunk->lines[lineindex + 1] = 0;
    }
    chunk->lines[lineindex] = scriptsrcline;
    chunk->lines[lineindex + 1]++;
}

size_t boa_chunk_addconstant(BoaState* state, BoaChunk* chunk, BoaValue constant)
{
    boa_state_pushvalueroot(state, constant);
    boa_dynlistval_push(&chunk->constantlist, constant);
    boa_state_poproot(state);
    return chunk->constantlist.listcount - 1;
}

size_t boa_chunk_getline(BoaChunk* chunk, size_t offset)
{
    size_t i;
    size_t rle;
    size_t scriptsrcline;
    size_t index;
    if(!chunk->haslineinfo)
    {
        return 0;
    }
    if(chunk->linecapacity == 0)
    {
        return 0;
    }
    rle = 0;
    scriptsrcline = 0;
    index = 0;
    for(i = 0; i <= offset; i++)
    {
        if(rle > 0)
        {
            rle--;
            continue;
        }
        scriptsrcline = 0;
        rle = 0;
        if(index <= chunk->linecapacity)
        {
            scriptsrcline = chunk->lines[index];
            if((index + 1) < chunk->linecapacity)
            {
                rle = chunk->lines[index + 1];
                if(rle > 0)
                {
                    rle--;
                }
            }
        }
        else
        {
            goto finishup;
        }
        index += 2;
    }
finishup:
    return scriptsrcline;
}

void boa_chunk_shrink(BoaChunk* chunk)
{
    size_t oldcapacity;
    (void)oldcapacity;
    if(chunk->capacity > chunk->compiledcodecount)
    {
        oldcapacity = chunk->capacity;
        chunk->capacity = chunk->compiledcodecount;
        chunk->compiledcodechunk = (uint64_t*)boa_sysmem_realloc(chunk->compiledcodechunk, sizeof(uint64_t) * (chunk->capacity));
    }
    if(chunk->linecapacity > chunk->linecount)
    {
        oldcapacity = chunk->linecapacity;
        chunk->linecapacity = chunk->linecount + 2;
        chunk->lines = (uint16_t*)boa_sysmem_realloc(chunk->lines, sizeof(uint16_t) * (chunk->linecapacity));
    }
}

BOA_FORCEINLINE uint32_t boa_string_hash(const char* key, size_t length)
{
    size_t i;
    uint32_t hash;
    hash = 2166136261u;
    for(i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

void boa_stream_initvars(BoaStream* pr, BoaStrMode mode)
{
    pr->fromstack = false;
    pr->wrmode = BOA_IOSTRMODE_UNDEFINED;
    pr->shouldclose = false;
    pr->shouldflush = false;
    pr->stringtaken = false;
    pr->shortenvalues = false;
    pr->jsonmode = false;
    pr->cachedistty = false;
    pr->havecachedtty = false;
    pr->maxvallength = 15;
    pr->desthndfile = NULL;
    pr->wrmode = mode;
}

bool boa_stream_makestackio(BoaStream* pr, FILE* fh, bool shouldclose)
{
    boa_stream_initvars(pr, BOA_IOSTRMODE_FILE);
    pr->fromstack = true;
    pr->desthndfile = fh;
    pr->shouldclose = shouldclose;
    return true;
}

bool boa_stream_makestackopenfile(BoaStream* pr, const char* path, const char* mode)
{
    boa_stream_initvars(pr, BOA_IOSTRMODE_FILE);
    pr->fromstack = true;
    pr->shouldclose = true;
    pr->desthndfile = fopen(path, mode);
    if(pr->desthndfile == NULL)
    {
        return false;
    }
    return true;
}

bool boa_stream_makestackstring(BoaStream* pr)
{
    boa_stream_initvars(pr, BOA_IOSTRMODE_STRING);
    pr->fromstack = true;
    pr->wrmode = BOA_IOSTRMODE_STRING;
    boa_strbuf_makebasicemptystack(&pr->desthndstring, NULL, 0);
    return true;
}

BoaStream* boa_stream_makeundefined(BoaStrMode mode)
{
    BoaStream* pr;
    pr = (BoaStream*)boa_sysmem_malloc(sizeof(BoaStream));
    if(!pr)
    {
        fprintf(stderr, "cannot allocate BoaStream\n");
        return NULL;
    }
    boa_stream_initvars(pr, mode);
    return pr;
}

BoaStream* boa_stream_makeio(FILE* fh, bool shouldclose)
{
    BoaStream* pr;
    pr = boa_stream_makeundefined(BOA_IOSTRMODE_FILE);
    pr->desthndfile = fh;
    pr->shouldclose = shouldclose;
    return pr;
}

BoaStream* boa_stream_makeopenfile(const char* path, const char* mode)
{
    BoaStream* pr;
    pr = boa_stream_makeundefined(BOA_IOSTRMODE_FILE);
    if(boa_stream_makestackopenfile(pr, path, mode))
    {
        pr->fromstack = false;
        return pr;
    }
    else
    {
        boa_stream_destroy(pr);
    }
    return NULL;
}

BoaStream* boa_stream_makestring()
{
    BoaStream* pr;
    pr = boa_stream_makeundefined(BOA_IOSTRMODE_STRING);
    boa_strbuf_makebasicemptystack(&pr->desthndstring, NULL, 0);
    return pr;
}

void boa_stream_destroy(BoaStream* pr)
{
    if(pr == NULL)
    {
        return;
    }
    if(pr->wrmode == BOA_IOSTRMODE_UNDEFINED)
    {
        return;
    }
    /*fprintf(stderr, "boa_stream_destroy: pr->wrmode=%d\n", pr->wrmode);*/
    if(pr->wrmode == BOA_IOSTRMODE_STRING)
    {
        if(!pr->stringtaken)
        {
            boa_strbuf_destroyfromstack(&pr->desthndstring);
        }
    }
    else if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        if(pr->shouldclose)
        {
            fclose(pr->desthndfile);
        }
    }
    if(!pr->fromstack)
    {
        boa_sysmem_free(pr);
        pr = NULL;
    }
}

bool boa_stream_istty(BoaStream* pr)
{
    int fd;
    if(pr->havecachedtty)
    {
        return pr->cachedistty;
    }
    if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        if(pr->desthndfile != NULL)
        {
            fd = fileno(pr->desthndfile);
            if(fd > 0)
            {
                pr->cachedistty = isatty(fd);
                pr->havecachedtty = true;
                return pr->cachedistty;
            }
        }
    }
    return false;
}

const char* boa_stream_getcolor(BoaStream* pr, int c)
{
    int i;
    static struct {
        int ident;
        const char* name;
        const char* code;
    } colortab[] =
    {
        {'0', "reset", "\x1B[0m"},
        {'r', "red", "\x1B[31m"},
        {'g', "green", "\x1B[32m"},
        {'y', "yellow", "\x1B[33m"},
        {'b', "blue", "\x1B[34m"},
        {'m', "magenta", "\x1B[35m"},
        {'c', "cyan", "\x1B[36m"},
        {-1, NULL, NULL}
    };
    (void)pr;
    for(i=0; colortab[i].name != NULL; i++)
    {
        if(colortab[i].ident == c)
        {
            return colortab[i].code;
        }
    }
    return NULL;
}

bool boa_stream_setcolor(BoaStream* pr, int c)
{
    const char* color;
    if(boa_stream_istty(pr))
    {
        color = boa_stream_getcolor(pr, c);
        if(color != NULL)
        {
            return boa_stream_puts(pr, color);
        }
    }
    return false;
}

bool boa_stream_resetcolor(BoaStream* pr)
{
    return boa_stream_setcolor(pr, '0');
}

void boa_stream_flush(BoaStream* pr)
{
    if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        if(pr->shouldflush)
        {
            fflush(pr->desthndfile);
        }
    }
}

int boa_stream_readchar(BoaStream* pr)
{
    if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        return fgetc(pr->desthndfile);
    }
    return EOF;
}

bool boa_stream_readio(BoaStream* pr, char** destbuf, size_t* destlen, size_t howmuch)
{
    if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        *destlen = fread(*destbuf, sizeof(char), howmuch, pr->desthndfile);
        return true;
    }
    return true;
}

BoaString* boa_stream_readstring(BoaStream* pr, BoaState* state, size_t howmuch)
{
    size_t actuallen;
    BoaString* res;
    res = boa_string_makeemptystring(state, howmuch, false);
    if(boa_stream_readio(pr, &res->strbuf.data, &actuallen, howmuch))
    {
        boa_strbuf_resize(&res->strbuf, actuallen);
        return res;
    }
    return res;
}

BoaString* boa_stream_readuntil(BoaStream* hnd, BoaState* state, bool havesizeparam, size_t howmuch)
{
    size_t rsz;
    size_t length;
    BoaString* result;
    if(hnd->wrmode == BOA_IOSTRMODE_FILE)
    {
        if(havesizeparam)
        {
            length = howmuch;
        }
        else
        {
            fseek(hnd->desthndfile, 0, SEEK_END);
            length = ftell(hnd->desthndfile);
            fseek(hnd->desthndfile, 0, SEEK_SET);
        }
        result = boa_string_makeemptystring(state, length, true);
        result->strbuf.data = (char*)boa_sysmem_malloc((length + 1) * sizeof(char));
        result->strbuf.data[length] = '\0';
        result->strbuf.isshort = false;
        result->strbuf.capacity = length + 1;
        rsz = fread(result->strbuf.data, sizeof(char), length, hnd->desthndfile);
        result->strbuf.length = rsz;
        result->strhash = boa_string_hash(result->strbuf.data, result->strbuf.length);
        boa_string_register(state, result);
        return result;
    }
    return NULL;
}

bool boa_stream_putlen(BoaStream* pr, const char* estr, size_t elen)
{
    if(elen > 0)
    {
        if(pr->wrmode == BOA_IOSTRMODE_FILE)
        {
            fwrite(estr, sizeof(char), elen, pr->desthndfile);
            boa_stream_flush(pr);
        }
        else if(pr->wrmode == BOA_IOSTRMODE_STRING)
        {
            boa_strbuf_appendstrn(&pr->desthndstring, estr, elen);
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool boa_stream_puts(BoaStream* pr, const char* estr)
{
    return boa_stream_putlen(pr, estr, strlen(estr));
}

bool boa_stream_putc(BoaStream* pr, int b)
{
    char ch;
    if(pr->wrmode == BOA_IOSTRMODE_STRING)
    {
        ch = b;
        boa_stream_putlen(pr, &ch, 1);
    }
    else if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        ch = b;
        boa_stream_putlen(pr, &ch, 1);   
        boa_stream_flush(pr);
    }
    return true;
}

int boa_stream_getc(BoaStream* pr)
{
    int c;
    if(pr->wrmode == BOA_IOSTRMODE_STRING)
    {
        return -1;
    }
    else if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        c = fgetc(pr->desthndfile);
        boa_stream_flush(pr);
        return c;
    }
    return -1;
}

bool boa_stream_putescapedchar(BoaStream* pr, int ch)
{
    switch(ch)
    {
        case '\'':
            {
                boa_stream_puts(pr, "\\\'");
            }
            break;
        case '\"':
            {
                boa_stream_putc(pr, '\\');
                boa_stream_putc(pr, '"');
            }
            break;
        case '\\':
            {
                boa_stream_puts(pr, "\\\\");
            }
            break;
        case '\b':
            {
                boa_stream_puts(pr, "\\b");
            }
            break;
        case '\f':
            {
                boa_stream_puts(pr, "\\f");
            }
            break;
        case '\n':
            {
                boa_stream_puts(pr, "\\n");
            }
            break;
        case '\r':
            {
                boa_stream_puts(pr, "\\r");
            }
            break;
        case '\t':
            {
                boa_stream_puts(pr, "\\t");
            }
            break;
        case 0:
            {
                boa_stream_puts(pr, "\\0");
            }
            break;
        default:
            {
                boa_stream_printf(pr, "\\x%02x", (unsigned char)ch);
            }
            break;
    }
    return true;
}

bool boa_stream_putquotedstring(BoaStream* pr, const char* str, size_t len, bool withquot)
{
    int bch;
    size_t i;
    bch = 0;
    if(withquot)
    {
        boa_stream_putc(pr, 34);
    }
    for(i = 0; i < len; i++)
    {
        bch = str[i];
        if((bch < 32) || (bch > 127) || (bch == '\"') || (bch == '\\'))
        {
            boa_stream_putescapedchar(pr, bch);
        }
        else
        {
            boa_stream_putc(pr, bch);
        }
    }
    if(withquot)
    {
        boa_stream_putc(pr, 34);
    }
    return true;
}

bool boa_stream_vprintftostring(BoaStream* pr, const char* fmt, va_list va)
{
    boa_strbuf_appendformatv(&pr->desthndstring, fmt, va);
    return true;
}

bool boa_stream_printfv(BoaStream* pr, const char* fmt, va_list va)
{
    if(pr->wrmode == BOA_IOSTRMODE_STRING)
    {
        return boa_stream_vprintftostring(pr, fmt, va);
    }
    else if(pr->wrmode == BOA_IOSTRMODE_FILE)
    {
        vfprintf(pr->desthndfile, fmt, va);
        boa_stream_flush(pr);
    }
    return true;
}

bool boa_stream_printf(BoaStream* pr, const char* fmt, ...) BOA_ATTRIB((format(printf, 2, 3)));
bool boa_stream_printf(BoaStream* pr, const char* fmt, ...)
{
    bool b;
    va_list va;
    va_start(va, fmt);
    b = boa_stream_printfv(pr, fmt, va);
    va_end(va);
    return b;
}

BoaString* boa_stream_takestring(BoaState* state, BoaStream* pr)
{
    BoaString* os;
    uint32_t hash;
    BoaString* interned;
    hash = boa_string_hash(pr->desthndstring.data, pr->desthndstring.length);
    interned = boa_strtable_find(&state->vmstate.storedstrings, pr->desthndstring.data, pr->desthndstring.length, hash);
    if(interned != NULL)
    {
        boa_strbuf_destroyfromstack(&pr->desthndstring);
        pr->stringtaken = true;
        return interned;
    }
    os = boa_string_makewithstrbuf(state, NULL, pr->desthndstring, false);
    boa_string_register(state, os);
    pr->stringtaken = true;
    return os;
}

void boa_gcmem_deallocobject(BoaState* state, size_t typesz, void* pointer)
{
    boa_gcmem_allocate(state, pointer, typesz, 0);
}

void* boa_gcmem_allocate(BoaState* state, void* pointer, size_t oldsize, size_t newsize)
{
    void* ptr;
    state->bytesallocated += (int64_t)newsize - (int64_t)oldsize;
    if(newsize > oldsize)
    {
#if defined(BOA_CONFIG_DEBUGSTRESSTESTGC) && (BOA_CONFIG_DEBUGSTRESSTESTGC == 1)
        boa_collect_garbage(state);
#endif
        if(state->bytesallocated > state->gcnextgc)
        {
            boa_collect_garbage(state);
        }
    }
    if(newsize == 0)
    {
        boa_sysmem_free(pointer);
        return NULL;
    }
    ptr = boa_sysmem_realloc(pointer, newsize);
    if(ptr == NULL)
    {
        boa_state_raiseerror(state, "fatal error: out of memory! aborting now\n");
        abort();
    }
    return ptr;
}

void boa_gcmem_freeobject(BoaState* state, BoaObject* object)
{
#if defined(BOA_CONFIG_DEBUGLOGALLOCATION) && (BOA_CONFIG_DEBUGLOGALLOCATION == 1)
    fprintf(stderr, "(%s) %p free %s\n", boa_value_objtypename(object->type), (void*)object, boa_value_objtypename(object->type));
#endif
    switch(object->type)
    {
        case BOA_OBJTYPE_STRING:
            {
                BoaString* optr;
                optr = (BoaString*)object;
                boa_string_destroy(optr);
                boa_gcmem_deallocobject(state, sizeof(BoaString), optr);
            }
            break;
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                BoaFuncScript* optr;
                optr = (BoaFuncScript*)object;
                boa_chunk_destroy(&optr->chunk);
                boa_gcmem_deallocobject(state, sizeof(BoaFuncScript), optr);
            }
            break;
        case BOA_OBJTYPE_FUNCNATIVE:
            {
                BoaFuncNative* optr;
                optr = (BoaFuncNative*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaFuncNative), optr);
            }
            break;
        case BOA_OBJTYPE_FUNCNATMETHOD:
            {
                BoaFuncNative* optr;
                optr = (BoaFuncNative*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaFuncNative), optr);
            }
            break;
        case BOA_OBJTYPE_FIBER:
            {
                BoaFiber* optr;
                optr = (BoaFiber*)object;
                boa_sysmem_free(optr->framevals);
                boa_sysmem_free(optr->registeritems);
                if(optr->handleritems != NULL)
                {
                    boa_sysmem_free(optr->handleritems);
                }
                boa_gcmem_deallocobject(state, sizeof(BoaFiber), optr);
            }
            break;
        case BOA_OBJTYPE_MODULE:
            {
                BoaModule* optr;
                optr = (BoaModule*)object;
                boa_sysmem_free(optr->privatevalues);
                boa_gcmem_deallocobject(state, sizeof(BoaModule), optr);
            }
            break;
        case BOA_OBJTYPE_FUNCCLOSURE:
            {
                BoaFuncClosure* optr;
                optr = (BoaFuncClosure*)object;
                boa_sysmem_free(optr->closureupvalueitems);
                boa_gcmem_deallocobject(state, sizeof(BoaFuncClosure), optr);
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                BoaClsPrototype* optr;
                optr = (BoaClsPrototype*)object;
                boa_sysmem_free(optr->indexes);
                boa_sysmem_free(optr->local);
                boa_gcmem_deallocobject(state, sizeof(BoaClsPrototype), optr);
            }
            break;
        case BOA_OBJTYPE_UPVALUE:
            {
                BoaUpvalue* optr;
                optr = (BoaUpvalue*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaUpvalue), optr);
            }
            break;
        case BOA_OBJTYPE_CLASS:
            {
                BoaClass* optr = (BoaClass*)object;
                boa_free_table(&optr->mthtable);
                boa_free_table(&optr->staticstable);
                boa_gcmem_deallocobject(state, sizeof(BoaClass), optr);
            }
            break;
        case BOA_OBJTYPE_INSTANCE:
            {
                BoaInstance* optr;
                optr = (BoaInstance*)object;
                boa_free_table(&optr->fields);
                boa_gcmem_deallocobject(state, sizeof(BoaInstance), optr);
            }
            break;
        case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            {
                BoaFuncBound* optr;
                optr = (BoaFuncBound*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaFuncBound), optr);
            }
            break;
        case BOA_OBJTYPE_ARRAY:
            {
                BoaArray* optr;
                optr = (BoaArray*)object;
                boa_array_destroy(optr);
                boa_gcmem_deallocobject(state, sizeof(BoaArray), optr);
            }
            break;
        case BOA_OBJTYPE_VARARGARRAY:
            {
                BoaVarargArray* optr;
                optr = (BoaVarargArray*)object;
                boa_array_destroy(&optr->innerarray);
                boa_gcmem_deallocobject(state, sizeof(BoaVarargArray), optr);
            }
            break;
        case BOA_OBJTYPE_MAP:
            {
                BoaMap* optr;
                optr = (BoaMap*)object;
                boa_free_table(&optr->innertable);
                boa_gcmem_deallocobject(state, sizeof(BoaMap), optr);
            }
            break;
        case BOA_OBJTYPE_USERDATA:
            {
                BoaUserdata* optr;
                optr = (BoaUserdata*)object;
                if(optr->oncleanupfn != NULL)
                {
                    optr->oncleanupfn(state, optr, false);
                }
                if(optr->size > 0)
                {
                    boa_sysmem_free(optr->data);
                }
                boa_gcmem_deallocobject(state, sizeof(BoaUserdata), optr);
            }
            break;
        case BOA_OBJTYPE_RANGE:
            {
                BoaRange* optr;
                optr = (BoaRange*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaRange), optr);
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                BoaField* optr;
                optr = (BoaField*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaField), optr);
            }
            break;
        case BOA_OBJTYPE_REFERENCE:
            {
                BoaReference* optr;
                optr = (BoaReference*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaReference), optr);
            }
            break;
        case BOA_OBJTYPE_EXCEPTION:
            {
                BoaException* optr;
                optr = (BoaException*)object;
                boa_gcmem_deallocobject(state, sizeof(BoaException), optr);                
            }
            break;
        default:
            {
                BOA_UTIL_UNREACHABLE();
            }
            break;
    }
}

void boa_gcmem_freeobjlist(BoaState* state, BoaObject* objects)
{
    BoaObject* next;
    BoaObject* curr;
    curr = objects;
    while(curr != NULL)
    {
        next = curr->next;
        boa_gcmem_freeobject(state, curr);
        curr = next;
    }
    boa_sysmem_free(state->vmstate.gcgraystack);
    state->vmstate.gcgraycapacity = 0;
}

void boa_gcmem_markobject(BoaState* state, BoaObject* object)
{
    if(object == NULL || object->marked)
    {
        return;
    }
    object->marked = true;
#if defined(BOA_CONFIG_DEBUGLOGMARKING) && (BOA_CONFIG_DEBUGLOGMARKING == 1)
    fprintf(stderr, "%p mark ", (void*)object);
    boa_value_printvalue(state->streamstderr, boa_value_fromobject(object), true);
    fprintf(stderr, "\n");
#endif
    if(state->vmstate.gcgraycapacity < state->vmstate.gcgraycount + 1)
    {
        state->vmstate.gcgraycapacity = boa_util_grownextcapacity(state->vmstate.gcgraycapacity);
        state->vmstate.gcgraystack = (BoaObject**)boa_sysmem_realloc(state->vmstate.gcgraystack, sizeof(BoaObject*) * state->vmstate.gcgraycapacity);
    }
    state->vmstate.gcgraystack[state->vmstate.gcgraycount++] = object;
}

void boa_gcmem_markvalue(BoaState* state, BoaValue value)
{
    if(boa_value_isobject(value))
    {
        boa_gcmem_markobject(state, boa_value_asobject(value));
    }
}

void boa_gcmem_markroots(BoaState* state)
{
    size_t i;
    for(i = 0; i < state->rootcount; i++)
    {
        boa_gcmem_markvalue(state, state->rootvalues[i]);
    }
    boa_gcmem_markobject(state, (BoaObject*)state->vmstate.fiber);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassclass);
    boa_gcmem_markobject(state, (BoaObject*)state->stdobjectclass);
    boa_gcmem_markobject(state, (BoaObject*)state->stdnullclass);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassnumber);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassstring);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassbool);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassfunction);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassfiber);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassmodule);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassarray);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassmap);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassrange);
    boa_gcmem_markobject(state, (BoaObject*)state->stdclassregex);
    boa_gcmem_markobject(state, (BoaObject*)state->exceptions.stdexception);
    boa_gcmem_markobject(state, (BoaObject*)state->exceptions.stdioerror);
    boa_gcmem_markobject(state, (BoaObject*)state->exceptions.stdargumenterror);
    boa_gcmem_markobject(state, (BoaObject*)state->apiname);
    boa_gcmem_markobject(state, (BoaObject*)state->apifunction);
    boa_table_markentries(&state->vmstate.modules->innertable);
    boa_table_markentries(&state->vmstate.globals->innertable);
}

void boa_gcmem_markarray(BoaState* state, BoaDynListVal* list)
{
    size_t i;
    for(i = 0; i < list->listcount; i++)
    {
        boa_gcmem_markvalue(state, list->listitems[i]);
    }
}

void boa_gcmem_blackenobject(BoaState* state, BoaObject* object)
{
#if defined(BOA_CONFIG_DEBUGLOGBLACKING) && (BOA_CONFIG_DEBUGLOGBLACKING == 1)
    fprintf(stderr, "%p blacken ", (void*)object);
    boa_value_printvalue(state->streamstderr, boa_value_fromobject(object), true);
    fprintf(stderr, "\n");
#endif
    switch(object->type)
    {
        case BOA_OBJTYPE_FUNCNATIVE:
        case BOA_OBJTYPE_FUNCNATMETHOD:
        case BOA_OBJTYPE_RANGE:
        case BOA_OBJTYPE_STRING:
            {
            }
            break;
        case BOA_OBJTYPE_USERDATA:
            {
                BoaUserdata* data;
                data = (BoaUserdata*)object;
                if(data->oncleanupfn != NULL)
                {
                    data->oncleanupfn(state, data, true);
                }
            }
            break;
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                BoaFuncScript* function;
                function = (BoaFuncScript*)object;
                boa_gcmem_markobject(state, (BoaObject*)function->name);
                boa_gcmem_markarray(state, &function->chunk.constantlist);
            }
            break;
        case BOA_OBJTYPE_FIBER:
            {
                size_t i;
                BoaFiber* fiber;
                BoaUpvalue* upvalue;
                fiber = (BoaFiber*)object;
                for(i = 0; i < fiber->registersallocated; i++)
                {
                    boa_gcmem_markvalue(state, fiber->registeritems[i]);
                }
                for(i = 0; i < fiber->framecount; i++)
                {
                    BoaCallFrame* frame = &fiber->framevals[i];
                    if(frame->closure != NULL)
                    {
                        boa_gcmem_markobject(state, (BoaObject*)frame->closure);
                    }
                    else
                    {
                        boa_gcmem_markobject(state, (BoaObject*)frame->function);
                    }
                }
                for(upvalue = fiber->openupvalues; upvalue != NULL; upvalue = upvalue->next)
                {
                    boa_gcmem_markobject(state, (BoaObject*)upvalue);
                }
                boa_gcmem_markvalue(state, fiber->error);
                boa_gcmem_markobject(state, (BoaObject*)fiber->module);
                boa_gcmem_markobject(state, (BoaObject*)fiber->parent);
            }
            break;
        case BOA_OBJTYPE_MODULE:
            {
                size_t i;
                BoaModule* module;
                module = (BoaModule*)object;
                boa_gcmem_markvalue(state, module->returnvalue);
                boa_gcmem_markobject(state, (BoaObject*)module->name);
                boa_gcmem_markobject(state, (BoaObject*)module->mainfunction);
                boa_gcmem_markobject(state, (BoaObject*)module->mainfiber);
                boa_gcmem_markobject(state, (BoaObject*)module->privatenames);
                for(i = 0; i < module->privatecount; i++)
                {
                    boa_gcmem_markvalue(state, module->privatevalues[i]);
                }
            }
            break;
        case BOA_OBJTYPE_FUNCCLOSURE:
            {
                size_t i;
                BoaFuncClosure* closure;
                closure = (BoaFuncClosure*)object;
                boa_gcmem_markobject(state, (BoaObject*)closure->function);
                /* Check for NULL is needed for a really specific gc-case */
                if(closure->closureupvalueitems != NULL)
                {
                    for(i = 0; i < closure->upvaluecount; i++)
                    {
                        boa_gcmem_markobject(state, (BoaObject*)closure->closureupvalueitems[i]);
                    }
                }
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                boa_gcmem_markobject(state, (BoaObject*)((BoaFuncClosure*)object)->function);
            }
            break;
        case BOA_OBJTYPE_UPVALUE:
            {
                boa_gcmem_markvalue(state, ((BoaUpvalue*)object)->closed);
            }
            break;
        case BOA_OBJTYPE_CLASS:
            {
                BoaClass* klass;
                klass = (BoaClass*)object;
                boa_gcmem_markobject(state, (BoaObject*)klass->name);
                boa_gcmem_markobject(state, (BoaObject*)klass->super);
                boa_table_markentries(&klass->mthtable);
                boa_table_markentries(&klass->staticstable);
            }
            break;
        case BOA_OBJTYPE_INSTANCE:
            {
                BoaInstance* instance;
                instance = (BoaInstance*)object;
                boa_gcmem_markobject(state, (BoaObject*)instance->klass);
                boa_table_markentries(&instance->fields);
            }
            break;
        case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            {
                BoaFuncBound* boundmethod;
                boundmethod = (BoaFuncBound*)object;
                boa_gcmem_markvalue(state, boundmethod->receiver);
                boa_gcmem_markvalue(state, boundmethod->method);
            }
            break;
        case BOA_OBJTYPE_ARRAY:
            {
                BoaArray* optr;
                optr = (BoaArray*)object;
                boa_gcmem_markarray(state, &optr->innerlist);
            }
            break;

        case BOA_OBJTYPE_VARARGARRAY:
            {
                BoaVarargArray* optr;
                optr = (BoaVarargArray*)object;
                boa_gcmem_markarray(state, &optr->innerarray.innerlist);
            }
            break;
        case BOA_OBJTYPE_MAP:
            {
                boa_table_markentries(&((BoaMap*)object)->innertable);
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                BoaField* field;
                field = (BoaField*)object;
                boa_gcmem_markobject(state, (BoaObject*)field->getter);
                boa_gcmem_markobject(state, (BoaObject*)field->setter);
            }
            break;
        case BOA_OBJTYPE_REFERENCE:
            {
                BoaReference* optr;
                optr = (BoaReference*)object;
                boa_gcmem_markvalue(state, *(optr->slot));
            }
            break;
        case BOA_OBJTYPE_EXCEPTION:
            {
                BoaException* exception;
                exception = (BoaException*)object;
                boa_gcmem_markobject(state, (BoaObject*)exception->baseclass);
                boa_gcmem_markvalue(state, exception->message);
            }
            break;
        default:
            {
                boa_vm_raisefatalerror(state, "unknown object with type %i", object->type);
            }
            break;
    }
}

void boa_gcmem_tracereferences(BoaState* state)
{
    BoaObject* object;
    while(state->vmstate.gcgraycount > 0)
    {
        object = state->vmstate.gcgraystack[--state->vmstate.gcgraycount];
        boa_gcmem_blackenobject(state, object);
    }
}

void boa_gcmem_sweep(BoaState* state)
{
    BoaObject* object;
    BoaObject* previous;
    BoaObject* unreached;
    previous = NULL;
    object = state->vmstate.objects;
    while(object != NULL)
    {
        if(object->marked)
        {
            object->marked = false;
            previous = object;
            object = object->next;
        }
        else
        {
            unreached = object;
            object = object->next;
            if(previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                state->vmstate.objects = object;
            }
            boa_gcmem_freeobject(state, unreached);
        }
    }
}

uint64_t boa_collect_garbage(BoaState* state)
{
    uint64_t before;
    uint64_t collected;
#if defined(BOA_CONFIG_DEBUGLOGGC) && (BOA_CONFIG_DEBUGLOGGC == 1)
    clock_t t;
#endif
    if(!state->gcallowgc)
    {
        return 0;
    }
    state->gcallowgc = false;
    before = state->bytesallocated;
#if defined(BOA_CONFIG_DEBUGLOGGC) && (BOA_CONFIG_DEBUGLOGGC == 1)
    fprintf(stderr, "-- gc begin\n");
    t = clock();
#endif
    boa_gcmem_markroots(state);
    boa_gcmem_tracereferences(state);
    boa_strtable_removewhite(&state->vmstate.storedstrings);
    boa_gcmem_sweep(state);
    state->gcnextgc = state->bytesallocated * BOA_CONFIG_GCHEAPGROWFACTOR;
    state->gcallowgc = true;
    collected = before - state->bytesallocated;
#if defined(BOA_CONFIG_DEBUGLOGGC) && (BOA_CONFIG_DEBUGLOGGC == 1)
    fprintf(stderr, "-- gc end. Collected %imb (%ib) in %gms\n", ((int)((collected / 1024.0 + 0.5) / 10)) * 10, collected, (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
#endif
    return collected;
}

BoaString* boa_string_makewithstrbuf(BoaState* state, BoaString* target, BoaStrBuffer sb, bool interned)
{
    BoaString* sobj;
    uint32_t hash;
    hash = boa_string_hash(sb.data, sb.length);
    if((target != NULL) && interned)
    {
        sobj = target;
        memset(sobj, 0, sizeof(BoaString));
        ((BoaObject*)sobj)->type = BOA_OBJTYPE_STRING;
    }
    else
    {
        sobj = (BoaString*)boa_object_allocobject(state, sizeof(BoaString), BOA_OBJTYPE_STRING);
    }
    sobj->strhash = hash;
    sobj->strbuf = sb;
    if (sobj->strbuf.isshort)
    {
        sobj->strbuf.data = sobj->strbuf.sso;
    }
    if(interned)
    {
        boa_string_register(state, sobj);
    }
    return sobj;
}

BoaString* boa_string_makeemptystringactual(BoaState* state, size_t length, bool preallocated, bool interned)
{
    BoaStrBuffer sb;
    boa_strbuf_initbasicempty(&sb, length, false, preallocated);
    return boa_string_makewithstrbuf(state, NULL, sb, interned);
}

BoaString* boa_string_makeemptystring(BoaState* state, size_t length, bool preallocated)
{
    return boa_string_makeemptystringactual(state, length, preallocated, false);
}

void boa_string_register(BoaState* state, BoaString* sobj)
{
    BoaString* tmp;
    const char* sdata;
    size_t slen;
    if(sobj->strhash == 0)
    {
        sobj->strhash = boa_string_hash(boa_string_getdata(sobj), boa_string_getlength(sobj));
    }
    sdata = boa_string_getdata(sobj);
    slen = boa_string_getlength(sobj);
    tmp = boa_strtable_find(&state->vmstate.storedstrings, sdata, slen, sobj->strhash);
    if(tmp != NULL)
    {
        return;
    }

    boa_state_pushroot(state, (BoaObject*)sobj);
    boa_strtable_set(&state->vmstate.storedstrings, sobj);
    boa_state_poproot(state);
}

BoaString* boa_string_makestringfrom(BoaState* state, char* chars, size_t length, uint32_t hash, bool preallocated)
{
    BoaString* sobj;
    sobj = boa_string_makeemptystring(state, length, preallocated);
    if(preallocated)
    {
        boa_strbuf_setdata(&sobj->strbuf, (char*)chars);
        boa_strbuf_setlength(&sobj->strbuf, length);
    }
    else
    {
        boa_strbuf_appendstrn(&sobj->strbuf, chars, length);
    }
    sobj->strhash = hash;
    boa_string_register(state, sobj);
    return sobj;
}

void boa_string_destroy(BoaString* str)
{
    boa_strbuf_destroyfromstack(&str->strbuf);
}

BoaString* boa_string_take(BoaState* state, char* chars, size_t length)
{
    uint32_t hash;
    BoaString* interned;
    hash = boa_string_hash(chars, length);
    interned = boa_strtable_find(&state->vmstate.storedstrings, chars, length, hash);
    if(interned != NULL)
    {
        boa_sysmem_free(chars);
        return interned;
    }
    return boa_string_makestringfrom(state, (char*)chars, length, hash, true);
}

BoaString* boa_string_copylen(BoaState* state, const char* chars, size_t length)
{
    uint32_t hash;
    char* heapchars;
    BoaString* res;
    hash = boa_string_hash(chars, length);
    res = boa_strtable_find(&state->vmstate.storedstrings, chars, length, hash);
    if(res != NULL)
    {
        return res;
    }
    heapchars = (char*)boa_sysmem_malloc((length + 1) * sizeof(char));
    memcpy(heapchars, chars, length);
    heapchars[length] = '\0';
#if defined(BOA_CONFIG_DEBUGLOGALLOCATION) && (BOA_CONFIG_DEBUGLOGALLOCATION == 1)
    printf("allocated new string <%s>\n", chars);
#endif
    return boa_string_makestringfrom(state, heapchars, length, hash, true);
}

BoaString* boa_string_copy(BoaState* state, const char* chars)
{
    return boa_string_copylen(state, chars, strlen(chars));
}

BoaString* boa_string_maketemplen(BoaState* state, BoaString* stacktarget, const char* chars, size_t length)
{
    uint32_t hash;
    BoaString* res;
    BoaStrBuffer sb;
    hash = boa_string_hash(chars, length);
    res = boa_strtable_find(&state->vmstate.storedstrings, chars, length, hash);
    if(res != NULL)
    {
        return res;
    }
    boa_strbuf_initbasicempty(&sb, length, false, true);
    boa_strbuf_setdata(&sb, (char*)chars);
    boa_strbuf_setlength(&sb, length);
    return boa_string_makewithstrbuf(state, stacktarget, sb, true);
}

BoaString* boa_string_maketemp(BoaState* state, BoaString* stacktarget, const char* chars)
{
    return boa_string_maketemplen(state, stacktarget, chars, strlen(chars));
}

BOA_FORCEINLINE size_t boa_string_getlength(BoaString* string)
{
    return string->strbuf.length;
}

BOA_FORCEINLINE void boa_string_setlength(BoaString* string, size_t ns)
{
    string->strbuf.length = ns;
}

BOA_FORCEINLINE char* boa_string_getdata(BoaString* string)
{
    return string->strbuf.data;
}

BOA_FORCEINLINE BoaStrBuffer* boa_string_getstrbuf(BoaString* string)
{
    return &string->strbuf;
}

BOA_FORCEINLINE char boa_string_getat(BoaString* string, size_t pos)
{
    return string->strbuf.data[pos];
}

BoaString* boa_string_clone(BoaState* state, BoaString* string)
{
    size_t slen;
    const char* sdata;
    slen = string->strbuf.length;
    sdata = string->strbuf.data;
    return boa_string_copylen(state, sdata, slen);
}

size_t boa_string_utflength(BoaString* string)
{
    size_t i;
    size_t slen;
    size_t length;
    const char* sdata;
    length = 0;
    slen = boa_string_getlength(string);
    sdata = boa_string_getdata(string);
    for(i=0; i < slen;)
    {
        i += boa_util_utfchargetcountdecode(sdata[i]);
        length++;
    }
    return length;
}

int boa_string_codepointcodeat(BoaState* state, BoaString* string, uint32_t index)
{
    size_t slen;
    int codepoint;
    const char* sdata;
    (void)state;
    slen = boa_string_getlength(string);
    sdata = boa_string_getdata(string);
    if(index >= slen)
    {
        return 0;
    }
    codepoint = boa_util_utfstrdecode((uint8_t*)sdata + index, slen - index);
    if(codepoint == -1)
    {
        return sdata[index];
    }
    return codepoint;
}

BoaString* boa_string_codepointstringat(BoaState* state, BoaString* string, uint32_t index)
{
    size_t slen;
    int codepoint;
    const char* sdata;
    slen = boa_string_getlength(string);
    sdata = boa_string_getdata(string);
    if(index >= slen)
    {
        return NULL;
    }
    codepoint = boa_util_utfstrdecode((uint8_t*)sdata + index, slen - index);
    if(codepoint == -1)
    {
        char bytes[2];
        bytes[0] = sdata[index];
        bytes[1] = '\0';
        return boa_string_copylen(state, bytes, 1);
    }
    return boa_string_fromcodepoint(state, codepoint);
}

BoaString* boa_string_fromcodepoint(BoaState* state, int value)
{
    uint8_t length;
    BoaString* res;
    length = boa_util_utfchargetcountencode(value);
    res = boa_string_makeemptystring(state, length, false);
    boa_util_utfcharwritebytes(value, (uint8_t*)boa_string_getdata(res));
    boa_string_setlength(res, length);
    return res;
}

BoaString* boa_string_fromrange(BoaState* state, BoaString* source, int start, uint32_t count)
{
    int index;
    int length;
    int codepoint;
    uint32_t i;
    uint8_t* to;
    uint8_t* from;
    BoaString* res;
    from = (uint8_t*)boa_string_getdata(source);
    length = 0;
    for(i = 0; i < count; i++)
    {
        length += boa_util_utfchargetcountdecode(from[start + i]);
    }
    res = boa_string_makeemptystring(state, length, false);
    to = (uint8_t*)boa_string_getdata(res);
    for(i = 0; i < count; i++)
    {
        index = start + i;
        codepoint = boa_util_utfstrdecode(from + index, boa_string_getlength(source) - index);
        if(codepoint != -1)
        {
            to += boa_util_utfcharwritebytes(codepoint, to);
        }
    }
    boa_string_setlength(res, length);
    return res;
}

void boa_string_appendlen(BoaString* dest, const char* str, size_t len)
{
    boa_strbuf_appendstrn(&dest->strbuf, str, len);
}

void boa_string_append(BoaString* dest, const char* str)
{
    boa_string_appendlen(dest, str, strlen(str));
}

void boa_string_appendbyte(BoaString* dest, int b)
{
    char c;
    c = b;
    boa_string_appendlen(dest, &c, 1);
}

BoaValue boa_string_numbertostring(BoaState* state, BoaNumber value)
{
    BoaStream pr;
    boa_stream_makestackstring(&pr);
    boa_value_printnumber(&pr, value);
    return boa_value_fromobject(boa_stream_takestring(state, &pr));
}

BoaValue boa_string_valformat(BoaState* state, const char* format, ...)
{
    va_list arglist;    
    bool wasallowed;
    const char* c;
    BoaString* result;
    wasallowed = state->gcallowgc;
    state->gcallowgc = false;
    result = boa_string_makeemptystring(state, 10, false);
    va_start(arglist, format);
    for(c = format; *c != '\0'; c++)
    {
        switch(*c)
        {
            case '$':
            {
                const char* string;
                size_t length;
                string = va_arg(arglist, const char*);
                if(string != NULL)
                {
                    length = strlen(string);
                    boa_string_appendlen(result, string, length);
                    break;
                }
                goto defaultendingcopying;
            }
            case '@':
            {
                BoaString* string;
                string = boa_value_asstring(va_arg(arglist, BoaValue));
                if(string != NULL)
                {
                    boa_string_appendlen(result, boa_string_getdata(string), boa_string_getlength(string));
                    break;
                }
                goto defaultendingcopying;
            }
            case '#':
            {
                BoaString* string;
                string = boa_value_asstring(boa_string_numbertostring(state, va_arg(arglist, BoaNumber)));
                boa_string_appendlen(result, boa_string_getdata(string), boa_string_getlength(string));
                break;
            }
            default:
            {
            defaultendingcopying:
                boa_string_appendbyte(result, *c);
                break;
            }
        }
    }
    va_end(arglist);
    result->strhash = boa_string_hash(boa_string_getdata(result), boa_string_getlength(result));
    boa_string_register(state, result);
    state->gcallowgc = wasallowed;
    return boa_value_fromobject(result);
}

void boa_value_printobjtable(BoaStream* pr, BoaObject* self, BoaTable* tab)
{
    bool didprint;
    size_t i;
    size_t index;
    size_t valueamount;
    BoaValue field;
    BoaTabEntry* entry;
    valueamount = tab->htcount;
    boa_stream_puts(pr, "{");
    if(valueamount > 0)
    {
        i = 0;
        index = 0;
        do
        {
            entry = &tab->htentries[index];
            index++;
            didprint =false;
            if(entry->entkey != NULL)
            {
                /* Special hidden key */
                field = entry->entvalue;
                boa_stream_putlen(pr, boa_string_getdata(entry->entkey), boa_string_getlength(entry->entkey));
                boa_stream_puts(pr, ": ");
                if((boa_value_ismap(field) && (boa_value_asobject(field) == self)))
                {
                    boa_stream_puts(pr, "<recursion>");
                }
                else
                {
                    boa_value_printvalue(pr, field, true);
                }
                i++;
                didprint = true;
            }
            if(didprint)
            {
                if((i + 0) < valueamount)
                {
                    boa_stream_puts(pr, ",");
                }
            }
        } while(i < valueamount);
    }
    boa_stream_puts(pr, "}");
}

void boa_value_printobjinstance(BoaStream* pr, BoaClass* klass, BoaInstance* self)
{
    BoaString* sr;
    BoaState* state;
    const char* clname;
    clname = "undefined";
    (void)state;
    state = ((BoaObject*)self)->pstate;
    sr = NULL;
    #if 0
    sr = boa_value_tostrinvoketostring(state, boa_value_fromobject(self), 2, false);
    #endif
    if(sr != NULL)
    {
        boa_stream_putlen(pr, boa_string_getdata(sr), boa_string_getlength(sr));
    }
    else
    {
        if(klass != NULL)
        {
            clname = boa_string_getdata(klass->name);
        }
        boa_stream_printf(pr, "<instance of %s: ", clname);
        boa_value_printobjtable(pr, (BoaObject*)self, &self->fields);
        boa_stream_printf(pr, "  >");
    }
}

void boa_value_printobjarray(BoaStream* pr, BoaArray* self)
{
    size_t i;
    size_t valueamount;
    BoaValue field;
    BoaDynListVal* vdlist;
    valueamount = self->innerlist.listcount;
    vdlist = &self->innerlist;
    boa_stream_puts(pr, "[");
    if(vdlist->listcount > 0)
    {
        for(i = 0; i < valueamount; i++)
        {
            field = vdlist->listitems[i];
            if(boa_value_isarray(field) && boa_value_asarray(field) == self)
            {
                boa_stream_puts(pr, "<recursion>");
            }
            else
            {
                boa_value_printvalue(pr, field, true);
            }
            if((i + 1) < valueamount)
            {
                boa_stream_puts(pr, ", ");
            }
        }
    }
    boa_stream_puts(pr, "]");
}

void boa_value_printobject(BoaStream* pr, BoaValue value, bool reprmode)
{
    BoaState* state;
    BoaObject* object;
    (void)state;
    object = boa_value_asobject(value);
    if(object == NULL)
    {
        boa_stream_puts(pr, "<!!NULLOBJECT!!>");
        return;
    }
    state = object->pstate;
    switch(object->type)
    {
        case BOA_OBJTYPE_STRING:
            {
                BoaString* str;
                str = boa_value_asstring(value);
                if(reprmode)
                {
                    boa_stream_putquotedstring(pr, boa_string_getdata(str), boa_string_getlength(str), true);
                }
                else
                {
                    boa_stream_putlen(pr, boa_string_getdata(str), boa_string_getlength(str));
                }
            }
            break;
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                BoaFuncScript* fn;
                fn = boa_value_asfuncscript(value);
                boa_stream_printf(pr, "function %s", boa_string_getdata(fn->name));
            }
            break;
        case BOA_OBJTYPE_FUNCCLOSURE:
            {
                BoaFuncClosure* fn;
                fn = boa_value_asfuncclosure(value);
                boa_stream_printf(pr, "closure %s", boa_string_getdata(fn->function->name));
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                BoaClsPrototype* fnprot;
                fnprot = boa_value_asclsproto(value);
                boa_stream_printf(pr, "closure %s", boa_string_getdata(fnprot->function->name));
            }
            break;
        case BOA_OBJTYPE_FUNCNATIVE:
            {
                BoaFuncNative* fn;
                fn = boa_value_asfuncnative(value);
                boa_stream_printf(pr, "function %s", boa_string_getdata(fn->name));
            }
            break;
        case BOA_OBJTYPE_FUNCNATMETHOD:
            {
                BoaFuncNative* fn;
                fn = boa_value_asfuncmethod(value);
                boa_stream_printf(pr, "function %s", boa_string_getdata(fn->name));
            }
            break;
        case BOA_OBJTYPE_FIBER:
            {
                BoaFiber* fiber;
                (void)fiber;
                fiber = boa_value_asfiber(value);
                boa_stream_printf(pr, "<fiber>");
            }
            break;
        case BOA_OBJTYPE_MODULE:
            {
                BoaModule* mod;
                mod = boa_value_asmodule(value);
                boa_stream_printf(pr, "<module %s>", boa_string_getdata(mod->name));
            }
            break;
        case BOA_OBJTYPE_UPVALUE:
            {
                BoaUpvalue* upvalue;
                upvalue = boa_value_asupvalue(value);
                boa_stream_puts(pr, "<upvalue to ");
                if(upvalue->location == NULL)
                {
                    boa_stream_puts(pr, " closed ");
                    boa_value_printvalue(pr, upvalue->closed, reprmode);
                }
                else
                {
                    boa_stream_puts(pr, " location ");
                    boa_value_printobject(pr, *upvalue->location, reprmode);
                }
                boa_stream_puts(pr, ">");
            }
            break;
        case BOA_OBJTYPE_CLASS:
            {
                BoaClass* klass;
                klass = boa_value_asclass(value);
                boa_stream_printf(pr, "<class %s>", boa_string_getdata(klass->name));
            }
            break;
        case BOA_OBJTYPE_INSTANCE:
            {
                BoaInstance* inst;
                inst = boa_value_asinstance(value);
                boa_value_printobjinstance(pr, inst->klass, inst);
            }
            break;
        case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            {
                BoaFuncBound* fn;
                fn = boa_value_asfuncboundmethod(value);
                boa_value_printvalue(pr, fn->method, reprmode);
            }
            break;
        case BOA_OBJTYPE_VARARGARRAY:
        case BOA_OBJTYPE_ARRAY:
            {
                BoaArray* array = boa_value_asarray(value);
                boa_value_printobjarray(pr, array);
            }
            break;
        case BOA_OBJTYPE_MAP:
            {
                BoaMap* map;
                map = boa_value_asmap(value);
                boa_value_printobjtable(pr, (BoaObject*)map, &map->innertable);
            }
            break;

        case BOA_OBJTYPE_USERDATA:
            {
                BoaUserdata* ud;
                (void)ud;
                ud = boa_value_asuserdata(value);
                boa_stream_printf(pr, "<userdata>");
            }
            break;
        case BOA_OBJTYPE_RANGE:
            {
                BoaRange* range;
                range = boa_value_asrange(value);
                boa_stream_printf(pr, "%g .. %g", range->from, range->to);
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                BoaField* field;
                field = boa_value_asfield(value);
                boa_stream_printf(pr, "<field ");
                boa_value_printvalue(pr, boa_value_fromobject(&field->innerobject), true);
                if(field->getter != NULL)
                {
                    boa_stream_puts(pr, " getter=");
                    boa_value_printvalue(pr, boa_value_fromobject(field->getter), true);
                }
                if(field->setter != NULL)
                {
                    boa_stream_puts(pr, " setter=");
                    boa_value_printvalue(pr, boa_value_fromobject(field->getter), true);
                }
                boa_stream_puts(pr, ">");
            }
            break;
        case BOA_OBJTYPE_REFERENCE:
            {
                BoaValue* slot;
                boa_stream_printf(pr, "<reference to ");
                slot = boa_value_asreference(value)->slot;
                if(slot == NULL)
                {
                    boa_stream_printf(pr, "null");
                }
                else
                {
                    boa_value_printvalue(pr, *slot, true);
                }
                boa_stream_puts(pr, ">");
            }
            break;
        case BOA_OBJTYPE_EXCEPTION:
            {
                BoaException* exception;
                exception = boa_value_asexception(value);
                if(exception->baseclass != NULL && exception->baseclass->name != NULL)
                {
                    boa_stream_puts(pr, boa_string_getdata(exception->baseclass->name));
                    boa_stream_puts(pr, ": ");
                }
                boa_value_printvalue(pr, exception->message, false);
            }
            break;
        default:
            {
                boa_stream_printf(pr, "[unknown object %p %i]", (void*)&value, boa_value_objtype(value));
            }
            break;
    }
}

void boa_value_printnumber(BoaStream* pr, BoaNumber dn)
{
    if(isnan(dn))
    {
        boa_stream_puts(pr, "nan");
        return;
    }
    #if 0
    if(isinf(dn))
    {
        if(dn > 0.0)
        {
            boa_stream_puts(pr, "infinity");
            return;
        }
        else
        {
            boa_stream_puts(pr, "-infinity");
            return;
        }
    }
    #endif
    if(((int64_t)dn) == dn)
    {
        boa_stream_printf(pr, "%ld", (int64_t)dn);
    }
    else
    {
        boa_stream_printf(pr, "%g", dn);
    }
}

void boa_value_printvalue(BoaStream* pr, BoaValue value, bool reprmode)
{
    if(boa_value_isbool(value))
    {
        boa_stream_printf(pr, boa_value_asbool(value) ? "true" : "false");
    }
    else if(boa_value_isnull(value))
    {
        boa_stream_printf(pr, "null");
    }
    else if(boa_value_isnumber(value))
    {
        boa_value_printnumber(pr, boa_value_asnumber(value));
    }
    else if(boa_value_isobject(value))
    {
        boa_value_printobject(pr, value, reprmode);
    }
    else
    {
        boa_stream_printf(pr, "[unknown value %p (%d <%s>)]", (void*)&value, value.type, boa_value_valtypename(value));
    }
}

BoaClass* boa_class_make(BoaState* state, const char* name, BoaClass* super)
{
    BoaClass* klass;
    klass = boa_object_makeclass(state, boa_string_copylen(state, name, strlen(name)));
    boa_state_setglobal(state, klass->name, boa_value_fromobject(klass));
    if(super != NULL)
    {
        boa_class_inherit(klass, super);
    }
    return klass;
}

void boa_class_bindstaticgetter(BoaClass* selfclass, const char* name, BoaNativeFunctionFn getter)
{
    BoaString* nm;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    nm = boa_string_copylen(state, name, strlen(name));
    boa_table_set(&selfclass->staticstable, nm, boa_value_fromobject(boa_object_makefield(state, (BoaObject*)boa_object_makenativemethod(state, getter, nm), NULL)));
}

void boa_class_inherit(BoaClass* selfclass, BoaClass* other)
{
    selfclass->super = (BoaClass*)other;
    if(selfclass->mthconstructor == NULL)
    {
        selfclass->mthconstructor = other->mthconstructor;
    }
    boa_table_addallignoring(&other->mthtable, &selfclass->mthtable);
    boa_table_addallignoring(&other->staticstable, &selfclass->staticstable);
}

void boa_class_bindmethod(BoaClass* selfclass, const char* name, BoaNativeFunctionFn fn)
{
    BoaString* nm;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    nm = boa_string_copylen(state, name, strlen(name));
    boa_table_set(&selfclass->mthtable, nm, boa_value_fromobject(boa_object_makenativemethod(state, fn, nm)));
}

void boa_class_bindconstructor(BoaClass* selfclass, BoaNativeFunctionFn fn)
{
    BoaFuncNative* meth;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    meth = boa_object_makenativemethod(state, fn, state->strings.strconstructor);
    selfclass->mthconstructor = (BoaObject*)meth;
    boa_table_set(&selfclass->mthtable, state->strings.strconstructor, boa_value_fromobject(meth));
}

void boa_class_bindstaticmethod(BoaClass* selfclass, const char* name, BoaNativeFunctionFn fn)
{
    BoaString* nm;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    nm = boa_string_copylen(state, name, strlen(name));
    boa_table_set(&selfclass->staticstable, nm, boa_value_fromobject(boa_object_makenativemethod(state, fn, nm)));
}

void boa_class_setstaticfield(BoaClass* selfclass, const char* name, BoaValue val)
{
    BoaString* nm;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    nm = boa_string_copylen(state, name, strlen(name));
    boa_table_set(&selfclass->staticstable, nm, val);
}

void boa_class_bindgetsetter(BoaClass* selfclass, const char* name, BoaNativeFunctionFn fnget, BoaNativeFunctionFn fnset)
{
    BoaString* nm;
    BoaObject* mthset;
    BoaObject* mthget;
    BoaState* state;
    state = ((BoaObject*)selfclass)->pstate;
    mthset = NULL;
    mthget = NULL;
    nm = boa_string_copylen(state, name, strlen(name));
    if(fnget)
    {
        mthget = (BoaObject*)boa_object_makenativemethod(state, fnget, nm);
    }
    if(fnset)
    {
        mthset = (BoaObject*)boa_object_makenativemethod(state, fnset, nm);
    }
    boa_table_set(&selfclass->mthtable, nm, boa_value_fromobject(boa_object_makefield(state, mthget, mthset)));
}

BoaValue boa_instance_getthis(BoaInstance* selfinst)
{
    BoaValue res;
    BoaState* state;
    state = ((BoaObject*)selfinst)->pstate;
    if(boa_table_getentry(&selfinst->fields, state->strings.strthis, &res))
    {
        return res;
    }
    return boa_value_makenull();
}

bool boa_instance_setfield(BoaInstance* selfinst, BoaString* key, BoaValue val)
{
    return boa_table_set(&selfinst->fields, key, val);
}

BoaValue boa_objfnexception_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaException* exception;
    exception = boa_object_makeexception(state, boa_value_asinstance(instance)->klass);
    exception->message = (argc > 0) ? args[0] : boa_value_makenull();
    return boa_value_fromobject(exception);
}

BoaValue boa_objfnexception_messageget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaException* ex;
    (void)argc;
    (void)args;
    if(!boa_value_isexception(instance))
    {
        boa_state_raiseerror(state, "cannot invoke 'message' of a non-exception");
    }
    ex = boa_value_asexception(instance);
    return ex->message;
}

void boa_api_init(BoaState* state)
{
    state->apiname = boa_string_copylen(state, "nativeapi", 1);
    state->apifunction = NULL;
}

void boa_api_destroy(BoaState* state)
{
    state->apiname = NULL;
    state->apifunction = NULL;
}

bool boa_state_getglobaltovalue(BoaState* state, BoaString* name, BoaValue* dest)
{
    return boa_map_getvalue(state->vmstate.globals, name, dest);
}

bool boa_state_globalexists(BoaState* state, BoaString* name)
{
    BoaValue val;
    return boa_state_getglobaltovalue(state, name, &val);
}

BoaValue boa_state_getglobal(BoaState* state, BoaString* name)
{
    BoaValue global;
    if(!boa_state_getglobaltovalue(state, name, &global))
    {
        return boa_value_makenull();
    }
    return global;
}

BoaFuncScript* boa_state_getglobalfunction(BoaState* state, BoaString* name)
{
    BoaValue val;
    if(!boa_state_getglobaltovalue(state, name, &val))
    {
        return NULL;
    }
    if(boa_value_isfuncscript(val))
    {
        return boa_value_asfuncscript(val);
    }
    return NULL;
}

void boa_state_setglobal(BoaState* state, BoaString* name, BoaValue value)
{
    boa_state_pushroot(state, (BoaObject*)name);
    boa_state_pushvalueroot(state, value);
    boa_map_setvalue(state->vmstate.globals, name, value);
    boa_state_poproots(state, 2);
}

void boa_state_defnative(BoaState* state, const char* name, BoaNativeFunctionFn native)
{
    boa_state_pushroot(state, (BoaObject*)boa_string_copy(state, name));
    boa_state_pushroot(state, (BoaObject*)boa_object_makenativefunc(state, native, boa_value_asstring(boa_state_peekroot(state, 0))));
    boa_map_setvalue(state->vmstate.globals, boa_value_asstring(boa_state_peekroot(state, 1)), boa_state_peekroot(state, 0));
    boa_state_poproots(state, 2);
}

void boa_ast_destroyparamlist(BoaState* state, BoaDynListParam* parameters)
{
    size_t i;
    for(i = 0; i < parameters->listcount; i++)
    {
        boa_ast_destroyexpression(state, parameters->listitems[i].defaultval);
    }
    boa_dynlistparam_destroy(parameters);
}

void boa_ast_destroyexprlist(BoaState* state, BoaDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->listcount; i++)
    {
        boa_ast_destroyexpression(state, expressions->listitems[i]);
    }
    boa_dynlistexpr_destroy(expressions);
}

void boa_ast_destroyexpression(BoaState* state, BoaAstExpression* topexpr)
{
    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case BOA_ASTEXPRTYP_LITERAL:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_BINARY:
        {
            BoaAstBinaryExpr* expr = (BoaAstBinaryExpr*)topexpr;
            if(!expr->ignoreleft)
            {
                boa_ast_destroyexpression(state, expr->left);
            }
            boa_ast_destroyexpression(state, expr->right);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_UNARY:
        {
            boa_ast_destroyexpression(state, ((BoaAstUnaryExpr*)topexpr)->right);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_VARGET:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_ASSIGN:
        {
            BoaAstAssignExpr* expr = (BoaAstAssignExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->to);
            boa_ast_destroyexpression(state, expr->value);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_CALL:
        {
            BoaAstCallExpr* expr = (BoaAstCallExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->excallee);
            boa_ast_destroyexpression(state, expr->init);
            boa_ast_destroyexprlist(state, &expr->callargs);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_INDEXGET:
        {
            boa_ast_destroyexpression(state, ((BoaAstIndexGetExpr*)topexpr)->where);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_INDEXSET:
        {
            BoaAstIndexSetExpr* expr = (BoaAstIndexSetExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->where);
            boa_ast_destroyexpression(state, expr->value);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_FUNCANON:
        {
            BoaAstFunctionExpr* expr = (BoaAstFunctionExpr*)topexpr;
            boa_ast_destroyparamlist(state, &expr->parameters);
            boa_ast_destroyexpression(state, expr->body);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_ARRAY:
        {
            boa_ast_destroyexprlist(state, &((BoaAstLiteralArrayExpr*)topexpr)->exvalues);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_OBJECT:
        {
            BoaAstLiteralObjectExpr* map = (BoaAstLiteralObjectExpr*)topexpr;
            boa_dynlistval_destroy(&map->objexkeys);
            boa_ast_destroyexprlist(state, &map->objexvalues);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_SUBSCRIPT:
        {
            BoaAstSubscriptExpr* expr = (BoaAstSubscriptExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->array);
            boa_ast_destroyexpression(state, expr->index);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_THIS:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_SUPER:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_RANGE:
        {
            BoaAstRangeExpr* expr = (BoaAstRangeExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->from);
            boa_ast_destroyexpression(state, expr->to);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_TERNARY:
        {
            BoaAstTernaryExpr* expr = (BoaAstTernaryExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->condition);
            boa_ast_destroyexpression(state, expr->branchif);
            boa_ast_destroyexpression(state, expr->branchelse);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_INTERPOLATION:
        {
            boa_ast_destroyexprlist(state, &((BoaAstStrTemplateExpr*)topexpr)->expressions);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_REFERENCE:
        {
            boa_ast_destroyexpression(state, ((BoaAstRefExpr*)topexpr)->to);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_EXPRESSION:
        {
            boa_ast_destroyexpression(state, ((BoaAstExprStmtExpr*)topexpr)->exvalue);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_BLOCK:
        {
            boa_ast_destroyexprlist(state, &((BoaAstBlockExpr*)topexpr)->statements);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_VARDECL:
        {
            boa_ast_destroyexpression(state, ((BoaAstVarDeclExpr*)topexpr)->init);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_IF:
        {
            BoaAstIfExpr* stmt;
            stmt = (BoaAstIfExpr*)topexpr;
            boa_ast_destroyexpression(state, stmt->condition);
            boa_ast_destroyexpression(state, stmt->branchif);
            boa_ast_destroyallocatedexprlist(state, stmt->elseifcondlist);
            boa_ast_destroyallocatedstmtlist(state, stmt->branchelseiflist);
            boa_ast_destroyexpression(state, stmt->branchelse);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_SWITCH:
        {
            BoaAstSwitchExpr* stmt;
            stmt = (BoaAstSwitchExpr*)topexpr;
            boa_ast_destroyexpression(state, stmt->condition);
            boa_ast_destroyexprlist(state, &stmt->caseconditions);
            boa_ast_destroyexprlist(state, &stmt->casebodies);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_WHILE:
        {
            BoaAstWhileExpr* stmt;
            stmt = (BoaAstWhileExpr*)topexpr;
            boa_ast_destroyexpression(state, stmt->condition);
            boa_ast_destroyexpression(state, stmt->body);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_FOR:
        {
            BoaAstForExpr* stmt;
            stmt = (BoaAstForExpr*)topexpr;
            boa_ast_destroyexpression(state, stmt->increment);
            boa_ast_destroyexpression(state, stmt->condition);
            boa_ast_destroyexpression(state, stmt->init);
            boa_ast_destroyexpression(state, stmt->var);
            boa_ast_destroyexpression(state, stmt->body);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_CONTINUE:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_BREAK:
        {
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_FUNCTION:
        {
            BoaAstFunctionExpr* stmt = (BoaAstFunctionExpr*)topexpr;
            boa_ast_destroyexpression(state, stmt->body);
            boa_ast_destroyparamlist(state, &stmt->parameters);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_RETURN:
        {
            boa_ast_destroyexpression(state, ((BoaAstReturnExpr*)topexpr)->exvalue);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_METHOD:
        {
            BoaAstMethodExpr* stmt = (BoaAstMethodExpr*)topexpr;
            boa_ast_destroyparamlist(state, &stmt->parameters);
            boa_ast_destroyexpression(state, stmt->body);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_CLASS:
        {
            BoaAstClassExpr* expr;
            expr = (BoaAstClassExpr*)topexpr;
            boa_ast_destroyexprlist(state, &expr->staticfields);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_FIELD:
        {
            BoaAstFieldExpr* expr = (BoaAstFieldExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->getter);
            boa_ast_destroyexpression(state, expr->setter);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_TRY:
        {
            BoaAstTryExpr* expr = (BoaAstTryExpr*)topexpr;
            boa_ast_destroyexpression(state, expr->tryblock);
            if(expr->catchblock) boa_ast_destroyexpression(state, expr->catchblock);
            if(expr->finallyblock) boa_ast_destroyexpression(state, expr->finallyblock);
            boa_sysmem_free(topexpr);
            break;
        }
        case BOA_ASTEXPRTYP_THROW:
        {
            boa_ast_destroyexpression(state, ((BoaAstThrowExpr*)topexpr)->exvalue);
            boa_sysmem_free(topexpr);
            break;
        }
        default:
        {
            boa_state_raiseerror(state, "unknown expression type %d", (int)topexpr->type);
            break;
        }
    }
}

BoaAstExpression* boa_ast_allocexpression(uint64_t ssrcln, size_t size, BoaAstExprType type)
{
    BoaAstExpression* object;
    object = (BoaAstExpression*)boa_sysmem_malloc(size);
    object->type = type;
    object->line = ssrcln;
    return object;
}

BoaAstLiteralValExpr* boa_ast_makeliteralexpr(size_t ssrcln, BoaValue value)
{
    BoaAstLiteralValExpr* expr = (BoaAstLiteralValExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstLiteralValExpr), BOA_ASTEXPRTYP_LITERAL);
    expr->value = value;
    return expr;
}

BoaAstBinaryExpr* boa_ast_makebinaryexpr(size_t ssrcln, BoaAstExpression* left, BoaAstExpression* right, BoaAstTokType op)
{
    BoaAstBinaryExpr* expr = (BoaAstBinaryExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstBinaryExpr), BOA_ASTEXPRTYP_BINARY);
    expr->left = left;
    expr->right = right;
    expr->op = op;
    expr->ignoreleft = false;
    return expr;
}

BoaAstUnaryExpr* boa_ast_makeunaryexpr(size_t ssrcln, BoaAstExpression* right, BoaAstTokType op)
{
    BoaAstUnaryExpr* expr = (BoaAstUnaryExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstUnaryExpr), BOA_ASTEXPRTYP_UNARY);
    expr->right = right;
    expr->op = op;
    return expr;
}

BoaAstAssignExpr* boa_ast_makeassignexpr(size_t ssrcln, BoaAstExpression* to, BoaAstExpression* value)
{
    BoaAstAssignExpr* expr = (BoaAstAssignExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstAssignExpr), BOA_ASTEXPRTYP_ASSIGN);
    expr->to = to;
    expr->value = value;
    return expr;
}

BoaAstCallExpr* boa_ast_makecallexpr(size_t ssrcln, BoaAstExpression* callee)
{
    BoaAstCallExpr* expr = (BoaAstCallExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstCallExpr), BOA_ASTEXPRTYP_CALL);
    expr->excallee = callee;
    expr->init = NULL;
    boa_dynlistexpr_init(&expr->callargs);
    return expr;
}

BoaAstIndexGetExpr* boa_ast_makegetexpr(size_t ssrcln, BoaAstExpression* where, const char* name, size_t length, bool questionable, bool ignresult, bool isdot)
{
    BoaAstIndexGetExpr* expr = (BoaAstIndexGetExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstIndexGetExpr), BOA_ASTEXPRTYP_INDEXGET);
    expr->where = where;
    expr->name = name;
    expr->length = length;
    expr->ignoreemit = false;
    expr->jump = questionable ? 0 : -1;
    expr->ignoreresult = ignresult;
    expr->isdot = isdot;
    return expr;
}

BoaAstIndexSetExpr* boa_ast_makesetexpr(size_t ssrcln, BoaAstExpression* where, const char* name, size_t length, BoaAstExpression* value, bool isdot)
{
    BoaAstIndexSetExpr* expr = (BoaAstIndexSetExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstIndexSetExpr), BOA_ASTEXPRTYP_INDEXSET);
    expr->where = where;
    expr->name = name;
    expr->length = length;
    expr->value = value;
    expr->isdot = isdot;
    return expr;
}

BoaAstLiteralArrayExpr* boa_ast_makearrayexpr(size_t ssrcln)
{
    BoaAstLiteralArrayExpr* expr = (BoaAstLiteralArrayExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstLiteralArrayExpr), BOA_ASTEXPRTYP_ARRAY);
    boa_dynlistexpr_init(&expr->exvalues);
    return expr;
}

BoaAstLiteralObjectExpr* boa_ast_makeobjectexpr(size_t ssrcln)
{
    BoaAstLiteralObjectExpr* expr = (BoaAstLiteralObjectExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstLiteralObjectExpr), BOA_ASTEXPRTYP_OBJECT);
    boa_dynlistval_init(&expr->objexkeys);
    boa_dynlistexpr_init(&expr->objexvalues);
    return expr;
}

BoaAstSubscriptExpr* boa_ast_makesubscriptexpr(size_t ssrcln, BoaAstExpression* array, BoaAstExpression* index)
{
    BoaAstSubscriptExpr* expr = (BoaAstSubscriptExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstSubscriptExpr), BOA_ASTEXPRTYP_SUBSCRIPT);
    expr->array = array;
    expr->index = index;
    return expr;
}

BoaAstThisExpr* boa_ast_makethisexpr(size_t ssrcln)
{
    return (BoaAstThisExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstThisExpr), BOA_ASTEXPRTYP_THIS);
}

BoaAstSuperExpr* boa_ast_makesuperexpr(size_t ssrcln, BoaString* method, bool ignresult)
{
    BoaAstSuperExpr* expr = (BoaAstSuperExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstSuperExpr), BOA_ASTEXPRTYP_SUPER);
    expr->methodname = method;
    expr->ignoreemit = false;
    expr->ignoreresult = ignresult;
    return expr;
}

BoaAstRangeExpr* boa_ast_makerangeexpr(size_t ssrcln, BoaAstExpression* from, BoaAstExpression* to)
{
    BoaAstRangeExpr* expr = (BoaAstRangeExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstRangeExpr), BOA_ASTEXPRTYP_RANGE);
    expr->from = from;
    expr->to = to;
    return expr;
}

BoaAstTernaryExpr* boa_ast_maketernaryexpr(size_t ssrcln, BoaAstExpression* condition, BoaAstExpression* ifbranch, BoaAstExpression* elsebranch)
{
    BoaAstTernaryExpr* expr = (BoaAstTernaryExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstTernaryExpr), BOA_ASTEXPRTYP_TERNARY);
    expr->condition = condition;
    expr->branchif = ifbranch;
    expr->branchelse = elsebranch;
    return expr;
}

BoaAstStrTemplateExpr* boa_ast_makeinterpolationexpr(size_t ssrcln)
{
    BoaAstStrTemplateExpr* expr = (BoaAstStrTemplateExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstStrTemplateExpr), BOA_ASTEXPRTYP_INTERPOLATION);
    boa_dynlistexpr_init(&expr->expressions);
    return expr;
}

BoaAstRefExpr* boa_ast_makerefexpr(size_t ssrcln, BoaAstExpression* to)
{
    BoaAstRefExpr* expr = (BoaAstRefExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstRefExpr), BOA_ASTEXPRTYP_REFERENCE);
    expr->to = to;
    return expr;
}

BoaAstExprStmtExpr* boa_ast_makeexprstmt(size_t ssrcln, BoaAstExpression* exv)
{
    BoaAstExprStmtExpr* expr = (BoaAstExprStmtExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstExprStmtExpr), BOA_ASTEXPRTYP_EXPRESSION);
    expr->exvalue = exv;
    return expr;
}

BoaAstBlockExpr* boa_ast_makeblockstmt(size_t ssrcln)
{
    BoaAstBlockExpr* expr = (BoaAstBlockExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstBlockExpr), BOA_ASTEXPRTYP_BLOCK);
    boa_dynlistexpr_init(&expr->statements);
    return expr;
}

BoaAstVarGetExpr* boa_ast_makevargetexpr(size_t ssrcln, const char* name, size_t length)
{
    BoaAstVarGetExpr* expr = (BoaAstVarGetExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstVarGetExpr), BOA_ASTEXPRTYP_VARGET);
    expr->name = name;
    expr->length = length;
    return expr;
}

BoaAstVarDeclExpr* boa_ast_makevardeclstmt(size_t ssrcln, const char* name, size_t length, BoaAstExpression* init, bool constant)
{
    BoaAstVarDeclExpr* expr = (BoaAstVarDeclExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstVarDeclExpr), BOA_ASTEXPRTYP_VARDECL);
    expr->name = name;
    expr->length = length;
    expr->init = init;
    expr->isconstant = constant;
    return expr;
}

BoaAstIfExpr* boa_ast_makeifstatement(size_t ssrcln, BoaAstExpression* condition, BoaAstExpression* ifbranch, BoaAstExpression* elsebranch, BoaDynListExpr* elseifconditions, BoaDynListExpr* elseifbranches)
{
    BoaAstIfExpr* expr = (BoaAstIfExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstIfExpr), BOA_ASTEXPRTYP_IF);
    expr->condition = condition;
    expr->branchif = ifbranch;
    expr->branchelse = elsebranch;
    expr->elseifcondlist = elseifconditions;
    expr->branchelseiflist = elseifbranches;
    return expr;
}

BoaAstSwitchExpr* boa_ast_makeswitchstatement(size_t ssrcln, BoaAstExpression* condition)
{
    BoaAstSwitchExpr* expr = (BoaAstSwitchExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstSwitchExpr), BOA_ASTEXPRTYP_SWITCH);
    expr->condition = condition;
    boa_dynlistexpr_init(&expr->caseconditions);
    boa_dynlistexpr_init(&expr->casebodies);
    return expr;
}

BoaAstWhileExpr* boa_ast_makewhilestmt(size_t ssrcln, BoaAstExpression* condition, BoaAstExpression* body)
{
    BoaAstWhileExpr* expr = (BoaAstWhileExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstWhileExpr), BOA_ASTEXPRTYP_WHILE);
    expr->condition = condition;
    expr->body = body;
    return expr;
}

BoaAstForExpr* boa_ast_makeforstmt(size_t ssrcln, BoaAstExpression* init, BoaAstExpression* var, BoaAstExpression* condition, BoaAstExpression* increment, BoaAstExpression* body, bool cstyle)
{
    BoaAstForExpr* expr = (BoaAstForExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstForExpr), BOA_ASTEXPRTYP_FOR);
    expr->init = init;
    expr->var = var;
    expr->condition = condition;
    expr->increment = increment;
    expr->body = body;
    expr->iscstyle = cstyle;
    return expr;
}

BoaAstContinueExpr* boa_ast_makecontinuestmt(size_t ssrcln)
{
    return (BoaAstContinueExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstContinueExpr), BOA_ASTEXPRTYP_CONTINUE);
}

BoaBreakStatement* boa_ast_makebreakstmt(size_t ssrcln)
{
    return (BoaBreakStatement*)boa_ast_allocexpression(ssrcln, sizeof(BoaBreakStatement), BOA_ASTEXPRTYP_BREAK);
}

BoaAstFunctionExpr* boa_ast_makefuncdefstmt(size_t ssrcln, const char* name, size_t length)
{
    BoaAstFunctionExpr* function = (BoaAstFunctionExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstFunctionExpr), BOA_ASTEXPRTYP_FUNCTION);
    function->name = name;
    function->length = length;
    function->body = NULL;
    boa_dynlistparam_init(&function->parameters);
    return function;
}

BoaAstFunctionExpr* boa_ast_makelambdaexpr(size_t ssrcln)
{
    BoaAstFunctionExpr* expr = (BoaAstFunctionExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstFunctionExpr), BOA_ASTEXPRTYP_FUNCANON);
    expr->body = NULL;
    expr->name = NULL;
    boa_dynlistparam_init(&expr->parameters);
    return expr;
}

BoaAstReturnExpr* boa_ast_makereturnstmt(size_t ssrcln, BoaAstExpression* exv)
{
    BoaAstReturnExpr* expr = (BoaAstReturnExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstReturnExpr), BOA_ASTEXPRTYP_RETURN);
    expr->exvalue = exv;
    return expr;
}

BoaAstMethodExpr* boa_ast_makemethoddefstmt(size_t ssrcln, BoaString* name, bool isstatic)
{
    BoaAstMethodExpr* expr = (BoaAstMethodExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstMethodExpr), BOA_ASTEXPRTYP_METHOD);
    expr->name = name;
    expr->body = NULL;
    expr->isstatic = isstatic;
    boa_dynlistparam_init(&expr->parameters);
    return expr;
}

BoaAstClassExpr* boa_ast_makeclassdefstmt(size_t ssrcln, BoaString* name, BoaString* parent)
{
    BoaAstClassExpr* expr = (BoaAstClassExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstClassExpr), BOA_ASTEXPRTYP_CLASS);
    expr->name = name;
    expr->parent = parent;
    boa_dynlistexpr_init(&expr->staticfields);
    return expr;
}

BoaAstFieldExpr* boa_ast_makefieldstmt(size_t ssrcln, BoaString* name, BoaAstExpression* getter, BoaAstExpression* setter, bool isstatic)
{
    BoaAstFieldExpr* expr = (BoaAstFieldExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstFieldExpr), BOA_ASTEXPRTYP_FIELD);
    expr->name = name;
    expr->getter = getter;
    expr->setter = setter;
    expr->isstatic = isstatic;
    return expr;
}

BoaAstTryExpr* boa_ast_maketrystmt(size_t ssrcln, BoaAstExpression* tryblock, BoaAstExpression* catchblock, BoaAstExpression* finallyblock, const char* catchvarstr, size_t catchvarlen)
{
    BoaAstTryExpr* expr = (BoaAstTryExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstTryExpr), BOA_ASTEXPRTYP_TRY);
    expr->tryblock = tryblock;
    expr->catchblock = catchblock;
    expr->finallyblock = finallyblock;
    expr->catchvarstr = catchvarstr;
    expr->catchvarlen = catchvarlen;
    return expr;
}

BoaAstThrowExpr* boa_ast_makethrowstmt(size_t ssrcln, BoaAstExpression* exvalue)
{
    BoaAstThrowExpr* expr = (BoaAstThrowExpr*)boa_ast_allocexpression(ssrcln, sizeof(BoaAstThrowExpr), BOA_ASTEXPRTYP_THROW);
    expr->exvalue = exvalue;
    return expr;
}

BoaDynListExpr* boa_ast_allocexprlist()
{
    BoaDynListExpr* expressions = (BoaDynListExpr*)boa_sysmem_malloc(sizeof(BoaDynListExpr));
    boa_dynlistexpr_init(expressions);
    return expressions;
}

void boa_ast_destroyallocatedexprlist(BoaState* state, BoaDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->listcount; i++)
    {
        boa_ast_destroyexpression(state, expressions->listitems[i]);
    }
    boa_dynlistexpr_destroy(expressions);
    boa_sysmem_free(expressions);
}

BoaDynListExpr* boa_ast_allocstmtlist()
{
    BoaDynListExpr* statements = (BoaDynListExpr*)boa_sysmem_malloc(sizeof(BoaDynListExpr));
    boa_dynlistexpr_init(statements);
    return statements;
}

void boa_ast_destroyallocatedstmtlist(BoaState* state, BoaDynListExpr* statements)
{
    size_t i;
    if(statements == NULL)
    {
        return;
    }
    for(i = 0; i < statements->listcount; i++)
    {
        boa_ast_destroyexpression(state, statements->listitems[i]);
    }
    boa_dynlistexpr_destroy(statements);
    boa_sysmem_free(statements);
}

void boa_astlex_init(BoaState* state, BoaAstLexer* lex, const char* filename, const char* source)
{
    lex->sourcecurrentline = 1;
    lex->sourcedatastart = source;
    lex->sourcedatacurrent = source;
    lex->sourcefilename = filename;
    lex->pstate = state;
    lex->bracecount = 0;
    lex->haderror = false;
}

BoaAstToken boa_astlex_maketoken(BoaAstLexer* lex, BoaAstTokType type)
{
    BoaAstToken token;
    token.type = type;
    token.start = lex->sourcedatastart;
    token.length = (size_t)(lex->sourcedatacurrent - lex->sourcedatastart);
    token.line = lex->sourcecurrentline;
    return token;
}

BoaAstToken boa_astlex_makeerrortoken(BoaAstLexer* lex, const char* fmt, ...)
{
    va_list args;
    BoaAstToken token;
    BoaString* result;
    lex->haderror = true;
    va_start(args, fmt);
    result = boa_state_errorfmtv(lex->pstate, lex->sourcecurrentline, fmt, args);
    va_end(args);
    token.type = BOA_ASTTOKTYP_ERROR;
    token.start = boa_string_getdata(result);
    token.length = boa_string_getlength(result);
    token.line = lex->sourcecurrentline;
    return token;
}

bool boa_astlex_isatend(BoaAstLexer* lex)
{
    return *lex->sourcedatacurrent == '\0';
}

char boa_astlex_advance(BoaAstLexer* lex)
{
    lex->sourcedatacurrent++;
    return lex->sourcedatacurrent[-1];
}

bool boa_astlex_match(BoaAstLexer* lex, char expected)
{
    if(boa_astlex_isatend(lex))
    {
        return false;
    }
    if(*lex->sourcedatacurrent != expected)
    {
        return false;
    }
    lex->sourcedatacurrent++;
    return true;
}

BoaAstToken boa_astlex_matchtoken(BoaAstLexer* lex, char c, BoaAstTokType a, BoaAstTokType b)
{
    return boa_astlex_maketoken(lex, boa_astlex_match(lex, c) ? a : b);
}

BoaAstToken boa_astlex_matchtokens(BoaAstLexer* lex, char cr, char cb, BoaAstTokType a, BoaAstTokType b, BoaAstTokType c)
{
    return boa_astlex_maketoken(lex, boa_astlex_match(lex, cr) ? a : (boa_astlex_match(lex, cb) ? b : c));
}

char boa_astlex_peek(BoaAstLexer* lex)
{
    return *lex->sourcedatacurrent;
}

char boa_astlex_peeknext(BoaAstLexer* lex)
{
    if(boa_astlex_isatend(lex))
    {
        return '\0';
    }
    return lex->sourcedatacurrent[1];
}

bool boa_astlex_skipwhitespace(BoaAstLexer* lex)
{
    char c;
    while(true)
    {
        c = boa_astlex_peek(lex);
        switch(c)
        {
            case 1:
            case 2:
            case 3:
            case ' ':
            case '\r':
            case '\t':
            {
                boa_astlex_advance(lex);
                break;
            }
            case '\n':
            {
                lex->sourcedatastart = lex->sourcedatacurrent;
                boa_astlex_advance(lex);
                return true;
            }
            case '/':
            {
                if(boa_astlex_peeknext(lex) == '/')
                {
                    while(boa_astlex_peek(lex) != '\n' && !boa_astlex_isatend(lex))
                    {
                        boa_astlex_advance(lex);
                    }
                    return boa_astlex_skipwhitespace(lex);
                }
                else if(boa_astlex_peeknext(lex) == '*')
                {
                    boa_astlex_advance(lex);
                    boa_astlex_advance(lex);
                    while((boa_astlex_peek(lex) != '*' || boa_astlex_peeknext(lex) != '/') && !boa_astlex_isatend(lex))
                    {
                        if(boa_astlex_peek(lex) == '\n')
                        {
                            lex->sourcecurrentline++;
                        }
                        boa_astlex_advance(lex);
                    }
                    boa_astlex_advance(lex);
                    boa_astlex_advance(lex);
                    return boa_astlex_skipwhitespace(lex);
                }
                return false;
            }
            default:
                return false;
        }
    }
}

BoaAstToken boa_astlex_scanstring(BoaAstLexer* lex, bool istplstring, bool useescapes, char endch)
{
    char currch;
    char nextch;
    const char* cstr;
    BoaDynListByte tgtbuf;
    BoaAstTokType stringtype;
    BoaAstToken token;
    BoaState* state;
    nextch = -1;
    state = lex->pstate;
    stringtype = BOA_ASTTOKTYP_STRING;
    boa_dynlistbyte_init(&tgtbuf);
    while(true)
    {
        currch = boa_astlex_advance(lex);
        if(currch == endch)
        {
            break;
        }
        else if(istplstring && currch == '$' && boa_astlex_peek(lex) == '{')
        {
            if(lex->bracecount >= BOA_CONFIG_TPLSTRINGNMAXNESTING)
            {
                boa_dynlistbyte_destroy(&tgtbuf);
                return boa_astlex_makeerrortoken(lex, "template string nesting is too deep, maximum is %i", BOA_CONFIG_TPLSTRINGNMAXNESTING);
            }
            boa_astlex_advance(lex);
            stringtype = BOA_ASTTOKTYP_STRTEMPLATE;
            lex->bracevalues[lex->bracecount++] = 1;
            break;
        }
        if(useescapes)
        {
            switch(currch)
            {
                case '\0':
                    {
                        boa_dynlistbyte_destroy(&tgtbuf);
                        return boa_astlex_makeerrortoken(lex, "unterminated string");
                    }
                    break;
                case '\n':
                    {
                        lex->sourcecurrentline++;
                        boa_dynlistbyte_push(&tgtbuf, currch);
                    }
                    break;
                case '\\':
                    {
                        nextch = boa_astlex_advance(lex);
                        if(nextch == '\n')
                        {
                            continue;
                        }
                        if(nextch == endch)
                        {
                            boa_dynlistbyte_push(&tgtbuf, endch);
                        }
                        else
                        {
                            switch(nextch)
                            {
                                case '\"':
                                    boa_dynlistbyte_push(&tgtbuf, '\"');
                                    break;
                                case '\\':
                                    boa_dynlistbyte_push(&tgtbuf, '\\');
                                    break;
                                case '0':
                                    boa_dynlistbyte_push(&tgtbuf, '\0');
                                    break;
                                case '{':
                                    boa_dynlistbyte_push(&tgtbuf, '{');
                                    break;
                                case '$':
                                    boa_dynlistbyte_push(&tgtbuf, '$');
                                    break;
                                case 'a':
                                    boa_dynlistbyte_push(&tgtbuf, '\a');
                                    break;
                                case 'b':
                                    boa_dynlistbyte_push(&tgtbuf, '\b');
                                    break;
                                case 'f':
                                    boa_dynlistbyte_push(&tgtbuf, '\f');
                                    break;
                                case 'n':
                                    boa_dynlistbyte_push(&tgtbuf, '\n');
                                    break;
                                case 'r':
                                    boa_dynlistbyte_push(&tgtbuf, '\r');
                                    break;
                                case 't':
                                    boa_dynlistbyte_push(&tgtbuf, '\t');
                                    break;
                                case 'v':
                                    boa_dynlistbyte_push(&tgtbuf, '\v');
                                    break;
                                case 'e':
                                    boa_dynlistbyte_push(&tgtbuf, 27);
                                    break;
                                default:
                                    {
                                        boa_dynlistbyte_destroy(&tgtbuf);
                                        return boa_astlex_makeerrortoken(lex, "invalid escape character <%c>", lex->sourcedatacurrent[-1]);
                                    }
                                    break;
                            }
                        }
                    }
                    break;
                default:
                    {
                        boa_dynlistbyte_push(&tgtbuf, currch);
                    }
                    break;
            }
        }
        else
        {
            boa_dynlistbyte_push(&tgtbuf, currch);
        }
    }
    token = boa_astlex_maketoken(lex, stringtype);
    /*if(tgtbuf.listcount == 0)
    {
        token.value = boa_value_makenull();
    }
    else
    */
    {
        cstr = (const char*)tgtbuf.listitems;
        if(tgtbuf.listcount == 0)
        {
            cstr = "";
        }
        token.tokvalue = boa_value_fromobject(boa_string_copylen(state, cstr, tgtbuf.listcount));
    }
    boa_dynlistbyte_destroy(&tgtbuf);
    return token;
}

int boa_astlex_scanhexdigit(BoaAstLexer* lex)
{
    char c = boa_astlex_advance(lex);
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    lex->sourcedatacurrent--;
    return -1;
}

int boa_astlex_scanbinarydigit(BoaAstLexer* lex)
{
    char c = boa_astlex_advance(lex);
    if(c >= '0' && c <= '1')
    {
        return c - '0';
    }
    lex->sourcedatacurrent--;
    return -1;
}

BoaAstToken boa_astlex_makenumbertoken(BoaAstLexer* lex, bool ishex, bool isbinary)
{
    BoaValue value;
    BoaAstToken token;
    errno = 0;
    if(ishex)
    {
        value = boa_value_makenumber((BoaNumber)strtoll(lex->sourcedatastart, NULL, 16));
    }
    else if(isbinary)
    {
        value = boa_value_makenumber((int)strtoll(lex->sourcedatastart + 2, NULL, 2));
    }
    else
    {
        value = boa_value_makenumber(strtod(lex->sourcedatastart, NULL));
    }
    if(errno == ERANGE)
    {
        errno = 0;
        return boa_astlex_makeerrortoken(lex, "number is too big to be represented by a single literal");
    }
    token = boa_astlex_maketoken(lex, BOA_ASTTOKTYP_NUMBER);
    token.tokvalue = value;
    return token;
}

BoaAstToken boa_astlex_scannumber(BoaAstLexer* lex)
{
    if(boa_astlex_match(lex, 'x'))
    {
        while(boa_astlex_scanhexdigit(lex) != -1)
        {
            continue;
        }
        return boa_astlex_makenumbertoken(lex, true, false);
    }
    if(boa_astlex_match(lex, 'b'))
    {
        while(boa_astlex_scanbinarydigit(lex) != -1)
        {
            continue;
        }
        return boa_astlex_makenumbertoken(lex, false, true);
    }
    while(boa_util_charisdigit(boa_astlex_peek(lex)))
    {
        boa_astlex_advance(lex);
    }
    /* look for a fractional part */
    if(boa_astlex_peek(lex) == '.' && boa_util_charisdigit(boa_astlex_peeknext(lex)))
    {
        /* consume the '.' */
        boa_astlex_advance(lex);
        while(boa_util_charisdigit(boa_astlex_peek(lex)))
        {
            boa_astlex_advance(lex);
        }
    }
    return boa_astlex_makenumbertoken(lex, false, false);
}

BoaAstTokType boa_astlex_scanidenttype(BoaAstLexer* lex)
{
    /* clang-format off*/
    static struct
    {
        BoaAstTokType type;
        const char* kw;
    } keywords[] = {
        { BOA_ASTTOKTYP_KWCLASS, "class" },
        { BOA_ASTTOKTYP_KWELSE, "else" },
        { BOA_ASTTOKTYP_KWEXTENDS, "extends"},
        { BOA_ASTTOKTYP_KWFALSE, "false" },
        { BOA_ASTTOKTYP_KWFOR, "for" },
        { BOA_ASTTOKTYP_KWFUNCTION, "function" },
        { BOA_ASTTOKTYP_KWIF, "if" },
        { BOA_ASTTOKTYP_KWNULL, "null" },
        { BOA_ASTTOKTYP_KWRETURN, "return" },
        { BOA_ASTTOKTYP_KWSUPER, "super" },
        { BOA_ASTTOKTYP_KWTHIS, "this" },
        { BOA_ASTTOKTYP_KWTRUE, "true" },
        { BOA_ASTTOKTYP_KWVAR, "var" },
        { BOA_ASTTOKTYP_KWVAR, "let" },
        { BOA_ASTTOKTYP_KWWHILE, "while" },
        { BOA_ASTTOKTYP_KWCONTINUE, "continue" },
        { BOA_ASTTOKTYP_KWBREAK, "break" },
        { BOA_ASTTOKTYP_KWNEW, "new" },
        { BOA_ASTTOKTYP_KWEXPORT, "export" },
        { BOA_ASTTOKTYP_KWIS, "is" },
        { BOA_ASTTOKTYP_KWIS, "instanceof" },
        { BOA_ASTTOKTYP_KWSTATIC, "static" },
        { BOA_ASTTOKTYP_KWOPERATOR, "operator" },
        { BOA_ASTTOKTYP_KWIN, "in" },
        { BOA_ASTTOKTYP_KWCONST, "const" },
        { BOA_ASTTOKTYP_KWREF, "ref" },
        { BOA_ASTTOKTYP_KWTRY, "try" },
        { BOA_ASTTOKTYP_KWCATCH, "catch" },
        { BOA_ASTTOKTYP_KWFINALLY, "finally" },
        { BOA_ASTTOKTYP_KWTHROW, "throw" },
        { BOA_ASTTOKTYP_KWSWITCH, "switch" },
        { BOA_ASTTOKTYP_KWCASE, "case" },
        { BOA_ASTTOKTYP_KWDEFAULT, "default" },
        { (BoaAstTokType)0, NULL },
    };
    /* clang-format on */
    size_t i;
    size_t kwlen;
    size_t ofs;
    const char* kwtext;
    for(i = 0; keywords[i].kw != NULL; i++)
    {
        kwtext = keywords[i].kw;
        kwlen = strlen(keywords[i].kw);
        ofs = (lex->sourcedatacurrent - lex->sourcedatastart);
        if((ofs == (0 + kwlen)) && (memcmp(lex->sourcedatastart + 0, kwtext, kwlen) == 0))
        {
            return keywords[i].type;
        }
    }
    return BOA_ASTTOKTYP_IDENTIFIER;
}

BoaAstToken boa_astlex_scanident(BoaAstLexer* lex)
{
    char pch;
    while(true)
    {
        pch = boa_astlex_peek(lex);
        if(boa_util_charisalpha(pch) || boa_util_charisdigit(pch) || (pch == '$'))
        {
            boa_astlex_advance(lex);
        }
        else
        {
            break;
        }
    }
    return boa_astlex_maketoken(lex, boa_astlex_scanidenttype(lex));
}

BoaAstToken boa_astlex_scantoken(BoaAstLexer* lex)
{
    char c;
    BoaAstToken token;
    if(boa_astlex_skipwhitespace(lex))
    {
        token = boa_astlex_maketoken(lex, BOA_ASTTOKTYP_LINEFEED);
        lex->sourcecurrentline++;
        return token;
    }
    lex->sourcedatastart = lex->sourcedatacurrent;
    if(boa_astlex_isatend(lex))
    {
        return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_EOF);
    }
    c = boa_astlex_advance(lex);
    if(boa_util_charisdigit(c))
    {
        return boa_astlex_scannumber(lex);
    }
    if(boa_util_charisalpha(c))
    {
        return boa_astlex_scanident(lex);
    }
    switch(c)
    {
        case '@':
            {
                return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_REFSYM);
            }
            break;
        case '(':
            {
                return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_LEFTPAREN);
            }
            break;
        case ')':
            {
                return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_RIGHTPAREN);
            };
        case '{':
            {
                if(lex->bracecount > 0)
                {
                    lex->bracevalues[lex->bracecount - 1]++;
                }
                return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_LEFTBRACE);
            }
            break;
        case '}':
            {
                if(lex->bracecount > 0 && --lex->bracevalues[lex->bracecount - 1] == 0)
                {
                    lex->bracecount--;
                    return boa_astlex_scanstring(lex, true, true, '`');
                }
                return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_RIGHTBRACE);
            }
            break;
        case '[':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_LEFTBRACKET);
        case ']':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_RIGHTBRACKET);
        case ';':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_SEMICOLON);
        case ',':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_COMMA);
        case ':':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_COLON);
        case '~':
            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_TILDE);
        case '+':
            return boa_astlex_matchtokens(lex, '=', '+', BOA_ASTTOKTYP_PLUSEQUAL, BOA_ASTTOKTYP_PLUSPLUS, BOA_ASTTOKTYP_PLUS);
        case '-':
            {
                if(boa_astlex_match(lex, '>'))
                {
                    return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_SMALLARROW);
                }
                else
                {
                    return boa_astlex_matchtokens(lex, '=', '-', BOA_ASTTOKTYP_MINUSEQUAL, BOA_ASTTOKTYP_MINUSMINUS, BOA_ASTTOKTYP_MINUS);
                }
            }
            break;
        case '/':
            return boa_astlex_matchtoken(lex, '=', BOA_ASTTOKTYP_SLASHEQUAL, BOA_ASTTOKTYP_SLASH);
        case '#':
            return boa_astlex_matchtoken(lex, '=', BOA_ASTTOKTYP_SHARPEQUAL, BOA_ASTTOKTYP_SHARP);
        case '!':
            {
                {
                    if(boa_astlex_match(lex, '='))
                    {
                        if(boa_astlex_match(lex, '='))
                        {
                            /*js-ism: !== */
                        }
                        return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_BANGEQUAL);
                    }
                    else
                    {
                        return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_BANG);
                    }
                }
            }
            break;
        case '?':
            {
                return boa_astlex_matchtoken(lex, '?', BOA_ASTTOKTYP_QUESTIONQUESTION, BOA_ASTTOKTYP_QUESTION);
            }
            break;
        case '%':
            return boa_astlex_matchtoken(lex, '=', BOA_ASTTOKTYP_PERCENTEQUAL, BOA_ASTTOKTYP_PERCENT);
        case '^':
            return boa_astlex_matchtoken(lex, '=', BOA_ASTTOKTYP_CARETEQUAL, BOA_ASTTOKTYP_CARET);
        case '>':
            return boa_astlex_matchtokens(lex, '=', '>', BOA_ASTTOKTYP_GREATEREQUAL, BOA_ASTTOKTYP_GREATERGREATER, BOA_ASTTOKTYP_GREATERTHAN);
        case '<':
            return boa_astlex_matchtokens(lex, '=', '<', BOA_ASTTOKTYP_LESSEQUAL, BOA_ASTTOKTYP_LESSLESS, BOA_ASTTOKTYP_LESSTHAN);
        case '*':
            return boa_astlex_matchtokens(lex, '=', '*', BOA_ASTTOKTYP_STAREQUAL, BOA_ASTTOKTYP_STARSTAR, BOA_ASTTOKTYP_STAR);
        case '=':
            {
                {
                    if(boa_astlex_match(lex, '='))
                    {
                        if(boa_astlex_match(lex, '='))
                        {
                            /*js-ism: === */
                        }
                        return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_EQUALEQUAL);
                    }
                    else
                    {
                        if(boa_astlex_match(lex, '>'))
                        {
                            return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_ARROW);
                        }
                        return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_EQUAL);
                    }
                }
            }
            break;
        case '|':
            {
                return boa_astlex_matchtokens(lex, '=', '|', BOA_ASTTOKTYP_BAREQUAL, BOA_ASTTOKTYP_BARBAR, BOA_ASTTOKTYP_BAR);
            }
            break;
        case '&':
            {
                return boa_astlex_matchtokens(lex, '=', '&', BOA_ASTTOKTYP_AMPERSANDEQUAL, BOA_ASTTOKTYP_AMPERSANDAMPERSAND, BOA_ASTTOKTYP_AMPERSAND);
            }
            break;
        case '.':
            {
                if(!boa_astlex_match(lex, '.'))
                {
                    return boa_astlex_maketoken(lex, BOA_ASTTOKTYP_DOT);
                }
                return boa_astlex_matchtoken(lex, '.', BOA_ASTTOKTYP_DOTDOTDOT, BOA_ASTTOKTYP_DOTDOT);
            }
            break;
        case '`':
            {
                return boa_astlex_scanstring(lex, true, true, '`');
            }
        case 34:
            {
                return boa_astlex_scanstring(lex, false, true, 34);
            }
            break;
        case '\'':
            {
                return boa_astlex_scanstring(lex, false, false, '\'');
            }
            break;
    }
    fprintf(stderr, "current=%s\n", lex->sourcedatacurrent);
    return boa_astlex_makeerrortoken(lex, "unexpected character <%c>", c);
}

static BoaAstRule g_astparserules[BOA_ASTTOKTYP_EOF + 1] = {};
static bool didsetuprules = false;
static jmp_buf g_parserjumpbuffer = {};

void boa_astparser_compilerinit(BoaAstParser* prs, BoaAstCompiler* ccx)
{
    ccx->scopedepth = 0;
    ccx->function = NULL;
    ccx->enclosing = (struct BoaAstCompiler*)prs->compiler;
    prs->compiler = ccx;
}

void boa_astparser_compilerend(BoaAstParser* prs, BoaAstCompiler* ccx)
{
    prs->compiler = (BoaAstCompiler*)ccx->enclosing;
}

void boa_astparser_scopebegin(BoaAstParser* prs)
{
    prs->compiler->scopedepth++;
}

void boa_astparser_scopeend(BoaAstParser* prs)
{
    prs->compiler->scopedepth--;
}

BoaAstRule* boa_astparser_getrule(BoaAstTokType type)
{
    return &g_astparserules[type];
}

bool boa_astparser_isatend(BoaAstParser* prs)
{
    return prs->currenttoken.type == BOA_ASTTOKTYP_EOF;
}

void boa_astparser_init(BoaState* state, BoaAstParser* prs)
{
    if(!didsetuprules)
    {
        didsetuprules = true;
        boa_astparser_setuprules();
    }
    prs->pstate = state;
    prs->haderror = false;
    prs->panicmode = false;
}

void boa_astparser_destroy(BoaAstParser* prs)
{
    (void)prs;
}

void boa_astparser_failactual(BoaAstParser* prs, BoaAstToken* token, const char* message)
{
    (void)token;
    if(prs->panicmode)
    {
        return;
    }
    boa_state_raiseerror(prs->pstate, message);
    prs->haderror = true;
    boa_astparser_sync(prs);
}

void boa_astparser_failatv(BoaAstParser* prs, BoaAstToken* token, const char* fmt, va_list args)
{
    boa_astparser_failactual(prs, token, boa_string_getdata(boa_state_errorfmtv(prs->pstate, token->line, fmt, args)));
}

void boa_astparser_failherefmt(BoaAstParser* prs, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    boa_astparser_failatv(prs, &prs->currenttoken, fmt, args);
    va_end(args);
}

void boa_astparser_failfmt(BoaAstParser* prs, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    boa_astparser_failatv(prs, &prs->previoustoken, fmt, args);
    va_end(args);
}

void boa_astparser_advance(BoaAstParser* prs)
{
    prs->previoustoken = prs->currenttoken;
    while(true)
    {
        prs->currenttoken = boa_astlex_scantoken(prs->pstate->activelexer);
        if(prs->currenttoken.type != BOA_ASTTOKTYP_ERROR)
        {
            break;
        }
        boa_astparser_failactual(prs, &prs->currenttoken, prs->currenttoken.start);
    }
}

bool boa_astparser_check(BoaAstParser* prs, BoaAstTokType type)
{
    return prs->currenttoken.type == type;
}

bool boa_astparser_match(BoaAstParser* prs, BoaAstTokType type)
{
    if(prs->currenttoken.type == type)
    {
        boa_astparser_advance(prs);
        return true;
    }
    return false;
}

bool boa_astparser_matchident(BoaAstParser* prs, const char* type)
{
    BoaAstTokType ctp;
    ctp = prs->currenttoken.type;
    if(ctp == BOA_ASTTOKTYP_IDENTIFIER || ctp == BOA_ASTTOKTYP_KWCLASS || ctp == BOA_ASTTOKTYP_KWTRY)
    {
        if(memcmp(prs->previoustoken.start, type, fmax(strlen(type), prs->previoustoken.length)))
        {
            boa_astparser_advance(prs);
            return true;
        }
    }
    return false;
}

void boa_astparser_consume(BoaAstParser* prs, BoaAstTokType type, const char* error)
{
    bool islinefeed;
    BoaString* r;
    if(prs->currenttoken.type == type)
    {
        boa_astparser_advance(prs);
        return;
    }
    islinefeed = prs->previoustoken.type == BOA_ASTTOKTYP_LINEFEED;
    r = boa_state_errorfmt(prs->pstate, prs->currenttoken.line, "expected %s, got <%.*s>", error, islinefeed ? 8 : prs->previoustoken.length, islinefeed ? "new line" : prs->previoustoken.start);
    boa_astparser_failactual(prs, &prs->currenttoken, boa_string_getdata(r));
}

bool boa_astparser_matchlinefeed(BoaAstParser* prs)
{
    if(!boa_astparser_match(prs, BOA_ASTTOKTYP_LINEFEED))
    {
        return false;
    }
    while(boa_astparser_match(prs, BOA_ASTTOKTYP_LINEFEED))
    {
    }
    return true;
}

void boa_astparser_ignorelinefeeds(BoaAstParser* prs)
{
    boa_astparser_matchlinefeed(prs);
}

BoaAstExpression* boa_astparser_parseblock(BoaAstParser* prs)
{
    BoaAstBlockExpr* expr;
    boa_astparser_scopebegin(prs);
    expr = boa_ast_makeblockstmt(prs->previoustoken.line);
    boa_astparser_ignorelinefeeds(prs);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE) && !boa_astparser_check(prs, BOA_ASTTOKTYP_EOF))
    {
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_SEMICOLON))
        {
            boa_astparser_ignorelinefeeds(prs);
            continue;
        }
        boa_dynlistexpr_push(&expr->statements, boa_astparser_parsestatement(prs));
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_match(prs, BOA_ASTTOKTYP_SEMICOLON);
        boa_astparser_ignorelinefeeds(prs);
    }
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACE, "'}'");
    boa_astparser_scopeend(prs);
    return (BoaAstExpression*)expr;
}

static BoaAstExpression* boa_astparser_parsestatementorblock(BoaAstParser* prs)
{
    boa_astparser_ignorelinefeeds(prs);
    if (boa_astparser_check(prs, BOA_ASTTOKTYP_LEFTBRACE))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "'{'");
        return boa_astparser_parseblock(prs);
    }
    return boa_astparser_parsestatement(prs);
}

BoaAstExpression* boa_astparser_parseprec(BoaAstParser* prs, BoaPrecedence precedence, bool err)
{
    BoaAstToken previous;
    BoaParsePrefixFn prefixrule;
    BoaParseInfixFn infixrule;
    BoaAstRule* rule;
    BoaAstExpression* expr;
    bool canassign;
    bool prevnewline;
    bool parserprevnewline;
    int exlen;
    int gotlen;
    const char* gotstr;
    const char* exstr;
    previous = prs->previoustoken;
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_advance(prs);
    rule = boa_astparser_getrule(prs->previoustoken.type);
    if(rule == NULL)
    {
        boa_astparser_failfmt(prs, "no rule for this token found");
    }
    prefixrule = rule->prefix;
    if(prefixrule == NULL)
    {
        if(!err)
        {
            return NULL;
        }
        /* todo: file start */
        prevnewline = ((previous.start != NULL) && (*previous.start == '\n'));
        parserprevnewline = ((prs->previoustoken.start != NULL) && (*prs->previoustoken.start == '\n'));
        exstr = "new line";
        gotstr = "new line";
        exlen = strlen(exstr);
        gotlen = strlen(gotstr);
        if(!prevnewline)
        {
            exlen = previous.length;
            exstr = previous.start;
        }
        if(!parserprevnewline)
        {
            gotlen = prs->previoustoken.length;
            gotstr = prs->previoustoken.start;
        }
        boa_astparser_failfmt(prs, "expected expression after <%.*s>, got <%.*s>", exlen, exstr, gotlen, gotstr);
        return NULL;
    }
    canassign = precedence <= BOA_ASTPREC_ASSIGNMENT;
    expr = prefixrule(prs, canassign);
    if(expr == NULL)
    {
        return NULL;
    }
    boa_astparser_ignorelinefeeds(prs);
    while(true)
    {
        boa_astparser_ignorelinefeeds(prs);
        if(precedence > boa_astparser_getrule(prs->currenttoken.type)->precedence)
        {
            break;
        }
        boa_astparser_advance(prs);
        infixrule = boa_astparser_getrule(prs->previoustoken.type)->infix;
        if(infixrule == NULL)
        {
            break;
        }
        expr = infixrule(prs, expr, canassign);
    }
    if(err && canassign && boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        boa_astparser_failfmt(prs, "invalid assigment target");
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulenumber(BoaAstParser* prs, bool canassign)
{
    (void)canassign;
    return (BoaAstExpression*)boa_ast_makeliteralexpr(prs->previoustoken.line, prs->previoustoken.tokvalue);
}

BoaAstExpression* boa_astparser_parselambda(BoaAstParser* prs, BoaAstFunctionExpr* lambda)
{
    lambda->body = boa_astparser_parsestmtorblock(prs);
    return (BoaAstExpression*)lambda;
}

BoaAstFuncParamExpr boa_astparser_makeparameter(const char* name, size_t length, uint8_t reg, BoaAstExpression* defval)
{
    BoaAstFuncParamExpr pm;
    pm.name = name;
    pm.length = length;
    pm.reg = reg;
    pm.defaultval = defval;
    return pm;
}

void boa_astparser_parseparams(BoaAstParser* prs, BoaDynListParam* parameters)
{
    bool haddefault;
    size_t dotlen;
    size_t arglength;
    const char* dotstr;
    const char* argname;
    BoaAstExpression* defval;
    haddefault = false;
    dotstr = boa_string_getdata(prs->pstate->strings.strdots);
    dotlen = boa_string_getlength(prs->pstate->strings.strdots);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTPAREN))
    {
        boa_astparser_ignorelinefeeds(prs);
        /* variadic argument ... */
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_DOTDOTDOT))
        {
            boa_dynlistparam_push(parameters, boa_astparser_makeparameter(dotstr, dotlen, 0, NULL));
            return;
        }
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "argument name");
        argname = prs->previoustoken.start;
        arglength = prs->previoustoken.length;
        defval = NULL;
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
        {
            haddefault = true;
            defval = boa_astparser_parseexpression(prs);
        }
        else if(haddefault)
        {
            boa_astparser_failfmt(prs, "default arguments must always be in the end of the argument list.");
        }
        boa_dynlistparam_push(parameters, boa_astparser_makeparameter(argname, arglength, 0, defval));
        boa_astparser_ignorelinefeeds(prs);
        if(!boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA))
        {
            break;
        }
    }
}

BoaAstExpression* boa_astparser_rulegroupingorlambda(BoaAstParser* prs, bool canassign)
{
    const char* start;
    const char* firstargstart;
    const char* argname;
    size_t line;
    size_t firstarglength;
    size_t arglength;
    BoaState* state;
    BoaAstLexer* lex;
    BoaAstFunctionExpr* lambda;
    BoaAstExpression* defvalue;
    BoaAstExpression* defval;
    BoaAstExpression* expr;
    bool hadarrow;
    bool hadvararg;
    bool haddefault;
    bool stop;
    (void)canassign;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_RIGHTPAREN))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_ARROW, "=> after lambda arguments");
        return boa_astparser_parselambda(prs, boa_ast_makelambdaexpr(prs->previoustoken.line));
    }
    start = prs->previoustoken.start;
    line = prs->previoustoken.line;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_IDENTIFIER) || boa_astparser_match(prs, BOA_ASTTOKTYP_DOTDOTDOT))
    {
        state = prs->pstate;
        firstargstart = prs->previoustoken.start;
        firstarglength = prs->previoustoken.length;
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA) || (boa_astparser_match(prs, BOA_ASTTOKTYP_RIGHTPAREN) && boa_astparser_match(prs, BOA_ASTTOKTYP_ARROW)))
        {
            hadarrow = prs->previoustoken.type == BOA_ASTTOKTYP_ARROW;
            hadvararg = prs->previoustoken.type == BOA_ASTTOKTYP_DOTDOTDOT;
            /* this is a lambda */
            lambda = boa_ast_makelambdaexpr(line);
            defvalue = NULL;
            haddefault = boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL);
            if(haddefault)
            {
                defvalue = boa_astparser_parseexpression(prs);
            }
            boa_astparser_ignorelinefeeds(prs);
            boa_dynlistparam_push(&lambda->parameters, boa_astparser_makeparameter(firstargstart, firstarglength, 0, defvalue));
            if(!hadvararg && prs->previoustoken.type == BOA_ASTTOKTYP_COMMA)
            {
                do
                {
                    stop = false;
                    if(boa_astparser_match(prs, BOA_ASTTOKTYP_DOTDOTDOT))
                    {
                        stop = true;
                    }
                    else
                    {
                        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "argument name");
                    }
                    argname = prs->previoustoken.start;
                    arglength = prs->previoustoken.length;
                    defval = NULL;
                    if(boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
                    {
                        defval = boa_astparser_parseexpression(prs);
                        haddefault = true;
                    }
                    else if(haddefault)
                    {
                        boa_astparser_failfmt(prs, "default arguments must always be in the end of the argument list.");
                    }
                    boa_dynlistparam_push(&lambda->parameters, boa_astparser_makeparameter(argname, arglength, 0, defval));
                    if(stop)
                    {
                        break;
                    }
                } while(boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA));
            }
            if(!hadarrow)
            {
                boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "')' after lambda parameters");
                boa_astparser_consume(prs, BOA_ASTTOKTYP_ARROW, "=> after lambda parameters");
            }
            return boa_astparser_parselambda(prs, lambda);
        }
        else
        {
            /* ouch, this was a grouping with a single identifier */
            lex = state->activelexer;
            lex->sourcedatacurrent = start;
            lex->sourcecurrentline = line;
            prs->currenttoken = boa_astlex_scantoken(lex);
            boa_astparser_advance(prs);
        }
    }
    expr = boa_astparser_parseexpression(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "')' after grouping expression");
    return expr;
}

BoaAstExpression* boa_astparser_ruleparsecall(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    size_t dotlen;
    const char* dotstr;
    BoaAstExpression* e;
    BoaAstCallExpr* expr;
    (void)canassign;
    dotstr = boa_string_getdata(prs->pstate->strings.strdots);
    dotlen = boa_string_getlength(prs->pstate->strings.strdots);
    expr = boa_ast_makecallexpr(prs->previoustoken.line, prev);
    boa_astparser_ignorelinefeeds(prs);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTPAREN))
    {
        e = boa_astparser_parseexpression(prs);
        boa_dynlistexpr_push(&expr->callargs, e);
        boa_astparser_ignorelinefeeds(prs);
        if(!boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA))
        {
            break;
        }
        boa_astparser_ignorelinefeeds(prs);
        if(e->type == BOA_ASTEXPRTYP_VARGET)
        {
            BoaAstVarGetExpr* ee = (BoaAstVarGetExpr*)e;
            /* variadic arg ... */
            if(ee->length == dotlen && memcmp(ee->name, dotstr, dotlen) == 0)
            {
                break;
            }
        }
    }
    if(expr->callargs.listcount > 255)
    {
        boa_astparser_failfmt(prs, "function cannot have more than 255 arguments, got %i", (int)expr->callargs.listcount);
    }
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "')' after arguments");
    return (BoaAstExpression*)expr;
}

BoaAstExpression* boa_astparser_ruleunary(BoaAstParser* prs, bool canassign)
{
    bool isrefsym;
    size_t line;
    BoaAstTokType op;
    BoaAstRefExpr* refexp;
    BoaAstExpression* targetexpr;
    (void)canassign;
    op = prs->previoustoken.type;
    line = prs->previoustoken.line;
    isrefsym = (prs->previoustoken.start[0] == '@');
    #if 1
    if(isrefsym)
    {
        targetexpr = boa_astparser_parseprec(prs, BOA_ASTPREC_CALL, false);
    }
    else
    #endif
    {
        targetexpr = boa_astparser_parseprec(prs, BOA_ASTPREC_UNARY, true);
    }
    if(isrefsym)
    {
        refexp = boa_ast_makerefexpr(line, targetexpr);
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
        {
            return (BoaAstExpression*)boa_ast_makeassignexpr(line, (BoaAstExpression*)refexp, boa_astparser_parseexpression(prs));
        }
        return (BoaAstExpression*)refexp;
    }
    return (BoaAstExpression*)boa_ast_makeunaryexpr(line, targetexpr, op);
}

BoaAstExpression* boa_astparser_rulebinary(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    bool invert;
    BoaAstTokType op;
    size_t line;
    BoaAstRule* rule;
    BoaAstExpression* expr;
    (void)canassign;
    invert = prs->previoustoken.type == BOA_ASTTOKTYP_BANG;
    if(invert)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_KWIS, "<is> after <!>");
    }
    op = prs->previoustoken.type;
    rule = boa_astparser_getrule(op);
    boa_astparser_ignorelinefeeds(prs);
    expr = boa_astparser_parseprec(prs, (BoaPrecedence)(rule->precedence + 1), true);
    boa_astparser_ignorelinefeeds(prs);
    line = prs->previoustoken.line;
    expr = (BoaAstExpression*)boa_ast_makebinaryexpr(line, prev, expr, op);
    if(invert)
    {
        expr = (BoaAstExpression*)boa_ast_makeunaryexpr(line, expr, BOA_ASTTOKTYP_BANG);
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulelogicaland(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    BoaAstExpression* expr;
    BoaAstTokType op;
    size_t line;
    (void)canassign;
    op = prs->previoustoken.type;
    line = prs->previoustoken.line;
    expr = boa_astparser_parseprec(prs, BOA_ASTPREC_AND, true);
    boa_astparser_ignorelinefeeds(prs);
    return (BoaAstExpression*)boa_ast_makebinaryexpr(line, prev, expr, op);
}

BoaAstExpression* boa_astparser_rulelogicalor(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    BoaAstExpression* expr;
    BoaAstTokType op;
    size_t line;
    (void)canassign;
    op = prs->previoustoken.type;
    line = prs->previoustoken.line;
    expr = boa_astparser_parseprec(prs, BOA_ASTPREC_OR, true);
    boa_astparser_ignorelinefeeds(prs);
    return (BoaAstExpression*)boa_ast_makebinaryexpr(line, prev, expr, op);
}

BoaAstExpression* boa_astparser_rulenullfilter(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    BoaAstTokType op;
    size_t line;
    (void)canassign;
    op = prs->previoustoken.type;
    line = prs->previoustoken.line;
    return (BoaAstExpression*)boa_ast_makebinaryexpr(line, prev, boa_astparser_parseprec(prs, BOA_ASTPREC_NULL, true), op);
}

BoaAstTokType boa_astparser_convertcompoundop(BoaAstTokType op)
{
    switch(op)
    {
        case BOA_ASTTOKTYP_PLUSEQUAL:
            return BOA_ASTTOKTYP_PLUS;
        case BOA_ASTTOKTYP_MINUSEQUAL:
            return BOA_ASTTOKTYP_MINUS;
        case BOA_ASTTOKTYP_STAREQUAL:
            return BOA_ASTTOKTYP_STAR;
        case BOA_ASTTOKTYP_SLASHEQUAL:
            return BOA_ASTTOKTYP_SLASH;
        case BOA_ASTTOKTYP_SHARPEQUAL:
            return BOA_ASTTOKTYP_SHARP;
        case BOA_ASTTOKTYP_PERCENTEQUAL:
            return BOA_ASTTOKTYP_PERCENT;
        case BOA_ASTTOKTYP_CARETEQUAL:
            return BOA_ASTTOKTYP_CARET;
        case BOA_ASTTOKTYP_BAREQUAL:
            return BOA_ASTTOKTYP_BAR;
        case BOA_ASTTOKTYP_AMPERSANDEQUAL:
            return BOA_ASTTOKTYP_AMPERSAND;
        case BOA_ASTTOKTYP_PLUSPLUS:
            return BOA_ASTTOKTYP_PLUS;
        case BOA_ASTTOKTYP_MINUSMINUS:
            return BOA_ASTTOKTYP_MINUS;
        default:
        {
            BOA_UTIL_UNREACHABLE();
        }
    }
    return BOA_ASTTOKTYP_EOF;
}

BoaAstExpression* boa_astparser_rulecompound(BoaAstParser* prs, BoaAstExpression* prev, bool canassign)
{
    size_t line;
    BoaAstTokType op;
    BoaAstRule* rule;
    BoaAstExpression* expr;
    BoaAstBinaryExpr* binary;
    (void)canassign;
    op = prs->previoustoken.type;
    line = prs->previoustoken.line;
    rule = boa_astparser_getrule(op);
    if(op == BOA_ASTTOKTYP_PLUSPLUS || op == BOA_ASTTOKTYP_MINUSMINUS)
    {
        expr = (BoaAstExpression*)boa_ast_makeliteralexpr(line, boa_value_makenumber(1));
    }
    else
    {
        expr = boa_astparser_parseprec(prs, (BoaPrecedence)(rule->precedence + 1), true);
    }
    line = prs->previoustoken.line;
    binary = boa_ast_makebinaryexpr(line, prev, expr, boa_astparser_convertcompoundop(op));
    /* to make sure we don't free it twice */
    binary->ignoreleft = true;
    return (BoaAstExpression*)boa_ast_makeassignexpr(line, prev, (BoaAstExpression*)binary);
}

BoaAstExpression* boa_astparser_ruleliteral(BoaAstParser* prs, bool canassign)
{
    size_t line;
    (void)canassign;
    line = prs->previoustoken.line;
    switch(prs->previoustoken.type)
    {
        case BOA_ASTTOKTYP_KWTRUE:
        {
            return (BoaAstExpression*)boa_ast_makeliteralexpr(line, boa_value_makebool(true));
        }
        case BOA_ASTTOKTYP_KWFALSE:
        {
            return (BoaAstExpression*)boa_ast_makeliteralexpr(line, boa_value_makebool(false));
        }
        case BOA_ASTTOKTYP_KWNULL:
        {
            return (BoaAstExpression*)boa_ast_makeliteralexpr(line, boa_value_makenull());
        }
        default:
            BOA_UTIL_UNREACHABLE();
    }
    return NULL;
}

BoaAstExpression* boa_astparser_rulestring(BoaAstParser* prs, bool canassign)
{
    BoaAstExpression* expr = (BoaAstExpression*)boa_ast_makeliteralexpr(prs->previoustoken.line, prs->previoustoken.tokvalue);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, expr, canassign);
    }
    return expr;
}

BoaAstExpression* boa_astparser_ruleinterpolation(BoaAstParser* prs, bool canassign)
{
    BoaValue tval;
    BoaString* str;
    BoaAstExpression* innerexpr;
    BoaAstStrTemplateExpr* expr;
    expr = boa_ast_makeinterpolationexpr(prs->previoustoken.line);
    do
    {
        tval = prs->previoustoken.tokvalue;
        str = boa_value_asstring(tval);
        if(str != NULL)
        {
            if(boa_string_getlength(str) > 0)
            {
                boa_dynlistexpr_push(&expr->expressions, (BoaAstExpression*)boa_ast_makeliteralexpr(prs->previoustoken.line, tval));
            }
        }
        innerexpr = boa_astparser_parseexpression(prs);
        boa_dynlistexpr_push(&expr->expressions, innerexpr);
    } while(boa_astparser_match(prs, BOA_ASTTOKTYP_STRTEMPLATE));
    boa_astparser_consume(prs, BOA_ASTTOKTYP_STRING, "end of template string");
    tval = prs->previoustoken.tokvalue;
    str = boa_value_asstring(tval);
    if(str != NULL)
    {
        if(boa_string_getlength(str) > 0)
        {
            boa_dynlistexpr_push(&expr->expressions, (BoaAstExpression*)boa_ast_makeliteralexpr(prs->previoustoken.line, prs->previoustoken.tokvalue));
        }
    }
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, (BoaAstExpression*)expr, canassign);
    }
    return (BoaAstExpression*)expr;
}

BoaAstExpression* boa_astparser_rulearray(BoaAstParser* prs, bool canassign)
{
    BoaAstLiteralArrayExpr* array = boa_ast_makearrayexpr(prs->previoustoken.line);
    boa_astparser_ignorelinefeeds(prs);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACKET))
    {
        boa_astparser_ignorelinefeeds(prs);
        boa_dynlistexpr_push(&array->exvalues, boa_astparser_parseexpression(prs));
        boa_astparser_ignorelinefeeds(prs);
        if(!boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA))
        {
            break;
        }
        boa_astparser_ignorelinefeeds(prs);
    }
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACKET, "']' after array");
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, (BoaAstExpression*)array, canassign);
    }
    return (BoaAstExpression*)array;
}

BoaAstExpression* boa_astparser_ruleobject(BoaAstParser* prs, bool canassign)
{
    BoaString* keystr;
    BoaAstLiteralObjectExpr* object;
    (void)canassign;
    object = boa_ast_makeobjectexpr(prs->previoustoken.line);
    boa_astparser_ignorelinefeeds(prs);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE))
    {
        keystr = NULL;
        boa_astparser_ignorelinefeeds(prs);
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_STRING))
        {
            keystr = boa_string_copylen(prs->pstate, prs->previoustoken.start+1, prs->previoustoken.length-2);
        }
        else if(boa_astparser_match(prs, BOA_ASTTOKTYP_IDENTIFIER))
        {
            keystr = boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length);
        }
        else
        {
            boa_astparser_failherefmt(prs, "expected identifier or string");
            return NULL;
        }
        boa_dynlistval_push(&object->objexkeys, boa_value_fromobject(keystr));
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_COLON, "':' after key string");
        boa_astparser_ignorelinefeeds(prs);
        boa_dynlistexpr_push(&object->objexvalues, boa_astparser_parseexpression(prs));
        if(!boa_astparser_match(prs, BOA_ASTTOKTYP_COMMA))
        {
            break;
        }
        boa_astparser_ignorelinefeeds(prs);
    }
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACE, "'}' after object");
    return (BoaAstExpression*)object;
}

/* Speculatively scan for object notation ('{ key: value (, key: value)* }'),
   so that a '{' expression can also be used as a code block. The lexer is passed
   in by value and is thus left untouched. */
static bool boa_astparser_scanobjectnotation(BoaAstLexer* lex, BoaAstToken token)
{
    size_t depth;
    depth = 0;
    for(;;)
    {
        while(token.type == BOA_ASTTOKTYP_LINEFEED)
        {
            token = boa_astlex_scantoken(lex);
        }
        /* an empty object is still object notation */
        if(token.type == BOA_ASTTOKTYP_RIGHTBRACE)
        {
            return depth == 0;
        }
        if(token.type != BOA_ASTTOKTYP_IDENTIFIER && token.type != BOA_ASTTOKTYP_STRING)
        {
            return false;
        }
        token = boa_astlex_scantoken(lex);
        while(token.type == BOA_ASTTOKTYP_LINEFEED)
        {
            token = boa_astlex_scantoken(lex);
        }
        if(token.type != BOA_ASTTOKTYP_COLON)
        {
            return false;
        }
        /* now skip the value, tracking nesting so that the ',' and '}' that
           end this pair can be told apart from the ones of nested structures */
        for(;;)
        {
            token = boa_astlex_scantoken(lex);
            if(token.type == BOA_ASTTOKTYP_EOF || token.type == BOA_ASTTOKTYP_ERROR)
            {
                return false;
            }
            if(depth == 0 && (token.type == BOA_ASTTOKTYP_COMMA || token.type == BOA_ASTTOKTYP_RIGHTBRACE))
            {
                break;
            }
            switch(token.type)
            {
                case BOA_ASTTOKTYP_LEFTBRACE:
                case BOA_ASTTOKTYP_LEFTPAREN:
                case BOA_ASTTOKTYP_LEFTBRACKET:
                    depth++;
                    break;
                case BOA_ASTTOKTYP_RIGHTBRACE:
                case BOA_ASTTOKTYP_RIGHTPAREN:
                case BOA_ASTTOKTYP_RIGHTBRACKET:
                    depth--;
                    break;
                default:
                    break;
            }
        }
        if(token.type == BOA_ASTTOKTYP_RIGHTBRACE)
        {
            return true;
        }
        /* step over the ',' and expect another key */
        token = boa_astlex_scantoken(lex);
    }
}

bool boa_astparser_braceisobject(BoaAstParser* prs)
{
    BoaAstLexer lex;
    lex = *(prs->pstate->activelexer);
    return boa_astparser_scanobjectnotation(&lex, prs->currenttoken);
}

BoaAstExpression* boa_astparser_rulebrace(BoaAstParser* prs, bool canassign)
{
    if(boa_astparser_braceisobject(prs))
    {
        return boa_astparser_ruleobject(prs, canassign);
    }
    return boa_astparser_parseblock(prs);
}

BoaAstExpression* boa_astparser_parsevarexprbase(BoaAstParser* prs, bool canassign, bool isnew)
{
    bool hadargs;
    BoaAstCallExpr* call;
    BoaAstExpression* expr;
    expr = (BoaAstExpression*)boa_ast_makevargetexpr(prs->previoustoken.line, prs->previoustoken.start, prs->previoustoken.length);
    if(isnew)
    {
        hadargs = boa_astparser_check(prs, BOA_ASTTOKTYP_LEFTPAREN);
        call = NULL;
        if(hadargs)
        {
            boa_astparser_advance(prs);
            call = (BoaAstCallExpr*)boa_astparser_ruleparsecall(prs, expr, false);
        }
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACE))
        {
            if(call == NULL)
            {
                call = boa_ast_makecallexpr(expr->line, expr);
            }
            call->init = boa_astparser_ruleobject(prs, false);
        }
        else if(!hadargs)
        {
            boa_astparser_failherefmt(prs, "expected %s, got <%.*s>", "argument list for instance creation", prs->previoustoken.length, prs->previoustoken.start);
        }
        return (BoaAstExpression*)call;
    }
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, expr, canassign);
    }
    if(canassign && boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        return (BoaAstExpression*)boa_ast_makeassignexpr(prs->previoustoken.line, expr, boa_astparser_parseexpression(prs));
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulevarexpr(BoaAstParser* prs, bool canassign)
{
    return boa_astparser_parsevarexprbase(prs, canassign, false);
}

BoaAstExpression* boa_astparser_rulenewexpr(BoaAstParser* prs, bool canassign)
{
    (void)canassign;
    boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "class name after <new>");
    return boa_astparser_parsevarexprbase(prs, false, true);
}

BoaAstExpression* boa_astparser_ruledot(BoaAstParser* prs, BoaAstExpression* previous, bool canassign)
{
    const char* name;
    size_t length;
    size_t line;
    bool ignored;
    BoaAstExpression* expr;
    line = prs->previoustoken.line;
    ignored = prs->previoustoken.type == BOA_ASTTOKTYP_SMALLARROW;
    boa_astparser_ignorelinefeeds(prs);
    /* class and super are allowed field names */
    if(!(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCLASS) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWSUPER) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWTRY)))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, ignored ? "property name after < -> >" : "property name after <.>");
    }
    name = prs->previoustoken.start;
    length = prs->previoustoken.length;
    if(!ignored && canassign && boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        return (BoaAstExpression*)boa_ast_makesetexpr(line, previous, name, length, boa_astparser_parseexpression(prs), true);
    }
    expr = (BoaAstExpression*)boa_ast_makegetexpr(line, previous, name, length, false, ignored, true);
    if(!ignored && boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, expr, canassign);
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulerange(BoaAstParser* prs, BoaAstExpression* previous, bool canassign)
{
    size_t line;
    (void)canassign;
    line = prs->previoustoken.line;
    return (BoaAstExpression*)boa_ast_makerangeexpr(line, previous, boa_astparser_parseexpression(prs));
}

BoaAstExpression* boa_astparser_ruleternaryorquestion(BoaAstParser* prs, BoaAstExpression* previous, bool canassign)
{
    size_t line;
    bool ignored;
    BoaAstExpression* ifbranch;
    BoaAstExpression* elsebranch;
    (void)canassign;
    line = prs->previoustoken.line;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_DOT) /* || boa_astparser_match(prs, BOA_ASTTOKTYP_SMALLARROW)*/)
    {
        ignored = prs->previoustoken.type == BOA_ASTTOKTYP_SMALLARROW;
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, ignored ? "property name after <->>" : "property name after <.>");
        return (BoaAstExpression*)boa_ast_makegetexpr(line, previous, prs->previoustoken.start, prs->previoustoken.length, true, ignored, true);
    }
    ifbranch = boa_astparser_parseexpression(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_COLON, "':' after expression");
    elsebranch = boa_astparser_parseexpression(prs);
    return (BoaAstExpression*)boa_ast_maketernaryexpr(line, previous, ifbranch, elsebranch);
}

BoaAstExpression* boa_astparser_parsesubscript(BoaAstParser* prs, BoaAstExpression* previous, bool canassign)
{
    size_t line;
    BoaAstExpression* index;
    BoaAstExpression* expr;
    line = prs->previoustoken.line;
    index = boa_astparser_parseexpression(prs);
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACKET, "']' after subscript");
    expr = (BoaAstExpression*)boa_ast_makesubscriptexpr(line, previous, index);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, expr, canassign);
    }
    else if(canassign && boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        return (BoaAstExpression*)boa_ast_makeassignexpr(prs->previoustoken.line, expr, boa_astparser_parseexpression(prs));
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulethis(BoaAstParser* prs, bool canassign)
{
    BoaAstExpression* expr;
    expr = (BoaAstExpression*)boa_ast_makethisexpr(prs->previoustoken.line);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACKET))
    {
        return boa_astparser_parsesubscript(prs, expr, canassign);
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulesuper(BoaAstParser* prs, bool canassign)
{
    size_t line;
    bool ignoring;
    BoaAstExpression* res;
    BoaAstExpression* expr;
    (void)canassign;
    line = prs->previoustoken.line;
    if(!(boa_astparser_match(prs, BOA_ASTTOKTYP_DOT) || boa_astparser_match(prs, BOA_ASTTOKTYP_SMALLARROW)))
    {
        expr = (BoaAstExpression*)boa_ast_makesuperexpr(line, prs->pstate->strings.strconstructor, false);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTPAREN, "<(> after <super>");
        res = boa_astparser_ruleparsecall(prs, expr, false);
        return res;
    }
    ignoring = prs->previoustoken.type == BOA_ASTTOKTYP_SMALLARROW;
    boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, ignoring ? "super method name after <->>" : "super method name after <.>");
    expr = (BoaAstExpression*)boa_ast_makesuperexpr(line, boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length), ignoring);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN))
    {
        res = boa_astparser_ruleparsecall(prs, expr, false);
        return res;
    }
    return expr;
}

BoaAstExpression* boa_astparser_rulenothing(BoaAstParser* prs, bool canassign)
{
    (void)prs;
    (void)canassign;
    return NULL;
}

BoaAstExpression* boa_astparser_rulefunction(BoaAstParser* prs, bool canassign)
{
    (void)canassign;
    return boa_astparser_parsefunction(prs);
}

BoaAstExpression* boa_astparser_rulereference(BoaAstParser* prs, bool canassign)
{
    size_t line;
    BoaAstRefExpr* expr;
    (void)canassign;
    line = prs->previoustoken.line;
    boa_astparser_ignorelinefeeds(prs);
    expr = boa_ast_makerefexpr(line, boa_astparser_parseprec(prs, BOA_ASTPREC_CALL, false));
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        return (BoaAstExpression*)boa_ast_makeassignexpr(line, (BoaAstExpression*)expr, boa_astparser_parseexpression(prs));
    }
    return (BoaAstExpression*)expr;
}


BoaAstExpression* boa_astparser_parsestatement(BoaAstParser* prs)
{
    BoaAstExpression* expr;
    if(setjmp(g_parserjumpbuffer))
    {
        return NULL;
    }
    boa_astparser_ignorelinefeeds(prs);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWVAR) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWCONST))
    {
        return boa_astparser_parsevardecl(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCLASS))
    {
        return boa_astparser_parseclass(prs, false);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWIF))
    {
        return boa_astparser_parseif(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWSWITCH))
    {
        return boa_astparser_parseswitch(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWTRY))
    {
        return boa_astparser_parsetry(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWTHROW))
    {
        return boa_astparser_parsethrow(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWFOR))
    {
        return boa_astparser_parsefor(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWWHILE))
    {
        return boa_astparser_parsewhile(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCONTINUE))
    {
        return (BoaAstExpression*)boa_ast_makecontinuestmt(prs->previoustoken.line);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWBREAK))
    {
        return (BoaAstExpression*)boa_ast_makebreakstmt(prs->previoustoken.line);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWFUNCTION) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWEXPORT))
    {
        return boa_astparser_parsefunction(prs);
    }
    else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWRETURN))
    {
        return boa_astparser_parsereturn(prs);
    }
    expr = boa_astparser_parseexpression(prs);
    if(expr == NULL)
    {
        return NULL;
    }
    return (BoaAstExpression*)boa_ast_makeexprstmt(prs->previoustoken.line, expr);
}

BoaAstExpression* boa_astparser_parseexpression(BoaAstParser* prs)
{
    BoaAstExpression* expr;
    boa_astparser_ignorelinefeeds(prs);
    expr = boa_astparser_parseprec(prs, BOA_ASTPREC_ASSIGNMENT, true);
    if(expr != NULL)
    {
        return expr;
    }
    return NULL;
}

BoaAstExpression* boa_astparser_parsevardecl(BoaAstParser* prs)
{
    const char* name;
    size_t length;
    BoaAstExpression* init;
    bool constant;
    size_t line;
    constant = prs->previoustoken.type == BOA_ASTTOKTYP_KWCONST;
    line = prs->previoustoken.line;
    init = NULL;
    boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "variable name");
    name = prs->previoustoken.start;
    length = prs->previoustoken.length;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_EQUAL))
    {
        init = boa_astparser_parseexpression(prs);
    }
    return (BoaAstExpression*)boa_ast_makevardeclstmt(line, name, length, init, constant);
}

BoaAstExpression* boa_astparser_parsetry(BoaAstParser* prs)
{
    size_t line;
    BoaAstExpression* tryblock;
    BoaAstExpression* catchblock;
    BoaAstExpression* finallyblock;
    const char* catchvarstr;
    size_t catchvarlen;
    line = prs->previoustoken.line;
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "Expect <{> after <try>");
    tryblock = boa_astparser_parseblock(prs);
    catchblock = NULL;
    finallyblock = NULL;
    catchvarstr = NULL;
    catchvarlen = 0;
    boa_astparser_ignorelinefeeds(prs);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCATCH))
    {
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN))
        {
            boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "expected identifier after <catch(>");
            catchvarstr = prs->previoustoken.start;
            catchvarlen = prs->previoustoken.length;
            boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "expected <)> after catch identifier");
        }
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "expected <{> after <catch>");
        catchblock = boa_astparser_parseblock(prs);
    }
    boa_astparser_ignorelinefeeds(prs);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWFINALLY))
    {
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "expected <{> after <finally>");
        finallyblock = boa_astparser_parseblock(prs);
    }
    if(catchblock == NULL && finallyblock == NULL)
    {
        boa_state_raiseerror(prs->pstate, "expected <catch> or <finally> after <try>");
    }
    return (BoaAstExpression*)boa_ast_maketrystmt(line, tryblock, catchblock, finallyblock, catchvarstr, catchvarlen);
}

BoaAstExpression* boa_astparser_parsethrow(BoaAstParser* prs)
{
    size_t line;
    BoaAstExpression* exvalue;
    line = prs->previoustoken.line;
    exvalue = boa_astparser_parseexpression(prs);
    return (BoaAstExpression*)boa_ast_makethrowstmt(line, exvalue);
}

BoaAstExpression* boa_astparser_parseif(BoaAstParser* prs)
{
    BoaAstExpression* ifbranch;
    BoaDynListExpr* elseifconditions;
    BoaDynListExpr* elseifbranches;
    BoaAstExpression* elsebranch;
    BoaAstExpression* condition;
    BoaAstExpression* e;
    size_t line;
    bool invert;
    bool hadparen;
    line = prs->previoustoken.line;
    invert = boa_astparser_match(prs, BOA_ASTTOKTYP_BANG);
    hadparen = boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN);
    condition = boa_astparser_parseexpression(prs);
    if(hadparen)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)>");
    }
    if(invert)
    {
        condition = (BoaAstExpression*)boa_ast_makeunaryexpr(condition->line, condition, BOA_ASTTOKTYP_BANG);
    }
    boa_astparser_ignorelinefeeds(prs);
    ifbranch = boa_astparser_parsestatementorblock(prs);
    elseifconditions = NULL;
    elseifbranches = NULL;
    elsebranch = NULL;
    boa_astparser_ignorelinefeeds(prs);
    while(boa_astparser_match(prs, BOA_ASTTOKTYP_KWELSE))
    {
        /* else if */
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWIF))
        {
            if(elseifconditions == NULL)
            {
                elseifconditions = boa_ast_allocexprlist();
                elseifbranches = boa_ast_allocstmtlist();
            }
            invert = boa_astparser_match(prs, BOA_ASTTOKTYP_BANG);
            hadparen = boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN);
            e = boa_astparser_parseexpression(prs);
            if(hadparen)
            {
                boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)>");
            }
            if(invert)
            {
                e = (BoaAstExpression*)boa_ast_makeunaryexpr(condition->line, e, BOA_ASTTOKTYP_BANG);
            }
            boa_dynlistexpr_push(elseifconditions, e);
            boa_astparser_ignorelinefeeds(prs);
            boa_dynlistexpr_push(elseifbranches, boa_astparser_parsestatementorblock(prs));
            boa_astparser_ignorelinefeeds(prs);
            continue;
        }
        /* else */
        if(elsebranch != NULL)
        {
            boa_astparser_failfmt(prs, "if-statement can have only one else-branch");
        }
        boa_astparser_ignorelinefeeds(prs);
        elsebranch = boa_astparser_parsestatementorblock(prs);
        boa_astparser_ignorelinefeeds(prs);
    }
    return (BoaAstExpression*)boa_ast_makeifstatement(line, condition, ifbranch, elsebranch, elseifconditions, elseifbranches);
}

BoaAstExpression* boa_astparser_parseswitch(BoaAstParser* prs)
{
    bool hadparen;
    size_t line;
    BoaAstExpression* condition;
    BoaAstBlockExpr* bodyblock;
    BoaAstExpression* casecond;
    BoaAstSwitchExpr* switchexpr;
    line = prs->previoustoken.line;
    hadparen = boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN);
    condition = boa_astparser_parseexpression(prs);
    if(hadparen)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)> after switch condition");
    }
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "<{> before switch body");
    boa_astparser_ignorelinefeeds(prs);
    switchexpr = boa_ast_makeswitchstatement(line, condition);
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE) && !boa_astparser_check(prs, BOA_ASTTOKTYP_EOF))
    {
        casecond = NULL;
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCASE))
        {
            casecond = boa_astparser_parseexpression(prs);
            boa_astparser_consume(prs, BOA_ASTTOKTYP_COLON, "<:> after case expression");
        }
        else if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWDEFAULT))
        {
            boa_astparser_consume(prs, BOA_ASTTOKTYP_COLON, "<:> after default");
        }
        else
        {
            boa_astparser_consume(prs, BOA_ASTTOKTYP_KWCASE, "expected <case> or <default>");
        }
        boa_astparser_ignorelinefeeds(prs);
        bodyblock = boa_ast_makeblockstmt(prs->previoustoken.line);
        while(!boa_astparser_check(prs, BOA_ASTTOKTYP_KWCASE) &&
              !boa_astparser_check(prs, BOA_ASTTOKTYP_KWDEFAULT) &&
              !boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE) &&
              !boa_astparser_check(prs, BOA_ASTTOKTYP_EOF))
        {
            boa_dynlistexpr_push(&bodyblock->statements, boa_astparser_parsestatement(prs));
            boa_astparser_ignorelinefeeds(prs);
        }
        boa_dynlistexpr_push(&switchexpr->caseconditions, casecond);
        boa_dynlistexpr_push(&switchexpr->casebodies, (BoaAstExpression*)bodyblock);
    }
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACE, "<}> after switch body");
    return (BoaAstExpression*)switchexpr;
}

BoaAstExpression* boa_astparser_parsefor(BoaAstParser* prs)
{
    BoaAstExpression* var;
    BoaAstExpression* init;
    BoaAstExpression* condition;
    BoaAstExpression* increment;
    size_t line;
    bool hadparen;
    bool cstyle;
    line = prs->previoustoken.line;
    hadparen = boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN);
    var = NULL;
    init = NULL;
    if(!boa_astparser_check(prs, BOA_ASTTOKTYP_SEMICOLON))
    {
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWVAR))
        {
            var = boa_astparser_parsevardecl(prs);
        }
        else
        {
            init = boa_astparser_parseexpression(prs);
        }
    }
    cstyle = !boa_astparser_match(prs, BOA_ASTTOKTYP_KWIN);
    condition = NULL;
    increment = NULL;
    if(cstyle)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_SEMICOLON, "<;>");
        condition = boa_astparser_check(prs, BOA_ASTTOKTYP_SEMICOLON) ? NULL : boa_astparser_parseexpression(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_SEMICOLON, "<;>");
        increment = boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTPAREN) ? NULL : boa_astparser_parseexpression(prs);
    }
    else
    {
        condition = boa_astparser_parseexpression(prs);
        if(var == NULL)
        {
            boa_astparser_failfmt(prs, "for-loops using in-iteration must declare a new variable");
        }
    }
    if(hadparen)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)>");
    }
    boa_astparser_ignorelinefeeds(prs);
    return (BoaAstExpression*)boa_ast_makeforstmt(line, init, var, condition, increment, boa_astparser_parsestatementorblock(prs), cstyle);
}

BoaAstExpression* boa_astparser_parsewhile(BoaAstParser* prs)
{
    size_t line;
    bool hadparen;
    BoaAstExpression* condition;
    BoaAstExpression* body;
    line = prs->previoustoken.line;
    hadparen = boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN);
    condition = boa_astparser_parseexpression(prs);
    if(hadparen)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)>");
    }
    boa_astparser_ignorelinefeeds(prs);
    body = boa_astparser_parsestatementorblock(prs);
    return (BoaAstExpression*)boa_ast_makewhilestmt(line, condition, body);
}

BoaAstExpression* boa_astparser_parsefunction(BoaAstParser* prs)
{
    size_t line;
    size_t namelen;
    bool isexport;
    bool noname;
    const char* fnname;
    BoaAstFunctionExpr* lambda;
    BoaAstIndexSetExpr* to;
    BoaAstCompiler stackcc;
    BoaAstFunctionExpr* function;
    noname = false;
    fnname = "anonymous";
    namelen = strlen(fnname);
    isexport = prs->previoustoken.type == BOA_ASTTOKTYP_KWEXPORT;
    if(isexport)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_KWFUNCTION, "<function> after <export>");
    }
    line = prs->previoustoken.line;
    if(boa_astparser_check(prs, BOA_ASTTOKTYP_IDENTIFIER))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "function name");
        fnname = prs->previoustoken.start;
        namelen = prs->previoustoken.length;
    }
    else
    {
        noname = true;
    }
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_DOT))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "function name");
        lambda = boa_ast_makelambdaexpr(line);
        to = boa_ast_makesetexpr(line, (BoaAstExpression*)boa_ast_makevargetexpr(line, fnname, namelen), prs->previoustoken.start, prs->previoustoken.length, (BoaAstExpression*)lambda, true);
        boa_astparser_compilerinit(prs, &stackcc);
        boa_astparser_scopebegin(prs);
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN))
        {
            boa_astparser_parseparams(prs, &lambda->parameters);
            if(lambda->parameters.listcount > 255)
            {
                boa_astparser_failfmt(prs, "function cannot have more than 255 arguments, got %i", (int)lambda->parameters.listcount);
            }
            boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)> after function arguments");
        }
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "expect <{> before function body");
        lambda->body = boa_astparser_parseblock(prs);
        boa_astparser_scopeend(prs);
        boa_astparser_compilerend(prs, &stackcc);
        return (BoaAstExpression*)boa_ast_makeexprstmt(line, (BoaAstExpression*)to);
    }
    if(noname)
    {
        function = boa_ast_makelambdaexpr(line);
    }
    else
    {
        function = boa_ast_makefuncdefstmt(line, fnname, namelen);
    }
    function->exported = isexport;
    boa_astparser_compilerinit(prs, &stackcc);
    boa_astparser_scopebegin(prs);
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTPAREN))
    {
        boa_astparser_parseparams(prs, &function->parameters);
        if(function->parameters.listcount > 255)
        {
            boa_astparser_failfmt(prs, "function cannot have more than 255 arguments, got %i", (int)function->parameters.listcount);
        }
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)> after function arguments");
    }
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "expect <{> before function body");
    function->body = boa_astparser_parseblock(prs);
    boa_astparser_scopeend(prs);
    boa_astparser_compilerend(prs, &stackcc);
    return (BoaAstExpression*)function;
}

BoaAstExpression* boa_astparser_parsereturn(BoaAstParser* prs)
{
    size_t line = prs->previoustoken.line;
    BoaAstExpression* expr = NULL;
    if(!boa_astparser_check(prs, BOA_ASTTOKTYP_SEMICOLON) && !boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE) && !boa_astparser_check(prs, BOA_ASTTOKTYP_EOF))
    {
        expr = boa_astparser_parseexpression(prs);
    }
    return (BoaAstExpression*)boa_ast_makereturnstmt(line, expr);
}

BoaAstExpression* boa_astparser_parsefield(BoaAstParser* prs, BoaString* name, bool isstatic)
{
    size_t line = prs->previoustoken.line;
    BoaAstExpression* getter = NULL;
    BoaAstExpression* setter = NULL;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_ARROW))
    {
        getter = boa_astparser_parsestmtorblock(prs);
    }
    else
    {
        /* will be BOA_ASTTOKTYP_LEFTBRACE, otherwise this method won't be called */
        boa_astparser_match(prs, BOA_ASTTOKTYP_LEFTBRACE);
        boa_astparser_ignorelinefeeds(prs);
        if(boa_astparser_matchident(prs, "get"))
        {
            /* ignore it if it's present */
            boa_astparser_match(prs, BOA_ASTTOKTYP_ARROW);
            getter = boa_astparser_parsestmtorblock(prs);
        }
        boa_astparser_ignorelinefeeds(prs);
        if(boa_astparser_matchident(prs, "set"))
        {
            /* Ignore it if it's present */
            boa_astparser_match(prs, BOA_ASTTOKTYP_ARROW);
            setter = boa_astparser_parsestmtorblock(prs);
        }
        if(getter == NULL && setter == NULL)
        {
            boa_astparser_failfmt(prs, "expected declaration of either getter or setter, got none");
        }
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACE, "<}> after field declaration");
    }
    return (BoaAstExpression*)boa_ast_makefieldstmt(line, name, getter, setter, isstatic);
}

/* clang-format off */
static BoaAstTokType operators[] = {
    BOA_ASTTOKTYP_PLUS, BOA_ASTTOKTYP_MINUS, BOA_ASTTOKTYP_STAR, BOA_ASTTOKTYP_PERCENT,
    BOA_ASTTOKTYP_SLASH, BOA_ASTTOKTYP_SHARP, BOA_ASTTOKTYP_BANG, BOA_ASTTOKTYP_LESSTHAN,
    BOA_ASTTOKTYP_LESSEQUAL, BOA_ASTTOKTYP_GREATERTHAN, BOA_ASTTOKTYP_GREATEREQUAL, BOA_ASTTOKTYP_EQUALEQUAL,
    BOA_ASTTOKTYP_LEFTBRACKET, BOA_ASTTOKTYP_EOF
};
/* clang-format on */

BoaAstExpression* boa_astparser_parsemethod(BoaAstParser* prs, bool isstatic)
{
    size_t i;
    BoaString* name;
    BoaAstMethodExpr* method;
    BoaAstCompiler stackcc;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWSTATIC))
    {
        isstatic = true;
    }
    name = NULL;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWOPERATOR))
    {
        if(isstatic)
        {
            boa_astparser_failfmt(prs, "operator methods cannot be static or defined in static classes");
        }
        i = 0;
        while(operators[i] != BOA_ASTTOKTYP_EOF)
        {
            if(boa_astparser_match(prs, operators[i]))
            {
                break;
            }
            i++;
        }
        if(prs->previoustoken.type == BOA_ASTTOKTYP_LEFTBRACKET)
        {
            boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACKET, "<]> after <[> in op method declaration");
            name = prs->pstate->strings.stropindex;
        }
        else
        {
            name = boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length);
        }
    }
    else
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "method name");
        name = boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length);
        if(boa_astparser_check(prs, BOA_ASTTOKTYP_LEFTBRACE) || boa_astparser_check(prs, BOA_ASTTOKTYP_ARROW))
        {
            return boa_astparser_parsefield(prs, name, isstatic);
        }
    }
    method = boa_ast_makemethoddefstmt(prs->previoustoken.line, name, isstatic);
    boa_astparser_compilerinit(prs, &stackcc);
    boa_astparser_scopebegin(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTPAREN, "<(> after method name");
    boa_astparser_parseparams(prs, &method->parameters);
    if(method->parameters.listcount > 255)
    {
        boa_astparser_failfmt(prs, "function cannot have more than 255 arguments, got %i", (int)method->parameters.listcount);
    }
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTPAREN, "<)> after method arguments");
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "expect <{> before method body");
    method->body = boa_astparser_parseblock(prs);
    boa_astparser_scopeend(prs);
    boa_astparser_compilerend(prs, &stackcc);
    return (BoaAstExpression*)method;
}

BoaAstExpression* boa_astparser_parseclass(BoaAstParser* prs, bool optionalname)
{
    size_t line;
    bool isstatic;
    BoaString* name;
    BoaString* super;
    BoaAstClassExpr* klass;
    bool finishedparsingfields;
    bool fieldisstatic;
    BoaAstExpression* var;
    BoaAstExpression* method;
    if(setjmp(g_parserjumpbuffer))
    {
        return NULL;
    }
    line = prs->previoustoken.line;
    isstatic = prs->previoustoken.type == BOA_ASTTOKTYP_KWSTATIC;
    if(isstatic)
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_KWCLASS, "<class> after <static>");
    }
    if(optionalname && !boa_astparser_check(prs, BOA_ASTTOKTYP_IDENTIFIER))
    {
        /* anonymous class, still needs a name for the class registry */
        name = (BoaString*)boa_value_asobject(boa_string_valformat(prs->pstate, "anonymous class :#", (BoaNumber)line));
    }
    else
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "class name after <class>");
        name = boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length);
    }
    super = NULL;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_COLON) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWEXTENDS))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_IDENTIFIER, "super class name after <:> (or <extends>)");
        super = boa_string_copylen(prs->pstate, prs->previoustoken.start, prs->previoustoken.length);
        if(super == name)
        {
            boa_astparser_failfmt(prs, "class cannot inherit itself");
        }
    }
    klass = boa_ast_makeclassdefstmt(line, name, super);
    boa_astparser_ignorelinefeeds(prs);
    boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "<{> before class body");
    boa_astparser_ignorelinefeeds(prs);
    finishedparsingfields = false;
    while(!boa_astparser_check(prs, BOA_ASTTOKTYP_RIGHTBRACE))
    {
        fieldisstatic = false;
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWSTATIC))
        {
            fieldisstatic = true;
            if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWVAR))
            {
                if(finishedparsingfields)
                {
                    boa_astparser_failfmt(prs, "all static fields must be defined before the methods");
                }
                var = boa_astparser_parsevardecl(prs);
                if(var != NULL)
                {
                    boa_dynlistexpr_push(&klass->staticfields, var);
                }
                boa_astparser_ignorelinefeeds(prs);
                continue;
            }
            else
            {
                finishedparsingfields = true;
            }
        }
        method = boa_astparser_parsemethod(prs, isstatic || fieldisstatic);
        if(method != NULL)
        {
            boa_dynlistexpr_push(&klass->staticfields, method);
        }
        boa_astparser_ignorelinefeeds(prs);
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_SEMICOLON))
        {
        }
        boa_astparser_ignorelinefeeds(prs);
    }
    boa_astparser_consume(prs, BOA_ASTTOKTYP_RIGHTBRACE, "<}> after class body");
    return (BoaAstExpression*)klass;
}

BoaAstExpression* boa_astparser_ruleclass(BoaAstParser* prs, bool canassign)
{
    (void)canassign;
    return boa_astparser_parseclass(prs, true);
}

BoaAstExpression* boa_astparser_ruleif(BoaAstParser* prs, bool canassign)
{
    (void)canassign;
    return boa_astparser_parseif(prs);
}

void boa_astparser_sync(BoaAstParser* prs)
{
    prs->panicmode = false;
    while(prs->currenttoken.type != BOA_ASTTOKTYP_EOF)
    {
        if(prs->previoustoken.type == BOA_ASTTOKTYP_LINEFEED)
        {
            longjmp(g_parserjumpbuffer, 1);
            return;
        }
        switch(prs->currenttoken.type)
        {
            case BOA_ASTTOKTYP_KWCLASS:
            case BOA_ASTTOKTYP_KWFUNCTION:
            case BOA_ASTTOKTYP_KWEXPORT:
            case BOA_ASTTOKTYP_KWVAR:
            case BOA_ASTTOKTYP_KWCONST:
            case BOA_ASTTOKTYP_KWFOR:
            case BOA_ASTTOKTYP_KWSTATIC:
            case BOA_ASTTOKTYP_KWIF:
            case BOA_ASTTOKTYP_KWSWITCH:
            case BOA_ASTTOKTYP_KWWHILE:
            case BOA_ASTTOKTYP_KWRETURN:
            {
                longjmp(g_parserjumpbuffer, 1);
                return;
            }
            default:
            {
                boa_astparser_advance(prs);
            }
        }
    }
}

BoaAstExpression* boa_astparser_parsestmtorblock(BoaAstParser* prs)
{
    boa_astparser_ignorelinefeeds(prs);
    if(boa_astparser_check(prs, BOA_ASTTOKTYP_LEFTBRACE))
    {
        boa_astparser_consume(prs, BOA_ASTTOKTYP_LEFTBRACE, "<{> before block");
        return boa_astparser_parseblock(prs);
    }
    return boa_astparser_parsestatement(prs);
}

BoaAstExpression* boa_astparser_parsedecl(BoaAstParser* prs)
{
    BoaAstExpression* expr = NULL;
    if(boa_astparser_match(prs, BOA_ASTTOKTYP_KWCLASS) || boa_astparser_match(prs, BOA_ASTTOKTYP_KWSTATIC))
    {
        expr = boa_astparser_parseclass(prs, false);
    }
    else
    {
        expr = boa_astparser_parsestatement(prs);
    }
    return expr;
}

bool boa_astparser_parsesource(BoaAstParser* prs, const char* filename, const char* source, BoaDynListExpr* statements)
{
    BoaAstCompiler stackcc;
    BoaAstExpression* expr;
    prs->haderror = false;
    prs->panicmode = false;
    boa_astlex_init(prs->pstate, prs->pstate->activelexer, filename, source);
    boa_astparser_compilerinit(prs, &stackcc);
    boa_astparser_advance(prs);
    boa_astparser_ignorelinefeeds(prs);
    while(!boa_astparser_isatend(prs))
    {
        if(boa_astparser_match(prs, BOA_ASTTOKTYP_SEMICOLON))
        {
            boa_astparser_ignorelinefeeds(prs);
            continue;
        }
        expr = boa_astparser_parsedecl(prs);
        if(expr != NULL)
        {
            boa_dynlistexpr_push(statements, expr);
        }
        boa_astparser_ignorelinefeeds(prs);
        boa_astparser_match(prs, BOA_ASTTOKTYP_SEMICOLON);
        boa_astparser_ignorelinefeeds(prs);
    }
    return prs->haderror || prs->pstate->activelexer->haderror;
}

BoaAstRule boa_astparser_makerule(BoaParsePrefixFn prefix, BoaParseInfixFn infix, BoaPrecedence precedence)
{
    BoaAstRule pr;
    pr.prefix = prefix;
    pr.infix = infix;
    pr.precedence = precedence;
    return pr;
}

void boa_astparser_setuprules()
{
    g_astparserules[BOA_ASTTOKTYP_LEFTPAREN] = boa_astparser_makerule(boa_astparser_rulegroupingorlambda, boa_astparser_ruleparsecall, BOA_ASTPREC_CALL);
    g_astparserules[BOA_ASTTOKTYP_PLUS] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_TERM);
    g_astparserules[BOA_ASTTOKTYP_MINUS] = boa_astparser_makerule(boa_astparser_ruleunary, boa_astparser_rulebinary, BOA_ASTPREC_TERM);
    g_astparserules[BOA_ASTTOKTYP_BANG] = boa_astparser_makerule(boa_astparser_ruleunary, boa_astparser_rulebinary, BOA_ASTPREC_IS);
    g_astparserules[BOA_ASTTOKTYP_STAR] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_FACTOR);
    g_astparserules[BOA_ASTTOKTYP_STARSTAR] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_FACTOR);
    g_astparserules[BOA_ASTTOKTYP_SLASH] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_FACTOR);
    g_astparserules[BOA_ASTTOKTYP_SHARP] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_FACTOR);
    g_astparserules[BOA_ASTTOKTYP_BAR] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_BOR);
    g_astparserules[BOA_ASTTOKTYP_AMPERSAND] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_BAND);
    g_astparserules[BOA_ASTTOKTYP_TILDE] = boa_astparser_makerule(boa_astparser_ruleunary, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_CARET] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_BXOR);
    g_astparserules[BOA_ASTTOKTYP_LESSLESS] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_SHIFT);
    g_astparserules[BOA_ASTTOKTYP_GREATERGREATER] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_SHIFT);
    g_astparserules[BOA_ASTTOKTYP_PERCENT] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_FACTOR);
    g_astparserules[BOA_ASTTOKTYP_KWIS] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_IS);
    g_astparserules[BOA_ASTTOKTYP_NUMBER] = boa_astparser_makerule(boa_astparser_rulenumber, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWTRUE] = boa_astparser_makerule(boa_astparser_ruleliteral, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWFALSE] = boa_astparser_makerule(boa_astparser_ruleliteral, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWNULL] = boa_astparser_makerule(boa_astparser_ruleliteral, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_BANGEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_EQUALITY);
    g_astparserules[BOA_ASTTOKTYP_EQUALEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_EQUALITY);
    g_astparserules[BOA_ASTTOKTYP_GREATERTHAN] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_COMPARISON);
    g_astparserules[BOA_ASTTOKTYP_GREATEREQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_COMPARISON);
    g_astparserules[BOA_ASTTOKTYP_LESSTHAN] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_COMPARISON);
    g_astparserules[BOA_ASTTOKTYP_LESSEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulebinary, BOA_ASTPREC_COMPARISON);
    g_astparserules[BOA_ASTTOKTYP_STRING] = boa_astparser_makerule(boa_astparser_rulestring, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_STRTEMPLATE] = boa_astparser_makerule(boa_astparser_ruleinterpolation, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_IDENTIFIER] = boa_astparser_makerule(boa_astparser_rulevarexpr, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWNEW] = boa_astparser_makerule(boa_astparser_rulenewexpr, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_PLUSEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_MINUSEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_STAREQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_SLASHEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_SHARPEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_PERCENTEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_CARETEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_BAREQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_AMPERSANDEQUAL] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_ASSIGNMENT);
    g_astparserules[BOA_ASTTOKTYP_PLUSPLUS] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_COMPOUND);
    g_astparserules[BOA_ASTTOKTYP_MINUSMINUS] = boa_astparser_makerule(NULL, boa_astparser_rulecompound, BOA_ASTPREC_COMPOUND);
    g_astparserules[BOA_ASTTOKTYP_AMPERSANDAMPERSAND] = boa_astparser_makerule(NULL, boa_astparser_rulelogicaland, BOA_ASTPREC_AND);
    g_astparserules[BOA_ASTTOKTYP_BARBAR] = boa_astparser_makerule(NULL, boa_astparser_rulelogicalor, BOA_ASTPREC_OR);
    g_astparserules[BOA_ASTTOKTYP_QUESTIONQUESTION] = boa_astparser_makerule(NULL, boa_astparser_rulenullfilter, BOA_ASTPREC_NULL);
    g_astparserules[BOA_ASTTOKTYP_DOT] = boa_astparser_makerule(NULL, boa_astparser_ruledot, BOA_ASTPREC_CALL);
#if 0
        g_astparserules[BOA_ASTTOKTYP_SMALLARROW] = boa_astparser_makerule(NULL, boa_astparser_ruledot, BOA_ASTPREC_CALL);
#endif
    g_astparserules[BOA_ASTTOKTYP_DOTDOT] = boa_astparser_makerule(NULL, boa_astparser_rulerange, BOA_ASTPREC_RANGE);
    g_astparserules[BOA_ASTTOKTYP_DOTDOTDOT] = boa_astparser_makerule(boa_astparser_rulevarexpr, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_LEFTBRACKET] = boa_astparser_makerule(boa_astparser_rulearray, boa_astparser_parsesubscript, BOA_ASTPREC_CALL);
    g_astparserules[BOA_ASTTOKTYP_LEFTBRACE] = boa_astparser_makerule(boa_astparser_rulebrace, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWCLASS] = boa_astparser_makerule(boa_astparser_ruleclass, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWIF] = boa_astparser_makerule(boa_astparser_ruleif, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWTHIS] = boa_astparser_makerule(boa_astparser_rulethis, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWSUPER] = boa_astparser_makerule(boa_astparser_rulesuper, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_QUESTION] = boa_astparser_makerule(NULL, boa_astparser_ruleternaryorquestion, BOA_ASTPREC_EQUALITY);
    g_astparserules[BOA_ASTTOKTYP_KWREF] = boa_astparser_makerule(boa_astparser_rulereference, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_REFSYM] = boa_astparser_makerule(boa_astparser_ruleunary, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_SEMICOLON] = boa_astparser_makerule(boa_astparser_rulenothing, NULL, BOA_ASTPREC_NONE);
    g_astparserules[BOA_ASTTOKTYP_KWFUNCTION] = boa_astparser_makerule(boa_astparser_rulefunction, NULL, BOA_ASTPREC_NONE);
}

const char* boa_astprint_tokopstring(int t)
{
    switch(t)
    {
        case BOA_ASTTOKTYP_LINEFEED: return "<linefeed>";
        case BOA_ASTTOKTYP_LEFTPAREN: return "(";
        case BOA_ASTTOKTYP_RIGHTPAREN: return ")";
        case BOA_ASTTOKTYP_LEFTBRACE: return "{";
        case BOA_ASTTOKTYP_RIGHTBRACE: return "}";
        case BOA_ASTTOKTYP_LEFTBRACKET: return "[";
        case BOA_ASTTOKTYP_RIGHTBRACKET: return "]";
        case BOA_ASTTOKTYP_COMMA: return ",";
        case BOA_ASTTOKTYP_SEMICOLON: return ";";
        case BOA_ASTTOKTYP_COLON: return ":";
        case BOA_ASTTOKTYP_BAREQUAL: return "|=";
        case BOA_ASTTOKTYP_BAR: return "|";
        case BOA_ASTTOKTYP_BARBAR: return "||";
        case BOA_ASTTOKTYP_AMPERSANDEQUAL: return "&=";
        case BOA_ASTTOKTYP_AMPERSAND: return "&";
        case BOA_ASTTOKTYP_AMPERSANDAMPERSAND: return "&&";
        case BOA_ASTTOKTYP_BANG: return "!";
        case BOA_ASTTOKTYP_BANGEQUAL: return "!=";
        case BOA_ASTTOKTYP_EQUAL: return "=";
        case BOA_ASTTOKTYP_EQUALEQUAL: return "==";
        case BOA_ASTTOKTYP_GREATERTHAN: return ">";
        case BOA_ASTTOKTYP_GREATEREQUAL: return ">=";
        case BOA_ASTTOKTYP_GREATERGREATER: return ">>";
        case BOA_ASTTOKTYP_LESSTHAN: return "<";
        case BOA_ASTTOKTYP_LESSEQUAL: return "<=";
        case BOA_ASTTOKTYP_LESSLESS: return "<<";
        case BOA_ASTTOKTYP_PLUS: return "+";
        case BOA_ASTTOKTYP_PLUSEQUAL: return "+=";
        case BOA_ASTTOKTYP_PLUSPLUS: return "++";
        case BOA_ASTTOKTYP_MINUS: return "-";
        case BOA_ASTTOKTYP_MINUSEQUAL: return "-=";
        case BOA_ASTTOKTYP_MINUSMINUS: return "--";
        case BOA_ASTTOKTYP_STAR: return "*";
        case BOA_ASTTOKTYP_STAREQUAL: return "*=";
        case BOA_ASTTOKTYP_STARSTAR: return "**";
        case BOA_ASTTOKTYP_SLASH: return "/";
        case BOA_ASTTOKTYP_SLASHEQUAL: return "/=";
        case BOA_ASTTOKTYP_QUESTION: return "?";
        case BOA_ASTTOKTYP_QUESTIONQUESTION: return "??";
        case BOA_ASTTOKTYP_PERCENT: return "%";
        case BOA_ASTTOKTYP_PERCENTEQUAL: return "%=";
        case BOA_ASTTOKTYP_ARROW: return "=>";
        case BOA_ASTTOKTYP_SMALLARROW: return "->";
        case BOA_ASTTOKTYP_TILDE: return "~";
        case BOA_ASTTOKTYP_CARET: return "^";
        case BOA_ASTTOKTYP_CARETEQUAL: return "^=";
        case BOA_ASTTOKTYP_DOT: return ".";
        case BOA_ASTTOKTYP_DOTDOT: return "..";
        case BOA_ASTTOKTYP_DOTDOTDOT: return "...";
        case BOA_ASTTOKTYP_SHARP: return "#";
        case BOA_ASTTOKTYP_SHARPEQUAL: return "#=";
        case BOA_ASTTOKTYP_KWCLASS: return "class";
        case BOA_ASTTOKTYP_KWELSE: return "else";
        case BOA_ASTTOKTYP_KWFALSE: return "false";
        case BOA_ASTTOKTYP_KWFOR: return "for";
        case BOA_ASTTOKTYP_KWFUNCTION: return "function";
        case BOA_ASTTOKTYP_KWIF: return "if";
        case BOA_ASTTOKTYP_KWNULL: return "null";
        case BOA_ASTTOKTYP_KWRETURN: return "return";
        case BOA_ASTTOKTYP_KWSUPER: return "super";
        case BOA_ASTTOKTYP_KWTHIS: return "this";
        case BOA_ASTTOKTYP_KWTRUE: return "true";
        case BOA_ASTTOKTYP_KWVAR: return "var";
        case BOA_ASTTOKTYP_KWWHILE: return "while";
        case BOA_ASTTOKTYP_KWCONTINUE: return "continue";
        case BOA_ASTTOKTYP_KWBREAK: return "break";
        case BOA_ASTTOKTYP_KWNEW: return "new";
        case BOA_ASTTOKTYP_KWEXPORT: return "export";
        case BOA_ASTTOKTYP_KWIS: return "is";
        case BOA_ASTTOKTYP_KWSTATIC: return "statis";
        case BOA_ASTTOKTYP_KWIN: return "in";
        case BOA_ASTTOKTYP_KWCONST: return "const";
        case BOA_ASTTOKTYP_KWSWITCH: return "switch";
        case BOA_ASTTOKTYP_KWCASE: return "case";
        case BOA_ASTTOKTYP_KWDEFAULT: return "default";
        default:
            break;
    }
    return "<unknown>";
}

void boa_astprint_init(BoaState* state, BoaAstPrinter* apr, BoaStream* printer, BoaAstCallback startfn)
{
    apr->startfunc = startfn;
    apr->pstate = state;
    apr->printer = printer;
    apr->indentlevel = 0;
    apr->fromcall = false;
}

void boa_astprint_warnv(BoaAstPrinter* apr, const char* fmt, va_list va)
{
    BoaStream* pr;
    (void)apr;
    pr = apr->pstate->streamstderr;
    boa_stream_setcolor(pr, 'r');
    boa_stream_printf(pr, "astprinter:warning: ");
    boa_stream_printfv(pr, fmt, va);
    boa_stream_printf(pr, "\n");
    boa_stream_resetcolor(pr);
}

void boa_astprint_warn(BoaAstPrinter* apr, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    boa_astprint_warnv(apr, fmt, va);
    va_end(va);
}

void boa_astprint_indentpush(BoaAstPrinter* apr)
{
    apr->indentlevel++;
}

void boa_astprint_indentpop(BoaAstPrinter* apr)
{
    apr->indentlevel--;
    if(((int)apr->indentlevel) <= 0)
    {
        apr->indentlevel = 0;
    }
}

void boa_astprint_indentprint(BoaAstPrinter* apr)
{
    size_t i;
    for(i=0; i<apr->indentlevel; i++)
    {
        boa_stream_puts(apr->printer, "    ");
    }
}

void boa_astprintdefault_printfuncparams(BoaAstPrinter* apr, BoaDynListParam* params)
{
    size_t i;
    BoaStream* pr;
    BoaAstFuncParamExpr* param;
    pr = apr->printer;
    {
        boa_stream_puts(pr, "(");
        for(i=0; i<params->listcount; i++)
        {
            param = &params->listitems[i];
            boa_stream_putlen(pr, param->name, param->length);
            if(param->defaultval != NULL)
            {
                boa_stream_puts(pr, "=");
                boa_astprintdefault_printexpression(apr, param->defaultval);
            }
            if((i+1) < params->listcount)
            {
                boa_stream_puts(pr, ", ");
            }
        }
        boa_stream_puts(pr, ")");
    }
}


void boa_astprinter_printblockorexpr(BoaAstPrinter* apr, BoaAstExpression* expr)
{
    bool isblock;
    BoaStream* pr;
    BoaAstBlockExpr* blockex;
    pr = apr->printer;
    isblock = (expr->type == BOA_ASTEXPRTYP_BLOCK);
    {
        boa_astprint_indentprint(apr);
        boa_stream_puts(pr, "{\n");
        boa_astprint_indentpush(apr);
        if(isblock)
        {
            blockex = (BoaAstBlockExpr*)expr;
            boa_astprintdefault_printexprlist(apr, &blockex->statements);
        }
        else
        {
            boa_astprintdefault_printexpression(apr, expr);
            #if 0
                boa_stream_puts(pr, ";\n");
            #endif
        }
        boa_astprint_indentpop(apr);
        boa_astprint_indentprint(apr);
        boa_stream_puts(pr, "}\n");
    }
}



void boa_astprintdefault_printexpression(BoaAstPrinter* apr, BoaAstExpression* expr)
{
    BoaStream* pr;
    pr = apr->printer;
    if(expr == NULL)
    {
        return;
    }
    switch(expr->type)
    {
        case BOA_ASTEXPRTYP_LITERAL:
            {
                BoaAstLiteralValExpr* oex;
                oex = (BoaAstLiteralValExpr*)expr;
                boa_value_printvalue(pr, oex->value, true);
            }
            break;
        case BOA_ASTEXPRTYP_BINARY:
            {
                BoaAstBinaryExpr* oex;
                oex = (BoaAstBinaryExpr*)expr;
                {
                    boa_stream_puts(pr, "(");
                    boa_astprintdefault_printexpression(apr, oex->left);
                    boa_stream_puts(pr, " ");
                    boa_stream_puts(pr, boa_astprint_tokopstring(oex->op));
                    boa_stream_puts(pr, " ");
                    boa_astprintdefault_printexpression(apr, oex->right);
                    boa_stream_puts(pr, ")");
                }
            }
            break;
        case BOA_ASTEXPRTYP_UNARY:
            {
                BoaAstUnaryExpr* oex;
                oex = (BoaAstUnaryExpr*)expr;
                {
                    boa_stream_puts(pr, "(");
                    boa_stream_puts(pr, boa_astprint_tokopstring(oex->op));
                    boa_astprintdefault_printexpression(apr, oex->right);
                    boa_stream_puts(pr, ")");
                }
            }
            break;
        case BOA_ASTEXPRTYP_VARGET:
            {
                BoaAstVarGetExpr* oex;
                oex = (BoaAstVarGetExpr*)expr;
                {
                    boa_stream_putlen(pr, oex->name, oex->length);
                }
            }
            break;
        case BOA_ASTEXPRTYP_ASSIGN:
            {
                BoaAstAssignExpr* oex;
                oex = (BoaAstAssignExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_astprintdefault_printexpression(apr, oex->to);
                boa_stream_puts(pr, " = ");
                boa_astprintdefault_printexpression(apr, oex->value);
            }
            break;
        case BOA_ASTEXPRTYP_CALL:
            {
                size_t i;
                size_t count;
                BoaAstCallExpr* oex;
                oex = (BoaAstCallExpr*)expr;
                count = oex->callargs.listcount;
                if(!apr->fromcall)
                {
                    boa_astprint_indentprint(apr);
                }
                {
                    apr->fromcall = true;
                    boa_astprintdefault_printexpression(apr, oex->excallee);
                    apr->fromcall = false;
                    boa_stream_puts(pr, "(");
                    for(i=0; i<count; i++)
                    {
                        apr->fromcall = true;
                        boa_astprintdefault_printexpression(apr, oex->callargs.listitems[i]);
                        apr->fromcall = false;
                        if((i+1) != count)
                        {
                            boa_stream_puts(pr, ", ");
                        }
                    }
                    boa_stream_puts(pr, ")");
                }
            }
            break;
        case BOA_ASTEXPRTYP_INDEXSET:
            {
                BoaAstIndexSetExpr* oex;
                oex = (BoaAstIndexSetExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_astprintdefault_printexpression(apr, oex->where);
                if(oex->isdot)
                {
                    boa_stream_putc(pr, '.');
                    boa_stream_putlen(pr, oex->name, oex->length);
                }
                else
                {
                    boa_stream_puts(pr, "[");
                    boa_stream_putc(pr, '"');
                    boa_stream_putlen(pr, oex->name, oex->length);
                    boa_stream_putc(pr, '"');
                    boa_stream_puts(pr, "]");
                }
                boa_stream_puts(pr, " = ");
                boa_astprintdefault_printexpression(apr, oex->value);
            }
            break;
        case BOA_ASTEXPRTYP_INDEXGET:
            {
                BoaAstIndexGetExpr* oex;
                oex = (BoaAstIndexGetExpr*)expr;
                boa_astprintdefault_printexpression(apr, oex->where);
                if(oex->isdot)
                {
                    boa_stream_putc(pr, '.');
                    boa_stream_putlen(pr, oex->name, oex->length);
                }
                else
                {
                    boa_stream_puts(pr, "[");
                    boa_stream_putc(pr, '"');
                    boa_stream_putlen(pr, oex->name, oex->length);
                    boa_stream_putc(pr, '"');
                    boa_stream_puts(pr, "]");
                }
                if(oex->ignoreresult)
                {
                    /*boa_stream_puts(pr, ";\n");*/
                }
            }
            break;
        case BOA_ASTEXPRTYP_SUBSCRIPT:
            {
                BoaAstSubscriptExpr* oex;
                oex = (BoaAstSubscriptExpr*)expr;
                {
                    boa_astprintdefault_printexpression(apr, oex->array);
                    boa_stream_puts(pr, "[");
                    boa_astprintdefault_printexpression(apr, oex->index);
                    boa_stream_puts(pr, "]");
                }
            }
            break;
        case BOA_ASTEXPRTYP_FUNCANON:
            {
                BoaAstFunctionExpr* oex;
                oex = (BoaAstFunctionExpr*)expr;
                boa_astprint_warn(apr, "anonymous functions are NOT supported in csi");
                {
                    boa_stream_puts(pr, "function");
                }
                boa_astprintdefault_printfuncparams(apr, &oex->parameters);
                boa_astprintdefault_printexpression(apr, oex->body);
            }
            break;
        case BOA_ASTEXPRTYP_ARRAY:
            {
                size_t i;
                size_t count;
                BoaAstLiteralArrayExpr* oex;
                oex = (BoaAstLiteralArrayExpr*)expr;
                count = oex->exvalues.listcount;
                {
                    boa_stream_puts(pr, "[");
                    for(i=0; i<count; i++)
                    {
                        boa_astprintdefault_printexpression(apr, oex->exvalues.listitems[i]);
                        if((i+1) < count)
                        {
                            boa_stream_puts(pr, ", ");
                        }
                    }
                    boa_stream_puts(pr, "]");
                }
            }
            break;
        case BOA_ASTEXPRTYP_OBJECT:
            {
                size_t i;
                size_t count;
                BoaAstLiteralObjectExpr* oex;
                oex = (BoaAstLiteralObjectExpr*)expr;
                count = oex->objexkeys.listcount;
                boa_stream_puts(pr, "{");
                for(i=0; i<count; i++)
                {
                    boa_value_printvalue(pr, oex->objexkeys.listitems[i], true);
                    boa_stream_puts(pr, ": ");
                    boa_astprintdefault_printexpression(apr, oex->objexvalues.listitems[i]);
                    if((i+1) != count)
                    {
                        boa_stream_puts(pr, ", ");
                    }
                }
                boa_stream_puts(pr, "}");
            }
            break;
        case BOA_ASTEXPRTYP_THIS:
            {
                BoaAstThisExpr* oex;
                (void)oex;
                oex = (BoaAstThisExpr*)expr;
                boa_stream_puts(pr, "this");
            }
            break;
        case BOA_ASTEXPRTYP_SUPER:
            {
                BoaAstSuperExpr* oex;
                oex = (BoaAstSuperExpr*)expr;
                boa_stream_puts(pr, "super");
                if(oex->methodname != NULL)
                {
                    boa_stream_puts(pr, ".");
                    boa_stream_putlen(pr, boa_string_getdata(oex->methodname), boa_string_getlength(oex->methodname));
                }
            }
            break;
        case BOA_ASTEXPRTYP_RANGE:
            {
                BoaAstRangeExpr* oex;
                oex = (BoaAstRangeExpr*)expr;
                boa_stream_puts(pr, "(");
                boa_astprintdefault_printexpression(apr, oex->from);
                boa_stream_puts(pr, " .. ");
                boa_astprintdefault_printexpression(apr, oex->to);
                boa_stream_puts(pr, ")");
            }
            break;
        case BOA_ASTEXPRTYP_TERNARY:
            {
                BoaAstTernaryExpr* oex;
                oex = (BoaAstTernaryExpr*)expr;
                boa_stream_puts(pr, "(");
                boa_astprintdefault_printexpression(apr, oex->condition);
                boa_stream_puts(pr, " ? ");
                boa_astprintdefault_printexpression(apr, oex->branchif);
                boa_stream_puts(pr, " : ");
                boa_astprintdefault_printexpression(apr, oex->branchelse);
                boa_stream_puts(pr, ")");
            }
            break;
        case BOA_ASTEXPRTYP_INTERPOLATION:
            {
                size_t i;
                size_t count;
                BoaAstStrTemplateExpr* oex;
                oex = (BoaAstStrTemplateExpr*)expr;
                count = oex->expressions.listcount;
                boa_stream_puts(pr, "(");
                boa_stream_putc(pr, '"');
                boa_stream_putc(pr, '"');
                boa_stream_putc(pr, '+');
                for(i=0; i<count; i++)
                {
                    boa_astprintdefault_printexpression(apr, oex->expressions.listitems[i]);
                    if((i+1) != count)
                    {
                        boa_stream_puts(pr, " + ");
                    }
                }
                boa_stream_puts(pr, ")");
            }
            break;
        case BOA_ASTEXPRTYP_REFERENCE:
            {
                BoaAstRefExpr* oex;
                oex = (BoaAstRefExpr*)expr;
                boa_stream_puts(pr, "ref ");
                boa_astprintdefault_printexpression(apr, oex->to);
                boa_stream_puts(pr, "");
            }
            break;
        case BOA_ASTEXPRTYP_EXPRESSION:
            {
                BoaAstExprStmtExpr* oex;
                oex = (BoaAstExprStmtExpr*)expr;
                boa_astprintdefault_printexpression(apr, oex->exvalue);
                boa_stream_puts(pr, ";\n");
            }
            break;
        case BOA_ASTEXPRTYP_BLOCK:
            {
                boa_astprinter_printblockorexpr(apr, expr);
            }
            break;
        case BOA_ASTEXPRTYP_IF:
            {
                size_t i;
                size_t count;
                BoaAstIfExpr* oex;
                oex = (BoaAstIfExpr*)expr;
                {
                    boa_astprint_indentprint(apr);
                    boa_stream_puts(pr, "if(");
                    boa_astprintdefault_printexpression(apr, oex->condition);
                    boa_stream_puts(pr, ")\n");
                    boa_astprinter_printblockorexpr(apr, oex->branchif);
                    if(oex->elseifcondlist != NULL)
                    {
                        count = oex->elseifcondlist->listcount;
                        for(i=0; i<count; i++)
                        {
                            boa_astprint_indentprint(apr);
                            boa_stream_puts(pr, "else if(");
                            boa_astprintdefault_printexpression(apr, oex->elseifcondlist->listitems[i]);
                            boa_stream_puts(pr, ")\n");
                            boa_astprinter_printblockorexpr(apr, oex->branchelseiflist->listitems[i]);
                        }
                    }
                    if(oex->branchelse != NULL)
                    {
                        boa_astprint_indentprint(apr);
                        boa_stream_puts(pr, "else\n");
                        boa_astprinter_printblockorexpr(apr, oex->branchelse);
                    }
                }
            }
            break;
        case BOA_ASTEXPRTYP_SWITCH:
            {
                size_t i;
                size_t count;
                BoaAstSwitchExpr* oex = (BoaAstSwitchExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "switch(");
                boa_astprintdefault_printexpression(apr, oex->condition);
                boa_stream_puts(pr, ") {\n");
                boa_astprint_indentpush(apr);
                count = oex->caseconditions.listcount;
                for(i = 0; i < count; i++)
                {
                    BoaAstExpression* cond = oex->caseconditions.listitems[i];
                    boa_astprint_indentprint(apr);
                    if(cond == NULL)
                    {
                        boa_stream_puts(pr, "default:\n");
                    }
                    else
                    {
                        boa_stream_puts(pr, "case ");
                        boa_astprintdefault_printexpression(apr, cond);
                        boa_stream_puts(pr, ":\n");
                    }
                    boa_astprint_indentpush(apr);
                    boa_astprintdefault_printexpression(apr, oex->casebodies.listitems[i]);
                    boa_astprint_indentpop(apr);
                }
                boa_astprint_indentpop(apr);
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "}\n");
            }
            break;
        case BOA_ASTEXPRTYP_WHILE:
            {
                BoaAstWhileExpr* oex;
                oex = (BoaAstWhileExpr*)expr;
                boa_astprint_indentprint(apr);
                {
                    boa_stream_puts(pr, "while(");
                    boa_astprintdefault_printexpression(apr, oex->condition);
                    boa_stream_puts(pr, ")\n");
                    boa_astprintdefault_printexpression(apr, oex->body);
                }
            }
            break;
        case BOA_ASTEXPRTYP_FOR:
            {
                BoaAstForExpr* oex;
                oex = (BoaAstForExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "for(");
                if(oex->iscstyle)
                {
                    
                }
                else
                {
                    boa_astprintdefault_printexpression(apr, oex->var);
                    boa_stream_puts(pr, " in ");
                    boa_astprintdefault_printexpression(apr, oex->condition);
                }
                boa_stream_puts(pr, ")\n");
                boa_astprintdefault_printexpression(apr, oex->body);
            }
            break;
        case BOA_ASTEXPRTYP_TRY:
            {
                BoaAstTryExpr* oex;
                oex = (BoaAstTryExpr*)expr;
                boa_stream_puts(pr, "try\n");
                boa_astprintdefault_printexpression(apr, oex->tryblock);
                if(oex->catchblock != NULL)
                {
                    boa_stream_puts(pr, "catch");
                    if(oex->catchvarstr != NULL)
                    {
                        boa_stream_printf(pr, "(%.*s)", (int)oex->catchvarlen, oex->catchvarstr);
                    }
                    boa_stream_puts(pr, "\n");
                    boa_astprintdefault_printexpression(apr, oex->catchblock);
                }
                if(oex->finallyblock != NULL)
                {
                    boa_stream_puts(pr, "finally\n");
                    boa_astprintdefault_printexpression(apr, oex->finallyblock);
                }
            }
            break;
        case BOA_ASTEXPRTYP_THROW:
            {
                BoaAstThrowExpr* oex;
                oex = (BoaAstThrowExpr*)expr;
                boa_stream_puts(pr, "throw ");
                boa_astprintdefault_printexpression(apr, oex->exvalue);
                boa_stream_puts(pr, ";\n");
            }
            break;
        case BOA_ASTEXPRTYP_VARDECL:
            {
                BoaAstVarDeclExpr* oex;
                oex = (BoaAstVarDeclExpr*)expr;
                boa_astprint_indentprint(apr);
                {
                    if(oex->isconstant)
                    {
                        boa_stream_puts(pr, "const ");
                    }
                    else
                    {
                        boa_stream_puts(pr, "var ");
                    }
                    boa_stream_putlen(pr, oex->name, oex->length);
                    if(oex->init != NULL)
                    {
                        boa_stream_puts(pr, " = ");
                        boa_astprintdefault_printexpression(apr, oex->init);
                    }
                    boa_stream_puts(pr, ";\n");
                }
            }
            break;
        case BOA_ASTEXPRTYP_CONTINUE:
            {
                BoaAstContinueExpr* oex;
                (void)oex;
                oex = (BoaAstContinueExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "continue;\n");
            }
            break;
        case BOA_ASTEXPRTYP_BREAK:
            {
                BoaBreakStatement* oex;
                (void)oex;
                oex = (BoaBreakStatement*)expr;
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "break;\n");
            }
            break;
        case BOA_ASTEXPRTYP_FUNCTION:
            {
                BoaAstFunctionExpr* oex;
                oex = (BoaAstFunctionExpr*)expr;
                boa_astprint_indentprint(apr);
                {
                    if(oex->exported)
                    {
                        boa_stream_puts(pr, "export ");
                    }
                    boa_stream_puts(pr, "function");
                    if(oex->name != NULL)
                    {
                        boa_stream_puts(pr, " ");
                        boa_stream_putlen(pr, oex->name, oex->length);
                    }
                    boa_astprintdefault_printfuncparams(apr, &oex->parameters);
                    boa_stream_puts(pr, "\n");
                    boa_astprintdefault_printexpression(apr, oex->body);
                }
            }
            break;
        case BOA_ASTEXPRTYP_RETURN:
            {
                BoaAstReturnExpr* oex;
                oex = (BoaAstReturnExpr*)expr;
                boa_astprint_indentprint(apr);
                boa_stream_puts(pr, "return");
                if(oex->exvalue != NULL)
                {
                    boa_stream_puts(pr, " ");
                    boa_astprintdefault_printexpression(apr, oex->exvalue);
                }
                boa_stream_puts(pr, ";\n");
            }
            break;
        case BOA_ASTEXPRTYP_METHOD:
            {
                bool notoper;
                BoaAstMethodExpr* oex;
                {
                    oex = (BoaAstMethodExpr*)expr;
                    notoper = boa_util_charisalpha(boa_string_getat(oex->name, 0));
                    boa_astprint_indentprint(apr);
                    if(!notoper)
                    {
                        boa_stream_puts(pr, "operator ");
                    }
                    boa_stream_putlen(pr, boa_string_getdata(oex->name), boa_string_getlength(oex->name));
                    boa_astprintdefault_printfuncparams(apr, &oex->parameters);
                    boa_stream_puts(pr, "\n");
                    boa_astprintdefault_printexpression(apr, oex->body);
                }
            }
            break;
        case BOA_ASTEXPRTYP_CLASS:
            {
                size_t i;
                size_t count;
                BoaAstClassExpr* oex;
                {
                    oex = (BoaAstClassExpr*)expr;
                    count = oex->staticfields.listcount;
                    boa_astprint_indentprint(apr);
                    boa_stream_puts(pr, "class ");
                    boa_stream_putlen(pr, boa_string_getdata(oex->name), boa_string_getlength(oex->name));
                    if(oex->parent != NULL)
                    {
                        boa_stream_puts(pr, ": ");
                        boa_stream_putlen(pr, boa_string_getdata(oex->parent), boa_string_getlength(oex->parent));
                    }
                    boa_stream_puts(pr, "\n");
                    boa_astprint_indentprint(apr);
                    boa_stream_puts(pr, "{\n");
                    boa_astprint_indentpush(apr);
                    for(i=0; i<count; i++)
                    {
                        boa_astprintdefault_printexpression(apr, oex->staticfields.listitems[i]);
                    }
                    boa_astprint_indentpop(apr);
                    boa_stream_puts(pr, "\n");
                    boa_astprint_indentprint(apr);
                    boa_stream_puts(pr, "}\n");
                }
            }
            break;
        case BOA_ASTEXPRTYP_FIELD:
            {
                BoaAstFieldExpr* oex;
                oex = (BoaAstFieldExpr*)expr;
                boa_stream_puts(pr, "<FIELD>");
                boa_stream_putlen(pr, boa_string_getdata(oex->name), boa_string_getlength(oex->name));
                boa_stream_puts(pr, ";\n");
            }
            break;
    }
}

void boa_astprintdefault_printexprlist(BoaAstPrinter* apr, BoaDynListExpr* elist)
{
    size_t i;
    BoaAstExpression* expr;
    for(i=0; i<elist->listcount; i++)
    {
        expr = elist->listitems[i];
        boa_astprintdefault_printexpression(apr, expr);
    }
}

void boa_astprintdefault_start(BoaAstPrinter* apr, BoaDynListExpr* elist)
{
    boa_astprintdefault_printexprlist(apr, elist);
}

void boa_astprintdefault_printbeginlist(BoaState* state, FILE* ofh, BoaDynListExpr* statements)
{
    bool printtrailing;
    BoaAstPrinter apr;
    BoaAstCallback startfn;
    startfn = boa_astprintdefault_start;
    printtrailing = true;
    if(state->config.quitafterdump)
    {
       printtrailing = false;
    }
    boa_astprint_init(state, &apr, boa_stream_makeio(ofh, false), startfn);
    if(printtrailing)
    {
        boa_stream_puts(apr.printer, "<<<astdump begin>>>\n");
    }
    apr.startfunc(&apr, statements);
    if(printtrailing)
    {
        boa_stream_puts(apr.printer, "\n<<<astdump end>>>\n");
    }
    boa_stream_destroy(apr.printer);
}

void boa_emitter_resolvestmtlist(BoaAstEmitter* emt, BoaDynListExpr* statements)
{
    size_t i;
    for(i = 0; i < statements->listcount; i++)
    {
        boa_emitter_resolvestatement(emt, statements->listitems[i]);
    }
}

void boa_emitter_init(BoaState* state, BoaAstEmitter* emt)
{
    boa_emitter_reset(state, emt);
    boa_dynlistpriv_init(&emt->privlist);
    boa_dynlistuint_init(&emt->breaks);
    boa_dynlistuint_init(&emt->continues);
}

void boa_emitter_reset(BoaState* state, BoaAstEmitter* emt)
{
    emt->pstate = state;
    emt->loopstart = 0;
    emt->emitreference = 0;
    emt->classname = NULL;
    emt->compiler = NULL;
    emt->chunk = NULL;
    emt->module = NULL;
    emt->classhassuper = false;
}

void boa_emitter_destroy(BoaAstEmitter* emt)
{
    boa_dynlistuint_destroy(&emt->breaks);
    boa_dynlistuint_destroy(&emt->continues);
}

void boa_emitter_raiseerror(BoaAstEmitter* emt, size_t line, const char* fmt, ...)
{
    BoaString* cs;
    va_list args;
    va_start(args, fmt);
    cs = boa_state_errorfmtv(emt->pstate, line, fmt, args);
    boa_state_raiseerror(emt->pstate, boa_string_getdata(cs));
    va_end(args);
}

void boa_emitter_raisewarning(BoaAstEmitter* emt, size_t line, const char* fmt, ...)
{
    BoaString* cs;
    va_list args;
    va_start(args, fmt);
    cs = boa_state_errorfmtv(emt->pstate, line, fmt, args);
    boa_state_raisewarning(emt->pstate, boa_string_getdata(cs));
    va_end(args);
}

size_t boa_emitter_emittmp(BoaAstEmitter* emt)
{
    boa_chunk_push(emt->chunk, 0, emt->lastline);
    return emt->chunk->compiledcodecount - 1;
}

void boa_emitter_patchinstr(BoaAstEmitter* emt, uint64_t position, uint64_t instruction)
{
    emt->chunk->compiledcodechunk[position] = instruction;
}

void boa_emitter_emitabc(BoaAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, uint16_t b, uint16_t c)
{
    emt->lastline = fmax(line, emt->lastline);
    boa_chunk_push(emt->chunk, BOA_REG_FORMABCINST(opcode, a, b, c), emt->lastline);
}

void boa_emitter_emitabx(BoaAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, uint32_t bx)
{
    emt->lastline = fmax(line, emt->lastline);
    boa_chunk_push(emt->chunk, BOA_REG_FORMABXINST(opcode, a, bx), emt->lastline);
}

void boa_emitter_emitasbx(BoaAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, int32_t sbx)
{
    emt->lastline = fmax(line, emt->lastline);
    boa_chunk_push(emt->chunk, BOA_REG_FORMASBXINST(opcode, a, sbx), emt->lastline);
}

/*
 * be very careful with the use of this function:
 * always reserve a register just before using it, do not wait around!
 */
uint64_t boa_emitter_reserveregister(BoaAstEmitter* emt)
{
    BoaAstCompiler* ccx;
    ccx = emt->compiler;
    if(ccx->registersused == BOA_CONFIG_REGISTERSMAX)
    {
        boa_emitter_raiseerror(emt, emt->lastline, "too many registers required");
        return 0;
    }
    if(ccx->function->maxregisters > (ccx->registersused+1))
    {
        ccx->function->maxregisters = (ccx->function->maxregisters);
    }
    else
    {
        ccx->function->maxregisters = (ccx->registersused+1);
    }
    ++ccx->registersused;
    return ccx->registersused - 1;
}

void boa_emitter_freeregister(BoaAstEmitter* emt, uint64_t reg)
{
    BoaAstCompiler* ccx;
    if(BOA_BIT_ISSET(reg, BOA_BITFLAG_REGISTER))
    {
        return;
    }
    ccx = emt->compiler;
    if(ccx->registersused == 0)
    {
        boa_emitter_raiseerror(emt, emt->lastline, "invalid register was freed");
    }
    else
    {
        ccx->registersused--;
    }
}

BoaAstLocal boa_emitter_makelocal(const char* name, size_t length, int depth, bool captured, bool constant, uint8_t reg)
{
    BoaAstLocal rt;
    rt.name = name;
    rt.length = length;
    rt.depth = depth;
    rt.captured = captured;
    rt.constant = constant;
    rt.reg = reg;
    return rt;
}

void boa_emitter_compilerinit(BoaAstEmitter* emt, BoaAstCompiler* ccx, BoaFuncType type)
{
    const char* name;
    boa_dynlistloc_init(&ccx->locals);
    ccx->type = type;
    ccx->scopedepth = -1;
    ccx->enclosing = (struct BoaAstCompiler*)emt->compiler;
    ccx->skipreturn = false;
    ccx->function = boa_object_makefunction(emt->pstate, emt->module);
    ccx->loopdepth = 0;
    ccx->switchdepth = 0;
    ccx->registersused = 0;
    emt->compiler = ccx;
    name = emt->pstate->activelexer->sourcefilename;
    if(emt->compiler == NULL)
    {
        ccx->function->name = boa_string_copylen(emt->pstate, name, strlen(name));
    }
    emt->chunk = &ccx->function->chunk;
    if(type == BOA_FUNCTYPE_METHOD || type == BOA_FUNCTYPE_STATIC_METHOD || type == BOA_FUNCTYPE_CONSTRUCTOR)
    {
        boa_dynlistloc_push(&ccx->locals, boa_emitter_makelocal("this", 4, -1, false, false, boa_emitter_reserveregister(emt)));
    }
    else
    {
        boa_dynlistloc_push(&ccx->locals, boa_emitter_makelocal("", 0, -1, false, false, boa_emitter_reserveregister(emt)));
    }
}

BoaFuncScript* boa_emitter_compilerend(BoaAstEmitter* emt, BoaString* name)
{
    uint8_t reg;
    BoaFuncType type;
    BoaFuncScript* function;

    boa_emitter_freeregister(emt, 0);
    if(emt->compiler->registersused > 0)
    {
        boa_emitter_raiseerror(emt, emt->lastline, "not all registers were freed (%i left)", emt->compiler->registersused);
    }
    if(!emt->compiler->skipreturn)
    {
        reg = boa_emitter_reserveregister(emt);
        type = emt->compiler->type;
        if(type == BOA_FUNCTYPE_CONSTRUCTOR)
        {
            /* load <this> */
            boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, reg, 0);
        }
        else
        {
            boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_LOADNULL, reg, 0, 0);
        }
        boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_RETURN, reg, 1, 0);
        
        boa_emitter_freeregister(emt, reg);
        emt->compiler->skipreturn = true;
    }
    function = emt->compiler->function;
    boa_dynlistloc_destroy(&emt->compiler->locals);
    emt->compiler = (BoaAstCompiler*)emt->compiler->enclosing;
    emt->chunk = emt->compiler == NULL ? NULL : &emt->compiler->function->chunk;
    if(name != NULL)
    {
        function->name = name;
    }
#if defined(BOA_CONFIG_DEBUGTRACECHUNK) && (BOA_CONFIG_DEBUGTRACECHUNK == 1)
    if(!emt->pstate->haderror)
    {
        boa_debug_disaschunk(&function->chunk, boa_string_getdata(function->name), NULL);
    }
#endif
    return function;
}

void boa_emitter_scopebegin(BoaAstEmitter* emt)
{
    emt->compiler->scopedepth++;
}

void boa_emitter_scopeend(BoaAstEmitter* emt)
{
    BoaAstCompiler* ccx;
    BoaDynListLoc* locals;
    BoaAstLocal* local;

    if(emt->compiler->scopedepth == -1)
    {
        boa_emitter_raiseerror(emt, emt->lastline, "invalid scope ending");
    }
    emt->compiler->scopedepth--;
    ccx = emt->compiler;
    locals = &ccx->locals;
    /* locals that are still being initialized (depth is UINT16_MAX) belong to an
       enclosing declaration, so they must survive the ending of a nested scope */
    while(locals->listcount > 0 && locals->listitems[locals->listcount - 1].depth > ccx->scopedepth && locals->listitems[locals->listcount - 1].depth != UINT16_MAX)
    {
        local = &locals->listitems[locals->listcount - 1];
        if(local->captured)
        {
            boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_UPVALUECLOSE, local->reg, 0, 0);
        }
        boa_emitter_freeregister(emt, local->reg);
        locals->listcount--;
    }
}

uint16_t boa_emitter_addconst(BoaAstEmitter* emt, size_t line, BoaValue val)
{
    size_t constant = boa_chunk_addconstant(emt->pstate, emt->chunk, val);
    if(constant >= UINT16_MAX)
    {
        boa_emitter_raiseerror(emt, line, "too many constants for one chunk");
    }
    return constant;
}

int boa_emitter_addprivate(BoaAstEmitter* emt, const char* name, size_t length, size_t line, bool constant)
{
    int index;
    BoaValue vidx;
    BoaAstPrivate priv;
    BoaState* state;
    BoaString* key;
    BoaTable* privnames;
    BoaDynListPriv* privates;
    privates = &emt->privlist;
    if(privates->listcount == UINT16_MAX)
    {
        boa_emitter_raiseerror(emt, line, "too many private locals for one module");
    }
    privnames = &emt->module->privatenames->innertable;
    key = boa_table_findstring(privnames, name, length, boa_string_hash(name, length));
    if(key != NULL)
    {
        boa_emitter_raisewarning(emt, line, "variable <%.*s> was already declared in this scope", length, name);
        if(boa_table_getentry(privnames, key, &vidx))
        {
            return boa_value_asnumber(vidx);
        }
    }
    state = emt->pstate;
    index = (int)privates->listcount;
    priv.initialized = false;
    priv.constant = constant;
    boa_dynlistpriv_push(privates, priv);
    boa_table_set(privnames, boa_string_copylen(state, name, length), boa_value_makenumber(index));
    emt->module->privatecount++;
    return index;
}

int boa_emitter_resolveprivate(BoaAstEmitter* emt, const char* name, size_t length, size_t line)
{
    int numberindex;
    BoaValue index;
    BoaString* key;
    BoaTable* privnames;
    privnames = &emt->module->privatenames->innertable;
    key = boa_table_findstring(privnames, name, length, boa_string_hash(name, length));
    if(key != NULL)
    {
        if(boa_table_getentry(privnames, key, &index))
        {
            numberindex = boa_value_asnumber(index);
            if(!emt->privlist.listitems[numberindex].initialized)
            {
                boa_emitter_raiseerror(emt, line, "variable <%.*s> cannot use itself in its initializer", length, name);
            }
            return numberindex;
        }
    }
    return -1;
}

int boa_emitter_addlocal(BoaAstEmitter* emt, const char* name, size_t length, size_t line, bool constant, uint8_t reg)
{
    int i;
    BoaAstCompiler* ccx = emt->compiler;
    BoaDynListLoc* locals = &ccx->locals;
    if(locals->listcount == UINT16_MAX)
    {
        boa_emitter_raiseerror(emt, line, "too many local variables for one function");
    }
    for(i = (int)locals->listcount - 1; i >= 0; i--)
    {
        BoaAstLocal* local = &locals->listitems[i];
        if(local->depth != UINT16_MAX && local->depth < ccx->scopedepth)
        {
            break;
        }
        if(length == local->length && memcmp(local->name, name, length) == 0)
        {
            boa_emitter_raisewarning(emt, line, "variable <%.*s> was already declared in this scope", length, name);
        }
    }
    boa_dynlistloc_push(locals, boa_emitter_makelocal(name, length, UINT16_MAX, false, constant, reg));
    return (int)locals->listcount - 1;
}

int boa_emitter_resolvelocal(BoaAstEmitter* emt, BoaAstCompiler* ccx, const char* name, size_t length, size_t line)
{
    int i;
    BoaDynListLoc* locals = &ccx->locals;
    for(i = (int)locals->listcount - 1; i >= 0; i--)
    {
        BoaAstLocal* local = &locals->listitems[i];
        if(local->length == length && memcmp(local->name, name, length) == 0)
        {
            if(local->depth == UINT16_MAX)
            {
                boa_emitter_raiseerror(emt, line, "variable <%.*s> cannot use itself in its initializer", length, name);
            }
            return i;
        }
    }
    return -1;
}

int boa_emitter_addupvalue(BoaAstEmitter* emt, BoaAstCompiler* ccx, size_t index, size_t line, bool islocal)
{
    size_t i;
    size_t upvaluecount = ccx->function->upvaluecount;
    for(i = 0; i < upvaluecount; i++)
    {
        BoaAstUpvalue* upvalue = &ccx->compiledupvalueitems[i];
        if(upvalue->index == index && upvalue->islocal == islocal)
        {
            return i;
        }
    }
    if(upvaluecount == BOA_CONFIG_UINT16COUNT)
    {
        boa_emitter_raiseerror(emt, line, "too many upvalues for one function");
        return 0;
    }
    ccx->compiledupvalueitems[upvaluecount].islocal = islocal;
    ccx->compiledupvalueitems[upvaluecount].index = index;
    return ccx->function->upvaluecount++;
}

int boa_emitter_resolveupvalue(BoaAstEmitter* emt, BoaAstCompiler* ccx, const char* name, size_t length, size_t line)
{
    int local;
    int upvalue;

    if(ccx->enclosing == NULL)
    {
        return -1;
    }
    local = boa_emitter_resolvelocal(emt, (BoaAstCompiler*)ccx->enclosing, name, length, line);
    if(local != -1)
    {
        /* the index must be the register, because the VM reads it straight
           out of the register window of the enclosing frame */
        ((BoaAstCompiler*)ccx->enclosing)->locals.listitems[local].captured = true;
        local = (int)((BoaAstCompiler*)ccx->enclosing)->locals.listitems[local].reg;
        return boa_emitter_addupvalue(emt, ccx, local, line, true);
    }
    upvalue = boa_emitter_resolveupvalue(emt, (BoaAstCompiler*)ccx->enclosing, name, length, line);
    if(upvalue != -1)
    {
        return boa_emitter_addupvalue(emt, ccx, upvalue, line, false);
    }
    return -1;
}

void boa_emitter_marklocalinit(BoaAstEmitter* emt, size_t index)
{
    emt->compiler->locals.listitems[index].depth = emt->compiler->scopedepth;
}

void boa_emitter_markprivateinit(BoaAstEmitter* emt, size_t index)
{
    emt->privlist.listitems[index].initialized = true;
}

void boa_emitter_resolvestatement(BoaAstEmitter* emt, BoaAstExpression* topexpr)
{
    BoaAstVarDeclExpr* vexpr;
    BoaAstFunctionExpr* fexpr;

    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case BOA_ASTEXPRTYP_VARDECL:
        {
            vexpr = (BoaAstVarDeclExpr*)topexpr;
            boa_emitter_markprivateinit(emt, boa_emitter_addprivate(emt, vexpr->name, vexpr->length, topexpr->line, vexpr->isconstant));
            break;
        }
        case BOA_ASTEXPRTYP_FUNCTION:
        {
            fexpr = (BoaAstFunctionExpr*)topexpr;
            if(!fexpr->exported)
            {
                boa_emitter_markprivateinit(emt, boa_emitter_addprivate(emt, fexpr->name, fexpr->length, topexpr->line, false));
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

BoaOpCode boa_emitter_translateunaryop(BoaAstTokType token)
{
    switch(token)
    {
        case BOA_ASTTOKTYP_MINUS:
            return BOA_OPCODE_NEGATE;
        case BOA_ASTTOKTYP_BANG:
            return BOA_OPCODE_NOT;
        case BOA_ASTTOKTYP_TILDE:
            return BOA_OPCODE_BINNOT;
        default:
            BOA_UTIL_UNREACHABLE();
    }
    return BOA_OPCODE_RETURN;
}

BoaOpCode boa_emitter_translatebinaryop(BoaAstTokType token)
{
    switch(token)
    {
        case BOA_ASTTOKTYP_BANGEQUAL:
        case BOA_ASTTOKTYP_EQUALEQUAL:
            return BOA_OPCODE_EQUAL;
        case BOA_ASTTOKTYP_LESSTHAN:
            return BOA_OPCODE_LESSTHAN;
        case BOA_ASTTOKTYP_LESSEQUAL:
            return BOA_OPCODE_LESSEQUAL;
        case BOA_ASTTOKTYP_GREATERTHAN:
            return BOA_OPCODE_GREATERTHAN;
        case BOA_ASTTOKTYP_GREATEREQUAL:
            return BOA_OPCODE_GREATEREQUAL;
        case BOA_ASTTOKTYP_PLUS:
            return BOA_OPCODE_MATHADD;
        case BOA_ASTTOKTYP_MINUS:
            return BOA_OPCODE_MATHSUBTRACT;
        case BOA_ASTTOKTYP_STAR:
            return BOA_OPCODE_MATHMULTIPLY;
        case BOA_ASTTOKTYP_STARSTAR:
            return BOA_OPCODE_MATHPOWER;
        case BOA_ASTTOKTYP_SLASH:
            return BOA_OPCODE_MATHDIVIDE;
        case BOA_ASTTOKTYP_SHARP:
            return BOA_OPCODE_MATHFLOORDIVIDE;
        case BOA_ASTTOKTYP_PERCENT:
            return BOA_OPCODE_MATHMOD;
        case BOA_ASTTOKTYP_LESSLESS:
            return BOA_OPCODE_MATHLEFTSHIFT;
        case BOA_ASTTOKTYP_GREATERGREATER:
            return BOA_OPCODE_MATHRIGHTSHIFT;
        case BOA_ASTTOKTYP_CARET:
            return BOA_OPCODE_BINXOR;
        case BOA_ASTTOKTYP_AMPERSAND:
            return BOA_OPCODE_BINAND;
        case BOA_ASTTOKTYP_BAR:
            return BOA_OPCODE_BINOR;
        case BOA_ASTTOKTYP_KWIS:
            return BOA_OPCODE_IS;
        default:
            BOA_UTIL_UNREACHABLE();
    }
    return BOA_OPCODE_RETURN;
}

uint16_t boa_emitter_parsearg(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint8_t reg)
{
    BoaValue value;
    BoaAstVarGetExpr* expr;
    uint64_t arg;
    int index;

    if(topexpr->type == BOA_ASTEXPRTYP_LITERAL)
    {
        value = ((BoaAstLiteralValExpr*)topexpr)->value;
        if(boa_value_isnumber(value) || boa_value_isstring(value))
        {
            arg = boa_emitter_addconst(emt, topexpr->line, value);
            /* Mark that this is a constant */
            BOA_BIT_SETBIT(arg, BOA_BITFLAG_CONSTANT);
            return arg;
        }
    }
    else if(topexpr->type == BOA_ASTEXPRTYP_VARGET)
    {
        expr = ((BoaAstVarGetExpr*)topexpr);
        index = boa_emitter_resolvelocal(emt, emt->compiler, expr->name, expr->length, topexpr->line);
        if(index != -1)
        {
            return emt->compiler->locals.listitems[index].reg;
        }
    }
    boa_emitter_emitexpr(emt, topexpr, reg);
    return reg;
}

void boa_emitter_emitbinaryexpr(BoaAstEmitter* emt, BoaAstBinaryExpr* expr, uint8_t reg, bool swap)
{
    size_t jump;
    uint16_t b;
    uint16_t rc;
    uint16_t c;
    int constant;
    BoaOpCode opcode;
    BoaAstVarGetExpr* e;
    BoaAstTokType op;

    op = expr->op;
    if(op == BOA_ASTTOKTYP_AMPERSANDAMPERSAND || op == BOA_ASTTOKTYP_BARBAR || op == BOA_ASTTOKTYP_QUESTIONQUESTION)
    {
        boa_emitter_emitexpr(emt, expr->left, reg);
        jump = boa_emitter_emittmp(emt);
        boa_emitter_emitexpr(emt, expr->right, reg);
        boa_emitter_patchinstr(emt, jump, BOA_REG_FORMABXINST(op == BOA_ASTTOKTYP_BARBAR ? BOA_OPCODE_JUMPIFTRUE : (op == BOA_ASTTOKTYP_QUESTIONQUESTION ? BOA_OPCODE_JUMPIFNONNULL : BOA_OPCODE_JUMPIFFALSE), reg, emt->chunk->compiledcodecount - jump - 1));
    }
    else
    {
        b = boa_emitter_parsearg(emt, expr->left, reg);
        opcode = boa_emitter_translatebinaryop(op);
        if(opcode == BOA_OPCODE_IS)
        {
            if(expr->right->type != BOA_ASTEXPRTYP_VARGET)
            {
                boa_emitter_raiseerror(emt, ((BoaAstExpression*)expr)->line, "<is> operator is not used with a var expression");
                return;
            }
            e = (BoaAstVarGetExpr*)expr->right;
            constant = boa_emitter_addconst(emt, ((BoaAstExpression*)expr)->line, boa_value_fromobject(boa_string_copylen(emt->pstate, e->name, e->length)));
            boa_emitter_emitabc(emt, ((BoaAstExpression*)expr)->line, opcode, reg, b, constant);
        }
        else
        {
            rc = boa_emitter_reserveregister(emt);
            c = boa_emitter_parsearg(emt, expr->right, rc);
            boa_emitter_emitabc(emt, ((BoaAstExpression*)expr)->line, opcode, reg, swap ? c : b, swap ? b : c);
            boa_emitter_freeregister(emt, rc);
        }
    }
}

bool boa_emitter_emitparams(BoaAstEmitter* emt, BoaDynListParam* parameters, size_t line)
{
    size_t i;
    size_t jump;
    size_t dotlen;
    uint8_t reg;
    int index;
    const char* dotstr;
    BoaAstFuncParamExpr* parameter;
    dotstr = boa_string_getdata(emt->pstate->strings.strdots);
    dotlen = boa_string_getlength(emt->pstate->strings.strdots);
    for(i = 0; i < parameters->listcount; i++)
    {
        parameter = &parameters->listitems[i];
        reg = boa_emitter_reserveregister(emt);
        parameter->reg = reg;
        index = boa_emitter_addlocal(emt, parameter->name, parameter->length, line, false, reg);
        boa_emitter_marklocalinit(emt, index);
        /* variadic arg ...  */
        if(parameter->length == dotlen && memcmp(parameter->name, dotstr, dotlen) == 0)
        {
            return true;
        }
        if(parameter->defaultval != NULL)
        {
            jump = boa_emitter_emittmp(emt);
            boa_emitter_emitexpr(emt, parameter->defaultval, reg);
            boa_emitter_patchinstr(emt, jump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFNONNULL, reg, (int64_t)emt->chunk->compiledcodecount - jump - 1));
        }
    }
    return false;
}

/* Pre-declare the functions of a block as locals, so that they can refer to
   themselves (and each other) regardless of the order they appear in. */
void boa_emitter_emitblockhoistfunctions(BoaAstEmitter* emt, BoaAstExpression* blockexpr)
{
    size_t i;
    BoaDynListExpr* statements;
    statements = &((BoaAstBlockExpr*)blockexpr)->statements;
    for(i = 0; i < statements->listcount; i++)
    {
        if(statements->listitems[i] != NULL && statements->listitems[i]->type == BOA_ASTEXPRTYP_FUNCTION)
        {
            BoaAstFunctionExpr* fexpr = (BoaAstFunctionExpr*)statements->listitems[i];
            bool isexport = fexpr->exported;
            bool isprivate = !isexport && emt->compiler->enclosing == NULL && emt->compiler->scopedepth == 0;
            bool local = !(isexport || isprivate);
            if(local)
            {
                uint16_t r = boa_emitter_reserveregister(emt);
                int idx = boa_emitter_addlocal(emt, fexpr->name, fexpr->length, blockexpr->line, false, r);
                boa_emitter_marklocalinit(emt, idx);
            }
        }
    }
}

/* Emit a code block as an rvalue: the value of its trailing expression
   statement becomes the value of the block, otherwise the value is null. */
void boa_emitter_emitblockvalue(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint64_t reg)
{
    size_t i;
    bool endedscope;
    bool hasvalue;
    BoaDynListExpr* statements;
    statements = &((BoaAstBlockExpr*)topexpr)->statements;
    boa_emitter_scopebegin(emt);
    boa_emitter_emitblockhoistfunctions(emt, topexpr);
    endedscope = false;
    hasvalue = false;
    for(i = 0; i < statements->listcount; i++)
    {
        if(i == statements->listcount - 1 && statements->listitems[i] != NULL)
        {
            BoaAstExprType lasttype = statements->listitems[i]->type;
            if(lasttype == BOA_ASTEXPRTYP_EXPRESSION)
            {
                boa_emitter_emitexpr(emt, ((BoaAstExprStmtExpr*)statements->listitems[i])->exvalue, reg);
                hasvalue = true;
                break;
            }
            if(lasttype == BOA_ASTEXPRTYP_IF || lasttype == BOA_ASTEXPRTYP_BLOCK || lasttype == BOA_ASTEXPRTYP_CLASS)
            {
                boa_emitter_emitexpr(emt, statements->listitems[i], reg);
                hasvalue = true;
                break;
            }
        }
        if(boa_emitter_emitstmt(emt, statements->listitems[i]))
        {
            endedscope = true;
            break;
        }
    }
    if(!endedscope)
    {
        if(!hasvalue)
        {
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADNULL, reg, 0, 0);
        }
        boa_emitter_scopeend(emt);
    }
}

/* Emit a class definition into an already reserved register (or into 'reg' when
   the class itself is the value of the enclosing expression). */
void boa_emitter_emitclassdef(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint64_t classreg)
{
    BoaAstVarDeclExpr* var;
    BoaAstClassExpr* classexpr;
    BoaAstExpression* s;
    BoaString* oldclassname;
    size_t i;
    size_t superlocal;
    int nameconst;
    int fieldnameconst;
    uint16_t reg;
    uint16_t b;
    uint16_t constidx;
    bool hasparent;
    bool oldclasshassuper;
    uint32_t oldclassregister;
    classexpr = (BoaAstClassExpr*)topexpr;
    hasparent = classexpr->parent != NULL;
    b = 0;
    oldclassname = emt->classname;
    oldclasshassuper = emt->classhassuper;
    oldclassregister = emt->classregister;
    emt->classname = classexpr->name;
    if(hasparent)
    {
        constidx = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(classexpr->parent));
        b = boa_emitter_reserveregister(emt);
        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_GLOBALGET, b, constidx);
    }
    nameconst = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(classexpr->name));
    emt->classregister = (uint32_t)classreg;
    boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_CLASSMAKE, nameconst, hasparent ? b + 1 : 0, classreg);
    if(hasparent)
    {
        boa_emitter_freeregister(emt, b);
        emt->classhassuper = true;
        boa_emitter_scopebegin(emt);
        superlocal = boa_emitter_addlocal(emt, boa_string_getdata(emt->pstate->strings.strsuper), boa_string_getlength(emt->pstate->strings.strsuper), emt->lastline, false, boa_emitter_reserveregister(emt));
        boa_emitter_marklocalinit(emt, superlocal);
    }
    for(i = 0; i < classexpr->staticfields.listcount; i++)
    {
        s = classexpr->staticfields.listitems[i];
        if(s->type == BOA_ASTEXPRTYP_VARDECL)
        {
            var = (BoaAstVarDeclExpr*)s;
            reg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, var->init, reg);
            fieldnameconst = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(boa_string_copylen(emt->pstate, var->name, var->length)));
            boa_emitter_emitabc(emt, s->line, BOA_OPCODE_CLASSPUTFIELDSTATIC, classreg, fieldnameconst, reg);
            boa_emitter_freeregister(emt, reg);
        }
        else
        {
            boa_emitter_emitstmt(emt, s);
        }
    }
    if(hasparent)
    {
        boa_emitter_scopeend(emt);
    }
    emt->classname = oldclassname;
    emt->classhassuper = oldclasshassuper;
    emt->classregister = oldclassregister;
}

void boa_emitter_emitexprfull(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint64_t reg, bool ignored);

void boa_emitter_emitexprignoringregister(BoaAstEmitter* emt, BoaAstExpression* topexpr)
{
    uint8_t reg = boa_emitter_reserveregister(emt);
    boa_emitter_emitexprfull(emt, topexpr, reg, true);
    boa_emitter_freeregister(emt, reg);
}

void boa_emitter_emitexpr(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint64_t reg)
{
    boa_emitter_emitexprfull(emt, topexpr, reg, false);
}

void boa_emitter_emitexprfull(BoaAstEmitter* emt, BoaAstExpression* topexpr, uint64_t reg, bool ignored)
{
    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case BOA_ASTEXPRTYP_LITERAL:
        {
            BoaValue value;
            uint64_t constant;
            value = ((BoaAstLiteralValExpr*)topexpr)->value;
            if(boa_value_isnull(value))
            {
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADNULL, reg, 0, 0);
            }
            else if(boa_value_isbool(value))
            {
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADBOOL, reg, (uint8_t)boa_value_asbool(value), 0);
            }
            else
            {
                constant = boa_emitter_addconst(emt, topexpr->line, value);
                BOA_BIT_SETBIT(constant, BOA_BITFLAG_CONSTANT_BX);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, constant);
            }
            break;
        }
        case BOA_ASTEXPRTYP_UNARY:
            {
                BoaAstUnaryExpr* uexpr;
                uint16_t b;
                uexpr = (BoaAstUnaryExpr*)topexpr;
                b = boa_emitter_parsearg(emt, uexpr->right, reg);
                boa_emitter_emitabc(emt, topexpr->line, boa_emitter_translateunaryop(uexpr->op), reg, b, 0);
            }
            break;
        case BOA_ASTEXPRTYP_BINARY:
        {
            BoaAstBinaryExpr* bexpr;

            bexpr = (BoaAstBinaryExpr*)topexpr;
            switch(bexpr->op)
            {
                case BOA_ASTTOKTYP_GREATERTHAN:
                case BOA_ASTTOKTYP_GREATEREQUAL:
                case BOA_ASTTOKTYP_LESSTHAN:
                case BOA_ASTTOKTYP_LESSEQUAL:
                case BOA_ASTTOKTYP_EQUALEQUAL:
                {
                    boa_emitter_emitbinaryexpr(emt, bexpr, reg, false);
                    break;
                }
                case BOA_ASTTOKTYP_BANGEQUAL:
                {
                    boa_emitter_emitbinaryexpr(emt, bexpr, reg, false);
                    boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_NOT, reg, reg, false);
                    break;
                }
                default:
                {
                    boa_emitter_emitbinaryexpr(emt, bexpr, reg, false);
                    break;
                }
            }
            break;
        }
        case BOA_ASTEXPRTYP_VARGET:
        {
            uint64_t constant;
            BoaAstVarGetExpr* vgetexpr;
            bool ref;
            int index;
            uint64_t originalreg;
            vgetexpr = (BoaAstVarGetExpr*)topexpr;
            ref = emt->emitreference > 0;
            if(ref)
            {
                emt->emitreference--;
            }
            index = boa_emitter_resolvelocal(emt, emt->compiler, vgetexpr->name, vgetexpr->length, topexpr->line);
            if(index == -1)
            {
                index = boa_emitter_resolveupvalue(emt, emt->compiler, vgetexpr->name, vgetexpr->length, topexpr->line);
                if(index == -1)
                {
                    index = boa_emitter_resolveprivate(emt, vgetexpr->name, vgetexpr->length, topexpr->line);
                    if(index == -1)
                    {
                        constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(boa_string_copylen(emt->pstate, vgetexpr->name, vgetexpr->length)));
                        if(ref)
                        {
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_REFGLOBAL, reg, constant);
                        }
                        else
                        {
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_GLOBALGET, reg, constant);
                        }
                    }
                    else
                    {
                        if(ref)
                        {
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_REFPRIVATE, reg, index);
                        }
                        else
                        {
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_PRIVATEGET, reg, index);
                        }
                    }
                }
                else
                {
                    if(ref)
                    {
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_REFUPVALUE, reg, index);
                    }
                    else
                    {
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_UPVALUEGET, reg, index);
                    }
                }
            }
            else
            {
                originalreg = emt->compiler->locals.listitems[index].reg;
                if(ref)
                {
                    boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_REFLOCAL, reg, originalreg, 0);
                }
                else if(reg != originalreg)
                {
                    boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, originalreg);
                }
            }
            break;
        }
        case BOA_ASTEXPRTYP_ASSIGN:
        {
            uint64_t constant;
            uint16_t b;
            BoaAstVarGetExpr* vgetexpr;
            int index;
            BoaAstAssignExpr* aexpr;
            uint16_t r;
            BoaAstSubscriptExpr* subexpr;
            uint8_t rega;
            uint8_t regb;
            BoaAstIndexGetExpr* igetexpr;
            uint8_t rv;
            uint64_t originalreg;

            aexpr = (BoaAstAssignExpr*)topexpr;
            if(aexpr->to->type == BOA_ASTEXPRTYP_VARGET)
            {
                vgetexpr = (BoaAstVarGetExpr*)aexpr->to;
                index = boa_emitter_resolvelocal(emt, emt->compiler, vgetexpr->name, vgetexpr->length, aexpr->to->line);
                if(index == -1)
                {
                    b = boa_emitter_parsearg(emt, aexpr->value, reg);
                    index = boa_emitter_resolveupvalue(emt, emt->compiler, vgetexpr->name, vgetexpr->length, aexpr->to->line);
                    if(index == -1)
                    {
                        index = boa_emitter_resolveprivate(emt, vgetexpr->name, vgetexpr->length, aexpr->to->line);
                        if(index == -1)
                        {
                            constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(boa_string_copylen(emt->pstate, vgetexpr->name, vgetexpr->length)));
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_GLOBALSET, constant, b);
                        }
                        else
                        {
                            if(emt->privlist.listitems[index].constant)
                            {
                                boa_emitter_raiseerror(emt, topexpr->line, "attempt to modify constant <%.*s>", vgetexpr->length, vgetexpr->name);
                            }
                            if(BOA_BIT_ISSET(b, BOA_BITFLAG_CONSTANT))
                            {
                                BOA_BIT_SETBIT(index, BOA_BITFLAG_VMCONST);
                            }
                            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_PRIVATESET, b, index);
                        }
                    }
                    else
                    {
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_UPVALUESET, index, b);
                    }
                    if(!ignored && reg != b)
                    {
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, b);
                    }
                }
                else
                {
                    originalreg = emt->compiler->locals.listitems[index].reg;
                    if(emt->compiler->locals.listitems[index].constant)
                    {
                        boa_emitter_raiseerror(emt, topexpr->line, "attempt to modify constant <%.*s>", vgetexpr->length, vgetexpr->name);
                    }
                    if(aexpr->value->type == BOA_ASTEXPRTYP_LITERAL || aexpr->value->type == BOA_ASTEXPRTYP_VARGET)
                    {
                        boa_emitter_emitexpr(emt, aexpr->value, originalreg);
                    }
                    else
                    {
                        r = boa_emitter_reserveregister(emt);
                        boa_emitter_emitexpr(emt, aexpr->value, r);
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, originalreg, r);
                        boa_emitter_freeregister(emt, r);
                    }
                    if(!ignored && reg != originalreg)
                    {
                        boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, originalreg);
                    }
                }
            }
            else if(aexpr->to->type == BOA_ASTEXPRTYP_SUBSCRIPT)
            {
                subexpr = (BoaAstSubscriptExpr*)aexpr->to;
                boa_emitter_emitexpr(emt, subexpr->array, reg);
                rega = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, subexpr->index, rega);
                regb = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, aexpr->value, regb);
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_SUBSCRIPTSET, reg, rega, regb);
                boa_emitter_freeregister(emt, rega);
                boa_emitter_freeregister(emt, regb);
            }
            else if(aexpr->to->type == BOA_ASTEXPRTYP_INDEXGET)
            {
                igetexpr = (BoaAstIndexGetExpr*)aexpr->to;
                r = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, igetexpr->where, r);
                rv = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, aexpr->value, rv);
                constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(boa_string_copylen(emt->pstate, igetexpr->name, igetexpr->length)));
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_FIELDSET, r, constant, rv);
                if(!ignored && reg != rv)
                {
                    boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, rv);
                }
                boa_emitter_freeregister(emt, r);
                boa_emitter_freeregister(emt, rv);
            }
            else if(aexpr->to->type == BOA_ASTEXPRTYP_REFERENCE)
            {
                r = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, ((BoaAstRefExpr*)aexpr->to)->to, r);
                rv = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, aexpr->value, rv);
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_REFSET, r, rv, 0);
                if(!ignored && reg != rv)
                {
                    boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, rv);
                }
                boa_emitter_freeregister(emt, r);
                boa_emitter_freeregister(emt, rv);
            }
            else
            {
                boa_emitter_raiseerror(emt, topexpr->line, "invalid assigment target");
            }
            break;
        }
        case BOA_ASTEXPRTYP_CALL:
        {
            uint64_t constant;
            int index;
            BoaAstExpression* e;
            uint16_t r;
            BoaAstIndexGetExpr* igetexpr;
            BoaAstCallExpr* cexpr;
            size_t i;
            size_t argc;
            uint16_t* argregs;
            bool ismethod;
            bool issuper;
            uint64_t originalreg;
            bool shouldmoveback;
            uint64_t tmpreg;
            uint64_t supadd;
            uint64_t argreg;
            BoaAstLiteralObjectExpr* init;
            BoaAstSuperExpr* supexpr;
            BoaAstVarGetExpr* vargetexpr;
            BoaAstFunctionExpr* funcexpr;
            ismethod = false;
            issuper = false;
            cexpr = (BoaAstCallExpr*)topexpr;
            argc = cexpr->callargs.listcount;
            argregs = (uint16_t*)boa_sysmem_malloc(argc * sizeof(uint16_t));
            if(cexpr->excallee != NULL)
            {
                ismethod = cexpr->excallee->type == BOA_ASTEXPRTYP_INDEXGET;
                issuper = cexpr->excallee->type == BOA_ASTEXPRTYP_SUPER;
            }
            originalreg = reg;
            shouldmoveback = false;
            if(reg != emt->compiler->registersused - 1)
            {
                reg = boa_emitter_reserveregister(emt);
                shouldmoveback = true;
            }
            if(ismethod)
            {
                ((BoaAstIndexGetExpr*)cexpr->excallee)->ignoreemit = true;
            }
            else if(issuper)
            {
                ((BoaAstSuperExpr*)cexpr->excallee)->ignoreemit = true;
            }
            boa_emitter_emitexpr(emt, cexpr->excallee, reg);
            tmpreg = issuper ? boa_emitter_reserveregister(emt) : 0;
            for(i = 0; i < argc; i++)
            {
                supadd = (issuper ? 2 : 1);
                argreg = boa_emitter_reserveregister(emt);
                e = cexpr->callargs.listitems[i];
                if(argreg != ((reg + i) + supadd))
                {
                    /* something went terribly wrong */
                    argreg = (reg + i + supadd);
                    BOA_UTIL_UNREACHABLE();
                }
                argregs[i] = argreg;
                boa_emitter_emitexpr(emt, e, argreg);
            }
            if(ismethod)
            {
                if(cexpr->excallee->type != BOA_ASTEXPRTYP_INDEXGET)
                {
                    /* TODO: replace with a proper error code? */
                    BOA_UTIL_UNREACHABLE();
                }
                igetexpr = (BoaAstIndexGetExpr*)cexpr->excallee;
                constant = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, igetexpr->name, igetexpr->length)));
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_INVOKE, reg, argc + 1, constant);
            }
            else if(issuper)
            {
                supexpr = (BoaAstSuperExpr*)cexpr->excallee;
                index = boa_emitter_resolveupvalue(emt, emt->compiler, boa_string_getdata(emt->pstate->strings.strsuper), boa_string_getlength(emt->pstate->strings.strsuper), emt->lastline);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_UPVALUEGET, tmpreg, index);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, 0);
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_INVOKESUPER, reg, argc + 1, boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(supexpr->methodname)));
                boa_emitter_freeregister(emt, tmpreg);
            }
            else
            {
                uint64_t nameconst;
                if(cexpr->excallee->type == BOA_ASTEXPRTYP_VARGET)
                {
                    vargetexpr = (BoaAstVarGetExpr*)cexpr->excallee;
                    nameconst = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, vargetexpr->name, vargetexpr->length)));
                }
                else if(cexpr->excallee->type == BOA_ASTEXPRTYP_FUNCANON)
                {
                    funcexpr = (BoaAstFunctionExpr*)cexpr->excallee;
                    if(funcexpr->name != NULL)
                    {
                        nameconst = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, funcexpr->name, funcexpr->length)));
                    }
                    else
                    {
                        nameconst = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, "?", 1)));
                    }
                }
                else
                {
                    nameconst = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, "?", 1)));
                }
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_CALLCALLABLE, reg, argc + 1, nameconst);
            }
            for(i = 0; i < argc; i++)
            {
                boa_emitter_freeregister(emt, argregs[i]);
            }
            boa_sysmem_free(argregs);
            if(ismethod)
            {
                e = cexpr->excallee;
                while(e != NULL)
                {
                    if(e->type == BOA_ASTEXPRTYP_INDEXGET)
                    {
                        igetexpr = (BoaAstIndexGetExpr*)e;
                        if(igetexpr->jump > 0)
                        {
                            boa_emitter_patchinstr(emt, igetexpr->jump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFNULL, reg, (int64_t)emt->chunk->compiledcodecount - igetexpr->jump - 1));
                        }
                        e = igetexpr->where;
                    }
                    else if(e->type == BOA_ASTEXPRTYP_SUBSCRIPT)
                    {
                        e = ((BoaAstSubscriptExpr*)e)->array;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(shouldmoveback)
            {
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, originalreg, reg);
                boa_emitter_freeregister(emt, reg);
                reg = originalreg;
            }
            if(cexpr->init != NULL)
            {
                init = (BoaAstLiteralObjectExpr*)cexpr->init;
                r = boa_emitter_reserveregister(emt);
                for(i = 0; i < init->objexvalues.listcount; i++)
                {
                    e = init->objexvalues.listitems[i];
                    emt->lastline = e->line;
                    boa_emitter_emitexpr(emt, e, r);
                    boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_OBJECTPUSH, reg, boa_emitter_addconst(emt, emt->lastline, init->objexkeys.listitems[i]), r);
                }
                boa_emitter_freeregister(emt, r);
            }
            break;
        }
        case BOA_ASTEXPRTYP_INDEXGET:
        {
            uint64_t constant;
            bool ref;
            BoaAstIndexGetExpr* igetexpr;
            bool jump;
            bool emit;

            igetexpr = (BoaAstIndexGetExpr*)topexpr;
            ref = emt->emitreference > 0;
            if(ref)
            {
                emt->emitreference--;
            }
            jump = igetexpr->jump == 0;
            emit = !igetexpr->ignoreemit;
            boa_emitter_emitexpr(emt, igetexpr->where, reg);
            if(jump)
            {
                igetexpr->jump = boa_emitter_emittmp(emt);
                if(!igetexpr->ignoreemit)
                {
                    constant = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, igetexpr->name, igetexpr->length)));
                    if(ref)
                    {
                        boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_REFFIELD, reg, reg, constant);
                    }
                    else
                    {
                        boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_FIELDGET, reg, reg, constant);
                    }
                }
                boa_emitter_patchinstr(emt, igetexpr->jump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFNULL, reg, (int64_t)emt->chunk->compiledcodecount - igetexpr->jump - 1));
            }
            else if(emit)
            {
                constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(boa_string_copylen(emt->pstate, igetexpr->name, igetexpr->length)));
                if(ref)
                {
                    boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_REFFIELD, reg, reg, constant);
                }
                else
                {
                    boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_FIELDGET, reg, reg, constant);
                }
            }
            break;
        }
        case BOA_ASTEXPRTYP_INDEXSET:
        {
            uint64_t constant;
            BoaAstIndexSetExpr* isetexpr;
            uint8_t wherereg;
            uint8_t valuereg;

            isetexpr = (BoaAstIndexSetExpr*)topexpr;
            wherereg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, isetexpr->where, wherereg);
            valuereg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, isetexpr->value, valuereg);
            constant = boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(boa_string_copylen(emt->pstate, isetexpr->name, isetexpr->length)));
            boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_FIELDSET, wherereg, constant, valuereg);
            if(!ignored && reg != valuereg)
            {
                /* pains me to do this, but we gotta ensure that the value is after the where */
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, reg, valuereg);
            }
            boa_emitter_freeregister(emt, wherereg);
            boa_emitter_freeregister(emt, valuereg);
            break;
        }
        case BOA_ASTEXPRTYP_SUBSCRIPT:
        {
            uint16_t r;
            BoaAstSubscriptExpr* subexpr;

            subexpr = (BoaAstSubscriptExpr*)topexpr;
            boa_emitter_emitexpr(emt, subexpr->array, reg);
            r = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, subexpr->index, r);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_SUBSCRIPTGET, reg, r, 0);
            boa_emitter_freeregister(emt, r);
            break;
        }
        case BOA_ASTEXPRTYP_ARRAY:
        {
            uint16_t r;
            size_t i;
            BoaAstLiteralArrayExpr* arrexpr;

            arrexpr = (BoaAstLiteralArrayExpr*)topexpr;
            r = boa_emitter_reserveregister(emt);
            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MAKEARRAY, reg, arrexpr->exvalues.listcount);
            for(i = 0; i < arrexpr->exvalues.listcount; i++)
            {
                boa_emitter_emitexpr(emt, arrexpr->exvalues.listitems[i], r);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_ARRAYPUSH, reg, r);
            }
            boa_emitter_freeregister(emt, r);
            break;
        }
        case BOA_ASTEXPRTYP_OBJECT:
        {
            uint16_t r;
            size_t i;
            BoaAstLiteralObjectExpr* init;

            init = (BoaAstLiteralObjectExpr*)topexpr;
            r = boa_emitter_reserveregister(emt);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_MAKEOBJECT, reg, 0, 0);
            for(i = 0; i < init->objexvalues.listcount; i++)
            {
                boa_emitter_emitexpr(emt, init->objexvalues.listitems[i], r);
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_OBJECTPUSH, reg, boa_emitter_addconst(emt, emt->lastline, init->objexkeys.listitems[i]), r);
            }
            boa_emitter_freeregister(emt, r);
            break;
        }
        case BOA_ASTEXPRTYP_FUNCANON:
        {
            uint64_t constant;
            uint16_t r;
            size_t i;
            bool jump;
            bool emit;
            BoaAstFunctionExpr* fexpr;
            BoaAstCompiler stackcc;
            BoaFuncScript* function;
            BoaClsPrototype* clsproto;
            BoaString* name;
            uint64_t functionreg;
            bool closure;
            fexpr = (BoaAstFunctionExpr*)topexpr;
            name = boa_value_asstring(boa_string_valformat(emt->pstate, "lambda @:@", boa_value_fromobject(emt->module->name), boa_string_numbertostring(emt->pstate, topexpr->line)));
            boa_emitter_compilerinit(emt, &stackcc, BOA_FUNCTYPE_REGULAR);
            boa_emitter_scopebegin(emt);
            jump = boa_emitter_emitparams(emt, &fexpr->parameters, topexpr->line);
            emit = false;
            if(fexpr->body != NULL)
            {
                if(fexpr->body->type == BOA_ASTEXPRTYP_EXPRESSION)
                {
                    r = boa_emitter_reserveregister(emt);
                    stackcc.skipreturn = true;
                    boa_emitter_emitexpr(emt, ((BoaAstExprStmtExpr*)fexpr->body)->exvalue, r);
                    boa_emitter_emitabc(emt, fexpr->body->line, BOA_OPCODE_RETURN, r, 0, 0);
                    boa_emitter_freeregister(emt, r);
                }
                else
                {
                    emit = boa_emitter_emitstmt(emt, fexpr->body);
                }
            }
            if(!emit)
            {
                boa_emitter_scopeend(emt);
            }
            function = boa_emitter_compilerend(emt, name);
            function->argcount = fexpr->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = jump;
            closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = boa_emitter_reserveregister(emt);
                clsproto = boa_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    BoaAstUpvalue* upvalue = &stackcc.compiledupvalueitems[i];
                    clsproto->local[i] = upvalue->islocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(clsproto));
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MAKECLOSURE, functionreg, constant);
            }
            else
            {
                functionreg = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(function));
                BOA_BIT_SETBIT(functionreg, BOA_BITFLAG_CONSTANT);
            }
            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, functionreg);
            if(closure)
            {
                boa_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case BOA_ASTEXPRTYP_RANGE:
        {
            uint8_t regb;
            BoaAstRangeExpr* ranexpr;

            ranexpr = (BoaAstRangeExpr*)topexpr;
            boa_emitter_emitexpr(emt, ranexpr->to, reg);
            regb = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, ranexpr->from, regb);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_MAKERANGE, reg, regb, reg);
            boa_emitter_freeregister(emt, regb);
            break;
        }
        case BOA_ASTEXPRTYP_INTERPOLATION:
        {
            uint16_t r;
            size_t i;
            BoaAstStrTemplateExpr* temexpr;

            temexpr = (BoaAstStrTemplateExpr*)topexpr;
            boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MAKEARRAY, reg, temexpr->expressions.listcount);
            r = boa_emitter_reserveregister(emt);
            for(i = 0; i < temexpr->expressions.listcount; i++)
            {
                boa_emitter_emitexpr(emt, temexpr->expressions.listitems[i], r);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_ARRAYPUSH, reg, r);
            }
            boa_emitter_freeregister(emt, r);
            boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_INVOKE, reg, 1, boa_emitter_addconst(emt, emt->lastline, boa_value_fromobject(emt->pstate->strings.strjoin)));
            break;
        }
        case BOA_ASTEXPRTYP_THIS:
        {
            int index;
            BoaFuncType ftype;
            ftype = emt->compiler->type;
            if(ftype == BOA_FUNCTYPE_STATIC_METHOD)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "<this> cannot be used %s", "in static methods");
            }
            if(ftype == BOA_FUNCTYPE_CONSTRUCTOR || ftype == BOA_FUNCTYPE_METHOD || ftype == BOA_FUNCTYPE_SCRIPT || ftype == BOA_FUNCTYPE_REGULAR)
            {
                /* the instance is always in register 0 */
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, 0);
            }
            else
            {
                if(emt->compiler->enclosing == NULL)
                {
                    boa_emitter_raiseerror(emt, topexpr->line, "<this> cannot be used %s", "in functions outside of any class");
                }
                else
                {
                    index = boa_emitter_resolveupvalue(emt, emt->compiler, "this", 4, topexpr->line);
                    boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_UPVALUEGET, reg, index);
                }
            }
            break;
        }
        case BOA_ASTEXPRTYP_TERNARY:
        {
            BoaAstTernaryExpr* terexpr;
            uint8_t condreg;
            size_t condbranchskip;
            int64_t start;
            size_t elseskip;
            int64_t elsestart;

            terexpr = (BoaAstTernaryExpr*)topexpr;
            condreg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, terexpr->condition, condreg);
            condbranchskip = boa_emitter_emittmp(emt);
            boa_emitter_freeregister(emt, condreg);
            start = emt->chunk->compiledcodecount;
            boa_emitter_emitexpr(emt, terexpr->branchif, reg);
            elseskip = boa_emitter_emittmp(emt);
            boa_emitter_patchinstr(emt, condbranchskip, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - start));
            elsestart = emt->chunk->compiledcodecount;
            boa_emitter_emitexpr(emt, terexpr->branchelse, reg);
            boa_emitter_patchinstr(emt, elseskip, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - elsestart));
            break;
        }
        case BOA_ASTEXPRTYP_SUPER:
        {
            int index;
            uint64_t tmpreg;
            BoaAstSuperExpr* supexpr;

            if(emt->compiler->type == BOA_FUNCTYPE_STATIC_METHOD)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "<super> cannot be used %s", "in static methods");
            }
            else if(!emt->classhassuper)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "<super> cannot be used in class <%s>, because it does not have a super class", boa_string_getdata(emt->classname));
            }
            supexpr = (BoaAstSuperExpr*)topexpr;
            if(!supexpr->ignoreemit)
            {
                index = boa_emitter_resolveupvalue(emt, emt->compiler, boa_string_getdata(emt->pstate->strings.strsuper), boa_string_getlength(emt->pstate->strings.strsuper), emt->lastline);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, 0);
                tmpreg = boa_emitter_reserveregister(emt);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_UPVALUEGET, tmpreg, index);
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_CLASSGETSUPERMETHOD, reg, tmpreg, boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(supexpr->methodname)));
                boa_emitter_freeregister(emt, tmpreg);
            }
            break;
        }
        case BOA_ASTEXPRTYP_REFERENCE:
        {
            BoaAstExpression* to;
            int old;

            to = ((BoaAstRefExpr*)topexpr)->to;
            if(to->type != BOA_ASTEXPRTYP_VARGET && to->type != BOA_ASTEXPRTYP_INDEXGET && to->type != BOA_ASTEXPRTYP_THIS && to->type != BOA_ASTEXPRTYP_SUPER)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "invalid reference target (cannot reference literals)");
                break;
            }
            old = emt->emitreference;
            emt->emitreference++;
            boa_emitter_emitexpr(emt, to, reg);
            emt->emitreference = old;
            break;
        }
        case BOA_ASTEXPRTYP_BLOCK:
        {
            boa_emitter_emitblockvalue(emt, topexpr, reg);
            break;
        }
        case BOA_ASTEXPRTYP_IF:
        {
            BoaAstIfExpr* ifexpr;
            BoaAstExpression* e;
            uint64_t nextjump;
            uint64_t* endjumps;
            size_t i;
            size_t condbranchskip;
            size_t ifskip;
            size_t endjumpcount;
            size_t start;
            uint16_t condreg;
            uint16_t elseifcondreg;
            ifexpr = (BoaAstIfExpr*)topexpr;
            condreg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, ifexpr->condition, condreg);
            condbranchskip = boa_emitter_emittmp(emt);
            boa_emitter_freeregister(emt, condreg);
            start = emt->chunk->compiledcodecount;
            boa_emitter_emitblockvalue(emt, ifexpr->branchif, reg);
            /* the taken if-branch must jump over the else part */
            ifskip = boa_emitter_emittmp(emt);
            boa_emitter_patchinstr(emt, condbranchskip, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - start));
            endjumpcount = ifexpr->branchelseiflist == NULL ? 0 : ifexpr->branchelseiflist->listcount;
            endjumps = (uint64_t*)boa_sysmem_malloc(endjumpcount * sizeof(uint64_t));
            if(ifexpr->branchelseiflist != NULL)
            {
                for(i = 0; i < ifexpr->branchelseiflist->listcount; i++)
                {
                    e = ifexpr->elseifcondlist->listitems[i];
                    if(e == NULL)
                    {
                        continue;
                    }
                    elseifcondreg = boa_emitter_reserveregister(emt);
                    boa_emitter_emitexpr(emt, e, elseifcondreg);
                    nextjump = boa_emitter_emittmp(emt);
                    boa_emitter_freeregister(emt, elseifcondreg);
                    boa_emitter_emitblockvalue(emt, ifexpr->branchelseiflist->listitems[i], reg);
                    endjumps[i] = boa_emitter_emittmp(emt);
                    boa_emitter_patchinstr(emt, nextjump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, elseifcondreg, (int64_t)emt->chunk->compiledcodecount - nextjump - 1));
                }
            }
            if(ifexpr->branchelse)
            {
                boa_emitter_emitblockvalue(emt, ifexpr->branchelse, reg);
            }
            else
            {
                /* no branch was taken, so the value is null */
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADNULL, reg, 0, 0);
            }
            boa_emitter_patchinstr(emt, ifskip, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - ifskip - 1));
            for(i = 0; i < endjumpcount; i++)
            {
                if(ifexpr->elseifcondlist->listitems[i] == NULL)
                {
                    continue;
                }
                boa_emitter_patchinstr(emt, endjumps[i], BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - endjumps[i] - 1));
            }
            boa_sysmem_free(endjumps);
            break;
        }
        case BOA_ASTEXPRTYP_CLASS:
        {
            boa_emitter_emitclassdef(emt, topexpr, reg);
            break;
        }
        default:
        {
            boa_emitter_raiseerror(emt, topexpr->line, "unknown expression with id <%i>", (int)topexpr->type);
            break;
        }
    }
}

void boa_emitter_patchloopjumps(BoaAstEmitter* emt, BoaDynListUInt* breaks)
{
    size_t i;
    for(i = 0; i < breaks->listcount; i++)
    {
        boa_emitter_patchinstr(emt, breaks->listitems[i], BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - breaks->listitems[i] - 1));
    }
    boa_dynlistuint_destroy(breaks);
}

void boa_emitter_emitstmtscoped(BoaAstEmitter* emt, BoaAstExpression* expr)
{
    boa_emitter_scopebegin(emt);
    if(!boa_emitter_emitstmt(emt, expr))
    {
        boa_emitter_scopeend(emt);
    }
}

bool boa_emitter_emitstmt(BoaAstEmitter* emt, BoaAstExpression* topexpr)
{
    if(topexpr == NULL)
    {
        return false;
    }
    switch(topexpr->type)
    {
        case BOA_ASTEXPRTYP_EXPRESSION:
        {
            boa_emitter_emitexprignoringregister(emt, ((BoaAstExprStmtExpr*)topexpr)->exvalue);
            break;
        }
        case BOA_ASTEXPRTYP_BLOCK:
        {
            BoaDynListExpr* statements;
            size_t i;
            bool endedscope;
            statements = &((BoaAstBlockExpr*)topexpr)->statements;
            endedscope = false;
            boa_emitter_emitblockhoistfunctions(emt, topexpr);
            for(i = 0; i < statements->listcount; i++)
            {
                if(boa_emitter_emitstmt(emt, statements->listitems[i]))
                {
                    endedscope = true;
                    break;
                }
            }
            if(endedscope)
            {
                return true;
            }
            break;
        }
        case BOA_ASTEXPRTYP_VARDECL:
        {
            BoaAstVarDeclExpr* var;
            int index;
            uint16_t reg;
            bool isprivate;
            var = (BoaAstVarDeclExpr*)topexpr;
            isprivate = emt->compiler->enclosing == NULL && emt->compiler->scopedepth == 0;
            index = 0;
            reg = boa_emitter_reserveregister(emt);
            if(!isprivate)
            {
                index = boa_emitter_addlocal(emt, var->name, var->length, topexpr->line, var->isconstant, reg);
            }
            else
            {
                index = boa_emitter_resolveprivate(emt, var->name, var->length, topexpr->line);
            }
            if(var->init == NULL)
            {
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADNULL, reg, 0, 0);
            }
            else
            {
                boa_emitter_emitexpr(emt, var->init, reg);
            }
            if(isprivate)
            {
                boa_emitter_markprivateinit(emt, index);
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_PRIVATESET, reg, index);
                boa_emitter_freeregister(emt, reg);
            }
            else
            {
                boa_emitter_marklocalinit(emt, index);
            }
            break;
        }
        case BOA_ASTEXPRTYP_IF:
        {
            BoaAstIfExpr* ifexpr;
            BoaAstExpression* e;
            uint64_t nextjump;
            uint64_t* endjumps;
            size_t i;
            size_t condbranchskip;
            size_t elseskip;
            size_t endjumpcount;
            size_t start;
            uint16_t condreg;
            uint16_t elseifcondreg;
            ifexpr = (BoaAstIfExpr*)topexpr;
            condreg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, ifexpr->condition, condreg);
            condbranchskip = boa_emitter_emittmp(emt);
            elseskip = 0;
            boa_emitter_freeregister(emt, condreg);
            start = emt->chunk->compiledcodecount;
            boa_emitter_emitstmtscoped(emt, ifexpr->branchif);
            if(ifexpr->branchelse)
            {
                elseskip = boa_emitter_emittmp(emt);
            }
            boa_emitter_patchinstr(emt, condbranchskip, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - start));
            endjumpcount = ifexpr->branchelseiflist == NULL ? 0 : ifexpr->branchelseiflist->listcount;
            endjumps = (uint64_t*)boa_sysmem_malloc(endjumpcount * sizeof(uint64_t));
            if(ifexpr->branchelseiflist != NULL)
            {
                for(i = 0; i < ifexpr->branchelseiflist->listcount; i++)
                {
                    e = ifexpr->elseifcondlist->listitems[i];
                    if(e == NULL)
                    {
                        continue;
                    }
                    elseifcondreg = boa_emitter_reserveregister(emt);
                    boa_emitter_emitexpr(emt, e, elseifcondreg);
                    nextjump = boa_emitter_emittmp(emt);
                    boa_emitter_freeregister(emt, elseifcondreg);
                    boa_emitter_emitstmtscoped(emt, ifexpr->branchelseiflist->listitems[i]);
                    endjumps[i] = boa_emitter_emittmp(emt);
                    boa_emitter_patchinstr(emt, nextjump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, elseifcondreg, (int64_t)emt->chunk->compiledcodecount - nextjump - 1));
                }
            }
            if(ifexpr->branchelse)
            {
                boa_emitter_emitstmtscoped(emt, ifexpr->branchelse);
                boa_emitter_patchinstr(emt, elseskip, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - elseskip - 1));
            }
            for(i = 0; i < endjumpcount; i++)
            {
                if(ifexpr->elseifcondlist->listitems[i] == NULL)
                {
                    continue;
                }
                boa_emitter_patchinstr(emt, endjumps[i], BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - endjumps[i] - 1));
            }
            boa_sysmem_free(endjumps);
            break;
        }
        case BOA_ASTEXPRTYP_SWITCH:
            {
                size_t i;
                size_t count;
                size_t defaultidx;
                uint16_t condreg;
                uint16_t rc;
                uint16_t b;
                uint64_t fallbackjump;
                uint64_t bodystartpc;
                uint64_t* bodyjumps;
                uint64_t defaultstartpc;
                BoaDynListUInt oldbreaks;
                BoaAstExpression* casecond;
                BoaAstSwitchExpr* switchexpr;
                switchexpr = (BoaAstSwitchExpr*)topexpr;
                count = switchexpr->caseconditions.listcount;
                bodyjumps = (uint64_t*)boa_sysmem_malloc(count * sizeof(uint64_t));
                defaultidx = (size_t)-1;
                defaultstartpc = 0;
                boa_emitter_scopebegin(emt);
                emt->compiler->switchdepth++;
                oldbreaks = emt->breaks;
                boa_dynlistuint_init(&emt->breaks);
                /* 1. Evaluate condition/scrutinee */
                condreg = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, switchexpr->condition, condreg);
                /* 2. Emit checks */
                rc = boa_emitter_reserveregister(emt);
                for(i = 0; i < count; i++)
                {
                    casecond = switchexpr->caseconditions.listitems[i];
                    if(casecond == NULL)
                    {
                        defaultidx = i;
                        bodyjumps[i] = 0;
                        continue;
                    }
                    b = boa_emitter_parsearg(emt, casecond, rc);
                    boa_emitter_emitabc(emt, casecond->line, BOA_OPCODE_EQUAL, rc, condreg, b);
                    bodyjumps[i] = boa_emitter_emittmp(emt);
                }
                boa_emitter_freeregister(emt, rc);
                boa_emitter_freeregister(emt, condreg);
                /* 3. Fallback jump (if no cases matched) */
                fallbackjump = boa_emitter_emittmp(emt);

                /* 4. Emit bodies */
                for(i = 0; i < count; i++)
                {
                    bodystartpc = emt->chunk->compiledcodecount;
                    if(i == defaultidx)
                    {
                        defaultstartpc = bodystartpc;
                    }
                    else
                    {
                        boa_emitter_patchinstr(emt, bodyjumps[i], BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFTRUE, rc, (int64_t)bodystartpc - bodyjumps[i] - 1));
                    }
                    /* Emit body statements */
                    boa_emitter_emitstmt(emt, switchexpr->casebodies.listitems[i]);
                }
                /* 5. Patch fallback jump */
                if(defaultidx != (size_t)-1)
                {
                    boa_emitter_patchinstr(emt, fallbackjump, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)defaultstartpc - fallbackjump - 1));
                }
                else
                {
                    boa_emitter_patchinstr(emt, fallbackjump, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - fallbackjump - 1));
                }
                /* 6. Patch breaks and clean up */
                boa_emitter_patchloopjumps(emt, &emt->breaks);
                emt->breaks = oldbreaks;
                emt->compiler->switchdepth--;
                boa_emitter_scopeend(emt);
                boa_sysmem_free(bodyjumps);
            }
            break;

        case BOA_ASTEXPRTYP_THROW:
        {
            BoaAstThrowExpr* throwexpr;
            uint16_t reg;
            throwexpr = (BoaAstThrowExpr*)topexpr;
            reg = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, throwexpr->exvalue, reg);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_THROW, reg, 0, 0);
            boa_emitter_freeregister(emt, reg);
            break;
        }
        case BOA_ASTEXPRTYP_TRY:
        {
            BoaAstTryExpr* tryexpr;
            uint64_t tryinstridx;
            uint64_t jumptofinally;
            uint64_t catchstart;
            uint64_t localidx;
            uint64_t finallystart;
            uint16_t reg;
            uint8_t ereg;
            tryexpr = (BoaAstTryExpr*)topexpr;
            ereg = 0;
            tryinstridx = boa_emitter_emittmp(emt);
            boa_emitter_emitstmtscoped(emt, tryexpr->tryblock);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_POPTRY, 0, 0, 0);
            jumptofinally = boa_emitter_emittmp(emt);
            catchstart = emt->chunk->compiledcodecount;
            boa_emitter_patchinstr(emt, tryinstridx, BOA_REG_FORMABXINST(BOA_OPCODE_PUSHTRY, ereg, (int64_t)catchstart - (int64_t)tryinstridx - 1));

            if(tryexpr->catchblock != NULL)
            {
                boa_emitter_scopebegin(emt);
                if(tryexpr->catchvarstr != NULL)
                {
                    reg = boa_emitter_reserveregister(emt);
                    localidx = boa_emitter_addlocal(emt, tryexpr->catchvarstr, tryexpr->catchvarlen, topexpr->line, false, reg);
                    ereg = reg;
                    boa_emitter_patchinstr(emt, tryinstridx, BOA_REG_FORMABXINST(BOA_OPCODE_PUSHTRY, ereg, (int64_t)catchstart - (int64_t)tryinstridx - 1));
                    boa_emitter_marklocalinit(emt, localidx);
                }
                boa_emitter_emitstmt(emt, tryexpr->catchblock);
                boa_emitter_scopeend(emt);
                
                finallystart = emt->chunk->compiledcodecount;
                boa_emitter_patchinstr(emt, jumptofinally, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)finallystart - (int64_t)jumptofinally - 1));

                if(tryexpr->finallyblock != NULL)
                {
                    boa_emitter_emitstmtscoped(emt, tryexpr->finallyblock);
                }
            }
            else
            {
                /*
                * No catch block. If an error occurs, it jumps here.
                * We MUST run finally and then rethrow.
                */
                if(tryexpr->finallyblock != NULL)
                {
                    boa_emitter_emitstmtscoped(emt, tryexpr->finallyblock);
                }
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_RETHROW, 0, 0, 0);
                finallystart = emt->chunk->compiledcodecount;
                boa_emitter_patchinstr(emt, jumptofinally, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)finallystart - (int64_t)jumptofinally - 1));
                /* Normal path (no error) */
                if(tryexpr->finallyblock != NULL)
                {
                    boa_emitter_emitstmtscoped(emt, tryexpr->finallyblock);
                }
            }
            break;
        }
        case BOA_ASTEXPRTYP_FUNCTION:
        {
            BoaAstFunctionExpr* funcexpr;
            BoaAstCompiler stackcc;
            BoaFuncScript* function;
            BoaString* name;
            BoaClsPrototype* clsproto;
            uint64_t functionreg;
            size_t i;
            int index;
            int nameconst;
            uint16_t reg;
            uint16_t constidx;
            bool isprivate;
            bool isexport;
            bool local;
            bool vararg;
            bool closure;
            funcexpr = (BoaAstFunctionExpr*)topexpr;
            isexport = funcexpr->exported;
            isprivate = !isexport && emt->compiler->enclosing == NULL && emt->compiler->scopedepth == 0;
            local = !(isexport || isprivate);
            reg = 0;
            index = 0;
            if(!isexport)
            {
                if(local)
                {
                    index = boa_emitter_resolvelocal(emt, emt->compiler, funcexpr->name, funcexpr->length, topexpr->line);
                    if(index == -1)
                    {
                        index = boa_emitter_addlocal(emt, funcexpr->name, funcexpr->length, topexpr->line, false, reg = boa_emitter_reserveregister(emt));
                    }
                    else
                    {
                        reg = emt->compiler->locals.listitems[index].reg;
                    }
                }
                else
                {
                    index = boa_emitter_resolveprivate(emt, funcexpr->name, funcexpr->length, topexpr->line);
                }
            }
            name = boa_string_copylen(emt->pstate, funcexpr->name, funcexpr->length);
            if(local)
            {
                boa_emitter_marklocalinit(emt, index);
            }
            else if(isprivate)
            {
                boa_emitter_markprivateinit(emt, index);
            }
            boa_emitter_compilerinit(emt, &stackcc, BOA_FUNCTYPE_REGULAR);
            boa_emitter_scopebegin(emt);
            vararg = boa_emitter_emitparams(emt, &funcexpr->parameters, topexpr->line);
            if(!boa_emitter_emitstmt(emt, funcexpr->body))
            {
                boa_emitter_scopeend(emt);
            }
            function = boa_emitter_compilerend(emt, name);
            function->argcount = funcexpr->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = boa_emitter_reserveregister(emt);
                clsproto = boa_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    BoaAstUpvalue* upvalue = &stackcc.compiledupvalueitems[i];
                    clsproto->local[i] = upvalue->islocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                constidx = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(clsproto));
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MAKECLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(function));
                BOA_BIT_SETBIT(functionreg, BOA_BITFLAG_CONSTANT);
            }
            if(isexport)
            {
                nameconst = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(function->name));
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_GLOBALSET, nameconst, functionreg);
            }
            else if(isprivate)
            {
                if(!closure)
                {
                    BOA_BIT_SETBIT(index, BOA_BITFLAG_VMCONST);
                }
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_PRIVATESET, functionreg, index);
            }
            else
            {
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MOVE, reg, functionreg);
            }
            if(closure)
            {
                boa_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case BOA_ASTEXPRTYP_RETURN:
        {
            BoaAstReturnExpr* retexpr;
            uint16_t reg;
            retexpr = (BoaAstReturnExpr*)topexpr;
            reg = boa_emitter_reserveregister(emt);
            if(retexpr->exvalue == NULL)
            {
                boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_LOADNULL, reg, 0, 0);
            }
            else
            {
                boa_emitter_emitexpr(emt, retexpr->exvalue, reg);
            }
            boa_emitter_scopeend(emt);
            boa_emitter_emitabc(emt, topexpr->line, BOA_OPCODE_RETURN, reg, 0, 0);
            boa_emitter_freeregister(emt, reg);
            return true;
        }
        case BOA_ASTEXPRTYP_WHILE:
        {
            BoaAstWhileExpr* whileexpr;
            BoaDynListUInt oldbreaks;
            BoaDynListUInt oldcontinues;
            size_t beforecond;
            size_t tmpinstr;
            uint16_t reg;
            whileexpr = (BoaAstWhileExpr*)topexpr;
            reg = boa_emitter_reserveregister(emt);
            beforecond = boa_emitter_emittmp(emt);
            emt->loopstart = beforecond;
            emt->compiler->loopdepth++;
            oldbreaks = emt->breaks;
            oldcontinues = emt->continues;
            boa_dynlistuint_init(&emt->breaks);
            boa_dynlistuint_init(&emt->continues);
            boa_emitter_emitexpr(emt, whileexpr->condition, reg);
            tmpinstr = boa_emitter_emittmp(emt);
            boa_emitter_emitstmtscoped(emt, whileexpr->body);
            boa_emitter_patchloopjumps(emt, &emt->continues);
            boa_emitter_emitasbx(emt, topexpr->line, BOA_OPCODE_JUMP, 0, (int)beforecond - emt->chunk->compiledcodecount - 1);
            boa_emitter_patchinstr(emt, tmpinstr, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, reg, emt->chunk->compiledcodecount - tmpinstr - 1));
            boa_emitter_patchloopjumps(emt, &emt->breaks);
            emt->breaks = oldbreaks;
            emt->continues = oldcontinues;
            emt->compiler->loopdepth--;
            boa_emitter_freeregister(emt, reg);
            break;
        }
        case BOA_ASTEXPRTYP_FOR:
        {
            BoaValue tmpv;
            BoaAstVarDeclExpr* var;
            BoaAstForExpr* forexpr;
            BoaDynListExpr* statements;
            BoaDynListUInt oldbreaks;
            BoaDynListUInt oldcontinues;
            size_t i;
            size_t start;
            size_t exitjump;
            size_t bodyjump;
            size_t incrstart;
            size_t sequence;
            size_t iterator;
            size_t localidx;
            uint16_t condreg;
            uint8_t tmprega;
            uint8_t tmpregb;
            bool endedscope;
            forexpr = (BoaAstForExpr*)topexpr;
            emt->compiler->loopdepth++;
            oldbreaks = emt->breaks;
            oldcontinues = emt->continues;
            boa_dynlistuint_init(&emt->breaks);
            boa_dynlistuint_init(&emt->continues);
            boa_emitter_scopebegin(emt);
            if(forexpr->iscstyle)
            {
                if(forexpr->var != NULL)
                {
                    boa_emitter_emitstmt(emt, forexpr->var);
                }
                else if(forexpr->init != NULL)
                {
                    boa_emitter_emitexprignoringregister(emt, forexpr->init);
                }
                start = emt->chunk->compiledcodecount;
                exitjump = 0;
                condreg = 0;
                if(forexpr->condition != NULL)
                {
                    condreg = boa_emitter_reserveregister(emt);
                    boa_emitter_emitexpr(emt, forexpr->condition, condreg);
                    exitjump = boa_emitter_emittmp(emt);
                }
                if(forexpr->increment != NULL)
                {
                    bodyjump = boa_emitter_emittmp(emt);
                    incrstart = emt->chunk->compiledcodecount;
                    boa_emitter_emitexprignoringregister(emt, forexpr->increment);
                    boa_emitter_emitasbx(emt, topexpr->line, BOA_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                    start = incrstart;
                    boa_emitter_patchinstr(emt, bodyjump, BOA_REG_FORMASBXINST(BOA_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - bodyjump - 1));
                }
                emt->loopstart = start;
                endedscope = false;
                boa_emitter_scopebegin(emt);
                if(forexpr->body != NULL)
                {
                    if(forexpr->body->type == BOA_ASTEXPRTYP_BLOCK)
                    {
                        statements = &((BoaAstBlockExpr*)forexpr->body)->statements;
                        for(i = 0; i < statements->listcount; i++)
                        {
                            if(boa_emitter_emitstmt(emt, statements->listitems[i]))
                            {
                                endedscope = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        endedscope = boa_emitter_emitstmt(emt, forexpr->body);
                    }
                }
                boa_emitter_patchloopjumps(emt, &emt->continues);
                if(!endedscope)
                {
                    boa_emitter_scopeend(emt);
                }
                boa_emitter_emitasbx(emt, topexpr->line, BOA_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                if(forexpr->condition != NULL)
                {
                    boa_emitter_patchinstr(emt, exitjump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - exitjump - 1));
                    boa_emitter_freeregister(emt, condreg);
                }
            }
            else
            {
                sequence = boa_emitter_reserveregister(emt);
                boa_emitter_marklocalinit(emt, boa_emitter_addlocal(emt, "seq ", 4, topexpr->line, false, sequence));
                condreg = boa_emitter_reserveregister(emt);
                boa_emitter_emitexpr(emt, forexpr->condition, condreg);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, sequence, condreg);
                iterator = boa_emitter_reserveregister(emt);
                boa_emitter_marklocalinit(emt, boa_emitter_addlocal(emt, "iter ", 5, topexpr->line, false, iterator));
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_LOADNULL, iterator, 0, 0);
                start = emt->chunk->compiledcodecount;
                emt->loopstart = emt->chunk->compiledcodecount;
                /* iter = seq.iterator(iter) */
                tmprega = boa_emitter_reserveregister(emt);
                tmpregb = boa_emitter_reserveregister(emt);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, tmprega, sequence);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, tmpregb, iterator);
                tmpv = boa_value_fromobject(emt->pstate->strings.striterator);
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_INVOKE, tmprega, 2, boa_emitter_addconst(emt, emt->lastline, tmpv));
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, iterator, tmprega);
                /* if iter is null, just get out of the loop */
                exitjump = boa_emitter_emittmp(emt);
                boa_emitter_scopebegin(emt);
                /* var i = seq.iteratorValue(iter) */
                var = (BoaAstVarDeclExpr*)forexpr->var;
                localidx = boa_emitter_reserveregister(emt);
                boa_emitter_marklocalinit(emt, boa_emitter_addlocal(emt, var->name, var->length, topexpr->line, false, localidx));
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, tmprega, sequence);
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, tmpregb, iterator);
                tmpv = boa_value_fromobject(emt->pstate->strings.stritervalue);
                boa_emitter_emitabc(emt, emt->lastline, BOA_OPCODE_INVOKE, tmprega, 2, boa_emitter_addconst(emt, emt->lastline, tmpv));
                boa_emitter_emitabx(emt, emt->lastline, BOA_OPCODE_MOVE, localidx, tmprega);
                if(forexpr->body != NULL)
                {
                    if(forexpr->body->type == BOA_ASTEXPRTYP_BLOCK)
                    {
                        statements = &((BoaAstBlockExpr*)forexpr->body)->statements;
                        for(i = 0; i < statements->listcount; i++)
                        {
                            boa_emitter_emitstmt(emt, statements->listitems[i]);
                        }
                    }
                    else
                    {
                        boa_emitter_emitstmt(emt, forexpr->body);
                    }
                }
                boa_emitter_patchloopjumps(emt, &emt->continues);
                boa_emitter_scopeend(emt);
                boa_emitter_emitasbx(emt, topexpr->line, BOA_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                boa_emitter_patchinstr(emt, exitjump, BOA_REG_FORMABXINST(BOA_OPCODE_JUMPIFNULL, iterator, (int64_t)emt->chunk->compiledcodecount - exitjump - 1));
                boa_emitter_freeregister(emt, tmprega);
                boa_emitter_freeregister(emt, tmpregb);
                boa_emitter_freeregister(emt, condreg);
            }
            boa_emitter_patchloopjumps(emt, &emt->breaks);
            boa_emitter_scopeend(emt);
            emt->breaks = oldbreaks;
            emt->continues = oldcontinues;
            emt->compiler->loopdepth--;
            break;
        }
        case BOA_ASTEXPRTYP_BREAK:
        {
            if(emt->compiler->loopdepth == 0 && emt->compiler->switchdepth == 0)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "cannot use <%s> outside of loops/switch", "break");
            }
            boa_dynlistuint_push(&emt->breaks, boa_emitter_emittmp(emt));
            break;
        }
        case BOA_ASTEXPRTYP_CONTINUE:
        {
            if(emt->compiler->loopdepth == 0)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "cannot use <%s> outside of loops", "continue");
            }
            boa_dynlistuint_push(&emt->continues, boa_emitter_emittmp(emt));
            break;
        }
        case BOA_ASTEXPRTYP_CLASS:
        {
            uint16_t classregister;
            classregister = boa_emitter_reserveregister(emt);
            boa_emitter_emitclassdef(emt, topexpr, classregister);
            boa_emitter_freeregister(emt, classregister);
            break;
        }
        case BOA_ASTEXPRTYP_METHOD:
        {
            BoaAstMethodExpr* methodexpr;
            BoaAstCompiler stackcc;
            BoaFuncScript* function;
            BoaString* clsname;
            BoaClsPrototype* clsproto;
            BoaAstUpvalue* upvalue;
            uint64_t functionreg;
            size_t i;
            size_t ctorlen;
            int fieldnameconst;
            uint16_t constidx;
            bool vararg;
            bool closure;
            bool isconstructor;
            const char* ctorstr;
            ctorlen = boa_string_getlength(emt->pstate->strings.strconstructor);
            ctorstr = boa_string_getdata(emt->pstate->strings.strconstructor);
            methodexpr = (BoaAstMethodExpr*)topexpr;
            isconstructor = boa_string_getlength(methodexpr->name) == ctorlen && memcmp(boa_string_getdata(methodexpr->name), ctorstr, ctorlen) == 0;
            if(isconstructor && methodexpr->isstatic)
            {
                boa_emitter_raiseerror(emt, topexpr->line, "constructors cannot be static");
            }
            boa_emitter_compilerinit(emt, &stackcc, isconstructor ? BOA_FUNCTYPE_CONSTRUCTOR : (methodexpr->isstatic ? BOA_FUNCTYPE_STATIC_METHOD : BOA_FUNCTYPE_METHOD));
            boa_emitter_scopebegin(emt);
            vararg = boa_emitter_emitparams(emt, &methodexpr->parameters, topexpr->line);
            if(!boa_emitter_emitstmt(emt, methodexpr->body))
            {
                boa_emitter_scopeend(emt);
            }
            clsname = (BoaString*)boa_value_asobject(boa_value_fromobject(emt->classname));
            function = boa_emitter_compilerend(emt, clsname);
            function->argcount = methodexpr->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = boa_emitter_reserveregister(emt);
                clsproto = boa_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    upvalue = &stackcc.compiledupvalueitems[i];
                    clsproto->local[i] = upvalue->islocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                constidx = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(clsproto));
                boa_emitter_emitabx(emt, topexpr->line, BOA_OPCODE_MAKECLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(function));
                BOA_BIT_SETBIT(functionreg, BOA_BITFLAG_CONSTANT);
            }
            fieldnameconst = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(methodexpr->name));
            boa_emitter_emitabc(emt, topexpr->line, methodexpr->isstatic ? BOA_OPCODE_CLASSPUTFIELDSTATIC : BOA_OPCODE_CLASSPUTMETHOD, emt->classregister, fieldnameconst, functionreg);
            if(closure)
            {
                boa_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case BOA_ASTEXPRTYP_FIELD:
        {
            BoaAstFieldExpr* fieldexpr;
            BoaAstCompiler stackcc;
            BoaFuncScript* getter;
            BoaFuncScript* setter;
            BoaField* field;
            uint64_t constant;
            uint16_t reg;
            uint8_t r;
            fieldexpr = (BoaAstFieldExpr*)topexpr;
            getter = NULL;
            setter = NULL;
            if(fieldexpr->getter != NULL)
            {
                boa_emitter_compilerinit(emt, &stackcc, fieldexpr->isstatic ? BOA_FUNCTYPE_STATIC_METHOD : BOA_FUNCTYPE_METHOD);
                boa_emitter_scopebegin(emt);
                if(fieldexpr->getter->type == BOA_ASTEXPRTYP_EXPRESSION)
                {
                    r = boa_emitter_reserveregister(emt);
                    stackcc.skipreturn = true;
                    boa_emitter_emitexpr(emt, ((BoaAstExprStmtExpr*)fieldexpr->getter)->exvalue, r);
                    boa_emitter_emitabc(emt, fieldexpr->getter->line, BOA_OPCODE_RETURN, r, 0, 0);
                    boa_emitter_freeregister(emt, r);
                }
                if(!boa_emitter_emitstmt(emt, fieldexpr->getter))
                {
                    boa_emitter_scopeend(emt);
                }
                getter = boa_emitter_compilerend(emt, boa_value_asstring(boa_string_valformat(emt->pstate, "@:get @", boa_value_fromobject(emt->classname), fieldexpr->name)));
            }
            if(fieldexpr->setter != NULL)
            {
                boa_emitter_compilerinit(emt, &stackcc, fieldexpr->isstatic ? BOA_FUNCTYPE_STATIC_METHOD : BOA_FUNCTYPE_METHOD);
                reg = boa_emitter_reserveregister(emt);
                boa_emitter_marklocalinit(emt, boa_emitter_addlocal(emt, "value", 5, topexpr->line, false, reg));
                boa_emitter_scopebegin(emt);
                if(!boa_emitter_emitstmt(emt, fieldexpr->setter))
                {
                    boa_emitter_scopeend(emt);
                }
                boa_emitter_freeregister(emt, reg);
                setter = boa_emitter_compilerend(emt, boa_value_asstring(boa_string_valformat(emt->pstate, "@:set @", boa_value_fromobject(emt->classname), fieldexpr->name)));
                setter->argcount = 1;
                setter->maxregisters++;
            }
            field = boa_object_makefield(emt->pstate, (BoaObject*)getter, (BoaObject*)setter);
            constant = boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(field));
            BOA_BIT_SETBIT(constant, BOA_BITFLAG_CONSTANT);
            boa_emitter_emitabc(emt, topexpr->line, fieldexpr->isstatic ? BOA_OPCODE_CLASSPUTFIELDSTATIC : BOA_OPCODE_CLASSPUTMETHOD, emt->classregister, boa_emitter_addconst(emt, topexpr->line, boa_value_fromobject(fieldexpr->name)), constant);
            break;
        }
        default:
        {
            boa_emitter_raiseerror(emt, topexpr->line, "unknown statement with id <%i>", (int)topexpr->type);
            break;
        }
    }
    return false;
}

BoaModule* boa_emitter_emitmod(BoaAstEmitter* emt, BoaDynListExpr* statements, BoaString* modname)
{
    bool isnew;
    bool endedscope;
    uint8_t r;
    size_t i;
    size_t oldprivatescnt;
    size_t total;
    size_t add;
    BoaAstPrivate priv;
    BoaState* state;
    BoaValue modulevalue;
    BoaModule* module;
    BoaAstCompiler stackcc;
    BoaDynListPriv* privates;
    BoaAstExpression* stmt;
    emt->lastline = 1;
    emt->emitreference = 0;
    state = emt->pstate;
    isnew = false;
    if(boa_map_getvalue(emt->pstate->vmstate.modules, modname, &modulevalue))
    {
        module = boa_value_asmodule(modulevalue);
    }
    else
    {
        module = boa_object_makemodule(emt->pstate, modname);
        isnew = true;
    }
    emt->module = module;
    oldprivatescnt = module->privatecount;
    if(oldprivatescnt > 0)
    {
        privates = &emt->privlist;
        privates->listcount = oldprivatescnt - 1;
        priv.initialized = true;
        priv.constant = false;
        boa_dynlistpriv_push(privates, priv);
        for(i = 0; i < oldprivatescnt; i++)
        {
            privates->listitems[i].initialized = true;
        }
    }
    boa_emitter_compilerinit(emt, &stackcc, BOA_FUNCTYPE_SCRIPT);
    emt->chunk = &stackcc.function->chunk;
    boa_emitter_resolvestmtlist(emt, statements);
    boa_emitter_scopebegin(emt);
    endedscope = false;
    for(i = 0; i < statements->listcount; i++)
    {
        stmt = statements->listitems[i];
        if(i == statements->listcount - 1 && stmt->type == BOA_ASTEXPRTYP_EXPRESSION && !endedscope)
        {
            r = boa_emitter_reserveregister(emt);
            boa_emitter_emitexpr(emt, ((BoaAstExprStmtExpr*)stmt)->exvalue, r);
            boa_emitter_emitabc(emt, stmt->line, BOA_OPCODE_RETURN, r, 1, 0);
            boa_emitter_freeregister(emt, r);
            emt->compiler->skipreturn = true;
            break;
        }
        if(boa_emitter_emitstmt(emt, stmt))
        {
            endedscope = true;
            break;
        }
    }
    if(!endedscope)
    {
        boa_emitter_scopeend(emt);
    }
    module->mainfunction = boa_emitter_compilerend(emt, modname);
    if(isnew)
    {
        total = emt->privlist.listcount;
        module->privatevalues = (BoaValue*)boa_sysmem_malloc(total * sizeof(BoaValue));
        for(i = 0; i < total; i++)
        {
            module->privatevalues[i] = boa_value_makenull();
        }
    }
    else
    {
        add = (module->privatecount);
        if(add == 0)
        {
            add = 1;
        }
        module->privatevalues = (BoaValue*)boa_sysmem_realloc(module->privatevalues, sizeof(BoaValue) * add);
        for(i = oldprivatescnt; i < module->privatecount; i++)
        {
            module->privatevalues[i] = boa_value_makenull();
        }
    }
    boa_dynlistpriv_destroy(&emt->privlist);
    if(isnew && !state->haderror)
    {
        boa_map_setvalue(state->vmstate.modules, modname, boa_value_fromobject(module));
    }
    module->ran = true;
    return module;
}

const char* boa_debug_opcname(uint64_t opc)
{
    switch(opc)
    {
        case BOA_OPCODE_MOVE: return "move";
        case BOA_OPCODE_LOADNULL: return "ldnull";
        case BOA_OPCODE_LOADBOOL: return "ldbool";
        case BOA_OPCODE_MAKECLOSURE: return "mkclosure";
        case BOA_OPCODE_MAKEARRAY: return "mkarray";
        case BOA_OPCODE_MAKEOBJECT: return "mkobject";
        case BOA_OPCODE_MAKERANGE: return "mkrange";
        case BOA_OPCODE_RETURN: return "return";
        case BOA_OPCODE_MATHADD: return "m.add";
        case BOA_OPCODE_MATHSUBTRACT: return "m.subtr";
        case BOA_OPCODE_MATHMULTIPLY: return "m.mult";
        case BOA_OPCODE_MATHDIVIDE: return "m.div";
        case BOA_OPCODE_MATHFLOORDIVIDE: return "m.fldiv";
        case BOA_OPCODE_MATHMOD: return "m.mod";
        case BOA_OPCODE_MATHPOWER: return "m.pow";
        case BOA_OPCODE_MATHLEFTSHIFT: return "m.shleft";
        case BOA_OPCODE_MATHRIGHTSHIFT: return "m.shright";
        case BOA_OPCODE_BINXOR: return "m.bxor";
        case BOA_OPCODE_BINAND: return "m.band";
        case BOA_OPCODE_BINOR: return "m.bor";
        case BOA_OPCODE_JUMP: return "jump";
        case BOA_OPCODE_JUMPIFTRUE: return "jumpiftrue";
        case BOA_OPCODE_JUMPIFFALSE: return "jumpiffalse";
        case BOA_OPCODE_JUMPIFNONNULL: return "nonnulljump";
        case BOA_OPCODE_JUMPIFNULL: return "nulljump";
        case BOA_OPCODE_EQUAL: return "eq";
        case BOA_OPCODE_LESSTHAN: return "lessthan";
        case BOA_OPCODE_LESSEQUAL: return "lessequal";
        case BOA_OPCODE_GREATERTHAN: return "greater";
        case BOA_OPCODE_GREATEREQUAL: return "greaterequal";
        case BOA_OPCODE_NEGATE: return "negate";
        case BOA_OPCODE_NOT: return "not";
        case BOA_OPCODE_BINNOT: return "m.bnot";
        case BOA_OPCODE_GLOBALSET: return "globalset";
        case BOA_OPCODE_GLOBALGET: return "globalget";
        case BOA_OPCODE_UPVALUESET: return "upvset";
        case BOA_OPCODE_UPVALUEGET: return "upvget";
        case BOA_OPCODE_PRIVATESET: return "privset";
        case BOA_OPCODE_PRIVATEGET: return "privget";
        case BOA_OPCODE_CALLCALLABLE: return "call";
        case BOA_OPCODE_UPVALUECLOSE: return "upvclose";
        case BOA_OPCODE_CLASSMAKE: return "mkclass";
        case BOA_OPCODE_CLASSPUTFIELDSTATIC: return "classputstaticfield";
        case BOA_OPCODE_CLASSPUTMETHOD: return "classputmethod";
        case BOA_OPCODE_FIELDGET: return "fieldget";
        case BOA_OPCODE_CLASSGETSUPERMETHOD: return "classgetsupermethod";
        case BOA_OPCODE_FIELDSET: return "fieldset";
        case BOA_OPCODE_IS: return "is";
        case BOA_OPCODE_INVOKE: return "classinvoke";
        case BOA_OPCODE_INVOKESUPER: return "classinvokesuper";
        case BOA_OPCODE_SUBSCRIPTGET: return "indexget";
        case BOA_OPCODE_SUBSCRIPTSET: return "indexset";
        case BOA_OPCODE_ARRAYPUSH: return "arraypush";
        case BOA_OPCODE_OBJECTPUSH: return "objectpush";
        case BOA_OPCODE_REFGLOBAL: return "refglobal";
        case BOA_OPCODE_REFPRIVATE: return "refprivate";
        case BOA_OPCODE_REFLOCAL: return "reflocal";
        case BOA_OPCODE_REFUPVALUE: return "refupvalue";
        case BOA_OPCODE_REFFIELD: return "reffield";
        case BOA_OPCODE_REFSET: return "setref";
        case BOA_OPCODE_PUSHTRY: return "pushtry";
        case BOA_OPCODE_POPTRY: return "poptry";
        case BOA_OPCODE_THROW: return "throw";
        case BOA_OPCODE_RETHROW: return "rethrow";
    }
    return "?unknown?";
}

/*
 * Instruction can follow one of the three formats:
 *
 * ABC  opcode:6 bits (starting from bit 0), A:8 bits, B:9 bits, C:9 bits
 * ABx  opcode:6 bits (starting from bit 0), A:8 bits, Bx:18 bits
 * AsBx opcode:6 bits (starting from bit 0), A:8 bits, sBx:18 bits (signed)
 */

int64_t boa_vmutil_getopcode(uint64_t instruction)
{
    return (instruction & BOA_CONFIG_OPCODESIZE);
}

int64_t boa_vmutil_geta(uint64_t instruction)
{
    return (((int64_t)(instruction >> BOA_CONFIG_ARGPOSA)) & BOA_CONFIG_ARGSIZEA);
}

int64_t boa_vmutil_getb(uint64_t instruction)
{
    return (((int64_t)(instruction >> BOA_CONFIG_ARGPOSB)) & BOA_CONFIG_ARGSIZEB);
}

int64_t boa_vmutil_getc(uint64_t instruction)
{
    return (((int64_t)(instruction >> BOA_CONFIG_ARGPOSC)) & BOA_CONFIG_ARGSIZEC);
}

int64_t boa_vmutil_getbx(uint64_t instruction)
{
    return (((int64_t)(instruction >> BOA_CONFIG_ARGPOSBX)) & BOA_CONFIG_ARGSIZEBX);
}

int64_t boa_vmutil_getsbx(uint64_t instruction)
{
    int64_t first;
    int64_t second;
    first = (((int64_t)(instruction >> BOA_CONFIG_ARGPOSSBX)) & BOA_CONFIG_ARGSIZESBX);
    second = (((((int64_t)instruction) >> BOA_CONFIG_FLAGPOSSBX) & 0x1) == 1 ? -1 : 1);
    return (first * second);
}

void boa_debug_disasmodrecursive(BoaState* state, BoaStream* pr, BoaFuncScript* function, const char* source, BoaTable* disassembled)
{
    size_t i;
    BoaValue val;
    if(function == NULL)
    {
        return;
    }
    if(boa_table_getentry(disassembled, function->name, NULL))
    {
        return;
    }
    boa_table_set(disassembled, function->name, boa_value_makenull());
    boa_debug_disaschunk(pr, &function->chunk, boa_string_getdata(function->name), source);
    for(i = 0; i < function->chunk.constantlist.listcount; i++)
    {
        val = function->chunk.constantlist.listitems[i];
        if(boa_value_isfuncscript(val))
        {
            boa_debug_disasmodrecursive(state, pr, boa_value_asfuncscript(val), source, disassembled);
        }
        else if(boa_value_isobjtype(val, BOA_OBJTYPE_FUNCCLOSURE))
        {
            boa_debug_disasmodrecursive(state, pr, ((BoaFuncClosure*)boa_value_asobject(val))->function, source, disassembled);
        }
        else if(boa_value_isobjtype(val, BOA_OBJTYPE_CLSPROTOTYPE))
        {
            boa_debug_disasmodrecursive(state, pr, ((BoaClsPrototype*)boa_value_asobject(val))->function, source, disassembled);
        }
    }
}

void boa_debug_disasmodule(BoaStream* pr, BoaModule* module, const char* source)
{
    BoaTable disassembled;
    BoaState* state;
    state = module->innerobject.pstate;
    boa_table_init(state, &disassembled);
    boa_debug_disasmodrecursive(state, pr, module->mainfunction, source, &disassembled);
    boa_free_table(&disassembled);
}

void boa_debug_printconst(BoaStream* pr, BoaValue value)
{
    boa_stream_setcolor(pr, 'c');
    boa_value_printvalue(pr, value, true);
    boa_stream_resetcolor(pr);
}

void boa_debug_disaschunk(BoaStream* pr, BoaChunk* chunk, const char* name, const char* source)
{
    size_t i;
    size_t offset;
    BoaValue value;
    BoaDynListVal* list;
    list = &chunk->constantlist;
    boa_stream_printf(pr, "CHUNK %s {\n", name);
    if(list->listcount > 0)
    {
        boa_stream_setcolor(pr, 'm');
        boa_stream_printf(pr, "constants:\n");
        boa_stream_resetcolor(pr);
        for(i = 0; i < list->listcount; i++)
        {
            value = list->listitems[i];
            boa_stream_printf(pr, "% 4ld ", i);
            boa_debug_printconst(pr, value);
            boa_stream_printf(pr, "\n");
        }
    }
    boa_stream_setcolor(pr, 'm');
    boa_stream_printf(pr, "text:\n");
    boa_stream_resetcolor(pr);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        boa_debug_disasinstr(pr, chunk, offset, source, false);
    }
    boa_stream_setcolor(pr, 'm');
    boa_stream_printf(pr, "hex:\n");
    boa_stream_resetcolor(pr);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        boa_stream_printf(pr, "%08lX ", chunk->compiledcodechunk[offset]);
    }
    boa_stream_printf(pr, "\n");
    boa_stream_printf(pr, "}\n");
}

void boa_debug_callbackprintabcinstr(BoaStream* pr, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu \t%lu \t%lu\n", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction), boa_vmutil_getb(instruction), boa_vmutil_getc(instruction));
}

void boa_debug_callbackprintabxinstr(BoaStream* pr, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu \t%lu\n", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction), boa_vmutil_getbx(instruction));
}

void boa_debug_callbackprintasbxinstr(BoaStream* pr, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu \t%li\n", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction), boa_vmutil_getsbx(instruction));
}

void boa_debug_printregister(BoaStream* pr, uint16_t reg)
{
    boa_stream_printf(pr, " \t%hu", reg);
}

void boa_debug_printconstarg(BoaStream* pr, BoaChunk* chunk, uint32_t arg, bool doindent)
{
    boa_stream_printf(pr, "%sc%u (", doindent ? " \t" : "", arg);
    boa_debug_printconst(pr, chunk->constantlist.listitems[arg]);
    boa_stream_printf(pr, ")");
}

void boa_debug_printconstorregister(BoaStream* pr, BoaChunk* chunk, uint16_t arg)
{
    if(BOA_BIT_ISSET(arg, BOA_BITFLAG_CONSTANT_BX))
    {
        boa_debug_printconstarg(pr, chunk, arg & ~(1UL << BOA_BITFLAG_CONSTANT_BX), true);
    }
    else if(BOA_BIT_ISSET(arg, BOA_BITFLAG_CONSTANT))
    {
        boa_debug_printconstarg(pr, chunk, arg & ~(1UL << BOA_BITFLAG_CONSTANT), true);
    }
    else
    {
        boa_debug_printregister(pr, arg);
    }
}

void boa_debug_printunaryinstr(BoaStream* pr, BoaChunk* chunk, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction));
    boa_debug_printconstorregister(pr, chunk, boa_vmutil_getb(instruction));
    boa_stream_printf(pr, "\n");
}

void boa_debug_printbinaryinstr(BoaStream* pr, BoaChunk* chunk, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction));
    boa_debug_printconstorregister(pr, chunk, boa_vmutil_getb(instruction));
    boa_debug_printconstorregister(pr, chunk, boa_vmutil_getc(instruction));
    boa_stream_printf(pr, "\n");
}

void boa_debug_printmoveinstr(BoaStream* pr, BoaChunk* chunk, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s %lu", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "", boa_vmutil_geta(instruction));
    boa_debug_printconstorregister(pr, chunk, (uint16_t)boa_vmutil_getbx(instruction));
    boa_stream_printf(pr, "\n");
}

void boa_debug_printglobalinstr(BoaStream* pr, BoaChunk* chunk, BoaOpCode opc, uint64_t instruction)
{
    const char* name;
    name = boa_debug_opcname(opc);
    boa_stream_setcolor(pr, 'y');
    boa_stream_printf(pr, "%s", name);
    boa_stream_resetcolor(pr);
    boa_stream_printf(pr, "%*s", BOA_CONFIG_LONGESTOPNAME - (int)strlen(name), "");
    if (opc == BOA_OPCODE_GLOBALSET)
    {
        boa_debug_printconstarg(pr, chunk, (uint16_t)boa_vmutil_geta(instruction), false);
        boa_debug_printconstorregister(pr, chunk, (uint16_t)boa_vmutil_getbx(instruction));
    }
    else
    {
        boa_debug_printregister(pr, (uint16_t)boa_vmutil_geta(instruction));
        boa_debug_printconstarg(pr, chunk, (uint16_t)boa_vmutil_getbx(instruction), true);
    }
    boa_stream_printf(pr, "\n");
}

void boa_debug_disasinstr(BoaStream* pr, BoaChunk* chunk, size_t offset, const char* source, bool forceline)
{
    typedef void (*BoaDebugInstructionFn)(BoaStream*, BoaOpCode opc, uint64_t);
    static BoaDebugInstructionFn debuginstrfuncs[] = {
        boa_debug_callbackprintabcinstr,
        boa_debug_callbackprintabxinstr,
        boa_debug_callbackprintasbxinstr
    };
    uint8_t opcode;
    uint64_t instruction;
    size_t line;
    size_t index;
    bool same;
    char c;
    char* nextline;
    char* prevline;
    char* outputline;
    char* currentline;
    line = boa_chunk_getline(chunk, offset);
    same = !chunk->haslineinfo || (offset > 0 && line == boa_chunk_getline(chunk, offset - 1));
    if(!same && source != NULL)
    {
        index = 0;
        currentline = (char*)source;
        while(currentline)
        {
            nextline = strchr(currentline, '\n');
            prevline = currentline;
            index++;
            currentline = nextline ? (nextline + 1) : NULL;
            if(index == line)
            {
                outputline = prevline ? prevline : nextline;
                while((c = *outputline) && (c == '\t' || c == ' '))
                {
                    outputline++;
                }
                boa_stream_setcolor(pr, 'r');
                boa_stream_printf(pr, "        %.*s\n", nextline ? (int)(nextline - outputline) : (int)strlen(prevline), outputline);
                boa_stream_resetcolor(pr);
                break;
            }
        }
    }
    boa_stream_printf(pr, "%04ld ", offset);
    if(same && !forceline)
    {
        boa_stream_printf(pr, "   | ");
    }
    else
    {
        boa_stream_setcolor(pr, 'b');
        boa_stream_printf(pr, "%4ld ", line);
        boa_stream_resetcolor(pr);
    }
    instruction = chunk->compiledcodechunk[offset];
    opcode = boa_vmutil_getopcode(instruction);
    switch(opcode)
    {
        case BOA_OPCODE_MOVE:
            boa_debug_printmoveinstr(pr, chunk, (BoaOpCode)opcode, instruction);
            break;
        case BOA_OPCODE_MATHADD:
        case BOA_OPCODE_MATHSUBTRACT:
        case BOA_OPCODE_MATHMULTIPLY:
        case BOA_OPCODE_MATHDIVIDE:
        case BOA_OPCODE_EQUAL:
        case BOA_OPCODE_LESSTHAN:
        case BOA_OPCODE_LESSEQUAL:
            boa_debug_printbinaryinstr(pr, chunk, (BoaOpCode)opcode, instruction);
            break;
        case BOA_OPCODE_NEGATE:
        case BOA_OPCODE_NOT:
            boa_debug_printunaryinstr(pr, chunk, (BoaOpCode)opcode, instruction);
            break;
        case BOA_OPCODE_GLOBALSET:
        case BOA_OPCODE_GLOBALGET:
            boa_debug_printglobalinstr(pr, chunk, (BoaOpCode)opcode, instruction);
            break;
        default:
        {
            switch(opcode)
            {
                /* A simple way to automatically generate case printers for all the opcodes */
                #define handle_opcode(name, type)                           \
                    case name:                                              \
                    {                                                            \
                        debuginstrfuncs[(int)type](pr, name, instruction); \
                        break;                                                   \
                    }
                #if 1
                handle_opcode(BOA_OPCODE_MOVE, BOA_INSTYP_ABX) /* R(A) := RC(Bx) */
                handle_opcode(BOA_OPCODE_LOADNULL, BOA_INSTYP_ABC) /* R(A) := null */
                handle_opcode(BOA_OPCODE_LOADBOOL, BOA_INSTYP_ABC) /* R(A) := (bool) B */
                handle_opcode(BOA_OPCODE_MAKECLOSURE, BOA_INSTYP_ABX) /* R(A) := PrC[Bx] */
                handle_opcode(BOA_OPCODE_MAKEARRAY, BOA_INSTYP_ABX) /* R(A) := new Array(Bx) */
                handle_opcode(BOA_OPCODE_MAKEOBJECT, BOA_INSTYP_ABC) /* R(A) = new Object() */
                handle_opcode(BOA_OPCODE_MAKERANGE, BOA_INSTYP_ABC) /* R(A) = new Range(RC(B), RC(C)) */
                handle_opcode(BOA_OPCODE_RETURN, BOA_INSTYP_ABC) /* return R(A) */
                handle_opcode(BOA_OPCODE_MATHADD, BOA_INSTYP_ABC) /* R(A) := RC(B) + RC(C) */
                handle_opcode(BOA_OPCODE_MATHSUBTRACT, BOA_INSTYP_ABC) /* R(A) := RC(B) - RC(C) */
                handle_opcode(BOA_OPCODE_MATHMULTIPLY, BOA_INSTYP_ABC) /* R(A) := RC(B) * RC(C) */
                handle_opcode(BOA_OPCODE_MATHDIVIDE, BOA_INSTYP_ABC) /* R(A) := RC(B) / RC(C) */
                handle_opcode(BOA_OPCODE_MATHFLOORDIVIDE, BOA_INSTYP_ABC) /* R(A) := floor(RC(B) / RC(C)) */
                handle_opcode(BOA_OPCODE_MATHMOD, BOA_INSTYP_ABC) /* R(A) := RC(B) % RC(C) */
                handle_opcode(BOA_OPCODE_MATHPOWER, BOA_INSTYP_ABC) /* R(A) := pow(RC(B), RC(C)) */
                handle_opcode(BOA_OPCODE_MATHLEFTSHIFT, BOA_INSTYP_ABC) /* R(A) := RC(B) << RC(C) */
                handle_opcode(BOA_OPCODE_MATHRIGHTSHIFT, BOA_INSTYP_ABC) /* R(A) := RC(B) >> RC(C) */
                handle_opcode(BOA_OPCODE_BINXOR, BOA_INSTYP_ABC) /* R(A) := RC(B) ^ RC(C) */
                handle_opcode(BOA_OPCODE_BINAND, BOA_INSTYP_ABC) /* R(A) := RC(B) & RC(C) */
                handle_opcode(BOA_OPCODE_BINOR, BOA_INSTYP_ABC) /* R(A) := RC(B) | RC(C) */
                handle_opcode(BOA_OPCODE_JUMP, BOA_INSTYP_ASBX) /* PC += sBx */
                handle_opcode(BOA_OPCODE_JUMPIFTRUE, BOA_INSTYP_ABX) /* if (R(A)) PC += Bx */
                handle_opcode(BOA_OPCODE_JUMPIFFALSE, BOA_INSTYP_ABX) /* if (not R(A)) PC += Bx */
                handle_opcode(BOA_OPCODE_JUMPIFNONNULL, BOA_INSTYP_ABX) /* if (R(A) != null) PC += Bx */
                handle_opcode(BOA_OPCODE_JUMPIFNULL, BOA_INSTYP_ABX) /* if (R(A) == null) PC += Bx */
                handle_opcode(BOA_OPCODE_EQUAL, BOA_INSTYP_ABC) /* R(A) := RC(B) == RC(C) */
                handle_opcode(BOA_OPCODE_LESSTHAN, BOA_INSTYP_ABC) /* R(A) := RC(B) < RC(C) */
                handle_opcode(BOA_OPCODE_LESSEQUAL, BOA_INSTYP_ABC) /* R(A) := RC(B) <= RC(C) */
                handle_opcode(BOA_OPCODE_GREATERTHAN, BOA_INSTYP_ABC) /* R(A) := RC(B) > RC(C) */
                handle_opcode(BOA_OPCODE_GREATEREQUAL, BOA_INSTYP_ABC) /* R(A) := RC(B) >= RC(C) */
                handle_opcode(BOA_OPCODE_NEGATE, BOA_INSTYP_ABC) /* R(A) := -RC(B) */
                handle_opcode(BOA_OPCODE_NOT, BOA_INSTYP_ABC) /* R(A) := !RC(B) */
                handle_opcode(BOA_OPCODE_BINNOT, BOA_INSTYP_ABC) /* R(A) := ~RC(B) */
                handle_opcode(BOA_OPCODE_GLOBALSET, BOA_INSTYP_ABX) /* G[C(A)] := RC(BX) */
                handle_opcode(BOA_OPCODE_GLOBALGET, BOA_INSTYP_ABX) /* R(A) := G[C(Bx)] */
                handle_opcode(BOA_OPCODE_UPVALUESET, BOA_INSTYP_ABX) /* U[A] := RC(Bx) */
                handle_opcode(BOA_OPCODE_UPVALUEGET, BOA_INSTYP_ABX) /* R(A) := U[Bx] */
                handle_opcode(BOA_OPCODE_PRIVATESET, BOA_INSTYP_ABX) /* P[A] := RC(Bx) */
                handle_opcode(BOA_OPCODE_PRIVATEGET, BOA_INSTYP_ABX) /* R(A) := P[C(Bx)] */
                handle_opcode(BOA_OPCODE_CALLCALLABLE, BOA_INSTYP_ABC) /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(BOA_OPCODE_UPVALUECLOSE, BOA_INSTYP_ABC) /* close_upvalue(R(A)) */
                handle_opcode(BOA_OPCODE_CLASSMAKE, BOA_INSTYP_ABC) /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
                handle_opcode(BOA_OPCODE_CLASSPUTFIELDSTATIC, BOA_INSTYP_ABC) /* R(A)[C(B)] = RC(C) */
                handle_opcode(BOA_OPCODE_CLASSPUTMETHOD, BOA_INSTYP_ABC) /* R(A).Methods[C(B)] = RC(C) */
                handle_opcode(BOA_OPCODE_FIELDGET, BOA_INSTYP_ABC) /* R(A) = R(B)[C(C)] */
                handle_opcode(BOA_OPCODE_CLASSGETSUPERMETHOD, BOA_INSTYP_ABC) /* R(A) = R(B).super[C(C)] */
                handle_opcode(BOA_OPCODE_FIELDSET, BOA_INSTYP_ABC) /* R(A)[C(B)] = R(C) */
                handle_opcode(BOA_OPCODE_IS, BOA_INSTYP_ABC) /* R(A) := RC(B) is G[C(C)] */
                handle_opcode(BOA_OPCODE_INVOKE, BOA_INSTYP_ABC) /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(BOA_OPCODE_INVOKESUPER, BOA_INSTYP_ABC) /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(BOA_OPCODE_SUBSCRIPTGET, BOA_INSTYP_ABC) /* R(A) := R(A)[RC(B)] */
                handle_opcode(BOA_OPCODE_SUBSCRIPTSET, BOA_INSTYP_ABC) /* R(A)[RC(B)] := R(C) */
                handle_opcode(BOA_OPCODE_ARRAYPUSH, BOA_INSTYP_ABX) /* R(A)[R(A).listcount++] = RC(Bx) */
                handle_opcode(BOA_OPCODE_OBJECTPUSH, BOA_INSTYP_ABC) /* R(A)[R(B)] = RC(C) */
                handle_opcode(BOA_OPCODE_REFGLOBAL, BOA_INSTYP_ABX) /* R(A) := ref G(C[Bx]) */
                handle_opcode(BOA_OPCODE_REFPRIVATE, BOA_INSTYP_ABX) /* R(A) := ref P(Bx) */
                handle_opcode(BOA_OPCODE_REFLOCAL, BOA_INSTYP_ABC) /* R(A) := ref R(B) */
                handle_opcode(BOA_OPCODE_REFUPVALUE, BOA_INSTYP_ABX) /* R(A) := ref U(Bx) */
                handle_opcode(BOA_OPCODE_REFFIELD, BOA_INSTYP_ABC) /* R(A) = ref R(B)[C(C)] */
                handle_opcode(BOA_OPCODE_REFSET, BOA_INSTYP_ABC) /* ref R(A) := R(B) */
                handle_opcode(BOA_OPCODE_PUSHTRY, BOA_INSTYP_ABX) /* push try handler at PC + Bx */
                handle_opcode(BOA_OPCODE_POPTRY, BOA_INSTYP_ABC) /* pop try handler */
                handle_opcode(BOA_OPCODE_THROW, BOA_INSTYP_ABC) /* throw R(A) */
                handle_opcode(BOA_OPCODE_RETHROW, BOA_INSTYP_ABC) /* rethrow fiber->error */
                #endif
                #undef handle_opcode
                default:
                {
                    boa_stream_printf(pr, "unknown opcode %d\n", opcode);
                    break;
                }
            }
        }
    }
}

BoaString* boa_state_errorfmtv(BoaState* state, size_t line, const char* fmt, va_list args)
{
    BoaStream pr;
    boa_stream_makestackstring(&pr);
    boa_stream_printf(&pr, "[line %ld]: ", line);
    boa_stream_printfv(&pr, fmt, args);
    return boa_stream_takestring(state, &pr);
}

BoaString* boa_state_errorfmt(BoaState* state, size_t line, const char* fmt, ...)
{
    BoaString* result;
    va_list args;
    va_start(args, fmt);
    result = boa_state_errorfmtv(state, line, fmt, args);
    va_end(args);
    return result;
}

BoaValue boa_objfndefault_invalidconstructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    const char* cname;
    BoaString* name;
    (void)argc;
    (void)args;
    name = NULL; 
    cname = "?unknown?";
    if(boa_value_isinstance(instance))
    {
        name = boa_value_asinstance(instance)->klass->name;
    }
    else if(boa_value_isclass(instance))
    {
        name = boa_value_asclass(instance)->name;
    }
    if(name != NULL)
    {
        cname = boa_string_getdata(name);
    }
    return boa_vm_raiseexception(state, state->exceptions.stdexception, "class %s has no constructor", cname);
}

/*
 * Class
 */

BoaValue boa_objfnclass_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_string_valformat(state, "class @", boa_value_fromobject(boa_value_asclass(instance)->name));
}

int boa_coreutil_tableiterator(BoaTable* table, int number)
{
    if(table->htcount == 0)
    {
        return -1;
    }
    if(number >= (int)table->htcapacity)
    {
        return -1;
    }
    number++;
    for(; number < table->htcapacity; number++)
    {
        if(table->htentries[number].entkey != NULL)
        {
            return number;
        }
    }
    return -1;
}

BoaValue boa_coreutil_tableiterkey(BoaTable* table, int index)
{
    if(table->htcapacity <= index)
    {
        return boa_value_makenull();
    }
    return boa_value_fromobject(table->htentries[index].entkey);
}

BoaValue boa_objfnclass_iterator(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaClass* klass;
    int index;
    int methodscapacity;
    int value;
    bool fields;
    (void)state;
    (void)argc;
    klass = boa_value_asclass(instance);
    index = boa_value_isnull(args[0]) ? -1 : boa_value_asnumber(args[0]);
    methodscapacity = (int)klass->mthtable.htcapacity;
    fields = index >= methodscapacity;
    value = boa_coreutil_tableiterator(fields ? &klass->staticstable : &klass->mthtable, fields ? index - methodscapacity : index);
    if(value == -1)
    {
        if(fields)
        {
            return boa_value_makenull();
        }
        index++;
        fields = true;
        value = boa_coreutil_tableiterator(&klass->staticstable, index - methodscapacity);
    }
    return value == -1 ? boa_value_makenull() : boa_value_makenumber(fields ? value + methodscapacity : value);
}

BoaValue boa_objfnclass_itervalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaClass* klass;
    size_t index;
    size_t methodscapacity;
    bool fields;
    (void)state;
    (void)argc;
    index = boa_value_asnumber(args[0]);
    klass = boa_value_asclass(instance);
    methodscapacity = klass->mthtable.htcapacity;
    fields = index >= methodscapacity;
    return boa_coreutil_tableiterkey(fields ? &klass->staticstable : &klass->mthtable, fields ? index - methodscapacity : index);
}

BoaValue boa_objfnclass_superget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaClass* super;
    (void)state;
    (void)argc;
    (void)args;
    super = NULL;
    if(boa_value_isinstance(instance))
    {
        super = boa_value_asinstance(instance)->klass->super;
    }
    else
    {
        super = boa_value_asclass(instance)->super;
    }
    if(super == NULL)
    {
        return boa_value_makenull();
    }
    return boa_value_fromobject(super);
}

BoaValue boa_objfnclass_subscript(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaClass* klass;
    BoaValue value;

    klass = boa_value_asclass(instance);
    if(argc == 2)
    {
        if(!boa_value_isstring(args[0]))
        {
            return boa_vm_raiseexception(state, state->exceptions.stdexception, "class index must be a string");
        }
        boa_table_set(&klass->staticstable, boa_value_asstring(args[0]), args[1]);
        return args[1];
    }
    if(!boa_value_isstring(args[0]))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "class index must be a string");
    }
    if(boa_table_getentry(&klass->staticstable, boa_value_asstring(args[0]), &value))
    {
        return value;
    }
    if(boa_table_getentry(&klass->mthtable, boa_value_asstring(args[0]), &value))
    {
        return value;
    }
    return boa_value_makenull();
}

BoaValue boa_objfnclass_nameget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return boa_value_fromobject(boa_value_asclass(instance)->name);
}

void boa_objfnutil_tabtoarray(BoaState* state, BoaArray* arr, BoaTable* table)
{
    size_t i;
    BoaTabEntry* entry;
    (void)state;
    for(i = 0; i < (size_t)table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry == NULL)
        {
            return;
        }
        if(entry->entkey != NULL)
        {
            boa_array_push(arr, boa_value_fromobject(entry->entkey));
        }
    }
}

BoaValue boa_objfnobject_keys(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    BoaValue val;
    BoaArray* arr;
    BoaMap* map;
    BoaInstance* oinst;
    (void)thisval;
    if(argc == 0)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "keys() requires an argument");
    }
    val = args[0];
    arr = boa_array_make(state);
    if(boa_value_ismap(val))
    {
        map = boa_value_asmap(val);
        boa_objfnutil_tabtoarray(state, arr, &map->innertable);
    }
    else if(boa_value_isinstance(val))
    {
        oinst = boa_value_asinstance(val);
        boa_objfnutil_tabtoarray(state, arr, &oinst->fields);
    }
    return boa_value_fromobject(arr);
}

BoaValue boa_objfnobject_classget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_value_fromobject(boa_state_getclassfor(state, instance));
}

BoaValue boa_objfnobject_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaStream pr;
    BoaInstance* self;
    BoaString* dest;
    BoaClass* klass;

    (void)argc;
    (void)args;
    self = boa_value_asinstance(instance);
    klass = boa_state_getclassfor(state, instance);
    boa_stream_makestackstring(&pr);
    boa_value_printobjinstance(&pr, klass, self);
    dest = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(dest);
}

BoaValue boa_objfnobject_dump(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaValue dumpme;
    BoaStream pr;
    BoaString* dest;
    (void)argc;
    (void)args;
    dumpme = instance;
    boa_stream_makestackstring(&pr);
    if(argc > 0)
    {
        dumpme = args[0];
    }
    boa_value_printvalue(&pr, dumpme, true);
    dest = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(dest);
}

BoaValue boa_objfnobject_iscallable(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makebool(boa_value_iscallablefunction(instance));
}

BoaValue boa_objfnobject_subscript(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* tgname;
    BoaInstance* inst;
    BoaValue value;
    if(!boa_value_isinstance(instance))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot modify built-in type '%s'", boa_value_valtypename(instance));
    }
    inst = boa_value_asinstance(instance);
    if(!boa_value_isstring(args[0]))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "object index must be a string, but got %s instead", boa_value_valtypename(args[0]));
    }
    tgname = boa_value_asstring(args[0]);
    if(argc == 2)
    {
        boa_instance_setfield(inst, tgname, args[1]);
        return args[1];
    }
    if(boa_table_getentry(&inst->fields, tgname, &value))
    {
        return value;
    }
    if(boa_table_getentry(&inst->klass->staticstable, tgname, &value))
    {
        return value;
    }
    if(boa_table_getentry(&inst->klass->mthtable, tgname, &value))
    {
        return value;
    }
    return boa_value_makenull();
}

BoaValue boa_objfnobject_iterator(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int index;
    int value;
    BoaInstance* self;
    (void)state;
    (void)argc;
    if(!boa_value_isinstance(instance))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot iterate non-instance type %s", boa_value_valtypename(args[0]));
    }
    self = boa_value_asinstance(instance);
    index = -1;
    if(!boa_value_isnull(args[0]))
    {
        index = boa_value_asnumber(args[0]);
    }
    value = boa_coreutil_tableiterator(&self->fields, index);
    if(value == -1)
    {
        return boa_value_makenull();
    }
    return boa_value_makenumber(value);
}

BoaValue boa_objfnobject_itervalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t index;
    BoaInstance* self;
    (void)state;
    (void)argc;
    index = boa_value_asnumber(args[0]);
    self = boa_value_asinstance(instance);
    return boa_coreutil_tableiterkey(&self->fields, index);
}

BoaValue boa_objfnnumber_constructor_actual(BoaState* state, BoaValue instance, size_t argc, BoaValue* args, bool isatoi)
{
    BoaValue res;
    BoaValue arg;
    BoaAstLexer lexer;
    BoaAstToken tok;
    BoaString* strv;
    const char* cstr;
    (void)instance;
    if((argc > 0) || isatoi)
    {
        arg = args[0];
        if(boa_value_isnumber(arg))
        {
            return arg;
        }
        else if(boa_value_isstring(arg))
        {
            strv = boa_value_asstring(arg);
            cstr = boa_string_getdata(strv);
            boa_astlex_init(state, &lexer, "Number.constructor", cstr);
            tok = boa_astlex_scannumber(&lexer);
            if(tok.type == BOA_ASTTOKTYP_NUMBER)
            {
                res = tok.tokvalue;
                return res;
            }
            else
            {
                return boa_vm_raiseexception(state, state->exceptions.stdexception, "Number() failed to parse '%s': %s", cstr, tok.start);
            }
        }
        else
        {
            return boa_vm_raiseexception(state, state->exceptions.stdexception, "Number() expects either a number or a string");
        }
    }
    return boa_value_makenumber(0);
}

BoaValue boa_objfnnumber_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    return boa_objfnnumber_constructor_actual(state, instance, argc, args, false);
}

BoaValue boa_objfnnumber_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_string_numbertostring(state, boa_value_asnumber(instance));
}

BoaValue boa_objfnnumber_chrget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    char c;
    BoaNumber dn;
    BoaString* cs;
    (void)args;
    (void)argc;
    dn = boa_value_asnumber(instance);
    c = dn;
    cs = boa_string_copylen(state, &c, 1);
    return boa_value_fromobject(cs);
}

BoaValue boa_objfnbool_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_value_fromobject(boa_value_asbool(instance) ? state->strings.strtrue : state->strings.strfalse);
}

BoaValue boa_objfnstring_chr(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    char ch;
    BoaString* res;
    (void)instance;
    (void)argc;
    if(argc == 1)
    {
        ch = boa_value_asnumber(args[0]);
        return boa_value_fromobject(boa_string_copylen(state, &ch, 1));
    }
    res = boa_string_copylen(state, "", 0);
    for(i=0; i<argc; i++)
    {
        ch = boa_value_asnumber(args[i]);
        boa_string_appendbyte(res, ch);
    }
    return boa_value_fromobject(res);
}

BoaValue boa_objfnstring_utf8encode(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int incode;
    size_t len;
    BoaString* res;
    char* buf;
    (void)instance;
    (void)argc;
    incode = boa_value_asnumber(args[0]);
    buf = boa_util_utfstrencode(incode, &len);
    res = boa_string_take(state, buf, len);
    return boa_value_fromobject(res);
}

static BoaValue boa_util_stringutf8chars(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args, bool onlycodepoint)
{
    int cp;
    bool havemax;
    size_t counter;
    size_t maxamount;
    const char* cstr;
    BoaArray* res;
    BoaString* os;
    BoaString* instr;
    BoaUTF8Iterator iter;
    havemax = false;
    instr = boa_value_asstring(thisval);
    if(argc > 0)
    {
        havemax = true;
        maxamount = boa_value_asnumber(args[0]);
    }
    res = boa_array_make(state);
    boa_utf8iter_init(&iter, boa_string_getdata(instr), boa_string_getlength (instr));
    counter = 0;
    while(boa_utf8iter_next(&iter))
    {
        cp = iter.codepoint;
        cstr = boa_utf8iter_getchar(&iter);
        counter++;
        if(havemax)
        {
            if(counter == maxamount)
            {
                goto finalize;
            }
        }
        if(onlycodepoint)
        {
            boa_array_push(res, boa_value_makenumber(cp));
        }
        else
        {
            os = boa_string_copylen(state, cstr, iter.charsize);
            boa_array_push(res, boa_value_fromobject(os));
        }
    }
    finalize:
    return boa_value_fromobject(res);
}

static BoaValue boa_objfnstring_utf8chars(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    return boa_util_stringutf8chars(state, thisval, argc, args, false);
}

static BoaValue boa_objfnstring_utf8codepoints(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    return boa_util_stringutf8chars(state, thisval, argc, args, true);
}

BoaValue boa_objfnstring_plus(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* res;
    BoaString* self;
    BoaStream pr;
    (void)argc;
    boa_stream_makestackstring(&pr);
    self = boa_value_asstring(instance);
    boa_stream_putlen(&pr, boa_string_getdata(self), boa_string_getlength(self));
    boa_value_printvalue(&pr, args[0], false);
    res = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(res);
}

BoaValue boa_objfnstring_lessthan(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t olen;
    size_t selflen;
    size_t otherlen;
    const char* selfstr;
    const char* otherstr;
    BoaString* self;
    BoaString* other;
    (void)state;
    (void)argc;
    self = boa_value_asstring(instance);
    other = boa_value_asstring(args[0]);
    selflen = boa_string_getlength(self);
    otherlen = boa_string_getlength(other);
    selfstr = boa_string_getdata(self);
    otherstr = boa_string_getdata(other);
    if(((selflen > 0) && (otherlen > 0)) && (selfstr != NULL && otherstr != NULL))
    {
        olen = selflen;
        if(selflen > otherlen)
        {
            olen = otherlen;
        }
        if(otherlen == 1 && selflen == 1)
        {
            return boa_value_makebool(selfstr[0] < otherstr[0]);
        }
        return boa_value_makebool(strncmp(selfstr, otherstr, olen) < 0);
    }
    return boa_value_makebool(false);
}

BoaValue boa_objfnstring_greaterthan(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t olen;
    size_t selflen;
    size_t otherlen;
    const char* selfstr;
    const char* otherstr;
    BoaString* self;
    BoaString* other;
    (void)state;
    (void)argc;
    self = boa_value_asstring(instance);
    other = boa_value_asstring(args[0]);
    selflen = boa_string_getlength(self);
    otherlen = boa_string_getlength(other);
    selfstr = boa_string_getdata(self);
    otherstr = boa_string_getdata(other);
    if(((selflen > 0) && (otherlen > 0)) && (selfstr != NULL && otherstr != NULL))
    {
        olen = selflen;
        if(selflen > otherlen)
        {
            olen = otherlen;
        }
        if(otherlen == 1 && selflen == 1)
        {
            return boa_value_makebool(selfstr[0] > otherstr[0]);
        }
        return boa_value_makebool(strncmp(selfstr, otherstr, olen) > 0);
    }
    return boa_value_makebool(false);
}

BoaValue boa_objfnstring_greaterequal(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t olen;
    size_t selflen;
    size_t otherlen;
    const char* selfstr;
    const char* otherstr;
    BoaString* self;
    BoaString* other;
    (void)state;
    (void)argc;
    self = boa_value_asstring(instance);
    other = boa_value_asstring(args[0]);
    selflen = boa_string_getlength(self);
    otherlen = boa_string_getlength(other);
    selfstr = boa_string_getdata(self);
    otherstr = boa_string_getdata(other);
    if(((selflen > 0) && (otherlen > 0)) && (selfstr != NULL && otherstr != NULL))
    {
        olen = selflen;
        if(selflen > otherlen)
        {
            olen = otherlen;
        }
        if(otherlen == 1 && selflen == 1)
        {
            return boa_value_makebool(selfstr[0] >= otherstr[0]);
        }
        return boa_value_makebool(strncmp(selfstr, otherstr, olen) >= 0);
    }
    return boa_value_makebool(false);
}

BoaValue boa_objfnstring_lessequal(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t olen;
    size_t selflen;
    size_t otherlen;
    const char* selfstr;
    const char* otherstr;
    BoaString* self;
    BoaString* other;
    (void)state;
    (void)argc;
    self = boa_value_asstring(instance);
    other = boa_value_asstring(args[0]);
    selflen = boa_string_getlength(self);
    otherlen = boa_string_getlength(other);
    selfstr = boa_string_getdata(self);
    otherstr = boa_string_getdata(other);
    if(((selflen > 0) && (otherlen > 0)) && (selfstr != NULL && otherstr != NULL))
    {
        olen = selflen;
        if(selflen > otherlen)
        {
            olen = otherlen;
        }
        if(otherlen == 1 && selflen == 1)
        {
            return boa_value_makebool(selfstr[0] <= otherstr[0]);
        }
        return boa_value_makebool(strncmp(selfstr, otherstr, olen) <= 0);
    }
    return boa_value_makebool(false);
}

BoaValue boa_objfnstring_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return instance;
}

BoaValue boa_objfnstring_tonumber(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaNumber result;
    (void)state;
    (void)argc;
    (void)args;
    result = strtod(boa_string_getdata(boa_value_asstring(instance)), NULL);
    if(errno == ERANGE)
    {
        errno = 0;
        return boa_value_makenull();
    }
    return boa_value_makenumber(result);
}

BoaValue boa_objfnstring_touppercase(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* selfstr;
    BoaString* copied;
    (void)argc;
    (void)args;
    selfstr = boa_value_asstring(instance);
    copied = boa_string_clone(state, selfstr);
    boa_util_strchangecase(boa_string_getdata(copied), boa_string_getlength(copied), boa_string_getdata(copied), boa_util_chartoupper);
    return boa_value_fromobject(copied);
}

BoaValue boa_objfnstring_tolowercase(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* selfstr;
    BoaString* copied;
    (void)argc;
    (void)args;
    selfstr = boa_value_asstring(instance);
    copied = boa_string_clone(state, selfstr);
    boa_util_strchangecase(boa_string_getdata(copied), boa_string_getlength(copied), boa_string_getdata(copied), boa_util_chartolower);
    return boa_value_fromobject(copied);
}

BoaValue boa_objfnstring_contains(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* sub;
    BoaString* selfstr;
    (void)state;
    (void)argc;
    selfstr = boa_value_asstring(instance);
    sub = boa_value_asstring(args[0]);
    if(sub == selfstr)
    {
        return boa_value_makebool(true);
    }
    return boa_value_makebool(strstr(boa_string_getdata(selfstr), boa_string_getdata(sub)) != NULL);
}

BoaValue boa_objfnstring_startswith(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaString* sub;
    BoaString* selfstr;
    (void)state;
    (void)argc;
    selfstr = boa_value_asstring(instance);
    sub = boa_value_asstring(args[0]);
    if(sub == selfstr)
    {
        return boa_value_makebool(true);
    }
    if(boa_string_getlength(sub) > boa_string_getlength(selfstr))
    {
        return boa_value_makebool(false);
    }
    for(i = 0; i < boa_string_getlength(sub); i++)
    {
        if(boa_string_getat(sub, i) != boa_string_getat(selfstr, i))
        {
            return boa_value_makebool(false);
        }
    }
    return boa_value_makebool(true);
}

BoaValue boa_objfnstring_endswith(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    size_t start;
    BoaString* sub;
    BoaString* selfstr;
    (void)state;
    (void)argc;
    selfstr = boa_value_asstring(instance);
    sub = boa_value_asstring(args[0]);
    if(sub == selfstr)
    {
        return boa_value_makebool(true);
    }
    if(boa_string_getlength(sub) > boa_string_getlength(selfstr))
    {
        return boa_value_makebool(false);
    }
    start = boa_string_getlength(selfstr) - boa_string_getlength(sub);
    for(i = 0; i < boa_string_getlength(sub); i++)
    {
        if(boa_string_getat(sub, i) != boa_string_getat(selfstr, i + start))
        {
            return boa_value_makebool(false);
        }
    }
    return boa_value_makebool(true);
}

BoaValue boa_objfnstring_replace(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* with;
    BoaString* what;
    BoaString* clone;
    BoaString* selfstr;
    (void)argc;
    if(!boa_value_isstring(args[0]) || !boa_value_isstring(args[1]))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "expected 2 string arguments");
    }
    selfstr = boa_value_asstring(instance);
    what = boa_value_asstring(args[0]);
    with = boa_value_asstring(args[1]);
    clone = boa_string_makeemptystring(state, 0, false);
    boa_strbuf_fullreplace(&selfstr->strbuf, &clone->strbuf, boa_string_getdata(what), boa_string_getlength(what), boa_string_getdata(with), boa_string_getlength(with));
    return boa_value_fromobject(clone);
}

BoaValue boa_objfnstring_splice(BoaState* state, BoaString* string, int64_t from, int64_t to)
{
    int64_t length;
    length = boa_string_utflength(string);
    if(from < 0)
    {
        from = length + from;
    }
    if(to < 0)
    {
        to = length + to;
    }
    from = fmax(from, 0);
    to = fmin(to, length - 1);
    if(from > to)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "String.splice 'from' (%ld) is larger than 'to' (%ld)", from, to);
    }
    from = boa_util_utfstrfindoffset(boa_string_getdata(string), from);
    to = boa_util_utfstrfindoffset(boa_string_getdata(string), to);
    return boa_value_fromobject(boa_string_fromrange(state, string, from, to - from + 1));
}

BoaString* dosubstring(BoaState* state, BoaString* selfstr, size_t start, size_t end, bool likejs)
{
    size_t asz;
    size_t len;
    size_t tmp;
    size_t maxlen;
    char* raw;
    (void)likejs;
    maxlen = boa_string_getlength(selfstr);
    len = maxlen;
    if(end > maxlen)
    {
        tmp = start;
        start = end;
        end = tmp;
        len = maxlen;
    }
    if(end < start)
    {
        tmp = end;
        end = start;
        start = tmp;
        len = end;
    }
    len = (end - start);
    if(len > maxlen)
    {
        len = maxlen;
    }
    asz = ((end + 1) * sizeof(char));
    raw = (char*)boa_sysmem_malloc(sizeof(char) * asz);
    memset(raw, 0, asz);
    memcpy(raw, boa_string_getdata(selfstr) + start, len);
    return boa_string_take(state, raw, len);
}

BoaValue boa_objfnstring_substring(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    size_t end;
    size_t start;
    size_t maxlen;
    BoaString* nos;
    BoaString* selfstr;
    selfstr = boa_value_asstring(thisval);
    maxlen = boa_string_getlength(selfstr);
    end = maxlen;
    start = boa_value_asnumber(args[0]);
    if(argc > 1)
    {
        end = boa_value_asnumber(args[1]);
    }
    nos = dosubstring(state, selfstr, start, end, true);
    return boa_value_fromobject(nos);
}

BoaValue boa_objfnstring_indexof(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    char findme;
    size_t i;
    BoaValue vfind;
    BoaString* tmp;
    BoaString* selfstr;
    (void)state;
    (void)argc;
    selfstr = boa_value_asstring(thisval);
    findme = -1;
    vfind = args[0];
    if(boa_value_isnumber(vfind))
    {
        findme = boa_value_asnumber(vfind);
    }
    else if(boa_value_isstring(vfind))
    {
        tmp = boa_value_asstring(vfind);
        findme = boa_string_getat(tmp, 0);
    }
    else
    {
        return boa_value_makenumber(-1);
    }
    for(i=0; i<boa_string_getlength(selfstr); i++)
    {
        if(boa_string_getat(selfstr, i) == findme)
        {
            return boa_value_makenumber(i);
        }
    }
    return boa_value_makenumber(-1);
}

BoaValue boa_objfnstring_charcodeat(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    int cp;
    int index;
    BoaString* selfstr;
    selfstr = boa_value_asstring(thisval);
    index = boa_value_asnumber(args[0]);
    if(argc != 1)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = boa_string_utflength(selfstr) + index;
        if(index < 0)
        {
            return boa_value_makenull();
        }
    }
    cp = boa_string_codepointcodeat(state, selfstr, boa_util_utfstrfindoffset(boa_string_getdata(selfstr), index));
    return boa_value_makenumber(cp);
}

BoaValue boa_objfnstring_charat(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    int index;
    BoaString* c;
    BoaString* selfstr;
    if(boa_value_isrange(args[0]))
    {
        BoaRange* range = boa_value_asrange(args[0]);
        return boa_objfnstring_splice(state, boa_value_asstring(thisval), range->from, range->to);
    }
    selfstr = boa_value_asstring(thisval);
    index = boa_value_asnumber(args[0]);
    if(argc != 1)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = boa_string_utflength(selfstr) + index;
        if(index < 0)
        {
            return boa_value_fromobject(state->strings.strempty);
        }
    }
    c = boa_string_codepointstringat(state, selfstr, boa_util_utfstrfindoffset(boa_string_getdata(selfstr), index));
    if(c == NULL)
    {
        return boa_value_fromobject(state->strings.strempty);
    }
    return boa_value_fromobject(c);
}

BoaValue boa_objfnstring_subscript(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    int index;
    BoaString* c;
    BoaString* selfstr;
    if(boa_value_isrange(args[0]))
    {
        BoaRange* range = boa_value_asrange(args[0]);
        return boa_objfnstring_splice(state, boa_value_asstring(thisval), range->from, range->to);
    }
    selfstr = boa_value_asstring(thisval);
    index = boa_value_asnumber(args[0]);
    if(argc != 1)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = boa_string_utflength(selfstr) + index;
        if(index < 0)
        {
            return boa_value_makenull();
        }
    }
    c = boa_string_codepointstringat(state, selfstr, boa_util_utfstrfindoffset(boa_string_getdata(selfstr), index));
    return c == NULL ? boa_value_makenull() : boa_value_fromobject(c);
}

BoaValue boa_objfnstring_split(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    size_t i;
    size_t end;
    size_t start;
    size_t length;
    bool havedelim;
    char ch;
    BoaArray* list;
    BoaString* selfstr;
    BoaString* delimeter;
    havedelim = false;
    selfstr = boa_value_asstring(thisval);
    if(argc > 0)
    {
        havedelim = true;
        delimeter = boa_value_asstring(args[0]);
        if(boa_string_getlength(delimeter) == 0)
        {
            havedelim = false;
        }
    }
    list = boa_array_make(state);
    if(!havedelim)
    {
        end = boa_string_getlength(selfstr);
        for(i=0; i<end; i++)
        {
            ch = boa_string_getat(selfstr, i);
            boa_array_push(list, boa_value_fromobject(boa_string_copylen(state, &ch, 1)));
        }
        return boa_value_fromobject(list);
    }
    /* empty string matches empty string to empty list */
    if(((boa_string_getlength(selfstr) == 0) && (boa_string_getlength(delimeter) == 0)) || (boa_string_getlength(selfstr) == 0) || (boa_string_getlength(delimeter) == 0))
    {
        return boa_value_fromobject(list);
    }
    if(boa_string_getlength(delimeter) > 0)
    {
        start = 0;
        for(i = 0; i <= boa_string_getlength(selfstr); i++)
        {
            /* match found. */
            if(memcmp(boa_string_getdata(selfstr) + i, boa_string_getdata(delimeter), boa_string_getlength(delimeter)) == 0 || i == boa_string_getlength(selfstr))
            {
                boa_array_push(list, boa_value_fromobject(boa_string_copylen(state, boa_string_getdata(selfstr) + start, i - start)));
                i += boa_string_getlength(delimeter) - 1;
                start = i + 1;
            }
        }
    }
    else
    {
        length = boa_string_getlength(selfstr);
        for(i = 0; i < length; i++)
        {
            start = i;
            end = i + 1;
            boa_array_push(list, boa_value_fromobject(boa_string_copylen(state, boa_string_getdata(selfstr) + start, (int)(end - start))));
        }
    }
    return boa_value_fromobject(list);
}

BoaValue boa_objfnstring_lengthget(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_string_utflength(boa_value_asstring(thisval)));
}

BoaValue boa_objfnstring_iterator(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    int index;
    BoaString* selfstr;
    (void)state;
    (void)argc;
    selfstr = boa_value_asstring(thisval);
    if(boa_value_isnull(args[0]))
    {
        if(boa_string_getlength(selfstr) == 0)
        {
            return boa_value_makenull();
        }
        return boa_value_makenumber(0);
    }
    index = boa_value_asnumber(args[0]);
    if(index < 0)
    {
        return boa_value_makenull();
    }
    do
    {
        index++;
        if(index >= (int)boa_string_getlength(selfstr))
        {
            return boa_value_makenull();
        }
    } while((boa_string_getat(selfstr, index) & 0xc0) == 0x80);
    return boa_value_makenumber(index);
}

BoaValue boa_objfnstring_itervalue(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    uint32_t index;
    BoaString* selfstr;
    (void)argc;
    selfstr = boa_value_asstring(thisval);
    index = boa_value_asnumber(args[0]);
    if(index == UINT32_MAX)
    {
        return boa_value_makebool(false);
    }
    return boa_value_fromobject(boa_string_codepointstringat(state, selfstr, index));
}

static BoaValue boa_objfnstring_trim(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    const char* trimmer;
    BoaString* selfstr;
    BoaString* newstr;
    trimmer = "\r\n\t ";
    if(argc == 1)
    {
        trimmer = boa_string_getdata(boa_value_asstring(args[0]));
    }
    selfstr = boa_value_asstring(thisval);
    newstr = boa_string_copylen(state, boa_string_getdata(selfstr), boa_string_getlength(selfstr));
    boa_strbuf_triminplace(boa_string_getstrbuf(newstr), trimmer);
    return boa_value_fromobject(newstr);
}

static BoaValue boa_objfnstring_ltrim(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    const char* trimmer;
    BoaString* newstr;
    BoaString* selfstr;
    trimmer = "\r\n\t ";
    if(argc == 1)
    {
        trimmer = boa_string_getdata(boa_value_asstring(args[0]));
    }
    selfstr = boa_value_asstring(thisval);
    newstr = boa_string_copylen(state, boa_string_getdata(selfstr), boa_string_getlength(selfstr));
    boa_strbuf_trimleftinplace(boa_string_getstrbuf(newstr), trimmer);
    return boa_value_fromobject(newstr);
}

static BoaValue boa_objfnstring_rtrim(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    const char* trimmer;
    BoaString* newstr;
    BoaString* selfstr;
    trimmer = "\r\n\t ";
    if(argc == 1)
    {
        trimmer = boa_string_getdata(boa_value_asstring(args[0]));
    }
    selfstr = boa_value_asstring(thisval);
    newstr = boa_string_copylen(state, boa_string_getdata(selfstr), boa_string_getlength(selfstr));
    boa_strbuf_trimrightinplace(boa_string_getstrbuf(newstr), trimmer);
    return boa_value_fromobject(newstr);
}

BoaValue boa_objfnfunction_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_function_getname(state, instance);
}

BoaValue boa_objfnfunction_nameget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_function_getname(state, instance);
}

BoaValue boa_objfnfiber_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaFiber* fiber;
    BoaValue arg;
    BoaModule* module;
    (void)instance;
    if((argc == 0) || (!boa_value_iscallablefunction(args[0])))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "Fiber constructor expects a function as its argument");
        return boa_value_makenull();
    }
    arg = args[0];
    module = state->vmstate.fiber->module;
    if(boa_value_isfuncscript(arg))
    {
        fiber = boa_object_makefiber(state, module, boa_value_asfuncscript(arg));
    }
    else
    {
        fiber = boa_object_makefiberclosure(state, module, boa_value_asfuncclosure(arg));
    }
    fiber->parent = state->vmstate.fiber;
    return boa_value_fromobject(fiber);
}

bool boa_coreutil_isfiberdone(BoaFiber* fiber)
{
    return fiber->framecount == 0 || fiber->muststop;
}

BoaValue boa_objfnfiber_doneget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makebool(boa_coreutil_isfiberdone(boa_value_asfiber(instance)));
}

BoaValue boa_objfnfiber_errorget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_asfiber(instance)->error;
}

BoaValue boa_objfnfiber_currentget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_fromobject(state->vmstate.fiber);
}

void boa_coreutil_runfiber(BoaState* state, BoaFiber* fiber, BoaValue* args, size_t argc, bool catcher)
{
    size_t i;
    size_t ai;
    size_t varargcount;
    size_t functionargcount;
    bool vararg;
    BoaCallFrame* frame;
    BoaFuncScript* function;
    BoaValue* start;
    BoaArray* array;

    if(boa_coreutil_isfiberdone(fiber))
    {
        boa_vm_raisefatalerror(state, "Fiber already finished executing");
    }
    fiber->parent = state->vmstate.fiber;
    fiber->catcher = catcher;
    state->vmstate.fiber = fiber;
    frame = &fiber->framevals[fiber->framecount - 1];
    if(frame->ip == frame->function->chunk.compiledcodechunk)
    {
        fiber->argcount = argc;
        function = frame->function;
        start = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
        boa_fiber_ensureregisters(fiber, start - fiber->registeritems + function->maxregisters);
        frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
        for(i = argc + 1; i < function->maxregisters; i++)
        {
            frame->slots[i] = boa_value_makenull();
        }
        frame->slots[0] = boa_value_fromobject(function);
        for(ai = 0; ai < argc; ai++)
        {
            frame->slots[ai + 1] = args[ai];
        }
        vararg = frame->function->vararg;
        functionargcount = function->argcount;
        fiber->argcount = functionargcount;
        if(vararg)
        {
            if(functionargcount == argc && boa_value_isvargarray(*(frame->slots + functionargcount)))
            {
                /* no need to repack the arguments */
            }
            else
            {
                array = &boa_object_makevararray(state)->innerarray;
                boa_state_pushroot(state, (BoaObject*)array);
                *(frame->slots + functionargcount) = boa_value_fromobject(array);
                varargcount = argc - functionargcount + 1;
                if(varargcount > 0)
                {
                    boa_dynlistval_ensuresize(&array->innerlist, varargcount);
                    for(i = 0; i < varargcount; i++)
                    {
                        array->innerlist.listitems[i] = args[i + functionargcount - 1];
                    }
                }
                boa_state_poproot(state);
            }
        }
    }
    if(BOA_UNLIKELY(state->config.traceexecution))
    {
        fprintf(stderr, "fiber start:\n");
    }
}

BoaValue boa_objfnfiber_run(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    boa_coreutil_runfiber(state, boa_value_asfiber(instance), args, argc, false);
    return boa_value_makenull();
}

BoaValue boa_objfnfiber_try(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    boa_coreutil_runfiber(state, boa_value_asfiber(instance), args, argc, true);
    return boa_value_makenull();
}

BoaValue boa_objfnfiber_yield(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    if(state->vmstate.fiber->parent == NULL)
    {
        boa_vm_handleerror(state, argc == 0 ? boa_value_fromobject(boa_string_copy(state, "Fiber was yielded")) : args[0]);
        return boa_value_makenull();
    }
    state->vmstate.fiber = state->vmstate.fiber->parent;
    *state->vmstate.fiber->returnaddress = argc == 0 ? boa_value_makenull() : boa_value_fromobject(boa_value_tostring(state, args[0], 0));
    return boa_value_makenull();
}

BoaValue boa_objfnfiber_abort(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaValue value;
    (void)instance;
    value = argc == 0 ? boa_value_fromobject(boa_string_copy(state, "Fiber was aborted")) : args[0];
    boa_vm_handleerror(state, value);
    if(state->vmstate.fiber->returnaddress != NULL)
    {
        *state->vmstate.fiber->returnaddress = value;
    }
    return boa_value_makenull();
}

BoaValue boa_objfnmodule_privatesget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaModule* module;
    BoaMap* map;
    (void)argc;
    (void)args;
    module = boa_value_ismodule(instance) ? boa_value_asmodule(instance) : state->vmstate.fiber->module;
    map = module->privatenames;
    return boa_value_fromobject(map);
}

BoaValue boa_objfnmodule_currentget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_fromobject(state->vmstate.fiber->module);
}

BoaValue boa_objfnmodule_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    return boa_string_valformat(state, "Module @", boa_value_fromobject(boa_value_asmodule(instance)->name));
}

BoaValue boa_objfnmodule_nameget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return boa_value_fromobject(boa_value_asmodule(instance)->name);
}

BoaValue boa_objfnarray_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    size_t count;
    BoaValue fill;
    BoaArray* arr;
    (void)instance;
    (void)argc;
    (void)args;
    arr = boa_array_make(state);
    if(argc > 0)
    {
        count = boa_value_asnumber(args[0]);
        fill = boa_value_makenull();
        if(argc > 1)
        {
            fill = args[1];
        }
        for(i = 0; i < count; i++)
        {
            boa_array_push(arr, fill);
        }
    }
    return boa_value_fromobject(arr);
}

BoaValue boa_objfnarray_splice(BoaState* state, BoaArray* array, int64_t from, int64_t to)
{
    size_t i;
    size_t length;
    BoaArray* newarray;
    length = array->innerlist.listcount;
    if(from < 0)
    {
        from = (int64_t)length + from;
    }
    if(to < 0)
    {
        to = (int64_t)length + to;
    }
    #if 0
    if(from > to)
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "Array.splice 'from' (%ld) is larger than 'to' (%ld)", from, to);
        return boa_value_makenull();
    }
    #endif
    from = fmax(from, 0);
    to = fmin(to, (int)length - 1);
    length = fmin(length, to - from + 1);
    newarray = boa_array_make(state);
    for(i = 0; i < length; i++)
    {
        boa_dynlistval_push(&newarray->innerlist, array->innerlist.listitems[from + i]);
    }
    return boa_value_fromobject(newarray);
}

BoaValue boa_objfnarray_slice(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int from;
    int to;
    BoaArray* selfarr;
    selfarr = boa_value_asarray(instance);
    to = boa_array_size(selfarr);
    from = boa_value_asnumber(args[0]);
    if(from < 0)
    if(argc > 1)
    {
        to = boa_value_asnumber(args[1]);
    }
    return boa_objfnarray_splice(state, selfarr, from, to);
}

BoaValue boa_objfnarray_subscript(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t asz;
    int64_t actualidx;
    int64_t index;
    BoaRange* range;
    BoaArray* arr;
    if(!boa_value_isarray(instance))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected array, got a %s instead", boa_value_valtypename(args[0]));
        return boa_value_makenull();
    }
    if(argc == 2)
    {
        if(!boa_value_isnumber(args[0]))
        {
            boa_vm_raiseexception(state, state->exceptions.stdexception, "array index must be a number, got a %s instead", boa_value_valtypename(args[0]));
            return boa_value_makenull();
        }
        arr = boa_value_asarray(instance);
        index = boa_value_asnumber(args[0]);
        if(index < 0)
        {
            index = fmax(0, boa_array_size(arr) + index);
        }
        boa_array_ensuresize(arr, index + 1);
        return boa_array_set(arr, index, args[1]);
    }
    if(!boa_value_isnumber(args[0]))
    {
        if(boa_value_isrange(args[0]))
        {
            range = boa_value_asrange(args[0]);
            return boa_objfnarray_splice(state, boa_value_asarray(instance), (int)range->from, (int)range->to);
        }
        boa_vm_raiseexception(state, state->exceptions.stdexception, "array index must be a number, got a %s instead", boa_value_valtypename(args[0]));
        return boa_value_makenull();
    }
    arr = boa_value_asarray(instance);
    asz = boa_array_size(arr);
    actualidx = boa_value_asnumber(args[0]);
    if(actualidx < 0)
    {
        actualidx = fmax(0, asz + actualidx);
    }
    if((size_t)actualidx >= asz)
    {
        return boa_value_makenull();
    }
    return boa_array_get(arr, actualidx);
}

BoaValue boa_objfnarray_push(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaArray* self;
    (void)state;
    self = boa_value_asarray(instance);
    for(i = 0; i < argc; i++)
    {
        boa_dynlistval_push(&self->innerlist, args[i]);
    }
    return boa_value_makenull();
}

BoaValue boa_objfnarray_insert(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int i;
    int index;
    BoaValue value;
    BoaDynListVal* list;
    (void)state;
    (void)argc;
    list = &boa_value_asarray(instance)->innerlist;
    index = boa_value_asnumber(args[0]);
    if(index < 0)
    {
        index = fmax(0, list->listcount + index);
    }
    value = args[1];
    if((int)list->listcount <= index)
    {
        boa_dynlistval_ensuresize(list, index + 1);
    }
    else
    {
        boa_dynlistval_ensuresize(list, list->listcount + 1);
        for(i = list->listcount - 1; i > index; i--)
        {
            list->listitems[i] = list->listitems[i - 1];
        }
    }
    list->listitems[index] = value;
    return boa_value_makenull();
}

BoaValue boa_objfnarray_addall(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaArray* array;
    BoaArray* toadd;
    (void)argc;
    if(!boa_value_isarray(args[0]))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected array as the argument");
        return boa_value_makenull();
    }
    array = boa_value_asarray(instance);
    toadd = boa_value_asarray(args[0]);
    for(i = 0; i < toadd->innerlist.listcount; i++)
    {
        boa_dynlistval_push(&array->innerlist, toadd->innerlist.listitems[i]);
    }
    return boa_value_makenull();
}

int boa_coreutil_indexof(BoaState* state, BoaArray* array, BoaValue value)
{
    size_t i;
    BoaValue* ptr;
    for(i = 0; i < array->innerlist.listcount; i++)
    {
        ptr = &array->innerlist.listitems[i];
        if(boa_value_compare(state, *ptr, value))
        {
            return (int)i;
        }
    }
    return -1;
}

BoaValue boa_objfnarray_indexof(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int index;
    (void)argc;
    index = boa_coreutil_indexof(state, boa_value_asarray(instance), args[0]);
    return boa_value_makenumber(index);
}

BoaValue boa_coreutil_removeat(BoaArray* array, size_t index)
{
    size_t i;
    size_t count;
    BoaValue value;
    BoaDynListVal* list;

    list = &array->innerlist;
    count = list->listcount;
    if(index >= count)
    {
        return boa_value_makenull();
    }
    value = list->listitems[index];
    if(index == count - 1)
    {
        list->listitems[index] = boa_value_makenull();
    }
    else
    {
        for(i = index; i < list->listcount - 1; i++)
        {
            list->listitems[i] = list->listitems[i + 1];
        }
        list->listitems[count - 1] = boa_value_makenull();
    }
    list->listcount--;
    return value;
}

BoaValue boa_objfnarray_remove(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaArray* array;
    int index;
    (void)argc;
    array = boa_value_asarray(instance);
    index = boa_coreutil_indexof(state, array, args[0]);
    if(index != -1)
    {
        return boa_coreutil_removeat(array, (size_t)index);
    }
    return boa_value_makenull();
}

BoaValue boa_objfnarray_pop(BoaState* state, BoaValue thisval, size_t argc, BoaValue* args)
{
    int index;
    BoaValue val;
    BoaArray* ary;
    (void)state;
    (void)argc;
    (void)args;
    ary = boa_value_asarray(thisval);
    if(boa_array_count(ary) == 0)
    {
        return boa_value_makenull();
    }
    index = boa_array_count(ary) - 1;
    val = boa_array_get(ary, index);
    boa_array_removeat(ary, (size_t)index);
    return val;
}

BoaValue boa_objfnarray_removeat(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int index;
    (void)state;
    (void)argc;
    index = boa_value_asnumber(args[0]);
    if(index < 0)
    {
        return boa_value_makenull();
    }
    return boa_coreutil_removeat(boa_value_asarray(instance), (size_t)index);
}

BoaValue boa_objfnarray_contains(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    return boa_value_makebool(boa_coreutil_indexof(state, boa_value_asarray(instance), args[0]) != -1);
}

BoaValue boa_objfnarray_clear(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    boa_value_asarray(instance)->innerlist.listcount = 0;
    return boa_value_makenull();
}

BoaValue boa_objfnarray_iterator(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaArray* array;
    int number;
    (void)state;
    (void)argc;
    array = boa_value_asarray(instance);
    number = 0;
    if(boa_value_isnumber(args[0]))
    {
        number = boa_value_asnumber(args[0]);
        if(number >= (int)array->innerlist.listcount - 1)
        {
            return boa_value_makenull();
        }
        number++;
    }
    return array->innerlist.listcount == 0 ? boa_value_makenull() : boa_value_makenumber(number);
}

BoaValue boa_objfnarray_itervalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t index;
    BoaDynListVal* list;
    (void)state;
    (void)argc;
    index = boa_value_asnumber(args[0]);
    list = &boa_value_asarray(instance)->innerlist;
    if(list->listcount <= index)
    {
        return boa_value_makenull();
    }
    return list->listitems[index];
}

BoaValue boa_objfnarray_foreach(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaValue callback;
    BoaDynListVal* list;
    (void)argc;
    callback = args[0];
    if(!boa_value_iscallablefunction(callback))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected a function as the callback");
        return boa_value_makenull();
    }
    list = &boa_value_asarray(instance)->innerlist;
    for(i = 0; i < list->listcount; i++)
    {
        boa_state_callvalue(state, state->strings.strcallbackforeach, callback, &list->listitems[i], 1);
    }
    return boa_value_makenull();
}

BoaValue boa_objfnarray_map(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    size_t asz;
    BoaValue val;
    BoaValue callback;
    BoaResult res;
    BoaArray* selfarr;
    BoaArray* narr;
    (void)argc;
    callback = args[0];
    if(!boa_value_iscallablefunction(callback))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected a function as the callback");
        return boa_value_makenull();
    }
    selfarr = boa_value_asarray(instance);
    asz = boa_array_size(selfarr);
    narr = boa_array_make(state);
    for(i = 0; i < asz; i++)
    {
        val = boa_array_get(selfarr, i);
        res = boa_state_callvalue(state, state->strings.strcallbackforeach, callback, &val, 1);
        boa_array_push(narr, res.result);
    }
    return boa_value_fromobject(narr);
}

BoaValue boa_objfnarray_join(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    bool havejoinee;
    size_t i;
    size_t asz;
    size_t printcnt;
    BoaStream pr;
    BoaString* res;
    BoaValue joinee;
    BoaArray* selfarr;
    (void)argc;
    (void)args;
    printcnt = 0;
    havejoinee = false;
    if(argc > 0)
    {
        joinee = args[0];
        havejoinee = true;
    }
    selfarr = boa_value_asarray(instance);
    boa_stream_makestackstring(&pr);
    asz = boa_array_size(selfarr);
    for(i = 0; i < asz; i++)
    {
        boa_value_printvalue(&pr, boa_array_get(selfarr, i), false);
        printcnt++;
        if((i + 1) < asz)
        {
            if(havejoinee)
            {
                boa_value_printvalue(&pr, joinee, false);
                printcnt++;
            }
        }
    }
    if(printcnt == 0)
    {
        res = state->strings.strempty;
    }
    else
    {
        res = boa_stream_takestring(state, &pr);
    }
    return boa_value_fromobject(res);
}

bool boa_callback_sortcompare(BoaState* state, BoaValue a, BoaValue b)
{
    BoaValue args[2];
    if(boa_value_isnumber(a) && boa_value_isnumber(b))
    {
        return boa_value_asnumber(a) < boa_value_asnumber(b);
    }
    args[0] = b;
    return !boa_value_isfalsy(boa_state_findandcallmethod(state, a, state->strings.stroplessthan, args, 1).result);
}

void boa_coreutil_basicquicksort(BoaState* state, BoaValue* l, int length)
{
    int pivotindex;
    int i;
    int j;
    BoaValue tmp;
    BoaValue pivot;
    if(length < 2)
    {
        return;
    }
    pivotindex = length / 2;
    pivot = l[pivotindex];
    for(i = 0, j = length - 1;; i++, j--)
    {
        while(i < pivotindex && boa_callback_sortcompare(state, l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && boa_callback_sortcompare(state, pivot, l[j]))
        {
            j--;
        }
        if(i >= j)
        {
            break;
        }
        tmp = l[i];
        l[i] = l[j];
        l[j] = tmp;
    }
    boa_coreutil_basicquicksort(state, l, i);
    boa_coreutil_basicquicksort(state, l + i, length - i);
}

bool boa_coreutil_inlinesortcompare(BoaState* state, BoaValue callee, BoaValue a, BoaValue b)
{
    BoaResult r;
    BoaValue args[3];
    args[0] = a;
    args[1] = b;
    r = boa_state_callvalue(state, state->strings.strcallbacksort, callee, args, 2);
    return !boa_value_isfalsy(r.result);
}

void boa_coreutil_customquicksort(BoaState* state, BoaValue* l, int length, BoaValue callee)
{
    int i;
    int j;
    int pivotindex;
    BoaValue tmp;
    BoaValue pivot;
    if(length < 2)
    {
        return;
    }
    pivotindex = length / 2;
    pivot = l[pivotindex];
    for(i = 0, j = length - 1;; i++, j--)
    {
        while(i < pivotindex && boa_coreutil_inlinesortcompare(state, callee, l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && boa_coreutil_inlinesortcompare(state, callee, pivot, l[j]))
        {
            j--;
        }
        if(i >= j)
        {
            break;
        }
        tmp = l[i];
        l[i] = l[j];
        l[j] = tmp;
    }
    boa_coreutil_customquicksort(state, l, i, callee);
    boa_coreutil_customquicksort(state, l + i, length - i, callee);
}

BoaValue boa_objfnarray_sort(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaDynListVal* list;
    list = &boa_value_asarray(instance)->innerlist;
    if(argc == 1 && boa_value_iscallablefunction(args[0]))
    {
        boa_coreutil_customquicksort(state, list->listitems, list->listcount, args[0]);
    }
    else
    {
        boa_coreutil_basicquicksort(state, list->listitems, list->listcount);
    }
    return instance;
}

BoaValue boa_objfnarray_clone(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaDynListVal* list;
    BoaArray* array;
    BoaDynListVal* newvalues;
    (void)argc;
    (void)args;
    list = &boa_value_asarray(instance)->innerlist;
    array = boa_array_make(state);
    newvalues = &array->innerlist;
    boa_dynlistval_ensuresize(newvalues, list->listcount);
    /* boa_dynlistval_ensuresize sets the count to max of previous count (0 in this case) and new count, so we have to reset it */
    newvalues->listcount = 0;
    for(i = 0; i < list->listcount; i++)
    {
        boa_dynlistval_push(newvalues, list->listitems[i]);
    }
    return boa_value_fromobject(array);
}

BoaValue boa_objfnarray_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaStream pr;
    BoaString* dest;
    BoaArray* self;
    (void)argc;
    (void)args;
    self = boa_value_asarray(instance);
    boa_stream_makestackstring(&pr);
    boa_value_printobjarray(&pr, self);
    dest = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(dest);
}

BoaValue boa_objfnarray_lengthget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return boa_value_makenumber(boa_value_asarray(instance)->innerlist.listcount);
}

BoaValue boa_objfnmap_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaInstance* inst;
    BoaTable* pfields;
    (void)instance;
    (void)argc;
    (void)args;
    pfields = NULL;
    if(argc > 0)
    {
        if(boa_value_isinstance(args[0]))
        {
            inst = boa_value_asinstance(args[0]);
            pfields = &inst->fields;
        }
    }
    return boa_value_fromobject(boa_object_makemap(state, pfields));
}

BoaValue boa_objfnmap_subscript(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaValue value;
    BoaMap* map;
    BoaString* index;
    if(!boa_value_isstring(args[0]))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "map index must be a string");
        return boa_value_makenull();
    }
    map = boa_value_asmap(instance);
    index = boa_value_asstring(args[0]);
    if(argc == 2)
    {
        BoaValue val = args[1];
        boa_map_setvalue(map, index, val);
        return val;
    }
    if(!boa_map_getvalue(map, index, &value))
    {
        return boa_value_makenull();
    }
    return value;
}

BoaValue boa_objfnmap_addall(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)argc;
    if(!boa_value_ismap(args[0]))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected map as the argument");
        return boa_value_makenull();
    }
    boa_map_addall(boa_value_asmap(args[0]), boa_value_asmap(instance));
    return boa_value_makenull();
}

BoaValue boa_objfnmap_clear(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaMap* map;
    (void)state;
    (void)argc;
    (void)args;
    map = boa_value_asmap(instance);
    map->innertable.htcount = 0;
    return boa_value_makenull();
}

BoaValue boa_objfnmap_iterator(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int index;
    int value;
    (void)state;
    (void)argc;
    index = boa_value_isnull(args[0]) ? -1 : boa_value_asnumber(args[0]);
    value = boa_coreutil_tableiterator(&boa_value_asmap(instance)->innertable, index);
    return value == -1 ? boa_value_makenull() : boa_value_makenumber(value);
}

BoaValue boa_objfnmap_itervalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t index;
    (void)state;
    (void)argc;
    index = boa_value_asnumber(args[0]);
    return boa_coreutil_tableiterkey(&boa_value_asmap(instance)->innertable, index);
}

BoaValue boa_objfnmap_foreach(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int i;
    BoaTable* tab;
    BoaTabEntry* entry;
    BoaValue callback;
    BoaValue callargs[3];
    (void)argc;
    callback = args[0];
    if(!boa_value_iscallablefunction(callback))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "expected a function as the callback");
        return boa_value_makenull();
    }
    tab = &boa_value_asmap(instance)->innertable;
    for(i = 0; i < tab->htcapacity; i++)
    {
        entry = &tab->htentries[i];
        if(entry->entkey != NULL)
        {
            callargs[0] = boa_value_fromobject(entry->entkey);
            callargs[1] = entry->entvalue;
            boa_state_callvalue(state, state->strings.strcallbackforeach, callback, callargs, 2);
        }
    }
    return boa_value_makenull();
}

BoaValue boa_objfnmap_clone(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaMap* map;
    (void)argc;
    (void)args;
    map = boa_object_makemap(state, NULL);
    boa_table_addall(&boa_value_asmap(instance)->innertable, &map->innertable);
    return boa_value_fromobject(map);
}

BoaValue boa_objfnmap_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaStream pr;
    BoaMap* self;
    BoaString* dest;
    (void)argc;
    (void)args;
    self = boa_value_asmap(instance);
    boa_stream_makestackstring(&pr);
    boa_value_printobjtable(&pr, (BoaObject*)self, &self->innertable);
    dest = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(dest);
}

BoaValue boa_objfnmap_lengthget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_value_asmap(instance)->innertable.htcount);
}

BoaValue boa_objfnrange_iterator(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int number;
    BoaRange* range;
    (void)state;
    (void)argc;
    range = boa_value_asrange(instance);
    number = range->from;
    if(boa_value_isnumber(args[0]))
    {
        number = boa_value_asnumber(args[0]);
        if(range->to > range->from ? number >= range->to : number <= range->to)
        {
            return boa_value_makenull();
        }
        number += (range->from - range->to) > 0 ? -1 : 1;
    }
    return boa_value_makenumber(number);
}

BoaValue boa_objfnrange_itervalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    return args[0];
}

BoaValue boa_objfnrange_tostring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaRange* range;
    (void)argc;
    (void)args;
    range = boa_value_asrange(instance);
    return boa_string_valformat(state, "Range(#, #)", range->from, range->to);
}

BoaValue boa_objfnrange_fromget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_value_asrange(instance)->from);
}

BoaValue boa_objfnrange_fromset(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    boa_value_asrange(instance)->from = boa_value_asnumber(args[0]);
    return args[0];
}

BoaValue boa_objfnrange_toget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_value_asrange(instance)->to);
}

BoaValue boa_objfnrange_toset(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    boa_value_asrange(instance)->to = boa_value_asnumber(args[0]);
    return args[0];
}

BoaValue boa_objfnrange_lengthget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaRange* range;
    (void)state;
    (void)argc;
    (void)args;
    range = boa_value_asrange(instance);
    return boa_value_makenumber(range->to - range->from);
}

BoaValue boa_objfnprocess_exit(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int ec;
    (void)instance;
    ec = 0;
    if(argc > 0)
    {
        if(boa_value_isnumber(args[0]))
        {
            ec = boa_value_asnumber(args[0]);
        }
    }
    boa_state_destroy(state);
    exit(ec);
    return boa_value_makenull();
}

BoaValue boa_objfnprocess_abort(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    abort();
    return boa_value_makenull();
}

BoaValue boa_objfnprocess_kill(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int rt;
    int pid;
    int code;
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    pid = boa_value_asnumber(args[0]);
    code = boa_value_asnumber(args[1]);
    rt = boa_util_kill(pid, code);
    return boa_value_makenumber(rt);
}

BoaValue boa_objfnprocess_getpid(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_util_getpid());
}

BoaValue boa_objfnprocess_setenv(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    bool replace;
    BoaString* key;
    BoaString* value;
    replace = true;
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    key = boa_value_tostring(state, args[0], 0);
    value = boa_value_tostring(state, args[1], 0);
    return boa_value_makebool(boa_util_setenv(boa_string_getdata(key), boa_string_getdata(value), replace));
}

BoaValue boa_objfnscriptvm_getglobal(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* name;
    (void)instance;
    (void)argc;
    name = boa_value_asstring(args[0]);
    return boa_state_getglobal(state, name);
}

BoaValue boa_objfnscriptvm_globalsget(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_fromobject(state->vmstate.globals);
}

void boa_classcallbackregex_oncleanup(BoaState* state, BoaUserdata* data, bool mark)
{
    BoaRegexData* rxdata;
    (void)state;
    (void)rxdata;
    if(mark)
    {
        return;
    }
    rxdata = ((BoaRegexData*)data->data);
    /* boa_sysmem_free(rxdata->rxctx);*/
}

BoaValue boa_objfnregex_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* pattern;
    const char* strpattern;
    BoaRegexData* data;
    BoaClass* rxclass;
    BoaChecker check;
    MRXContext rx;
    (void)argc;
    BOA_CHECK_INIT(state, &check, "Regexp::constructor", argc, args);
    BOA_CHECK_REQUIREARGS(&check, 1);
    BOA_CHECK_CHECKARGTYPE(&check, 0, boa_value_isstring);
    pattern = boa_value_asstring(args[0]);
    strpattern = boa_string_getdata(pattern);
    mrx_context_initctx(&rx, false);
    if(mrx_regex_parse(&rx, strpattern, 0) != 0)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdexception, rx.errorbuf);
    }
    if(!boa_value_isinstance(instance))
    {
        rxclass = boa_value_asclass(instance);
        instance = boa_value_fromobject(boa_object_makeinstance(state, rxclass));
    }
    data = (BoaRegexData*)boa_userdata_insertdata(state, instance, sizeof(BoaRegexData), NULL);
    data->rxctx = rx;
    return instance;
}

BoaValue boa_objfnregex_match(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    enum { kMaxCaps = 32 };
    int64_t ic; 
    int64_t textlen;
    int64_t matchcnt;
    const char* textstr;
    BoaArray* arr;
    BoaString* instr;
    BoaString* sub;
    BoaRegexData* data;
    BoaChecker check;
    int64_t cappos[kMaxCaps+1];
    int64_t capspan[kMaxCaps+1];
    BOA_CHECK_INIT(state, &check, "Regexp::match", argc, args);
    BOA_CHECK_REQUIREARGS(&check, 1);
    BOA_CHECK_CHECKARGTYPE(&check, 0, boa_value_isstring);
    instr = boa_value_asstring(args[0]);
    data = (BoaRegexData*)boa_userdata_extractdata(instance);
    memset(cappos, 0xFF, sizeof(cappos));
    memset(capspan, 0xFF, sizeof(capspan));
    textstr = boa_string_getdata(instr);
    textlen = boa_string_getlength(instr);
    matchcnt = mrx_regex_match(&data->rxctx, textstr, textlen, 0, kMaxCaps, cappos, capspan);
    #if 1
        fprintf(stderr, "Regexp::match: matchcnt=%ld\n", matchcnt);
    #endif
    if(matchcnt == -1)
    {
        return boa_value_makenull();
    }
    arr = boa_array_make(state);
    for(ic=0; ic<matchcnt; ic++)
    {
        if((cappos[ic] == -1))
        {
            continue;
        }
        sub = boa_string_copylen(state, &textstr[cappos[ic]], capspan[ic]);
        boa_array_push(arr, boa_value_fromobject(sub));
    }
    return boa_value_fromobject(arr);
}

void boa_state_openstdclasses(BoaState* state)
{
    BoaClass* klass;
    {
        klass = boa_class_make(state, "Class", NULL);
        boa_class_bindmethod(klass, "toString", boa_objfnclass_tostring);
        boa_class_bindmethod(klass, "[]", boa_objfnclass_subscript);
        boa_class_bindstaticmethod(klass, "toString", boa_objfnclass_tostring);
        boa_class_bindstaticmethod(klass, "iterator", boa_objfnclass_iterator);
        boa_class_bindstaticmethod(klass, "iteratorValue", boa_objfnclass_itervalue);
        boa_class_bindgetsetter(klass, "super", boa_objfnclass_superget, NULL);
        boa_class_bindstaticgetter(klass, "super", boa_objfnclass_superget);
        boa_class_bindstaticgetter(klass, "name", boa_objfnclass_nameget);
        state->stdclassclass = klass;
        boa_state_setglobal(state, klass->name, boa_value_fromobject(klass));
    }
    {
        klass = boa_class_make(state, "Object", NULL);
        boa_class_inherit(klass, state->stdclassclass);
        boa_class_bindstaticmethod(klass, "keys", boa_objfnobject_keys);
        boa_class_bindmethod(klass, "toString", boa_objfnobject_tostring);
        boa_class_bindmethod(klass, "dump", boa_objfnobject_dump);
        boa_class_bindmethod(klass, "isCallable", boa_objfnobject_iscallable);
        boa_class_bindmethod(klass, "[]", boa_objfnobject_subscript);
        boa_class_bindmethod(klass, "iterator", boa_objfnobject_iterator);
        boa_class_bindmethod(klass, "iteratorValue", boa_objfnobject_itervalue);
        boa_class_bindgetsetter(klass, "class", boa_objfnobject_classget, NULL);
        state->stdobjectclass = klass;
        state->stdobjectclass->super = state->stdclassclass;
    }
    {
        klass = boa_class_make(state, "Null", state->stdobjectclass);
        state->stdnullclass = klass;
    }
    {
        klass = boa_class_make(state, "Number", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnnumber_constructor);
        boa_class_bindgetsetter(klass, "chr", boa_objfnnumber_chrget, NULL);
        boa_class_bindmethod(klass, "toString", boa_objfnnumber_tostring);
        state->stdclassnumber = klass;
    }
    {
        klass = boa_class_make(state, "String", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindstaticmethod(klass, "chr", boa_objfnstring_chr);
        boa_class_bindstaticmethod(klass, "fromCharCode", boa_objfnstring_chr);
        boa_class_bindstaticmethod(klass, "utf8Encode", boa_objfnstring_utf8encode);

        #if 0
        boa_class_bindmethod(klass, "-", boa_objfnstring_minus);
        #endif
        boa_class_bindmethod(klass, "+", boa_objfnstring_plus);
        boa_class_bindmethod(klass, "<", boa_objfnstring_lessthan);
        boa_class_bindmethod(klass, ">", boa_objfnstring_greaterthan);
        boa_class_bindmethod(klass, ">=", boa_objfnstring_greaterequal);
        boa_class_bindmethod(klass, "<=", boa_objfnstring_lessequal);
        boa_class_bindmethod(klass, "utf8Codepoints", boa_objfnstring_utf8codepoints);
        boa_class_bindmethod(klass, "utf8Chars", boa_objfnstring_utf8chars);
        boa_class_bindmethod(klass, "toString", boa_objfnstring_tostring);
        boa_class_bindmethod(klass, "toNumber", boa_objfnstring_tonumber);
        boa_class_bindmethod(klass, "toUpperCase", boa_objfnstring_touppercase);
        boa_class_bindmethod(klass, "upper", boa_objfnstring_touppercase);
        boa_class_bindmethod(klass, "toLowerCase", boa_objfnstring_tolowercase);
        boa_class_bindmethod(klass, "lower", boa_objfnstring_tolowercase);
        boa_class_bindmethod(klass, "contains", boa_objfnstring_contains);
        boa_class_bindmethod(klass, "startsWith", boa_objfnstring_startswith);
        boa_class_bindmethod(klass, "endsWith", boa_objfnstring_endswith);
        boa_class_bindmethod(klass, "replace", boa_objfnstring_replace);
        boa_class_bindmethod(klass, "substring", boa_objfnstring_substring);
        boa_class_bindmethod(klass, "substr", boa_objfnstring_substring);
        boa_class_bindmethod(klass, "indexOf", boa_objfnstring_indexof);
        boa_class_bindmethod(klass, "iterator", boa_objfnstring_iterator);
        boa_class_bindmethod(klass, "iteratorValue", boa_objfnstring_itervalue);
        boa_class_bindmethod(klass, "trim", boa_objfnstring_trim);
        boa_class_bindmethod(klass, "trimLeft", boa_objfnstring_ltrim);
        boa_class_bindmethod(klass, "trimRight", boa_objfnstring_rtrim);
        boa_class_bindmethod(klass, "[]", boa_objfnstring_subscript);
        boa_class_bindmethod(klass, "charAt", boa_objfnstring_charat);
        boa_class_bindmethod(klass, "charCodeAt", boa_objfnstring_charcodeat);
        boa_class_bindmethod(klass, "size", boa_objfnstring_lengthget);
        boa_class_bindmethod(klass, "split", boa_objfnstring_split);
        boa_class_bindgetsetter(klass, "length", boa_objfnstring_lengthget, NULL);
        state->stdclassstring = klass;
    }
    {
        klass = boa_class_make(state, "Bool", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindmethod(klass, "toString", boa_objfnbool_tostring);
        state->stdclassbool = klass;
    }
    {
        klass = boa_class_make(state, "Function", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindmethod(klass, "toString", boa_objfnfunction_tostring);
        boa_class_bindgetsetter(klass, "name", boa_objfnfunction_nameget, NULL);
        state->stdclassfunction = klass;
    }
    {
        klass = boa_class_make(state, "Fiber", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnfiber_constructor);
        boa_class_bindmethod(klass, "run", boa_objfnfiber_run);
        boa_class_bindmethod(klass, "try", boa_objfnfiber_try);
        boa_class_bindgetsetter(klass, "done", boa_objfnfiber_doneget, NULL);
        boa_class_bindgetsetter(klass, "error", boa_objfnfiber_errorget, NULL);
        boa_class_bindstaticmethod(klass, "yield", boa_objfnfiber_yield);
        boa_class_bindstaticmethod(klass, "abort", boa_objfnfiber_abort);
        boa_class_bindstaticgetter(klass, "current", boa_objfnfiber_currentget);
        state->stdclassfiber = klass;
    }
    {
        klass = boa_class_make(state, "Module", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_setstaticfield(klass, "loaded", boa_value_fromobject(state->vmstate.modules));
        boa_class_bindstaticgetter(klass, "privates", boa_objfnmodule_privatesget);
        boa_class_bindstaticgetter(klass, "current", boa_objfnmodule_currentget);
        boa_class_bindmethod(klass, "toString", boa_objfnmodule_tostring);
        boa_class_bindgetsetter(klass, "name", boa_objfnmodule_nameget, NULL);
        boa_class_bindgetsetter(klass, "privates", boa_objfnmodule_privatesget, NULL);
        state->stdclassmodule = klass;
    }
    {
        klass = boa_class_make(state, "Array", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnarray_constructor);
        boa_class_bindmethod(klass, "[]", boa_objfnarray_subscript);
        boa_class_bindmethod(klass, "add", boa_objfnarray_push);
        boa_class_bindmethod(klass, "push", boa_objfnarray_push);
        boa_class_bindmethod(klass, "insert", boa_objfnarray_insert);
        boa_class_bindmethod(klass, "slice", boa_objfnarray_slice);
        boa_class_bindmethod(klass, "addAll", boa_objfnarray_addall);
        boa_class_bindmethod(klass, "pop", boa_objfnarray_pop);
        boa_class_bindmethod(klass, "remove", boa_objfnarray_remove);
        boa_class_bindmethod(klass, "removeAt", boa_objfnarray_removeat);
        boa_class_bindmethod(klass, "indexOf", boa_objfnarray_indexof);
        boa_class_bindmethod(klass, "contains", boa_objfnarray_contains);
        boa_class_bindmethod(klass, "includes", boa_objfnarray_contains);
        boa_class_bindmethod(klass, "clear", boa_objfnarray_clear);
        boa_class_bindmethod(klass, "iterator", boa_objfnarray_iterator);
        boa_class_bindmethod(klass, "iteratorValue", boa_objfnarray_itervalue);
        boa_class_bindmethod(klass, "forEach", boa_objfnarray_foreach);
        boa_class_bindmethod(klass, "map", boa_objfnarray_map);
        boa_class_bindmethod(klass, "join", boa_objfnarray_join);
        boa_class_bindmethod(klass, "sort", boa_objfnarray_sort);
        boa_class_bindmethod(klass, "clone", boa_objfnarray_clone);
        boa_class_bindmethod(klass, "toString", boa_objfnarray_tostring);
        boa_class_bindgetsetter(klass, "length", boa_objfnarray_lengthget, NULL);
        state->stdclassarray = klass;
    }
    {
        klass = boa_class_make(state, "Map", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnmap_constructor);
        boa_class_bindmethod(klass, "[]", boa_objfnmap_subscript);
        boa_class_bindmethod(klass, "addAll", boa_objfnmap_addall);
        boa_class_bindmethod(klass, "clear", boa_objfnmap_clear);
        boa_class_bindmethod(klass, "iterator", boa_objfnmap_iterator);
        boa_class_bindmethod(klass, "iteratorValue", boa_objfnmap_itervalue);
        boa_class_bindmethod(klass, "forEach", boa_objfnmap_foreach);
        boa_class_bindmethod(klass, "clone", boa_objfnmap_clone);
        boa_class_bindmethod(klass, "toString", boa_objfnmap_tostring);
        boa_class_bindgetsetter(klass, "length", boa_objfnmap_lengthget, NULL);
        state->stdclassmap = klass;
    }
    {
        klass = boa_class_make(state, "Range", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindmethod(klass, "iterator", boa_objfnrange_iterator);
        boa_class_bindmethod(klass, "iteratorValue", boa_objfnrange_itervalue);
        boa_class_bindmethod(klass, "toString", boa_objfnrange_tostring);
        boa_class_bindgetsetter(klass, "from", boa_objfnrange_fromget, boa_objfnrange_fromset);
        boa_class_bindgetsetter(klass, "to", boa_objfnrange_toget, boa_objfnrange_toset);
        boa_class_bindgetsetter(klass, "length", boa_objfnrange_lengthget, NULL);
        state->stdclassrange = klass;
    }
    {
        klass = boa_class_make(state, "JSON", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindstaticmethod(klass, "stringify", boa_objfnobject_dump);        
    }
    {
        klass = boa_class_make(state, "Regexp", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnregex_constructor);
        boa_class_bindmethod(klass, "match", boa_objfnregex_match);
        state->stdclassregex = klass;
    }
    {
        klass = boa_class_make(state, "Process", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfndefault_invalidconstructor);
        boa_class_bindstaticmethod(klass, "exit", boa_objfnprocess_exit);
        boa_class_bindstaticmethod(klass, "abort", boa_objfnprocess_abort);
        boa_class_bindstaticmethod(klass, "kill", boa_objfnprocess_kill);
        boa_class_bindstaticmethod(klass, "pid", boa_objfnprocess_getpid);
        boa_class_bindstaticmethod(klass, "setenv", boa_objfnprocess_setenv);
        boa_class_setstaticfield(klass, "platform", boa_value_fromobject(boa_string_copy(state, BOA_CONFIG_PLATFORMNAME)));
        boa_class_setstaticfield(klass, "arch", boa_value_fromobject(boa_string_copy(state, BOA_CONFIG_ARCHNAME)));
        boa_class_setstaticfield(klass, "bits", boa_value_makenumber(BOA_CONFIG_ARCHBITS));
        #if 0
            boa_class_bindstaticgetter(klass, boa_string_copy(state, "STDOUT"), boa_value_fromobject(state->streamstdout));
        #endif
    }
    {
        klass = boa_class_make(state, "ScriptVM", state->stdobjectclass);
        boa_class_bindstaticmethod(klass, "getglobal", boa_objfnscriptvm_getglobal);
        boa_class_bindstaticgetter(klass, "globals", boa_objfnscriptvm_globalsget);
    }
    {
        {
            klass = boa_class_make(state, "Exception", state->stdobjectclass);
            boa_class_bindconstructor(klass, boa_objfnexception_constructor);
            boa_class_bindgetsetter(klass, "message", boa_objfnexception_messageget, NULL);
            state->exceptions.stdexception = boa_object_makeexception(state, klass);
        }
        {
            klass = boa_class_make(state, "IOError", state->exceptions.stdexception->baseclass);
            state->exceptions.stdioerror = boa_object_makeexception(state, klass);
        }
        {
            klass = boa_class_make(state, "ArgumentError", state->exceptions.stdexception->baseclass);
            state->exceptions.stdargumenterror = boa_object_makeexception(state, klass);            
        }
    }
}

BoaValue boa_cfn_srand(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    srand(boa_value_asnumber(args[0]));
    return boa_value_makenull();
}

BoaValue boa_cfn_random(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    uint64_t iv;
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    #if defined(BOA_OSPLATFORM_ISWINNT) && (BOA_OSPLATFORM_ISWINNT == 1)
        iv = rand();
    #else
        iv = random();
    #endif
    return boa_value_makenumber(iv);
}

BoaValue boa_cfn_time(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber((double)clock() / CLOCKS_PER_SEC);
}

BoaValue boa_cfn_systemtime(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(time(NULL));
}

BoaValue boa_cfn_printvalues(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    (void)instance;
    if(argc == 0)
    {
        return boa_value_makenull();
    }
    for(i = 0; i < argc; i++)
    {
        boa_value_printvalue(state->streamstdout, args[i], false);
    }
    return boa_value_makenull();
}

BoaValue boa_cfn_println(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaValue r;
    (void)instance;
    r = boa_cfn_printvalues(state, instance, argc, args);
    boa_stream_putc(state->streamstdout, '\n');
    return r;
}

BoaValue boa_cfn_printchar(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    char ch;
    size_t i;
    (void)instance;
    if(argc == 0)
    {
        return boa_value_makenull();
    }
    for(i = 0; i < argc; i++)
    {
        ch = boa_value_asnumber(args[i]);
        boa_stream_putlen(state->streamstdout, &ch, 1);
    }
    return boa_value_makenull();
}

void boa_util_printfhelper(BoaState* state, BoaStream* strm, size_t argc, BoaValue* args)
{
    char spec;
    size_t argid;
    const char* fmt;
    const char* c;
    if(argc < 1)
    {
        return;
    }
    fmt = boa_string_getdata(boa_value_asstring(args[0]));
    argid = 1;
    for(c = fmt; *c != '\0'; c++)
    {
        if(*c != '%')
        {
            boa_stream_putc(strm, (int)*c);
            continue;
        }
        c++;
        spec = *c;
        switch(spec)
        {
            case 'd':
            case 'i':
                {
                    BoaNumber v;
                    BoaValue str;
                    if(argid >= argc)
                    {
                        goto done;
                    }
                    v = boa_value_asnumber(args[argid++]);
                    str = boa_string_numbertostring(state, v);
                    boa_stream_putlen(strm, boa_string_getdata(boa_value_asstring(str)), boa_string_getlength(boa_value_asstring(str)));
                }
                break;
            case 's':
                {
                    const char* s;
                    if(argid >= argc)
                    {
                        goto done;
                    }
                    s = boa_string_getdata(boa_value_asstring(args[argid++]));
                    boa_stream_puts(strm, s);
                }
                break;
            case 'c':
                {
                    BoaNumber v;
                    if(argid >= argc)
                    {
                        goto done;
                    }
                    v = boa_value_asnumber(args[argid++]);
                    boa_stream_putc(strm, (int)v);
                }
                break;
            case 'p':
                {
                    if(argid >= argc)
                    {
                        goto done;
                    }
                    boa_value_printvalue(strm, args[argid++], true);
                }
                break;
            case '%':
                {
                    boa_stream_putc(strm, '%');
                }
                break;
            default:
                {
                    boa_stream_putc(strm, '%');
                    boa_stream_putc(strm, (int)spec);
                }
                break;
        }
    }
    done:
    return;
}

BoaValue boa_cfn_printf(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    boa_util_printfhelper(state, state->streamstdout, argc, args);
    return boa_value_makenull();
}

BoaValue boa_cfn_sprintf(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaStream pr;
    BoaString* str;
    (void)instance;
    boa_stream_makestackstring(&pr);
    boa_util_printfhelper(state, &pr, argc, args);
    str = boa_stream_takestring(state, &pr);
    return boa_value_fromobject(str);
}

BoaValue boa_cfn_atoi(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    return boa_objfnnumber_constructor_actual(state, instance, argc, args, true);
}

BoaValue boa_cfn_typeof(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* r;
    (void)argc;
    (void)instance;
    if(boa_value_isexception(args[0]))
    {
        r = boa_value_asexception(args[0])->baseclass->name;
        return boa_value_fromobject(r);
    }
    r = boa_string_copy(state, boa_value_valtypename(args[0]));
    return boa_value_fromobject(r);
}

BoaValue boa_cfn_require(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* name;
    (void)instance;
    (void)argc;
    name = boa_value_asstring(args[0]);
    /*
    if(strcmp(name, "network") == 0)
    {
        boa_extmodnetwork_open(state);
    }
    else
    */
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "unknown built-in library %s", boa_string_getdata(name));
    }
    return boa_value_makenull();
}

bool boa_evalutil_interpmodule(BoaState* state, BoaModule* module)
{
    BoaFiber* fiber;
    BoaFuncScript* function;
    function = module->mainfunction;
    fiber = boa_object_makefiber(state, module, function);
    fiber->parent = state->vmstate.fiber;
    state->vmstate.fiber = fiber;
    return true;
}

bool boa_evalutil_compileandrun(BoaState* state, BoaString* modname, const char* source)
{
    BoaModule* module;
    module = boa_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        return false;
    }
    module->ran = true;
    return boa_evalutil_interpmodule(state, module);
}

BoaValue boa_cfn_eval(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    const char* strcode;
    BoaString* code;
    BoaFiber* fiber;
    (void)instance;
    (void)argc;
    code = boa_value_asstring(args[0]);
    strcode = boa_string_getdata(code);
    fiber = state->vmstate.fiber;
    if(boa_evalutil_compileandrun(state, fiber->module->name, strcode))
    {
        fiber->returnaddress = args - 1;
    }
    return boa_value_makenull();
}

void boa_state_openglobalfuncs(BoaState* state)
{
    boa_state_defnative(state, "srand", boa_cfn_srand);
    boa_state_defnative(state, "random", boa_cfn_random);
    boa_state_defnative(state, "time", boa_cfn_time);
    boa_state_defnative(state, "systemTime", boa_cfn_systemtime);
    boa_state_defnative(state, "print", boa_cfn_printvalues);
    boa_state_defnative(state, "printchar", boa_cfn_printchar);
    boa_state_defnative(state, "println", boa_cfn_println);
    boa_state_defnative(state, "printf", boa_cfn_printf);
    boa_state_defnative(state, "sprintf", boa_cfn_sprintf);
    boa_state_defnative(state, "atoi", boa_cfn_atoi);
    boa_state_defnative(state, "typeof", boa_cfn_typeof);
    boa_state_defnative(state, "require", boa_cfn_require);
    boa_state_defnative(state, "eval", boa_cfn_eval);
}

void boa_bcemu_initfile(BoaEmulatedFile* emu, const char* source)
{
    emu->source = source;
    emu->position = 0;
}

void boa_bcemu_readuint8(BoaEmulatedFile* emu, uint8_t* dest)
{
    *dest = emu->source[emu->position++];
}

void boa_bcemu_readuint16(BoaEmulatedFile* emu, uint16_t* dest)
{
    uint8_t v1;
    uint8_t v2;
    boa_bcemu_readuint8(emu, &v1);
    boa_bcemu_readuint8(emu, &v2);
    *dest = (uint16_t)(v1 | (v2 << 8u));
}

void boa_bcemu_readuint32(BoaEmulatedFile* emu, uint32_t* dest)
{
    uint8_t v1;
    uint8_t v2;
    uint8_t v3;
    uint8_t v4;
    boa_bcemu_readuint8(emu, &v1);
    boa_bcemu_readuint8(emu, &v2);
    boa_bcemu_readuint8(emu, &v3);
    boa_bcemu_readuint8(emu, &v4);
    *dest = (uint32_t)(v1 | (v2 << 8u) | (v3 << 16u) | (v4 << 24u));
}

void boa_bcemu_readuint64(BoaEmulatedFile* emu, uint64_t* dest)
{
    uint32_t v1;
    uint32_t v2;
    boa_bcemu_readuint32(emu, &v1);
    boa_bcemu_readuint32(emu, &v2);
    *dest = (uint64_t)(v1 | ((uint64_t)v2 << 32u));
}

void boa_bcemu_readnumber(BoaEmulatedFile* emu, BoaNumber* dest)
{
    size_t i;
    uint8_t buf[sizeof(BoaNumber)+1];
    BoaNumber result;
    for(i = 0; i < sizeof(BoaNumber); i++)
    {
        boa_bcemu_readuint8(emu, &buf[i]);
    }
    memcpy(&result, buf, sizeof(BoaNumber));
    *dest = result;
}

size_t boa_bcfile_writeuint8(FILE* hnd, uint8_t byte)
{
    size_t rsz;
    rsz = fwrite(&byte, sizeof(uint8_t), 1, hnd);
    return rsz;
}

size_t boa_bcfile_writeuint16(FILE* hnd, uint16_t byte)
{
    size_t rsz;
    rsz = fwrite(&byte, sizeof(uint16_t), 1, hnd);
    return rsz;
}

size_t boa_bcfile_writeuint32(FILE* hnd, uint32_t byte)
{
    size_t rsz;
    rsz = fwrite(&byte, sizeof(uint32_t), 1, hnd);
    return rsz;
}

size_t boa_bcfile_writeuint64(FILE* hnd, uint64_t byte)
{
    size_t rsz;
    rsz = fwrite(&byte, sizeof(uint64_t), 1, hnd);
    return rsz;
}

size_t boa_bcfile_writenumber(FILE* hnd, BoaNumber byte)
{
    size_t rsz;
    rsz = fwrite(&byte, sizeof(BoaNumber), 1, hnd);
    return rsz;
}

void boa_util_writestringdata(FILE* hnd, const char* sdata, uint32_t slen)
{
    uint8_t wch;
    uint8_t rch;
    uint32_t i;
    for(i = 0; i < slen; i++)
    {
        rch = (uint8_t)sdata[i];
        #if 1
            wch = rch ^ BOA_CONFIG_BCSTRINGKEY;
        #else
            wch = rch;
        #endif
        boa_bcfile_writeuint8(hnd, wch);
    }
}

void boa_bcfile_writestring(FILE* hnd, BoaString* string)
{
    uint32_t len;
    uint32_t i;
    uint8_t wch;
    uint8_t rch;
    len = boa_string_getlength(string);
    boa_bcfile_writeuint32(hnd, len);
    for(i = 0; i < len; i++)
    {
        rch = (uint8_t)boa_string_getat(string, i);
        #if 1
            wch = rch ^ BOA_CONFIG_BCSTRINGKEY;
        #else
            wch = rch;
        #endif
        boa_bcfile_writeuint8(hnd, wch);
    }
}

BoaString* boa_bcemu_readstring(BoaState* state, BoaEmulatedFile* emu)
{
    uint8_t tmp;
    uint32_t i;
    uint32_t length;
    char* buffer;
    BoaString* res;
    boa_bcemu_readuint32(emu, &length);
    if(length == 0)
    {
        /* return interned empty string instead of NULL, so that empty string
         * constants ("") round-trip correctly through serialization */
        return state->strings.strempty;
    }
    buffer = (char*)boa_sysmem_malloc(length + 1);
    for(i = 0; i < length; i++)
    {
        boa_bcemu_readuint8(emu, &tmp);
        buffer[i] = (char)tmp ^ BOA_CONFIG_BCSTRINGKEY;
    }
    buffer[length] = '\0';
    res = boa_string_copylen(state, buffer, length);
    boa_sysmem_free(buffer);
    return res;
}

void boa_bcfile_writefunction(FILE* hnd, BoaFuncScript* function)
{
    boa_bcfile_writechunk(hnd, &function->chunk);
    boa_bcfile_writestring(hnd, function->name);
    boa_bcfile_writeuint32(hnd, function->argcount);
    boa_bcfile_writeuint32(hnd, function->upvaluecount);
    boa_bcfile_writeuint8(hnd, (uint8_t)function->vararg);
    boa_bcfile_writeuint64(hnd, function->maxregisters);
}

BoaFuncScript* boa_bcemu_readfunction(BoaState* state, BoaEmulatedFile* emu, BoaModule* module)
{
    uint8_t tmp;
    BoaFuncScript* function;
    function = boa_object_makefunction(state, module);
    boa_bcemu_readchunk(state, emu, module, &function->chunk);
    function->name = boa_bcemu_readstring(state, emu);
    boa_bcemu_readuint32(emu, &function->argcount);
    boa_bcemu_readuint32(emu, &function->upvaluecount);
    boa_bcemu_readuint8(emu, &tmp);
    function->vararg = tmp;
    boa_bcemu_readuint64(emu, &function->maxregisters);
    return function;
}

void boa_bcfile_writeobject(FILE* hnd, BoaObject* obj)
{
    BoaObjType type;
    if(obj == NULL)
    {
        boa_bcfile_writeuint64(hnd, 0xffffffffffffffff);
        return;
    }
    type = obj->type;
    boa_bcfile_writeuint64(hnd, (uint64_t)(type + 1));
    switch(type)
    {
        case BOA_OBJTYPE_STRING:
            {
                boa_bcfile_writestring(hnd, (BoaString*)obj);
            }
            break;
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                boa_bcfile_writefunction(hnd, (BoaFuncScript*)obj);
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                uint32_t i;
                BoaClsPrototype* clsproto;
                clsproto = (BoaClsPrototype*)obj;
                boa_bcfile_writefunction(hnd, clsproto->function);
                boa_bcfile_writeuint32(hnd, clsproto->upvaluecount);
                for(i = 0; i < clsproto->upvaluecount; i++)
                {
                    boa_bcfile_writeuint8(hnd, (uint8_t)clsproto->local[i]);
                    boa_bcfile_writeuint32(hnd, clsproto->indexes[i]);
                }
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                BoaField* field;
                field = (BoaField*)obj;
                boa_bcfile_writeobject(hnd, field->getter);
                boa_bcfile_writeobject(hnd, field->setter);
            }
            break;
        default:
            {
                fprintf(stderr, "ERROR: serializing type %d (%s) not implemented!\n", type, boa_value_objtypename(type));
                BOA_UTIL_UNREACHABLE();
            }
            break;
    }
}

BoaObject* boa_bcemu_readobject(BoaState* state, BoaEmulatedFile* emu, BoaModule* module)
{
    uint64_t type;
    static uint64_t kInvalidType = 0xffffffffffffffff;
    boa_bcemu_readuint64(emu, &type);
    if(type == kInvalidType)
    {
        return NULL;
    }
    switch((BoaObjType)(type - 1))
    {
        case BOA_OBJTYPE_STRING:
            {
                return (BoaObject*)boa_bcemu_readstring(state, emu);
            }
            break;
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                return (BoaObject*)boa_bcemu_readfunction(state, emu, module);
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                uint32_t upvaluecount;
                uint32_t i;
                uint8_t tmp;
                BoaFuncScript* function;
                BoaClsPrototype* clsproto;
                function = boa_bcemu_readfunction(state, emu, module);
                clsproto = boa_object_makeclsproto(state, function);
                boa_bcemu_readuint32(emu, &upvaluecount);
                for(i = 0; i < upvaluecount; i++)
                {
                    boa_bcemu_readuint8(emu, &tmp);
                    clsproto->local[i] = (bool)tmp;
                    boa_bcemu_readuint32(emu, &clsproto->indexes[i]);
                }
                return (BoaObject*)clsproto;
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                BoaObject* getter;
                BoaObject* setter;
                getter = boa_bcemu_readobject(state, emu, module);
                setter = boa_bcemu_readobject(state, emu, module);
                return (BoaObject*)boa_object_makefield(state, getter, setter);
            }
            break;
        default:
            {
                BOA_UTIL_UNREACHABLE();
            }
            break;
    }
    return NULL;
}

void boa_bcfile_writechunk(FILE* hnd, BoaChunk* chunk)
{
    size_t i;
    size_t c;
    BoaValue constant;
    boa_bcfile_writeuint64(hnd, chunk->compiledcodecount);
    for(i = 0; i < chunk->compiledcodecount; i++)
    {
        boa_bcfile_writeuint64(hnd, chunk->compiledcodechunk[i]);
    }
    if(chunk->haslineinfo)
    {
        c = chunk->linecount + 2;
        boa_bcfile_writeuint64(hnd, c);
        for(i = 0; i < c; i++)
        {
            boa_bcfile_writeuint16(hnd, chunk->lines[i]);
        }
    }
    else
    {
        boa_bcfile_writeuint64(hnd, 0);
    }
    boa_bcfile_writeuint64(hnd, chunk->constantlist.listcount);
    for(i = 0; i < chunk->constantlist.listcount; i++)
    {
        constant = chunk->constantlist.listitems[i];
        if(boa_value_isobject(constant))
        {
            boa_bcfile_writeobject(hnd, boa_value_asobject(constant));
        }
        else
        {
            boa_bcfile_writeuint64(hnd, 0);
            boa_bcfile_writenumber(hnd, boa_value_asnumber(constant));
        }
    }
}

void boa_bcemu_readchunk(BoaState* state, BoaEmulatedFile* emu, BoaModule* module, BoaChunk* chunk)
{
    size_t i;
    size_t count;
    uint64_t type;
    BoaNumber dtmp;
    BoaObject* obj;
    boa_chunk_init(chunk);
    boa_bcemu_readuint64(emu, &count);
    chunk->compiledcodechunk = (uint64_t*)boa_sysmem_malloc(sizeof(uint64_t) * count);
    chunk->compiledcodecount = count;
    chunk->capacity = count;
    for(i = 0; i < count; i++)
    {
        boa_bcemu_readuint64(emu, &chunk->compiledcodechunk[i]);
    }
    boa_bcemu_readuint64(emu, &count);
    if(count > 0)
    {
        chunk->lines = (uint16_t*)boa_sysmem_malloc(sizeof(uint16_t) * count);
        chunk->linecount = count;
        chunk->linecapacity = count;
        for(i = 0; i < count; i++)
        {
            boa_bcemu_readuint16(emu, &chunk->lines[i]);
        }
    }
    else
    {
        chunk->haslineinfo = false;
    }
    boa_bcemu_readuint64(emu, &count);
    chunk->constantlist.listitems = (BoaValue*)boa_sysmem_malloc(sizeof(BoaValue) * count);
    chunk->constantlist.listcount = count;
    chunk->constantlist.listcapacity = count;
    for(i = 0; i < count; i++)
    {
        boa_bcemu_readuint64(emu, &type);
        if(type == 0)
        {
            boa_bcemu_readnumber(emu, &dtmp);
            chunk->constantlist.listitems[i] = boa_value_makenumber(dtmp);
        }
        else
        {
            BoaEmulatedFile* ef = (BoaEmulatedFile*)emu;
            /* rewind to re-read type in loadobject */
            ef->position -= sizeof(BoaObject*);
            obj = boa_bcemu_readobject(state, emu, module);
            chunk->constantlist.listitems[i] = boa_value_fromobject(obj);
        }
    }
}

void boa_bcfile_writemodule(BoaModule* module, FILE* hnd)
{
    size_t i;
    bool disabled;
    disabled = false;
    boa_bcfile_writestring(hnd, module->name);
    boa_bcfile_writeuint32(hnd, (uint32_t)module->privatecount);
    boa_bcfile_writeuint8(hnd, (uint8_t)disabled);
    if(!disabled)
    {
        BoaTable* privates = &module->privatenames->innertable;
        if(privates->htcapacity > 0)
        {
            for(i = 0; i < (size_t)privates->htcapacity; i++)
            {
                if(privates->htentries[i].entkey != NULL)
                {
                    boa_bcfile_writestring(hnd, privates->htentries[i].entkey);
                    boa_bcfile_writeuint64(hnd, (uint64_t)boa_value_asnumber(privates->htentries[i].entvalue));
                }
            }
        }
    }
    boa_bcfile_writefunction(hnd, module->mainfunction);
}

BoaModule* boa_bcemu_initloadmodule(BoaState* state, const char* input)
{
    uint16_t j;
    uint16_t i;
    uint32_t privatescount;
    uint32_t modulecount;
    uint32_t bytecodeversion;
    uint8_t tmp;
    uint64_t tmp64;
    uint32_t utmp;
    bool enabled;
    BoaModule* first;
    BoaModule* module;
    BoaTable* privates;
    BoaEmulatedFile emu;
    boa_bcemu_initfile(&emu, input);
    boa_bcemu_readuint32(&emu, &utmp);
    if(utmp != BOA_CONFIG_BCMAGICNUMBER)
    {
        boa_state_raiseerror(state, "failed to read compiled code, unknown magic number");
        return NULL;
    }
    boa_bcemu_readuint32(&emu, &bytecodeversion);
    if(bytecodeversion > BOA_CONFIG_BCVERSION)
    {
        boa_state_raiseerror(state, "failed to read compiled code, unknown bytecode version '%i'", (int)bytecodeversion);
        return NULL;
    }
    boa_bcemu_readuint32(&emu, &modulecount);
    first = NULL;
    for(j = 0; j < modulecount; j++)
    {
        module = boa_object_makemodule(state, boa_bcemu_readstring(state, &emu));
        privates = &module->privatenames->innertable;
        boa_bcemu_readuint32(&emu, &privatescount);
        boa_bcemu_readuint8(&emu, &tmp);
        enabled = !((bool)tmp);
        module->privatevalues = (BoaValue*)boa_sysmem_malloc(privatescount * sizeof(BoaValue));
        module->privatecount = privatescount;
        for(i = 0; i < privatescount; i++)
        {
            module->privatevalues[i] = boa_value_makenull();
            if(enabled)
            {
                BoaString* name = boa_bcemu_readstring(state, &emu);
                boa_bcemu_readuint64(&emu, &tmp64);
                boa_table_set(privates, name, boa_value_makenumber(tmp64));
            }
        }
        module->mainfunction = boa_bcemu_readfunction(state, &emu, module);
        boa_map_setvalue(state->vmstate.modules, module->name, boa_value_fromobject(module));
        if(j == 0)
        {
            first = module;
        }
    }
#if 1
    boa_bcemu_readuint32(&emu, &utmp);
    if(utmp != BOA_CONFIG_BCENDNUMBER)
    {
        boa_state_raiseerror(state, "failed to read compiled code, unknown end number");
        return NULL;
    }
#endif
    return first;
}

void boa_classcallbackfile_oncleanup(BoaState* state, BoaUserdata* data, bool mark)
{
    BoaFileData* filedata;
    (void)state;
    if(mark)
    {
        return;
    }
    filedata = ((BoaFileData*)data->data);
    if(filedata->fdhandle != NULL)
    {
        boa_stream_destroy(filedata->fdhandle);
        filedata->fdhandle = NULL;
    }
}

bool boa_util_openmodeisvalid(const char* mode, size_t len)
{
    char first;
    if(len > 0)
    {
        first = mode[0];
        if(first == 'r' || first == 'w' || first == 'a')
        {
            return true;
        }
    }
    return false;
}

BoaValue boa_objfnfile_constructor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* path;
    BoaString* mode;
    const char* strpath;
    const char* strmode;
    BoaStream* strm;
    BoaFileData* data;
    BoaClass* fileclass;
    BoaChecker check;
    (void)argc;
    BOA_CHECK_INIT(state, &check, "File::constructor", argc, args);
    BOA_CHECK_REQUIREARGS(&check, 2);
    if(argc < 2)
    {
        return boa_vm_raiseexception(state, state->exceptions.stdargumenterror, "expected two arguments [path, mode]");
    }
    BOA_CHECK_CHECKARGTYPE(&check, 0, boa_value_isstring);
    path = boa_value_asstring(args[0]);
    BOA_CHECK_CHECKARGTYPE(&check, 1, boa_value_isstring);
    mode = boa_value_asstring(args[1]);
    strpath = boa_string_getdata(path);
    strmode = boa_string_getdata(mode);
    if(!boa_util_openmodeisvalid(strmode, boa_string_getlength(mode)))
    {
        return boa_vm_raiseexception(state, state->exceptions.stdargumenterror, "invalid open mode '%s'", strmode);
    }
    strm = boa_stream_makeopenfile(strpath, strmode);
    if(strm == NULL)
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "failed to open file %s with mode %s (C error: %s)", strpath, strmode, strerror(errno));
        return boa_value_makenull();
    }
    if(!boa_value_isinstance(instance))
    {
        fileclass = boa_value_asclass(instance);
        instance = boa_value_fromobject(boa_object_makeinstance(state, fileclass));
    }
    data = (BoaFileData*)boa_userdata_insertdata(state, instance, sizeof(BoaFileData), boa_classcallbackfile_oncleanup);
    data->path = (char*)strpath;
    data->fdhandle = strm;
    return instance;
}

BoaValue boa_objfnfile_close(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaFileData* data;
    (void)state;
    (void)argc;
    (void)args;
    data = (BoaFileData*)boa_userdata_extractdata(instance);
    boa_stream_destroy(data->fdhandle);
    data->fdhandle = NULL;
    return boa_value_makenull();
}

BoaValue boa_objfnfile_staticopen(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    return boa_objfnfile_constructor(state, instance, argc, args);
}

BoaValue boa_objfnfile_staticexists(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* fname;
    const char* strfname;
    (void)state;
    (void)instance;
    (void)argc;
    fname = boa_value_asstring(args[0]);
    strfname = boa_string_getdata(fname);
    return boa_value_makebool(boa_util_fsfileexists(strfname));
}

BoaValue boa_objfnfile_staticcreate(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* path;
    const char* strpath;
    FILE* fhnd;
    (void)instance;
    (void)argc;
    path = boa_value_asstring(args[0]);
    strpath = boa_string_getdata(path);
    fhnd = fopen(strpath, "w");
    if(fhnd == NULL)
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "failed to create file %s", strpath);
        return boa_value_makenull();
    }
    fclose(fhnd);
    return boa_value_makenull();
}

BoaValue boa_objfnfile_writevalvalue(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t i;
    BoaFileData* lfd;
    (void)state;
    lfd = (BoaFileData*)boa_userdata_extractdata(instance);
    for(i = 0; i < argc; i++)
    {
        boa_value_printvalue(lfd->fdhandle, args[i], false);
    }
    return boa_value_makenull();
}

BoaValue boa_objfnfile_writevalstring(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t wr;
    size_t maxlen;
    BoaString* string;
    BoaFileData* data;
    (void)state;
    string = boa_value_asstring(args[0]);
    if(string == NULL)
    {
        return boa_value_makenull();
    }
    string = boa_value_asstring(args[0]);
    maxlen = boa_string_getlength(string);
    if(argc > 1)
    {
        maxlen = boa_value_asnumber(args[1]);
    }
    data = (BoaFileData*)boa_userdata_extractdata(instance);
    wr = boa_stream_putlen(data->fdhandle, boa_string_getdata(string), maxlen);
    return boa_value_makenumber(wr);
}

BoaValue boa_objfnfile_readallinstance(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    bool havesizeparam;
    size_t howmuch;
    BoaFileData* data;
    BoaString* result;
    (void)argc;
    (void)args;
    howmuch = 0;
    havesizeparam = false;
    if(argc > 0)
    {
        howmuch = boa_value_asnumber(args[0]);
        havesizeparam = true;
    }
    data = (BoaFileData*)boa_userdata_extractdata(instance);
    result = boa_stream_readuntil(data->fdhandle, state, havesizeparam, howmuch);
    return boa_value_fromobject(result);
}

BoaValue boa_objfnfile_staticreadall(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    bool havesizeparam;
    size_t howmuch;
    const char* filename;
    FILE* hnd;
    BoaString* result;
    BoaStream pr;
    (void)instance;
    (void)argc;
    (void)args;
    howmuch = 0;
    havesizeparam = false;
    filename = boa_string_getdata(boa_value_asstring(args[0]));
    if(argc > 1)
    {
        howmuch = boa_value_asnumber(args[1]);
        havesizeparam = true;
    }
    hnd = fopen(filename, "rb");
    if(!hnd)
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "cannot open '%s' for reading", filename);
        return boa_value_makenull();
    }
    boa_stream_makestackio(&pr, hnd, true);
    result = boa_stream_readuntil(&pr, state, havesizeparam, howmuch);
    boa_stream_destroy(&pr);
    return boa_value_fromobject(result);
}

BoaValue boa_objfnfile_staticunlink(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaString* path;
    (void)instance;
    (void)argc;
    path = boa_value_asstring(args[0]);
    if(boa_util_unlink(boa_string_getdata(path)) != 0)
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "unlink(%s): %s", boa_string_getdata(path), strerror(errno));
    }
    return boa_value_makenull();
}

BoaValue boa_objfnfile_staticdirname(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t len;
    char* dn;
    BoaString* path;
    (void)instance;
    (void)argc;
    path = boa_value_asstring(args[0]);
    dn = boa_util_dirname(boa_string_getdata(path), &len);
    if(dn == NULL)
    {
        return boa_value_makenull();
    }
    return boa_value_fromobject(boa_string_take(state, dn, len));
}

BoaValue boa_objfnfile_readline(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    char ch;
    int64_t written;
    BoaString* res;
    BoaFileData* data;
    (void)argc;
    (void)args;
    written = 0;
    data = (BoaFileData*)boa_userdata_extractdata(instance);
    res = boa_string_makeemptystring(state, 64, false);
    while(true)
    {
        ch = boa_stream_getc(data->fdhandle);
        if(ch == EOF)
        {
            if(written == 0)
            {
                return boa_value_makenull();
            }
            break;
        }
        if(ch == '\n')
        {
            written++;
            break;
        }
        boa_string_appendbyte(res, ch);
        written++;
    }
    if(written == 0)
    {
        return boa_value_makenull();
    }
    return boa_value_fromobject(res);
}

BoaValue boa_objfndirectory_exists(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    const char* directoryname;
    struct stat buffer;
    (void)state;
    (void)instance;
    (void)argc;
    directoryname = boa_string_getdata(boa_value_asstring(args[0]));
    return boa_value_makebool(stat(directoryname, &buffer) == 0);
}

BoaValue boa_objfndirectory_chdir(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    const char* cstr;
    BoaString* str;
    (void)instance;
    (void)argc;
    str = boa_value_asstring(args[0]);
    cstr = boa_string_getdata(str);
    if(!boa_util_chdir(cstr))
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "chdir(%s): %s", cstr, strerror(errno));
        return boa_value_makenull();
    }
    return boa_value_makenull();
}

BoaValue boa_objfndirectory_mkdir(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int mode;
    const char* cstr;
    BoaString* str;
    mode = 0644;
    (void)instance;
    (void)argc;
    str = boa_value_asstring(args[0]);
    cstr = boa_string_getdata(str);
    if(argc > 1)
    {
        mode = boa_value_asnumber(args[1]);
    }
    if(!boa_util_mkdir(cstr, mode))
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "mkdir(%s): %s", cstr, strerror(errno));
        return boa_value_makenull();
    }
    return boa_value_makenull();
}

BoaValue boa_objfndirectory_rmdir(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    const char* cstr;
    BoaString* str;
    (void)instance;
    (void)argc;
    str = boa_value_asstring(args[0]);
    cstr = boa_string_getdata(str);
    if(!boa_util_rmdir(cstr))
    {
        boa_vm_raiseexception(state, state->exceptions.stdioerror, "rmdir(%s): %s", cstr, strerror(errno));
        return boa_value_makenull();
    }
    return boa_value_makenull();
}

BoaValue boa_objfndirectory_getcwd(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    char cwdbuf[512];
    const char* cstr;
    (void)instance;
    (void)argc;
    (void)args;
    cstr = boa_util_getcwd(cwdbuf, 512);
    if(!cstr)
    {
        return boa_value_makenull();
    }
    return boa_value_fromobject(boa_string_copy(state, cwdbuf));
}

BoaValue boa_objfndirectory_read(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    size_t fnlen;
    size_t topdlen;
    BoaString* res;
    BoaString* topdstring;
    BoaArray* array;
    const char* topdname;
    const char* fname;
    BoaStream pr;
    BoaFSDirReader rd;
    BoaFSDirItem ent;
    (void)instance;
    (void)argc;
    topdstring = boa_value_asstring(args[0]);
    topdname = boa_string_getdata(topdstring);
    topdlen = boa_string_getlength(topdstring);
    if(fslib_diropen(&rd, topdname))
    {
        array = boa_array_make(state);
        while(fslib_dirread(&rd, &ent))
        {
            fname = ent.name;
            fnlen = strlen(fname);
            if(strcmp(fname, "..") == 0 || strcmp(fname, ".") == 0)
            {
                continue;
            }
            #if 1
                boa_stream_makestackstring(&pr);
                boa_stream_putlen(&pr, topdname, topdlen);
                if(topdname[topdlen-1] != '/')
                {
                    boa_stream_putlen(&pr, "/", 1);
                }
                boa_stream_putlen(&pr, fname, fnlen);
                res = boa_stream_takestring(state, &pr);
            #else
                res = boa_string_copylen(state, fname, fnlen);
            #endif
            boa_array_push(array, boa_value_fromobject(res));
        }
        fslib_dirclose(&rd);
        return boa_value_fromobject(array);
    }
    else
    {
        boa_state_raiseerror(state, "cannot open directory '%s'", topdname);
    }
    return boa_value_makenull();
}

void boa_corelib_installfile(BoaState* state)
{
    BoaClass* klass;
    {
        klass = boa_class_make(state, "File", state->stdobjectclass);
        boa_class_bindconstructor(klass, boa_objfnfile_constructor);
        boa_class_bindstaticmethod(klass, "open", boa_objfnfile_staticopen);
        boa_class_bindstaticmethod(klass, "exists", boa_objfnfile_staticexists);
        boa_class_bindstaticmethod(klass, "create", boa_objfnfile_staticcreate);
        boa_class_bindstaticmethod(klass, "read", boa_objfnfile_staticreadall);
        boa_class_bindstaticmethod(klass, "unlink", boa_objfnfile_staticunlink);
        boa_class_bindstaticmethod(klass, "dirname", boa_objfnfile_staticdirname);
        boa_class_bindmethod(klass, "close", boa_objfnfile_close);
        boa_class_bindmethod(klass, "write", boa_objfnfile_writevalvalue);
        boa_class_bindmethod(klass, "writeString", boa_objfnfile_writevalstring);
        boa_class_bindmethod(klass, "readAll", boa_objfnfile_readallinstance);
        boa_class_bindmethod(klass, "readLine", boa_objfnfile_readline);
    }
    {
        klass = boa_class_make(state, "Directory", state->stdobjectclass);
        boa_class_bindstaticmethod(klass, "exists", boa_objfndirectory_exists);
        boa_class_bindstaticmethod(klass, "read", boa_objfndirectory_read);
        boa_class_bindstaticmethod(klass, "chdir", boa_objfndirectory_chdir);
        boa_class_bindstaticmethod(klass, "mkdir", boa_objfndirectory_mkdir);
        boa_class_bindstaticmethod(klass, "rmdir", boa_objfndirectory_rmdir);
        boa_class_bindstaticmethod(klass, "cwd", boa_objfndirectory_getcwd);
    }
}

BoaValue boa_objfngc_memoryused(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(state->bytesallocated);
}

BoaValue boa_objfngc_nextround(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(state->gcnextgc);
}

BoaValue boa_objfngc_trigger(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    int64_t collected;
    (void)instance;
    (void)argc;
    (void)args;
    state->gcallowgc = true;
    collected = boa_collect_garbage(state);
    state->gcallowgc = false;
    return boa_value_makenumber(collected);
}

void boa_corelib_installgc(BoaState* state)
{
    BoaClass* klass;
    klass = boa_class_make(state, "GC", state->stdobjectclass);
    boa_class_bindstaticgetter(klass, "memoryUsed", boa_objfngc_memoryused);
    boa_class_bindstaticgetter(klass, "nextRound", boa_objfngc_nextround);
    boa_class_bindstaticmethod(klass, "trigger", boa_objfngc_trigger);
}

void boa_state_openstdlibs(BoaState* state)
{
    boa_corelib_installmath(state);
    boa_corelib_installfile(state);
    boa_corelib_installgc(state);
}

BoaValue boa_objfnmath_abs(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(fabs(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_hypot(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(hypot(boa_value_asnumber(args[0]), boa_value_asnumber(args[1])));
}

BoaValue boa_objfnmath_cos(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(cos(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_sin(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(sin(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_tan(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(tan(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_acos(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(acos(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_asin(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(asin(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_atan(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(atan(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_atan2(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(atan2(boa_value_asnumber(args[0]), boa_value_asnumber(args[1])));
}

BoaValue boa_objfnmath_floor(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(floor(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_ceil(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(ceil(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_round(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaNumber value;
    int places;
    (void)state;
    (void)instance;
    value = boa_value_asnumber(args[0]);
    if(argc > 1)
    {
        places = (int)pow(10, boa_value_asnumber(args[1]));
        return boa_value_makenumber(round(value * places) / places);
    }
    return boa_value_makenumber(round(value));
}

BoaValue boa_objfnmath_min(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)instance;
    return boa_value_makenumber(fmin(boa_value_asnumber(args[0]), boa_value_asnumber(args[1])));
}

BoaValue boa_objfnmath_max(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)argc;
    (void)instance;
    return boa_value_makenumber(fmax(boa_value_asnumber(args[0]), boa_value_asnumber(args[1])));
}

BoaValue boa_objfnmath_mid(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    BoaNumber x;
    BoaNumber y;
    BoaNumber z;
    (void)state;
    (void)argc;
    (void)instance;
    x = boa_value_asnumber(args[0]);
    y = boa_value_asnumber(args[1]);
    z = boa_value_asnumber(args[2]);
    if(x > y)
    {
        return boa_value_makenumber(fmax(x, fmin(y, z)));
    }
    else
    {
        return boa_value_makenumber(fmax(y, fmin(x, z)));
    }
}

BoaValue boa_objfnmath_toRadians(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_value_asnumber(args[0]) * BOA_CONST_M_PI / 180.0);
}

BoaValue boa_objfnmath_toDegrees(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(boa_value_asnumber(args[0]) * 180.0 / BOA_CONST_M_PI);
}

BoaValue boa_objfnmath_sqrt(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(sqrt(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_log(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(exp(boa_value_asnumber(args[0])));
}

BoaValue boa_objfnmath_exp(BoaState* state, BoaValue instance, size_t argc, BoaValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return boa_value_makenumber(exp(boa_value_asnumber(args[0])));
}

void boa_corelib_installmath(BoaState* state)
{
    BoaClass* klass;
    {
        klass = boa_class_make(state, "Math", state->stdobjectclass);
        boa_class_setstaticfield(klass, "Pi", boa_value_makenumber(BOA_CONST_M_PI));
        boa_class_setstaticfield(klass, "Tau", boa_value_makenumber(BOA_CONST_M_PI * 2));
        boa_class_bindstaticmethod(klass, "abs", boa_objfnmath_abs);
        boa_class_bindstaticmethod(klass, "hypot", boa_objfnmath_hypot);
        boa_class_bindstaticmethod(klass, "sin", boa_objfnmath_sin);
        boa_class_bindstaticmethod(klass, "cos", boa_objfnmath_cos);
        boa_class_bindstaticmethod(klass, "tan", boa_objfnmath_tan);
        boa_class_bindstaticmethod(klass, "asin", boa_objfnmath_asin);
        boa_class_bindstaticmethod(klass, "acos", boa_objfnmath_acos);
        boa_class_bindstaticmethod(klass, "atan", boa_objfnmath_atan);
        boa_class_bindstaticmethod(klass, "atan2", boa_objfnmath_atan2);
        boa_class_bindstaticmethod(klass, "floor", boa_objfnmath_floor);
        boa_class_bindstaticmethod(klass, "ceil", boa_objfnmath_ceil);
        boa_class_bindstaticmethod(klass, "round", boa_objfnmath_round);
        boa_class_bindstaticmethod(klass, "min", boa_objfnmath_min);
        boa_class_bindstaticmethod(klass, "max", boa_objfnmath_max);
        boa_class_bindstaticmethod(klass, "mid", boa_objfnmath_mid);
        boa_class_bindstaticmethod(klass, "toRadians", boa_objfnmath_toRadians);
        boa_class_bindstaticmethod(klass, "toDegrees", boa_objfnmath_toDegrees);
        boa_class_bindstaticmethod(klass, "sqrt", boa_objfnmath_sqrt);
        boa_class_bindstaticmethod(klass, "log", boa_objfnmath_log);
        boa_class_bindstaticmethod(klass, "exp", boa_objfnmath_exp);
    }
}

bool boa_fiber_ensureframes(BoaState* state, BoaFiber* fiber)
{
    size_t incsize;
    size_t inccap;
    if(fiber == NULL)
    {
        boa_vm_raisefatalerror(state, "no Fiber to run on");
        return true;
    }
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        inccap = (fiber->framecapacity * 2);
        incsize = (sizeof(BoaCallFrame) * inccap);
        fiber->framevals = (BoaCallFrame*)boa_sysmem_realloc(fiber->framevals, incsize);
        if(fiber->framevals == NULL)
        {
            return false;
        }
        fiber->framecapacity = inccap;
    }
    return false;
}

BoaCallFrame* boa_state_setupcallonframe(BoaState* state, BoaFuncScript* callee, BoaValue* arguments, size_t argc)
{
    size_t i;
    size_t ai;
    size_t ti;
    size_t targetargcount;
    size_t j;
    bool vararg;
    BoaValue* start;
    BoaCallFrame* frame;
    BoaArray* array;
    BoaFiber* fiber;
    fiber = state->vmstate.fiber;
    if(callee == NULL)
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot setup a call for a null value");
        return NULL;
    }
    if(boa_fiber_ensureframes(state, fiber))
    {
        return NULL;
    }
    start = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
    boa_fiber_ensureregisters(fiber, start - fiber->registeritems + callee->maxregisters);
    frame = &fiber->framevals[fiber->framecount++];
    frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
    for(i = argc + 1; i < callee->maxregisters; i++)
    {
        frame->slots[i] = boa_value_makenull();
    }
    frame->slots[0] = boa_value_fromobject(callee);
    for(ai = 0; ai < argc; ai++)
    {
        frame->slots[ai + 1] = arguments[ai];
    }
    targetargcount = callee->argcount;
    vararg = callee->vararg;
    if(targetargcount > argc)
    {
        for(ti = argc; ti < targetargcount; ti++)
        {
            *(frame->slots + ti + 1) = boa_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = boa_value_fromobject(boa_array_make(state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && boa_value_isvargarray(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            array = &boa_object_makevararray(state)->innerarray;
            boa_state_pushroot(state, (BoaObject*)array);
            boa_dynlistval_ensuresize(&array->innerlist, argc - targetargcount + 1);
            j = 0;
            for(ti = targetargcount - 1; ti < argc; ti++)
            {
                array->innerlist.listitems[j++] = *(frame->slots + ti + 1);
            }
            *(frame->slots + targetargcount) = boa_value_fromobject(array);
            boa_state_poproot(state);
        }
    }
    frame->ip = callee->chunk.compiledcodechunk;
    frame->closure = NULL;
    frame->function = callee;
    frame->resultignored = false;
    frame->returntoc = true;
    frame->returnaddress = NULL;
    return frame;
}

BoaResult boa_state_execcallonframe(BoaState* state, BoaCallFrame* frame)
{
    BoaFiber* fiber;
    BoaResult result;
    if(frame == NULL)
    {
        return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
    }
    fiber = state->vmstate.fiber;
    result = boa_state_execfiber(state, fiber);
    if(!boa_value_isnull(fiber->error))
    {
        result.result = fiber->error;
    }
    return result;
}

BoaResult boa_state_callfunction(BoaState* state, BoaFuncScript* callee, BoaValue* arguments, size_t argc)
{
    return boa_state_execcallonframe(state, boa_state_setupcallonframe(state, callee, arguments, argc));
}

BoaResult boa_state_callclosure(BoaState* state, BoaFuncClosure* callee, BoaValue* arguments, size_t argc)
{
    BoaCallFrame* frame;
    frame = boa_state_setupcallonframe(state, callee->function, arguments, argc);
    if(frame == NULL)
    {
        return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
    }
    frame->closure = callee;
    return boa_state_execcallonframe(state, frame);
}

BoaResult boa_state_callmethod(BoaState* state, BoaString* name, BoaValue instance, BoaValue callee, BoaValue* arguments, size_t argc)
{
    size_t i;
    size_t ai;
    BoaObjType type;
    BoaFiber* fiber;
    BoaValue* start;
    BoaValue* slot;
    BoaValue value;
    BoaFuncNative* method;
    BoaClass* klass;
    BoaInstance* inst;
    BoaFuncBound* boundmethod;
    BoaValue mth;
    if(boa_value_isobject(callee))
    {
        if(boa_jmpstate_setnativeexit())
        {
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        }
        type = boa_value_objtype(callee);
        if(type == BOA_OBJTYPE_FUNCSCRIPT)
        {
            return boa_state_callfunction(state, boa_value_asfuncscript(callee), arguments, argc);
        }
        else if(type == BOA_OBJTYPE_FUNCCLOSURE)
        {
            return boa_state_callclosure(state, boa_value_asfuncclosure(callee), arguments, argc);
        }
        fiber = state->vmstate.fiber;
        if(boa_fiber_ensureframes(state, fiber))
        {
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        }
        start = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
        boa_fiber_ensureregisters(fiber, start - fiber->registeritems + 3 + argc);
        slot = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
        for(i = argc; i < argc + 3; i++)
        {
            *(slot + i) = boa_value_makenull();
        }
        *slot = instance;
        if(type != BOA_OBJTYPE_CLASS)
        {
            for(ai = 0; ai < argc; ai++)
            {
                *(slot + ai + 1) = arguments[ai];
            }
        }
        if(BOA_UNLIKELY(state->config.traceexecution))
        {
            if(!state->config.traceinstsonly)
            {
                boa_debug_traceprintvalue(state->config.desttrace, "<vm:slots>", fiber->framecount, argc, slot);
            }
        }
        switch(type)
        {
            case BOA_OBJTYPE_FUNCNATIVE:
                {
                    /* for some reason, single line expression doesn't work */
                    value = boa_value_asfuncnative(callee)->natfuncptr(state, boa_value_makenull(), argc, slot + 1);
                    return boa_result_make(BOA_STATUS_OK, value);
                }
                break;
            case BOA_OBJTYPE_FUNCNATMETHOD:
                {
                    method = boa_value_asfuncmethod(callee);
                    /* For some reason, single line expression doesn't work */
                    value = method->natfuncptr(state, *slot, argc, slot + 1);
                    return boa_result_make(BOA_STATUS_OK, value);
                }
                break;
            case BOA_OBJTYPE_CLASS:
                {
                    klass = boa_value_asclass(callee);
                    inst = boa_object_makeinstance(state, klass);
                    if(klass->mthconstructor != NULL)
                    {
                        boa_state_callmethod(state, state->strings.strconstructor, *slot, boa_value_fromobject(klass->mthconstructor), arguments, argc);
                    }
                    return boa_result_make(BOA_STATUS_OK, boa_value_fromobject(inst));
                }
                break;
            case BOA_OBJTYPE_FUNCBOUNDMETHOD:
                {
                    boundmethod = boa_value_asfuncboundmethod(callee);
                    mth = boundmethod->method;
                    if(boa_value_isfuncmethod(mth))
                    {
                        /* For some reason, single line expression doesn't work */
                        value = boa_value_asfuncmethod(mth)->natfuncptr(state, boundmethod->receiver, argc, slot + 1);
                        return boa_result_make(BOA_STATUS_OK, value);
                    }
                    else
                    {
                        *slot = boundmethod->receiver;
                        return boa_state_callfunction(state, boa_value_asfuncscript(mth), arguments, argc);
                    }
                }
                break;
            default:
                {
                }
                break;
        }
    }
    if(boa_value_isnull(callee))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "cannot call null value method '%s'", boa_string_getdata(name));
    }
    else
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "can only call functions and classes");
    }
    return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
}

BoaResult boa_state_callvalue(BoaState* state, BoaString* name, BoaValue callee, BoaValue* arguments, size_t argc)
{
    return boa_state_callmethod(state, name, callee, callee, arguments, argc);
}

BoaResult boa_state_findandcallmethod(BoaState* state, BoaValue callee, BoaString* mthname, BoaValue* arguments, size_t argc)
{
    bool ok;
    BoaValue method;
    BoaFiber* fiber;
    BoaClass* klass;
    BoaInstance* inst;
    fiber = state->vmstate.fiber;
    ok = false;
    if(fiber == NULL)
    {
        boa_vm_raisefatalerror(state, "no Fiber to run on");
        return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
    }
    klass = boa_state_getclassfor(state, callee);
    if(boa_value_isinstance(callee))
    {
        inst = boa_value_asinstance(callee);
        if(boa_table_getentry(&inst->fields, mthname, &method))
        {
            ok = true;
        }
    }
    else if(klass != NULL)
    {
        if(boa_table_getentry(&klass->mthtable, mthname, &method))
        {
            ok = true;
        }
    }
    if(ok)
    {
        return boa_state_callmethod(state, mthname, callee, method, arguments, argc);
    }
    return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
}

BoaString* boa_value_tostrinvoketostring(BoaState* state, BoaValue object, size_t indentation, bool explicitfail)
{
    size_t needed;
    BoaValue tmpv;
    BoaValue* tmptr;
    BoaFiber* fiber;
    BoaInstance* inst;
    BoaFuncScript* function;
    BoaChunk* chunk;
    int constant;
    BoaCallFrame* frame;
    BoaResult result;
    fiber = state->vmstate.fiber;
    /*
    * NB: only do this with instances for now.
    */
    if(!boa_value_isinstance(object))
    {
        goto failed;
    }
    if(boa_value_isinstance(object))
    {
        inst = boa_value_asinstance(object);
        if(!boa_table_getentry(&inst->klass->mthtable, state->strings.strtostring, &tmpv))
        {
            return NULL;
        }       
    }
    if(boa_fiber_ensureframes(state, fiber))
    {
        goto failed;
    }
    function = state->apifunction;
    if(function == NULL)
    {
        function = state->apifunction = boa_object_makefunction(state, fiber->module);
        function->chunk.haslineinfo = false;
        function->name = state->apiname;
        chunk = &function->chunk;
        chunk->compiledcodecount = 0;
        chunk->constantlist.listcount = 0;
        function->maxregisters = 3;
        constant = boa_chunk_addconstant(state, chunk, boa_value_fromobject(state->strings.strtostring));
        boa_chunk_push(chunk, BOA_REG_FORMABCINST(BOA_OPCODE_INVOKE, 1, 2, constant), 1);
        boa_chunk_push(chunk, BOA_REG_FORMABCINST(BOA_OPCODE_RETURN, 1, 0, 0), 1);
    }
    tmptr = fiber->registeritems;
    if(fiber->framecount > 0)
    {
        tmptr = (fiber->framevals[fiber->framecount - 1].slots + (int)fiber->framevals[fiber->framecount - 1].function->maxregisters);
    }
    needed = (tmptr - fiber->registeritems + function->maxregisters);
    boa_fiber_ensureregisters(fiber, needed);
    frame = &fiber->framevals[fiber->framecount++];
    frame->ip = function->chunk.compiledcodechunk;
    frame->closure = NULL;
    frame->function = function;
    /* "duplicated" code due to boa_fiber_ensureregisters messing with register pointers */
    frame->slots = fiber->registeritems;
    if(fiber->framecount > 1)
    {
        frame->slots = (fiber->framevals[fiber->framecount - 2].slots + (int)fiber->framevals[fiber->framecount - 2].function->maxregisters);
    }
    frame->resultignored = false;
    frame->returntoc = true;
    frame->returnaddress = NULL;
    frame->slots[0] = boa_value_fromobject(function);
    frame->slots[1] = object;
    frame->slots[2] = boa_value_makenumber(indentation);
    result = boa_state_execfiber(state, fiber);
    if(result.type != BOA_STATUS_OK)
    {
        if(explicitfail)
        {
            return state->strings.strnull;
        }
        return NULL;
    }
    if(!boa_value_isstring(result.result))
    {
        if(explicitfail)
        {
            return boa_string_copy(state, "invalid toString()");
        }
        return NULL;
    }
    return boa_value_asstring(result.result);
    failed:
        if(explicitfail)
        {
            return state->strings.strnull;
        }
        return NULL;
}

BoaString* boa_value_tostring(BoaState* state, BoaValue object, size_t indentation)
{
    BoaValue* slot;
    BoaStream pr;
    BoaException* exception;
    BoaString* message;
    if(boa_value_isstring(object))
    {
        return boa_value_asstring(object);
    }
    else if(!boa_value_isobject(object))
    {
        if(boa_value_isnull(object))
        {
            return state->strings.strnull;
        }
        else if(boa_value_isnumber(object))
        {
            return boa_value_asstring(boa_string_numbertostring(state, boa_value_asnumber(object)));
        }
        else if(boa_value_isbool(object))
        {
            if(boa_value_asbool(object))
            {
                return state->strings.strtrue;
            }
            else
            {
                return state->strings.strfalse;
            }
        }
    }
    else if(boa_value_isreference(object))
    {
        slot = boa_value_asreference(object)->slot;
        if(slot == NULL)
        {
            return state->strings.strnull;
        }
        return boa_value_tostring(state, *slot, 0);
    }
    else if(boa_value_isexception(object))
    {
        exception = boa_value_asexception(object);
        message = boa_value_tostring(state, exception->message, 0);
        if(exception->baseclass == NULL || exception->baseclass->name == NULL)
        {
            return message;
        }
        boa_stream_makestackstring(&pr);
        boa_stream_printf(&pr, "%s: %s", boa_string_getdata(exception->baseclass->name), boa_string_getdata(message));
        return boa_stream_takestring(state, &pr);
    }
    return boa_value_tostrinvoketostring(state, object, indentation, true);
}

BoaValue boa_state_callnew(BoaState* state, BoaString* cname, BoaValue* args, size_t argc)
{
    BoaValue value;
    BoaClass* klass;
    if(!boa_map_getvalue(state->vmstate.globals, cname, &value))
    {
        boa_vm_raiseexception(state, state->exceptions.stdexception, "failed to create instance of class %s: class not found", boa_string_getdata(cname));
        return boa_value_makenull();
    }
    klass = boa_value_asclass(value);
    if(klass->mthconstructor == NULL)
    {
        return boa_value_fromobject(boa_object_makeinstance(state, klass));
    }
    return boa_state_callmethod(state, cname, value, value, args, argc).result;
}

BoaObject* boa_object_allocobject(BoaState* state, size_t size, BoaObjType type)
{
    BoaObject* object;
    object = (BoaObject*)boa_sysmem_malloc(size);
    object->pstate = state;
    object->type = type;
    object->marked = false;
    object->next = state->vmstate.objects;
    state->vmstate.objects = object;
#if defined(BOA_CONFIG_DEBUGLOGALLOCATION) && (BOA_CONFIG_DEBUGLOGALLOCATION == 1)
    printf("%p allocate %ld for %s\n", (void*)object, size, boa_value_objtypename(type));
#endif
    return object;
}

BoaFuncScript* boa_object_makefunction(BoaState* state, BoaModule* module)
{
    BoaFuncScript* function;
    function = (BoaFuncScript*)boa_object_allocobject(state, sizeof(BoaFuncScript), BOA_OBJTYPE_FUNCSCRIPT);
    boa_chunk_init(&function->chunk);
    function->name = NULL;
    function->argcount = 0;
    function->upvaluecount = 0;
    function->maxregisters = 0;
    function->module = module;
    function->vararg = false;
    return function;
}

BoaValue boa_function_getname(BoaState* state, BoaValue instance)
{
    BoaString* name;
    BoaField* field;
    name = NULL;
    switch(boa_value_objtype(instance))
    {
        case BOA_OBJTYPE_FUNCSCRIPT:
            {
                name = boa_value_asfuncscript(instance)->name;
            }
            break;
        case BOA_OBJTYPE_FUNCCLOSURE:
            {
                name = boa_value_asfuncclosure(instance)->function->name;
            }
            break;
        case BOA_OBJTYPE_CLSPROTOTYPE:
            {
                name = boa_value_asclsproto(instance)->function->name;
            }
            break;
        case BOA_OBJTYPE_FIELD:
            {
                field = boa_value_asfield(instance);
                if(field->getter != NULL)
                {
                    return boa_function_getname(state, boa_value_fromobject(field->getter));
                }
                return boa_function_getname(state, boa_value_fromobject(field->setter));
            }
            break;
        case BOA_OBJTYPE_FUNCNATIVE:
            {
                name = boa_value_asfuncnative(instance)->name;
            }
            break;
        case BOA_OBJTYPE_FUNCNATMETHOD:
            {
                name = boa_value_asfuncmethod(instance)->name;
            }
            break;
        case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            {
                return boa_function_getname(state, boa_value_asfuncboundmethod(instance)->method);
            }
            break;
        default:
            {
            }
            break;
    }
    if(name == NULL)
    {
        return boa_string_valformat(state, "function #", *((BoaNumber*)boa_value_asobject(instance)));
    }
    return boa_string_valformat(state, "function @", boa_value_fromobject(name));
}

BoaUpvalue* boa_object_makeupvalue(BoaState* state, BoaValue* slot)
{
    BoaUpvalue* upvalue;
    upvalue = (BoaUpvalue*)boa_object_allocobject(state, sizeof(BoaUpvalue), BOA_OBJTYPE_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = boa_value_makenull();
    upvalue->next = NULL;
    return upvalue;
}

BoaFuncClosure* boa_object_makeclosure(BoaState* state, BoaFuncScript* function)
{
    size_t i;
    BoaFuncClosure* closure;
    BoaUpvalue** upvalues;
    closure = (BoaFuncClosure*)boa_object_allocobject(state, sizeof(BoaFuncClosure), BOA_OBJTYPE_FUNCCLOSURE);
    closure->function = function;
    /* to prevent GC crashes */
    closure->upvaluecount = 0;
    boa_state_pushroot(state, (BoaObject*)closure);
    upvalues = (BoaUpvalue**)boa_sysmem_malloc(function->upvaluecount * sizeof(BoaUpvalue*));
    boa_state_poproot(state);
    for(i = 0; i < function->upvaluecount; i++)
    {
        upvalues[i] = NULL;
    }
    closure->closureupvalueitems = upvalues;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

BoaClsPrototype* boa_object_makeclsproto(BoaState* state, BoaFuncScript* function)
{
    BoaClsPrototype* closure;
    closure = (BoaClsPrototype*)boa_object_allocobject(state, sizeof(BoaClsPrototype), BOA_OBJTYPE_CLSPROTOTYPE);
    boa_state_pushroot(state, (BoaObject*)closure);
    closure->indexes = (uint32_t*)boa_sysmem_malloc(function->upvaluecount * sizeof(uint32_t));
    closure->local = (bool*)boa_sysmem_malloc(function->upvaluecount * sizeof(bool));
    boa_state_poproot(state);
    closure->function = function;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

BoaFuncNative* boa_object_makenativefunc(BoaState* state, BoaNativeFunctionFn function, BoaString* name)
{
    BoaFuncNative* native;
    native = (BoaFuncNative*)boa_object_allocobject(state, sizeof(BoaFuncNative), BOA_OBJTYPE_FUNCNATIVE);
    native->natfuncptr = function;
    native->name = name;
    return native;
}

BoaFuncNative* boa_object_makenativemethod(BoaState* state, BoaNativeFunctionFn method, BoaString* name)
{
    BoaFuncNative* native;
    native = (BoaFuncNative*)boa_object_allocobject(state, sizeof(BoaFuncNative), BOA_OBJTYPE_FUNCNATMETHOD);
    native->natfuncptr = method;
    native->name = name;
    return native;
}

BoaFiber* boa_object_makefiber(BoaState* state, BoaModule* module, BoaFuncScript* function)
{
    size_t i;
    size_t registersallocated;
    BoaValue* registers;
    BoaCallFrame* framevals;
    BoaFiber* fiber;
    BoaCallFrame* frame;
    /* Allocate in advance, just in case GC is triggered */
    registersallocated = function == NULL ? 1 : boa_util_closestpoweroftwo(function->maxregisters);
    registers = (BoaValue*)boa_sysmem_malloc(registersallocated * sizeof(BoaValue));
    framevals = (BoaCallFrame*)boa_sysmem_malloc(BOA_CONFIG_INITIALCALLFRAMES * sizeof(BoaCallFrame));
    fiber = (BoaFiber*)boa_object_allocobject(state, sizeof(BoaFiber), BOA_OBJTYPE_FIBER);
    if(module->mainfiber == NULL)
    {
        module->mainfiber = fiber;
    }
    fiber->registeritems = registers;
    for(i = 0; i < registersallocated; i++)
    {
        fiber->registeritems[i] = boa_value_makenull();
    }
    fiber->registersallocated = registersallocated;
    fiber->framevals = framevals;
    fiber->framecapacity = BOA_CONFIG_INITIALCALLFRAMES;
    fiber->parent = NULL;
    fiber->framecount = function == NULL ? 0 : 1;
    fiber->argcount = 0;
    fiber->module = module;
    fiber->catcher = false;
    fiber->caught = false;
    fiber->error = boa_value_makenull();
    fiber->openupvalues = NULL;
    fiber->muststop = false;
    fiber->returnaddress = NULL;
    fiber->handleritems = NULL;
    fiber->handlercount = 0;
    fiber->handlercapacity = 0;
    if(function != NULL)
    {
        frame = &fiber->framevals[0];
        frame->closure = NULL;
        frame->function = function;
        frame->slots = fiber->registeritems;
        frame->resultignored = false;
        frame->returntoc = false;
        frame->returnaddress = NULL;
        boa_fiber_ensureregisters(fiber, function->maxregisters);
        frame->ip = function->chunk.compiledcodechunk;
    }
    return fiber;
}

BoaFiber* boa_object_makefiberclosure(BoaState* state, BoaModule* module, BoaFuncClosure* closure)
{
    BoaFiber* fiber;
    fiber = boa_object_makefiber(state, module, closure->function);
    fiber->framevals[0].closure = closure;
    return fiber;
}

void boa_fiber_ensureregisters(BoaFiber* fiber, size_t needed)
{
    size_t i;
    size_t capacity;
    BoaUpvalue* upvalue;
    BoaValue* oldregisters;
    BoaCallFrame* frame;
    BoaValue* oldslots;
    int difference;
    if(fiber->registersallocated >= needed)
    {
        return;
    }
    capacity = (size_t)boa_util_closestpoweroftwo((int)needed);
    oldregisters = fiber->registeritems;
    fiber->registeritems = (BoaValue*)boa_sysmem_realloc(fiber->registeritems, sizeof(BoaValue) * capacity);
    for(i = fiber->registersallocated; i < capacity; i++)
    {
        fiber->registeritems[i] = boa_value_makenull();
    }
    fiber->registersallocated = capacity;
    if(fiber->registeritems != oldregisters)
    {
        for(i = 0; i < fiber->framecount; i++)
        {
            frame = &fiber->framevals[i];
            difference = (frame->slots - oldregisters);
            oldslots = frame->slots;
            frame->slots = fiber->registeritems + difference;
            if(frame->returnaddress != NULL)
            {
                frame->returnaddress = fiber->registeritems + (oldslots - oldregisters);
            }
        }
        for(upvalue = fiber->openupvalues; upvalue != NULL; upvalue = upvalue->next)
        {
            upvalue->location = fiber->registeritems + (upvalue->location - oldregisters);
        }
    }
}

BoaModule* boa_object_makemodule(BoaState* state, BoaString* name)
{
    BoaModule* module;
    module = (BoaModule*)boa_object_allocobject(state, sizeof(BoaModule), BOA_OBJTYPE_MODULE);
    module->name = name;
    module->returnvalue = boa_value_makenull();
    module->mainfunction = NULL;
    module->privatevalues = NULL;
    module->ran = false;
    module->mainfiber = NULL;
    module->privatecount = 0;
    module->privatenames = boa_object_makemap(state, NULL);
    return module;
}

BoaClass* boa_object_makeclass(BoaState* state, BoaString* name)
{
    BoaClass* klass;
    klass = (BoaClass*)boa_object_allocobject(state, sizeof(BoaClass), BOA_OBJTYPE_CLASS);
    klass->name = name;
    klass->mthconstructor = NULL;
    klass->super = NULL;
    boa_table_init(state, &klass->mthtable);
    boa_table_init(state, &klass->staticstable);
    return klass;
}

BoaInstance* boa_object_makeinstance(BoaState* state, BoaClass* klass)
{
    BoaInstance* instance;
    instance = (BoaInstance*)boa_object_allocobject(state, sizeof(BoaInstance), BOA_OBJTYPE_INSTANCE);
    instance->klass = klass;
    boa_table_init(state, &instance->fields);
    return instance;
}

BoaFuncBound* boa_object_makeboundmethod(BoaState* state, BoaValue receiver, BoaValue method)
{
    BoaFuncBound* boundmethod;
    boundmethod = (BoaFuncBound*)boa_object_allocobject(state, sizeof(BoaFuncBound), BOA_OBJTYPE_FUNCBOUNDMETHOD);
    boundmethod->receiver = receiver;
    boundmethod->method = method;
    return boundmethod;
}

BoaArray* boa_array_make(BoaState* state)
{
    BoaArray* array;
    array = (BoaArray*)boa_object_allocobject(state, sizeof(BoaArray), BOA_OBJTYPE_ARRAY);
    boa_dynlistval_init(&array->innerlist);
    return array;
}

void boa_array_destroy(BoaArray* arr)
{
    boa_dynlistval_destroy(&arr->innerlist);
}

void boa_array_push(BoaArray* array, BoaValue val)
{
    boa_dynlistval_push(&array->innerlist, val);
}

size_t boa_array_size(BoaArray* array)
{
    return array->innerlist.listcount;
}

size_t boa_array_count(BoaArray* array)
{
    return array->innerlist.listcount;
}

BoaValue boa_array_get(BoaArray* ary, size_t idx)
{
    return boa_dynlistval_get(&ary->innerlist, idx);
}

BoaValue boa_array_set(BoaArray* ary, size_t idx, BoaValue val)
{
    return boa_dynlistval_set(&ary->innerlist, idx, val);
}

bool boa_array_ensuresize(BoaArray* arr, size_t howmuch)
{
    boa_dynlistval_ensuresize(&arr->innerlist, howmuch);
    return true;
}

BoaValue boa_array_removeat(BoaArray* array, size_t index)
{
    size_t i;
    size_t count;
    BoaValue value;
    BoaDynListVal* vl;
    vl = &array->innerlist;
    count = vl->listcount;
    if(index >= count)
    {
        return boa_value_makenull();
    }
    value = boa_dynlistval_get(vl, index);
    if(index == count - 1)
    {
        boa_dynlistval_set(vl, index, boa_value_makenull());
    }
    else
    {
        for(i = index; i < vl->listcount - 1; i++)
        {
            boa_dynlistval_set(vl, i, boa_dynlistval_get(vl, i + 1));
        }
        boa_dynlistval_set(vl, count - 1, boa_value_makenull());
    }
    vl->listcount--;
    return value;
}

BoaVarargArray* boa_object_makevararray(BoaState* state)
{
    BoaVarargArray* array;
    array = (BoaVarargArray*)boa_object_allocobject(state, sizeof(BoaVarargArray), BOA_OBJTYPE_VARARGARRAY);
    boa_dynlistval_init(&array->innerarray.innerlist);
    return array;
}

BoaMap* boa_object_makemap(BoaState* state, BoaTable* fields)
{
    BoaMap* map;
    map = (BoaMap*)boa_object_allocobject(state, sizeof(BoaMap), BOA_OBJTYPE_MAP);
    boa_table_init(state, &map->innertable);
    if(fields != NULL)
    {
        boa_table_addall(fields, &map->innertable);
    }
    return map;
}

bool boa_map_setvalue(BoaMap* map, BoaString* key, BoaValue value)
{
    if(boa_value_isnull(value))
    {
        boa_map_delete(map, key);
        return false;
    }
    return boa_table_set(&map->innertable, key, value);
}

bool boa_map_getvalue(BoaMap* map, BoaString* key, BoaValue* value)
{
    return boa_table_getentry(&map->innertable, key, value);
}

bool boa_map_delete(BoaMap* map, BoaString* key)
{
    return boa_table_delete(&map->innertable, key);
}

void boa_map_addall(BoaMap* from, BoaMap* to)
{
    int i;
    BoaTabEntry* entry;
    for(i = 0; i <= from->innertable.htcapacity; i++)
    {
        entry = &from->innertable.htentries[i];
        if(entry->entkey != NULL)
        {
            boa_table_set(&to->innertable, entry->entkey, entry->entvalue);
        }
    }
}

bool boa_map_getcstr(BoaMap* map, const char* name, BoaValue* dest)
{
    BoaState* state;
    BoaString sname;
    state = ((BoaObject*)map)->pstate;
    boa_string_maketemp(state, &sname, name);
    return boa_map_getvalue(map, &sname, dest);
}

void boa_map_setcstr(BoaMap* map, const char* name, BoaValue value)
{
    BoaState* state;
    BoaString* sname;
    state = ((BoaObject*)map)->pstate;
    sname = boa_string_copy(state, name);
    boa_map_setvalue(map, sname, value);
}

BoaUserdata* boa_userdata_makeuserdata(BoaState* state, size_t size)
{
    BoaUserdata* userdata;
    userdata = (BoaUserdata*)boa_object_allocobject(state, sizeof(BoaUserdata), BOA_OBJTYPE_USERDATA);
    if(size > 0)
    {
        userdata->data = boa_sysmem_malloc(size);
    }
    else
    {
        userdata->data = NULL;
    }
    userdata->size = size;
    userdata->oncleanupfn = NULL;
    return userdata;
}

void* boa_userdata_insertdata(BoaState* state, BoaValue instance, size_t typesz, BoaCleanupFn cleanup)
{
    BoaUserdata* userdata;
    BoaInstance* iptr;
    userdata = boa_userdata_makeuserdata(state, typesz);
    userdata->oncleanupfn = cleanup;
    iptr = boa_value_asinstance(instance);
    boa_table_set(&iptr->fields, state->strings.struserdatafield, boa_value_fromobject(userdata));
    return userdata->data;
}

void* boa_userdata_extractdata(BoaValue instance)
{
    BoaValue temp;
    BoaState* state;
    BoaInstance* inst;
    inst = boa_value_asinstance(instance);
    state = ((BoaObject*)inst)->pstate;
    if(!boa_table_getentry(&inst->fields, state->strings.struserdatafield, &temp))
    {
        boa_vm_raisefatalerror(state, "failed to extract userdata");
    }
    return boa_value_asuserdata(temp)->data;
}

BoaRange* boa_object_makerange(BoaState* state, BoaNumber from, BoaNumber to)
{
    BoaRange* range;
    range = (BoaRange*)boa_object_allocobject(state, sizeof(BoaRange), BOA_OBJTYPE_RANGE);
    range->from = from;
    range->to = to;
    return range;
}

BoaField* boa_object_makefield(BoaState* state, BoaObject* getter, BoaObject* setter)
{
    BoaField* field;
    field = (BoaField*)boa_object_allocobject(state, sizeof(BoaField), BOA_OBJTYPE_FIELD);
    field->getter = getter;
    field->setter = setter;
    return field;
}

BoaReference* boa_object_makereference(BoaState* state, BoaValue* slot)
{
    BoaReference* reference;
    reference = (BoaReference*)boa_object_allocobject(state, sizeof(BoaReference), BOA_OBJTYPE_REFERENCE);
    reference->slot = slot;
    return reference;
}

BoaException* boa_object_makeexception(BoaState* state, BoaClass* baseclass)
{
    BoaException* ex;
    ex = (BoaException*)boa_object_allocobject(state, sizeof(BoaException), BOA_OBJTYPE_EXCEPTION);
    ex->baseclass = baseclass;
    ex->message = boa_value_makenull();
    return ex;
}

void boa_state_defaultprinterrmsgerror(BoaState* state, const char* message, bool iswarning)
{
    BoaStream* pr;
    (void)state;
    pr = state->streamstderr;
    fflush(stdout);
    if(message != NULL)
    {
        boa_stream_setcolor(pr, 'r');
        if(iswarning)
        {
            boa_stream_printf(pr, "warning: ");
            boa_stream_printf(pr, "  %s\n", message);            
        }
        else
        {
            boa_stream_printf(pr, "unhandled error in state:\n");
            boa_stream_printf(pr, "  %s\n", message);
        }
        boa_stream_resetcolor(pr);
        fflush(stderr);
    }
    if(!iswarning)
    {
        state->haderror = true;
    }
}

void boa_state_setdefaultconfig(BoaState* state)
{
    state->config.mempooldisable = false;
    state->config.mempoolforcegeneric = false;
    state->config.dumpast = false;
    state->config.traceexecution = false;
    state->config.traceinstsonly = false;
    state->config.isreplmode = false;
    state->config.havedesttrace = false;
    state->config.quitafterdump = false;
}

void boa_state_make(BoaState* state, BoaConfig* cfg)
{
    state->stdclassclass = NULL;
    state->stdobjectclass = NULL;
    state->stdnullclass = NULL;
    state->stdclassnumber = NULL;
    state->stdclassstring = NULL;
    state->stdclassbool = NULL;
    state->stdclassfunction = NULL;
    state->stdclassfiber = NULL;
    state->stdclassmodule = NULL;
    state->stdclassarray = NULL;
    state->stdclassmap = NULL;
    state->stdclassrange = NULL;
    state->bytesallocated = 0;
    state->gcnextgc = 256 * 1024;
    state->gcallowgc = false;
    state->printerrmessagefn = boa_state_defaultprinterrmsgerror;
    state->haderror = false;
    state->rootvalues = NULL;
    state->rootcount = 0;
    state->rootcapacity = 0;
    state->lastmodule = NULL;
    boa_state_setdefaultconfig(state);
    if(cfg != NULL)
    {
        state->config = *cfg;
    }
    boa_sysmem_poolinit(&state->config);
    state->streamstdout = boa_stream_makeio(stdout, false);
    state->streamstdout->shouldflush = true;
    state->streamstderr = boa_stream_makeio(stderr, false);
    state->config.desttrace = state->streamstderr;
    boa_init_vm(state);
    boa_api_init(state);
    {
        state->strings.strempty = boa_string_copylen(state, "", 0);
        state->strings.strnull = boa_string_copy(state, "null");
        state->strings.strtrue = boa_string_copy(state, "true");
        state->strings.strfalse = boa_string_copy(state, "false");
        state->strings.strthis = boa_string_copy(state, "this");
        state->strings.strtostring = boa_string_copy(state, "toString");
        state->strings.strconstructor = boa_string_copy(state, "constructor");
        state->strings.strsuper = boa_string_copy(state, "super");
        state->strings.strjoin = boa_string_copy(state, "join");
        state->strings.striterator = boa_string_copy(state, "iterator");
        state->strings.stritervalue = boa_string_copy(state, "iteratorValue");
        state->strings.struserdatafield = boa_string_copy(state, "_data");
        state->strings.stropequal = boa_string_copy(state, "==");
        state->strings.stropnot = boa_string_copy(state, "!");
        state->strings.stropindex = boa_string_copy(state, "[]");
        state->strings.stropplus = boa_string_copy(state, "+");
        state->strings.stropminus = boa_string_copy(state, "-");
        state->strings.stropdivide = boa_string_copy(state, "/");
        state->strings.stropmultiply = boa_string_copy(state, "*");
        state->strings.stroplessthan = boa_string_copy(state, "<");
        state->strings.stroplessequal = boa_string_copy(state, "<=");
        state->strings.stropgreaterthan = boa_string_copy(state, ">");
        state->strings.stropgreaterequal = boa_string_copy(state, ">=");
        state->strings.stroppower = boa_string_copy(state, "**");
        state->strings.stropfloordiv = boa_string_copy(state, "#");
        state->strings.stropmodulo = boa_string_copy(state, "%");
        state->strings.strdots = boa_string_copy(state, "...");
        state->strings.strcallbackforeach = boa_string_copy(state, "<callback for foreach>");
        state->strings.strcallbacksort = boa_string_copy(state, "<callback for sort>");
    }
    state->activelexer = (BoaAstLexer*)boa_sysmem_malloc(sizeof(BoaAstLexer));
    state->activeparser = (BoaAstParser*)boa_sysmem_malloc(sizeof(BoaAstParser));
    boa_astparser_init(state, (BoaAstParser*)state->activeparser);
    state->activeemitter = (BoaAstEmitter*)boa_sysmem_malloc(sizeof(BoaAstEmitter));
    boa_emitter_init(state, state->activeemitter);
    boa_state_openstdclasses(state);
    boa_state_openstdlibs(state);
    boa_state_openglobalfuncs(state);
}

int64_t boa_state_destroy(BoaState* state)
{
    int64_t amount;
    if(state->rootvalues != NULL)
    {
        boa_sysmem_free(state->rootvalues);
        state->rootvalues = NULL;
    }
    boa_api_destroy(state);
    boa_stream_destroy(state->streamstdout);
    boa_stream_destroy(state->streamstderr);
    if(state->config.havedesttrace)
    {
        boa_stream_destroy(state->config.desttrace);
    }
    boa_sysmem_free(state->activelexer);
    boa_astparser_destroy(state->activeparser);
    boa_sysmem_free(state->activeparser);
    boa_emitter_destroy(state->activeemitter);
    boa_sysmem_free(state->activeemitter);
    boa_free_vm(state);
    amount = state->bytesallocated;
    return amount;
}

void boa_state_pushroot(BoaState* state, BoaObject* object)
{
    boa_state_pushvalueroot(state, boa_value_fromobject(object));
}

void boa_state_pushvalueroot(BoaState* state, BoaValue value)
{
    if(state->rootcount + 1 >= state->rootcapacity)
    {
        state->rootcapacity = boa_util_grownextcapacity(state->rootcapacity);
        state->rootvalues = (BoaValue*)boa_sysmem_realloc(state->rootvalues, state->rootcapacity * sizeof(BoaValue));
    }
    state->rootvalues[state->rootcount++] = value;
}

BoaValue boa_state_peekroot(BoaState* state, size_t distance)
{
    return state->rootvalues[state->rootcount - distance - 1];
}

void boa_state_poproot(BoaState* state)
{
    state->rootcount--;
}

void boa_state_poproots(BoaState* state, size_t amount)
{
    state->rootcount -= amount;
}

BoaClass* boa_state_getclassfor(BoaState* state, BoaValue value)
{
    BoaUpvalue* upvalue;
    BoaValue* slot;
    if(boa_value_isnull(value))
    {
        return state->stdnullclass;
    }
    if(boa_value_isobject(value))
    {
        switch(boa_value_objtype(value))
        {
            case BOA_OBJTYPE_STRING:
                {
                    return state->stdclassstring;
                }
                break;
            case BOA_OBJTYPE_USERDATA:
                {
                    return state->stdobjectclass;
                }
                break;
            case BOA_OBJTYPE_FIELD:
            case BOA_OBJTYPE_FUNCSCRIPT:
            case BOA_OBJTYPE_FUNCCLOSURE:
            case BOA_OBJTYPE_CLSPROTOTYPE:
            case BOA_OBJTYPE_FUNCNATIVE:
            case BOA_OBJTYPE_FUNCBOUNDMETHOD:
            case BOA_OBJTYPE_FUNCNATMETHOD:
                {
                    return state->stdclassfunction;
                }
                break;
            case BOA_OBJTYPE_FIBER:
                {
                    return state->stdclassfiber;
                }
                break;
            case BOA_OBJTYPE_MODULE:
                {
                    return state->stdclassmodule;
                }
                break;
            case BOA_OBJTYPE_UPVALUE:
                {
                    upvalue = boa_value_asupvalue(value);
                    if(upvalue->location == NULL)
                    {
                        return boa_state_getclassfor(state, upvalue->closed);
                    }
                    return boa_state_getclassfor(state, *upvalue->location);
                }
                break;
            case BOA_OBJTYPE_INSTANCE:
                {
                    return boa_value_asinstance(value)->klass;
                }
                break;
            case BOA_OBJTYPE_CLASS:
                {
                    return state->stdclassclass;
                }
                break;
            case BOA_OBJTYPE_ARRAY:
            case BOA_OBJTYPE_VARARGARRAY:
                {
                    return state->stdclassarray;
                }
                break;
            case BOA_OBJTYPE_MAP:
                {
                    return state->stdclassmap;
                }
                break;
            case BOA_OBJTYPE_RANGE:
                {
                    return state->stdclassrange;
                }
                break;
            case BOA_OBJTYPE_REFERENCE:
                {
                    slot = boa_value_asreference(value)->slot;
                    if(slot != NULL)
                    {
                        return boa_state_getclassfor(state, *slot);
                    }
                    return state->stdobjectclass;
                }
                break;
            case BOA_OBJTYPE_EXCEPTION:
                {
                    return boa_value_asexception(value)->baseclass;
                }
                break;
        }
    }
    else if(boa_value_isnumber(value))
    {
        return state->stdclassnumber;
    }
    else if(boa_value_isbool(value))
    {
        return state->stdclassbool;
    }
    return NULL;
}

BoaResult boa_state_interpretsource(BoaState* state, const char* modname, const char* code)
{
    return boa_state_interninterpretsource(state, boa_string_copylen(state, modname, strlen(modname)), code);
}

BoaModule* boa_state_compilemodulesource(BoaState* state, BoaString* modname, const char* code)
{
    bool allowedgc;
    BoaModule* module;
    BoaDynListExpr statements;
    allowedgc = state->gcallowgc;
    state->gcallowgc = false;
    state->haderror = false;
    module = NULL;
    /* this is a lbc format */
    if((code[1] << 8 | code[0]) == BOA_CONFIG_BCMAGICNUMBER)
    {
        module = boa_bcemu_initloadmodule(state, code);
    }
    else
    {
        boa_dynlistexpr_init(&statements);
        if(boa_astparser_parsesource(state->activeparser, boa_string_getdata(modname), code, &statements))
        {
            boa_ast_destroyexprlist(state, &statements);
            return NULL;
        }
        if(state->config.dumpast)
        {
            boa_astprintdefault_printbeginlist(state, stdout, &statements);
            if(state->config.quitafterdump)
            {
                boa_ast_destroyexprlist(state, &statements);
                return NULL;
            }
        }
        module = boa_emitter_emitmod(state->activeemitter, &statements, modname);
        boa_ast_destroyexprlist(state, &statements);
    }
    state->gcallowgc = allowedgc;
    return state->haderror ? NULL : module;
}

BoaResult boa_state_interninterpretsource(BoaState* state, BoaString* modname, const char* code)
{
    BoaModule* module;
    BoaResult result;
    module = boa_state_compilemodulesource(state, modname, code);
    if(module == NULL)
    {
        return boa_result_make(BOA_STATUS_COMPILEERROR, boa_value_makenull());
    }
    result = boa_interpret_module(state, module);
    state->lastmodule = module;
    return result;
}

bool boa_state_compileandsavefile(BoaState* state, const char* inputfile, const char* outputfile)
{
    size_t flen;
    BoaModule* compiledmodule;
    char* filename;
    char* source;
    BoaString* modname;
    BoaModule* module;
    FILE* hnd;
    (void)flen;
    {
        filename = boa_util_dupstring(inputfile);
        source = boa_util_readfile(filename, &flen);
        if(source == NULL)
        {
            boa_state_raiseerror(state, "failed to open file '%s'", filename);
            return false;
        }
        modname = boa_string_copylen(state, filename, strlen(filename));
        module = boa_state_compilemodulesource(state, modname, source);
        compiledmodule = module;
        boa_sysmem_free((void*)source);
        if(module == NULL)
        {
            return false;
        }
    }
    hnd = fopen(outputfile, "w+b");
    if(hnd == NULL)
    {
        boa_state_raiseerror(state, "failed to open for writing file '%s'", outputfile);
        return false;
    }
    boa_bcfile_writeuint32(hnd, BOA_CONFIG_BCMAGICNUMBER);
    boa_bcfile_writeuint32(hnd, BOA_CONFIG_BCVERSION);
    boa_bcfile_writeuint32(hnd, 1);
    {
        boa_bcfile_writemodule(compiledmodule, hnd);
    }
    boa_bcfile_writeuint32(hnd, BOA_CONFIG_BCENDNUMBER);
    fclose(hnd);
    return true;
}

char* boa_util_readsource(BoaState* state, const char* filename)
{
    size_t flen;
    char* source;
    (void)flen;
    source = boa_util_readfile(filename, &flen);
    if(source == NULL)
    {
        boa_state_raiseerror(state, "failed to open file '%s'", filename);
    }
    return source;
}

BoaResult boa_state_interpretfile(BoaState* state, const char* filepath)
{
    char* source;
    const char* modname;
    BoaResult result;
    modname = boa_util_fsgetbasename(filepath);
    source = boa_util_readsource(state, filepath);
    if(source == NULL)
    {
        return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
    }
    result = boa_state_interpretsource(state, modname, source);
    boa_sysmem_free((void*)source);
    return result;
}

BoaResult boa_state_dumpfile(BoaState* state, BoaStream* pr, const char* binfilepath)
{
    char* source;
    BoaResult result;
    BoaString* modname;
    BoaModule* module;
    source = boa_util_readsource(state, binfilepath);
    if(source == NULL)
    {
        return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
    }
    modname = boa_string_copylen(state, binfilepath, strlen(binfilepath));
    module = boa_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        result = boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
    }
    else
    {
        boa_debug_disasmodule(pr, module, source);
        result = boa_result_make(BOA_STATUS_OK, boa_value_makenull());
    }
    boa_sysmem_free((void*)source);
    return result;
}

void boa_state_raiseerror(BoaState* state, const char* fmt, ...)
{
    va_list args;
    BoaStream pr;
    boa_stream_makestackstring(&pr);
    va_start(args, fmt);
    boa_stream_printfv(&pr, fmt, args);
    va_end(args);
    state->printerrmessagefn(state, pr.desthndstring.data, false);
    boa_stream_destroy(&pr);
}

void boa_state_raisewarning(BoaState* state, const char* fmt, ...)
{
    va_list args;
    BoaStream pr;
    boa_stream_makestackstring(&pr);
    va_start(args, fmt);
    boa_stream_printfv(&pr, fmt, args);
    va_end(args);
    state->printerrmessagefn(state, pr.desthndstring.data, true);
    boa_stream_destroy(&pr);
}

void boa_debug_traceprintvalue(BoaStream* pr, const char* prefix, size_t framecount, size_t argc, BoaValue* vals)
{
    size_t i;
    boa_stream_printf(pr, "-> f%ld %s{\n", framecount, prefix);
    for(i = 0; i <= argc; i++)
    {
        boa_stream_printf(pr, "  [%ld]: ", i);
        boa_value_printvalue(pr, *(vals + i), true);
        boa_stream_printf(pr, "\n");
    }
    boa_stream_printf(pr, "}\n");
}

void boa_vmexec_resetvm(BoaState* state)
{
    state->vmstate.objects = NULL;
    state->vmstate.fiber = NULL;
    state->vmstate.gcgraystack = NULL;
    state->vmstate.gcgraycount = 0;
    state->vmstate.gcgraycapacity = 0;
    boa_strtable_init(&state->vmstate.storedstrings);
    state->vmstate.globals = NULL;
    state->vmstate.modules = NULL;
}

void boa_init_vm(BoaState* state)
{
    boa_vmexec_resetvm(state);
    state->vmstate.globals = boa_object_makemap(state, NULL);
    state->vmstate.modules = boa_object_makemap(state, NULL);
}

void boa_free_vm(BoaState* state)
{
    boa_strtable_free(&state->vmstate.storedstrings);
    boa_gcmem_freeobjlist(state, state->vmstate.objects);
    boa_vmexec_resetvm(state);
}

bool boa_vm_handleerror(BoaState* state, BoaValue errorvalue)
{
    int i;
    int count;
    BoaStream* pr;
    BoaValue errstrval;
    BoaFiber* fiber;
    BoaHandler* handler;
    BoaFiber* caller;
    BoaCallFrame* frame;
    BoaFuncScript* function;
    BoaChunk* chunk;
    const char* name;
    size_t line;
    BoaException* exception;
    pr = state->streamstderr;
    errstrval = errorvalue;
    fiber = state->vmstate.fiber;
    while(fiber != NULL)
    {
        fiber->error = errstrval;
        if(fiber->handlercount > 0)
        {
            handler = &fiber->handleritems[--fiber->handlercount];
            state->vmstate.fiber = fiber;
            fiber->framecount = handler->framecount;
            state->vmstate.frame = &fiber->framevals[fiber->framecount - 1];
            state->vmstate.currentchunk = &state->vmstate.frame->function->chunk;
            state->vmstate.ip = handler->handlerip;
            state->vmstate.frame->ip = handler->handlerip;
            state->vmstate.vmregisteritems = fiber->registeritems + handler->registercount;
            state->vmstate.vmregisteritems[handler->errorreg] = errorvalue;
            /* exception has been caught, time to move on */
            return true;
        }
        if(fiber->catcher)
        {
            if(fiber->parent == NULL)
            {
                break;
            }
            fiber->caught = true;
            state->vmstate.fiber = fiber->parent;
            if(state->vmstate.fiber->returnaddress != NULL)
            {
                *state->vmstate.fiber->returnaddress = errstrval;
            }
            /* exception caught via Fiber, time to move on */
            return true;
        }
        caller = fiber->parent;
        fiber->parent = NULL;
        fiber = caller;
    }
    /* at this point, the exception has not been caught, so print info about it */
    fiber = state->vmstate.fiber;
    fiber->muststop = true;
    fiber->error = errstrval;
    if(fiber->parent != NULL)
    {
        fiber->parent->muststop = true;
    }
    count = (int)fiber->framecount - 1;
    boa_stream_setcolor(pr, 'r');
    if(boa_value_isexception(errstrval))
    {
        exception = boa_value_asexception(errstrval);
        name = (exception->baseclass != NULL && exception->baseclass->name != NULL) ? boa_string_getdata(exception->baseclass->name) : "unknown";
        boa_stream_printf(pr, "uncaught %s in vm:\n", name);
        boa_value_printvalue(pr, exception->message, true);
    }
    else
    {
        boa_stream_printf(pr, "unhandled error in vm:\n");
        boa_value_printvalue(pr, errorvalue, true);
    }
    boa_stream_printf(pr, "\n");
    for(i = count; i >= 0; i--)
    {
        frame = &fiber->framevals[i];
        function = frame->function;
        chunk = &function->chunk;
        name = NULL;
        if(function->name != NULL)
        {
            name = boa_string_getdata(function->name);
        }
        if(chunk->haslineinfo)
        {
            line = boa_chunk_getline(chunk, frame->ip - chunk->compiledcodechunk - 1);
            boa_stream_printf(pr, "  [line %ld] in %s()\n", line, name);
        }
        else
        {
            boa_stream_printf(pr, "\tin %s()\n", name);
        }
    }
    boa_stream_resetcolor(pr);
    state->printerrmessagefn(state, NULL, false);
    return false;
}

bool boa_vm_raiseerrorva(BoaState* state, BoaException* exclass, const char* format, va_list args)
{
    BoaStream pr;
    BoaString* str;
    BoaException* exception;
    boa_stream_makestackstring(&pr);
    boa_stream_printfv(&pr, format, args);
    str = boa_stream_takestring(state, &pr);
    if(exclass == NULL)
    {
        exclass = state->exceptions.stdexception;
    }
    boa_state_pushroot(state, (BoaObject*)str);
    exception = boa_object_makeexception(state, exclass->baseclass);
    boa_state_poproot(state);
    exception->message = boa_value_fromobject(str);
    return boa_vm_handleerror(state, boa_value_fromobject(exception));
}

bool boa_vm_raiseerror(BoaState* state, const char* format, ...)
{
    va_list args;
    bool result;
    va_start(args, format);
    result = boa_vm_raiseerrorva(state, state->exceptions.stdexception, format, args);
    va_end(args);
    return result;
}

BoaValue boa_vm_raiseexception(BoaState* state, BoaException* exclass, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    boa_vm_raiseerrorva(state, exclass, format, args);
    va_end(args);
    return boa_value_makenull();
}

bool boa_vm_raisefatalerror(BoaState* state, const char* format, ...)
{
    va_list args;
    bool result;
    (void)result;
    va_start(args, format);
    result = boa_vm_raiseerrorva(state, state->exceptions.stdexception, format, args);
    va_end(args);
    boa_jmpstate_nativeexitjump();
    return result;
}

bool boa_vmexec_callcallable(BoaState* state, BoaFuncScript* function, BoaFuncClosure* closure, size_t argc, size_t calleeregister)
{
    size_t i;
    size_t newcapacity;
    size_t targetargcount;
    size_t j;
    bool vararg;
    BoaFiber* fiber;
    BoaCallFrame* frame;
    BoaCallFrame* previousframe;
    BoaArray* array;
    fiber = state->vmstate.fiber;
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        newcapacity = ((fiber->framecapacity + 1) * 2);
        fiber->framevals = (BoaCallFrame*)boa_sysmem_realloc(fiber->framevals, sizeof(BoaCallFrame) * newcapacity);
        fiber->framecapacity = newcapacity;
    }
    frame = &fiber->framevals[fiber->framecount++];
    previousframe = &fiber->framevals[fiber->framecount - 2];
    frame->function = function;
    frame->closure = closure;
    frame->ip = function->chunk.compiledcodechunk;
    frame->slots = previousframe->slots + calleeregister;
    frame->resultignored = false;
    frame->returntoc = false;
    frame->returnaddress = previousframe->slots + (int)calleeregister;
    boa_fiber_ensureregisters(fiber, frame->slots - fiber->registeritems + function->maxregisters);
    targetargcount = function->argcount;
    vararg = function->vararg;
    if(targetargcount > argc)
    {
        for(i = argc; i < targetargcount; i++)
        {
            *(frame->slots + i + 1) = boa_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = boa_value_fromobject(boa_array_make(state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && boa_value_isvargarray(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            array = &boa_object_makevararray(state)->innerarray;
            boa_state_pushroot(state, (BoaObject*)array);
            boa_dynlistval_ensuresize(&array->innerlist, argc - targetargcount + 1);
            j = 0;
            for(i = targetargcount - 1; i < argc; i++)
            {
                array->innerlist.listitems[j++] = *(frame->slots + i + 1);
            }
            *(frame->slots + targetargcount) = boa_value_fromobject(array);
            boa_state_poproot(state);
        }
    }
    return true;
}

bool boa_vmexec_actualcallvalue(BoaState* state, size_t calleeregister, size_t argc, BoaValue alternatecallee, const char* fname)
{
    bool caught;
    BoaValue callee;
    BoaCallFrame* frame;
    BoaFuncClosure* closure;
    BoaValue value;
    BoaFiber* fiber;
    BoaFuncNative* method;
    BoaClass* klass;
    BoaInstance* instance;
    BoaValue mth;
    BoaFuncBound* boundmethod;
    uint32_t entryframecount;
    uint64_t* savedip;
    fiber = state->vmstate.fiber;
    frame = &fiber->framevals[fiber->framecount - 1];
    if(boa_value_isnull(alternatecallee))
    {
        callee = frame->slots[calleeregister];
    }
    else
    {
        callee = alternatecallee;
    }
    if(boa_value_isobject(callee))
    {
        if(boa_jmpstate_setnativeexit())
        {
            caught = state->vmstate.fiber->caught;
            state->vmstate.fiber->caught = false;
            return !caught;
        }
        switch(boa_value_objtype(callee))
        {
            case BOA_OBJTYPE_FUNCSCRIPT:
                {
                    return boa_vmexec_callcallable(state, boa_value_asfuncscript(callee), NULL, argc, calleeregister);
                }
                break;
            case BOA_OBJTYPE_FUNCCLOSURE:
                {
                    closure = boa_value_asfuncclosure(callee);
                    return boa_vmexec_callcallable(state, closure->function, closure, argc, calleeregister);
                }
                break;
            case BOA_OBJTYPE_FUNCNATIVE:
                {
                    entryframecount = fiber->framecount;
                    savedip = frame->ip;
                    value = boa_value_asfuncnative(callee)->natfuncptr(state, boa_value_makenull(), argc, frame->slots + calleeregister + 1);
                    /* if the native raised a handled error, the vm already
                     * unwound to the catch handler (rewriting frame->ip);
                     * writing the result would clobber handler registers */
                    if(state->vmstate.fiber == fiber && fiber->framecount == entryframecount && frame->ip == savedip)
                    {
                        frame->slots[calleeregister] = value;
                    }
                    return !state->vmstate.fiber->muststop;
                }
                break;
            case BOA_OBJTYPE_FUNCNATMETHOD:
                {
                    boa_vmexec_pushgc(state, false);
                    method = boa_value_asfuncmethod(callee);
                    fiber = state->vmstate.fiber;
                    entryframecount = fiber->framecount;
                    savedip = frame->ip;
                    value = method->natfuncptr(state, *(frame->slots + calleeregister), argc, frame->slots + calleeregister + 1);
                    if(state->vmstate.fiber == fiber && fiber->framecount == entryframecount && frame->ip == savedip)
                    {
                        frame->slots[calleeregister] = value;
                    }
                    boa_vmexec_popgc(state);
                    return !fiber->muststop;
                }
                break;
            case BOA_OBJTYPE_CLASS:
                {
                    klass = boa_value_asclass(callee);
                    instance = boa_object_makeinstance(state, klass);
                    frame->slots[calleeregister] = boa_value_fromobject(instance);
                    if(klass->mthconstructor != NULL)
                    {
                        return boa_vmexec_actualcallvalue(state, calleeregister, argc, boa_value_fromobject(klass->mthconstructor), boa_string_getdata(state->strings.strconstructor));
                    }
                    return true;
                }
                break;
            case BOA_OBJTYPE_FUNCBOUNDMETHOD:
                {
                    boundmethod = boa_value_asfuncboundmethod(callee);
                    mth = boundmethod->method;
                    if(boa_value_isfuncmethod(mth))
                    {
                        boa_vmexec_pushgc(state, false);
                        entryframecount = fiber->framecount;
                        savedip = frame->ip;
                        value = boa_value_asfuncmethod(mth)->natfuncptr(state, boundmethod->receiver, argc, frame->slots + calleeregister + 1);
                        if(state->vmstate.fiber == fiber && fiber->framecount == entryframecount && frame->ip == savedip)
                        {
                            frame->slots[calleeregister] = value;
                        }
                        boa_vmexec_popgc(state);
                        return !state->vmstate.fiber->muststop;
                    }
                    else
                    {
                        frame->slots[calleeregister] = boundmethod->receiver;
                        return boa_vmexec_callcallable(state, boa_value_asfuncscript(mth), NULL, argc, calleeregister);
                    }
                    return !state->vmstate.fiber->muststop;
                }
                break;
            default:
                {
                }
                break;
        }
    }
    if(boa_value_isnull(callee))
    {
        #if 0
        if(fname == NULL)
        {
            abort();
        }
        #endif
        return boa_vm_raiseerror(state, "cannot call a null value (name '%s')", fname);
    }
    else
    {
        return boa_vm_raiseerror(state, "can only call functions and classes, got %s", boa_value_valtypename(callee));
    }
    return true;
}

BoaUpvalue* boa_vmexec_captureupvalue(BoaState* state, BoaValue* local)
{
    BoaUpvalue* upvalue;
    BoaUpvalue* previousupvalue;
    BoaUpvalue* createdupvalue;
    previousupvalue = NULL;
    upvalue = state->vmstate.fiber->openupvalues;
    while(upvalue != NULL && upvalue->location > local)
    {
        previousupvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    createdupvalue = boa_object_makeupvalue(state, local);
    createdupvalue->next = upvalue;
    if(previousupvalue == NULL)
    {
        state->vmstate.fiber->openupvalues = createdupvalue;
    }
    else
    {
        previousupvalue->next = createdupvalue;
    }
    return createdupvalue;
}

void boa_vmexec_closeupvalues(BoaState* state, BoaValue* last)
{
    BoaFiber* fiber;
    BoaUpvalue* upvalue;
    fiber = state->vmstate.fiber;
    while(fiber->openupvalues != NULL && fiber->openupvalues->location >= last)
    {
        upvalue = fiber->openupvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        fiber->openupvalues = upvalue->next;
    }
}

BoaResult boa_interpret_module(BoaState* state, BoaModule* module)
{
    BoaResult result;
    BoaFiber* fiber;
    fiber = boa_object_makefiber(state, module, module->mainfunction);
    state->vmstate.fiber = fiber;
    result = boa_state_execfiber(state, fiber);
    return result;
}

/*
* only a very few compilers support 'computed goto's.
* that is, using 'goto' on a computed address, rather than a label.
* there's much better info on the net about this, but in practice, it works like this (simplified!):
*
*
*   void* addr = &&mylabel;
*   if(somethingchanged())
*   {
*       addr = &&myotherlabel;
*   }
*   goto* addr;
*   mylabel:
*       dosomething();
*       return;
*   myotherlabel:
*       dosomethingelse();
*       return;
* ----
* __CPPCHECK__ is not an official ident; but it is for cppcheck.
* i'm not sure why cppcheck doesn't have an identifier like this already, though.
*/
#if (!defined(__CPPCHECK__) && (!defined(__cplusplus__) && !defined(__STRICT_ANSI__))) && (defined(__GNUC__) || defined(__clang__) || defined(__TINYCC__))
    #undef BOA_CONF_USECOMPUTEDGOTO
    #define BOA_CONF_USECOMPUTEDGOTO 1
#endif

#define boa_vmmac_dispatchnext() goto dispatch;

#if defined(BOA_CONF_USECOMPUTEDGOTO) && (BOA_CONF_USECOMPUTEDGOTO == 1)
    #define LABELNAME(nm) label_##nm
    #define CASE_CODE(name) label_##name:
#else
    #define CASE_CODE(name) case name:
#endif

BOA_FORCEINLINE void boa_vmmac_readframe(BoaState* state, BoaFiber** destfiber)
{
    *destfiber = state->vmstate.fiber;
    state->vmstate.frame = &(*destfiber)->framevals[(*destfiber)->framecount - 1];
    state->vmstate.currentchunk = &state->vmstate.frame->function->chunk;
    state->vmstate.vmconstantvalues = state->vmstate.currentchunk->constantlist.listitems;
    state->vmstate.ip = state->vmstate.frame->ip;
    (*destfiber)->module = state->vmstate.frame->function->module;
    state->vmstate.vmregisteritems = state->vmstate.frame->slots;
    state->vmstate.vmprivatevalues = (*destfiber)->module->privatevalues;
    state->vmstate.vmupvalueitems = state->vmstate.frame->closure == NULL ? NULL : state->vmstate.frame->closure->closureupvalueitems;
}

BOA_FORCEINLINE void boa_vmmac_writeframe(BoaState* state)
{
    state->vmstate.frame->ip = state->vmstate.ip;
}

BOA_FORCEINLINE bool boa_vmmac_recoverstate(BoaState* state, BoaFiber** fiber, BoaResult* result)
{
    boa_vmmac_writeframe(state);
    (*fiber) = state->vmstate.fiber;
    if((*fiber) == NULL)
    {
        *result = boa_result_make(BOA_STATUS_OK, boa_value_makenull());
        return false;
    }
    if((*fiber)->muststop)
    {
        boa_vmexec_popgc(state);
        *result = boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        return false;
    }
    boa_vmmac_readframe(state, fiber);
    return true;
}

BOA_FORCEINLINE BoaValue boa_vmmac_getrc(BoaState* state, int64_t r)
{
    if(BOA_BIT_ISSET(r, BOA_BITFLAG_CONSTANT_BX))
    {
        return state->vmstate.vmconstantvalues[r & ~(1UL << BOA_BITFLAG_CONSTANT_BX)];
    }
    if(BOA_BIT_ISSET(r, BOA_BITFLAG_CONSTANT))
    {
        return state->vmstate.vmconstantvalues[r & ~(1UL << BOA_BITFLAG_CONSTANT)];
    }
    return state->vmstate.vmregisteritems[r];
}

BOA_FORCEINLINE bool boa_vmmac_callvalue(BoaState* state, BoaFiber** fiber, BoaValue callee, size_t reg, size_t argc, BoaResult* res, const char* fname)
{
    if(!boa_vmexec_actualcallvalue(state, reg, argc, callee, fname))
    {
        if(!boa_vmmac_recoverstate(state, fiber, res))
        {
            return false;
        }
    }
    return true;
}

#define boa_vmmac_fail(state, ...) \
    { \
        BoaResult tmprecoverres; \
        if(boa_vm_raiseerror(state, __VA_ARGS__)) \
        { \
            if(!boa_vmmac_recoverstate(state, &fiber, &tmprecoverres)) \
            { \
                return tmprecoverres; \
            } \
            boa_vmmac_dispatchnext(); \
        } \
        else \
        { \
            boa_vmexec_popgc(state); \
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull()); \
        } \
    }

BOA_FORCEINLINE BoaResult boa_vmmac_invokeoperatormethoddefault(BoaState* state, BoaFiber** fiber, size_t reg, BoaValue bv, BoaString* mthname, size_t argc)
{
    BoaValue method;
    BoaResult tmpres;
    BoaClass* klass;
    boa_vmmac_writeframe(state);
    klass = boa_state_getclassfor(state, bv);
    if(klass == NULL)
    {
        if(boa_vm_raiseerror(state, "use of method '%s' on a null value", boa_string_getdata(mthname)))
        {
            if(!boa_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
        }
        else
        {
            boa_vmexec_popgc(state);
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        }
    }
    if((boa_value_isinstance(bv) && (boa_table_getentry(&boa_value_asinstance(bv)->fields, mthname, &method))) || boa_table_getentry(&klass->mthtable, mthname, &method))
    {
        if(!boa_vmmac_callvalue(state, fiber, method, reg, argc, &tmpres, boa_string_getdata(mthname)))
        {
            return tmpres;
        }
        boa_vmmac_readframe(state, fiber);
        return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
    }
    else
    {
        if(boa_vm_raiseerror(state, "attempt to invoke undefined operator method '%s#operator %s'", boa_string_getdata(klass->name), boa_string_getdata(mthname)))
        {
            if(!boa_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
        }
        else
        {
            boa_vmexec_popgc(state);
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        }
    }
    boa_vmmac_readframe(state, fiber);
    return boa_result_make(BOA_STATUS_OK, boa_value_makenull());
}

BOA_FORCEINLINE BoaResult boa_vmmac_invokeoperatormethodandcontinue(BoaState* state, BoaFiber** fiber, size_t reg, BoaValue bv, BoaString* mthname, size_t argc)
{
    BoaResult tmpres;
    BoaResult invmcres;
    BoaClass* klass;
    BoaValue method;
    boa_vmmac_writeframe(state);
    klass = boa_state_getclassfor(state, bv);
    if(klass == NULL)
    {
        if(boa_vm_raiseerror(state, "only instances and classes have methods"))
        {
            if(!boa_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
        }
        else
        {
            boa_vmexec_popgc(state);
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());
        }
    }
    if((boa_value_isinstance(bv) && (boa_table_getentry(&boa_value_asinstance(bv)->fields, mthname, &method))) || boa_table_getentry(&klass->mthtable, mthname, &method))
    {
        if(!boa_vmmac_callvalue(state, fiber, method, reg, argc, &invmcres, boa_string_getdata(mthname)))
        {
           return invmcres;
        }
        boa_vmmac_readframe(state, fiber);
        return boa_result_make(BOA_STATUS_INVALID, boa_value_makenull());
    }
    return boa_result_make(BOA_STATUS_OK, boa_value_makenull());
}

#define boa_vmmac_dobinaryop(state, fiber, typefn, opcode, opstring) \
    BoaValue cv; \
    BoaValue bv; \
    BoaValue res; \
    BoaValue tmpb; \
    BoaValue tmpval; \
    BoaResult invres; \
    bool isinst; \
    BoaNumber dnbv; \
    BoaNumber dncv; \
    uint64_t ra; \
    uint64_t rb; \
    uint64_t rc; \
    const char* snbv; \
    const char* sncv; \
    ra = boa_vmutil_geta(state->vmstate.instruction); \
    rb = boa_vmutil_getb(state->vmstate.instruction); \
    rc = boa_vmutil_getc(state->vmstate.instruction); \
    bv = boa_vmmac_getrc(state, rb); \
    cv = boa_vmmac_getrc(state, rc); \
    if(BOA_UNLIKELY(boa_value_isnumber(bv) && !boa_value_isnumber(cv))) \
    { \
        snbv = boa_value_valtypename(bv); \
        sncv = boa_value_valtypename(cv); \
        boa_vmmac_fail(state, "srcline %d: attempt to use operator '%s' with a %s and a %s", __LINE__, boa_string_getdata(opstring), snbv, sncv); \
    } \
    isinst = (boa_value_isinstance(bv) && boa_value_asinstance(bv)->klass == state->stdclassnumber); \
    if(boa_value_isnumber(bv) || isinst) \
    { \
        if(BOA_UNLIKELY(isinst)) \
        { \
            tmpval = boa_instance_getthis(boa_value_asinstance(bv)); \
            if(boa_value_isnull(tmpval)) \
            { \
                boa_vmmac_fail(state, "failed to extract 'this' value from Number instance"); \
            } \
            dnbv = boa_value_asnumber(tmpval); \
        } \
        else \
        { \
            dnbv = boa_value_asnumber(bv); \
        } \
        dncv = boa_value_asnumber(cv); \
        res = boa_value_makenull(); \
        switch(opcode) \
        { \
            case BOA_OPCODE_MATHADD: \
                { \
                    res = typefn(dnbv + dncv); \
                } \
                break; \
            case BOA_OPCODE_MATHSUBTRACT: \
                { \
                    res = typefn(dnbv - dncv); \
                } \
                break; \
            case BOA_OPCODE_MATHMULTIPLY: \
                { \
                    res = typefn(dnbv * dncv); \
                } \
                break; \
            case BOA_OPCODE_MATHDIVIDE: \
                { \
                    res = typefn(dnbv / dncv); \
                } \
                break; \
            default: \
                { \
                    boa_vmmac_fail(state, "INTERNAL ERROR: no case clause for op %d (%s)", opcode, boa_string_getdata(opstring)); \
                } \
                break; \
        } \
        state->vmstate.vmregisteritems[ra] = res; \
    } \
    else \
    { \
        if(BOA_UNLIKELY(boa_value_isnull(bv))) \
        { \
            boa_vmmac_fail(state, "srcline %d: attempt to use operator '%s' on a null value", __LINE__, boa_string_getdata(opstring)); \
        } \
        state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb); \
        tmpb = state->vmstate.vmregisteritems[ra + 1]; \
        state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc); \
        invres = boa_vmmac_invokeoperatormethoddefault(state, (fiber), ra, state->vmstate.vmregisteritems[ra], opstring, 1); \
        if(BOA_UNLIKELY(invres.type != BOA_STATUS_OK)) \
        { \
            if(invres.type == BOA_STATUS_INVALID) \
            { \
                boa_vmmac_dispatchnext(); \
            } \
            return invres; \
        } \
        state->vmstate.vmregisteritems[ra + 1] = tmpb; \
    }

#define boa_vmmac_docomparisonop(typefn, opcode, opstring) \
    BoaValue cv; \
    BoaValue bv; \
    BoaValue res; \
    BoaValue tmpb; \
    BoaResult invres; \
    uint64_t ra; \
    uint64_t rc; \
    uint64_t rb; \
    const char* snbv; \
    const char* sncv; \
    ra = boa_vmutil_geta(state->vmstate.instruction); \
    rb = boa_vmutil_getb(state->vmstate.instruction); \
    rc = boa_vmutil_getc(state->vmstate.instruction); \
    bv = boa_vmmac_getrc(state, rb); \
    cv = boa_vmmac_getrc(state, rc); \
    if(boa_value_isnumber(bv)) \
    { \
        if(!boa_value_isnumber(cv)) \
        { \
            snbv = boa_value_valtypename(bv); \
            sncv = boa_value_valtypename(cv); \
            boa_vmmac_fail(state, "srcline %d: attempt to use operator '%s' with a %s and a %s", __LINE__, boa_string_getdata(opstring), snbv, sncv); \
        } \
        res = boa_value_makenull();\
        switch(opcode) \
        { \
            case BOA_OPCODE_LESSTHAN: \
                { \
                    res = typefn(boa_value_asnumber(bv) < boa_value_asnumber(cv)); \
                } \
                break; \
            case BOA_OPCODE_LESSEQUAL: \
                { \
                    res = typefn(boa_value_asnumber(bv) <= boa_value_asnumber(cv)); \
                } \
                break; \
            case BOA_OPCODE_GREATERTHAN: \
                { \
                    res = typefn(boa_value_asnumber(bv) > boa_value_asnumber(cv)); \
                } \
                break; \
            case BOA_OPCODE_GREATEREQUAL: \
                { \
                    res = typefn(boa_value_asnumber(bv) >= boa_value_asnumber(cv)); \
                } \
                break; \
            default: \
                { \
                    boa_vmmac_fail(state, "INTERNAL ERROR: no case clause for op %d (%s)", opcode, boa_string_getdata(opstring)); \
                } \
                break; \
        } \
        state->vmstate.vmregisteritems[ra] = res; \
    } \
    else if(boa_value_isnull(bv)) \
    { \
        boa_vmmac_fail(state, "srcline %d: attempt to use operator '%s' on a null value", __LINE__, boa_string_getdata(opstring)); \
    } \
    else \
    { \
        state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb); \
        tmpb = state->vmstate.vmregisteritems[ra + 1]; \
        state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc); \
        invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], opstring, 1); \
        if(invres.type != BOA_STATUS_OK) \
        { \
            if(invres.type == BOA_STATUS_INVALID) \
            { \
                boa_vmmac_dispatchnext(); \
            } \
            return invres; \
        } \
        state->vmstate.vmregisteritems[ra + 1] = tmpb; \
    }

BoaResult boa_state_execfiber(BoaState* state, BoaFiber* fiber)
{
    bool traceforcenl;
    size_t traceofs;
    size_t tracemaxreg;
    BoaCallFrame* previousframe;
    BoaTable* currentvmglobals;
#if defined(BOA_CONF_USECOMPUTEDGOTO) && (BOA_CONF_USECOMPUTEDGOTO == 1)
    static void* dispatchtable[] = {
        &&LABELNAME(BOA_OPCODE_MOVE),
        &&LABELNAME(BOA_OPCODE_LOADNULL),
        &&LABELNAME(BOA_OPCODE_LOADBOOL),
        &&LABELNAME(BOA_OPCODE_MAKECLOSURE),
        &&LABELNAME(BOA_OPCODE_MAKEARRAY),
        &&LABELNAME(BOA_OPCODE_MAKEOBJECT),
        &&LABELNAME(BOA_OPCODE_MAKERANGE),
        &&LABELNAME(BOA_OPCODE_RETURN),
        &&LABELNAME(BOA_OPCODE_MATHADD),
        &&LABELNAME(BOA_OPCODE_MATHSUBTRACT),
        &&LABELNAME(BOA_OPCODE_MATHMULTIPLY),
        &&LABELNAME(BOA_OPCODE_MATHDIVIDE),
        &&LABELNAME(BOA_OPCODE_MATHFLOORDIVIDE),
        &&LABELNAME(BOA_OPCODE_MATHMOD),
        &&LABELNAME(BOA_OPCODE_MATHPOWER),
        &&LABELNAME(BOA_OPCODE_MATHLEFTSHIFT),
        &&LABELNAME(BOA_OPCODE_MATHRIGHTSHIFT),
        &&LABELNAME(BOA_OPCODE_BINXOR),
        &&LABELNAME(BOA_OPCODE_BINAND),
        &&LABELNAME(BOA_OPCODE_BINOR),
        &&LABELNAME(BOA_OPCODE_JUMP),
        &&LABELNAME(BOA_OPCODE_JUMPIFTRUE),
        &&LABELNAME(BOA_OPCODE_JUMPIFFALSE),
        &&LABELNAME(BOA_OPCODE_JUMPIFNONNULL),
        &&LABELNAME(BOA_OPCODE_JUMPIFNULL),
        &&LABELNAME(BOA_OPCODE_EQUAL),
        &&LABELNAME(BOA_OPCODE_LESSTHAN),
        &&LABELNAME(BOA_OPCODE_LESSEQUAL),
        &&LABELNAME(BOA_OPCODE_GREATERTHAN),
        &&LABELNAME(BOA_OPCODE_GREATEREQUAL),
        &&LABELNAME(BOA_OPCODE_NEGATE),
        &&LABELNAME(BOA_OPCODE_NOT),
        &&LABELNAME(BOA_OPCODE_BINNOT),
        &&LABELNAME(BOA_OPCODE_GLOBALSET),
        &&LABELNAME(BOA_OPCODE_GLOBALGET),
        &&LABELNAME(BOA_OPCODE_UPVALUESET),
        &&LABELNAME(BOA_OPCODE_UPVALUEGET),
        &&LABELNAME(BOA_OPCODE_PRIVATESET),
        &&LABELNAME(BOA_OPCODE_PRIVATEGET),
        &&LABELNAME(BOA_OPCODE_CALLCALLABLE),
        &&LABELNAME(BOA_OPCODE_UPVALUECLOSE),
        &&LABELNAME(BOA_OPCODE_CLASSMAKE),
        &&LABELNAME(BOA_OPCODE_CLASSPUTFIELDSTATIC),
        &&LABELNAME(BOA_OPCODE_CLASSPUTMETHOD),
        &&LABELNAME(BOA_OPCODE_FIELDGET),
        &&LABELNAME(BOA_OPCODE_CLASSGETSUPERMETHOD),
        &&LABELNAME(BOA_OPCODE_FIELDSET),
        &&LABELNAME(BOA_OPCODE_IS),
        &&LABELNAME(BOA_OPCODE_INVOKE),
        &&LABELNAME(BOA_OPCODE_INVOKESUPER),
        &&LABELNAME(BOA_OPCODE_SUBSCRIPTGET),
        &&LABELNAME(BOA_OPCODE_SUBSCRIPTSET),
        &&LABELNAME(BOA_OPCODE_ARRAYPUSH),
        &&LABELNAME(BOA_OPCODE_OBJECTPUSH),
        &&LABELNAME(BOA_OPCODE_REFGLOBAL),
        &&LABELNAME(BOA_OPCODE_REFPRIVATE),
        &&LABELNAME(BOA_OPCODE_REFLOCAL),
        &&LABELNAME(BOA_OPCODE_REFUPVALUE),
        &&LABELNAME(BOA_OPCODE_REFFIELD),
        &&LABELNAME(BOA_OPCODE_REFSET),
        &&LABELNAME(BOA_OPCODE_PUSHTRY),
        &&LABELNAME(BOA_OPCODE_POPTRY),
        &&LABELNAME(BOA_OPCODE_THROW),
        &&LABELNAME(BOA_OPCODE_RETHROW),
    };
#endif

    state->vmstate.fiber = fiber;
    currentvmglobals = &state->vmstate.globals->innertable;
    boa_vmexec_pushgc(state, true) fiber->muststop = false;
    boa_vmmac_readframe(state, &fiber);
    state->vmstate.fiber = fiber;
    state->vmstate.vmregisteritems[0] = boa_value_fromobject(state->vmstate.frame->function);
    if(BOA_UNLIKELY(state->config.traceexecution))
    {
        boa_stream_printf(state->config.desttrace, "fiber start:\n");
    }

dispatch:
    state->vmstate.instruction = *state->vmstate.ip++;
    if(BOA_UNLIKELY(state->config.traceexecution))
    {
        previousframe = state->vmstate.frame;
        traceofs = (size_t)(state->vmstate.ip - state->vmstate.currentchunk->compiledcodechunk - 1);
        traceforcenl = state->vmstate.frame != previousframe;
        boa_debug_disasinstr(state->config.desttrace, state->vmstate.currentchunk, traceofs, NULL, traceforcenl);
        if(!state->config.traceinstsonly)
        {
            if(BOA_LIKELY(state->vmstate.frame->function->maxregisters > 0))
            {
                tracemaxreg = state->vmstate.frame->function->maxregisters;
                boa_debug_traceprintvalue(state->config.desttrace, "<vm:registers>", fiber->framecount, tracemaxreg, state->vmstate.vmregisteritems);
            }
        }
        previousframe = state->vmstate.frame;
    }
#if defined(BOA_CONF_USECOMPUTEDGOTO) && (BOA_CONF_USECOMPUTEDGOTO == 1)
    goto* dispatchtable[boa_vmutil_getopcode(state->vmstate.instruction)];
#else
    switch(boa_vmutil_getopcode(state->vmstate.instruction))
#endif
    {
        CASE_CODE(BOA_OPCODE_MOVE)
        {
            uint64_t ra;
            uint64_t rb;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getbx(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb);
            boa_vmmac_dispatchnext()
        }
        CASE_CODE(BOA_OPCODE_LOADNULL)
        {
            uint64_t ra;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_makenull();
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_LOADBOOL)
        {
            uint64_t ra;
            uint64_t rb;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_makebool(rb != 0);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MAKECLOSURE)
        {
            size_t i;
            uint64_t index;
            uint64_t rbx;
            uint64_t ra;
            BoaFuncClosure* closure;
            BoaClsPrototype* clsproto;
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            ra = boa_vmutil_geta(state->vmstate.instruction);
            clsproto = boa_value_asclsproto(state->vmstate.vmconstantvalues[rbx]);
            closure = boa_object_makeclosure(state, clsproto->function);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(closure);
            for(i = 0; i < closure->function->upvaluecount; i++)
            {
                index = clsproto->indexes[i];
                if(clsproto->local[i])
                {
                    closure->closureupvalueitems[i] = boa_vmexec_captureupvalue(state, state->vmstate.vmregisteritems + index);
                }
                else
                {
                    closure->closureupvalueitems[i] = state->vmstate.vmupvalueitems[index];
                }
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MAKEARRAY)
        {
            size_t sz;
            uint64_t ra;
            uint64_t rb;
            BoaArray* array;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            array = boa_array_make(state);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(array);
            sz = rb;
            if(sz > array->innerlist.listcapacity)
            {
                if(array->innerlist.listcapacity > 0)
                {
                    sz = array->innerlist.listcapacity - 1;
                }
                else
                {
                    sz = 0;
                }
            }
            boa_dynlistval_ensureactualsize(&array->innerlist, sz);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MAKEOBJECT)
        {
            uint64_t ra;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(boa_object_makeinstance(state, state->stdobjectclass));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MAKERANGE)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            BoaValue first;
            BoaValue second;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            first = boa_vmmac_getrc(state, rb);
            second = boa_vmmac_getrc(state, rc);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(boa_object_makerange(state, boa_value_asnumber(first), boa_value_asnumber(second)));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_RETURN)
        {
            BoaValue value;
            while(fiber->handlercount > 0 && fiber->handleritems[fiber->handlercount - 1].framecount >= fiber->framecount)
            {
                fiber->handlercount--;
            }
            value = state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)];
            boa_vmexec_closeupvalues(state, state->vmstate.vmregisteritems);
            fiber->framecount--;
            if(state->vmstate.frame->returntoc)
            {
                state->vmstate.frame->returntoc = false;
                fiber->module->returnvalue = value;
                return boa_result_make(BOA_STATUS_OK, value);
            }
            if(fiber->framecount == 0 || state->vmstate.frame->returnaddress == NULL)
            {
                if(fiber->framecount == 0)
                {
                    fiber->module->returnvalue = value;
                }
                if(fiber->parent != NULL)
                {
                    state->vmstate.fiber = fiber->parent;
                    if(state->vmstate.fiber->returnaddress != NULL)
                    {
                        *state->vmstate.fiber->returnaddress = value;
                    }
                    if(BOA_UNLIKELY(state->config.traceexecution))
                    {
                        boa_stream_printf(state->config.desttrace, "fiber continue:\n");
                    }
                    boa_vmmac_readframe(state, &fiber);
                    boa_vmmac_dispatchnext();
                }
                return boa_result_make(BOA_STATUS_OK, value);
            }
            *state->vmstate.frame->returnaddress = value;
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHADD)
        {
            boa_vmmac_dobinaryop(state, &fiber, boa_value_makenumber, BOA_OPCODE_MATHADD, state->strings.stropplus);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHSUBTRACT)
        {
            boa_vmmac_dobinaryop(state, &fiber, boa_value_makenumber, BOA_OPCODE_MATHSUBTRACT, state->strings.stropminus);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHMULTIPLY)
        {
            boa_vmmac_dobinaryop(state, &fiber, boa_value_makenumber, BOA_OPCODE_MATHMULTIPLY, state->strings.stropmultiply);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHDIVIDE)
        {
            boa_vmmac_dobinaryop(state, &fiber, boa_value_makenumber, BOA_OPCODE_MATHDIVIDE, state->strings.stropdivide);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHFLOORDIVIDE)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            BoaValue bv;
            BoaValue cv;
            BoaValue tmpb;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, rb);
            cv = boa_vmmac_getrc(state, rc);
            if(BOA_LIKELY(boa_value_isnumber(bv) && boa_value_isnumber(cv)))
            {
                state->vmstate.vmregisteritems[ra] = boa_value_makenumber(floor(boa_value_asnumber(bv) / boa_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb);
                tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc);
                {
                    invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], state->strings.stropfloordiv, 1);
                    if(invres.type != BOA_STATUS_OK)
                    {
                        if(invres.type == BOA_STATUS_INVALID)
                        {
                            boa_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHMOD)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            int64_t nintbv;
            int64_t nintbc;
            BoaNumber nddbv;
            BoaNumber nddbc;
            BoaValue bv;
            BoaValue cv;
            BoaValue res;
            BoaValue tmpb;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, rb);
            cv = boa_vmmac_getrc(state, rc);
            if(boa_value_isnumber(bv) && boa_value_isnumber(cv))
            {
                nddbv = boa_value_asnumber(bv);
                nddbc = boa_value_asnumber(cv);
                nintbv = ((int64_t)nddbv);
                nintbc = ((int64_t)nddbc);
                if((nintbv == nddbv) && (nintbc == nddbc))
                {
                    res = boa_value_makenumber(nintbv % nintbc);
                }
                else
                {
                    res = boa_value_makenumber(fmod(nddbv, nddbc));
                }
                state->vmstate.vmregisteritems[ra] = res;
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb);
                tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc);
                {
                    invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], state->strings.stropmodulo, 1);
                    if(invres.type != BOA_STATUS_OK)
                    {
                        if(invres.type == BOA_STATUS_INVALID)
                        {
                            boa_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHPOWER)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            BoaValue bv;
            BoaValue cv;
            BoaValue tmpb;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, rb);
            cv = boa_vmmac_getrc(state, rc);
            if(boa_value_isnumber(bv) && boa_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[ra] = boa_value_makenumber(pow(boa_value_asnumber(bv), boa_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb);
                tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc);
                {
                    invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], state->strings.stroppower, 1);
                    if(invres.type != BOA_STATUS_OK)
                    {
                        if(invres.type == BOA_STATUS_INVALID)
                        {
                            boa_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHLEFTSHIFT)
        {
            int32_t ivbv;
            int32_t ivbc;
            BoaValue bv;
            BoaValue cv;
            BoaValue res;
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            cv = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            if(!boa_value_isnumber(bv) && !boa_value_isnumber(cv))
            {
                boa_vmmac_fail(state, "operands of '%s' must be two numbers, got %s and %s", "<<", boa_value_valtypename(bv), boa_value_valtypename(cv));
            }
            ivbv = (uint32_t)boa_value_asnumber(bv);
            ivbc = (uint32_t)boa_value_asnumber(cv);
            {
                res = boa_value_makenumber(ivbv << ivbc);
            }
            state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)] = res;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_MATHRIGHTSHIFT)
        {
            int32_t ivbv;
            int32_t ivbc;
            BoaValue bv;
            BoaValue cv;
            BoaValue res;
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            cv = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            if(!boa_value_isnumber(bv) && !boa_value_isnumber(cv))
            {
                boa_vmmac_fail(state, "operands of '%s' must be two numbers, got %s and %s", ">>", boa_value_valtypename(bv), boa_value_valtypename(cv));
            }
            ivbv = (uint32_t)boa_value_asnumber(bv);
            ivbc = (uint32_t)boa_value_asnumber(cv);
            {
                res = boa_value_makenumber(ivbv >> ivbc);
            }
            state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)] = res;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_BINXOR)
        {
            uint64_t ra;
            int32_t nbv;
            int32_t ncv;
            BoaValue bv;
            BoaValue cv;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            cv = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            if(!boa_value_isnumber(bv) && !boa_value_isnumber(cv))
            {
                boa_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "^", boa_value_valtypename(bv), boa_value_valtypename(cv));
            }
            nbv = (uint32_t)boa_value_asnumber(bv);
            ncv = (uint32_t)boa_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (boa_value_makenumber(nbv ^ ncv));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_BINAND)
        {
            uint64_t ra;
            int32_t nbv;
            int32_t ncv;
            BoaValue bv;
            BoaValue cv;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            cv = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            if(!boa_value_isnumber(bv) && !boa_value_isnumber(cv))
            {
                boa_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "&", boa_value_valtypename(bv), boa_value_valtypename(cv));
            }
            nbv = (uint32_t)boa_value_asnumber(bv);
            ncv = (uint32_t)boa_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (boa_value_makenumber(nbv & ncv));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_BINOR)
        {
            uint64_t ra;
            int32_t nbv;
            int32_t ncv;
            BoaValue bv;
            BoaValue cv;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            cv = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            if(!boa_value_isnumber(bv) && !boa_value_isnumber(cv))
            {
                boa_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "|", boa_value_valtypename(bv), boa_value_valtypename(cv));
            }
            nbv = (uint32_t)boa_value_asnumber(bv);
            ncv = (uint32_t)boa_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (boa_value_makenumber(nbv | ncv));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_JUMP)
        {
            state->vmstate.ip += boa_vmutil_getsbx(state->vmstate.instruction);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_JUMPIFTRUE)
        {
            if(!boa_value_isfalsy(state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)]))
            {
                state->vmstate.ip += boa_vmutil_getbx(state->vmstate.instruction);
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_JUMPIFFALSE)
        {
            if(boa_value_isfalsy(state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)]))
            {
                state->vmstate.ip += boa_vmutil_getbx(state->vmstate.instruction);
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_JUMPIFNONNULL)
        {
            if(!boa_value_isnull(state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)]))
            {
                state->vmstate.ip += boa_vmutil_getbx(state->vmstate.instruction);
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_JUMPIFNULL)
        {
            uint64_t ra;
            uint64_t rbx;
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            ra = boa_vmutil_geta(state->vmstate.instruction);
            if(boa_value_isnull(state->vmstate.vmregisteritems[ra]))
            {
                state->vmstate.ip += rbx;
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_EQUAL)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            BoaValue bv;
            BoaValue ptmp;
            BoaValue tmpb;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            bv = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            if(boa_value_isinstance(bv))
            {
                state->vmstate.vmregisteritems[ra] = boa_vmmac_getrc(state, rb);
                tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rc);
                {
                    invres = boa_vmmac_invokeoperatormethodandcontinue(state, &fiber, ra, state->vmstate.vmregisteritems[ra], state->strings.stropequal, 1);
                    if(invres.type != BOA_STATUS_OK)
                    {
                        if(invres.type == BOA_STATUS_INVALID)
                        {
                            boa_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            ptmp = boa_vmmac_getrc(state, rc);
            state->vmstate.vmregisteritems[ra] = boa_value_makebool(boa_value_compare(state, bv, ptmp));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_LESSTHAN)
        {
            boa_vmmac_docomparisonop(boa_value_makebool, BOA_OPCODE_LESSTHAN, state->strings.stroplessthan);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_LESSEQUAL)
        {
            boa_vmmac_docomparisonop(boa_value_makebool, BOA_OPCODE_LESSEQUAL, state->strings.stroplessequal);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_GREATERTHAN)
        {
            boa_vmmac_docomparisonop(boa_value_makebool, BOA_OPCODE_GREATERTHAN, state->strings.stropgreaterthan);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_GREATEREQUAL)
        {
            boa_vmmac_docomparisonop(boa_value_makebool, BOA_OPCODE_GREATEREQUAL, state->strings.stropgreaterequal);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_NEGATE)
        {
            uint64_t ra;
            uint64_t rb;
            BoaNumber dn;
            BoaValue value;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            value = boa_vmmac_getrc(state, rb);
            if(!boa_value_isnumber(value))
            {
                boa_vmmac_fail(state, "operand must be a number");
            }
            dn = boa_value_asnumber(value);
            state->vmstate.vmregisteritems[ra] = boa_value_makenumber(-dn);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_NOT)
        {
            uint64_t ra;
            uint64_t rb;
            BoaValue value;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            value = boa_vmmac_getrc(state, rb);
            if(boa_value_isinstance(value))
            {
                invres = boa_vmmac_invokeoperatormethodandcontinue(state, &fiber, rb, value, state->strings.stropnot, 0);
                if(invres.type != BOA_STATUS_OK)
                {
                    if(invres.type == BOA_STATUS_INVALID)
                    {
                        boa_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra] = boa_value_makebool(boa_value_isfalsy(value));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_BINNOT)
        {
            uint64_t rb;
            uint64_t ra;
            BoaNumber dn;
            int32_t cn;
            BoaValue value;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            value = boa_vmmac_getrc(state, rb);
            if(!boa_value_isnumber(value))
            {
                boa_vmmac_fail(state, "operand must be a number");
            }
            dn = boa_value_asnumber(value);
            cn = (uint32_t)dn;
            state->vmstate.vmregisteritems[ra] = boa_value_makenumber(~cn);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_GLOBALSET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            boa_table_set(currentvmglobals, boa_value_asstring(state->vmstate.vmconstantvalues[ra]), boa_vmmac_getrc(state, rbx));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_GLOBALGET)
        {
            uint64_t ra;
            uint64_t rbx;
            BoaValue* reg;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            reg = &state->vmstate.vmregisteritems[ra];
            if(!boa_table_getentry(currentvmglobals, boa_value_asstring(state->vmstate.vmconstantvalues[rbx]), reg))
            {
                *reg = boa_value_makenull();
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_UPVALUESET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            *state->vmstate.frame->closure->closureupvalueitems[ra]->location = boa_vmmac_getrc(state, rbx);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_UPVALUEGET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = *state->vmstate.frame->closure->closureupvalueitems[rbx]->location;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_PRIVATESET)
        {
            uint64_t ra;
            uint64_t rbx;
            BoaValue res;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            if(BOA_BIT_ISSET(rbx, BOA_BITFLAG_VMCONST))
            {
                res = state->vmstate.vmconstantvalues[ra];
            }
            else
            {
                res = state->vmstate.vmregisteritems[ra];
            }
            state->vmstate.vmprivatevalues[(uint16_t)rbx] = res;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_PRIVATEGET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = state->vmstate.vmprivatevalues[rbx];
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_CALLCALLABLE)
        {
            BoaValue altcallee;
            uint64_t calleereg;
            uint64_t argc;
            const char* funcname;
            boa_vmmac_writeframe(state);
            calleereg = boa_vmutil_geta(state->vmstate.instruction);
            argc = boa_vmutil_getb(state->vmstate.instruction);
            altcallee = boa_value_makenull();
            funcname = boa_string_getdata(boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]));
            if(!boa_vmexec_actualcallvalue(state, calleereg, argc - 1, altcallee, funcname))
            {
                boa_vmexec_popgc(state);
                return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());                
            }
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_UPVALUECLOSE)
        {
            uint64_t ra;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            boa_vmexec_closeupvalues(state, (&state->vmstate.vmregisteritems[ra]) - 1);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_CLASSMAKE)
        {
            uint64_t rb;
            BoaValue super;
            BoaString* name;
            BoaClass* klass;
            BoaClass* superklass;
            name = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_geta(state->vmstate.instruction)]);
            klass = boa_object_makeclass(state, name);
            state->vmstate.vmregisteritems[boa_vmutil_getc(state->vmstate.instruction)] = boa_value_fromobject(klass);
            boa_map_setvalue(state->vmstate.globals, name, boa_value_fromobject(klass));
            rb = boa_vmutil_getb(state->vmstate.instruction);
            if(rb == 0)
            {
                klass->super = state->stdobjectclass;
                boa_table_addall(&klass->super->mthtable, &klass->mthtable);
                boa_table_addall(&klass->super->staticstable, &klass->staticstable);
            }
            else
            {
                super = state->vmstate.vmregisteritems[--rb];
                if(!boa_value_isclass(super))
                {
                    boa_vmmac_fail(state, "superclass must be a class");
                }
                superklass = boa_value_asclass(super);
                klass->super = superklass;
                klass->mthconstructor = superklass->mthconstructor;
                boa_table_addall(&superklass->mthtable, &klass->mthtable);
                boa_table_addall(&klass->super->staticstable, &klass->staticstable);
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_CLASSPUTFIELDSTATIC)
        {
            BoaClass* klass;
            BoaValue vklass;
            BoaValue setkey;
            BoaValue setval;
            vklass = state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)];
            setkey = state->vmstate.vmconstantvalues[boa_vmutil_getb(state->vmstate.instruction)];
            setval = boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction));
            klass = boa_value_asclass(vklass);
            boa_table_set(&klass->staticstable, boa_value_asstring(setkey), setval);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_CLASSPUTMETHOD)
        {
            size_t ctorlen;
            size_t mthlen;
            const char* mthstr;
            const char* ctorstr;
            BoaClass* klass;
            BoaString* name;
            ctorlen = boa_string_getlength(state->strings.strconstructor);
            ctorstr = boa_string_getdata(state->strings.strconstructor);
            klass = boa_value_asclass(state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)]);
            name = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getb(state->vmstate.instruction)]);
            mthlen = boa_string_getlength(name);
            mthstr = boa_string_getdata(name);
            if((klass->mthconstructor == NULL || (klass->super != NULL && klass->mthconstructor == ((BoaClass*)klass->super)->mthconstructor)) && mthlen == ctorlen && memcmp(mthstr, ctorstr, ctorlen) == 0)
            {
                klass->mthconstructor = boa_value_asobject(boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction)));
            }
            boa_table_set(&klass->mthtable, name, boa_vmmac_getrc(state, boa_vmutil_getc(state->vmstate.instruction)));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_FIELDGET)
        {
            uint64_t ra;
            BoaValue value;
            BoaResult tmpres;
            BoaValue object;
            BoaString* name;
            BoaField* field;
            BoaClass* klass;
            BoaInstance* instance;
            object = state->vmstate.vmregisteritems[boa_vmutil_getb(state->vmstate.instruction)];
            name = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]);
            #if 0
            if(boa_value_isnull(object))
            {
                boa_vmmac_fail(state, "attempt to get '%s' of a null value", boa_string_getdata(name));
            }
            #endif
            ra = boa_vmutil_geta(state->vmstate.instruction);
            if(boa_value_isinstance(object))
            {
                instance = boa_value_asinstance(object);
                if(!boa_table_getentry(&instance->fields, name, &value))
                {
                    if(boa_table_getentry(&instance->klass->mthtable, name, &value))
                    {
                        if(boa_value_isfield(value))
                        {
                            field = boa_value_asfield(value);
                            if(field->getter == NULL)
                            {
                                boa_vmmac_fail(state, "class %s does not have a getter for the field %s", boa_string_getdata(instance->klass->name), boa_string_getdata(name));
                            }
                            boa_vmmac_writeframe(state);
                            field = boa_value_asfield(value);
                            if(!boa_vmmac_callvalue(state, &fiber, boa_value_fromobject(field->getter), ra, 0, &tmpres, "<Field::getter>"))
                            {
                                return tmpres;
                            }
                            boa_vmmac_readframe(state, &fiber);
                            boa_vmmac_dispatchnext();
                        }
                        else
                        {
                            value = boa_value_fromobject(boa_object_makeboundmethod(state, object, value));
                        }
                    }
                    else
                    {
                        value = boa_value_makenull();
                    }
                }
            }
            else if(boa_value_isclass(object))
            {
                klass = boa_value_asclass(object);
                if(boa_table_getentry(&klass->staticstable, name, &value))
                {
                    if(boa_value_isfuncmethod(value))
                    {
                        value = boa_value_fromobject(boa_object_makeboundmethod(state, object, value));
                    }
                    else if(boa_value_isfield(value))
                    {
                        field = boa_value_asfield(value);
                        if(field->getter == NULL)
                        {
                            boa_vmmac_fail(state, "class %s does not have a getter for the field %s", boa_string_getdata(klass->name), boa_string_getdata(name));
                        }
                        boa_vmmac_writeframe(state);
                        if(!boa_vmmac_callvalue(state, &fiber, boa_value_fromobject(field->getter), ra, 0, &tmpres, "<Field::getter>"))
                        {
                            return tmpres;
                        }
                        boa_vmmac_readframe(state, &fiber);
                        boa_vmmac_dispatchnext();
                    }
                }
                else
                {
                    value = boa_value_makenull();
                }
            }
            else
            {
                if(boa_value_ismap(object))
                {
                    if(!boa_map_getvalue(boa_value_asmap(object), name, &value))
                    {
                        value = boa_value_makenull();
                    }
                    state->vmstate.vmregisteritems[ra] = value;
                    boa_vmmac_dispatchnext();
                }
                klass = boa_state_getclassfor(state, object);
                if(klass == NULL)
                {
                    boa_vmmac_fail(state, "only instances and classes have fields");
                }
                if(boa_table_getentry(&klass->mthtable, name, &value))
                {
                    if(boa_value_isfield(value))
                    {
                        field = boa_value_asfield(value);
                        if(field->getter == NULL)
                        {
                            boa_vmmac_fail(state, "class %s does not have a getter for the field %s", boa_string_getdata(klass->name), boa_string_getdata(name));
                        }
                        boa_vmmac_writeframe(state);
                        field = boa_value_asfield(value);
                        if(!boa_vmmac_callvalue(state, &fiber, boa_value_fromobject(field->getter), ra, 0, &tmpres, "<Field::setter>"))
                        {
                            return tmpres;
                        }
                        boa_vmmac_readframe(state, &fiber);
                        boa_vmmac_dispatchnext();
                    }
                    else if(boa_value_isfuncmethod(value))
                    {
                        value = boa_value_fromobject(boa_object_makeboundmethod(state, object, value));
                    }
                }
                else
                {
                    value = boa_value_makenull();
                }
            }
            state->vmstate.vmregisteritems[ra] = value;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_CLASSGETSUPERMETHOD)
        {
            BoaValue value;
            BoaValue instance;
            BoaClass* klass;
            BoaString* mthname;
            instance = state->vmstate.vmregisteritems[boa_vmutil_getb(state->vmstate.instruction)];
            klass = boa_value_asclass(instance);
            mthname = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]);
            if(boa_table_getentry(&klass->mthtable, mthname, &value) || boa_table_getentry(&klass->staticstable, mthname, &value))
            {
                value = boa_value_fromobject(boa_object_makeboundmethod(state, state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)], value));
            }
            else
            {
                value = boa_value_makenull();
            }
            state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)] = value;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_FIELDSET)
        {
            BoaResult tmpres;
            BoaValue value;
            BoaValue instance;
            BoaValue setter;
            BoaClass* klass;
            BoaField* field;
            BoaInstance* inst;
            uint64_t ra;
            BoaString* fieldname;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra];
            fieldname = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getb(state->vmstate.instruction)]);
            if(boa_value_isnull(instance))
            {
                boa_vmmac_fail(state, "attempt to set field '%s' on a null value", boa_string_getdata(fieldname));
            }
            value = state->vmstate.vmregisteritems[boa_vmutil_getc(state->vmstate.instruction)];
            if(boa_value_isclass(instance))
            {
                klass = boa_value_asclass(instance);
                if(boa_table_getentry(&klass->staticstable, fieldname, &setter) && boa_value_isfield(setter))
                {
                    field = boa_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        boa_vmmac_fail(state, "class %s does not have a setter for the field %s", boa_string_getdata(klass->name), boa_string_getdata(fieldname));
                    }
                    boa_vmmac_writeframe(state);
                    if(!boa_vmmac_callvalue(state, &fiber, boa_value_fromobject(field->setter), ra, 1, &tmpres, "<Field::setter>"))
                    {
                        return tmpres;
                    }
                    boa_vmmac_readframe(state, &fiber);
                    boa_vmmac_dispatchnext();
                }
                if(boa_value_isnull(value))
                {
                    boa_table_delete(&klass->staticstable, fieldname);
                }
                else
                {
                    if(boa_value_iscallablefunction(value))
                    {
                        boa_table_set(&klass->mthtable, fieldname, value);
                    }
                    else
                    {
                        boa_table_set(&klass->staticstable, fieldname, value);
                    }
                }
            }
            else if(boa_value_isinstance(instance))
            {
                inst = boa_value_asinstance(instance);
                if(boa_table_getentry(&inst->klass->mthtable, fieldname, &setter) && boa_value_isfield(setter))
                {
                    field = boa_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        boa_vmmac_fail(state, "class %s does not have a setter for the field %s", boa_string_getdata(inst->klass->name), boa_string_getdata(fieldname));
                    }
                    boa_vmmac_writeframe(state);
                    if(!boa_vmmac_callvalue(state, &fiber,boa_value_fromobject(field->setter), ra, 1, &tmpres, "Field::setter"))
                    {
                        return tmpres;
                    }
                    boa_vmmac_readframe(state, &fiber);
                    boa_vmmac_dispatchnext();
                }
                if(boa_value_isnull(value))
                {
                    boa_table_delete(&inst->fields, fieldname);
                }
                else
                {
                    boa_table_set(&inst->fields, fieldname, value);
                }
            }
            else
            {
                klass = boa_state_getclassfor(state, instance);
                if(klass == NULL)
                {
                    boa_vmmac_fail(state, "only instances and classes have fields");
                }
                if(boa_table_getentry(&klass->mthtable, fieldname, &setter) && boa_value_isfield(setter))
                {
                    field = boa_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        boa_vmmac_fail(state, "class %s does not have a setter for the field %s", boa_string_getdata(klass->name), boa_string_getdata(fieldname));
                    }
                    boa_vmmac_writeframe(state);
                    if(!boa_vmmac_callvalue(state, &fiber, boa_value_fromobject(field->setter), ra, 1, &tmpres, "<Field::setter>"))
                    {
                        return tmpres;
                    }
                    boa_vmmac_readframe(state, &fiber);
                    boa_vmmac_dispatchnext();
                }
                else
                {
                    boa_vmmac_fail(state, "class %s does not contain field %s", boa_string_getdata(klass->name), boa_string_getdata(fieldname));
                }
            }
            state->vmstate.vmregisteritems[ra] = value;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_IS)
        {
            uint64_t ra;
            bool found;
            BoaValue instance;
            BoaClass* instanceklass;
            BoaValue klass;
            BoaClass* type;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            instance = boa_vmmac_getrc(state, boa_vmutil_getb(state->vmstate.instruction));
            #if 0
            if(boa_value_isnull(instance))
            {
                state->vmstate.vmregisteritems[ra] = boa_value_makebool(false);
                boa_vmmac_dispatchnext();
            }
            #endif
            instanceklass = boa_state_getclassfor(state, instance);
            if(!boa_table_getentry(currentvmglobals, boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]), &klass))
            {
                state->vmstate.vmregisteritems[ra] = boa_value_makebool(false);
                boa_vmmac_dispatchnext();
            }
            if(instanceklass == NULL || !boa_value_isclass(klass))
            {
                boa_vmmac_fail(state, "operands must be an instance and a class");
            }
            type = boa_value_asclass(klass);
            found = false;
            while(instanceklass != NULL)
            {
                if(instanceklass == type)
                {
                    found = true;
                    break;
                }
                instanceklass = (BoaClass*)instanceklass->super;
            }
            state->vmstate.vmregisteritems[ra] = boa_value_makebool(found);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_INVOKE)
        {
            uint8_t ra;
            int argc;
            BoaValue method;
            BoaValue instance;
            BoaResult tmpres;
            BoaClass* klass;
            BoaString* mthname;
            boa_vmmac_writeframe(state);
            ra = boa_vmutil_geta(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra];
            mthname = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]);
            #if 0
            if(boa_value_isnull(instance))
            {
                boa_vmmac_fail(state, "attempt to invoke '%s' on a null value", boa_string_getdata(mthname));
            }
            #endif
            klass = boa_value_isclass(instance) ? boa_value_asclass(instance) : boa_state_getclassfor(state, instance);
            if(klass == NULL)
            {
                boa_vmmac_fail(state, "only instances and classes have methods");
            }
            argc = boa_vmutil_getb(state->vmstate.instruction) - 1;
            if(boa_value_isinstance(instance) && (boa_table_getentry(&boa_value_asinstance(instance)->fields, mthname, &method)))
            {
                if(!boa_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres, boa_string_getdata(mthname)))
                {
                    return tmpres;
                }
            }
            else if(boa_value_isclass(instance) && boa_table_getentry(&klass->staticstable, mthname, &method))
            {
                if(!boa_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres, boa_string_getdata(mthname)))
                {
                    return tmpres;
                }
            }
            else if(boa_table_getentry(&klass->mthtable, mthname, &method))
            {
                if(!boa_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres, boa_string_getdata(mthname)))
                {
                    return tmpres;
                }
            }
            else
            {
                boa_vmmac_fail(state, "attempt to invoke undefined method '%s#%s'", boa_string_getdata(klass->name), boa_string_getdata(mthname));
            }
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_INVOKESUPER)
        {
            size_t i;
            int argc;
            uint64_t ra;
            BoaValue method;
            BoaValue instance;
            BoaResult tmpres;
            BoaClass* klass;
            BoaString* mthname;
            boa_vmmac_writeframe(state);
            ra = boa_vmutil_geta(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra + 1];
            if(boa_value_isnull(instance))
            {
                boa_vmmac_fail(state, "attempt to index a null value");
            }
            klass = boa_value_asclass(instance);
            if(klass == NULL)
            {
                boa_vmmac_fail(state, "only instances and classes have methods");
            }
            mthname = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]);
            argc = boa_vmutil_getb(state->vmstate.instruction) - 1;
            if(boa_table_getentry(&klass->mthtable, mthname, &method) || boa_table_getentry(&klass->staticstable, mthname, &method))
            {
                for(i = ra + 1; i <= ra + (size_t)argc; i++)
                {
                    state->vmstate.vmregisteritems[i] = state->vmstate.vmregisteritems[i + 1];
                }
                if(!boa_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres, boa_string_getdata(mthname)))
                {
                    return tmpres;
                }
            }
            else
            {
                boa_vmmac_fail(state, "attempt to invoke undefined method '%s#%s' of super class", boa_string_getdata(klass->name), boa_string_getdata(mthname));
            }
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_SUBSCRIPTGET)
        {
            uint64_t ra;
            uint64_t rb;
            BoaValue instance;
            BoaValue tmpb;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra];
            tmpb = state->vmstate.vmregisteritems[ra + 1];
            state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rb);
            {
                invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, instance, state->strings.stropindex, 1);
                if(invres.type != BOA_STATUS_OK)
                {
                    if(invres.type == BOA_STATUS_INVALID)
                    {
                        boa_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra + 1] = tmpb;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_SUBSCRIPTSET)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            BoaValue instance;
            BoaValue tmpb;
            BoaValue tmpc;
            BoaResult invres;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            rc = boa_vmutil_getc(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra];
            tmpb = state->vmstate.vmregisteritems[ra + 1];
            tmpc = state->vmstate.vmregisteritems[ra + 2];
            state->vmstate.vmregisteritems[ra + 1] = boa_vmmac_getrc(state, rb);
            state->vmstate.vmregisteritems[ra + 2] = boa_vmmac_getrc(state, rc);
            {
                invres = boa_vmmac_invokeoperatormethoddefault(state, &fiber, ra, instance, state->strings.stropindex, 2);
                if(invres.type != BOA_STATUS_OK)
                {
                    if(invres.type == BOA_STATUS_INVALID)
                    {
                        boa_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra + 1] = tmpb;
            state->vmstate.vmregisteritems[ra + 2] = tmpc;
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_ARRAYPUSH)
        {
            uint64_t ra;
            BoaValue pval;
            BoaArray* array;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            array = boa_value_asarray(state->vmstate.vmregisteritems[ra]);
            pval = boa_vmmac_getrc(state, boa_vmutil_getbx(state->vmstate.instruction));
            boa_array_push(array, pval);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_OBJECTPUSH)
        {
            BoaValue value;
            BoaValue operand;
            BoaString* key;
            operand = state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)];
            key = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getb(state->vmstate.instruction)]);
            value = state->vmstate.vmregisteritems[boa_vmutil_getc(state->vmstate.instruction)];
            if(boa_value_ismap(operand))
            {
                boa_map_setvalue(boa_value_asmap(operand), key, value);
            }
            else if(boa_value_isinstance(operand))
            {
                boa_table_set(&boa_value_asinstance(operand)->fields, key, value);
            }
            else
            {
                boa_vmmac_fail(state, "slotted an object or a map as the operand, got %s", boa_value_valtypename(operand));
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFGLOBAL)
        {
            BoaString* name;
            BoaValue* value;
            name = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getbx(state->vmstate.instruction)]);
            if(boa_table_getslot(&state->vmstate.globals->innertable, name, &value))
            {
                state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)] = boa_value_fromobject(boa_object_makereference(state, value));
            }
            else
            {
                boa_vmmac_fail(state, "attempt to reference a null value");
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFPRIVATE)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(boa_object_makereference(state, &state->vmstate.vmprivatevalues[rbx]));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFLOCAL)
        {
            uint64_t ra;
            uint64_t rb;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rb = boa_vmutil_getb(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(boa_object_makereference(state, &state->vmstate.vmregisteritems[rb]));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFUPVALUE)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = boa_value_fromobject(boa_object_makereference(state, state->vmstate.vmupvalueitems[rbx]->location));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFFIELD)
        {
            BoaValue object;
            BoaValue* value;
            BoaString* name;
            object = state->vmstate.vmregisteritems[boa_vmutil_getb(state->vmstate.instruction)];
            if(boa_value_isnull(object))
            {
                boa_vmmac_fail(state, "attempt to index a null value");
            }
            name = boa_value_asstring(state->vmstate.vmconstantvalues[boa_vmutil_getc(state->vmstate.instruction)]);
            if(boa_value_isinstance(object))
            {
                if(!boa_table_getslot(&boa_value_asinstance(object)->fields, name, &value))
                {
                    boa_vmmac_fail(state, "attempt to reference a null value");
                }
            }
            else
            {
                boa_vmmac_fail(state, "can only reference fields of real instances");
            }
            state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)] = boa_value_fromobject(boa_object_makereference(state, value));
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_REFSET)
        {
            BoaValue reference;
            reference = state->vmstate.vmregisteritems[boa_vmutil_geta(state->vmstate.instruction)];
            if(!boa_value_isreference(reference))
            {
                boa_vmmac_fail(state, "provided value is not a reference");
            }
            *boa_value_asreference(reference)->slot = state->vmstate.vmregisteritems[boa_vmutil_getb(state->vmstate.instruction)];
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_PUSHTRY)
        {
            uint64_t rbx;
            BoaHandler* handler;
            rbx = boa_vmutil_getbx(state->vmstate.instruction);
            if(fiber->handlercount >= fiber->handlercapacity)
            {
                fiber->handlercapacity = boa_util_grownextcapacity(fiber->handlercapacity);
                fiber->handleritems = (BoaHandler*)boa_sysmem_realloc(fiber->handleritems, fiber->handlercapacity * sizeof(BoaHandler));
            }
            handler = &fiber->handleritems[fiber->handlercount++];
            handler->handlerip = state->vmstate.ip + (int)rbx;
            handler->registercount = (uint32_t)(state->vmstate.vmregisteritems - fiber->registeritems);
            handler->framecount = fiber->framecount;
            handler->errorreg = boa_vmutil_geta(state->vmstate.instruction);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_POPTRY)
        {
            if(fiber->handlercount > 0)
            {
                fiber->handlercount--;
            }
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_THROW)
        {
            uint64_t ra;
            ra = boa_vmutil_geta(state->vmstate.instruction);
            if(!boa_vm_handleerror(state, state->vmstate.vmregisteritems[ra]))
            {
                return boa_result_make(BOA_STATUS_RUNTIMEERROR, fiber->error);
            }
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
        CASE_CODE(BOA_OPCODE_RETHROW)
        {
            if(!boa_vm_handleerror(state, fiber->error))
            {
                return boa_result_make(BOA_STATUS_RUNTIMEERROR, fiber->error);
            }
            boa_vmmac_readframe(state, &fiber);
            boa_vmmac_dispatchnext();
        }
#if !defined(BOA_CONF_USECOMPUTEDGOTO) || (BOA_CONF_USECOMPUTEDGOTO == 0)
        default:
#endif
        {
            boa_vmmac_fail(state, "unknown opcode %i", state->vmstate.instruction);
            boa_vmexec_popgc(state);
            return boa_result_make(BOA_STATUS_RUNTIMEERROR, boa_value_makenull());            
        }
    }
}

int boa_jmpstate_setnativeexit()
{
    return setjmp(g_vmglobaljumpbuf);
}

void boa_jmpstate_nativeexitjump()
{
    longjmp(g_vmglobaljumpbuf, 1);
}

static BoaState* greplstate = NULL;

void boa_repl_interupthandler(int signalid)
{
    boa_state_destroy(greplstate);
    fprintf(stderr, "\n""received ^C, exiting (signalid=%d).\n", signalid);
    exit(0);
}

#if defined(BOA_CONFIG_USELINO) && (BOA_CONFIG_USELINO == 1)
static char* boa_repl_getinput(linocontext_t* lictx, const char* prompt)
{
    return lino_context_readline(lictx, prompt);
}

static void boa_repl_addhistoryline(linocontext_t* lictx, const char* line)
{
    lino_context_historyadd(lictx, line);
}

static void boa_repl_freeline(linocontext_t* lictx, char* line)
{
    lino_context_freeline(lictx, line);
}

size_t boa_repl_makevarname(char* buf, int level)
{
    return sprintf(buf, "$%d", level);
}

const char* boa_repl_setresultvar(BoaState* state, int level, BoaValue val)
{
    size_t len;
    BoaString* str;
    char buf[128];
    len = boa_repl_makevarname(buf, level);
    str = boa_string_copylen(state, buf, len);
    boa_state_setglobal(state, str, val);
    return boa_string_getdata(str);
}

void boa_repl_runrepl(BoaState* state, linocontext_t* lictx)
{
    int level;
    char* line;
    const char* vname;
    BoaResult result;
    BoaValue value;
    BoaStream* pr;
    level = 1;
    pr = state->streamstdout;
    greplstate = state;
    signal(SIGINT, boa_repl_interupthandler);
#ifndef _WIN32
    signal(SIGTSTP, boa_repl_interupthandler);
#endif
    while(true)
    {
        line = boa_repl_getinput(lictx, "> ");
        if(line == NULL)
        {
            break;
        }
        boa_repl_addhistoryline(lictx, line);
        result = boa_state_interpretsource(state, "repl", line);
        if(result.type == BOA_STATUS_OK)
        {
            boa_stream_setcolor(pr, 'g');
            value = result.result;
            value = state->vmstate.frame->slots[1];
            vname = boa_repl_setresultvar(state, level, value);
            boa_stream_printf(pr, "%s = ", vname);
            boa_value_printvalue(pr, value, true);
            boa_stream_resetcolor(pr);
            boa_stream_puts(pr, "\n");
            level++;
        }
        boa_repl_freeline(lictx, line);
    }
}
#endif

static void boa_cli_parseenv(BoaState* state, char** envp)
{
    enum { kMaxKeyLen = 40 };
    size_t i;
    int len;
    int pos;
    char* raw;
    char* valbuf;
    char keybuf[kMaxKeyLen];
    BoaString* oskey;
    BoaString* osval;
    BoaMap* envmap;
    envmap = boa_object_makemap(state, NULL);
    if(envp == NULL)
    {
        return;
    }
    for(i=0; envp[i] != NULL; i++)
    {
        raw = envp[i];
        len = strlen(raw);
        pos = boa_util_strfindfirstchar(raw, len, '=');
        if(pos == -1)
        {
            fprintf(stderr, "malformed environment string '%s'\n", raw);
        }
        else
        {
            memset(keybuf, 0, kMaxKeyLen);
            memcpy(keybuf, raw, pos);
            valbuf = &raw[pos+1];
            oskey = boa_string_copy(state, keybuf);
            osval = boa_string_copy(state, valbuf);
            boa_map_setvalue(envmap, oskey, boa_value_fromobject(osval));
            
        }
    }
    boa_state_setglobal(state, boa_string_copy(state, "ENV"), boa_value_fromobject(envmap));
}

static void optprs_fprintmaybearg(FILE* out, const char* begin, const char* flagname, size_t flaglen, bool needval, bool maybeval, const char* delim)
{
    fprintf(out, "%s%.*s", begin, (int)flaglen, flagname);
    if(needval)
    {
        if(maybeval)
        {
            fprintf(out, "[");
        }
        if(delim != NULL)
        {
            fprintf(out, "%s", delim);
        }
        fprintf(out, "<val>");
        if(maybeval)
        {
            fprintf(out, "]");
        }
    }
}

static void optprs_fprintusage(FILE* out, optcontext_t* ox)
{
    int i;
    char ch;
    bool needval;
    bool maybeval;
    bool hadshort;
    optflag_t* flag;
    for(i=0; i<ox->knowncount; i++)
    {
        flag = &ox->knownflags[i];
        hadshort = false;
        needval = (flag->argtype > OPTPARSE_NONE);
        maybeval = (flag->argtype == OPTPARSE_OPTIONAL);
        if(flag->shortname > 0)
        {
            hadshort = true;
            ch = flag->shortname;
            fprintf(out, "    ");
            optprs_fprintmaybearg(out, "-", &ch, 1, needval, maybeval, NULL);
        }
        if(flag->longname != NULL)
        {
            if(hadshort)
            {
                fprintf(out, ", ");
            }
            else
            {
                fprintf(out, "    ");
            }
            optprs_fprintmaybearg(out, "--", flag->longname, strlen(flag->longname), needval, maybeval, "=");
        }
        if(flag->helptext != NULL)
        {
            fprintf(out, "  -  %s", flag->helptext);
        }
        fprintf(out, "\n");
    }
}

static void boa_cli_showusage(char* argv[], optcontext_t* ox, bool fail)
{
    FILE* out;
    out = fail ? stderr : stdout;
    fprintf(out, "usage: %s [<options>] [<filename> | -e <code>]\n", argv[0]);
    optprs_fprintusage(out, ox);
}

typedef struct BoaCliOptions BoaCliOptions;
struct BoaCliOptions
{
    BoaConfig* cfg;
    bool wasusage;
    bool dumpbccode;
    char* source;
    const char* filename;
    const char* bytecodefile;
    BoaStatusCode result;
};

bool on_flag(optcontext_t* ox, optflag_t* flag, void* userptr)
{
    int co;
    FILE* tmpfh;
    BoaCliOptions* cli;
    cli = (BoaCliOptions*)userptr;
    co = flag->shortname;
    if(co == '?')
    {
        fprintf(stderr, "%s: %s\n", ox->argv[0], ox->errmsg);
        cli->result = BOA_STATUS_RUNTIMEERROR;
        return false;
    }
    else if(co == 'h')
    {
        boa_cli_showusage(ox->argv, ox, false);
        cli->wasusage = true;
        return false;
    }
    else if(co == 'a')
    {
        cli->cfg->dumpast = true;
    }
    else if(co == 'd')
    {
        cli->dumpbccode = true;
    }
    else if(co == 'o')
    {
        cli->bytecodefile = ox->optarg;
    }
    else if(co == 'e')
    {
        cli->source = ox->optarg;
    }
    else if(co == 'q')
    {
        cli->cfg->quitafterdump = true;
    }
    else if(co == 't')
    {
        cli->cfg->traceexecution = true;
    }
    else if(co == 'i')
    {
        cli->cfg->traceinstsonly = true;
    }
    else if(co == 'm')
    {
        if(boa_util_strcaseequal(ox->optarg, "default"))
        {
        }
        if(boa_util_strcaseequal(ox->optarg, "none"))
        {
            cli->cfg->mempooldisable = true;
        }
        else if(boa_util_strcaseequal(ox->optarg, "generic"))
        {
            cli->cfg->mempoolforcegeneric = true;
        }
        else
        {
            fprintf(stderr, "unrecognized mode '%s' for '-m'\n", ox->optarg);
            return false;
        }
    }
    else if(co == 'T')
    {
        tmpfh = fopen(ox->optarg, "wb");
        if(tmpfh == NULL)
        {
            fprintf(stderr, "cannot open trace destination file '%s' for writing\n", ox->optarg);
            return false;
        }
        cli->cfg->desttrace = boa_stream_makeio(tmpfh, true);
        cli->cfg->havedesttrace = true;
    }
    return true;
}

int main(int argc, char* argv[], char** envp)
{
    int i;
    char *arg;
    const char* filename;
    const char* climdname;
    BoaState statestack;
    BoaConfig config;
    optcontext_t options;
    #if defined(BOA_CONFIG_USELINO) && (BOA_CONFIG_USELINO == 1)
    linocontext_t lictx;
    #endif
    BoaCliOptions cli;
    BoaState* state;
    BoaArray* argarray;
    BoaModule* module;
    state = &statestack;
    memset(&config, 0, sizeof(BoaConfig));    
    memset(&cli, 0, sizeof(BoaCliOptions));
    config.mempooldisable = false;
    config.dumpast = false;
    config.traceexecution = false;
    config.traceinstsonly = false;
    config.isreplmode = false;
    config.havedesttrace = false;
    config.quitafterdump = false;
    #if defined(BOA_OSPLATFORM_ISWINNT) || defined(_MSC_VER)
        _setmode(fileno(stdin), _O_BINARY);
        _setmode(fileno(stdout), _O_BINARY);
        _setmode(fileno(stderr), _O_BINARY);
    #endif
    cli.result = BOA_STATUS_OK;
    cli.source = NULL;
    cli.cfg = &config;
    cli.dumpbccode = false;
    cli.bytecodefile = NULL;
    optprs_init(&options, argc, argv, &cli, on_flag);
    options.permute = 0;
    optprs_add(&options, "help", 'h', OPTPARSE_NONE, "this help");
    optprs_add(&options, "memory", 'm', OPTPARSE_REQUIRED, "specify type of memory handling (default, generic, none)");
    optprs_add(&options, "compileto", 'o', OPTPARSE_REQUIRED, "compile input script file to <val>");
    optprs_add(&options, "dump", 'd', OPTPARSE_NONE, "dump instructions");
    optprs_add(&options, "ast", 'a', OPTPARSE_NONE, "dump AST");
    optprs_add(&options, "eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code");
    optprs_add(&options, "trace", 't', OPTPARSE_NONE, "trace execution");
    optprs_add(&options, "instsonly", 'i', OPTPARSE_NONE, "when '-t' is specified, trace instructions only, skipping printing values");
    optprs_add(&options, "dest", 'T', OPTPARSE_REQUIRED, "when '-t' is specified, write trace output to file. defaults to stderr");
    optprs_add(&options, "quit", 'q', OPTPARSE_NONE, "when dumping flags (like '-a' or '-d') are specified, quit immediately after");
    if(!optprs_run(&options))
    {
        goto endmain;
    }
    if(cli.wasusage)
    {
        goto endmain;
    }
    boa_state_make(&statestack, &config);

    boa_cli_parseenv(state, envp);
    if(cli.bytecodefile != NULL)
    {
        if(!boa_state_compileandsavefile(state, options.restargv[0], cli.bytecodefile))
        {
            cli.result = BOA_STATUS_COMPILEERROR;
        }
        goto endmain;
    }
    {
        argarray = boa_array_make(state);
        boa_state_setglobal(state, boa_string_copy(state, "ARGV"), boa_value_fromobject(argarray));
        for(i = 0; i < (int)options.restargc; i++)
        {
            arg = options.restargv[i];
            boa_array_push(argarray, boa_value_fromobject(boa_string_copy(state, arg)));
        }
    }
    if(cli.source != NULL)
    {
        climdname = "<-e>";
        if(cli.dumpbccode)
        {
            module = boa_state_compilemodulesource(state, boa_string_copy(state, climdname), cli.source);
            if(module == NULL)
            {
                goto endmain;
            }
            boa_debug_disasmodule(state->streamstdout, module, cli.source);
            if(state->config.quitafterdump)
            {
                goto endmain;
            }
        }
        else
        {
            cli.result = boa_state_interpretsource(state, climdname, cli.source).type;
            if(cli.result != BOA_STATUS_OK)
            {
                goto endmain;
            }
        }
    }
    else if(options.restargc > 0)
    {
        filename = options.restargv[0];
        if(cli.dumpbccode)
        {
            cli.result = boa_state_dumpfile(state, state->streamstdout, filename).type;
            if(state->config.quitafterdump)
            {
                goto endmain;
            }
        }
        else
        {
            cli.result = boa_state_interpretfile(state, filename).type;
        }
        if(cli.result != BOA_STATUS_OK)
        {
            goto endmain;
        }
    }
    else
    {
        state->config.isreplmode = true;
        #if defined(BOA_CONFIG_USELINO) && (BOA_CONFIG_USELINO == 1)
            lino_context_init(&lictx);
            boa_repl_runrepl(state, &lictx);
        #else
            fprintf(stderr, "no REPL compiled in! nothing to do.\n");
        #endif
    }
    endmain:
    boa_state_destroy(state);
    boa_sysmem_pooldestroy();
    if(cli.result != BOA_STATUS_OK)
    {
        return 1;
    }
    return 0;
}

