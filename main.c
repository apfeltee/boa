
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>
#include <ctype.h>
#include <sys/stat.h>
#include <memory.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <io.h>
#endif

#include "allocator.h"
#include "optparse.h"
#include "oslib.h"
#include "lino.h"

#if !defined(_WIN32) && !defined(__CYGWIN__)
    #include <dirent.h>
#endif

#if !defined(LIT_INLINE)
    #if defined(__GNUC__) || defined(__TINYC__)
        #define LIT_INLINE __attribute__((always_inline)) inline
    #else
        #define LIT_INLINE inline
    #endif
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define LIT_LIKELY(x) (__builtin_expect(!!(x), 1))
    #define LIT_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
    #define LIT_LIKELY(x) (x)
    #define LIT_UNLIKELY(x) (x)
#endif

#if defined(__ANDROID__) || defined(_ANDROID_)
#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
    #define LIT_PLATFORM_WINDOWS
#endif

#if !defined(va_copy)
    #if defined(__GNUC__) || defined(__CLANG__)
        #define va_copy(d, s) __builtin_va_copy(d, s)
    #else
        #define va_copy(dest, src) memcpy(dest, src, sizeof(va_list))
    #endif
#endif

#if defined(__STRICT_ANSI__)
void* memccpy(void* dest, const void* src, int c, size_t n);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
#endif


#define LIT_BYTECODE_VERSION 0

#if !defined(M_PI)
    #define M_PI 3.14
#endif

#ifndef TESTING
    #ifndef RELEASE
        #define DEBUG
    #endif
#endif

#ifdef DEBUG
    #undef LIT_TRACE_EXECUTION
    #undef LIT_TRACE_CHUNK
    #undef LIT_TRACE_NULL_FILL
    #undef LIT_CONFIG_LOGGC
    #undef LIT_CONFIG_LOGALLOCATION
    #undef LIT_CONFIG_LOGMARKING
    #undef LIT_CONFIG_LOGBLACKING
    #undef LIT_CONFIG_STRESSTESTGC
#endif

#ifdef TESTING
    /* make sure that we did not break anything */
    #define LIT_CONFIG_STRESSTESTGC
#else
#endif

#define LIT_INTERPOLATION_NESTING_MAX 4

#define LIT_GC_HEAP_GROW_FACTOR 2
#define LIT_INITIAL_CALL_FRAMES 1024

#define STRBUF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define STRBUF_MAX(x, y) ((x) > (y) ? (x) : (y))


#if !defined(LIT_DISABLE_COLOR) && !defined(LIT_ENABLE_COLOR) && !(defined(LIT_PLATFORM_WINDOWS))
    #define LIT_ENABLE_COLOR
#endif

#ifdef LIT_ENABLE_COLOR
    #define COLOR_RESET "\x1B[0m"
    #define COLOR_RED "\x1B[31m"
    #define COLOR_GREEN "\x1B[32m"
    #define COLOR_YELLOW "\x1B[33m"
    #define COLOR_BLUE "\x1B[34m"
    #define COLOR_MAGENTA "\x1B[35m"
    #define COLOR_CYAN "\x1B[36m"
#else
    #define COLOR_RESET ""
    #define COLOR_RED ""
    #define COLOR_GREEN ""
    #define COLOR_YELLOW ""
    #define COLOR_BLUE ""
    #define COLOR_MAGENTA ""
    #define COLOR_CYAN ""
#endif

#define UNREACHABLE                                                                 \
    fprintf(stderr, "unreachable code was reached at %s:%i\n", __FILE__, __LINE__); \
    assert(false);
#define LIT_CONFIG_UINT8COUNT UINT8_MAX + 1
#define LIT_CONFIG_UINT16COUNT UINT16_MAX + 1

#define RETURN_RUNTIME_ERROR() return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());
#define LIT_STATUS_RUNTIME_FAIL (lit_result_make(LIT_STATUS_INVALID, lit_value_makenull()))

#define RETURN_OK(r) return lit_result_make(LIT_STATUS_OK, r);

#define LIT_CHECK_NUMBER(id) lit_args_checknumber(state, __FUNCTION__, args, argc, id)

#define LIT_CHECK_GETSTRINGDATA(id) lit_args_checkstring(state, args, argc, id)
#define LIT_CHECK_GETSTRINGDATAOR(id, def) lit_args_getstring(args, argc, id, def)

#define LIT_CHECK_GETSTRINGOBJECT(id) lit_args_checkobjstring(state, args, argc, id)

#define LIT_ENSURE_ARGS(count)                                                      \
    if(argc != count)                                                               \
    {                                                                               \
        lit_vm_raisefatalerror(state, "expected %ld arguments, got %ld", (size_t)count, (size_t)argc); \
        return lit_value_makenull();                                                \
    }

/* do not change these, or old bytecode files will break! */
#define LIT_CONFIG_BCMAGICNUMBER 6932
#define LIT_CONFIG_BCENDNUMBER 2942
#define LIT_CONFIG_BCSTRINGKEY 48

#define lit_set_native_exit_jump() setjmp(g_vmglobaljumpbuf)


#define LIT_CONFIG_TABLEMAXLOAD 0.75

#define LIT_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define LIT_GC_FREEOBJECT(state, type, pointer) lit_reallocate(state, pointer, sizeof(type), 0)

#define LIT_BIT_SETBIT(number, n) number |= 1UL << n
#define LIT_BIT_ISSET(number, n) (((number >> n) & 1U) != 0)

#define lit_vmexec_pushgc(state, allow) \
    state->gcwasallowed = state->gcallowgc;  \
    state->gcallowgc = allow;

#define lit_vmexec_popgc(state) \
    state->gcallowgc = state->gcwasallowed;

#define LIT_CONFIG_LONGESTOPNAME 13

/* can't be over 255 */
#define LIT_CONFIG_REGISTERSMAX 255

#define LIT_CONFIG_OPCODESIZE 0x3f
#define LIT_CONFIG_ARGSIZEA 0xff
#define LIT_CONFIG_ARGSIZEB 0x1ff
#define LIT_CONFIG_ARGSIZEC 0x1ff
/* 18 bits max */
#define LIT_CONFIG_ARGSIZEBX 0x3ffff
/* 17 bits max */
#define LIT_CONFIG_ARGSIZESBX 0x1ffff

#define LIT_CONFIG_ARGPOSA 6
#define LIT_CONFIG_ARGPOSB 14
#define LIT_CONFIG_ARGPOSC 23
#define LIT_CONFIG_ARGPOSBX 14
#define LIT_CONFIG_ARGPOSSBX 15
#define LIT_CONFIG_FLAGPOSSBX 14

/*
 * Instruction can follow one of the three formats:
 *
 * ABC  opcode:6 bits (starting from bit 0), A:8 bits, B:9 bits, C:9 bits
 * ABx  opcode:6 bits (starting from bit 0), A:8 bits, Bx:18 bits
 * AsBx opcode:6 bits (starting from bit 0), A:8 bits, sBx:18 bits (signed)
 */

#define LIT_INST_GETOPCODE(instruction) (instruction & LIT_CONFIG_OPCODESIZE)
#define LIT_INST_GETA(instruction) (((int64_t)(instruction >> LIT_CONFIG_ARGPOSA)) & LIT_CONFIG_ARGSIZEA)
#define LIT_INST_GETB(instruction) (((int64_t)(instruction >> LIT_CONFIG_ARGPOSB)) & LIT_CONFIG_ARGSIZEB)
#define LIT_INST_GETC(instruction) (((int64_t)(instruction >> LIT_CONFIG_ARGPOSC)) & LIT_CONFIG_ARGSIZEC)
#define LIT_INST_GETBX(instruction) (((int64_t)(instruction >> LIT_CONFIG_ARGPOSBX)) & LIT_CONFIG_ARGSIZEBX)
#define LIT_INST_GETSBX(instruction) ((((int64_t)(instruction >> LIT_CONFIG_ARGPOSSBX)) & LIT_CONFIG_ARGSIZESBX) * (((((int64_t)instruction) >> LIT_CONFIG_FLAGPOSSBX) & 0x1) == 1 ? -1 : 1))

#define LIT_REG_FORMABCINST(opcode, a, b, c) \
    ( \
        ((opcode) & LIT_CONFIG_OPCODESIZE) | \
        ((((int64_t)(a)) & LIT_CONFIG_ARGSIZEA) << LIT_CONFIG_ARGPOSA) | \
        ((((int64_t)(b)) & LIT_CONFIG_ARGSIZEB) << LIT_CONFIG_ARGPOSB) | \
        ((((int64_t)(c)) & LIT_CONFIG_ARGSIZEC) << LIT_CONFIG_ARGPOSC) \
    )

#define LIT_REG_FORMABXINST(opcode, a, bx) \
    ( \
        ((opcode) & LIT_CONFIG_OPCODESIZE) | \
        ((((int64_t)(a)) & LIT_CONFIG_ARGSIZEA) << LIT_CONFIG_ARGPOSA) | \
        ((((int64_t)(bx)) & LIT_CONFIG_ARGSIZEBX) << LIT_CONFIG_ARGPOSBX) \
    )

#define LIT_REG_FORMASBXINST(opcode, a, sbx) \
    ( \
        ((opcode) & LIT_CONFIG_OPCODESIZE) | \
        (((a) & LIT_CONFIG_ARGSIZEA) << LIT_CONFIG_ARGPOSA) | \
        ((labs((int64_t)(sbx)) & LIT_CONFIG_ARGSIZESBX) << LIT_CONFIG_ARGPOSSBX)) | \
        ((((((int64_t)(sbx)) < 0) ? 1 : 0) << LIT_CONFIG_FLAGPOSSBX) \
    )

enum LitBit
{
    LIT_BITFLAG_CONSTANT = 8,
    LIT_BITFLAG_REGISTER = 9,
    LIT_BITFLAG_VMCONST = 16,
};

enum LitObjType
{
    LIT_OBJ_STRING,
    LIT_OBJ_FUNCSCRIPT,
    LIT_OBJ_FUNCNATIVE,
    LIT_OBJ_FUNCNATMETHOD,
    LIT_OBJ_FIBER,
    LIT_OBJ_MODULE,
    LIT_OBJ_FUNCCLOSURE,
    LIT_OBJ_CLSPROTOTYPE,
    LIT_OBJ_UPVALUE,
    LIT_OBJ_CLASS,
    LIT_OBJ_INSTANCE,
    LIT_OBJ_FUNCBOUNDMETHOD,
    LIT_OBJ_ARRAY,
    LIT_OBJ_VARARGARRAY,
    LIT_OBJ_MAP,
    LIT_OBJ_USERDATA,
    LIT_OBJ_RANGE,
    LIT_OBJ_FIELD,
    LIT_OBJ_REFERENCE
};

enum LitValType
{
    LIT_VALTYP_NULL,
    LIT_VALTYP_BOOL,
    LIT_VALTYP_NUMBER,
    LIT_VALTYP_OBJECT,
};

enum LitFuncType
{
    LIT_FUNCTYPE_REGULAR,
    LIT_FUNCTYPE_SCRIPT,
    LIT_FUNCTYPE_METHOD,
    LIT_FUNCTYPE_STATIC_METHOD,
    LIT_FUNCTYPE_CONSTRUCTOR
};

enum LitErrorType
{
    LIT_ERROR_COMPILEERROR,
    LIT_ERROR_RUNTIMEERROR
};

enum LitStatusCode
{
    LIT_STATUS_OK,
    LIT_STATUS_COMPILEERROR,
    LIT_STATUS_RUNTIMEERROR,
    LIT_STATUS_INVALID
};

enum LitAstTokType
{
    LIT_ASTTOKTYP_LINEFEED,

    /* single-character tokens */
    LIT_ASTTOKTYP_LEFTPAREN,
    LIT_ASTTOKTYP_RIGHTPAREN,
    LIT_ASTTOKTYP_LEFTBRACE,
    LIT_ASTTOKTYP_RIGHTBRACE,
    LIT_ASTTOKTYP_LEFTBRACKET,
    LIT_ASTTOKTYP_RIGHTBRACKET,
    LIT_ASTTOKTYP_COMMA,
    LIT_ASTTOKTYP_SEMICOLON,
    LIT_ASTTOKTYP_COLON,

    /* one or two character tokens */
    LIT_ASTTOKTYP_BAREQUAL,
    LIT_ASTTOKTYP_BAR,
    LIT_ASTTOKTYP_BARBAR,
    LIT_ASTTOKTYP_AMPERSANDEQUAL,
    LIT_ASTTOKTYP_AMPERSAND,
    LIT_ASTTOKTYP_AMPERSANDAMPERSAND,
    LIT_ASTTOKTYP_BANG,
    LIT_ASTTOKTYP_BANGEQUAL,
    LIT_ASTTOKTYP_EQUAL,
    LIT_ASTTOKTYP_EQUALEQUAL,
    LIT_ASTTOKTYP_GREATER,
    LIT_ASTTOKTYP_GREATEREQUAL,
    LIT_ASTTOKTYP_GREATERGREATER,
    LIT_ASTTOKTYP_LESS,
    LIT_ASTTOKTYP_LESSEQUAL,
    LIT_ASTTOKTYP_LESSLESS,
    LIT_ASTTOKTYP_PLUS,
    LIT_ASTTOKTYP_PLUSEQUAL,
    LIT_ASTTOKTYP_PLUSPLUS,
    LIT_ASTTOKTYP_MINUS,
    LIT_ASTTOKTYP_MINUSEQUAL,
    LIT_ASTTOKTYP_MINUSMINUS,
    LIT_ASTTOKTYP_STAR,
    LIT_ASTTOKTYP_STAREQUAL,
    LIT_ASTTOKTYP_STARSTAR,
    LIT_ASTTOKTYP_SLASH,
    LIT_ASTTOKTYP_SLASHEQUAL,
    LIT_ASTTOKTYP_QUESTION,
    LIT_ASTTOKTYP_QUESTIONQUESTION,
    LIT_ASTTOKTYP_PERCENT,
    LIT_ASTTOKTYP_PERCENTEQUAL,
    LIT_ASTTOKTYP_ARROW,
    LIT_ASTTOKTYP_SMALLARROW,
    LIT_ASTTOKTYP_TILDE,
    LIT_ASTTOKTYP_REFSYM,
    LIT_ASTTOKTYP_CARET,
    LIT_ASTTOKTYP_CARETEQUAL,
    LIT_ASTTOKTYP_DOT,
    LIT_ASTTOKTYP_DOTDOT,
    LIT_ASTTOKTYP_DOTDOTDOT,
    LIT_ASTTOKTYP_SHARP,
    LIT_ASTTOKTYP_SHARPEQUAL,

    /* literals */
    LIT_ASTTOKTYP_IDENTIFIER,
    LIT_ASTTOKTYP_STRING,
    LIT_ASTTOKTYP_STRTEMPLATE,
    LIT_ASTTOKTYP_NUMBER,

    /* keywords */
    LIT_ASTTOKTYP_KWCLASS,
    LIT_ASTTOKTYP_KWELSE,
    LIT_ASTTOKTYP_KWFALSE,
    LIT_ASTTOKTYP_KWFOR,
    LIT_ASTTOKTYP_KWFUNCTION,
    LIT_ASTTOKTYP_KWIF,
    LIT_ASTTOKTYP_KWNULL,
    LIT_ASTTOKTYP_KWRETURN,
    LIT_ASTTOKTYP_KWSUPER,
    LIT_ASTTOKTYP_KWTHIS,
    LIT_ASTTOKTYP_KWTRUE,
    LIT_ASTTOKTYP_KWVAR,
    LIT_ASTTOKTYP_KWWHILE,
    LIT_ASTTOKTYP_KWCONTINUE,
    LIT_ASTTOKTYP_KWBREAK,
    LIT_ASTTOKTYP_KWNEW,
    LIT_ASTTOKTYP_KWEXPORT,
    LIT_ASTTOKTYP_KWIS,
    LIT_ASTTOKTYP_KWSTATIC,
    LIT_ASTTOKTYP_KWOPERATOR,
    LIT_ASTTOKTYP_KWIN,
    LIT_ASTTOKTYP_KWCONST,
    LIT_ASTTOKTYP_KWREF,

    LIT_ASTTOKTYP_KWTRY,
    LIT_ASTTOKTYP_KWCATCH,
    LIT_ASTTOKTYP_KWFINALLY,
    LIT_ASTTOKTYP_KWTHROW,

    LIT_ASTTOKTYP_ERROR,
    LIT_ASTTOKTYP_EOF
};

enum LitPrecedence
{
    LIT_ASTPREC_NONE,
    LIT_ASTPREC_ASSIGNMENT, /* = */
    LIT_ASTPREC_OR, /* || */
    LIT_ASTPREC_AND, /* && */
    LIT_ASTPREC_NULL, /* ?? */
    LIT_ASTPREC_BOR, /* | */
    LIT_ASTPREC_BXOR, /* ^ */
    LIT_ASTPREC_BAND, /* & */
    LIT_ASTPREC_EQUALITY, /* == != */
    LIT_ASTPREC_IS, /* is */
    LIT_ASTPREC_COMPARISON, /* < > <= >= */
    LIT_ASTPREC_RANGE, /* .. */
    LIT_ASTPREC_SHIFT, /* << >> */
    LIT_ASTPREC_TERM, /* + - */
    LIT_ASTPREC_FACTOR, /* * / % # */
    LIT_ASTPREC_COMPOUND, /* += -= *= /= ++ -- */
    LIT_ASTPREC_UNARY, /* ! - ~ */
    LIT_ASTPREC_CALL, /* . () [] */
    LIT_ASTPREC_PRIMARY
};

enum LitAstExprType
{
    LIT_ASTEXPRTYP_LITERAL,
    LIT_ASTEXPRTYP_BINARY,
    LIT_ASTEXPRTYP_UNARY,
    LIT_ASTEXPRTYP_VARGET,
    LIT_ASTEXPRTYP_ASSIGN,
    LIT_ASTEXPRTYP_CALL,
    LIT_ASTEXPRTYP_INDEXSET,
    LIT_ASTEXPRTYP_INDEXGET,
    LIT_ASTEXPRTYP_FUNCANON,
    LIT_ASTEXPRTYP_ARRAY,
    LIT_ASTEXPRTYP_OBJECT,
    LIT_ASTEXPRTYP_SUBSCRIPT,
    LIT_ASTEXPRTYP_THIS,
    LIT_ASTEXPRTYP_SUPER,
    LIT_ASTEXPRTYP_RANGE,
    LIT_ASTEXPRTYP_TERNARY,
    LIT_ASTEXPRTYP_INTERPOLATION,
    LIT_ASTEXPRTYP_REFERENCE,
    LIT_ASTEXPRTYP_EXPRESSION,
    LIT_ASTEXPRTYP_BLOCK,
    LIT_ASTEXPRTYP_IF,
    LIT_ASTEXPRTYP_WHILE,
    LIT_ASTEXPRTYP_FOR,
    LIT_ASTEXPRTYP_VARDECL,
    LIT_ASTEXPRTYP_CONTINUE,
    LIT_ASTEXPRTYP_BREAK,
    LIT_ASTEXPRTYP_FUNCTION,
    LIT_ASTEXPRTYP_RETURN,
    LIT_ASTEXPRTYP_METHOD,
    LIT_ASTEXPRTYP_CLASS,
    LIT_ASTEXPRTYP_FIELD,
    LIT_ASTEXPRTYP_TRY,
    LIT_ASTEXPRTYP_THROW
};

enum LitInstrucType
{
    LIT_INSTYP_ABC,
    LIT_INSTYP_ABX,
    LIT_INSTYP_ASBX,
};

enum LitOpCode
{
    LIT_OPCODE_MOVE, /* R(A) := RC(B) */
    LIT_OPCODE_LOADNULL, /* R(A) := null */
    LIT_OPCODE_LOADBOOL, /* R(A) := (bool) B */
    LIT_OPCODE_MAKECLOSURE, /* R(A) := PrC[Bx] */
    LIT_OPCODE_MAKEARRAY, /* R(A) := new Array(Bx) */
    LIT_OPCODE_MAKEOBJECT, /* R(A) = new Object() */
    LIT_OPCODE_MAKERANGE, /* R(A) = new Range(RC(B), RC(C)) */
    LIT_OPCODE_RETURN, /* return R(A) */
    LIT_OPCODE_MATHADD, /* R(A) := RC(B) + RC(C) */
    LIT_OPCODE_MATHSUBTRACT, /* R(A) := RC(B) - RC(C) */
    LIT_OPCODE_MATHMULTIPLY, /* R(A) := RC(B) * RC(C) */
    LIT_OPCODE_MATHDIVIDE, /* R(A) := RC(B) / RC(C) */
    LIT_OPCODE_MATHFLOORDIVIDE, /* R(A) := floor(RC(B) / RC(C)) */
    LIT_OPCODE_MATHMOD, /* R(A) := RC(B) % RC(C) */
    LIT_OPCODE_MATHPOWER, /* R(A) := pow(RC(B), RC(C)) */
    LIT_OPCODE_MATHLEFTSHIFT, /* R(A) := RC(B) << RC(C) */
    LIT_OPCODE_MATHRIGHTSHIFT, /* R(A) := RC(B) >> RC(C) */
    LIT_OPCODE_BINXOR, /* R(A) := RC(B) ^ RC(C) */
    LIT_OPCODE_BINAND, /* R(A) := RC(B) & RC(C) */
    LIT_OPCODE_BINOR, /* R(A) := RC(B) | RC(C) */
    LIT_OPCODE_JUMP, /* PC += sBx */
    LIT_OPCODE_JUMPIFTRUE, /* if (R(A)) PC += Bx */
    LIT_OPCODE_JUMPIFFALSE, /* if (not R(A)) PC += Bx */
    LIT_OPCODE_JUMPIFNONNULL, /* if (R(A) != null) PC += Bx */
    LIT_OPCODE_JUMPIFNULL, /* if (R(A) == null) PC += Bx */
    LIT_OPCODE_EQUAL, /* R(A) := RC(B) == RC(C) */
    LIT_OPCODE_LESSTHAN, /* R(A) := RC(B) < RC(C) */
    LIT_OPCODE_LESSEQUAL, /* R(A) := RC(B) <= RC(C) */
    LIT_OPCODE_GREATERTHAN, /* R(A) := RC(B) > RC(C) */
    LIT_OPCODE_GREATEREQUAL, /* R(A) := RC(B) >= RC(C) */
    LIT_OPCODE_NEGATE, /* R(A) := -RC(B) */
    LIT_OPCODE_NOT, /* R(A) := !RC(B) */
    LIT_OPCODE_BINNOT, /* R(A) := ~RC(B) */
    LIT_OPCODE_GLOBALSET, /* G[C(A)] := RC(BX) */
    LIT_OPCODE_GLOBALGET, /* R(A) := G[C(Bx)] */
    LIT_OPCODE_UPVALUESET, /* U[A] := RC(Bx) */
    LIT_OPCODE_UPVALUEGET, /* R(A) := U[Bx] */
    LIT_OPCODE_PRIVATESET, /* P[A] := RC(Bx) */
    LIT_OPCODE_PRIVATEGET, /* R(A) := P[C(Bx)] */
    LIT_OPCODE_CALLCALLABLE, /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
    LIT_OPCODE_UPVALUECLOSE, /* close_upvalue(R(A)) */
    LIT_OPCODE_CLASSMAKE, /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
    LIT_OPCODE_CLASSPUTFIELDSTATIC, /* R(A)[C(B)] = RC(C) */
    LIT_OPCODE_CLASSPUTMETHOD, /* R(A).Methods[C(B)] = RC(C) */
    LIT_OPCODE_FIELDGET, /* R(A) = R(B)[C(C)] */
    LIT_OPCODE_CLASSGETSUPERMETHOD, /* R(A) = R(B).super[C(C)] */
    LIT_OPCODE_FIELDSET, /* R(A)[C(B)] = R(C) */
    LIT_OPCODE_IS, /* R(A) := RC(B) is G[C(C)] */
    LIT_OPCODE_INVOKE, /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
    LIT_OPCODE_INVOKESUPER, /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
    LIT_OPCODE_SUBSCRIPTGET, /* R(A) := R(A)[RC(B)] */
    LIT_OPCODE_SUBSCRIPTSET, /* R(A)[RC(B)] := R(C) */
    LIT_OPCODE_ARRAYPUSH, /* R(A)[R(A).listcount++] = RC(Bx) */
    LIT_OPCODE_OBJECTPUSH, /* R(A)[R(B)] = RC(C) */
    LIT_OPCODE_REFGLOBAL, /* R(A) := ref G(C[Bx]) */
    LIT_OPCODE_REFPRIVATE, /* R(A) := ref P(Bx) */
    LIT_OPCODE_REFLOCAL, /* R(A) := ref R(B) */
    LIT_OPCODE_REFUPVALUE, /* R(A) := ref U(Bx) */
    LIT_OPCODE_REFFIELD, /* R(A) = ref R(B)[C(C)] */
    LIT_OPCODE_REFSET, /* ref R(A) := R(B) */
    LIT_OPCODE_PUSH_TRY, /* push try handler at PC + Bx */
    LIT_OPCODE_POP_TRY, /* pop try handler */
    LIT_OPCODE_THROW, /* throw R(A) */
    LIT_OPCODE_RETHROW, /* rethrow fiber->error */
};

enum LitIOStrMode
{
    LIT_IOSTRMODE_UNDEFINED,
    LIT_IOSTRMODE_STRING,
    LIT_IOSTRMODE_FILE
};

typedef enum LitIOStrMode LitIOStrMode;
typedef enum /**/ LitObjType LitObjType;
typedef enum /**/ LitFuncType LitFuncType;
typedef enum /**/ LitErrorType LitErrorType;
typedef enum /**/ LitStatusCode LitStatusCode;
typedef enum /**/ LitAstTokType LitAstTokType;
typedef enum /**/ LitPrecedence LitPrecedence;
typedef enum LitValType LitValType;
typedef enum LitAstExprType LitAstExprType;
typedef enum LitInstrucType LitInstrucType;
typedef enum LitOpCode LitOpCode;

typedef struct /**/ LitAstLexer LitAstLexer;
typedef struct /**/ LitState LitState;
typedef struct /**/ LitAstParser LitAstParser;
typedef struct /**/ LitAstEmitter LitAstEmitter;
typedef struct /**/ LitState LitState;
typedef struct /**/ LitResult LitResult;
typedef struct /**/ LitObject LitObject;
typedef struct /**/ LitMap LitMap;
typedef struct /**/ LitString LitString;
typedef struct /**/ LitModule LitModule;
typedef struct /**/ LitFiber LitFiber;
typedef struct /**/ LitUserdata LitUserdata;
typedef struct /**/ LitAstExpression LitAstExpression;
typedef struct /**/ LitUpvalue LitUpvalue;
typedef struct /**/ LitClass LitClass;
typedef struct /**/ LitAstLiteralValExpr LitAstLiteralValExpr;
typedef struct /**/ LitAstBinaryExpr LitAstBinaryExpr;
typedef struct /**/ LitAstUnaryExpr LitAstUnaryExpr;
typedef struct /**/ LitAstVarGetExpr LitAstVarGetExpr;
typedef struct /**/ LitAstAssignExpr LitAstAssignExpr;
typedef struct /**/ LitAstCallExpr LitAstCallExpr;
typedef struct /**/ LitAstIndexGetExpr LitAstIndexGetExpr;
typedef struct /**/ LitAstIndexSetExpr LitAstIndexSetExpr;
typedef struct /**/ LitAstFuncParamExpr LitAstFuncParamExpr;
typedef struct /**/ LitDynListParam LitDynListParam;
typedef struct /**/ LitAstLiteralArrayExpr LitAstLiteralArrayExpr;
typedef struct /**/ LitAstLiteralObjectExpr LitAstLiteralObjectExpr;
typedef struct /**/ LitAstSubscriptExpr LitAstSubscriptExpr;
typedef struct /**/ LitAstThisExpr LitAstThisExpr;
typedef struct /**/ LitAstSuperExpr LitAstSuperExpr;
typedef struct /**/ LitAstRangeExpr LitAstRangeExpr;
typedef struct /**/ LitAstTernaryExpr LitAstTernaryExpr;
typedef struct /**/ LitAstStrTemplateExpr LitAstStrTemplateExpr;
typedef struct /**/ LitAstRefExpr LitAstRefExpr;
typedef struct /**/ LitAstExprStmtExpr LitAstExprStmtExpr;
typedef struct /**/ LitAstBlockExpr LitAstBlockExpr;
typedef struct /**/ LitAstVarDeclExpr LitAstVarDeclExpr;
typedef struct /**/ LitAstIfExpr LitAstIfExpr;
typedef struct /**/ LitAstWhileExpr LitAstWhileExpr;
typedef struct /**/ LitAstForExpr LitAstForExpr;
typedef struct /**/ LitAstContinueExpr LitAstContinueExpr;
typedef struct /**/ LitBreakStatement LitBreakStatement;
typedef struct /**/ LitAstFunctionExpr LitAstFunctionExpr;
typedef struct /**/ LitAstReturnExpr LitAstReturnExpr;
typedef struct /**/ LitAstMethodExpr LitAstMethodExpr;
typedef struct /**/ LitAstClassExpr LitAstClassExpr;
typedef struct /**/ LitAstFieldExpr LitAstFieldExpr;
typedef struct /**/ LitAstTryExpr LitAstTryExpr;
typedef struct /**/ LitAstThrowExpr LitAstThrowExpr;
typedef struct /**/ LitAstPrivate LitAstPrivate;
typedef struct /**/ LitDynListPriv LitDynListPriv;
typedef struct /**/ LitAstLocal LitAstLocal;
typedef struct /**/ LitDynListLoc LitDynListLoc;
typedef struct /**/ LitAstUpvalue LitAstUpvalue;
typedef struct /**/ LitAstCompiler LitAstCompiler;
typedef struct /**/ LitAstEmitter LitAstEmitter;
typedef struct /**/ LitAstRule LitAstRule;
typedef struct /**/ LitAstParser LitAstParser;
typedef struct /**/ LitEmulatedFile LitEmulatedFile;
typedef struct /**/ LitAstLexer LitAstLexer;
typedef struct LitFileData LitFileData;
typedef struct LitResult LitResult;
typedef struct LitAstToken LitAstToken;
typedef struct LitDynListExpr LitDynListExpr;
typedef struct LitIOStream LitIOStream;
typedef struct LitStrBuffer LitStrBuffer;
typedef struct LitValue LitValue;
typedef struct LitVMState LitVMState;
typedef struct LitHandler LitHandler;
typedef struct LitDynListUInt LitDynListUInt;
typedef struct LitDynListByte LitDynListByte;
typedef struct LitDynListVal LitDynListVal;
typedef struct LitChunk LitChunk;
typedef struct LitTabEntry LitTabEntry;
typedef struct LitTable LitTable;
typedef struct LitObject LitObject;
typedef struct LitString LitString;
typedef struct LitFuncScript LitFuncScript;
typedef struct LitUpvalue LitUpvalue;
typedef struct LitFuncClosure LitFuncClosure;
typedef struct LitClsPrototype LitClsPrototype;
typedef struct LitFuncNative LitFuncNative;
typedef struct LitCallFrame LitCallFrame;
typedef struct LitMap LitMap;
typedef struct LitModule LitModule;
typedef struct LitFiber LitFiber;
typedef struct LitClass LitClass;
typedef struct LitInstance LitInstance;
typedef struct LitFuncBound LitFuncBound;
typedef struct LitArray LitArray;
typedef struct LitVarargArray LitVarargArray;
typedef struct LitUserdata LitUserdata;
typedef struct LitRange LitRange;
typedef struct LitField LitField;
typedef struct LitReference LitReference;
typedef struct LitState LitState;
typedef struct ListDynListPtr ListDynListPtr;
typedef struct LitAstPrinter LitAstPrinter;
typedef struct LitConstStrings LitConstStrings;
typedef struct LitConfig LitConfig;


typedef void (*LitErrorFn)(LitState* state, const char* message);
typedef void (*LitPrintFn)(LitState* state, const char* message);
typedef LitAstExpression* (*LitPrefixParseFn)(LitAstParser*, bool);
typedef LitAstExpression* (*LitInfixParseFn)(LitAstParser*, LitAstExpression*, bool);
typedef void (*LitCleanupFn)(LitState*, LitUserdata*, bool);
typedef LitValue (*LitMapIndexFn)(LitState*, LitObject*, LitString*, LitValue*);

typedef LitValue (*LitNativeFunctionFn)(LitState*, LitValue, size_t, LitValue*);

struct LitStrBuffer
{
    uint8_t isintern;
    /* capacity should be >= length+1 to allow for \0 */
    uint32_t capacity;
    uint32_t length;
    char* data;
};

struct LitIOStream
{
    /* if file: should be closed when writer is destroyed? */
    uint8_t shouldclose;
    /* if file: should write operations be flushed via fflush()? */
    uint8_t shouldflush;
    /* if string: true if $strbuf was taken via lit_iostream_take() */
    uint8_t stringtaken;
    /* was this writer instance created on stack? */
    uint8_t fromstack;
    uint8_t shortenvalues;
    uint8_t jsonmode;
    uint8_t cachedistty;
    uint8_t havecachedtty;
    size_t maxvallength;
    /* the mode that determines what writer actually does */
    LitIOStrMode wrmode;
    LitStrBuffer psbuf;
    FILE* handle;
};

struct LitAstPrinter
{
    int indent;
    LitIOStream* printer;
};

struct LitValue
{
    LitValType type;
    union
    {
        bool boolval;
        double numval;
        LitObject* obj;
    } as;
};

struct LitDynListUInt
{
    size_t listcapacity;
    size_t listcount;
    size_t* listitems;
};

struct LitDynListByte
{
    size_t listcapacity;
    size_t listcount;
    uint8_t* listitems;
};

struct LitDynListVal
{
    size_t listcapacity;
    size_t listcount;
    LitValue* listitems;
};

struct ListDynListPtr
{
    size_t listcapacity;
    size_t listcount;
    size_t itemsize;
    LitAstFuncParamExpr* listitems;    
};

struct LitDynListParam
{
    size_t listcapacity;
    size_t listcount;
    LitAstFuncParamExpr* listitems;
};

struct LitDynListExpr
{
    size_t listcapacity;
    size_t listcount;
    LitAstExpression** listitems;
};

struct LitDynListPriv
{
    size_t listcapacity;
    size_t listcount;
    LitAstPrivate* listitems;
};

struct LitDynListLoc
{
    size_t listcapacity;
    size_t listcount;
    LitAstLocal* listitems;
};

struct LitChunk
{
    uint32_t compiledcodecount;
    uint32_t capacity;
    uint64_t* compiledcodechunk;
    bool haslineinfo;
    uint32_t linecount;
    uint32_t linecapacity;
    uint16_t* lines;
    LitDynListVal constantlist;
};

struct LitTabEntry
{
    LitString* entkey;
    LitValue entvalue;
};

struct LitTable
{
    int htcount;
    int htcapacity;
    LitState* pstate;
    LitTabEntry* htentries;
};

struct LitObject
{
    LitObjType type;
    LitState* pstate;
    LitObject* next;
    bool marked;
};

struct LitString
{
    LitObject innerobject;
    uint32_t strhash;
    LitStrBuffer strbuf;
};

struct LitFuncScript
{
    LitObject innerobject;
    LitChunk chunk;
    LitString* name;
    uint32_t argcount;
    uint32_t upvaluecount;
    uint64_t maxregisters;
    bool vararg;
    LitModule* module;
};

struct LitUpvalue
{
    LitObject innerobject;
    LitValue* location;
    LitValue closed;
    LitUpvalue* next;
};

struct LitFuncClosure
{
    LitObject innerobject;
    LitFuncScript* function;
    LitUpvalue** upvalues;
    uint32_t upvaluecount;
};

struct LitClsPrototype
{
    LitObject innerobject;
    LitFuncScript* function;
    bool* local;
    uint32_t* indexes;
    uint32_t upvaluecount;
};

struct LitFuncNative
{
    LitObject innerobject;
    LitNativeFunctionFn natfuncptr;
    LitString* name;
};

struct LitCallFrame
{
    LitFuncScript* function;
    LitFuncClosure* closure;
    uint64_t* ip;
    LitValue* slots;
    LitValue* returnaddress;
    bool resultignored;
    bool returntoc;
};

struct LitMap
{
    LitObject innerobject;
    LitTable innertable;
};

struct LitModule
{
    LitObject innerobject;
    LitValue returnvalue;
    LitString* name;
    LitValue* privatevalues;
    LitMap* privatenames;
    uint32_t privatecount;
    LitFuncScript* mainfunction;
    LitFiber* mainfiber;
    bool ran;
};

struct LitHandler
{
    uint64_t* handler_ip;
    uint32_t register_count;
    uint32_t frame_count;
    uint8_t error_reg;
};

struct LitFiber
{
    LitObject innerobject;
    LitFiber* parent;
    LitValue* registeritems;
    uint32_t registersallocated;
    LitCallFrame* framevals;
    uint32_t framecapacity;
    uint32_t framecount;
    uint32_t argcount;
    LitValue* returnaddress;
    LitUpvalue* openupvalues;
    LitModule* module;
    LitValue error;
    bool abort;
    bool catcher;
    bool caught;

    LitHandler* handlers;
    uint32_t handler_count;
    uint32_t handler_capacity;
};

struct LitClass
{
    LitObject innerobject;
    LitString* name;
    LitObject* mthconstructor;
    LitTable mthtable;
    LitTable staticstable;
    LitClass* super;
};

struct LitInstance
{
    LitObject innerobject;
    LitClass* klass;
    LitTable fields;
};

struct LitFuncBound
{
    LitObject innerobject;
    LitValue receiver;
    LitValue method;
};

struct LitArray
{
    LitObject innerobject;
    LitDynListVal innerlist;
};

struct LitVarargArray
{
    LitArray innerarray;
};

struct LitUserdata
{
    LitObject innerobject;
    void* data;
    uint32_t size;
    LitCleanupFn oncleanupfn;
};

struct LitRange
{
    LitObject innerobject;
    double from;
    double to;
};

struct LitField
{
    LitObject innerobject;
    LitObject* getter;
    LitObject* setter;
};

struct LitReference
{
    LitObject innerobject;
    LitValue* slot;
};

struct LitVMState
{
    LitObject* objects;
    LitTable strings;
    LitMap* modules;
    LitMap* globals;
    LitFiber* fiber;
    /* For garbage collection */
    uint32_t gcgraycount;
    uint32_t gcgraycapacity;
    LitObject** gcgraystack;
    /* exec */
    LitCallFrame* frame;
    LitChunk* currentchunk;
    LitValue* vmregisteritems;
    LitValue* vmconstantvalues;
    LitValue* vmprivatevalues;
    LitUpvalue** upvalues;
    uint64_t* ip;
    uint64_t instruction;
};

struct LitConstStrings
{
    LitString* strthis;
    LitString* strtostring;
    LitString* strconstructor;
    LitString* strnull;
    LitString* stropequal;
    LitString* stropindex;
};

struct LitConfig
{
    bool dumpast;
    bool traceexecution;
    bool traceinstsonly;
    bool tracechunk;
    bool isreplmode;
    bool havedesttrace;
    bool quitafterdump;
    LitIOStream* desttrace;
};

struct LitState
{
    LitConfig config;

    LitVMState vmstate;

    int64_t bytes_allocated;
    int64_t gcnextgc;
    bool gcallowgc;
    bool gcwasallowed;
    LitIOStream* streamstdout;
    LitIOStream* streamstderr;
    LitErrorFn printerrmessagefn;
    LitValue* roots;
    size_t root_count;
    size_t root_capacity;
    LitAstLexer* lexer;
    LitAstParser* parser;
    LitAstEmitter* emitter;
    bool had_error;
    LitFuncScript* apifunction;
    LitString* apiname;
    /* Mental note: */
    /* When adding another class here, DO NOT forget to mark it or it will be GC-ed */
    LitClass* stdclassclass;
    LitClass* stdobjectclass;
    LitClass* number_class;
    LitClass* string_class;
    LitClass* bool_class;
    LitClass* function_class;
    LitClass* fiber_class;
    LitClass* module_class;
    LitClass* array_class;
    LitClass* map_class;
    LitClass* range_class;
    LitModule* last_module;

    LitConstStrings strings;
};

struct LitResult
{
    LitStatusCode type;
    LitValue result;
};

struct LitAstToken
{
    const char* start;
    LitAstTokType type;
    size_t length;
    size_t line;
    LitValue tokvalue;
};

/*
 * Expressions
 */

struct LitAstPrivate
{
    bool initialized;
    bool constant;
};


struct LitAstLocal
{
    const char* name;
    size_t length;
    int depth;
    bool captured;
    bool constant;
    uint32_t reg;
};


struct LitAstUpvalue
{
    uint32_t index;
    bool isLocal;
};

struct LitAstCompiler
{
    LitDynListLoc locals;
    int scope_depth;
    LitFuncScript* function;
    LitFuncType type;
    LitAstUpvalue upvalues[LIT_CONFIG_UINT8COUNT];
    uint64_t registersused;
    LitAstCompiler* enclosing;
    bool skip_return;
    size_t loop_depth;
};

struct LitAstEmitter
{
    LitState* pstate;
    LitChunk* chunk;
    LitAstCompiler* compiler;
    size_t last_line;
    size_t loop_start;
    LitDynListPriv privlist;
    LitDynListUInt breaks;
    LitDynListUInt continues;
    LitModule* module;
    LitString* class_name;
    uint32_t class_register;
    bool class_has_super;
    int emit_reference;
};

struct LitAstRule
{
    LitPrefixParseFn prefix;
    LitInfixParseFn infix;
    LitPrecedence precedence;
};

struct LitAstParser
{
    LitState* pstate;
    bool had_error;
    bool panic_mode;
    LitAstToken previous;
    LitAstToken current;
    LitAstCompiler* compiler;
    uint32_t exprrootcnt;
    uint32_t stmtrootcnt;
};

struct LitEmulatedFile
{
    const char* source;
    size_t position;
};

struct LitAstLexer
{
    size_t sourcecurrentline;
    const char* sourcedatastart;
    const char* sourcedatacurrent;
    const char* sourcefilename;
    LitState* pstate;
    size_t bracevalues[LIT_INTERPOLATION_NESTING_MAX];
    size_t bracecount;
    bool had_error;
};

struct LitAstExpression
{
    LitAstExprType type;
    size_t line;
};

struct LitAstLiteralValExpr
{
    LitAstExpression exprbase;
    LitValue value;
};

struct LitAstBinaryExpr
{
    LitAstExpression exprbase;
    LitAstExpression* left;
    LitAstExpression* right;
    LitAstTokType op;
    bool ignore_left;
};

struct LitAstUnaryExpr
{
    LitAstExpression exprbase;
    LitAstExpression* right;
    LitAstTokType op;
};

struct LitAstVarGetExpr
{
    LitAstExpression exprbase;
    const char* name;
    size_t length;
};

struct LitAstAssignExpr
{
    LitAstExpression exprbase;
    LitAstExpression* to;
    LitAstExpression* value;
};

struct LitAstCallExpr
{
    LitAstExpression exprbase;
    LitAstExpression* excallee;
    LitDynListExpr callargs;
    LitAstExpression* init;
};

struct LitAstIndexGetExpr
{
    LitAstExpression exprbase;
    LitAstExpression* where;
    const char* name;
    size_t length;
    int jump;
    bool ignore_emit;
    bool ignoreresult;
};

struct LitAstIndexSetExpr
{
    LitAstExpression exprbase;
    LitAstExpression* where;
    const char* name;
    size_t length;
    LitAstExpression* value;
};

struct LitAstFuncParamExpr
{
    const char* name;
    size_t length;
    uint32_t reg;
    LitAstExpression* defaultval;
};



struct LitAstLiteralArrayExpr
{
    LitAstExpression exprbase;
    LitDynListExpr exvalues;
};

struct LitAstLiteralObjectExpr
{
    LitAstExpression exprbase;
    LitDynListVal objexkeys;
    LitDynListExpr objexvalues;
};

struct LitAstSubscriptExpr
{
    LitAstExpression exprbase;
    LitAstExpression* array;
    LitAstExpression* index;
};

struct LitAstThisExpr
{
    LitAstExpression exprbase;
};

struct LitAstSuperExpr
{
    LitAstExpression exprbase;
    LitString* methodname;
    bool ignore_emit;
    bool ignoreresult;
};

struct LitAstRangeExpr
{
    LitAstExpression exprbase;
    LitAstExpression* from;
    LitAstExpression* to;
};

struct LitAstTernaryExpr
{
    LitAstExpression exprbase;
    LitAstExpression* condition;
    LitAstExpression* branchif;
    LitAstExpression* branchelse;
};

struct LitAstStrTemplateExpr
{
    LitAstExpression exprbase;
    LitDynListExpr expressions;
};

struct LitAstRefExpr
{
    LitAstExpression exprbase;
    LitAstExpression* to;
};

struct LitAstExprStmtExpr
{
    LitAstExpression exprbase;
    LitAstExpression* exvalue;
};

struct LitAstBlockExpr
{
    LitAstExpression exprbase;
    LitDynListExpr statements;
};

struct LitAstVarDeclExpr
{
    LitAstExpression exprbase;
    const char* name;
    size_t length;
    bool isconstant;
    LitAstExpression* init;
};

struct LitAstIfExpr
{
    LitAstExpression exprbase;
    LitAstExpression* condition;
    LitAstExpression* branchif;
    LitAstExpression* branchelse;
    LitDynListExpr* elseifcondlist;
    LitDynListExpr* branchelseiflist;
};

struct LitAstWhileExpr
{
    LitAstExpression exprbase;
    LitAstExpression* condition;
    LitAstExpression* body;
};

struct LitAstForExpr
{
    LitAstExpression exprbase;
    LitAstExpression* init;
    LitAstExpression* var;
    LitAstExpression* condition;
    LitAstExpression* increment;
    LitAstExpression* body;
    bool iscstyle;
};

struct LitAstContinueExpr
{
    LitAstExpression exprbase;
};

struct LitBreakStatement
{
    LitAstExpression exprbase;
};

struct LitAstFunctionExpr
{
    LitAstExpression exprbase;
    const char* name;
    size_t length;
    LitDynListParam parameters;
    LitAstExpression* body;
    bool exported;
};

struct LitAstReturnExpr
{
    LitAstExpression exprbase;
    LitAstExpression* exvalue;
};

struct LitAstMethodExpr
{
    LitAstExpression exprbase;
    LitString* name;
    LitDynListParam parameters;
    LitAstExpression* body;
    bool is_static;
};

struct LitAstClassExpr
{
    LitAstExpression exprbase;
    LitString* name;
    LitString* parent;
    LitDynListExpr staticfields;
};

struct LitAstFieldExpr
{
    LitAstExpression exprbase;
    LitString* name;
    LitAstExpression* getter;
    LitAstExpression* setter;
    bool is_static;
};

struct LitAstTryExpr
{
    LitAstExpression exprbase;
    LitAstExpression* try_block;
    LitAstExpression* catch_block;
    LitAstExpression* finally_block;
    const char* catch_var;
    size_t catch_var_len;
};

struct LitAstThrowExpr
{
    LitAstExpression exprbase;
    LitAstExpression* exvalue;
};

struct LitFileData
{
    char* path;
    FILE* file;
};

#include "prot.inc"

#if defined(__GNUC__)
    #define LIT_ATTRIB(...) __attribute__(__VA_ARGS__)
#else
    #define LIT_ATTRIB(...)
#endif


bool lit_vm_raisefatalerror(LitState *state, const char *format, ...) LIT_ATTRIB((format(printf, 2, 3)));

jmp_buf g_vmglobaljumpbuf;

LitResult lit_result_make(LitStatusCode type, LitValue result)
{
    LitResult r;
    r.type = type;
    r.result = result;
    return r;
}

/* Bounds check when inserting (pos <= len are valid) */
#define lit_strbuf_boundscheckinsert(sb, pos) lit_strbufutil_callboundscheckinsert(sb, pos, __FILE__, __LINE__)
#define lit_strbuf_boundscheckreadrange(sb, start, len) lit_strbufutil_callboundscheckreadrange(sb, start, len, __FILE__, __LINE__)

void* lit_sysmem_malloc(size_t sz)
{
    return malloc(sz);
}

void* lit_sysmem_realloc(void* p, size_t sz)
{
    return realloc(p, sz);
}

void* lit_sysmem_calloc(size_t count, size_t sz)
{
    return calloc(count, sz);
}

void lit_sysmem_free(void* p)
{
    free(p);
}

size_t lit_strbufutil_rndup2pow64(uint64_t x)
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
size_t lit_strbufutil_splitstr(char* str, char sep, char** ptrs, size_t nptrs)
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
size_t lit_strbufutil_charreplace(char* str, char from, char to)
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
void lit_strbufutil_reverseregion(char* str, size_t length)
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

bool lit_strbufutil_isallspace(const char* s)
{
    int i;
    for(i = 0; s[i] != '\0' && isspace((int)s[i]); i++)
    {
    }
    return (s[i] == '\0');
}

char* lit_strbufutil_nextspace(char* s)
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
char* lit_strbufutil_trim(char* str)
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
size_t lit_strbufutil_chomp(char* str, size_t len)
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
size_t lit_strbufutil_countchar(const char* str, char c)
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
size_t lit_strbufutil_split(const char* splitat, const char* sourcetxt, char*** result)
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
            arr = (char**)lit_sysmem_malloc(txtlen * sizeof(char*));
            for(i = 0; i < txtlen; i++)
            {
                arr[i] = (char*)lit_sysmem_malloc(2 * sizeof(char));
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
    arr = (char**)lit_sysmem_malloc(count * sizeof(char*));
    count = 0;
    plastpos = sourcetxt;
    while((find = strstr(plastpos, splitat)) != NULL)
    {
        slen = (size_t)(find - plastpos);
        arr[count] = (char*)lit_sysmem_malloc((slen + 1) * sizeof(char));
        strncpy(arr[count], plastpos, slen);
        arr[count][slen] = '\0';
        count++;
        plastpos = find + splitlen;
    }
    /* Copy last item */
    slen = (size_t)(sourcetxt + txtlen - plastpos);
    arr[count] = (char*)lit_sysmem_malloc((slen + 1) * sizeof(char));
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

void lit_strbufutil_callboundscheckinsert(LitStrBuffer* sb, size_t pos, const char* file, int line)
{
    if(pos > sb->length)
    {
        fprintf(stderr, "%s:%i: - out of bounds error [index: %ld, num_of_bits: %ld]\n", file, line, (long)pos, (long)sb->length);
        errno = EDOM;
        abort();
    }
}

/* Bounds check when reading a range (start+len < strlen is valid) */
void lit_strbufutil_callboundscheckreadrange(LitStrBuffer* sb, size_t start, size_t len, const char* file, int line)
{
    if(start + len > sb->length)
    {
        fprintf(stderr, "%s:%i: - out of bounds error [start: %ld; length: %ld; strlen: %ld; buf:%.*s%s]\n", file, line, (long)start, (long)len, (long)sb->length, (int)STRBUF_MIN(5, sb->length), lit_strbuf_data(sb), sb->length > 5 ? "..." : "");
        errno = EDOM;
        abort();
    }
}

/* via: codereview.stackexchange.com/q/274832 */
void lit_strbufutil_faststrncat(char* dest, const char* src, size_t* size)
{
    if(dest && src && size)
    {
        while((dest[*size] = *src++))
        {
            *size += 1;
        }
    }
}

size_t lit_strbufutil_strreplace1(char** str, size_t selflen, const char* findstr, size_t findlen, const char* substr, size_t sublen)
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
    buff = (char*)lit_sysmem_malloc((i + oldcount * (sublen - findlen) + 1) * sizeof(char));
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
            lit_strbufutil_faststrncat(&buff[i], substr, &x);
            i += sublen;
            temp += findlen;
        }
        else
        {
            buff[i++] = *temp++;
        }
    }
    lit_sysmem_free(*str);
    *str = (char*)lit_sysmem_malloc((i + 1) * sizeof(char));
    if(!(*str))
    {
        perror("bad allocation\n");
        exit(EXIT_FAILURE);
    }
    i = 0;
    lit_strbufutil_faststrncat(*str, (const char*)buff, &i);
    lit_sysmem_free(buff);
    return i;
}

size_t lit_strbufutil_strrepcount(const char* str, size_t slen, const char* findstr, size_t findlen, size_t sublen)
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
void lit_strbufutil_strreplace2(char* target, size_t tgtlen, const char* findstr, size_t findlen, const char* substr, size_t sublen)
{
    const char* p;
    const char* tmp;
    char* inspoint;
    char buffer[1024] = { 0 };
    (void)tgtlen;
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

bool lit_strbufutil_inpreplhelper(char* dest, const char* src, size_t srclen, int findme, const char* substr, size_t sublen, size_t maxlen, size_t* dlen)
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
        if(!lit_strbufutil_inpreplhelper(dest + sublen, src + 1, srclen, findme, substr, sublen, maxlen - sublen, dlen))
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
        if(!lit_strbufutil_inpreplhelper(dest + 1, src + 1, srclen, findme, substr, sublen, maxlen - 1, dlen))
        {
            return false;
        }
    }
    *dest = chatpos;
    return true;
}

size_t lit_strbufutil_inpreplace(char* target, size_t tgtlen, int findme, const char* substr, size_t sublen, size_t maxlen)
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
    lit_strbufutil_inpreplhelper(target, target, tgtlen, findme, substr, sublen, maxlen - 1, &nlen);
    return nlen;
}

LitStrBuffer* lit_strbuf_makelongfromptr(LitStrBuffer* sb, size_t len)
{
    /* fprintf(stderr, "in makelong...\n"); */
    sb->isintern = false;
    sb->length = 0;
#if 0
        sb->capacity = lit_strbufutil_rndup2pow64(len + 1);
#else
    sb->capacity = (len + 1);
#endif
    sb->data = (char*)lit_sysmem_malloc(sb->capacity);
    if(!sb->data)
    {
        return NULL;
    }
    sb->data[0] = '\0';
    return sb;
}

bool lit_strbuf_initbasicempty(LitStrBuffer* sb, size_t len, bool isintern, bool preallocated)
{
    memset(sb, 0, sizeof(LitStrBuffer));
    sb->isintern = isintern;
    sb->capacity = len;
    sb->length = 0;
    sb->data = NULL;
    if(preallocated)
    {
        return true;
    }
    if(len > 0)
    {
        lit_strbuf_resize(sb, len);
    }
    return true;
}

bool lit_strbuf_makebasicemptystack(LitStrBuffer* sb, const char* str, size_t len)
{
    lit_strbuf_initbasicempty(sb, len, false, false);
    lit_strbuf_appendstrn(sb, str, len);
    return true;
}

LitStrBuffer* lit_strbuf_makebasicempty(const char* str, size_t len)
{
    LitStrBuffer* sb;
    sb = (LitStrBuffer*)lit_sysmem_malloc(sizeof(LitStrBuffer));
    if(!sb)
    {
        return NULL;
    }
    if(!lit_strbuf_initbasicempty(sb, len, false, false))
    {
        return NULL;
    }
    lit_strbuf_appendstrn(sb, str, len);
    return sb;
}

bool lit_strbuf_destroyfromstack(LitStrBuffer* sb)
{
    if(!sb->isintern)
    {
        lit_sysmem_free(sb->data);
    }
    return true;
}

bool lit_strbuf_destroy(LitStrBuffer* sb)
{
    lit_strbuf_destroyfromstack(sb);
    lit_sysmem_free(sb);
    return true;
}

/* Clear the content of an existing LitStrBuffer (sets size to 0) */
void lit_strbuf_reset(LitStrBuffer* sb)
{
    if(sb->data)
    {
        memset(sb->data, 0, sb->length);
    }
    sb->length = 0;
}

/* Ensure capacity for len characters plus '\0' character - exits on FAILURE */
bool lit_strbuf_ensurecapacity(LitStrBuffer* sb, size_t len)
{
    bool mustcopy;
    char* ptr;
    char* tmpbuf;
    mustcopy = false;
    tmpbuf = NULL;

    /* for nul byte */
    len++;
    if((sb->capacity == 0) || (sb->capacity < len))
    {
        sb->capacity = lit_strbufutil_rndup2pow64(len);
        /* fprintf(stderr, "sizeptr=%ld\n", sb->capacity); */
        if(mustcopy /*|| sb->data == NULL*/)
        {
            ptr = (char*)lit_sysmem_malloc(sb->capacity);
        }
        else
        {
            ptr = (char*)lit_sysmem_realloc(sb->data, sb->capacity);
        }
        if(ptr == NULL)
        {
            fprintf(stderr, "[%s:%i] out of memory: tried to allocate %d bytes\n", __FILE__, __LINE__, sb->capacity);
            return false;
        }
        if(mustcopy)
        {
            /* fprintf(stderr, "ensurecapacity: copying from short ((%d) <<%.*s>>)\n", (int)sb->length, (int)sb->length, tmpbuf); */
            memcpy(ptr, tmpbuf, sb->length);
        }
        sb->data = ptr;
    }
    return true;
}

/*
 * Resize the buffer to have capacity to hold a string of length newlen
 * (+ a null terminating character).  Can also be used to downsize the buffer's
 * memory usage.  Returns 1 on success, 0 on failure.
 */
bool lit_strbuf_resize(LitStrBuffer* sb, size_t newlen)
{
    return lit_strbuf_ensurecapacity(sb, newlen);
}

bool lit_strbuf_setlength(LitStrBuffer* sb, size_t len)
{
    sb->length = len;
    return true;
}

bool lit_strbuf_setdata(LitStrBuffer* sb, char* str)
{
    sb->data = str;
    return true;
}

size_t lit_strbuf_length(LitStrBuffer* sb)
{
    return sb->length;
}

const char* lit_strbuf_data(LitStrBuffer* sb)
{
    return sb->data;
}

#define lit_strbuf_mutdata(sb) ((sb)->data)

int lit_strbuf_get(LitStrBuffer* sb, size_t idx)
{
    return sb->data[idx];
}

bool lit_strbuf_containschar(LitStrBuffer* sb, char ch)
{
    size_t i;
    const char* data;
    data = lit_strbuf_data(sb);
    for(i = 0; i < sb->length; i++)
    {
        if(data[i] == ch)
        {
            return true;
        }
    }
    return false;
}

bool lit_strbuf_fullreplace(LitStrBuffer* from, LitStrBuffer* dest, const char* findmestr, size_t findmelen, const char* repwithstr, size_t repwithlen)
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
                lit_strbuf_appendstrn(dest, repwithstr, repwithlen);
            }
            i += findmelen - 1;
        }
        else
        {
            lit_strbuf_appendchar(dest, from->data[i]);
        }
    }
    return true;
}

bool lit_strbuf_charreplace(LitStrBuffer* sb, int findme, const char* substr, size_t sublen)
{
    size_t i;
    size_t nlen;
    size_t needed;
    char* data;
    needed = sb->capacity;
    data = lit_strbuf_mutdata(sb);
    for(i = 0; i < sb->length; i++)
    {
        if(data[i] == findme)
        {
            needed += sublen;
        }
    }
    if(!lit_strbuf_ensurecapacity(sb, needed + 1))
    {
        return false;
    }
    data = lit_strbuf_mutdata(sb);
    nlen = lit_strbufutil_inpreplace(data, sb->length, findme, substr, sublen, sb->capacity);
    sb->length = nlen;
    return true;
}

/* Set string buffer to contain a given string */
bool lit_strbuf_set(LitStrBuffer* sb, size_t idx, int b)
{
    char* data;
    lit_strbuf_ensurecapacity(sb, idx);
    data = lit_strbuf_mutdata(sb);
    data[idx] = b;
    return true;
}

/* Add a character to the end of this LitStrBuffer */
bool lit_strbuf_appendchar(LitStrBuffer* sb, int c)
{
    char* data;
    lit_strbuf_ensurecapacity(sb, sb->length + 1);
    data = lit_strbuf_mutdata(sb);
    data[sb->length] = c;
    data[sb->length + 1] = '\0';
    sb->length++;
    return true;
}

/*
 * Copy N characters from a character array to the end of this LitStrBuffer
 * strlen(str) must be >= len
 */
bool lit_strbuf_appendstrn(LitStrBuffer* sb, const char* str, size_t len)
{
    int epos;
    char* data;
    epos = 0;
    if(len > 0)
    {
        lit_strbuf_ensurecapacity(sb, sb->length + len);
        data = lit_strbuf_mutdata(sb);
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

/* Copy a character array to the end of this LitStrBuffer */
bool lit_strbuf_appendstr(LitStrBuffer* sb, const char* str)
{
    return lit_strbuf_appendstrn(sb, str, strlen(str));
}

bool lit_strbuf_appendbuff(LitStrBuffer* sb1, LitStrBuffer* sb2)
{
    return lit_strbuf_appendstrn(sb1, lit_strbuf_data(sb2), sb2->length);
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
 *   lit_strbufutil_numofdigits(0)   = 1
 *   lit_strbufutil_numofdigits(1)   = 1
 *   lit_strbufutil_numofdigits(10)  = 2
 *   lit_strbufutil_numofdigits(123) = 3
 */
size_t lit_strbufutil_numofdigits(unsigned long v)
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
    return 12 + lit_strbufutil_numofdigits(v / DYN_STRCONST_P12);
}

/* Convert integers to string to append */
bool lit_strbuf_appendnumulong(LitStrBuffer* sb, unsigned long value)
{
    size_t v;
    size_t pos;
    size_t numdigits;
    char* dst;
    char* data;
    /* Append two digits at a time */
    static const char* digits = ("0001020304050607080910111213141516171819"
                                 "2021222324252627282930313233343536373839"
                                 "4041424344454647484950515253545556575859"
                                 "6061626364656667686970717273747576777879"
                                 "8081828384858687888990919293949596979899");
    numdigits = lit_strbufutil_numofdigits(value);
    pos = numdigits - 1;
    lit_strbuf_ensurecapacity(sb, sb->length + numdigits);
    data = lit_strbuf_mutdata(sb);
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

bool lit_strbuf_appendnumlong(LitStrBuffer* sb, long value)
{
    /* lit_strbuf_appendformat(sb, "%li", value); */
    if(value < 0)
    {
        lit_strbuf_appendchar(sb, '-');
        value = -value;
    }
    return lit_strbuf_appendnumulong(sb, value);
}

bool lit_strbuf_appendnumint(LitStrBuffer* sb, int value)
{
    /* lit_strbuf_appendformat(sb, "%i", value); */
    return lit_strbuf_appendnumlong(sb, value);
}

/* Append string converted to lowercase */
bool lit_strbuf_appendstrnlowercase(LitStrBuffer* sb, const char* str, size_t len)
{
    char* to;
    char* data;
    const char* plength;
    lit_strbuf_ensurecapacity(sb, sb->length + len);
    data = lit_strbuf_mutdata(sb);
    to = data + sb->length;
    plength = str + len;
    for(; str < plength; str++, to++)
    {
        *to = tolower(*str);
    }
    sb->length += len;
    data[sb->length] = '\0';
    return true;
}

/* Append string converted to uppercase */
bool lit_strbuf_appendstrnuppercase(LitStrBuffer* sb, const char* str, size_t len)
{
    char* to;
    char* data;
    const char* end;
    lit_strbuf_ensurecapacity(sb, sb->length + len);
    data = lit_strbuf_mutdata(sb);
    to = data + sb->length;
    end = str + len;
    for(; str < end; str++, to++)
    {
        *to = toupper(*str);
    }
    sb->length += len;
    data[sb->length] = '\0';
    return true;
}

void lit_strbuf_shrink(LitStrBuffer* sb, size_t len)
{
    char* data;
    data = lit_strbuf_mutdata(sb);
    data[len] = 0;
    sb->length = len;
}

/*
 * Remove \r and \n characters from the end of this StringBuffer
 * Returns the number of characters removed
 */
size_t lit_strbuf_chomp(LitStrBuffer* sb)
{
    size_t oldlen;
    char* data;
    data = lit_strbuf_mutdata(sb);
    oldlen = sb->length;
    sb->length = lit_strbufutil_chomp(data, sb->length);
    return oldlen - sb->length;
}

/* Reverse a string */
void lit_strbuf_reverse(LitStrBuffer* sb)
{
    char* data;
    data = lit_strbuf_mutdata(sb);
    lit_strbufutil_reverseregion(data, sb->length);
}

/*
 * Get a substring as a new null terminated char array
 * (remember to free the returned char* after you're done with it!)
 */
char* lit_strbuf_substr(LitStrBuffer* sb, size_t start, size_t len)
{
    char* data;
    char* newstr;
    lit_strbuf_boundscheckreadrange(sb, start, len);
    data = lit_strbuf_mutdata(sb);
    newstr = (char*)lit_sysmem_malloc((len + 1) * sizeof(char));
    strncpy(newstr, data + start, len);
    newstr[len] = '\0';
    return newstr;
}

void lit_strbuf_touppercase(LitStrBuffer* sb)
{
    char* pos;
    char* end;
    char* data;
    data = lit_strbuf_mutdata(sb);
    end = data + sb->length;
    for(pos = data; pos < end; pos++)
    {
        *pos = (char)toupper(*pos);
    }
}

void lit_strbuf_tolowercase(LitStrBuffer* sb)
{
    char* pos;
    char* end;
    char* data;
    data = lit_strbuf_mutdata(sb);
    end = data + sb->length;
    for(pos = data; pos < end; pos++)
    {
        *pos = (char)tolower(*pos);
    }
}

/*
 * Copy a string to this LitStrBuffer, overwriting any existing characters
 * Note: dstpos + len can be longer the the current sb LitStrBuffer
 */
void lit_strbuf_copyover(LitStrBuffer* sb, size_t dstpos, const char* src, size_t len)
{
    size_t newlen;
    char* data;
    if(src == NULL || len == 0)
    {
        return;
    }
    lit_strbuf_boundscheckinsert(sb, dstpos);
    /*
     * Check if sb buffer can handle string
     * src may have pointed to sb, which has now moved
     */
    newlen = STRBUF_MAX(dstpos + len, sb->length);
    lit_strbuf_ensurecapacity(sb, newlen);
    data = lit_strbuf_mutdata(sb);
    /* memmove instead of strncpy, as it can handle overlapping regions */
    memmove(data + dstpos, src, len * sizeof(char));
    if(dstpos + len > sb->length)
    {
        /* Extended string - add '\0' char */
        sb->length = dstpos + len;
        data[sb->length] = '\0';
    }
}

/* Insert: copy to a LitStrBuffer, shifting any existing characters along */
void lit_strbuf_insert(LitStrBuffer* sb, size_t dstpos, const char* src, size_t len)
{
    char* data;
    char* insert;
    if(src == NULL || len == 0)
    {
        return;
    }
    lit_strbuf_boundscheckinsert(sb, dstpos);
    /*
     * Check if sb buffer has capacity for inserted string plus \0
     * src may have pointed to sb, which will be moved in realloc when
     * calling ensure capacity
     */
    lit_strbuf_ensurecapacity(sb, sb->length + len);
    data = lit_strbuf_mutdata(sb);
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
 * lit_strbuf_set(sb, "aaabbccc");
 * char *mystr = "xxx";
 * lit_strbuf_overwrite(sb,3,2,mystr,strlen(mystr));
 * // sb is now "aaaxxxccc"
 * lit_strbuf_overwrite(sb,3,2,"_",1);
 * // sb is now "aaa_ccc"
 */
void lit_strbuf_overwrite(LitStrBuffer* sb, size_t dstpos, size_t dstlen, const char* src, size_t srclen)
{
    size_t len;
    size_t newlen;
    char* tgt;
    char* end;
    char* data;
    lit_strbuf_boundscheckreadrange(sb, dstpos, dstlen);
    if(src == NULL)
    {
        return;
    }
    if(dstlen == srclen)
    {
        lit_strbuf_copyover(sb, dstpos, src, srclen);
    }
    newlen = sb->length + srclen - dstlen;
    lit_strbuf_ensurecapacity(sb, newlen);
    data = lit_strbuf_mutdata(sb);
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
 * lit_strbuf_set(sb, "aaaBBccc");
 * lit_strbuf_erase(sb, 3, 2);
 * // sb is now "aaaccc"
 */
void lit_strbuf_erase(LitStrBuffer* sb, size_t pos, size_t len)
{
    char* data;
    lit_strbuf_boundscheckreadrange(sb, pos, len);
    data = lit_strbuf_mutdata(sb);
    memmove(data + pos, data + pos + len, sb->length - pos - len);
    sb->length -= len;
    data[sb->length] = '\0';
}

int lit_strbuf_appendformatposv(LitStrBuffer* sb, size_t pos, const char* fmt, va_list argptr)
{
    size_t buflen;
    int numchars;
    va_list vacpy;
    char* data;
    lit_strbuf_boundscheckinsert(sb, pos);
    /* Length of remaining buffer */
    buflen = sb->capacity - pos;
    if(buflen == 0 && !lit_strbuf_ensurecapacity(sb, sb->capacity << 1))
    {
        fprintf(stderr, "%s:%i:Error: Out of memory\n", __FILE__, __LINE__);
        abort();
    }
    data = lit_strbuf_mutdata(sb);
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
        fprintf(stderr, "warning: lit_strbuf_appendformatv something went wrong..\n");
        abort();
    }
    /* numchars does not include the null terminating byte */
    if((size_t)numchars + 1 > buflen)
    {
        lit_strbuf_ensurecapacity(sb, pos + (size_t)numchars);
        /*
         * now use the argptr copy we made earlier
         * Don't need to use vsnprintf now, vsprintf will do since we know it'll fit
         */
        data = lit_strbuf_mutdata(sb);
        numchars = vsprintf(data + pos, fmt, vacpy);
        if(numchars < 0)
        {
            fprintf(stderr, "warning: lit_strbuf_appendformatv something went wrong..\n");
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

int lit_strbuf_appendformatv(LitStrBuffer* sb, const char* fmt, va_list argptr)
{
    return lit_strbuf_appendformatposv(sb, sb->length, fmt, argptr);
}

/* sprintf to the end of a LitStrBuffer (adds string terminator after sprint) */
int lit_strbuf_appendformat(LitStrBuffer* sb, const char* fmt, ...)
{
    int numchars;
    va_list argptr;
    va_start(argptr, fmt);
    numchars = lit_strbuf_appendformatposv(sb, sb->length, fmt, argptr);
    va_end(argptr);
    return numchars;
}

/* Print at a given position (overwrite chars at positions >= pos) */
int lit_strbuf_appendformatat(LitStrBuffer* sb, size_t pos, const char* fmt, ...)
{
    int numchars;
    va_list argptr;
    lit_strbuf_boundscheckinsert(sb, pos);
    va_start(argptr, fmt);
    numchars = lit_strbuf_appendformatposv(sb, pos, fmt, argptr);
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
int lit_strbuf_appendformatnoterm(LitStrBuffer* sb, size_t pos, const char* fmt, ...)
{
    size_t len;
    int nchars;
    char lastchar;
    va_list argptr;
    char* data;
    lit_strbuf_boundscheckinsert(sb, pos);
    len = sb->length;
    /* Call vsnprintf with NULL, 0 to get resulting string length without writing */
    va_start(argptr, fmt);
    nchars = vsnprintf(NULL, 0, fmt, argptr);
    va_end(argptr);
    if(nchars < 0)
    {
        fprintf(stderr, "warning: lit_strbuf_appendformatv something went wrong..\n");
        abort();
    }
    /* Save overwritten char */
    data = lit_strbuf_mutdata(sb);
    lastchar = (pos + (size_t)nchars < sb->length) ? data[pos + (size_t)nchars] : 0;
    va_start(argptr, fmt);
    nchars = lit_strbuf_appendformatposv(sb, pos, fmt, argptr);
    va_end(argptr);
    if(nchars < 0)
    {
        fprintf(stderr, "warning: lit_strbuf_appendformatv something went wrong..\n");
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
void lit_strbuf_triminplace(LitStrBuffer* sb)
{
    size_t start;
    char* data;
    if(sb->length == 0)
    {
        return;
    }
    data = lit_strbuf_mutdata(sb);
    /* Trim end first */
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
}

/*
 * Trim the characters listed in `list` from the left of `sb`
 * `list` is a null-terminated string of characters
 */
void lit_strbuf_trimleftinplace(LitStrBuffer* sb, const char* list)
{
    size_t start;
    char* data;
    start = 0;
    data = lit_strbuf_mutdata(sb);
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
void lit_strbuf_trimrightinplace(LitStrBuffer* sb, const char* list)
{
    char* data;
    if(sb->length == 0)
    {
        return;
    }
    data = lit_strbuf_mutdata(sb);
    while(sb->length > 0 && strchr(list, data[sb->length - 1]) != NULL)
    {
        sb->length--;
    }
    data[sb->length] = '\0';
}

bool lit_util_charisdigit(char c)
{
    return c >= '0' && c <= '9';
}

bool lit_util_charisalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float */
int lit_util_closestpoweroftwo(int n)
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

int lit_util_stringutfgetcountdecode(uint8_t byte)
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

int lit_util_stringutfgetcountencode(int value)
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

int lit_util_stringutfencode(int value, uint8_t* bytes)
{
    if(value <= 0x7f)
    {
        *bytes = value & 0x7f;
        return 1;
    }
    else if(value <= 0x7ff)
    {
        *bytes = 0xc0 | ((value & 0x7c0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 2;
    }
    else if(value <= 0xffff)
    {
        *bytes = 0xe0 | ((value & 0xf000) >> 12);
        bytes++;
        *bytes = 0x80 | ((value & 0xfc0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 3;
    }
    else if(value <= 0x10ffff)
    {
        *bytes = 0xf0 | ((value & 0x1c0000) >> 18);
        bytes++;
        *bytes = 0x80 | ((value & 0x3f000) >> 12);
        bytes++;
        *bytes = 0x80 | ((value & 0xfc0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 4;
    }
    UNREACHABLE
    return 0;
}

int lit_util_stringutfdecode(const uint8_t* bytes, uint32_t length)
{
    if(*bytes <= 0x7f)
    {
        return *bytes;
    }
    int value;
    uint32_t remainingbytes;
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

int lit_util_isutfbyte(int c)
{
    return (((c) & 0xC0) != 0x80);
}

int lit_util_stringutfucharoffset(const char* str, int index)
{
    int offset = 0;
    while(index > 0 && str[offset])
    {
        if(!lit_util_isutfbyte(str[++offset]))
        {
            if(!lit_util_isutfbyte(str[++offset]))
            {
                if(!lit_util_isutfbyte(str[++offset]))
                {
                    ++offset;
                }
            }
        }
        index--;
    }
    return offset;
}

void lit_util_stringchangecase(char* instr, size_t inlen, int (*func)(int))
{
    size_t i;
    for(i = 0; i < inlen; i++)
    {
        instr[i] = func(instr[i]);
    }
}


static int lit_util_findfirstpos(const char* str, size_t len, int ch)
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



char* lit_util_readhandle(FILE* hnd, size_t* dlen)
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
    buf = (char*)calloc(toldlen + 1, sizeof(char));
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

char* lit_util_readfile(const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    if((fh = fopen(filename, "rb")) == NULL)
    {
        return NULL;
    }
    b = lit_util_readhandle(fh, dlen);
    fclose(fh);
    return b;
}


bool lit_util_fileexists(const char* path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

char* lit_util_dupstring(const char* string)
{
    size_t length = strlen(string) + 1;
    char* newstring = (char*)lit_sysmem_malloc(length);
    memcpy(newstring, string, length);
    return newstring;
}

// endutils

void lit_dynlistval_init(LitDynListVal* list)
{
    lit_dynlistval_reset(list);
}

void lit_dynlistval_reset(LitDynListVal* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistval_destroy(LitDynListVal* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistval_reset(list);
}

void lit_dynlistval_push(LitDynListVal* list, LitValue val)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t i;
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (LitValue*)lit_sysmem_realloc(list->listitems, sizeof(LitValue) * (list->listcapacity));
        for(i = oldcapacity; i < list->listcapacity; i++)
        {
            list->listitems[i] = lit_value_makenull();
        }
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}

LitValue lit_dynlistval_get(LitDynListVal* list, size_t idx)
{
    return list->listitems[idx];
}

LitValue lit_dynlistval_set(LitDynListVal* list, size_t idx, LitValue val)
{
    list->listitems[idx] = val;
    return list->listitems[idx];
}

void lit_dynlistval_ensuresize(LitDynListVal* list, size_t size)
{
    lit_dynlistval_ensureactualsize(list, size);
    if(list->listcount < size)
    {
        list->listcount = size;
    }
}

void lit_dynlistval_ensureactualsize(LitDynListVal* list, size_t size)
{
    size_t i;
    if(list->listcapacity < size)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = size;
        list->listitems = (LitValue*)lit_sysmem_realloc(list->listitems, sizeof(LitValue) * (size));
        for(i = oldcapacity; i < size; i++)
        {
            list->listitems[i] = lit_value_makenull();
        }
    }
}

void lit_dynlistpriv_init(LitDynListPriv* list)
{
    lit_dynlistpriv_reset(list);
}

void lit_dynlistpriv_reset(LitDynListPriv* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistpriv_destroy(LitDynListPriv* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistpriv_reset(list);
}

void lit_dynlistpriv_push(LitDynListPriv* list, LitAstPrivate priv)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (LitAstPrivate*)lit_sysmem_realloc(list->listitems, sizeof(LitAstPrivate) * (list->listcapacity));
    }
    list->listitems[list->listcount] = priv;
    list->listcount++;
}

void lit_dynlistloc_init(LitDynListLoc* list)
{
    lit_dynlistloc_reset(list);
}

void lit_dynlistloc_reset(LitDynListLoc* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistloc_destroy(LitDynListLoc* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistloc_reset(list);
}

void lit_dynlistloc_push(LitDynListLoc* list, LitAstLocal loc)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (LitAstLocal*)lit_sysmem_realloc(list->listitems, sizeof(LitAstLocal) * (list->listcapacity));
    }
    list->listitems[list->listcount] = loc;
    list->listcount++;
}

void lit_dynlistexpr_init(LitDynListExpr* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistexpr_destroy(LitDynListExpr* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistexpr_init(list);
}

void lit_dynlistexpr_push(LitDynListExpr* list, LitAstExpression* value)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (LitAstExpression**)lit_sysmem_realloc(list->listitems, sizeof(LitAstExpression*) * (list->listcapacity));
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}

void lit_dynlistparam_init(LitDynListParam* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistparam_destroy(LitDynListParam* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistparam_init(list);
}

void lit_dynlistparam_push(LitDynListParam* list, LitAstFuncParamExpr value)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (LitAstFuncParamExpr*)lit_sysmem_realloc(list->listitems, sizeof(LitAstFuncParamExpr) * (list->listcapacity));
    }
    list->listitems[list->listcount] = value;
    list->listcount++;
}

void lit_dynlistuint_init(LitDynListUInt* list)
{
    lit_dynlistuint_reset(list);
}

void lit_dynlistuint_reset(LitDynListUInt* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistuint_destroy(LitDynListUInt* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistuint_reset(list);
}

void lit_dynlistuint_push(LitDynListUInt* list, size_t val)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (size_t*)lit_sysmem_realloc(list->listitems, sizeof(size_t) * (list->listcapacity));
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}

void lit_dynlistbyte_init(LitDynListByte* list)
{
    lit_dynlistbyte_reset(list);
}

void lit_dynlistbyte_reset(LitDynListByte* list)
{
    list->listitems = NULL;
    list->listcapacity = 0;
    list->listcount = 0;
}

void lit_dynlistbyte_destroy(LitDynListByte* list)
{
    lit_sysmem_free(list->listitems);
    lit_dynlistbyte_reset(list);
}

void lit_dynlistbyte_push(LitDynListByte* list, uint8_t val)
{
    if(list->listcapacity < list->listcount + 1)
    {
        size_t oldcapacity = list->listcapacity;
        list->listcapacity = LIT_GROW_CAPACITY(oldcapacity);
        list->listitems = (uint8_t*)lit_sysmem_realloc(list->listitems, sizeof(uint8_t) * (list->listcapacity));
    }
    list->listitems[list->listcount] = val;
    list->listcount++;
}


void lit_table_init(LitState* state, LitTable* table)
{
    table->pstate = state;
    lit_table_resetvars(table);
}

void lit_table_resetvars(LitTable* table)
{
    table->htcapacity = -1;
    table->htcount = 0;
    table->htentries = NULL;
}

void lit_free_table(LitTable* table)
{
    if(table->htcapacity > 0)
    {
        lit_sysmem_free(table->htentries);
    }
    lit_table_resetvars(table);
}

LitTabEntry* lit_table_findentry(LitTabEntry* entries, int capacity, LitString* key)
{
    uint32_t index = key->strhash % capacity;
    LitTabEntry* tombstone = NULL;
    while(true)
    {
        LitTabEntry* entry = &entries[index];
        if(entry->entkey == NULL)
        {
            if(lit_value_isnull(entry->entvalue))
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

void lit_table_adjustcapacity(LitTable* table, int capacity)
{
    int i;
    LitTabEntry* entry;
    LitTabEntry* entries;
    LitTabEntry* destination;
    entries = (LitTabEntry*)lit_sysmem_malloc((capacity + 1) * sizeof(LitTabEntry));
    for(i = 0; i <= capacity; i++)
    {
        entries[i].entkey = NULL;
        entries[i].entvalue = lit_value_makenull();
    }
    table->htcount = 0;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->entkey == NULL)
        {
            continue;
        }
        destination = lit_table_findentry(entries, capacity, entry->entkey);
        destination->entkey = entry->entkey;
        destination->entvalue = entry->entvalue;
        table->htcount++;
    }
    lit_sysmem_free(table->htentries);
    table->htcapacity = capacity;
    table->htentries = entries;
}

bool lit_table_set(LitTable* table, LitString* key, LitValue value)
{
    if(table->htcount + 1 > (table->htcapacity + 1) * LIT_CONFIG_TABLEMAXLOAD)
    {
        int capacity = LIT_GROW_CAPACITY(table->htcapacity + 1) - 1;
        lit_table_adjustcapacity(table, capacity);
    }
    LitTabEntry* entry = lit_table_findentry(table->htentries, table->htcapacity, key);
    bool isnew = entry->entkey == NULL;
    if(isnew && lit_value_isnull(entry->entvalue))
    {
        table->htcount++;
    }
    entry->entkey = key;
    entry->entvalue = value;
    return isnew;
}

bool lit_table_getentry(LitTable* table, LitString* key, LitValue* value)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = lit_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    *value = entry->entvalue;
    return true;
}

bool lit_table_getslot(LitTable* table, LitString* key, LitValue** value)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = lit_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    *value = &entry->entvalue;
    return true;
}

bool lit_table_delete(LitTable* table, LitString* key)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = lit_table_findentry(table->htentries, table->htcapacity, key);
    if(entry->entkey == NULL)
    {
        return false;
    }
    entry->entkey = NULL;
    entry->entvalue = lit_value_makebool(true);
    return true;
}

LitString* lit_table_findstring(LitTable* table, const char* chars, size_t length, uint32_t hash)
{
    size_t entlen;
    const char* entstr;
    uint32_t enthash;
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
    uint32_t index = hash % table->htcapacity;
    while(true)
    {
        LitTabEntry* entry = &table->htentries[index];
        if(entry->entkey == NULL)
        {
            if(lit_value_isnull(entry->entvalue))
            {
                return NULL;
            }
        }
        else
        {
            entlen = entry->entkey->strbuf.length;
            enthash = entry->entkey->strhash;
            entstr = entry->entkey->strbuf.data;
            if(entlen == length && /* enthash == hash && */ memcmp(entstr, chars, length) == 0)
            {
                return entry->entkey;
            }
        }
        index = (index + 1) % table->htcapacity;
    }
}

void lit_table_addall(LitTable* from, LitTable* to)
{
    int i;
    LitTabEntry* entry;
    for(i = 0; i <= from->htcapacity; i++)
    {
        entry = &from->htentries[i];
        if(entry->entkey != NULL)
        {
            lit_table_set(to, entry->entkey, entry->entvalue);
        }
    }
}

void lit_table_addallignoring(LitTable* from, LitTable* to)
{
    int i;
    LitValue fake;
    LitTabEntry* entry;
    for(i = 0; i <= from->htcapacity; i++)
    {
        entry = &from->htentries[i];
        if(entry->entkey != NULL && !lit_table_getentry(to, entry->entkey, &fake))
        {
            lit_table_set(to, entry->entkey, entry->entvalue);
        }
    }
}

void lit_table_removewhite(LitTable* table)
{
    int i;
    LitObject* obj;
    LitTabEntry* entry;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->entkey != NULL)
        {
            obj = (LitObject*)entry->entkey;
            if(!obj->marked)
            {
                lit_table_delete(table, entry->entkey);
            }
        }
    }
}

void lit_table_markentries(LitTable* table)
{
    int i;
    LitState* state;
    LitTabEntry* entry;
    state = table->pstate;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        lit_gcmem_markobject(state, (LitObject*)entry->entkey);
        lit_gcmem_markvalue(state, entry->entvalue);
    }
}

void lit_chunk_init(LitChunk* chunk)
{
    lit_chunk_reset(chunk);
    lit_dynlistval_init(&chunk->constantlist);
}

void lit_chunk_reset(LitChunk* chunk)
{
    chunk->compiledcodecount = 0;
    chunk->capacity = 0;
    chunk->compiledcodechunk = NULL;
    chunk->haslineinfo = true;
    chunk->linecount = 0;
    chunk->linecapacity = 0;
    chunk->lines = NULL;
}

void lit_chunk_destroy(LitChunk* chunk)
{
    lit_sysmem_free(chunk->compiledcodechunk);
    lit_sysmem_free(chunk->lines);
    lit_dynlistval_destroy(&chunk->constantlist);
    lit_chunk_reset(chunk);
}

void lit_chunk_push(LitChunk* chunk, uint64_t word, uint16_t line)
{
    if(chunk->capacity < chunk->compiledcodecount + 1)
    {
        size_t oldcapacity = chunk->capacity;
        chunk->capacity = LIT_GROW_CAPACITY(oldcapacity);
        chunk->compiledcodechunk = (uint64_t*)lit_sysmem_realloc(chunk->compiledcodechunk, sizeof(uint64_t) * (chunk->capacity));
    }
    chunk->compiledcodechunk[chunk->compiledcodecount] = word;
    chunk->compiledcodecount++;
    if(!chunk->haslineinfo)
    {
        return;
    }
    if(chunk->linecapacity < chunk->linecount + 4)
    {
        size_t oldcapacity = chunk->linecapacity;
        chunk->linecapacity = LIT_GROW_CAPACITY(chunk->linecapacity);
        chunk->lines = (uint16_t*)lit_sysmem_realloc(chunk->lines, sizeof(uint16_t) * (chunk->linecapacity));
        if(oldcapacity == 0)
        {
            chunk->lines[0] = 0;
            chunk->lines[1] = 0;
        }
    }
    size_t lineindex = chunk->linecount;
    size_t value = chunk->lines[lineindex];
    if(value != 0 && value != line)
    {
        chunk->linecount += 2;
        lineindex = chunk->linecount;
        chunk->lines[lineindex + 1] = 0;
    }
    chunk->lines[lineindex] = line;
    chunk->lines[lineindex + 1]++;
}

size_t lit_chunk_addconstant(LitState* state, LitChunk* chunk, LitValue constant)
{
    lit_state_pushvalueroot(state, constant);
    lit_dynlistval_push(&chunk->constantlist, constant);
    lit_state_poproot(state);
    return chunk->constantlist.listcount - 1;
}

size_t lit_chunk_getline(LitChunk* chunk, size_t offset)
{
    size_t i;
    size_t rle;
    size_t line;
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
    line = 0;
    index = 0;
    for(i = 0; i <= offset; i++)
    {
        if(rle > 0)
        {
            rle--;
            continue;
        }
        line = 0;
        rle = 0;
        if(index <= chunk->linecapacity)
        {
            line = chunk->lines[index];
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
    return line;
}

void lit_chunk_shrink(LitChunk* chunk)
{
    size_t oldcapacity;
    (void)oldcapacity;
    if(chunk->capacity > chunk->compiledcodecount)
    {
        oldcapacity = chunk->capacity;
        chunk->capacity = chunk->compiledcodecount;
        chunk->compiledcodechunk = (uint64_t*)lit_sysmem_realloc(chunk->compiledcodechunk, sizeof(uint64_t) * (chunk->capacity));
    }
    if(chunk->linecapacity > chunk->linecount)
    {
        oldcapacity = chunk->linecapacity;
        chunk->linecapacity = chunk->linecount + 2;
        chunk->lines = (uint16_t*)lit_sysmem_realloc(chunk->lines, sizeof(uint16_t) * (chunk->linecapacity));
    }
}

void lit_iostream_initvars(LitIOStream* pr, LitIOStrMode mode)
{
    pr->fromstack = false;
    pr->wrmode = LIT_IOSTRMODE_UNDEFINED;
    pr->shouldclose = false;
    pr->shouldflush = false;
    pr->stringtaken = false;
    pr->shortenvalues = false;
    pr->jsonmode = false;
    pr->cachedistty = false;
    pr->havecachedtty = false;
    pr->maxvallength = 15;
    pr->handle = NULL;
    pr->wrmode = mode;
}

bool lit_iostream_makestackio(LitIOStream* pr, FILE* fh, bool shouldclose)
{
    lit_iostream_initvars(pr, LIT_IOSTRMODE_FILE);
    pr->fromstack = true;
    pr->handle = fh;
    pr->shouldclose = shouldclose;
    return true;
}

bool lit_iostream_makestackopenfile(LitIOStream* pr, const char* path, bool writemode)
{
    const char* mode;
    lit_iostream_initvars(pr, LIT_IOSTRMODE_FILE);
    mode = "rb";
    if(writemode)
    {
        mode = "wb";
    }
    pr->fromstack = true;
    pr->shouldclose = true;
    pr->handle = fopen(path, mode);
    if(pr->handle == NULL)
    {
        return false;
    }
    return true;
}

bool lit_iostream_makestackstring(LitIOStream* pr)
{
    lit_iostream_initvars(pr, LIT_IOSTRMODE_STRING);
    pr->fromstack = true;
    pr->wrmode = LIT_IOSTRMODE_STRING;
    lit_strbuf_makebasicemptystack(&pr->psbuf, NULL, 0);
    return true;
}

LitIOStream* lit_iostream_makeundefined(LitIOStrMode mode)
{
    LitIOStream* pr;
    pr = (LitIOStream*)lit_sysmem_malloc(sizeof(LitIOStream));
    if(!pr)
    {
        fprintf(stderr, "cannot allocate LitIOStream\n");
        return NULL;
    }
    lit_iostream_initvars(pr, mode);
    return pr;
}

LitIOStream* lit_iostream_makeio(FILE* fh, bool shouldclose)
{
    LitIOStream* pr;
    pr = lit_iostream_makeundefined(LIT_IOSTRMODE_FILE);
    pr->handle = fh;
    pr->shouldclose = shouldclose;
    return pr;
}

LitIOStream* lit_iostream_makeopenfile(const char* path, bool writemode)
{
    LitIOStream* pr;
    pr = lit_iostream_makeundefined(LIT_IOSTRMODE_FILE);
    if(lit_iostream_makestackopenfile(pr, path, writemode))
    {
        pr->fromstack = false;
        return pr;
    }
    else
    {
        lit_iostream_destroy(pr);
    }
    return NULL;
}

LitIOStream* lit_iostream_makestring()
{
    LitIOStream* pr;
    pr = lit_iostream_makeundefined(LIT_IOSTRMODE_STRING);
    lit_strbuf_makebasicemptystack(&pr->psbuf, NULL, 0);
    return pr;
}

void lit_iostream_destroy(LitIOStream* pr)
{
    if(pr == NULL)
    {
        return;
    }
    if(pr->wrmode == LIT_IOSTRMODE_UNDEFINED)
    {
        return;
    }
    /*fprintf(stderr, "lit_iostream_destroy: pr->wrmode=%d\n", pr->wrmode);*/
    if(pr->wrmode == LIT_IOSTRMODE_STRING)
    {
        if(!pr->stringtaken)
        {
            lit_strbuf_destroyfromstack(&pr->psbuf);
        }
    }
    else if(pr->wrmode == LIT_IOSTRMODE_FILE)
    {
        if(pr->shouldclose)
        {
#if 0
            fclose(pr->handle);
#endif
        }
    }
    if(!pr->fromstack)
    {
        lit_sysmem_free(pr);
        pr = NULL;
    }
}

bool lit_iostream_istty(LitIOStream* pr)
{
    int fd;
    if(pr->havecachedtty)
    {
        return pr->cachedistty;
    }
    if(pr->wrmode == LIT_IOSTRMODE_FILE)
    {
        if(pr->handle != NULL)
        {
            fd = fileno(pr->handle);
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

LitString* lit_iostream_takestring(LitState* state, LitIOStream* pr)
{
    LitString* os;
    os = lit_string_makewithstrbuf(state, pr->psbuf);
    pr->stringtaken = true;
    return os;
}

void lit_iostream_flush(LitIOStream* pr)
{
    if(pr->shouldflush)
    {
        fflush(pr->handle);
    }
}

bool lit_iostream_putlen(LitIOStream* pr, const char* estr, size_t elen)
{
    size_t chlen;
    chlen = sizeof(char);
    if(elen > 0)
    {
        if(pr->wrmode == LIT_IOSTRMODE_FILE)
        {
            fwrite(estr, chlen, elen, pr->handle);
            lit_iostream_flush(pr);
        }
        else if(pr->wrmode == LIT_IOSTRMODE_STRING)
        {
            lit_strbuf_appendstrn(&pr->psbuf, estr, elen);
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool lit_iostream_puts(LitIOStream* pr, const char* estr)
{
    return lit_iostream_putlen(pr, estr, strlen(estr));
}

bool lit_iostream_writecolor(LitIOStream* pr, const char* color)
{
    if(lit_iostream_istty(pr))
    {
        return lit_iostream_puts(pr, color);
    }
    return false;
}

bool lit_iostream_writechar(LitIOStream* pr, int b)
{
    char ch;
    if(pr->wrmode == LIT_IOSTRMODE_STRING)
    {
        ch = b;
        lit_iostream_putlen(pr, &ch, 1);
    }
    else if(pr->wrmode == LIT_IOSTRMODE_FILE)
    {
        fputc(b, pr->handle);
        lit_iostream_flush(pr);
    }
    return true;
}

bool lit_iostream_writeescapedchar(LitIOStream* pr, int ch)
{
    switch(ch)
    {
        case '\'':
        {
            lit_iostream_puts(pr, "\\\'");
        }
        break;
        case '\"':
        {
            lit_iostream_puts(pr, "\\\"");
        }
        break;
        case '\\':
        {
            lit_iostream_puts(pr, "\\\\");
        }
        break;
        case '\b':
        {
            lit_iostream_puts(pr, "\\b");
        }
        break;
        case '\f':
        {
            lit_iostream_puts(pr, "\\f");
        }
        break;
        case '\n':
        {
            lit_iostream_puts(pr, "\\n");
        }
        break;
        case '\r':
        {
            lit_iostream_puts(pr, "\\r");
        }
        break;
        case '\t':
        {
            lit_iostream_puts(pr, "\\t");
        }
        break;
        case 0:
        {
            lit_iostream_puts(pr, "\\0");
        }
        break;
        default:
        {
            lit_iostream_printf(pr, "\\x%02x", (unsigned char)ch);
        }
        break;
    }
    return true;
}

bool lit_iostream_putquotedstring(LitIOStream* pr, const char* str, size_t len, bool withquot)
{
    int bch;
    size_t i;
    bch = 0;
    if(withquot)
    {
        lit_iostream_writechar(pr, '"');
    }
    for(i = 0; i < len; i++)
    {
        bch = str[i];
        if((bch < 32) || (bch > 127) || (bch == '\"') || (bch == '\\'))
        {
            lit_iostream_writeescapedchar(pr, bch);
        }
        else
        {
            lit_iostream_writechar(pr, bch);
        }
    }
    if(withquot)
    {
        lit_iostream_writechar(pr, '"');
    }
    return true;
}

bool lit_iostream_vwritefmttostring(LitIOStream* pr, const char* fmt, va_list va)
{
    lit_strbuf_appendformatv(&pr->psbuf, fmt, va);
    return true;
}

bool lit_iostream_vwritefmt(LitIOStream* pr, const char* fmt, va_list va)
{
    if(pr->wrmode == LIT_IOSTRMODE_STRING)
    {
        return lit_iostream_vwritefmttostring(pr, fmt, va);
    }
    else if(pr->wrmode == LIT_IOSTRMODE_FILE)
    {
        vfprintf(pr->handle, fmt, va);
        lit_iostream_flush(pr);
    }
    return true;
}

bool lit_iostream_printf(LitIOStream* pr, const char* fmt, ...) LIT_ATTRIB((format(printf, 2, 3)));
bool lit_iostream_printf(LitIOStream* pr, const char* fmt, ...)
{
    bool b;
    va_list va;
    va_start(va, fmt);
    b = lit_iostream_vwritefmt(pr, fmt, va);
    va_end(va);
    return b;
}


bool lit_value_isbool(LitValue v)
{
    return (v.type == LIT_VALTYP_BOOL);
}

bool lit_value_isnull(LitValue v)
{
    return (v.type == LIT_VALTYP_NULL);
}

bool lit_value_isnumber(LitValue v)
{
    return (v.type == LIT_VALTYP_NUMBER);
}

bool lit_value_isobject(LitValue v)
{
    return (v.type == LIT_VALTYP_OBJECT);
}

bool lit_value_isobjtype(LitValue value, LitObjType t)
{
    if(lit_value_isobject(value))
    {
        return (lit_value_asobject(value)->type == t);
    }
    return false;
}

bool lit_value_ismap(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_MAP);
}

bool lit_value_isstring(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_STRING);
}

bool lit_value_isfuncscript(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_FUNCSCRIPT);
}

bool lit_value_isfuncmethod(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_FUNCNATMETHOD);
}

bool lit_value_ismodule(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_MODULE);
}

bool lit_value_isclass(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_CLASS);
}

bool lit_value_isinstance(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_INSTANCE);
}

bool lit_value_isvargarray(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_VARARGARRAY);
}

bool lit_value_isarray(LitValue value)
{
    return (lit_value_isobjtype(value, LIT_OBJ_ARRAY) || lit_value_isvargarray(value));
}

bool lit_value_isrange(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_RANGE);
}

bool lit_value_isfield(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_FIELD);
}

bool lit_value_isreference(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_REFERENCE);
}

double lit_value_asnumber(LitValue value)
{
    return value.as.numval;
}

bool lit_value_asbool(LitValue v)
{
    return (v.as.boolval);
}

LitObjType lit_value_objtype(LitValue value)
{
    return lit_value_asobject(value)->type;
}

LitObject* lit_value_asobject(LitValue v)
{
    return (v.as.obj);
}

LitString* lit_value_asstring(LitValue value)
{
    return ((LitString*)lit_value_asobject(value));
}

LitFuncScript* lit_value_asfuncscript(LitValue value)
{
    return ((LitFuncScript*)lit_value_asobject(value));
}

LitFuncNative* lit_value_asfuncnative(LitValue value)
{
    return ((LitFuncNative*)lit_value_asobject(value));
}

LitFuncNative* lit_value_asfuncmethod(LitValue value)
{
    return ((LitFuncNative*)lit_value_asobject(value));
}

LitModule* lit_value_asmodule(LitValue value)
{
    return ((LitModule*)lit_value_asobject(value));
}

LitFuncClosure* lit_value_asfuncclosure(LitValue value)
{
    return ((LitFuncClosure*)lit_value_asobject(value));
}

LitClsPrototype* lit_value_asclsproto(LitValue value)
{
    return ((LitClsPrototype*)lit_value_asobject(value));
}

LitUpvalue* lit_value_asupvalue(LitValue value)
{
    return ((LitUpvalue*)lit_value_asobject(value));
}

LitClass* lit_value_asclass(LitValue value)
{
    return ((LitClass*)lit_value_asobject(value));
}

LitInstance* lit_value_asinstance(LitValue value)
{
    return ((LitInstance*)lit_value_asobject(value));
}

LitArray* lit_value_asarray(LitValue value)
{
    return ((LitArray*)lit_value_asobject(value));
}

LitMap* lit_value_asmap(LitValue value)
{
    return ((LitMap*)lit_value_asobject(value));
}

LitFuncBound* lit_value_asfuncboundmethod(LitValue value)
{
    return ((LitFuncBound*)lit_value_asobject(value));
}

LitUserdata* lit_value_asuserdata(LitValue value)
{
    return ((LitUserdata*)lit_value_asobject(value));
}

LitRange* lit_value_asrange(LitValue value)
{
    return ((LitRange*)lit_value_asobject(value));
}

LitField* lit_value_asfield(LitValue value)
{
    return ((LitField*)lit_value_asobject(value));
}

LitFiber* lit_value_asfiber(LitValue value)
{
    return ((LitFiber*)lit_value_asobject(value));
}

LitReference* lit_value_asreference(LitValue value)
{
    return ((LitReference*)lit_value_asobject(value));
}

LitValue lit_value_makenull()
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = LIT_VALTYP_NULL;
    rt.as.numval = 0;
    return rt;
}

LitValue lit_value_makebool(bool b)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = LIT_VALTYP_BOOL;
    rt.as.boolval = b;
    return rt;
}

LitValue lit_value_makenumber(double num)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = LIT_VALTYP_NUMBER;
    rt.as.numval = num;
    return rt;
}

#define lit_value_fromobject(obj) lit_value_fromobject_actual((LitObject*)(obj))

LitValue lit_value_fromobject_actual(LitObject* obj)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = LIT_VALTYP_OBJECT;
    rt.as.obj = obj;
    return rt;
}

bool lit_is_falsey(LitValue value)
{
    if(lit_value_isbool(value))
    {
        return (lit_value_asbool(value) == false);
    }
    else if(lit_value_isnull(value))
    {
        return true;
    }
    else if(lit_value_isnumber(value))
    {
        return (lit_value_asnumber(value) == 0);
    }
    return false;
}

bool lit_value_compare(LitState* state, LitValue a, LitValue b)
{
    LitValue tmpargs[2];
    if(a.type == b.type)
    {
        if(lit_value_isnumber(a))
        {
            return lit_value_asnumber(a) == lit_value_asnumber(b);
        }
        if(lit_value_isnull(a))
        {
            return true;
        }
        if(lit_value_isbool(a))
        {
            return lit_value_asbool(a) == lit_value_asbool(b);
        }
        if(lit_value_asobject(a) == lit_value_asobject(b))
        {
            return true;
        }
        if(lit_value_isstring(a) && lit_value_isstring(b))
        {
            LitString* as = lit_value_asstring(a);
            LitString* bs = lit_value_asstring(b);
            return as->strbuf.length == bs->strbuf.length && memcmp(as->strbuf.data, bs->strbuf.data, as->strbuf.length) == 0;
        }
    }
    else
    {
        if(lit_value_isbool(a) && lit_value_isnumber(b))
        {
            return lit_value_asbool(a) == lit_value_asnumber(b);
        }
        if(lit_value_isnumber(a) && lit_value_isbool(b))
        {
            return lit_value_asnumber(a) == lit_value_asbool(b);
        }
        if(lit_value_isstring(a) || lit_value_isstring(b))
        {
            return false;
        }
    }
    tmpargs[0] = b;
    return !lit_is_falsey(lit_state_findandcallmethod(state, a, state->strings.stropequal, tmpargs, 1).result);
}

const char* lit_value_objtypename(int t)
{
    switch(t)
    {
        case LIT_OBJ_STRING:
            return "LIT_OBJ_STRING";
        case LIT_OBJ_FUNCSCRIPT:
            return "LIT_OBJ_FUNCSCRIPT";
        case LIT_OBJ_FUNCNATIVE:
            return "LIT_OBJ_FUNCNATIVE";
        case LIT_OBJ_FUNCNATMETHOD:
            return "LIT_OBJ_FUNCNATMETHOD";
        case LIT_OBJ_FIBER:
            return "LIT_OBJ_FIBER";
        case LIT_OBJ_MODULE:
            return "LIT_OBJ_MODULE";
        case LIT_OBJ_FUNCCLOSURE:
            return "LIT_OBJ_FUNCCLOSURE";
        case LIT_OBJ_CLSPROTOTYPE:
            return "LIT_OBJ_CLSPROTOTYPE";
        case LIT_OBJ_UPVALUE:
            return "LIT_OBJ_UPVALUE";
        case LIT_OBJ_CLASS:
            return "LIT_OBJ_CLASS";
        case LIT_OBJ_INSTANCE:
            return "LIT_OBJ_INSTANCE";
        case LIT_OBJ_FUNCBOUNDMETHOD:
            return "LIT_OBJ_FUNCBOUNDMETHOD";
        case LIT_OBJ_ARRAY:
            return "LIT_OBJ_ARRAY";
        case LIT_OBJ_VARARGARRAY:
            return "LIT_OBJ_VARARGARRAY";
        case LIT_OBJ_MAP:
            return "LIT_OBJ_MAP";
        case LIT_OBJ_USERDATA:
            return "LIT_OBJ_USERDATA";
        case LIT_OBJ_RANGE:
            return "LIT_OBJ_RANGE";
        case LIT_OBJ_FIELD:
            return "LIT_OBJ_FIELD";
        case LIT_OBJ_REFERENCE:
            return "LIT_OBJ_REFERENCE";
    }
    return "?unknown?";
}

const char* lit_value_valtypefromtype(int t)
{
    switch(t)
    {
        case LIT_VALTYP_NULL:
            return "null";
        case LIT_VALTYP_BOOL:
            return "bool";
        case LIT_VALTYP_NUMBER:
            return "number";
        /* technically never reached */
        case LIT_VALTYP_OBJECT:
            return "object";
    }
    return "?unknown?";
}

const char* lit_value_valtypename(LitValue val)
{
    if(lit_value_isobject(val))
    {
        return lit_value_objtypename(lit_value_asobject(val)->type);
    }
    return lit_value_valtypefromtype(val.type);
}


void* lit_reallocate(LitState* state, void* pointer, size_t oldsize, size_t newsize)
{
    state->bytes_allocated += (int64_t)newsize - (int64_t)oldsize;
    if(newsize > oldsize)
    {
#ifdef LIT_CONFIG_STRESSTESTGC
        lit_collect_garbage(state);
#endif
        if(state->bytes_allocated > state->gcnextgc)
        {
            lit_collect_garbage(state);
        }
    }
    if(newsize == 0)
    {
        lit_sysmem_free(pointer);
        return NULL;
    }
    void* ptr = lit_sysmem_realloc(pointer, newsize);
    if(ptr == NULL)
    {
        lit_state_raiseerror(state, LIT_ERROR_RUNTIMEERROR, "fatal error: out of memory! aborting now\n");
        abort();
    }
    return ptr;
}

void lit_free_object(LitState* state, LitObject* object)
{
#ifdef LIT_CONFIG_LOGALLOCATION
    fprintf(stderr, "(%s) %p free %s\n", lit_value_objtypename(object->type), (void*)object, lit_value_objtypename(object->type));
#endif
    switch(object->type)
    {
        case LIT_OBJ_STRING:
        {
            LitString* string = (LitString*)object;
            lit_strbuf_destroyfromstack(&string->strbuf);
            LIT_GC_FREEOBJECT(state, LitString, object);
            break;
        }
        case LIT_OBJ_FUNCSCRIPT:
        {
            LitFuncScript* function = (LitFuncScript*)object;
            lit_chunk_destroy(&function->chunk);
            LIT_GC_FREEOBJECT(state, LitFuncScript, object);
            break;
        }
        case LIT_OBJ_FUNCNATIVE:
        {
            LIT_GC_FREEOBJECT(state, LitFuncNative, object);
            break;
        }
        case LIT_OBJ_FUNCNATMETHOD:
        {
            LIT_GC_FREEOBJECT(state, LitFuncNative, object);
            break;
        }
        case LIT_OBJ_FIBER:
        {
            LitFiber* fiber = (LitFiber*)object;
            lit_sysmem_free(fiber->framevals);
            lit_sysmem_free(fiber->registeritems);
            if(fiber->handlers != NULL)
            {
                lit_sysmem_free(fiber->handlers);
            }
            LIT_GC_FREEOBJECT(state, LitFiber, object);
            break;
        }
        case LIT_OBJ_MODULE:
        {
            LitModule* module = (LitModule*)object;
            lit_sysmem_free(module->privatevalues);
            LIT_GC_FREEOBJECT(state, LitModule, object);
            break;
        }
        case LIT_OBJ_FUNCCLOSURE:
        {
            LitFuncClosure* closure = (LitFuncClosure*)object;
            lit_sysmem_free(closure->upvalues);
            LIT_GC_FREEOBJECT(state, LitFuncClosure, object);
            break;
        }
        case LIT_OBJ_CLSPROTOTYPE:
        {
            LitClsPrototype* clsproto = (LitClsPrototype*)object;
            lit_sysmem_free(clsproto->indexes);
            lit_sysmem_free(clsproto->local);
            LIT_GC_FREEOBJECT(state, LitClsPrototype, object);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            LIT_GC_FREEOBJECT(state, LitUpvalue, object);
            break;
        }
        case LIT_OBJ_CLASS:
        {
            LitClass* klass = (LitClass*)object;
            lit_free_table(&klass->mthtable);
            lit_free_table(&klass->staticstable);
            LIT_GC_FREEOBJECT(state, LitClass, object);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            lit_free_table(&((LitInstance*)object)->fields);
            LIT_GC_FREEOBJECT(state, LitInstance, object);
            break;
        }
        case LIT_OBJ_FUNCBOUNDMETHOD:
        {
            LIT_GC_FREEOBJECT(state, LitFuncBound, object);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            lit_dynlistval_destroy(&((LitArray*)object)->innerlist);
            LIT_GC_FREEOBJECT(state, LitArray, object);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            lit_dynlistval_destroy(&((LitVarargArray*)object)->innerarray.innerlist);
            LIT_GC_FREEOBJECT(state, LitVarargArray, object);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_free_table(&((LitMap*)object)->innertable);
            LIT_GC_FREEOBJECT(state, LitMap, object);
            break;
        }
        case LIT_OBJ_USERDATA:
        {
            LitUserdata* data = (LitUserdata*)object;
            if(data->oncleanupfn != NULL)
            {
                data->oncleanupfn(state, data, false);
            }
            if(data->size > 0)
            {
                lit_sysmem_free(data->data);
            }
            LIT_GC_FREEOBJECT(state, LitUserdata, object);
            break;
        }
        case LIT_OBJ_RANGE:
        {
            LIT_GC_FREEOBJECT(state, LitRange, object);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LIT_GC_FREEOBJECT(state, LitField, object);
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            LIT_GC_FREEOBJECT(state, LitReference, object);
            break;
        }
        default:
        {
            UNREACHABLE
        }
    }
}

void lit_free_objects(LitState* state, LitObject* objects)
{
    LitObject* object = objects;
    while(object != NULL)
    {
        LitObject* next = object->next;
        lit_free_object(state, object);
        object = next;
    }
    lit_sysmem_free(state->vmstate.gcgraystack);
    state->vmstate.gcgraycapacity = 0;
}

void lit_gcmem_markobject(LitState* state, LitObject* object)
{
    if(object == NULL || object->marked)
    {
        return;
    }
    object->marked = true;
#ifdef LIT_CONFIG_LOGMARKING
    fprintf(stderr, "%p mark ", (void*)object);
    lit_value_printvalue(state->streamstderr, lit_value_fromobject(object), true);
    fprintf(stderr, "\n");
#endif
    if(state->vmstate.gcgraycapacity < state->vmstate.gcgraycount + 1)
    {
        state->vmstate.gcgraycapacity = LIT_GROW_CAPACITY(state->vmstate.gcgraycapacity);
        state->vmstate.gcgraystack = (LitObject**)lit_sysmem_realloc(state->vmstate.gcgraystack, sizeof(LitObject*) * state->vmstate.gcgraycapacity);
    }
    state->vmstate.gcgraystack[state->vmstate.gcgraycount++] = object;
}

void lit_gcmem_markvalue(LitState* state, LitValue value)
{
    if(lit_value_isobject(value))
    {
        lit_gcmem_markobject(state, lit_value_asobject(value));
    }
}

void lit_gcmem_markroots(LitState* state)
{
    size_t i;
    for(i = 0; i < state->root_count; i++)
    {
        lit_gcmem_markvalue(state, state->roots[i]);
    }
    lit_gcmem_markobject(state, (LitObject*)state->vmstate.fiber);
    lit_gcmem_markobject(state, (LitObject*)state->stdclassclass);
    lit_gcmem_markobject(state, (LitObject*)state->stdobjectclass);
    lit_gcmem_markobject(state, (LitObject*)state->number_class);
    lit_gcmem_markobject(state, (LitObject*)state->string_class);
    lit_gcmem_markobject(state, (LitObject*)state->bool_class);
    lit_gcmem_markobject(state, (LitObject*)state->function_class);
    lit_gcmem_markobject(state, (LitObject*)state->fiber_class);
    lit_gcmem_markobject(state, (LitObject*)state->module_class);
    lit_gcmem_markobject(state, (LitObject*)state->array_class);
    lit_gcmem_markobject(state, (LitObject*)state->map_class);
    lit_gcmem_markobject(state, (LitObject*)state->range_class);
    lit_gcmem_markobject(state, (LitObject*)state->apiname);
    lit_gcmem_markobject(state, (LitObject*)state->apifunction);
    lit_table_markentries(&state->vmstate.modules->innertable);
    lit_table_markentries(&state->vmstate.globals->innertable);
}

void lit_gcmem_markarray(LitState* state, LitDynListVal* list)
{
    size_t i;
    for(i = 0; i < list->listcount; i++)
    {
        lit_gcmem_markvalue(state, list->listitems[i]);
    }
}

void lit_gcmem_blackenobject(LitState* state, LitObject* object)
{
#ifdef LIT_CONFIG_LOGBLACKING
    fprintf(stderr, "%p blacken ", (void*)object);
    lit_value_printvalue(state->streamstderr, lit_value_fromobject(object), true);
    fprintf(stderr, "\n");
#endif
    switch(object->type)
    {
        case LIT_OBJ_FUNCNATIVE:
        case LIT_OBJ_FUNCNATMETHOD:
        case LIT_OBJ_RANGE:
        case LIT_OBJ_STRING:
        {
            break;
        }
        case LIT_OBJ_USERDATA:
        {
            LitUserdata* data = (LitUserdata*)object;
            if(data->oncleanupfn != NULL)
            {
                data->oncleanupfn(state, data, true);
            }
            break;
        }
        case LIT_OBJ_FUNCSCRIPT:
        {
            LitFuncScript* function = (LitFuncScript*)object;
            lit_gcmem_markobject(state, (LitObject*)function->name);
            lit_gcmem_markarray(state, &function->chunk.constantlist);
            break;
        }
        case LIT_OBJ_FIBER:
        {
            size_t i;
            LitFiber* fiber;
            LitUpvalue* upvalue;
            fiber = (LitFiber*)object;
            for(i = 0; i < fiber->registersallocated; i++)
            {
                lit_gcmem_markvalue(state, fiber->registeritems[i]);
            }
            for(i = 0; i < fiber->framecount; i++)
            {
                LitCallFrame* frame = &fiber->framevals[i];
                if(frame->closure != NULL)
                {
                    lit_gcmem_markobject(state, (LitObject*)frame->closure);
                }
                else
                {
                    lit_gcmem_markobject(state, (LitObject*)frame->function);
                }
            }
            for(upvalue = fiber->openupvalues; upvalue != NULL; upvalue = upvalue->next)
            {
                lit_gcmem_markobject(state, (LitObject*)upvalue);
            }
            lit_gcmem_markvalue(state, fiber->error);
            lit_gcmem_markobject(state, (LitObject*)fiber->module);
            lit_gcmem_markobject(state, (LitObject*)fiber->parent);
            break;
        }
        case LIT_OBJ_MODULE:
        {
            size_t i;
            LitModule* module;
            module = (LitModule*)object;
            lit_gcmem_markvalue(state, module->returnvalue);
            lit_gcmem_markobject(state, (LitObject*)module->name);
            lit_gcmem_markobject(state, (LitObject*)module->mainfunction);
            lit_gcmem_markobject(state, (LitObject*)module->mainfiber);
            lit_gcmem_markobject(state, (LitObject*)module->privatenames);
            for(i = 0; i < module->privatecount; i++)
            {
                lit_gcmem_markvalue(state, module->privatevalues[i]);
            }
            break;
        }
        case LIT_OBJ_FUNCCLOSURE:
        {
            size_t i;
            LitFuncClosure* closure = (LitFuncClosure*)object;
            lit_gcmem_markobject(state, (LitObject*)closure->function);
            /* Check for NULL is needed for a really specific gc-case */
            if(closure->upvalues != NULL)
            {
                for(i = 0; i < closure->upvaluecount; i++)
                {
                    lit_gcmem_markobject(state, (LitObject*)closure->upvalues[i]);
                }
            }
            break;
        }
        case LIT_OBJ_CLSPROTOTYPE:
        {
            lit_gcmem_markobject(state, (LitObject*)((LitFuncClosure*)object)->function);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            lit_gcmem_markvalue(state, ((LitUpvalue*)object)->closed);
            break;
        }
        case LIT_OBJ_CLASS:
        {
            LitClass* klass = (LitClass*)object;
            lit_gcmem_markobject(state, (LitObject*)klass->name);
            lit_gcmem_markobject(state, (LitObject*)klass->super);
            lit_table_markentries(&klass->mthtable);
            lit_table_markentries(&klass->staticstable);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            LitInstance* instance = (LitInstance*)object;
            lit_gcmem_markobject(state, (LitObject*)instance->klass);
            lit_table_markentries(&instance->fields);
            break;
        }
        case LIT_OBJ_FUNCBOUNDMETHOD:
        {
            LitFuncBound* boundmethod = (LitFuncBound*)object;
            lit_gcmem_markvalue(state, boundmethod->receiver);
            lit_gcmem_markvalue(state, boundmethod->method);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            lit_gcmem_markarray(state, &((LitArray*)object)->innerlist);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            lit_gcmem_markarray(state, &((LitVarargArray*)object)->innerarray.innerlist);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_table_markentries(&((LitMap*)object)->innertable);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = (LitField*)object;
            lit_gcmem_markobject(state, (LitObject*)field->getter);
            lit_gcmem_markobject(state, (LitObject*)field->setter);
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            lit_gcmem_markvalue(state, *((LitReference*)object)->slot);
            break;
        }
        default:
        {
            lit_vm_raisefatalerror(state, "unknown object with type %i", object->type);
            break;
        }
    }
}

void lit_gcmem_tracereferences(LitState* state)
{
    while(state->vmstate.gcgraycount > 0)
    {
        LitObject* object = state->vmstate.gcgraystack[--state->vmstate.gcgraycount];
        lit_gcmem_blackenobject(state, object);
    }
}

void lit_gcmem_sweep(LitState* state)
{
    LitObject* previous = NULL;
    LitObject* object = state->vmstate.objects;
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
            LitObject* unreached = object;
            object = object->next;
            if(previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                state->vmstate.objects = object;
            }
            lit_free_object(state, unreached);
        }
    }
}

uint64_t lit_collect_garbage(LitState* state)
{
    if(!state->gcallowgc)
    {
        return 0;
    }
    state->gcallowgc = false;
    uint64_t before = state->bytes_allocated;
#ifdef LIT_CONFIG_LOGGC
    fprintf(stderr, "-- gc begin\n");
    clock_t t = clock();
#endif
    lit_gcmem_markroots(state);
    lit_gcmem_tracereferences(state);
    lit_table_removewhite(&state->vmstate.strings);
    lit_gcmem_sweep(state);
    state->gcnextgc = state->bytes_allocated * LIT_GC_HEAP_GROW_FACTOR;
    state->gcallowgc = true;
    uint64_t collected = before - state->bytes_allocated;
#ifdef LIT_CONFIG_LOGGC
    fprintf(stderr, "-- gc end. Collected %imb (%ib) in %gms\n", ((int)((collected / 1024.0 + 0.5) / 10)) * 10, collected, (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
#endif
    return collected;
}



LitString* lit_string_makewithstrbuf(LitState* state, LitStrBuffer sb)
{
    LitString* string = (LitString*)lit_object_allocobject(state, sizeof(LitString), LIT_OBJ_STRING);
    string->strbuf = sb;
    return string;
}

LitString* lit_string_makeemptystring(LitState* state, size_t length, bool preallocated)
{
    LitStrBuffer sb;
    lit_strbuf_initbasicempty(&sb, length, false, preallocated);
    return lit_string_makewithstrbuf(state, sb);
}

void lit_string_register(LitState* state, LitString* string)
{
    lit_state_pushroot(state, (LitObject*)string);
    lit_table_set(&state->vmstate.strings, string, lit_value_makenull());
    lit_state_poproot(state);
}

LitString* lit_string_makestringfrom(LitState* state, char* chars, size_t length, uint32_t hash, bool preallocated)
{
    LitString* string;
    string = lit_string_makeemptystring(state, length, preallocated);
    if(preallocated)
    {
        lit_strbuf_setdata(&string->strbuf, chars);
        lit_strbuf_setlength(&string->strbuf, length);
    }
    else
    {
        lit_strbuf_appendstrn(&string->strbuf, chars, length);
    }
    string->strhash = hash;
    lit_string_register(state, string);
    return string;
}

uint32_t lit_string_hash(const char* key, size_t length)
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

LitString* lit_string_take(LitState* state, const char* chars, size_t length)
{
    uint32_t hash = lit_string_hash(chars, length);
    LitString* interned = lit_table_findstring(&state->vmstate.strings, chars, length, hash);
    if(interned != NULL)
    {
        return interned;
    }
    return lit_string_makestringfrom(state, (char*)chars, length, hash, true);
}

#define lit_string_copylen(state, chars, length) \
    lit_string_copylen_real(state, chars, length, __FILE__, __LINE__)

#define lit_string_copy(state, chars) \
    lit_string_copy_real(state, chars, __FILE__, __LINE__)

LitString* lit_string_copylen_real(LitState* state, const char* chars, size_t length, const char* srcfilename, int srcline)
{
    uint32_t hash;
    char* heapchars;
    LitString* res;
    if(chars == NULL)
    {
        fprintf(stderr, "***ERROR at %s:%d: chars are NULL!*****\n", srcfilename, srcline);
        abort();
    }
    hash = lit_string_hash(chars, length);
    res = lit_table_findstring(&state->vmstate.strings, chars, length, hash);
    if(res != NULL)
    {
        return res;
    }
    heapchars = (char*)lit_sysmem_malloc((length + 1) * sizeof(char));
    memcpy(heapchars, chars, length);
    heapchars[length] = '\0';
#ifdef LIT_CONFIG_LOGALLOCATION
    printf("allocated new string '%s'\n", chars);
#endif
    return lit_string_makestringfrom(state, heapchars, length, hash, true);
}

LitString* lit_string_copy_real(LitState* state, const char* chars, const char* srcfilename, int srcline)
{
    return lit_string_copylen_real(state, chars, strlen(chars), srcfilename, srcline);
}

size_t lit_string_getlength(LitString* string)
{
    return string->strbuf.length;
}

char* lit_string_getdata(LitString* string)
{
    return string->strbuf.data;
}

LitString* lit_string_clone(LitState* state, LitString* string)
{
    size_t slen;
    const char* sdata;
    slen = string->strbuf.length;
    sdata = string->strbuf.data;
    return lit_string_copylen(state, sdata, slen);
}

size_t lit_string_utflength(LitString* string)
{
    size_t i;
    size_t length;
    length = 0;
    for(i=0; i < string->strbuf.length;)
    {
        i += lit_util_stringutfgetcountdecode(string->strbuf.data[i]);
        length++;
    }
    return length;
}

int lit_string_codepointcodeat(LitState* state, LitString* string, uint32_t index)
{
    int codepoint;
    (void)state;
    if(index >= string->strbuf.length)
    {
        return 0;
    }
    codepoint = lit_util_stringutfdecode((uint8_t*)string->strbuf.data + index, string->strbuf.length - index);
    if(codepoint == -1)
    {
        return string->strbuf.data[index];
    }
    return codepoint;
}

LitString* lit_string_codepointstringat(LitState* state, LitString* string, uint32_t index)
{
    if(index >= string->strbuf.length)
    {
        return NULL;
    }
    int codepoint = lit_util_stringutfdecode((uint8_t*)string->strbuf.data + index, string->strbuf.length - index);
    if(codepoint == -1)
    {
        char bytes[2];
        bytes[0] = string->strbuf.data[index];
        bytes[1] = '\0';
        return lit_string_copylen(state, bytes, 1);
    }
    return lit_string_fromcodepoint(state, codepoint);
}


LitString* lit_string_fromcodepoint(LitState* state, int value)
{
    int length;
    LitString* res;
    length = lit_util_stringutfgetcountencode(value);
    res = lit_string_makeemptystring(state, length, false);
    lit_util_stringutfencode(value, (uint8_t*)res->strbuf.data);
    res->strbuf.length = length;
    return res;
}

LitString* lit_string_fromrange(LitState* state, LitString* source, int start, uint32_t count)
{
    int index;
    int length;
    int codepoint;
    uint32_t i;
    uint8_t* to;
    uint8_t* from;
    LitString* res;
    from = (uint8_t*)source->strbuf.data;
    length = 0;
    for(i = 0; i < count; i++)
    {
        length += lit_util_stringutfgetcountdecode(from[start + i]);
    }
    res = lit_string_makeemptystring(state, length, false);
    to = (uint8_t*)res->strbuf.data;
    for(i = 0; i < count; i++)
    {
        index = start + i;
        codepoint = lit_util_stringutfdecode(from + index, source->strbuf.length - index);
        if(codepoint != -1)
        {
            to += lit_util_stringutfencode(codepoint, to);
        }
    }
    res->strbuf.length = length;
    return res;
}

void lit_string_appendlen(LitString* dest, const char* str, size_t len)
{
    lit_strbuf_appendstrn(&dest->strbuf, str, len);
}

void lit_string_append(LitString* dest, const char* str)
{
    return lit_string_appendlen(dest, str, strlen(str));
}

void lit_string_appendbyte(LitString* dest, int b)
{
    char c;
    c = b;
    return lit_string_appendlen(dest, &c, 1);
}

LitValue lit_string_numbertostring(LitState* state, double value)
{
    LitIOStream pr;
    lit_iostream_makestackstring(&pr);
    lit_value_printnumber(&pr, value);
    return lit_value_fromobject(lit_iostream_takestring(state, &pr));
}

LitValue lit_string_valformat(LitState* state, const char* format, ...)
{
    va_list arglist;    
    bool wasallowed;
    const char* c;
    LitString* result;
    wasallowed = state->gcallowgc;
    state->gcallowgc = false;
    result = lit_string_makeemptystring(state, 10, false);
    va_start(arglist, format);
    for(c = format; *c != '\0'; c++)
    {
        switch(*c)
        {
            case '$':
            {
                const char* string = va_arg(arglist, const char*);
                if(string != NULL)
                {
                    size_t length = strlen(string);
                    lit_string_appendlen(result, string, length);
                    break;
                }
                goto defaultendingcopying;
            }
            case '@':
            {
                LitString* string = lit_value_asstring(va_arg(arglist, LitValue));
                if(string != NULL)
                {
                    lit_string_appendlen(result, string->strbuf.data, string->strbuf.length);
                    break;
                }
                goto defaultendingcopying;
            }
            case '#':
            {
                LitString* string = lit_value_asstring(lit_string_numbertostring(state, va_arg(arglist, double)));
                lit_string_appendlen(result, string->strbuf.data, string->strbuf.length);
                break;
            }
            default:
            {
            defaultendingcopying:
                lit_string_appendbyte(result, *c);
                break;
            }
        }
    }
    va_end(arglist);
    result->strhash = lit_string_hash(result->strbuf.data, result->strbuf.length);
    lit_string_register(state, result);
    state->gcallowgc = wasallowed;
    return lit_value_fromobject(result);
}


void lit_value_printobjtable(LitIOStream* pr, LitObject* self, LitTable* tab)
{
    bool didprint;
    size_t i;
    size_t index;
    size_t valueamount;
    LitValue field;
    LitTabEntry* entry;
    valueamount = tab->htcount;
    lit_iostream_puts(pr, "{");
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
                lit_iostream_putlen(pr, entry->entkey->strbuf.data, entry->entkey->strbuf.length);
                lit_iostream_puts(pr, ": ");
                if((lit_value_ismap(field) && (lit_value_asobject(field) == self)))
                {
                    lit_iostream_puts(pr, "<recursion>");
                }
                else
                {
                    lit_value_printvalue(pr, field, true);
                }
                i++;
                didprint = true;
            }
            if(didprint)
            {
                if((i + 0) < valueamount)
                {
                    lit_iostream_puts(pr, ",");
                }
            }
        } while(i < valueamount);
    }
    lit_iostream_puts(pr, "}");
}

void lit_value_printobjinstance(LitIOStream* pr, LitClass* klass, LitInstance* self)
{
    LitString* sr;
    LitState* state;
    (void)state;
    state = ((LitObject*)self)->pstate;
    sr = NULL;
    #if 0
    sr = lit_value_tostrinvoketostring(state, lit_value_fromobject(self), 2, false);
    #endif
    if(sr != NULL)
    {
        lit_iostream_putlen(pr, lit_string_getdata(sr), lit_string_getlength(sr));
    }
    else
    {
        lit_iostream_printf(pr, "<instance of %s: ", klass->name->strbuf.data);
        lit_value_printobjtable(pr, (LitObject*)self, &self->fields);
        lit_iostream_printf(pr, "  >");
    }
}

void lit_value_printobjarray(LitIOStream* pr, LitArray* self)
{
    size_t i;
    size_t valueamount;
    LitValue field;
    LitDynListVal* vdlist;
    valueamount = self->innerlist.listcount;
    vdlist = &self->innerlist;
    lit_iostream_puts(pr, "[");
    if(vdlist->listcount > 0)
    {
        for(i = 0; i < valueamount; i++)
        {
            field = vdlist->listitems[i];
            if(lit_value_isarray(field) && lit_value_asarray(field) == self)
            {
                lit_iostream_puts(pr, "<recursion>");
            }
            else
            {
                lit_value_printvalue(pr, field, true);
            }
            if((i + 1) < valueamount)
            {
                lit_iostream_puts(pr, ", ");
            }
        }
    }
    lit_iostream_puts(pr, "]");
}

void lit_value_printobject(LitIOStream* pr, LitValue value, bool reprmode)
{
    LitState* state;
    LitObject* object;
    (void)state;
    object = lit_value_asobject(value);
    state = object->pstate;
    switch(object->type)
    {
        case LIT_OBJ_STRING:
            {
                LitString* str;
                str = lit_value_asstring(value);
                if(reprmode)
                {
                    lit_iostream_putquotedstring(pr, str->strbuf.data, str->strbuf.length, true);
                }
                else
                {
                    lit_iostream_putlen(pr, str->strbuf.data, str->strbuf.length);
                }
            }
            break;
        case LIT_OBJ_FUNCSCRIPT:
            {
                LitFuncScript* fn;
                fn = lit_value_asfuncscript(value);
                lit_iostream_printf(pr, "function %s", fn->name->strbuf.data);
            }
            break;
        case LIT_OBJ_FUNCCLOSURE:
            {
                LitFuncClosure* fn;
                fn = lit_value_asfuncclosure(value);
                lit_iostream_printf(pr, "closure %s", fn->function->name->strbuf.data);
            }
            break;
        case LIT_OBJ_CLSPROTOTYPE:
            {
                LitClsPrototype* fnprot;
                fnprot = lit_value_asclsproto(value);
                lit_iostream_printf(pr, "closure %s", fnprot->function->name->strbuf.data);
            }
            break;
        case LIT_OBJ_FUNCNATIVE:
            {
                LitFuncNative* fn;
                fn = lit_value_asfuncnative(value);
                lit_iostream_printf(pr, "function %s", fn->name->strbuf.data);
            }
            break;
        case LIT_OBJ_FUNCNATMETHOD:
            {
                LitFuncNative* fn;
                fn = lit_value_asfuncmethod(value);
                lit_iostream_printf(pr, "function %s", fn->name->strbuf.data);
            }
            break;
        case LIT_OBJ_FIBER:
            {
                LitFiber* fiber;
                (void)fiber;
                fiber = lit_value_asfiber(value);
                lit_iostream_printf(pr, "<fiber>");
            }
            break;
        case LIT_OBJ_MODULE:
            {
                LitModule* mod;
                mod = lit_value_asmodule(value);
                lit_iostream_printf(pr, "<module %s>", mod->name->strbuf.data);
            }
            break;
        case LIT_OBJ_UPVALUE:
            {
                LitUpvalue* upvalue;
                upvalue = lit_value_asupvalue(value);
                lit_iostream_puts(pr, "<upvalue to ");
                if(upvalue->location == NULL)
                {
                    lit_iostream_puts(pr, " closed ");
                    lit_value_printvalue(pr, upvalue->closed, reprmode);
                }
                else
                {
                    lit_iostream_puts(pr, " location ");
                    lit_value_printobject(pr, *upvalue->location, reprmode);
                }
                lit_iostream_puts(pr, ">");
            }
            break;
        case LIT_OBJ_CLASS:
            {
                LitClass* klass;
                klass = lit_value_asclass(value);
                lit_iostream_printf(pr, "<class %s>", lit_string_getdata(klass->name));
            }
            break;
        case LIT_OBJ_INSTANCE:
            {
                LitInstance* inst;
                inst = lit_value_asinstance(value);
                lit_value_printobjinstance(pr, inst->klass, inst);
            }
            break;
        case LIT_OBJ_FUNCBOUNDMETHOD:
            {
                LitFuncBound* fn;
                fn = lit_value_asfuncboundmethod(value);
                lit_value_printvalue(pr, fn->method, reprmode);
            }
            break;
        case LIT_OBJ_VARARGARRAY:
        case LIT_OBJ_ARRAY:
            {
                LitArray* array = lit_value_asarray(value);
                lit_value_printobjarray(pr, array);
            }
            break;
        case LIT_OBJ_MAP:
            {
                LitMap* map;
                map = lit_value_asmap(value);
                lit_value_printobjtable(pr, (LitObject*)map, &map->innertable);
            }
            break;

        case LIT_OBJ_USERDATA:
            {
                LitUserdata* ud;
                (void)ud;
                ud = lit_value_asuserdata(value);
                lit_iostream_printf(pr, "<userdata>");
            }
            break;
        case LIT_OBJ_RANGE:
            {
                LitRange* range;
                range = lit_value_asrange(value);
                lit_iostream_printf(pr, "%g .. %g", range->from, range->to);
            }
            break;
        case LIT_OBJ_FIELD:
            {
                LitField* field;
                field = lit_value_asfield(value);
                lit_iostream_printf(pr, "<field ");
                lit_value_printvalue(pr, lit_value_fromobject(&field->innerobject), true);
                if(field->getter != NULL)
                {
                    lit_iostream_puts(pr, " getter=");
                    lit_value_printvalue(pr, lit_value_fromobject(field->getter), true);
                }
                if(field->setter != NULL)
                {
                    lit_iostream_puts(pr, " setter=");
                    lit_value_printvalue(pr, lit_value_fromobject(field->getter), true);
                }
                lit_iostream_puts(pr, ">");
            }
            break;
        case LIT_OBJ_REFERENCE:
            {
                lit_iostream_printf(pr, "<reference to ");
                LitValue* slot = lit_value_asreference(value)->slot;
                if(slot == NULL)
                {
                    lit_iostream_printf(pr, "null");
                }
                else
                {
                    lit_value_printvalue(pr, *slot, true);
                }
                lit_iostream_puts(pr, ">");
            }
            break;
        default:
            {
                lit_iostream_printf(pr, "[unknown object %p %i]", &value, lit_value_objtype(value));
            }
            break;
    }
}

void lit_value_printnumber(LitIOStream* pr, double dn)
{
    if(isnan(dn))
    {
        lit_iostream_puts(pr, "nan");
        return;
    }
    if(isinf(dn))
    {
        if(dn > 0.0)
        {
            lit_iostream_puts(pr, "infinity");
            return;
        }
        else
        {
            lit_iostream_puts(pr, "-infinity");
            return;
        }
    }
    if(((int64_t)dn) == dn)
    {
        lit_iostream_printf(pr, "%ld", (int64_t)dn);
    }
    else
    {
        lit_iostream_printf(pr, "%g", dn);
    }
}

void lit_value_printvalue(LitIOStream* pr, LitValue value, bool reprmode)
{
    if(lit_value_isbool(value))
    {
        lit_iostream_printf(pr, lit_value_asbool(value) ? "true" : "false");
    }
    else if(lit_value_isnull(value))
    {
        lit_iostream_printf(pr, "null");
    }
    else if(lit_value_isnumber(value))
    {
        lit_value_printnumber(pr, lit_value_asnumber(value));
    }
    else if(lit_value_isobject(value))
    {
        lit_value_printobject(pr, value, reprmode);
    }
    else
    {
        lit_iostream_printf(pr, "[unknown value %p (%d <%s>)]", &value, value.type, lit_value_valtypename(value));
    }
}



LitClass* lit_class_make(LitState* state, const char* name, LitClass* super)
{
    LitClass* klass;
    klass = lit_object_makeclass(state, lit_string_copylen(state, name, strlen(name)));
    lit_state_setglobal(state, klass->name, lit_value_fromobject(klass));
    if(super != NULL)
    {
        lit_class_inherit(klass, super);
    }
    return klass;
}

void lit_class_bindstaticgetter(LitClass* selfclass, const char* name, LitNativeFunctionFn getter)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->staticstable, nm, lit_value_fromobject(lit_object_makefield(state, (LitObject*)lit_object_makenativemethod(state, getter, nm), NULL)));
}

void lit_class_inherit(LitClass* selfclass, LitClass* other)
{
    selfclass->super = (LitClass*)other;
    if(selfclass->mthconstructor == NULL)
    {
        selfclass->mthconstructor = other->mthconstructor;
    }
    lit_table_addallignoring(&other->mthtable, &selfclass->mthtable);
    lit_table_addallignoring(&other->staticstable, &selfclass->staticstable);
}

void lit_class_bindmethod(LitClass* selfclass, const char* name, LitNativeFunctionFn fn)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->mthtable, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}


void lit_class_bindconstructor(LitClass* selfclass, LitNativeFunctionFn fn)
{
    LitFuncNative* meth;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    meth = lit_object_makenativemethod(state, fn, state->strings.strconstructor);
    selfclass->mthconstructor = (LitObject*)meth;
    lit_table_set(&selfclass->mthtable, state->strings.strconstructor, lit_value_fromobject(meth));
}

void lit_class_bindstaticmethod(LitClass* selfclass, const char* name, LitNativeFunctionFn fn)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->staticstable, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}

void lit_class_setstaticfield(LitClass* selfclass, const char* name, LitValue val)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->staticstable, nm, val);
}

void lit_class_bindgetsetter(LitClass* selfclass, const char* name, LitNativeFunctionFn fnget, LitNativeFunctionFn fnset)
{
    LitString* nm;
    LitObject* mthset;
    LitObject* mthget;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    mthset = NULL;
    mthget = NULL;
    nm = lit_string_copylen(state, name, strlen(name));
    if(fnget)
    {
        mthget = (LitObject*)lit_object_makenativemethod(state, fnget, nm);
    }
    if(fnset)
    {
        mthset = (LitObject*)lit_object_makenativemethod(state, fnset, nm);
    }
    lit_table_set(&selfclass->mthtable, nm, lit_value_fromobject(lit_object_makefield(state, mthget, mthset)));
}

LitValue lit_instance_getthis(LitInstance* selfinst)
{
    LitValue res;
    LitState* state;
    state = ((LitObject*)selfinst)->pstate;
    if(lit_table_getentry(&selfinst->fields, state->strings.strthis, &res))
    {
        return res;
    }
    return lit_value_makenull();
}

void lit_api_init(LitState* state)
{
    state->apiname = lit_string_copylen(state, "c", 1);
    state->apifunction = NULL;
}

void lit_api_destroy(LitState* state)
{
    state->apiname = NULL;
    state->apifunction = NULL;
}

bool lit_state_getglobaltovalue(LitState* state, LitString* name, LitValue* dest)
{
    return lit_map_get(state->vmstate.globals, name, dest);
}

bool lit_state_globalexists(LitState* state, LitString* name)
{
    LitValue val;
    return lit_state_getglobaltovalue(state, name, &val);
}

LitValue lit_state_getglobal(LitState* state, LitString* name)
{
    LitValue global;
    if(!lit_state_getglobaltovalue(state, name, &global))
    {
        return lit_value_makenull();
    }
    return global;
}

LitFuncScript* lit_state_getglobalfunction(LitState* state, LitString* name)
{
    LitValue val;
    if(!lit_state_getglobaltovalue(state, name, &val))
    {
        return NULL;
    }
    if(lit_value_isfuncscript(val))
    {
        return lit_value_asfuncscript(val);
    }
    return NULL;
}

void lit_state_setglobal(LitState* state, LitString* name, LitValue value)
{
    lit_state_pushroot(state, (LitObject*)name);
    lit_state_pushvalueroot(state, value);
    lit_map_set(state->vmstate.globals, name, value);
    lit_state_poproots(state, 2);
}

void lit_state_defnative(LitState* state, const char* name, LitNativeFunctionFn native)
{
    lit_state_pushroot(state, (LitObject*)lit_string_copy(state, name));
    lit_state_pushroot(state, (LitObject*)lit_object_makenativefunc(state, native, lit_value_asstring(lit_state_peekroot(state, 0))));
    lit_map_set(state->vmstate.globals, lit_value_asstring(lit_state_peekroot(state, 1)), lit_state_peekroot(state, 0));
    lit_state_poproots(state, 2);
}


double lit_args_checknumber(LitState* state, const char* sourcefname, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !lit_value_isnumber(args[id]))
    {
        lit_vm_raisefatalerror(state, "in %s: Expected a number as argument #%i, got a %s", sourcefname, (int)id, id >= argc ? "null" : lit_value_valtypename(args[id]));
    }
    return lit_value_asnumber(args[id]);
}

const char* lit_args_checkstring(LitState* state, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !lit_value_isstring(args[id]))
    {
        lit_vm_raisefatalerror(state, "expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_value_valtypename(args[id]));
    }
    return lit_value_asstring(args[id])->strbuf.data;
}

const char* lit_args_getstring(LitValue* args, size_t argc, size_t id, const char* def)
{
    if(argc <= id || !lit_value_isstring(args[id]))
    {
        return def;
    }
    return lit_value_asstring(args[id])->strbuf.data;
}

LitString* lit_args_checkobjstring(LitState* state, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !lit_value_isstring(args[id]))
    {
        lit_vm_raisefatalerror(state, "expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_value_valtypename(args[id]));
    }
    return lit_value_asstring(args[id]);
}


void lit_ast_destroyparamlist(LitState* state, LitDynListParam* parameters)
{
    size_t i;
    for(i = 0; i < parameters->listcount; i++)
    {
        lit_ast_destroyexpression(state, parameters->listitems[i].defaultval);
    }
    lit_dynlistparam_destroy(parameters);
}

void lit_ast_destroyexprlist(LitState* state, LitDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->listcount; i++)
    {
        lit_ast_destroyexpression(state, expressions->listitems[i]);
    }
    lit_dynlistexpr_destroy(expressions);
}

void lit_ast_destroyexpression(LitState* state, LitAstExpression* topexpr)
{
    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case LIT_ASTEXPRTYP_LITERAL:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_BINARY:
        {
            LitAstBinaryExpr* expr = (LitAstBinaryExpr*)topexpr;
            if(!expr->ignore_left)
            {
                lit_ast_destroyexpression(state, expr->left);
            }
            lit_ast_destroyexpression(state, expr->right);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_UNARY:
        {
            lit_ast_destroyexpression(state, ((LitAstUnaryExpr*)topexpr)->right);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_VARGET:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_ASSIGN:
        {
            LitAstAssignExpr* expr = (LitAstAssignExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->to);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_CALL:
        {
            LitAstCallExpr* expr = (LitAstCallExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->excallee);
            lit_ast_destroyexpression(state, expr->init);
            lit_ast_destroyexprlist(state, &expr->callargs);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_INDEXGET:
        {
            lit_ast_destroyexpression(state, ((LitAstIndexGetExpr*)topexpr)->where);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_INDEXSET:
        {
            LitAstIndexSetExpr* expr = (LitAstIndexSetExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->where);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_FUNCANON:
        {
            LitAstFunctionExpr* expr = (LitAstFunctionExpr*)topexpr;
            lit_ast_destroyparamlist(state, &expr->parameters);
            lit_ast_destroyexpression(state, expr->body);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_ARRAY:
        {
            lit_ast_destroyexprlist(state, &((LitAstLiteralArrayExpr*)topexpr)->exvalues);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_OBJECT:
        {
            LitAstLiteralObjectExpr* map = (LitAstLiteralObjectExpr*)topexpr;
            lit_dynlistval_destroy(&map->objexkeys);
            lit_ast_destroyexprlist(state, &map->objexvalues);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_SUBSCRIPT:
        {
            LitAstSubscriptExpr* expr = (LitAstSubscriptExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->array);
            lit_ast_destroyexpression(state, expr->index);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_THIS:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_SUPER:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_RANGE:
        {
            LitAstRangeExpr* expr = (LitAstRangeExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->from);
            lit_ast_destroyexpression(state, expr->to);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_TERNARY:
        {
            LitAstTernaryExpr* expr = (LitAstTernaryExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->condition);
            lit_ast_destroyexpression(state, expr->branchif);
            lit_ast_destroyexpression(state, expr->branchelse);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_INTERPOLATION:
        {
            lit_ast_destroyexprlist(state, &((LitAstStrTemplateExpr*)topexpr)->expressions);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_REFERENCE:
        {
            lit_ast_destroyexpression(state, ((LitAstRefExpr*)topexpr)->to);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_EXPRESSION:
        {
            lit_ast_destroyexpression(state, ((LitAstExprStmtExpr*)topexpr)->exvalue);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_BLOCK:
        {
            lit_ast_destroyexprlist(state, &((LitAstBlockExpr*)topexpr)->statements);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_VARDECL:
        {
            lit_ast_destroyexpression(state, ((LitAstVarDeclExpr*)topexpr)->init);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_IF:
        {
            LitAstIfExpr* stmt = (LitAstIfExpr*)topexpr;
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroyexpression(state, stmt->branchif);
            lit_ast_destroyallocatedexprlist(state, stmt->elseifcondlist);
            lit_ast_destroyallocatedstmtlist(state, stmt->branchelseiflist);
            lit_ast_destroyexpression(state, stmt->branchelse);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_WHILE:
        {
            LitAstWhileExpr* stmt = (LitAstWhileExpr*)topexpr;
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroyexpression(state, stmt->body);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_FOR:
        {
            LitAstForExpr* stmt = (LitAstForExpr*)topexpr;
            lit_ast_destroyexpression(state, stmt->increment);
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroyexpression(state, stmt->init);
            lit_ast_destroyexpression(state, stmt->var);
            lit_ast_destroyexpression(state, stmt->body);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_CONTINUE:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_BREAK:
        {
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            LitAstFunctionExpr* stmt = (LitAstFunctionExpr*)topexpr;
            lit_ast_destroyexpression(state, stmt->body);
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_RETURN:
        {
            lit_ast_destroyexpression(state, ((LitAstReturnExpr*)topexpr)->exvalue);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_METHOD:
        {
            LitAstMethodExpr* stmt = (LitAstMethodExpr*)topexpr;
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_ast_destroyexpression(state, stmt->body);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_CLASS:
        {
            lit_ast_destroyexprlist(state, &((LitAstClassExpr*)topexpr)->staticfields);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_FIELD:
        {
            LitAstFieldExpr* expr = (LitAstFieldExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->getter);
            lit_ast_destroyexpression(state, expr->setter);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_TRY:
        {
            LitAstTryExpr* expr = (LitAstTryExpr*)topexpr;
            lit_ast_destroyexpression(state, expr->try_block);
            if(expr->catch_block) lit_ast_destroyexpression(state, expr->catch_block);
            if(expr->finally_block) lit_ast_destroyexpression(state, expr->finally_block);
            lit_sysmem_free(topexpr);
            break;
        }
        case LIT_ASTEXPRTYP_THROW:
        {
            lit_ast_destroyexpression(state, ((LitAstThrowExpr*)topexpr)->exvalue);
            lit_sysmem_free(topexpr);
            break;
        }
        default:
        {
            lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "unknown expression type %d", (int)topexpr->type);
            break;
        }
    }
}

LitAstExpression* lit_ast_allocexpression(uint64_t line, size_t size, LitAstExprType type)
{
    LitAstExpression* object = (LitAstExpression*)lit_sysmem_malloc(size);
    object->type = type;
    object->line = line;
    return object;
}

LitAstLiteralValExpr* lit_ast_makeliteralexpr(size_t line, LitValue value)
{
    LitAstLiteralValExpr* expr = (LitAstLiteralValExpr*)lit_ast_allocexpression(line, sizeof(LitAstLiteralValExpr), LIT_ASTEXPRTYP_LITERAL);
    expr->value = value;
    return expr;
}

LitAstBinaryExpr* lit_ast_makebinaryexpr(size_t line, LitAstExpression* left, LitAstExpression* right, LitAstTokType op)
{
    LitAstBinaryExpr* expr = (LitAstBinaryExpr*)lit_ast_allocexpression(line, sizeof(LitAstBinaryExpr), LIT_ASTEXPRTYP_BINARY);
    expr->left = left;
    expr->right = right;
    expr->op = op;
    expr->ignore_left = false;
    return expr;
}

LitAstUnaryExpr* lit_ast_makeunaryexpr(size_t line, LitAstExpression* right, LitAstTokType op)
{
    LitAstUnaryExpr* expr = (LitAstUnaryExpr*)lit_ast_allocexpression(line, sizeof(LitAstUnaryExpr), LIT_ASTEXPRTYP_UNARY);
    expr->right = right;
    expr->op = op;
    return expr;
}

LitAstAssignExpr* lit_ast_makeassignexpr(size_t line, LitAstExpression* to, LitAstExpression* value)
{
    LitAstAssignExpr* expr = (LitAstAssignExpr*)lit_ast_allocexpression(line, sizeof(LitAstAssignExpr), LIT_ASTEXPRTYP_ASSIGN);
    expr->to = to;
    expr->value = value;
    return expr;
}

LitAstCallExpr* lit_ast_makecallexpr(size_t line, LitAstExpression* callee)
{
    LitAstCallExpr* expr = (LitAstCallExpr*)lit_ast_allocexpression(line, sizeof(LitAstCallExpr), LIT_ASTEXPRTYP_CALL);
    expr->excallee = callee;
    expr->init = NULL;
    lit_dynlistexpr_init(&expr->callargs);
    return expr;
}

LitAstIndexGetExpr* lit_ast_makegetexpr(size_t line, LitAstExpression* where, const char* name, size_t length, bool questionable, bool ignresult)
{
    LitAstIndexGetExpr* expr = (LitAstIndexGetExpr*)lit_ast_allocexpression(line, sizeof(LitAstIndexGetExpr), LIT_ASTEXPRTYP_INDEXGET);
    expr->where = where;
    expr->name = name;
    expr->length = length;
    expr->ignore_emit = false;
    expr->jump = questionable ? 0 : -1;
    expr->ignoreresult = ignresult;
    return expr;
}

LitAstIndexSetExpr* lit_ast_makesetexpr(size_t line, LitAstExpression* where, const char* name, size_t length, LitAstExpression* value)
{
    LitAstIndexSetExpr* expr = (LitAstIndexSetExpr*)lit_ast_allocexpression(line, sizeof(LitAstIndexSetExpr), LIT_ASTEXPRTYP_INDEXSET);
    expr->where = where;
    expr->name = name;
    expr->length = length;
    expr->value = value;
    return expr;
}

LitAstLiteralArrayExpr* lit_ast_makearrayexpr(size_t line)
{
    LitAstLiteralArrayExpr* expr = (LitAstLiteralArrayExpr*)lit_ast_allocexpression(line, sizeof(LitAstLiteralArrayExpr), LIT_ASTEXPRTYP_ARRAY);
    lit_dynlistexpr_init(&expr->exvalues);
    return expr;
}

LitAstLiteralObjectExpr* lit_ast_makeobjectexpr(size_t line)
{
    LitAstLiteralObjectExpr* expr = (LitAstLiteralObjectExpr*)lit_ast_allocexpression(line, sizeof(LitAstLiteralObjectExpr), LIT_ASTEXPRTYP_OBJECT);
    lit_dynlistval_init(&expr->objexkeys);
    lit_dynlistexpr_init(&expr->objexvalues);
    return expr;
}

LitAstSubscriptExpr* lit_ast_makesubscriptexpr(size_t line, LitAstExpression* array, LitAstExpression* index)
{
    LitAstSubscriptExpr* expr = (LitAstSubscriptExpr*)lit_ast_allocexpression(line, sizeof(LitAstSubscriptExpr), LIT_ASTEXPRTYP_SUBSCRIPT);
    expr->array = array;
    expr->index = index;
    return expr;
}

LitAstThisExpr* lit_ast_makethisexpr(size_t line)
{
    return (LitAstThisExpr*)lit_ast_allocexpression(line, sizeof(LitAstThisExpr), LIT_ASTEXPRTYP_THIS);
}

LitAstSuperExpr* lit_ast_makesuperexpr(size_t line, LitString* method, bool ignresult)
{
    LitAstSuperExpr* expr = (LitAstSuperExpr*)lit_ast_allocexpression(line, sizeof(LitAstSuperExpr), LIT_ASTEXPRTYP_SUPER);
    expr->methodname = method;
    expr->ignore_emit = false;
    expr->ignoreresult = ignresult;
    return expr;
}

LitAstRangeExpr* lit_ast_makerangeexpr(size_t line, LitAstExpression* from, LitAstExpression* to)
{
    LitAstRangeExpr* expr = (LitAstRangeExpr*)lit_ast_allocexpression(line, sizeof(LitAstRangeExpr), LIT_ASTEXPRTYP_RANGE);
    expr->from = from;
    expr->to = to;
    return expr;
}

LitAstTernaryExpr* lit_ast_maketernaryexpr(size_t line, LitAstExpression* condition, LitAstExpression* if_branch, LitAstExpression* else_branch)
{
    LitAstTernaryExpr* expr = (LitAstTernaryExpr*)lit_ast_allocexpression(line, sizeof(LitAstTernaryExpr), LIT_ASTEXPRTYP_TERNARY);
    expr->condition = condition;
    expr->branchif = if_branch;
    expr->branchelse = else_branch;
    return expr;
}

LitAstStrTemplateExpr* lit_ast_makeinterpolationexpr(size_t line)
{
    LitAstStrTemplateExpr* expr = (LitAstStrTemplateExpr*)lit_ast_allocexpression(line, sizeof(LitAstStrTemplateExpr), LIT_ASTEXPRTYP_INTERPOLATION);
    lit_dynlistexpr_init(&expr->expressions);
    return expr;
}

LitAstRefExpr* lit_ast_makerefexpr(size_t line, LitAstExpression* to)
{
    LitAstRefExpr* expr = (LitAstRefExpr*)lit_ast_allocexpression(line, sizeof(LitAstRefExpr), LIT_ASTEXPRTYP_REFERENCE);
    expr->to = to;
    return expr;
}


LitAstExprStmtExpr* lit_ast_makeexprstmt(size_t line, LitAstExpression* exv)
{
    LitAstExprStmtExpr* expr = (LitAstExprStmtExpr*)lit_ast_allocexpression(line, sizeof(LitAstExprStmtExpr), LIT_ASTEXPRTYP_EXPRESSION);
    expr->exvalue = exv;
    return expr;
}

LitAstBlockExpr* lit_ast_makeblockstmt(size_t line)
{
    LitAstBlockExpr* expr = (LitAstBlockExpr*)lit_ast_allocexpression(line, sizeof(LitAstBlockExpr), LIT_ASTEXPRTYP_BLOCK);
    lit_dynlistexpr_init(&expr->statements);
    return expr;
}

LitAstVarGetExpr* lit_ast_makevargetexpr(size_t line, const char* name, size_t length)
{
    LitAstVarGetExpr* expr = (LitAstVarGetExpr*)lit_ast_allocexpression(line, sizeof(LitAstVarGetExpr), LIT_ASTEXPRTYP_VARGET);
    expr->name = name;
    expr->length = length;
    return expr;
}

LitAstVarDeclExpr* lit_ast_makevardeclstmt(size_t line, const char* name, size_t length, LitAstExpression* init, bool constant)
{
    LitAstVarDeclExpr* expr = (LitAstVarDeclExpr*)lit_ast_allocexpression(line, sizeof(LitAstVarDeclExpr), LIT_ASTEXPRTYP_VARDECL);
    expr->name = name;
    expr->length = length;
    expr->init = init;
    expr->isconstant = constant;
    return expr;
}

LitAstIfExpr* lit_ast_makeifstatement(size_t line, LitAstExpression* condition, LitAstExpression* if_branch, LitAstExpression* else_branch, LitDynListExpr* elseif_conditions, LitDynListExpr* elseif_branches)
{
    LitAstIfExpr* expr = (LitAstIfExpr*)lit_ast_allocexpression(line, sizeof(LitAstIfExpr), LIT_ASTEXPRTYP_IF);
    expr->condition = condition;
    expr->branchif = if_branch;
    expr->branchelse = else_branch;
    expr->elseifcondlist = elseif_conditions;
    expr->branchelseiflist = elseif_branches;
    return expr;
}

LitAstWhileExpr* lit_ast_makewhilestmt(size_t line, LitAstExpression* condition, LitAstExpression* body)
{
    LitAstWhileExpr* expr = (LitAstWhileExpr*)lit_ast_allocexpression(line, sizeof(LitAstWhileExpr), LIT_ASTEXPRTYP_WHILE);
    expr->condition = condition;
    expr->body = body;
    return expr;
}

LitAstForExpr* lit_ast_makeforstmt(size_t line, LitAstExpression* init, LitAstExpression* var, LitAstExpression* condition, LitAstExpression* increment, LitAstExpression* body, bool cstyle)
{
    LitAstForExpr* expr = (LitAstForExpr*)lit_ast_allocexpression(line, sizeof(LitAstForExpr), LIT_ASTEXPRTYP_FOR);
    expr->init = init;
    expr->var = var;
    expr->condition = condition;
    expr->increment = increment;
    expr->body = body;
    expr->iscstyle = cstyle;
    return expr;
}

LitAstContinueExpr* lit_ast_makecontinuestmt(size_t line)
{
    return (LitAstContinueExpr*)lit_ast_allocexpression(line, sizeof(LitAstContinueExpr), LIT_ASTEXPRTYP_CONTINUE);
}

LitBreakStatement* lit_ast_makebreakstmt(size_t line)
{
    return (LitBreakStatement*)lit_ast_allocexpression(line, sizeof(LitBreakStatement), LIT_ASTEXPRTYP_BREAK);
}

LitAstFunctionExpr* lit_ast_makefuncdefstmt(size_t line, const char* name, size_t length)
{
    LitAstFunctionExpr* function = (LitAstFunctionExpr*)lit_ast_allocexpression(line, sizeof(LitAstFunctionExpr), LIT_ASTEXPRTYP_FUNCTION);
    function->name = name;
    function->length = length;
    function->body = NULL;
    lit_dynlistparam_init(&function->parameters);
    return function;
}

LitAstFunctionExpr* lit_ast_makelambdaexpr(size_t line)
{
    LitAstFunctionExpr* expr = (LitAstFunctionExpr*)lit_ast_allocexpression(line, sizeof(LitAstFunctionExpr), LIT_ASTEXPRTYP_FUNCANON);
    expr->body = NULL;
    lit_dynlistparam_init(&expr->parameters);
    return expr;
}


LitAstReturnExpr* lit_ast_makereturnstmt(size_t line, LitAstExpression* exv)
{
    LitAstReturnExpr* expr = (LitAstReturnExpr*)lit_ast_allocexpression(line, sizeof(LitAstReturnExpr), LIT_ASTEXPRTYP_RETURN);
    expr->exvalue = exv;
    return expr;
}

LitAstMethodExpr* lit_ast_makemethoddefstmt(size_t line, LitString* name, bool is_static)
{
    LitAstMethodExpr* expr = (LitAstMethodExpr*)lit_ast_allocexpression(line, sizeof(LitAstMethodExpr), LIT_ASTEXPRTYP_METHOD);
    expr->name = name;
    expr->body = NULL;
    expr->is_static = is_static;
    lit_dynlistparam_init(&expr->parameters);
    return expr;
}

LitAstClassExpr* lit_ast_makeclassdefstmt(size_t line, LitString* name, LitString* parent)
{
    LitAstClassExpr* expr = (LitAstClassExpr*)lit_ast_allocexpression(line, sizeof(LitAstClassExpr), LIT_ASTEXPRTYP_CLASS);
    expr->name = name;
    expr->parent = parent;
    lit_dynlistexpr_init(&expr->staticfields);
    return expr;
}

LitAstFieldExpr* lit_ast_makefieldstmt(size_t line, LitString* name, LitAstExpression* getter, LitAstExpression* setter, bool is_static)
{
    LitAstFieldExpr* expr = (LitAstFieldExpr*)lit_ast_allocexpression(line, sizeof(LitAstFieldExpr), LIT_ASTEXPRTYP_FIELD);
    expr->name = name;
    expr->getter = getter;
    expr->setter = setter;
    expr->is_static = is_static;
    return expr;
}

LitAstTryExpr* lit_ast_maketrystmt(size_t line, LitAstExpression* try_block, LitAstExpression* catch_block, LitAstExpression* finally_block, const char* catch_var, size_t catch_var_len)
{
    LitAstTryExpr* expr = (LitAstTryExpr*)lit_ast_allocexpression(line, sizeof(LitAstTryExpr), LIT_ASTEXPRTYP_TRY);
    expr->try_block = try_block;
    expr->catch_block = catch_block;
    expr->finally_block = finally_block;
    expr->catch_var = catch_var;
    expr->catch_var_len = catch_var_len;
    return expr;
}

LitAstThrowExpr* lit_ast_makethrowstmt(size_t line, LitAstExpression* exvalue)
{
    LitAstThrowExpr* expr = (LitAstThrowExpr*)lit_ast_allocexpression(line, sizeof(LitAstThrowExpr), LIT_ASTEXPRTYP_THROW);
    expr->exvalue = exvalue;
    return expr;
}

LitDynListExpr* lit_ast_allocexprlist()
{
    LitDynListExpr* expressions = (LitDynListExpr*)lit_sysmem_malloc(sizeof(LitDynListExpr));
    lit_dynlistexpr_init(expressions);
    return expressions;
}

void lit_ast_destroyallocatedexprlist(LitState* state, LitDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->listcount; i++)
    {
        lit_ast_destroyexpression(state, expressions->listitems[i]);
    }
    lit_dynlistexpr_destroy(expressions);
    lit_sysmem_free(expressions);
}

LitDynListExpr* lit_ast_allocstmtlist()
{
    LitDynListExpr* statements = (LitDynListExpr*)lit_sysmem_malloc(sizeof(LitDynListExpr));
    lit_dynlistexpr_init(statements);
    return statements;
}

void lit_ast_destroyallocatedstmtlist(LitState* state, LitDynListExpr* statements)
{
    size_t i;
    if(statements == NULL)
    {
        return;
    }
    for(i = 0; i < statements->listcount; i++)
    {
        lit_ast_destroyexpression(state, statements->listitems[i]);
    }
    lit_dynlistexpr_destroy(statements);
    lit_sysmem_free(statements);
}

void lit_astlex_init(LitState* state, LitAstLexer* lex, const char* filename, const char* source)
{
    lex->sourcecurrentline = 1;
    lex->sourcedatastart = source;
    lex->sourcedatacurrent = source;
    lex->sourcefilename = filename;
    lex->pstate = state;
    lex->bracecount = 0;
    lex->had_error = false;
}

LitAstToken lit_astlex_maketoken(LitAstLexer* lex, LitAstTokType type)
{
    LitAstToken token;
    token.type = type;
    token.start = lex->sourcedatastart;
    token.length = (size_t)(lex->sourcedatacurrent - lex->sourcedatastart);
    token.line = lex->sourcecurrentline;
    return token;
}

LitAstToken lit_astlex_makeerrortoken(LitAstLexer* lex, const char* fmt, ...)
{
    lex->had_error = true;
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(lex->pstate, lex->sourcecurrentline, fmt, args);
    va_end(args);
    LitAstToken token;
    token.type = LIT_ASTTOKTYP_ERROR;
    token.start = result->strbuf.data;
    token.length = result->strbuf.length;
    token.line = lex->sourcecurrentline;
    return token;
}

bool lit_astlex_isatend(LitAstLexer* lex)
{
    return *lex->sourcedatacurrent == '\0';
}

char lit_astlex_advance(LitAstLexer* lex)
{
    lex->sourcedatacurrent++;
    return lex->sourcedatacurrent[-1];
}

bool lit_astlex_match(LitAstLexer* lex, char expected)
{
    if(lit_astlex_isatend(lex))
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

LitAstToken lit_astlex_matchtoken(LitAstLexer* lex, char c, LitAstTokType a, LitAstTokType b)
{
    return lit_astlex_maketoken(lex, lit_astlex_match(lex, c) ? a : b);
}

LitAstToken lit_astlex_matchtokens(LitAstLexer* lex, char cr, char cb, LitAstTokType a, LitAstTokType b, LitAstTokType c)
{
    return lit_astlex_maketoken(lex, lit_astlex_match(lex, cr) ? a : (lit_astlex_match(lex, cb) ? b : c));
}

char lit_astlex_peek(LitAstLexer* lex)
{
    return *lex->sourcedatacurrent;
}

char lit_astlex_peeknext(LitAstLexer* lex)
{
    if(lit_astlex_isatend(lex))
    {
        return '\0';
    }
    return lex->sourcedatacurrent[1];
}

bool lit_astlex_skipwhitespace(LitAstLexer* lex)
{
    while(true)
    {
        char c = lit_astlex_peek(lex);
        switch(c)
        {
            case 1:
            case 2:
            case 3:
            case ' ':
            case '\r':
            case '\t':
            {
                lit_astlex_advance(lex);
                break;
            }
            case '\n':
            {
                lex->sourcedatastart = lex->sourcedatacurrent;
                lit_astlex_advance(lex);
                return true;
            }
            case '/':
            {
                if(lit_astlex_peeknext(lex) == '/')
                {
                    while(lit_astlex_peek(lex) != '\n' && !lit_astlex_isatend(lex))
                    {
                        lit_astlex_advance(lex);
                    }
                    return lit_astlex_skipwhitespace(lex);
                }
                else if(lit_astlex_peeknext(lex) == '*')
                {
                    lit_astlex_advance(lex);
                    lit_astlex_advance(lex);
                    while((lit_astlex_peek(lex) != '*' || lit_astlex_peeknext(lex) != '/') && !lit_astlex_isatend(lex))
                    {
                        if(lit_astlex_peek(lex) == '\n')
                        {
                            lex->sourcecurrentline++;
                        }
                        lit_astlex_advance(lex);
                    }
                    lit_astlex_advance(lex);
                    lit_astlex_advance(lex);
                    return lit_astlex_skipwhitespace(lex);
                }
                return false;
            }
            default:
                return false;
        }
    }
}

LitAstToken lit_astlex_scanstring(LitAstLexer* lex, bool interpolation, bool useescapes, char endch)
{
    char currch;
    char nextch;
    const char* cstr;
    LitDynListByte bytes;
    LitAstTokType stringtype;
    LitState* state;
    state = lex->pstate;
    stringtype = LIT_ASTTOKTYP_STRING;
    lit_dynlistbyte_init(&bytes);
    while(true)
    {
        currch = lit_astlex_advance(lex);
        if(currch == endch)
        {
            break;
        }
        else if(interpolation && currch == '{')
        {
            if(lex->bracecount >= LIT_INTERPOLATION_NESTING_MAX)
            {
                lit_dynlistbyte_destroy(&bytes);
                return lit_astlex_makeerrortoken(lex, "interpolation nesting is too deep, maximum is %i", LIT_INTERPOLATION_NESTING_MAX);
            }
            stringtype = LIT_ASTTOKTYP_STRTEMPLATE;
            lex->bracevalues[lex->bracecount++] = 1;
            break;
        }
        if(useescapes)
        {
            switch(currch)
            {
                case '\0':
                {
                    lit_dynlistbyte_destroy(&bytes);
                    return lit_astlex_makeerrortoken(lex, "unterminated string");
                }
                break;
                case '\n':
                {
                    lex->sourcecurrentline++;
                    lit_dynlistbyte_push(&bytes, currch);
                }
                break;
                case '\\':
                {
                    nextch = lit_astlex_advance(lex);
                    if(nextch == '\n')
                    {
                        continue;
                    }
                    if(nextch == endch)
                    {
                        lit_dynlistbyte_push(&bytes, endch);
                    }
                    else
                    {
                        switch(nextch)
                        {
                            case '\"':
                                lit_dynlistbyte_push(&bytes, '\"');
                                break;
                            case '\\':
                                lit_dynlistbyte_push(&bytes, '\\');
                                break;
                            case '0':
                                lit_dynlistbyte_push(&bytes, '\0');
                                break;
                            case '{':
                                lit_dynlistbyte_push(&bytes, '{');
                                break;
                            case 'a':
                                lit_dynlistbyte_push(&bytes, '\a');
                                break;
                            case 'b':
                                lit_dynlistbyte_push(&bytes, '\b');
                                break;
                            case 'f':
                                lit_dynlistbyte_push(&bytes, '\f');
                                break;
                            case 'n':
                                lit_dynlistbyte_push(&bytes, '\n');
                                break;
                            case 'r':
                                lit_dynlistbyte_push(&bytes, '\r');
                                break;
                            case 't':
                                lit_dynlistbyte_push(&bytes, '\t');
                                break;
                            case 'v':
                                lit_dynlistbyte_push(&bytes, '\v');
                                break;
                            case 'e':
                                lit_dynlistbyte_push(&bytes, 27);
                                break;
                            default:
                            {
                                lit_dynlistbyte_destroy(&bytes);
                                return lit_astlex_makeerrortoken(lex, "invalid escape character '%c'", lex->sourcedatacurrent[-1]);
                            }
                            break;
                        }
                    }
                }
                break;
                default:
                {
                    lit_dynlistbyte_push(&bytes, currch);
                }
                break;
            }
        }
        else
        {
            lit_dynlistbyte_push(&bytes, currch);
        }
    }
    LitAstToken token = lit_astlex_maketoken(lex, stringtype);
    /*if(bytes.listcount == 0)
    {
        token.value = lit_value_makenull();
    }
    else
    */
    {
        cstr = (const char*)bytes.listitems;
        if(bytes.listcount == 0)
        {
            cstr = "";
        }
        token.tokvalue = lit_value_fromobject(lit_string_copylen(state, cstr, bytes.listcount));
    }
    lit_dynlistbyte_destroy(&bytes);
    return token;
}

int lit_astlex_scanhexdigit(LitAstLexer* lex)
{
    char c = lit_astlex_advance(lex);
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

int lit_astlex_scanbinarydigit(LitAstLexer* lex)
{
    char c = lit_astlex_advance(lex);
    if(c >= '0' && c <= '1')
    {
        return c - '0';
    }
    lex->sourcedatacurrent--;
    return -1;
}

LitAstToken lit_astlex_makenumbertoken(LitAstLexer* lex, bool ishex, bool isbinary)
{
    errno = 0;
    LitValue value;
    if(ishex)
    {
        value = lit_value_makenumber((double)strtoll(lex->sourcedatastart, NULL, 16));
    }
    else if(isbinary)
    {
        value = lit_value_makenumber((int)strtoll(lex->sourcedatastart + 2, NULL, 2));
    }
    else
    {
        value = lit_value_makenumber(strtod(lex->sourcedatastart, NULL));
    }
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_astlex_makeerrortoken(lex, "number is too big to be represented by a single literal");
    }
    LitAstToken token = lit_astlex_maketoken(lex, LIT_ASTTOKTYP_NUMBER);
    token.tokvalue = value;
    return token;
}

LitAstToken lit_astlex_scannumber(LitAstLexer* lex)
{
    if(lit_astlex_match(lex, 'x'))
    {
        while(lit_astlex_scanhexdigit(lex) != -1)
        {
            continue;
        }
        return lit_astlex_makenumbertoken(lex, true, false);
    }
    if(lit_astlex_match(lex, 'b'))
    {
        while(lit_astlex_scanbinarydigit(lex) != -1)
        {
            continue;
        }
        return lit_astlex_makenumbertoken(lex, false, true);
    }
    while(lit_util_charisdigit(lit_astlex_peek(lex)))
    {
        lit_astlex_advance(lex);
    }
    /* look for a fractional part */
    if(lit_astlex_peek(lex) == '.' && lit_util_charisdigit(lit_astlex_peeknext(lex)))
    {
        /* consume the '.' */
        lit_astlex_advance(lex);
        while(lit_util_charisdigit(lit_astlex_peek(lex)))
        {
            lit_astlex_advance(lex);
        }
    }
    return lit_astlex_makenumbertoken(lex, false, false);
}

LitAstTokType lit_astlex_scanidenttype(LitAstLexer* lex)
{
    /* clang-format off*/
    static struct
    {
        LitAstTokType type;
        const char* kw;
    } keywords[] = {
        { LIT_ASTTOKTYP_KWCLASS, "class" },
        { LIT_ASTTOKTYP_KWELSE, "else" },
        { LIT_ASTTOKTYP_KWFALSE, "false" },
        { LIT_ASTTOKTYP_KWFOR, "for" },
        { LIT_ASTTOKTYP_KWFUNCTION, "function" },
        { LIT_ASTTOKTYP_KWIF, "if" },
        { LIT_ASTTOKTYP_KWNULL, "null" },
        { LIT_ASTTOKTYP_KWRETURN, "return" },
        { LIT_ASTTOKTYP_KWSUPER, "super" },
        { LIT_ASTTOKTYP_KWTHIS, "this" },
        { LIT_ASTTOKTYP_KWTRUE, "true" },
        { LIT_ASTTOKTYP_KWVAR, "var" },
        { LIT_ASTTOKTYP_KWWHILE, "while" },
        { LIT_ASTTOKTYP_KWCONTINUE, "continue" },
        { LIT_ASTTOKTYP_KWBREAK, "break" },
        { LIT_ASTTOKTYP_KWNEW, "new" },
        { LIT_ASTTOKTYP_KWEXPORT, "export" },
        { LIT_ASTTOKTYP_KWIS, "is" },
        { LIT_ASTTOKTYP_KWSTATIC, "static" },
        { LIT_ASTTOKTYP_KWOPERATOR, "operator" },
        { LIT_ASTTOKTYP_KWIN, "in" },
        { LIT_ASTTOKTYP_KWCONST, "const" },
        { LIT_ASTTOKTYP_KWREF, "ref" },
        { LIT_ASTTOKTYP_KWTRY, "try" },
        { LIT_ASTTOKTYP_KWCATCH, "catch" },
        { LIT_ASTTOKTYP_KWFINALLY, "finally" },
        { LIT_ASTTOKTYP_KWTHROW, "throw" },
        { (LitAstTokType)0, NULL },
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
    return LIT_ASTTOKTYP_IDENTIFIER;
}

LitAstToken lit_astlex_scanident(LitAstLexer* lex)
{
    while(lit_util_charisalpha(lit_astlex_peek(lex)) || lit_util_charisdigit(lit_astlex_peek(lex)))
    {
        lit_astlex_advance(lex);
    }
    return lit_astlex_maketoken(lex, lit_astlex_scanidenttype(lex));
}

LitAstToken lit_astlex_scantoken(LitAstLexer* lex)
{
    char c;
    LitAstToken token;
    if(lit_astlex_skipwhitespace(lex))
    {
        token = lit_astlex_maketoken(lex, LIT_ASTTOKTYP_LINEFEED);
        lex->sourcecurrentline++;
        return token;
    }
    lex->sourcedatastart = lex->sourcedatacurrent;
    if(lit_astlex_isatend(lex))
    {
        return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_EOF);
    }
    c = lit_astlex_advance(lex);
    if(lit_util_charisdigit(c))
    {
        return lit_astlex_scannumber(lex);
    }
    if(lit_util_charisalpha(c))
    {
        return lit_astlex_scanident(lex);
    }
    switch(c)
    {
        case '@':
            {
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_REFSYM);
            }
            break;
        case '(':
            {
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_LEFTPAREN);
            }
            break;
        case ')':
            {
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_RIGHTPAREN);
            };
        case '{':
            {
                if(lex->bracecount > 0)
                {
                    lex->bracevalues[lex->bracecount - 1]++;
                }
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_LEFTBRACE);
            }
            break;
        case '}':
            {
                if(lex->bracecount > 0 && --lex->bracevalues[lex->bracecount - 1] == 0)
                {
                    lex->bracecount--;
                    return lit_astlex_scanstring(lex, true, true, '`');
                }
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_RIGHTBRACE);
            }
            break;
        case '[':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_LEFTBRACKET);
        case ']':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_RIGHTBRACKET);
        case ';':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_SEMICOLON);
        case ',':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_COMMA);
        case ':':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_COLON);
        case '~':
            return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_TILDE);
        case '+':
            return lit_astlex_matchtokens(lex, '=', '+', LIT_ASTTOKTYP_PLUSEQUAL, LIT_ASTTOKTYP_PLUSPLUS, LIT_ASTTOKTYP_PLUS);
        case '-':
            {
                if(lit_astlex_match(lex, '>'))
                {
                    return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_SMALLARROW);
                }
                else
                {
                    return lit_astlex_matchtokens(lex, '=', '-', LIT_ASTTOKTYP_MINUSEQUAL, LIT_ASTTOKTYP_MINUSMINUS, LIT_ASTTOKTYP_MINUS);
                }
            }
            break;
        case '/':
            return lit_astlex_matchtoken(lex, '=', LIT_ASTTOKTYP_SLASHEQUAL, LIT_ASTTOKTYP_SLASH);
        case '#':
            return lit_astlex_matchtoken(lex, '=', LIT_ASTTOKTYP_SHARPEQUAL, LIT_ASTTOKTYP_SHARP);
        case '!':
            return lit_astlex_matchtoken(lex, '=', LIT_ASTTOKTYP_BANGEQUAL, LIT_ASTTOKTYP_BANG);
        case '?':
            return lit_astlex_matchtoken(lex, '?', LIT_ASTTOKTYP_QUESTIONQUESTION, LIT_ASTTOKTYP_QUESTION);
        case '%':
            return lit_astlex_matchtoken(lex, '=', LIT_ASTTOKTYP_PERCENTEQUAL, LIT_ASTTOKTYP_PERCENT);
        case '^':
            return lit_astlex_matchtoken(lex, '=', LIT_ASTTOKTYP_CARETEQUAL, LIT_ASTTOKTYP_CARET);
        case '>':
            return lit_astlex_matchtokens(lex, '=', '>', LIT_ASTTOKTYP_GREATEREQUAL, LIT_ASTTOKTYP_GREATERGREATER, LIT_ASTTOKTYP_GREATER);
        case '<':
            return lit_astlex_matchtokens(lex, '=', '<', LIT_ASTTOKTYP_LESSEQUAL, LIT_ASTTOKTYP_LESSLESS, LIT_ASTTOKTYP_LESS);
        case '*':
            return lit_astlex_matchtokens(lex, '=', '*', LIT_ASTTOKTYP_STAREQUAL, LIT_ASTTOKTYP_STARSTAR, LIT_ASTTOKTYP_STAR);
        case '=':
            return lit_astlex_matchtokens(lex, '=', '>', LIT_ASTTOKTYP_EQUALEQUAL, LIT_ASTTOKTYP_ARROW, LIT_ASTTOKTYP_EQUAL);
        case '|':
            return lit_astlex_matchtokens(lex, '=', '|', LIT_ASTTOKTYP_BAREQUAL, LIT_ASTTOKTYP_BARBAR, LIT_ASTTOKTYP_BAR);
        case '&':
            return lit_astlex_matchtokens(lex, '=', '&', LIT_ASTTOKTYP_AMPERSANDEQUAL, LIT_ASTTOKTYP_AMPERSANDAMPERSAND, LIT_ASTTOKTYP_AMPERSAND);
        case '.':
        {
            if(!lit_astlex_match(lex, '.'))
            {
                return lit_astlex_maketoken(lex, LIT_ASTTOKTYP_DOT);
            }
            return lit_astlex_matchtoken(lex, '.', LIT_ASTTOKTYP_DOTDOTDOT, LIT_ASTTOKTYP_DOTDOT);
        }
        case '`':
            {
                return lit_astlex_scanstring(lex, true, true, '`');
            }
        case '"':
            {
                return lit_astlex_scanstring(lex, false, true, '"');
            }
            break;
        case '\'':
            {
                return lit_astlex_scanstring(lex, false, false, '\'');
            }
            break;
    }
    fprintf(stderr, "current=%s\n", lex->sourcedatacurrent);
    return lit_astlex_makeerrortoken(lex, "unexpected character '%c'", c);
}

static jmp_buf jumpbuffer;
static LitAstRule g_astparserules[LIT_ASTTOKTYP_EOF + 1];
static bool didsetuprules;

void lit_astparser_compilerinit(LitAstParser* prs, LitAstCompiler* ccx)
{
    ccx->scope_depth = 0;
    ccx->function = NULL;
    ccx->enclosing = (struct LitAstCompiler*)prs->compiler;
    prs->compiler = ccx;
}

void lit_astparser_compilerend(LitAstParser* prs, LitAstCompiler* ccx)
{
    prs->compiler = (LitAstCompiler*)ccx->enclosing;
}

void lit_astparser_scopebegin(LitAstParser* prs)
{
    prs->compiler->scope_depth++;
}

void lit_astparser_scopeend(LitAstParser* prs)
{
    prs->compiler->scope_depth--;
}

LitAstRule* lit_astparser_getrule(LitAstTokType type)
{
    return &g_astparserules[type];
}

bool lit_astparser_isatend(LitAstParser* prs)
{
    return prs->current.type == LIT_ASTTOKTYP_EOF;
}

void lit_astparser_init(LitState* state, LitAstParser* prs)
{
    if(!didsetuprules)
    {
        didsetuprules = true;
        lit_astparser_setuprules();
    }
    prs->pstate = state;
    prs->had_error = false;
    prs->panic_mode = false;
}

void lit_astparser_destroy(LitAstParser* prs)
{
    (void)prs;
}

void lit_astparser_failactual(LitAstParser* prs, LitAstToken* token, const char* message)
{
    (void)token;
    if(prs->panic_mode)
    {
        return;
    }
    lit_state_raiseerror(prs->pstate, LIT_ERROR_COMPILEERROR, message);
    prs->had_error = true;
    lit_astparser_sync(prs);
}

void lit_astparser_failatv(LitAstParser* prs, LitAstToken* token, const char* fmt, va_list args)
{
    lit_astparser_failactual(prs, token, lit_state_errorfmtv(prs->pstate, token->line, fmt, args)->strbuf.data);
}

void lit_astparser_failherefmt(LitAstParser* prs, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_astparser_failatv(prs, &prs->current, fmt, args);
    va_end(args);
}

void lit_astparser_failfmt(LitAstParser* prs, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_astparser_failatv(prs, &prs->previous, fmt, args);
    va_end(args);
}

void lit_astparser_advance(LitAstParser* prs)
{
    prs->previous = prs->current;
    while(true)
    {
        prs->current = lit_astlex_scantoken(prs->pstate->lexer);
        if(prs->current.type != LIT_ASTTOKTYP_ERROR)
        {
            break;
        }
        lit_astparser_failactual(prs, &prs->current, prs->current.start);
    }
}

bool lit_astparser_check(LitAstParser* prs, LitAstTokType type)
{
    return prs->current.type == type;
}

bool lit_astparser_match(LitAstParser* prs, LitAstTokType type)
{
    if(prs->current.type == type)
    {
        lit_astparser_advance(prs);
        return true;
    }
    return false;
}

bool lit_astparser_matchident(LitAstParser* prs, const char* type)
{
    if(prs->current.type == LIT_ASTTOKTYP_IDENTIFIER || prs->current.type == LIT_ASTTOKTYP_KWCLASS)
    {
        if(memcmp(prs->previous.start, type, fmax(strlen(type), prs->previous.length)))
        {
            lit_astparser_advance(prs);
            return true;
        }
    }
    return false;
}

void lit_astparser_consume(LitAstParser* prs, LitAstTokType type, const char* error)
{
    if(prs->current.type == type)
    {
        lit_astparser_advance(prs);
        return;
    }
    bool line = prs->previous.type == LIT_ASTTOKTYP_LINEFEED;
    lit_astparser_failactual(prs, &prs->current, lit_state_errorfmt(prs->pstate, prs->current.line, "expected %s, got '%.*s'", error, line ? 8 : prs->previous.length, line ? "new line" : prs->previous.start)->strbuf.data);
}

bool lit_astparser_matchlinefeed(LitAstParser* prs)
{
    if(!lit_astparser_match(prs, LIT_ASTTOKTYP_LINEFEED))
    {
        return false;
    }
    while(lit_astparser_match(prs, LIT_ASTTOKTYP_LINEFEED))
    {
    }
    return true;
}

void lit_astparser_ignorelinefeeds(LitAstParser* prs)
{
    lit_astparser_matchlinefeed(prs);
}

LitAstExpression* lit_astparser_parseblock(LitAstParser* prs)
{
    LitAstBlockExpr* expr;
    lit_astparser_scopebegin(prs);
    expr = lit_ast_makeblockstmt(prs->previous.line);
    lit_astparser_ignorelinefeeds(prs);
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTBRACE) && !lit_astparser_check(prs, LIT_ASTTOKTYP_EOF))
    {
        lit_dynlistexpr_push(&expr->statements, lit_astparser_parsestmt(prs));
        lit_astparser_ignorelinefeeds(prs);
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACE, "'}'");
    lit_astparser_scopeend(prs);
    return (LitAstExpression*)expr;
}

LitAstExpression* lit_astparser_parseprec(LitAstParser* prs, LitPrecedence precedence, bool err)
{
    LitAstToken previous;
    previous = prs->previous;
    lit_astparser_ignorelinefeeds(prs);
    lit_astparser_advance(prs);
    LitPrefixParseFn prefixrule = lit_astparser_getrule(prs->previous.type)->prefix;
    if(prefixrule == NULL)
    {
        /* todo: file start */
        bool prevnewline = ((previous.start != NULL) && (*previous.start == '\n'));
        bool parserprevnewline = ((prs->previous.start != NULL) && (*prs->previous.start == '\n'));
        int exlen;
        int gotlen;
        const char* gotstr;
        const char* exstr;
        exlen = 8;
        gotlen = 8;
        exstr = "new line";
        gotstr = "new line";
        if(!prevnewline)
        {
            exlen = previous.length;
            exstr = previous.start;
        }
        if(!parserprevnewline)
        {
            gotlen = prs->previous.length;
            gotstr = prs->previous.start;
        }
        lit_astparser_failfmt(prs, "expected expression after '%.*s', got '%.*s'", exlen, exstr, gotlen, gotstr);
        return NULL;
    }
    bool canassign = precedence <= LIT_ASTPREC_ASSIGNMENT;
    LitAstExpression* expr = prefixrule(prs, canassign);
    lit_astparser_ignorelinefeeds(prs);
    while(precedence <= lit_astparser_getrule(prs->current.type)->precedence)
    {
        lit_astparser_advance(prs);
        LitInfixParseFn infixrule = lit_astparser_getrule(prs->previous.type)->infix;
        if(infixrule == NULL)
        {
            break;
        }
        expr = infixrule(prs, expr, canassign);
    }
    if(err && canassign && lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        lit_astparser_failfmt(prs, "invalid assigment target");
    }
    return expr;
}

LitAstExpression* lit_astparser_rulenumber(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    return (LitAstExpression*)lit_ast_makeliteralexpr(prs->previous.line, prs->previous.tokvalue);
}

LitAstExpression* lit_astparser_parselambda(LitAstParser* prs, LitAstFunctionExpr* lambda)
{
    lambda->body = lit_astparser_parsestmt(prs);
    return (LitAstExpression*)lambda;
}

LitAstFuncParamExpr lit_astparser_makeparameter(const char* name, size_t length, uint8_t reg, LitAstExpression* defval)
{
    LitAstFuncParamExpr pm;
    pm.name = name;
    pm.length = length;
    pm.reg = reg;
    pm.defaultval = defval;
    return pm;
}

void lit_astparser_parseparams(LitAstParser* prs, LitDynListParam* parameters)
{
    bool haddefault;
    haddefault = false;
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTPAREN))
    {
        /* variadic argument ... */
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_DOTDOTDOT))
        {
            lit_dynlistparam_push(parameters, lit_astparser_makeparameter("...", 3, 0, NULL));
            return;
        }
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "argument name");
        const char* argname = prs->previous.start;
        size_t arglength = prs->previous.length;
        LitAstExpression* defval = NULL;
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
        {
            haddefault = true;
            defval = lit_astparser_parseexpr(prs);
        }
        else if(haddefault)
        {
            lit_astparser_failfmt(prs, "default arguments must always be in the end of the argument list.");
        }
        lit_dynlistparam_push(parameters, lit_astparser_makeparameter(argname, arglength, 0, defval));
        if(!lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
    }
}

LitAstExpression* lit_astparser_rulegroupingorlambda(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_RIGHTPAREN))
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_ARROW, "=> after lambda arguments");
        return lit_astparser_parselambda(prs, lit_ast_makelambdaexpr(prs->previous.line));
    }
    const char* start = prs->previous.start;
    size_t line = prs->previous.line;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_IDENTIFIER) || lit_astparser_match(prs, LIT_ASTTOKTYP_DOTDOTDOT))
    {
        LitState* state = prs->pstate;
        const char* firstargstart = prs->previous.start;
        size_t firstarglength = prs->previous.length;
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA) || (lit_astparser_match(prs, LIT_ASTTOKTYP_RIGHTPAREN) && lit_astparser_match(prs, LIT_ASTTOKTYP_ARROW)))
        {
            bool hadarrow = prs->previous.type == LIT_ASTTOKTYP_ARROW;
            bool hadvararg = prs->previous.type == LIT_ASTTOKTYP_DOTDOTDOT;
            /* this is a lambda */
            LitAstFunctionExpr* lambda = lit_ast_makelambdaexpr(line);
            LitAstExpression* defvalue = NULL;
            bool haddefault = lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL);
            if(haddefault)
            {
                defvalue = lit_astparser_parseexpr(prs);
            }
            lit_dynlistparam_push(&lambda->parameters, lit_astparser_makeparameter(firstargstart, firstarglength, 0, defvalue));
            if(!hadvararg && prs->previous.type == LIT_ASTTOKTYP_COMMA)
            {
                do
                {
                    bool stop = false;
                    if(lit_astparser_match(prs, LIT_ASTTOKTYP_DOTDOTDOT))
                    {
                        stop = true;
                    }
                    else
                    {
                        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "argument name");
                    }
                    const char* argname = prs->previous.start;
                    size_t arglength = prs->previous.length;
                    LitAstExpression* defval = NULL;
                    if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
                    {
                        defval = lit_astparser_parseexpr(prs);
                        haddefault = true;
                    }
                    else if(haddefault)
                    {
                        lit_astparser_failfmt(prs, "default arguments must always be in the end of the argument list.");
                    }
                    lit_dynlistparam_push(&lambda->parameters, lit_astparser_makeparameter(argname, arglength, 0, defval));
                    if(stop)
                    {
                        break;
                    }
                } while(lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA));
            }
            if(!hadarrow)
            {
                lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after lambda parameters");
                lit_astparser_consume(prs, LIT_ASTTOKTYP_ARROW, "=> after lambda parameters");
            }
            return lit_astparser_parselambda(prs, lambda);
        }
        else
        {
            /* ouch, this was a grouping with a single identifier */
            LitAstLexer* lex = state->lexer;
            lex->sourcedatacurrent = start;
            lex->sourcecurrentline = line;
            prs->current = lit_astlex_scantoken(lex);
            lit_astparser_advance(prs);
        }
    }
    LitAstExpression* expr = lit_astparser_parseexpr(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after grouping expression");
    return expr;
}

LitAstExpression* lit_astparser_parsecall(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    LitAstCallExpr* expr = lit_ast_makecallexpr(prs->previous.line, prev);
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTPAREN))
    {
        LitAstExpression* e = lit_astparser_parseexpr(prs);
        lit_dynlistexpr_push(&expr->callargs, e);
        if(!lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
        if(e->type == LIT_ASTEXPRTYP_VARGET)
        {
            LitAstVarGetExpr* ee = (LitAstVarGetExpr*)e;
            /* variadic arg ... */
            if(ee->length == 3 && memcmp(ee->name, "...", 3) == 0)
            {
                break;
            }
        }
    }
    if(expr->callargs.listcount > 255)
    {
        lit_astparser_failfmt(prs, "function can't have more than 255 arguments, got %i", (int)expr->callargs.listcount);
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after arguments");
    return (LitAstExpression*)expr;
}

/*
{
    size_t line;
    LitAstRefExpr* expr;
    (void)canassign;
    line = prs->previous.line;
    lit_astparser_ignorelinefeeds(prs);
    expr = lit_ast_makerefexpr(line, lit_astparser_parseprec(prs, LIT_ASTPREC_CALL, false));
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitAstExpression*)lit_ast_makeassignexpr(line, (LitAstExpression*)expr, lit_astparser_parseexpr(prs));
    }
    return (LitAstExpression*)expr;
}
*/
LitAstExpression* lit_astparser_ruleunary(LitAstParser* prs, bool canassign)
{
    bool isrefsym;
    size_t line;
    LitAstTokType op;
    LitAstRefExpr* refexp;
    LitAstExpression* targetexpr;
    (void)canassign;
    op = prs->previous.type;
    line = prs->previous.line;
    isrefsym = (prs->previous.start[0] == '@');
    #if 1
    if(isrefsym)
    {
        targetexpr = lit_astparser_parseprec(prs, LIT_ASTPREC_CALL, false);
    }
    else
    #endif
    {
        targetexpr = lit_astparser_parseprec(prs, LIT_ASTPREC_UNARY, true);
    }
    if(isrefsym)
    {
        //fprintf(stderr, "parsing symbol @ for ref\n");
        refexp = lit_ast_makerefexpr(line, targetexpr);
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
        {
            return (LitAstExpression*)lit_ast_makeassignexpr(line, (LitAstExpression*)refexp, lit_astparser_parseexpr(prs));
        }
        return (LitAstExpression*)refexp;
    }
    return (LitAstExpression*)lit_ast_makeunaryexpr(line, targetexpr, op);
}

LitAstExpression* lit_astparser_rulebinary(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    bool invert = prs->previous.type == LIT_ASTTOKTYP_BANG;
    if(invert)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_KWIS, "'is' after '!'");
    }
    LitAstTokType op = prs->previous.type;
    size_t line = prs->previous.line;
    LitAstRule* rule = lit_astparser_getrule(op);
    lit_astparser_ignorelinefeeds(prs);
    LitAstExpression* expr = lit_astparser_parseprec(prs, (LitPrecedence)(rule->precedence + 1), true);
    lit_astparser_ignorelinefeeds(prs);
    expr = (LitAstExpression*)lit_ast_makebinaryexpr(line, prev, expr, op);
    if(invert)
    {
        expr = (LitAstExpression*)lit_ast_makeunaryexpr(line, expr, LIT_ASTTOKTYP_BANG);
    }
    return expr;
}

LitAstExpression* lit_astparser_rulelogicaland(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    LitAstTokType op = prs->previous.type;
    size_t line = prs->previous.line;
    return (LitAstExpression*)lit_ast_makebinaryexpr(line, prev, lit_astparser_parseprec(prs, LIT_ASTPREC_AND, true), op);
}

LitAstExpression* lit_astparser_rulelogicalor(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    LitAstTokType op = prs->previous.type;
    size_t line = prs->previous.line;
    return (LitAstExpression*)lit_ast_makebinaryexpr(line, prev, lit_astparser_parseprec(prs, LIT_ASTPREC_OR, true), op);
}

LitAstExpression* lit_astparser_rulenullfilter(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    LitAstTokType op = prs->previous.type;
    size_t line = prs->previous.line;
    return (LitAstExpression*)lit_ast_makebinaryexpr(line, prev, lit_astparser_parseprec(prs, LIT_ASTPREC_NULL, true), op);
}

LitAstTokType lit_astparser_convertcompoundop(LitAstTokType op)
{
    switch(op)
    {
        case LIT_ASTTOKTYP_PLUSEQUAL:
            return LIT_ASTTOKTYP_PLUS;
        case LIT_ASTTOKTYP_MINUSEQUAL:
            return LIT_ASTTOKTYP_MINUS;
        case LIT_ASTTOKTYP_STAREQUAL:
            return LIT_ASTTOKTYP_STAR;
        case LIT_ASTTOKTYP_SLASHEQUAL:
            return LIT_ASTTOKTYP_SLASH;
        case LIT_ASTTOKTYP_SHARPEQUAL:
            return LIT_ASTTOKTYP_SHARP;
        case LIT_ASTTOKTYP_PERCENTEQUAL:
            return LIT_ASTTOKTYP_PERCENT;
        case LIT_ASTTOKTYP_CARETEQUAL:
            return LIT_ASTTOKTYP_CARET;
        case LIT_ASTTOKTYP_BAREQUAL:
            return LIT_ASTTOKTYP_BAR;
        case LIT_ASTTOKTYP_AMPERSANDEQUAL:
            return LIT_ASTTOKTYP_AMPERSAND;
        case LIT_ASTTOKTYP_PLUSPLUS:
            return LIT_ASTTOKTYP_PLUS;
        case LIT_ASTTOKTYP_MINUSMINUS:
            return LIT_ASTTOKTYP_MINUS;
        default:
        {
            UNREACHABLE
        }
    }
    return LIT_ASTTOKTYP_EOF;
}

LitAstExpression* lit_astparser_rulecompound(LitAstParser* prs, LitAstExpression* prev, bool canassign)
{
    (void)canassign;
    LitAstTokType op = prs->previous.type;
    size_t line = prs->previous.line;
    LitAstRule* rule = lit_astparser_getrule(op);
    LitAstExpression* expr;
    if(op == LIT_ASTTOKTYP_PLUSPLUS || op == LIT_ASTTOKTYP_MINUSMINUS)
    {
        expr = (LitAstExpression*)lit_ast_makeliteralexpr(line, lit_value_makenumber(1));
    }
    else
    {
        expr = lit_astparser_parseprec(prs, (LitPrecedence)(rule->precedence + 1), true);
    }
    LitAstBinaryExpr* binary = lit_ast_makebinaryexpr(line, prev, expr, lit_astparser_convertcompoundop(op));
    /* to make sure we don't free it twice */
    binary->ignore_left = true;
    return (LitAstExpression*)lit_ast_makeassignexpr(line, prev, (LitAstExpression*)binary);
}

LitAstExpression* lit_astparser_ruleliteral(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    size_t line = prs->previous.line;
    switch(prs->previous.type)
    {
        case LIT_ASTTOKTYP_KWTRUE:
        {
            return (LitAstExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(true));
        }
        case LIT_ASTTOKTYP_KWFALSE:
        {
            return (LitAstExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(false));
        }
        case LIT_ASTTOKTYP_KWNULL:
        {
            return (LitAstExpression*)lit_ast_makeliteralexpr(line, lit_value_makenull());
        }
        default:
            UNREACHABLE
    }
    return NULL;
}

LitAstExpression* lit_astparser_rulestring(LitAstParser* prs, bool canassign)
{
    LitAstExpression* expr = (LitAstExpression*)lit_ast_makeliteralexpr(prs->previous.line, prs->previous.tokvalue);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, expr, canassign);
    }
    return expr;
}

LitAstExpression* lit_astparser_ruleinterpolation(LitAstParser* prs, bool canassign)
{
    LitString* str;
    LitAstStrTemplateExpr* expr = lit_ast_makeinterpolationexpr(prs->previous.line);
    do
    {
        str = lit_value_asstring(prs->previous.tokvalue);
        if(str != NULL)
        {
            if(str->strbuf.length > 0)
            {
                lit_dynlistexpr_push(&expr->expressions, (LitAstExpression*)lit_ast_makeliteralexpr(prs->previous.line, prs->previous.tokvalue));
            }
        }
        lit_dynlistexpr_push(&expr->expressions, lit_astparser_parseexpr(prs));
    } while(lit_astparser_match(prs, LIT_ASTTOKTYP_STRTEMPLATE));
    lit_astparser_consume(prs, LIT_ASTTOKTYP_STRING, "end of interpolation");
    str = lit_value_asstring(prs->previous.tokvalue);
    if(str != NULL)
    {
        if(str->strbuf.length > 0)
        {
            lit_dynlistexpr_push(&expr->expressions, (LitAstExpression*)lit_ast_makeliteralexpr(prs->previous.line, prs->previous.tokvalue));
        }
    }
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, (LitAstExpression*)expr, canassign);
    }
    return (LitAstExpression*)expr;
}

LitAstExpression* lit_astparser_rulearray(LitAstParser* prs, bool canassign)
{
    LitAstLiteralArrayExpr* array = lit_ast_makearrayexpr(prs->previous.line);
    lit_astparser_ignorelinefeeds(prs);
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTBRACKET))
    {
        lit_astparser_ignorelinefeeds(prs);
        lit_dynlistexpr_push(&array->exvalues, lit_astparser_parseexpr(prs));
        lit_astparser_ignorelinefeeds(prs);
        if(!lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
        lit_astparser_ignorelinefeeds(prs);
    }
    lit_astparser_ignorelinefeeds(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACKET, "']' after array");
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, (LitAstExpression*)array, canassign);
    }
    return (LitAstExpression*)array;
}


LitAstExpression* lit_astparser_ruleobject(LitAstParser* prs, bool canassign)
{
    LitString* keystr;
    (void)canassign;
    LitAstLiteralObjectExpr* object = lit_ast_makeobjectexpr(prs->previous.line);
    lit_astparser_ignorelinefeeds(prs);
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTBRACE))
    {
        keystr = NULL;
        lit_astparser_ignorelinefeeds(prs);
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_STRING))
        {
            keystr = lit_string_copylen(prs->pstate, prs->previous.start+1, prs->previous.length-2);
        }
        else if(lit_astparser_match(prs, LIT_ASTTOKTYP_IDENTIFIER))
        {
            keystr = lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length);
        }
        else
        {
            lit_astparser_failherefmt(prs, "expected identifier or string");
            return NULL;
        }
        lit_dynlistval_push(&object->objexkeys, lit_value_fromobject(keystr));
        lit_astparser_ignorelinefeeds(prs);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_COLON, "':' after key string");
        lit_astparser_ignorelinefeeds(prs);
        lit_dynlistexpr_push(&object->objexvalues, lit_astparser_parseexpr(prs));
        if(!lit_astparser_match(prs, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
        lit_astparser_ignorelinefeeds(prs);
    }
    lit_astparser_ignorelinefeeds(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACE, "'}' after object");
    return (LitAstExpression*)object;
}

LitAstExpression* lit_astparser_parsevarexprbase(LitAstParser* prs, bool canassign, bool isnew)
{
    LitAstExpression* expr = (LitAstExpression*)lit_ast_makevargetexpr(prs->previous.line, prs->previous.start, prs->previous.length);
    if(isnew)
    {
        bool hadargs = lit_astparser_check(prs, LIT_ASTTOKTYP_LEFTPAREN);
        LitAstCallExpr* call = NULL;
        if(hadargs)
        {
            lit_astparser_advance(prs);
            call = (LitAstCallExpr*)lit_astparser_parsecall(prs, expr, false);
        }
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACE))
        {
            if(call == NULL)
            {
                call = lit_ast_makecallexpr(expr->line, expr);
            }
            call->init = lit_astparser_ruleobject(prs, false);
        }
        else if(!hadargs)
        {
            lit_astparser_failherefmt(prs, "expected %s, got '%.*s'", "argument list for instance creation", prs->previous.length, prs->previous.start);
        }
        return (LitAstExpression*)call;
    }
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, expr, canassign);
    }
    if(canassign && lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitAstExpression*)lit_ast_makeassignexpr(prs->previous.line, expr, lit_astparser_parseexpr(prs));
    }
    return expr;
}

LitAstExpression* lit_astparser_rulevarexpr(LitAstParser* prs, bool canassign)
{
    return lit_astparser_parsevarexprbase(prs, canassign, false);
}

LitAstExpression* lit_astparser_rulenewexpr(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "class name after 'new'");
    return lit_astparser_parsevarexprbase(prs, false, true);
}

LitAstExpression* lit_astparser_ruledot(LitAstParser* prs, LitAstExpression* previous, bool canassign)
{
    size_t line = prs->previous.line;
    bool ignored = prs->previous.type == LIT_ASTTOKTYP_SMALLARROW;
    /* class and super are allowed field names */
    if(!(lit_astparser_match(prs, LIT_ASTTOKTYP_KWCLASS) || lit_astparser_match(prs, LIT_ASTTOKTYP_KWSUPER)))
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
    }
    const char* name = prs->previous.start;
    size_t length = prs->previous.length;
    if(!ignored && canassign && lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitAstExpression*)lit_ast_makesetexpr(line, previous, name, length, lit_astparser_parseexpr(prs));
    }
    else
    {
        LitAstExpression* expr = (LitAstExpression*)lit_ast_makegetexpr(line, previous, name, length, false, ignored);
        if(!ignored && lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
        {
            return lit_astparser_parsesubscript(prs, expr, canassign);
        }
        return expr;
    }
}

LitAstExpression* lit_astparser_rulerange(LitAstParser* prs, LitAstExpression* previous, bool canassign)
{
    (void)canassign;
    size_t line = prs->previous.line;
    return (LitAstExpression*)lit_ast_makerangeexpr(line, previous, lit_astparser_parseexpr(prs));
}

LitAstExpression* lit_astparser_ruleternaryorquestion(LitAstParser* prs, LitAstExpression* previous, bool canassign)
{
    (void)canassign;
    size_t line = prs->previous.line;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_DOT) /* || lit_astparser_match(prs, LIT_ASTTOKTYP_SMALLARROW)*/)
    {
        bool ignored = prs->previous.type == LIT_ASTTOKTYP_SMALLARROW;
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
        return (LitAstExpression*)lit_ast_makegetexpr(line, previous, prs->previous.start, prs->previous.length, true, ignored);
    }
    LitAstExpression* if_branch = lit_astparser_parseexpr(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_COLON, "':' after expression");
    LitAstExpression* else_branch = lit_astparser_parseexpr(prs);
    return (LitAstExpression*)lit_ast_maketernaryexpr(line, previous, if_branch, else_branch);
}


LitAstExpression* lit_astparser_parsesubscript(LitAstParser* prs, LitAstExpression* previous, bool canassign)
{
    size_t line = prs->previous.line;
    LitAstExpression* index = lit_astparser_parseexpr(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACKET, "']' after subscript");
    LitAstExpression* expr = (LitAstExpression*)lit_ast_makesubscriptexpr(line, previous, index);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, expr, canassign);
    }
    else if(canassign && lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitAstExpression*)lit_ast_makeassignexpr(prs->previous.line, expr, lit_astparser_parseexpr(prs));
    }
    return expr;
}

LitAstExpression* lit_astparser_rulethis(LitAstParser* prs, bool canassign)
{
    LitAstExpression* expr = (LitAstExpression*)lit_ast_makethisexpr(prs->previous.line);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACKET))
    {
        return lit_astparser_parsesubscript(prs, expr, canassign);
    }
    return expr;
}

LitAstExpression* lit_astparser_rulesuper(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    size_t line = prs->previous.line;
    if(!(lit_astparser_match(prs, LIT_ASTTOKTYP_DOT) || lit_astparser_match(prs, LIT_ASTTOKTYP_SMALLARROW)))
    {
        LitAstExpression* expr = (LitAstExpression*)lit_ast_makesuperexpr(line, prs->pstate->strings.strconstructor, false);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTPAREN, "'(' after 'super'");
        return lit_astparser_parsecall(prs, expr, false);
    }
    bool ignoring = prs->previous.type == LIT_ASTTOKTYP_SMALLARROW;
    lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, ignoring ? "super method name after '->'" : "super method name after '.'");
    LitAstExpression* expr = (LitAstExpression*)lit_ast_makesuperexpr(line, lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length), ignoring);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN))
    {
        return lit_astparser_parsecall(prs, expr, false);
    }
    return expr;
}

LitAstExpression* lit_astparser_rulenothing(LitAstParser* prs, bool canassign)
{
    (void)prs;
    (void)canassign;
    return NULL;
}

LitAstExpression* lit_astparser_rulefunction(LitAstParser* prs, bool canassign)
{
    (void)canassign;
    return lit_astparser_parsefunction(prs);
}

LitAstExpression* lit_astparser_rulereference(LitAstParser* prs, bool canassign)
{
    size_t line;
    LitAstRefExpr* expr;
    (void)canassign;
    line = prs->previous.line;
    lit_astparser_ignorelinefeeds(prs);
    expr = lit_ast_makerefexpr(line, lit_astparser_parseprec(prs, LIT_ASTPREC_CALL, false));
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitAstExpression*)lit_ast_makeassignexpr(line, (LitAstExpression*)expr, lit_astparser_parseexpr(prs));
    }
    return (LitAstExpression*)expr;
}

LitAstExpression* lit_astparser_parseexpr(LitAstParser* prs)
{
    lit_astparser_ignorelinefeeds(prs);
    return lit_astparser_parseprec(prs, LIT_ASTPREC_ASSIGNMENT, true);
}

LitAstExpression* lit_astparser_parsevardecl(LitAstParser* prs)
{
    bool constant = prs->previous.type == LIT_ASTTOKTYP_KWCONST;
    size_t line = prs->previous.line;
    lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "variable name");
    const char* name = prs->previous.start;
    size_t length = prs->previous.length;
    LitAstExpression* init = NULL;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_EQUAL))
    {
        init = lit_astparser_parseexpr(prs);
    }
    return (LitAstExpression*)lit_ast_makevardeclstmt(line, name, length, init, constant);
}

LitAstExpression* lit_astparser_parsetry(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    lit_astparser_ignorelinefeeds(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTBRACE, "Expect '{' after 'try'");
    LitAstExpression* try_block = lit_astparser_parseblock(prs);
    LitAstExpression* catch_block = NULL;
    LitAstExpression* finally_block = NULL;
    const char* catch_var = NULL;
    size_t catch_var_len = 0;

    lit_astparser_ignorelinefeeds(prs);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWCATCH))
    {
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN))
        {
            lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "Expect identifier after 'catch('");
            catch_var = prs->previous.start;
            catch_var_len = prs->previous.length;
            lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "Expect ')' after catch identifier");
        }
        lit_astparser_ignorelinefeeds(prs);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTBRACE, "Expect '{' after 'catch'");
        catch_block = lit_astparser_parseblock(prs);
    }

    lit_astparser_ignorelinefeeds(prs);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWFINALLY))
    {
        lit_astparser_ignorelinefeeds(prs);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTBRACE, "Expect '{' after 'finally'");
        finally_block = lit_astparser_parseblock(prs);
    }

    if(catch_block == NULL && finally_block == NULL)
    {
        lit_state_raiseerror(prs->pstate, LIT_ERROR_COMPILEERROR, "Expect 'catch' or 'finally' after 'try'");
    }

    return (LitAstExpression*)lit_ast_maketrystmt(line, try_block, catch_block, finally_block, catch_var, catch_var_len);
}

LitAstExpression* lit_astparser_parsethrow(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    LitAstExpression* exvalue = lit_astparser_parseexpr(prs);
    return (LitAstExpression*)lit_ast_makethrowstmt(line, exvalue);
}

LitAstExpression* lit_astparser_parseif(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    bool invert = lit_astparser_match(prs, LIT_ASTTOKTYP_BANG);
    bool hadparen = lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN);
    LitAstExpression* condition = lit_astparser_parseexpr(prs);
    if(hadparen)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')'");
    }
    if(invert)
    {
        condition = (LitAstExpression*)lit_ast_makeunaryexpr(condition->line, condition, LIT_ASTTOKTYP_BANG);
    }
    lit_astparser_ignorelinefeeds(prs);
    LitAstExpression* if_branch = lit_astparser_parsestmt(prs);
    LitDynListExpr* elseif_conditions = NULL;
    LitDynListExpr* elseif_branches = NULL;
    LitAstExpression* else_branch = NULL;
    lit_astparser_ignorelinefeeds(prs);
    while(lit_astparser_match(prs, LIT_ASTTOKTYP_KWELSE))
    {
        /* else if */
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWIF))
        {
            if(elseif_conditions == NULL)
            {
                elseif_conditions = lit_ast_allocexprlist();
                elseif_branches = lit_ast_allocstmtlist();
            }
            invert = lit_astparser_match(prs, LIT_ASTTOKTYP_BANG);
            hadparen = lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN);
            LitAstExpression* e = lit_astparser_parseexpr(prs);
            if(hadparen)
            {
                lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')'");
            }
            if(invert)
            {
                e = (LitAstExpression*)lit_ast_makeunaryexpr(condition->line, e, LIT_ASTTOKTYP_BANG);
            }
            lit_dynlistexpr_push(elseif_conditions, e);
            lit_astparser_ignorelinefeeds(prs);
            lit_dynlistexpr_push(elseif_branches, lit_astparser_parsestmt(prs));
            lit_astparser_ignorelinefeeds(prs);
            continue;
        }
        /* else */
        if(else_branch != NULL)
        {
            lit_astparser_failfmt(prs, "if-statement can have only one else-branch");
        }
        else_branch = lit_astparser_parsestmt(prs);
    }
    return (LitAstExpression*)lit_ast_makeifstatement(line, condition, if_branch, else_branch, elseif_conditions, elseif_branches);
}

LitAstExpression* lit_astparser_parsefor(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    bool hadparen = lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN);
    LitAstExpression* var = NULL;
    LitAstExpression* init = NULL;
    if(!lit_astparser_check(prs, LIT_ASTTOKTYP_SEMICOLON))
    {
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWVAR))
        {
            var = lit_astparser_parsevardecl(prs);
        }
        else
        {
            init = lit_astparser_parseexpr(prs);
        }
    }
    bool cstyle = !lit_astparser_match(prs, LIT_ASTTOKTYP_KWIN);
    LitAstExpression* condition = NULL;
    LitAstExpression* increment = NULL;
    if(cstyle)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_SEMICOLON, "';'");
        condition = lit_astparser_check(prs, LIT_ASTTOKTYP_SEMICOLON) ? NULL : lit_astparser_parseexpr(prs);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_SEMICOLON, "';'");
        increment = lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTPAREN) ? NULL : lit_astparser_parseexpr(prs);
    }
    else
    {
        condition = lit_astparser_parseexpr(prs);
        if(var == NULL)
        {
            lit_astparser_failfmt(prs, "for-loops using in-iteration must declare a new variable");
        }
    }
    if(hadparen)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')'");
    }
    return (LitAstExpression*)lit_ast_makeforstmt(line, init, var, condition, increment, lit_astparser_parsestmt(prs), cstyle);
}

LitAstExpression* lit_astparser_parsewhile(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    bool hadparen = lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTPAREN);
    LitAstExpression* condition = lit_astparser_parseexpr(prs);
    if(hadparen)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')'");
    }
    LitAstExpression* body = lit_astparser_parsestmt(prs);
    return (LitAstExpression*)lit_ast_makewhilestmt(line, condition, body);
}

LitAstExpression* lit_astparser_parsefunction(LitAstParser* prs)
{
    size_t line;
    size_t namelen;
    bool isexport;
    bool noname;
    const char* fnname;
    noname = false;
    fnname = "anonymous";
    namelen = strlen(fnname);
    isexport = prs->previous.type == LIT_ASTTOKTYP_KWEXPORT;
    if(isexport)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_KWFUNCTION, "'function' after 'export'");
    }
    line = prs->previous.line;
    if(lit_astparser_check(prs, LIT_ASTTOKTYP_IDENTIFIER))
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "function name");
        fnname = prs->previous.start;
        namelen = prs->previous.length;
    }
    else
    {
        noname = true;
    }
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_DOT))
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "function name");
        LitAstFunctionExpr* lambda = lit_ast_makelambdaexpr(line);
        LitAstIndexSetExpr* to = lit_ast_makesetexpr(line, (LitAstExpression*)lit_ast_makevargetexpr(line, fnname, namelen), prs->previous.start, prs->previous.length, (LitAstExpression*)lambda);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTPAREN, "'(' after function name");
        LitAstCompiler stackcc;
        lit_astparser_compilerinit(prs, &stackcc);
        lit_astparser_scopebegin(prs);
        lit_astparser_parseparams(prs, &lambda->parameters);
        if(lambda->parameters.listcount > 255)
        {
            lit_astparser_failfmt(prs, "function can't have more than 255 arguments, got %i", (int)lambda->parameters.listcount);
        }
        lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after function arguments");
        lambda->body = lit_astparser_parsestmt(prs);
        lit_astparser_scopeend(prs);
        lit_astparser_compilerend(prs, &stackcc);
        return (LitAstExpression*)lit_ast_makeexprstmt(line, (LitAstExpression*)to);
    }
    LitAstFunctionExpr* function;

    if(noname)
    {
        function = lit_ast_makelambdaexpr(line);
    }
    else
    {
        function = lit_ast_makefuncdefstmt(line, fnname, namelen);
    }
    function->exported = isexport;
    lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTPAREN, "'(' after function name");
    LitAstCompiler stackcc;
    lit_astparser_compilerinit(prs, &stackcc);
    lit_astparser_scopebegin(prs);
    lit_astparser_parseparams(prs, &function->parameters);
    if(function->parameters.listcount > 255)
    {
        lit_astparser_failfmt(prs, "function can't have more than 255 arguments, got %i", (int)function->parameters.listcount);
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after function arguments");
    function->body = lit_astparser_parsestmt(prs);
    lit_astparser_scopeend(prs);
    lit_astparser_compilerend(prs, &stackcc);
    return (LitAstExpression*)function;
}

LitAstExpression* lit_astparser_parsereturn(LitAstParser* prs)
{
    size_t line = prs->previous.line;
    LitAstExpression* expr = NULL;
    if(!lit_astparser_check(prs, LIT_ASTTOKTYP_LINEFEED) && !lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTBRACE))
    {
        expr = lit_astparser_parseexpr(prs);
    }
    return (LitAstExpression*)lit_ast_makereturnstmt(line, expr);
}

LitAstExpression* lit_astparser_parsefield(LitAstParser* prs, LitString* name, bool is_static)
{
    size_t line = prs->previous.line;
    LitAstExpression* getter = NULL;
    LitAstExpression* setter = NULL;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_ARROW))
    {
        getter = lit_astparser_parsestmt(prs);
    }
    else
    {
        /* will be LIT_ASTTOKTYP_LEFTBRACE, otherwise this method won't be called */
        lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACE);
        lit_astparser_ignorelinefeeds(prs);
        if(lit_astparser_matchident(prs, "get"))
        {
            /* ignore it if it's present */
            lit_astparser_match(prs, LIT_ASTTOKTYP_ARROW);
            getter = lit_astparser_parsestmt(prs);
        }
        lit_astparser_ignorelinefeeds(prs);
        if(lit_astparser_matchident(prs, "set"))
        {
            /* Ignore it if it's present */
            lit_astparser_match(prs, LIT_ASTTOKTYP_ARROW);
            setter = lit_astparser_parsestmt(prs);
        }
        if(getter == NULL && setter == NULL)
        {
            lit_astparser_failfmt(prs, "expected declaration of either getter or setter, got none");
        }
        lit_astparser_ignorelinefeeds(prs);
        lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACE, "'}' after field declaration");
    }
    return (LitAstExpression*)lit_ast_makefieldstmt(line, name, getter, setter, is_static);
}

static LitAstTokType operators[] = { LIT_ASTTOKTYP_PLUS,         LIT_ASTTOKTYP_MINUS, LIT_ASTTOKTYP_STAR,       LIT_ASTTOKTYP_PERCENT, LIT_ASTTOKTYP_SLASH,         LIT_ASTTOKTYP_SHARP,

                                    LIT_ASTTOKTYP_BANG,         LIT_ASTTOKTYP_LESS,  LIT_ASTTOKTYP_LESSEQUAL, LIT_ASTTOKTYP_GREATER, LIT_ASTTOKTYP_GREATEREQUAL, LIT_ASTTOKTYP_EQUALEQUAL,

                                    LIT_ASTTOKTYP_LEFTBRACKET,

                                    LIT_ASTTOKTYP_EOF };

LitAstExpression* lit_astparser_parsemethod(LitAstParser* prs, bool is_static)
{
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWSTATIC))
    {
        is_static = true;
    }
    LitString* name = NULL;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWOPERATOR))
    {
        if(is_static)
        {
            lit_astparser_failfmt(prs, "operator methods can't be static or defined in static classes");
        }
        size_t i = 0;
        while(operators[i] != LIT_ASTTOKTYP_EOF)
        {
            if(lit_astparser_match(prs, operators[i]))
            {
                break;
            }
            i++;
        }
        if(prs->previous.type == LIT_ASTTOKTYP_LEFTBRACKET)
        {
            lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACKET, "']' after '[' in op method declaration");
            name = lit_string_copylen(prs->pstate, "[]", 2);
        }
        else
        {
            name = lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length);
        }
    }
    else
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "method name");
        name = lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length);
        if(lit_astparser_check(prs, LIT_ASTTOKTYP_LEFTBRACE) || lit_astparser_check(prs, LIT_ASTTOKTYP_ARROW))
        {
            return lit_astparser_parsefield(prs, name, is_static);
        }
    }
    LitAstMethodExpr* method = lit_ast_makemethoddefstmt(prs->previous.line, name, is_static);
    LitAstCompiler stackcc;
    lit_astparser_compilerinit(prs, &stackcc);
    lit_astparser_scopebegin(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTPAREN, "'(' after method name");
    lit_astparser_parseparams(prs, &method->parameters);
    if(method->parameters.listcount > 255)
    {
        lit_astparser_failfmt(prs, "function can't have more than 255 arguments, got %i", (int)method->parameters.listcount);
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTPAREN, "')' after method arguments");
    method->body = lit_astparser_parsestmt(prs);
    lit_astparser_scopeend(prs);
    lit_astparser_compilerend(prs, &stackcc);
    return (LitAstExpression*)method;
}

LitAstExpression* lit_astparser_parseclass(LitAstParser* prs)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }
    size_t line = prs->previous.line;
    bool is_static = prs->previous.type == LIT_ASTTOKTYP_KWSTATIC;
    if(is_static)
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_KWCLASS, "'class' after 'static'");
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "class name after 'class'");
    LitString* name = lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length);
    LitString* super = NULL;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_COLON))
    {
        lit_astparser_consume(prs, LIT_ASTTOKTYP_IDENTIFIER, "super class name after ':'");
        super = lit_string_copylen(prs->pstate, prs->previous.start, prs->previous.length);
        if(super == name)
        {
            lit_astparser_failfmt(prs, "class cannot inherit itself");
        }
    }
    LitAstClassExpr* klass = lit_ast_makeclassdefstmt(line, name, super);
    lit_astparser_ignorelinefeeds(prs);
    lit_astparser_consume(prs, LIT_ASTTOKTYP_LEFTBRACE, "'{' before class body");
    lit_astparser_ignorelinefeeds(prs);
    bool finishedparsingfields = false;
    while(!lit_astparser_check(prs, LIT_ASTTOKTYP_RIGHTBRACE))
    {
        bool fieldisstatic = false;
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWSTATIC))
        {
            fieldisstatic = true;
            if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWVAR))
            {
                if(finishedparsingfields)
                {
                    lit_astparser_failfmt(prs, "all static fields must be defined before the methods");
                }
                LitAstExpression* var = lit_astparser_parsevardecl(prs);
                if(var != NULL)
                {
                    lit_dynlistexpr_push(&klass->staticfields, var);
                }
                lit_astparser_ignorelinefeeds(prs);
                continue;
            }
            else
            {
                finishedparsingfields = true;
            }
        }
        LitAstExpression* method = lit_astparser_parsemethod(prs, is_static || fieldisstatic);
        if(method != NULL)
        {
            lit_dynlistexpr_push(&klass->staticfields, method);
        }
        lit_astparser_ignorelinefeeds(prs);
        if(lit_astparser_match(prs, LIT_ASTTOKTYP_SEMICOLON))
        {
        }
        lit_astparser_ignorelinefeeds(prs);
    }
    lit_astparser_consume(prs, LIT_ASTTOKTYP_RIGHTBRACE, "'}' after class body");
    return (LitAstExpression*)klass;
}

void lit_astparser_sync(LitAstParser* prs)
{
    prs->panic_mode = false;
    while(prs->current.type != LIT_ASTTOKTYP_EOF)
    {
        if(prs->previous.type == LIT_ASTTOKTYP_LINEFEED)
        {
            longjmp(jumpbuffer, 1);
            return;
        }
        switch(prs->current.type)
        {
            case LIT_ASTTOKTYP_KWCLASS:
            case LIT_ASTTOKTYP_KWFUNCTION:
            case LIT_ASTTOKTYP_KWEXPORT:
            case LIT_ASTTOKTYP_KWVAR:
            case LIT_ASTTOKTYP_KWCONST:
            case LIT_ASTTOKTYP_KWFOR:
            case LIT_ASTTOKTYP_KWSTATIC:
            case LIT_ASTTOKTYP_KWIF:
            case LIT_ASTTOKTYP_KWWHILE:
            case LIT_ASTTOKTYP_KWRETURN:
            {
                longjmp(jumpbuffer, 1);
                return;
            }
            default:
            {
                lit_astparser_advance(prs);
            }
        }
    }
}

LitAstExpression* lit_astparser_parsestmt(LitAstParser* prs)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }
    lit_astparser_ignorelinefeeds(prs);
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWVAR) || lit_astparser_match(prs, LIT_ASTTOKTYP_KWCONST))
    {
        return lit_astparser_parsevardecl(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWIF))
    {
        return lit_astparser_parseif(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWTRY))
    {
        return lit_astparser_parsetry(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWTHROW))
    {
        return lit_astparser_parsethrow(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWFOR))
    {
        return lit_astparser_parsefor(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWWHILE))
    {
        return lit_astparser_parsewhile(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWCONTINUE))
    {
        return (LitAstExpression*)lit_ast_makecontinuestmt(prs->previous.line);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWBREAK))
    {
        return (LitAstExpression*)lit_ast_makebreakstmt(prs->previous.line);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWFUNCTION) || lit_astparser_match(prs, LIT_ASTTOKTYP_KWEXPORT))
    {
        return lit_astparser_parsefunction(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWRETURN))
    {
        return lit_astparser_parsereturn(prs);
    }
    else if(lit_astparser_match(prs, LIT_ASTTOKTYP_LEFTBRACE))
    {
        lit_astparser_ignorelinefeeds(prs);
        return lit_astparser_parseblock(prs);
    }
    LitAstExpression* expr = lit_astparser_parseexpr(prs);
    return expr == NULL ? NULL : (LitAstExpression*)lit_ast_makeexprstmt(prs->previous.line, expr);
}

LitAstExpression* lit_astparser_parsedecl(LitAstParser* prs)
{
    LitAstExpression* expr = NULL;
    if(lit_astparser_match(prs, LIT_ASTTOKTYP_KWCLASS) || lit_astparser_match(prs, LIT_ASTTOKTYP_KWSTATIC))
    {
        expr = lit_astparser_parseclass(prs);
    }
    else
    {
        expr = lit_astparser_parsestmt(prs);
    }
    return expr;
}

bool lit_astparser_parsesource(LitAstParser* prs, const char* filename, const char* source, LitDynListExpr* statements)
{
    prs->had_error = false;
    prs->panic_mode = false;
    lit_astlex_init(prs->pstate, prs->pstate->lexer, filename, source);
    LitAstCompiler stackcc;
    lit_astparser_compilerinit(prs, &stackcc);
    lit_astparser_advance(prs);
    lit_astparser_ignorelinefeeds(prs);
    if(!lit_astparser_isatend(prs))
    {
        do
        {
            LitAstExpression* expr = lit_astparser_parsedecl(prs);
            if(expr != NULL)
            {
                lit_dynlistexpr_push(statements, expr);
            }
            if(!lit_astparser_matchlinefeed(prs))
            {
                if(lit_astparser_match(prs, LIT_ASTTOKTYP_EOF))
                {
                    break;
                }
            }
        } while(!lit_astparser_isatend(prs));
    }
    return prs->had_error || prs->pstate->lexer->had_error;
}


LitAstRule lit_astparser_makerule(LitPrefixParseFn prefix, LitInfixParseFn infix, LitPrecedence precedence)
{
    LitAstRule pr;
    pr.prefix = prefix;
    pr.infix = infix;
    pr.precedence = precedence;
    return pr;
}

void lit_astparser_setuprules()
{
    g_astparserules[LIT_ASTTOKTYP_LEFTPAREN] = lit_astparser_makerule(lit_astparser_rulegroupingorlambda, lit_astparser_parsecall, LIT_ASTPREC_CALL);
    g_astparserules[LIT_ASTTOKTYP_PLUS] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_TERM);
    g_astparserules[LIT_ASTTOKTYP_MINUS] = lit_astparser_makerule(lit_astparser_ruleunary, lit_astparser_rulebinary, LIT_ASTPREC_TERM);
    g_astparserules[LIT_ASTTOKTYP_BANG] = lit_astparser_makerule(lit_astparser_ruleunary, lit_astparser_rulebinary, LIT_ASTPREC_IS);
    g_astparserules[LIT_ASTTOKTYP_STAR] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_FACTOR);
    g_astparserules[LIT_ASTTOKTYP_STARSTAR] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_FACTOR);
    g_astparserules[LIT_ASTTOKTYP_SLASH] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_FACTOR);
    g_astparserules[LIT_ASTTOKTYP_SHARP] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_FACTOR);
    g_astparserules[LIT_ASTTOKTYP_BAR] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_BOR);
    g_astparserules[LIT_ASTTOKTYP_AMPERSAND] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_BAND);
    g_astparserules[LIT_ASTTOKTYP_TILDE] = lit_astparser_makerule(lit_astparser_ruleunary, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_CARET] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_BXOR);
    g_astparserules[LIT_ASTTOKTYP_LESSLESS] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_SHIFT);
    g_astparserules[LIT_ASTTOKTYP_GREATERGREATER] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_SHIFT);
    g_astparserules[LIT_ASTTOKTYP_PERCENT] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_FACTOR);
    g_astparserules[LIT_ASTTOKTYP_KWIS] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_IS);
    g_astparserules[LIT_ASTTOKTYP_NUMBER] = lit_astparser_makerule(lit_astparser_rulenumber, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWTRUE] = lit_astparser_makerule(lit_astparser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWFALSE] = lit_astparser_makerule(lit_astparser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWNULL] = lit_astparser_makerule(lit_astparser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_BANGEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_EQUALITY);
    g_astparserules[LIT_ASTTOKTYP_EQUALEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_EQUALITY);
    g_astparserules[LIT_ASTTOKTYP_GREATER] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_COMPARISON);
    g_astparserules[LIT_ASTTOKTYP_GREATEREQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_COMPARISON);
    g_astparserules[LIT_ASTTOKTYP_LESS] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_COMPARISON);
    g_astparserules[LIT_ASTTOKTYP_LESSEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulebinary, LIT_ASTPREC_COMPARISON);
    g_astparserules[LIT_ASTTOKTYP_STRING] = lit_astparser_makerule(lit_astparser_rulestring, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_STRTEMPLATE] = lit_astparser_makerule(lit_astparser_ruleinterpolation, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_IDENTIFIER] = lit_astparser_makerule(lit_astparser_rulevarexpr, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWNEW] = lit_astparser_makerule(lit_astparser_rulenewexpr, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_PLUSEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_MINUSEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_STAREQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_SLASHEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_SHARPEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_PERCENTEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_CARETEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_BAREQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_AMPERSANDEQUAL] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_ASSIGNMENT);
    g_astparserules[LIT_ASTTOKTYP_PLUSPLUS] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_COMPOUND);
    g_astparserules[LIT_ASTTOKTYP_MINUSMINUS] = lit_astparser_makerule(NULL, lit_astparser_rulecompound, LIT_ASTPREC_COMPOUND);
    g_astparserules[LIT_ASTTOKTYP_AMPERSANDAMPERSAND] = lit_astparser_makerule(NULL, lit_astparser_rulelogicaland, LIT_ASTPREC_AND);
    g_astparserules[LIT_ASTTOKTYP_BARBAR] = lit_astparser_makerule(NULL, lit_astparser_rulelogicalor, LIT_ASTPREC_OR);
    g_astparserules[LIT_ASTTOKTYP_QUESTIONQUESTION] = lit_astparser_makerule(NULL, lit_astparser_rulenullfilter, LIT_ASTPREC_NULL);
    g_astparserules[LIT_ASTTOKTYP_DOT] = lit_astparser_makerule(NULL, lit_astparser_ruledot, LIT_ASTPREC_CALL);
#if 0
        g_astparserules[LIT_ASTTOKTYP_SMALLARROW] = lit_astparser_makerule(NULL, lit_astparser_ruledot, LIT_ASTPREC_CALL);
#endif
    g_astparserules[LIT_ASTTOKTYP_DOTDOT] = lit_astparser_makerule(NULL, lit_astparser_rulerange, LIT_ASTPREC_RANGE);
    g_astparserules[LIT_ASTTOKTYP_DOTDOTDOT] = lit_astparser_makerule(lit_astparser_rulevarexpr, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_LEFTBRACKET] = lit_astparser_makerule(lit_astparser_rulearray, lit_astparser_parsesubscript, LIT_ASTPREC_CALL);
    g_astparserules[LIT_ASTTOKTYP_LEFTBRACE] = lit_astparser_makerule(lit_astparser_ruleobject, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWTHIS] = lit_astparser_makerule(lit_astparser_rulethis, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWSUPER] = lit_astparser_makerule(lit_astparser_rulesuper, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_QUESTION] = lit_astparser_makerule(NULL, lit_astparser_ruleternaryorquestion, LIT_ASTPREC_EQUALITY);
    g_astparserules[LIT_ASTTOKTYP_KWREF] = lit_astparser_makerule(lit_astparser_rulereference, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_REFSYM] = lit_astparser_makerule(lit_astparser_ruleunary, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_SEMICOLON] = lit_astparser_makerule(lit_astparser_rulenothing, NULL, LIT_ASTPREC_NONE);
    g_astparserules[LIT_ASTTOKTYP_KWFUNCTION] = lit_astparser_makerule(lit_astparser_rulefunction, NULL, LIT_ASTPREC_NONE);
}


const char* lit_astprint_tokname(int t)
{
    switch(t)
    {
        case LIT_ASTTOKTYP_LEFTPAREN: return "LIT_ASTTOKTYP_LEFTPAREN";
        case LIT_ASTTOKTYP_RIGHTPAREN: return "LIT_ASTTOKTYP_RIGHTPAREN";
        case LIT_ASTTOKTYP_LEFTBRACE: return "LIT_ASTTOKTYP_LEFTBRACE";
        case LIT_ASTTOKTYP_RIGHTBRACE: return "LIT_ASTTOKTYP_RIGHTBRACE";
        case LIT_ASTTOKTYP_LEFTBRACKET: return "LIT_ASTTOKTYP_LEFTBRACKET";
        case LIT_ASTTOKTYP_RIGHTBRACKET: return "LIT_ASTTOKTYP_RIGHTBRACKET";
        case LIT_ASTTOKTYP_COMMA: return "LIT_ASTTOKTYP_COMMA";
        case LIT_ASTTOKTYP_SEMICOLON: return "LIT_ASTTOKTYP_SEMICOLON";
        case LIT_ASTTOKTYP_COLON: return "LIT_ASTTOKTYP_COLON";
        case LIT_ASTTOKTYP_BAREQUAL: return "LIT_ASTTOKTYP_BAREQUAL";
        case LIT_ASTTOKTYP_BAR: return "LIT_ASTTOKTYP_BAR";
        case LIT_ASTTOKTYP_BARBAR: return "LIT_ASTTOKTYP_BARBAR";
        case LIT_ASTTOKTYP_AMPERSANDEQUAL: return "LIT_ASTTOKTYP_AMPERSANDEQUAL";
        case LIT_ASTTOKTYP_AMPERSAND: return "LIT_ASTTOKTYP_AMPERSAND";
        case LIT_ASTTOKTYP_AMPERSANDAMPERSAND: return "LIT_ASTTOKTYP_AMPERSANDAMPERSAND";
        case LIT_ASTTOKTYP_BANG: return "LIT_ASTTOKTYP_BANG";
        case LIT_ASTTOKTYP_BANGEQUAL: return "LIT_ASTTOKTYP_BANGEQUAL";
        case LIT_ASTTOKTYP_EQUAL: return "LIT_ASTTOKTYP_EQUAL";
        case LIT_ASTTOKTYP_EQUALEQUAL: return "LIT_ASTTOKTYP_EQUALEQUAL";
        case LIT_ASTTOKTYP_GREATER: return "LIT_ASTTOKTYP_GREATER";
        case LIT_ASTTOKTYP_GREATEREQUAL: return "LIT_ASTTOKTYP_GREATEREQUAL";
        case LIT_ASTTOKTYP_GREATERGREATER: return "LIT_ASTTOKTYP_GREATERGREATER";
        case LIT_ASTTOKTYP_LESS: return "LIT_ASTTOKTYP_LESS";
        case LIT_ASTTOKTYP_LESSEQUAL: return "LIT_ASTTOKTYP_LESSEQUAL";
        case LIT_ASTTOKTYP_LESSLESS: return "LIT_ASTTOKTYP_LESSLESS";
        case LIT_ASTTOKTYP_PLUS: return "LIT_ASTTOKTYP_PLUS";
        case LIT_ASTTOKTYP_PLUSEQUAL: return "LIT_ASTTOKTYP_PLUSEQUAL";
        case LIT_ASTTOKTYP_PLUSPLUS: return "LIT_ASTTOKTYP_PLUSPLUS";
        case LIT_ASTTOKTYP_MINUS: return "LIT_ASTTOKTYP_MINUS";
        case LIT_ASTTOKTYP_MINUSEQUAL: return "LIT_ASTTOKTYP_MINUSEQUAL";
        case LIT_ASTTOKTYP_MINUSMINUS: return "LIT_ASTTOKTYP_MINUSMINUS";
        case LIT_ASTTOKTYP_STAR: return "LIT_ASTTOKTYP_STAR";
        case LIT_ASTTOKTYP_STAREQUAL: return "LIT_ASTTOKTYP_STAREQUAL";
        case LIT_ASTTOKTYP_STARSTAR: return "LIT_ASTTOKTYP_STARSTAR";
        case LIT_ASTTOKTYP_SLASH: return "LIT_ASTTOKTYP_SLASH";
        case LIT_ASTTOKTYP_SLASHEQUAL: return "LIT_ASTTOKTYP_SLASHEQUAL";
        case LIT_ASTTOKTYP_QUESTION: return "LIT_ASTTOKTYP_QUESTION";
        case LIT_ASTTOKTYP_QUESTIONQUESTION: return "LIT_ASTTOKTYP_QUESTIONQUESTION";
        case LIT_ASTTOKTYP_PERCENT: return "LIT_ASTTOKTYP_PERCENT";
        case LIT_ASTTOKTYP_PERCENTEQUAL: return "LIT_ASTTOKTYP_PERCENTEQUAL";
        case LIT_ASTTOKTYP_ARROW: return "LIT_ASTTOKTYP_ARROW";
        case LIT_ASTTOKTYP_SMALLARROW: return "LIT_ASTTOKTYP_SMALLARROW";
        case LIT_ASTTOKTYP_TILDE: return "LIT_ASTTOKTYP_TILDE";
        case LIT_ASTTOKTYP_CARET: return "LIT_ASTTOKTYP_CARET";
        case LIT_ASTTOKTYP_CARETEQUAL: return "LIT_ASTTOKTYP_CARETEQUAL";
        case LIT_ASTTOKTYP_DOT: return "LIT_ASTTOKTYP_DOT";
        case LIT_ASTTOKTYP_DOTDOT: return "LIT_ASTTOKTYP_DOTDOT";
        case LIT_ASTTOKTYP_DOTDOTDOT: return "LIT_ASTTOKTYP_DOTDOTDOT";
        case LIT_ASTTOKTYP_SHARP: return "LIT_ASTTOKTYP_SHARP";
        case LIT_ASTTOKTYP_SHARPEQUAL: return "LIT_ASTTOKTYP_SHARPEQUAL";
        case LIT_ASTTOKTYP_IDENTIFIER: return "LIT_ASTTOKTYP_IDENTIFIER";
        case LIT_ASTTOKTYP_STRING: return "LIT_ASTTOKTYP_STRING";
        case LIT_ASTTOKTYP_STRTEMPLATE: return "LIT_ASTTOKTYP_STRTEMPLATE";
        case LIT_ASTTOKTYP_NUMBER: return "LIT_ASTTOKTYP_NUMBER";
        case LIT_ASTTOKTYP_KWCLASS: return "LIT_ASTTOKTYP_KWCLASS";
        case LIT_ASTTOKTYP_KWELSE: return "LIT_ASTTOKTYP_KWELSE";
        case LIT_ASTTOKTYP_KWFALSE: return "LIT_ASTTOKTYP_KWFALSE";
        case LIT_ASTTOKTYP_KWFOR: return "LIT_ASTTOKTYP_KWFOR";
        case LIT_ASTTOKTYP_KWFUNCTION: return "LIT_ASTTOKTYP_KWFUNCTION";
        case LIT_ASTTOKTYP_KWIF: return "LIT_ASTTOKTYP_KWIF";
        case LIT_ASTTOKTYP_KWNULL: return "LIT_ASTTOKTYP_KWNULL";
        case LIT_ASTTOKTYP_KWRETURN: return "LIT_ASTTOKTYP_KWRETURN";
        case LIT_ASTTOKTYP_KWSUPER: return "LIT_ASTTOKTYP_KWSUPER";
        case LIT_ASTTOKTYP_KWTHIS: return "LIT_ASTTOKTYP_KWTHIS";
        case LIT_ASTTOKTYP_KWTRUE: return "LIT_ASTTOKTYP_KWTRUE";
        case LIT_ASTTOKTYP_KWVAR: return "LIT_ASTTOKTYP_KWVAR";
        case LIT_ASTTOKTYP_KWWHILE: return "LIT_ASTTOKTYP_KWWHILE";
        case LIT_ASTTOKTYP_KWCONTINUE: return "LIT_ASTTOKTYP_KWCONTINUE";
        case LIT_ASTTOKTYP_KWBREAK: return "LIT_ASTTOKTYP_KWBREAK";
        case LIT_ASTTOKTYP_KWNEW: return "LIT_ASTTOKTYP_KWNEW";
        case LIT_ASTTOKTYP_KWEXPORT: return "LIT_ASTTOKTYP_KWEXPORT";
        case LIT_ASTTOKTYP_KWIS: return "LIT_ASTTOKTYP_KWIS";
        case LIT_ASTTOKTYP_KWSTATIC: return "LIT_ASTTOKTYP_KWSTATIC";
        case LIT_ASTTOKTYP_KWOPERATOR: return "LIT_ASTTOKTYP_KWOPERATOR";
        case LIT_ASTTOKTYP_KWIN: return "LIT_ASTTOKTYP_KWIN";
        case LIT_ASTTOKTYP_KWCONST: return "LIT_ASTTOKTYP_KWCONST";
        case LIT_ASTTOKTYP_KWREF: return "LIT_ASTTOKTYP_KWREF";
        case LIT_ASTTOKTYP_ERROR: return "LIT_ASTTOKTYP_ERROR";
        case LIT_ASTTOKTYP_EOF: return "LIT_ASTTOKTYP_EOF";

    }
    return "unknown";
}
const char* lit_astprint_tokopstring(int t)
{
    switch(t)
    {
        case LIT_ASTTOKTYP_LINEFEED: return "<linefeed>";
        case LIT_ASTTOKTYP_LEFTPAREN: return "(";
        case LIT_ASTTOKTYP_RIGHTPAREN: return ")";
        case LIT_ASTTOKTYP_LEFTBRACE: return "{";
        case LIT_ASTTOKTYP_RIGHTBRACE: return "}";
        case LIT_ASTTOKTYP_LEFTBRACKET: return "[";
        case LIT_ASTTOKTYP_RIGHTBRACKET: return "]";
        case LIT_ASTTOKTYP_COMMA: return ",";
        case LIT_ASTTOKTYP_SEMICOLON: return ";";
        case LIT_ASTTOKTYP_COLON: return ":";
        case LIT_ASTTOKTYP_BAREQUAL: return "|=";
        case LIT_ASTTOKTYP_BAR: return "|";
        case LIT_ASTTOKTYP_BARBAR: return "||";
        case LIT_ASTTOKTYP_AMPERSANDEQUAL: return "&=";
        case LIT_ASTTOKTYP_AMPERSAND: return "&";
        case LIT_ASTTOKTYP_AMPERSANDAMPERSAND: return "&&";
        case LIT_ASTTOKTYP_BANG: return "!";
        case LIT_ASTTOKTYP_BANGEQUAL: return "!=";
        case LIT_ASTTOKTYP_EQUAL: return "=";
        case LIT_ASTTOKTYP_EQUALEQUAL: return "==";
        case LIT_ASTTOKTYP_GREATER: return ">";
        case LIT_ASTTOKTYP_GREATEREQUAL: return ">=";
        case LIT_ASTTOKTYP_GREATERGREATER: return ">>";
        case LIT_ASTTOKTYP_LESS: return "<";
        case LIT_ASTTOKTYP_LESSEQUAL: return "<=";
        case LIT_ASTTOKTYP_LESSLESS: return "<<";
        case LIT_ASTTOKTYP_PLUS: return "+";
        case LIT_ASTTOKTYP_PLUSEQUAL: return "+=";
        case LIT_ASTTOKTYP_PLUSPLUS: return "++";
        case LIT_ASTTOKTYP_MINUS: return "-";
        case LIT_ASTTOKTYP_MINUSEQUAL: return "-=";
        case LIT_ASTTOKTYP_MINUSMINUS: return "--";
        case LIT_ASTTOKTYP_STAR: return "*";
        case LIT_ASTTOKTYP_STAREQUAL: return "*=";
        case LIT_ASTTOKTYP_STARSTAR: return "**";
        case LIT_ASTTOKTYP_SLASH: return "/";
        case LIT_ASTTOKTYP_SLASHEQUAL: return "/=";
        case LIT_ASTTOKTYP_QUESTION: return "?";
        case LIT_ASTTOKTYP_QUESTIONQUESTION: return "??";
        case LIT_ASTTOKTYP_PERCENT: return "%";
        case LIT_ASTTOKTYP_PERCENTEQUAL: return "%=";
        case LIT_ASTTOKTYP_ARROW: return "=>";
        case LIT_ASTTOKTYP_SMALLARROW: return "->";
        case LIT_ASTTOKTYP_TILDE: return "~";
        case LIT_ASTTOKTYP_CARET: return "^";
        case LIT_ASTTOKTYP_CARETEQUAL: return "^=";
        case LIT_ASTTOKTYP_DOT: return ".";
        case LIT_ASTTOKTYP_DOTDOT: return "..";
        case LIT_ASTTOKTYP_DOTDOTDOT: return "...";
        case LIT_ASTTOKTYP_SHARP: return "#";
        case LIT_ASTTOKTYP_SHARPEQUAL: return "#=";
        case LIT_ASTTOKTYP_KWCLASS: return "class";
        case LIT_ASTTOKTYP_KWELSE: return "else";
        case LIT_ASTTOKTYP_KWFALSE: return "false";
        case LIT_ASTTOKTYP_KWFOR: return "for";
        case LIT_ASTTOKTYP_KWFUNCTION: return "function";
        case LIT_ASTTOKTYP_KWIF: return "if";
        case LIT_ASTTOKTYP_KWNULL: return "null";
        case LIT_ASTTOKTYP_KWRETURN: return "return";
        case LIT_ASTTOKTYP_KWSUPER: return "super";
        case LIT_ASTTOKTYP_KWTHIS: return "this";
        case LIT_ASTTOKTYP_KWTRUE: return "true";
        case LIT_ASTTOKTYP_KWVAR: return "var";
        case LIT_ASTTOKTYP_KWWHILE: return "while";
        case LIT_ASTTOKTYP_KWCONTINUE: return "continue";
        case LIT_ASTTOKTYP_KWBREAK: return "break";
        case LIT_ASTTOKTYP_KWNEW: return "new";
        case LIT_ASTTOKTYP_KWEXPORT: return "export";
        case LIT_ASTTOKTYP_KWIS: return "is";
        case LIT_ASTTOKTYP_KWSTATIC: return "statis";
        case LIT_ASTTOKTYP_KWIN: return "in";
        case LIT_ASTTOKTYP_KWCONST: return "const";
        default:
            break;
    }
    return "<unknown>";
}

void lit_astprint_printfuncparams(LitAstPrinter* apr, LitDynListParam* params)
{
    size_t i;
    LitAstFuncParamExpr* param;
    lit_iostream_puts(apr->printer, "(");
    for(i=0; i<params->listcount; i++)
    {
        param = &params->listitems[i];
        lit_iostream_putlen(apr->printer, param->name, param->length);
        if(param->defaultval != NULL)
        {
            lit_iostream_puts(apr->printer, "=");
            lit_astprint_printexpression(apr, param->defaultval);
        }
        if((i+1) < params->listcount)
        {
            lit_iostream_puts(apr->printer, ", ");
        }
    }
    lit_iostream_puts(apr->printer, ")");
}

void lit_astprint_printexpression(LitAstPrinter* apr, LitAstExpression* expr)
{
    if(expr == NULL)
    {
        return;
    }
    switch(expr->type)
    {
        case LIT_ASTEXPRTYP_LITERAL:
            {
                LitAstLiteralValExpr* oex;
                oex = (LitAstLiteralValExpr*)expr;
                lit_value_printvalue(apr->printer, oex->value, true);
            }
            break;
        case LIT_ASTEXPRTYP_BINARY:
            {
                LitAstBinaryExpr* oex;
                oex = (LitAstBinaryExpr*)expr;
                lit_iostream_puts(apr->printer, "(");
                lit_astprint_printexpression(apr, oex->left);
                lit_iostream_puts(apr->printer, lit_astprint_tokopstring(oex->op));
                lit_astprint_printexpression(apr, oex->right);
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_UNARY:
            {
                LitAstUnaryExpr* oex;
                oex = (LitAstUnaryExpr*)expr;
                lit_iostream_puts(apr->printer, "(");
                lit_iostream_puts(apr->printer, lit_astprint_tokopstring(oex->op));
                lit_astprint_printexpression(apr, oex->right);
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_VARGET:
            {
                LitAstVarGetExpr* oex;
                oex = (LitAstVarGetExpr*)expr;
                lit_iostream_putlen(apr->printer, oex->name, oex->length);
            }
            break;
        case LIT_ASTEXPRTYP_ASSIGN:
            {
                LitAstAssignExpr* oex;
                oex = (LitAstAssignExpr*)expr;
                lit_astprint_printexpression(apr, oex->to);
                lit_iostream_puts(apr->printer, " = ");
                lit_astprint_printexpression(apr, oex->value);
            }
            break;
        case LIT_ASTEXPRTYP_CALL:
            {
                size_t i;
                size_t count;
                LitAstCallExpr* oex;
                oex = (LitAstCallExpr*)expr;
                count = oex->callargs.listcount;
                lit_astprint_printexpression(apr, oex->excallee);
                lit_iostream_puts(apr->printer, "(");
                for(i=0; i<count; i++)
                {
                    lit_astprint_printexpression(apr, (LitAstExpression*)oex->callargs.listitems[i]);
                    if((i+1) != count)
                    {
                        lit_iostream_puts(apr->printer, ", ");
                    }
                }
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_INDEXSET:
            {
                LitAstIndexSetExpr* oex;
                oex = (LitAstIndexSetExpr*)expr;
                lit_astprint_printexpression(apr, oex->where);
                lit_iostream_puts(apr->printer, "[\"");
                lit_iostream_putlen(apr->printer, oex->name, oex->length);
                lit_iostream_puts(apr->printer, "\"]");
                lit_iostream_puts(apr->printer, " = ");
                lit_astprint_printexpression(apr, oex->value);

            }
            break;
        case LIT_ASTEXPRTYP_INDEXGET:
            {
                LitAstIndexGetExpr* oex;
                oex = (LitAstIndexGetExpr*)expr;
                lit_astprint_printexpression(apr, oex->where);
                lit_iostream_puts(apr->printer, "[\"");
                lit_iostream_putlen(apr->printer, oex->name, oex->length);
                lit_iostream_puts(apr->printer, "\"]");
                if(oex->ignoreresult)
                {
                    /*lit_iostream_puts(apr->printer, ";\n");*/
                }
            }
            break;
        case LIT_ASTEXPRTYP_SUBSCRIPT:
            {
                LitAstSubscriptExpr* oex;
                oex = (LitAstSubscriptExpr*)expr;
                lit_astprint_printexpression(apr, oex->array);
                lit_iostream_puts(apr->printer, "[");
                lit_astprint_printexpression(apr, oex->index);
                lit_iostream_puts(apr->printer, "]");
            }
            break;
        case LIT_ASTEXPRTYP_FUNCANON:
            {
                LitAstFunctionExpr* oex;
                oex = (LitAstFunctionExpr*)expr;
                #if 0
                lit_astprint_printfuncparams(apr, &oex->parameters);
                lit_iostream_puts(apr->printer, " => ");
                #else
                lit_iostream_puts(apr->printer, "function");
                lit_astprint_printfuncparams(apr, &oex->parameters);
                #endif
                lit_astprint_printexpression(apr, oex->body);
            }
            break;
        case LIT_ASTEXPRTYP_ARRAY:
            {
                size_t i;
                size_t count;
                LitAstLiteralArrayExpr* oex;
                oex = (LitAstLiteralArrayExpr*)expr;
                count = oex->exvalues.listcount;
                lit_iostream_puts(apr->printer, "[");
                for(i=0; i<count; i++)
                {
                    lit_astprint_printexpression(apr, (LitAstExpression*)oex->exvalues.listitems[i]);
                    if((i+1) < count)
                    {
                        lit_iostream_puts(apr->printer, ", ");
                    }
                }
                lit_iostream_puts(apr->printer, "]");
            }
            break;
        case LIT_ASTEXPRTYP_OBJECT:
            {
                size_t i;
                size_t count;
                LitAstLiteralObjectExpr* oex;
                oex = (LitAstLiteralObjectExpr*)expr;
                count = oex->objexkeys.listcount;
                lit_iostream_puts(apr->printer, "{");
                for(i=0; i<count; i++)
                {
                    lit_value_printvalue(apr->printer, oex->objexkeys.listitems[i], true);
                    lit_iostream_puts(apr->printer, ": ");
                    lit_astprint_printexpression(apr, (LitAstExpression*)oex->objexvalues.listitems[i]);
                    if((i+1) != count)
                    {
                        lit_iostream_puts(apr->printer, ", ");
                    }
                }
                lit_iostream_puts(apr->printer, "}");
            }
            break;
        case LIT_ASTEXPRTYP_THIS:
            {
                LitAstThisExpr* oex;
                (void)oex;
                oex = (LitAstThisExpr*)expr;
                lit_iostream_puts(apr->printer, "this");
            }
            break;
        case LIT_ASTEXPRTYP_SUPER:
            {
                LitAstSuperExpr* oex;
                oex = (LitAstSuperExpr*)expr;
                lit_iostream_puts(apr->printer, "super");
                if(oex->methodname != NULL)
                {
                    lit_iostream_puts(apr->printer, ".");
                    lit_iostream_putlen(apr->printer, oex->methodname->strbuf.data, oex->methodname->strbuf.length);
                }
            }
            break;
        case LIT_ASTEXPRTYP_RANGE:
            {
                LitAstRangeExpr* oex;
                oex = (LitAstRangeExpr*)expr;
                lit_iostream_puts(apr->printer, "(");
                lit_astprint_printexpression(apr, oex->from);
                lit_iostream_puts(apr->printer, " .. ");
                lit_astprint_printexpression(apr, oex->to);
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_TERNARY:
            {
                LitAstTernaryExpr* oex;
                oex = (LitAstTernaryExpr*)expr;
                lit_iostream_puts(apr->printer, "(");
                lit_astprint_printexpression(apr, oex->condition);
                lit_iostream_puts(apr->printer, " ? ");
                lit_astprint_printexpression(apr, oex->branchif);
                lit_iostream_puts(apr->printer, " : ");
                lit_astprint_printexpression(apr, oex->branchelse);
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_INTERPOLATION:
            {
                size_t i;
                size_t count;
                LitAstStrTemplateExpr* oex;
                oex = (LitAstStrTemplateExpr*)expr;
                count = oex->expressions.listcount;
                lit_iostream_puts(apr->printer, "(\"\" + ");
                for(i=0; i<count; i++)
                {
                    lit_astprint_printexpression(apr, (LitAstExpression*)oex->expressions.listitems[i]);
                    if((i+1) != count)
                    {
                        lit_iostream_puts(apr->printer, " + ");
                    }
                }
                lit_iostream_puts(apr->printer, ")");
            }
            break;
        case LIT_ASTEXPRTYP_REFERENCE:
            {
                LitAstRefExpr* oex;
                oex = (LitAstRefExpr*)expr;
                lit_iostream_puts(apr->printer, "ref ");
                lit_astprint_printexpression(apr, oex->to);
                lit_iostream_puts(apr->printer, "");
            }
            break;
        case LIT_ASTEXPRTYP_EXPRESSION:
            {
                LitAstExprStmtExpr* oex;
                oex = (LitAstExprStmtExpr*)expr;
                lit_astprint_printexpression(apr, oex->exvalue);
                lit_iostream_puts(apr->printer, ";\n");
            }
            break;
        case LIT_ASTEXPRTYP_BLOCK:
            {
                LitAstBlockExpr* oex;
                oex = (LitAstBlockExpr*)expr;
                lit_iostream_printf(apr->printer, "{\n");
                lit_astprint_printexprlist(apr, &oex->statements);
                lit_iostream_printf(apr->printer, "}\n");
            }
            break;
        case LIT_ASTEXPRTYP_IF:
            {
                size_t i;
                size_t count;
                LitAstIfExpr* oex;
                oex = (LitAstIfExpr*)expr;
                lit_iostream_puts(apr->printer, "if(");
                lit_astprint_printexpression(apr, oex->condition);
                lit_iostream_puts(apr->printer, ")\n");
                lit_astprint_printexpression(apr, oex->branchif);
                if(oex->elseifcondlist != NULL)
                {
                    count = oex->elseifcondlist->listcount;
                    for(i=0; i<count; i++)
                    {
                        lit_iostream_puts(apr->printer, "else if(");
                        lit_astprint_printexpression(apr, (LitAstExpression*)oex->elseifcondlist->listitems[i]);
                        lit_iostream_puts(apr->printer, ")\n");
                        lit_astprint_printexpression(apr, (LitAstExpression*)oex->branchelseiflist->listitems[i]);
                    }
                }
                if(oex->branchelse != NULL)
                {
                    lit_iostream_puts(apr->printer, "else\n");
                    lit_astprint_printexpression(apr, oex->branchelse);
                }
            }
            break;
        case LIT_ASTEXPRTYP_WHILE:
            {
                LitAstWhileExpr* oex;
                oex = (LitAstWhileExpr*)expr;
                lit_iostream_puts(apr->printer, "while(");
                lit_astprint_printexpression(apr, oex->condition);
                lit_iostream_puts(apr->printer, ")\n");
                lit_astprint_printexpression(apr, oex->body);
            }
            break;
        case LIT_ASTEXPRTYP_FOR:
            {
                LitAstForExpr* oex;
                oex = (LitAstForExpr*)expr;
                lit_iostream_puts(apr->printer, "for(");
                if(oex->iscstyle)
                {
                    
                }
                else
                {
                    lit_astprint_printexpression(apr, oex->var);
                    lit_iostream_puts(apr->printer, " in ");
                    lit_astprint_printexpression(apr, oex->condition);
                }
                lit_iostream_puts(apr->printer, ")\n");
                lit_astprint_printexpression(apr, oex->body);
            }
            break;
        case LIT_ASTEXPRTYP_TRY:
            {
                LitAstTryExpr* oex = (LitAstTryExpr*)expr;
                lit_iostream_puts(apr->printer, "try\n");
                lit_astprint_printexpression(apr, oex->try_block);
                if(oex->catch_block != NULL)
                {
                    lit_iostream_puts(apr->printer, "catch");
                    if(oex->catch_var != NULL)
                    {
                        lit_iostream_printf(apr->printer, "(%.*s)", (int)oex->catch_var_len, oex->catch_var);
                    }
                    lit_iostream_puts(apr->printer, "\n");
                    lit_astprint_printexpression(apr, oex->catch_block);
                }
                if(oex->finally_block != NULL)
                {
                    lit_iostream_puts(apr->printer, "finally\n");
                    lit_astprint_printexpression(apr, oex->finally_block);
                }
            }
            break;
        case LIT_ASTEXPRTYP_THROW:
            {
                LitAstThrowExpr* oex = (LitAstThrowExpr*)expr;
                lit_iostream_puts(apr->printer, "throw ");
                lit_astprint_printexpression(apr, oex->exvalue);
                lit_iostream_puts(apr->printer, ";\n");
            }
            break;
        case LIT_ASTEXPRTYP_VARDECL:
            {
                LitAstVarDeclExpr* oex;
                oex = (LitAstVarDeclExpr*)expr;
                if(oex->isconstant)
                {
                    lit_iostream_puts(apr->printer, "const ");
                }
                else
                {
                    lit_iostream_puts(apr->printer, "var ");
                }
                lit_iostream_putlen(apr->printer, oex->name, oex->length);
                if(oex->init != NULL)
                {
                    lit_iostream_puts(apr->printer, " = ");
                    lit_astprint_printexpression(apr, oex->init);
                }
                lit_iostream_puts(apr->printer, ";\n");
            }
            break;
        case LIT_ASTEXPRTYP_CONTINUE:
            {
                LitAstContinueExpr* oex;
                (void)oex;
                oex = (LitAstContinueExpr*)expr;
                lit_iostream_puts(apr->printer, "continue;\n");
            }
            break;
        case LIT_ASTEXPRTYP_BREAK:
            {
                LitBreakStatement* oex;
                (void)oex;
                oex = (LitBreakStatement*)expr;
                lit_iostream_puts(apr->printer, "break;\n");
            }
            break;
        case LIT_ASTEXPRTYP_FUNCTION:
            {
                LitAstFunctionExpr* oex;
                oex = (LitAstFunctionExpr*)expr;
                if(oex->exported)
                {
                    lit_iostream_puts(apr->printer, "export ");
                }
                lit_iostream_puts(apr->printer, "function");
                if(oex->name != NULL)
                {
                    lit_iostream_puts(apr->printer, " ");
                    lit_iostream_putlen(apr->printer, oex->name, oex->length);
                }
                lit_astprint_printfuncparams(apr, &oex->parameters);
                lit_iostream_puts(apr->printer, "\n");
                lit_astprint_printexpression(apr, oex->body);
            }
            break;
        case LIT_ASTEXPRTYP_RETURN:
            {
                LitAstReturnExpr* oex;
                oex = (LitAstReturnExpr*)expr;
                lit_iostream_puts(apr->printer, "return");
                if(oex->exvalue != NULL)
                {
                    lit_iostream_puts(apr->printer, " ");
                    lit_astprint_printexpression(apr, oex->exvalue);
                }
                lit_iostream_puts(apr->printer, ";\n");
            }
            break;
        case LIT_ASTEXPRTYP_METHOD:
            {
                bool notoper;
                LitAstMethodExpr* oex;
                oex = (LitAstMethodExpr*)expr;
                notoper = lit_util_charisalpha(oex->name->strbuf.data[0]);
                if(!notoper)
                {
                    lit_iostream_puts(apr->printer, "operator ");
                }
                lit_iostream_putlen(apr->printer, oex->name->strbuf.data, oex->name->strbuf.length);
                lit_astprint_printfuncparams(apr, &oex->parameters);
                lit_iostream_puts(apr->printer, "\n");
                lit_astprint_printexpression(apr, oex->body);
            }
            break;
        case LIT_ASTEXPRTYP_CLASS:
            {
                size_t i;
                size_t count;
                LitAstClassExpr* oex;
                oex = (LitAstClassExpr*)expr;
                count = oex->staticfields.listcount;
                lit_iostream_puts(apr->printer, "class ");
                lit_iostream_putlen(apr->printer, oex->name->strbuf.data, oex->name->strbuf.length);
                if(oex->parent != NULL)
                {
                    lit_iostream_puts(apr->printer, ": ");
                    lit_iostream_putlen(apr->printer, oex->parent->strbuf.data, oex->parent->strbuf.length);
                }
                lit_iostream_puts(apr->printer, "\n{\n");
                for(i=0; i<count; i++)
                {
                    lit_astprint_printexpression(apr, (LitAstExpression*)oex->staticfields.listitems[i]);
                }
                lit_iostream_puts(apr->printer, "\n}\n");
            }
            break;
        case LIT_ASTEXPRTYP_FIELD:
            {
                LitAstFieldExpr* oex;
                oex = (LitAstFieldExpr*)expr;
                lit_iostream_puts(apr->printer, "<FIELD>");
                lit_iostream_putlen(apr->printer, oex->name->strbuf.data, oex->name->strbuf.length);
                lit_iostream_puts(apr->printer, ";\n");
            }
            break;

    }
}

void lit_astprint_printexprlist(LitAstPrinter* apr, LitDynListExpr* elist)
{
    size_t i;
    LitAstExpression* expr;
    for(i=0; i<elist->listcount; i++)
    {
        expr = (LitAstExpression*)elist->listitems[i];
        lit_astprint_printexpression(apr, expr);
    }
}

void lit_astprint_printbeginlist(FILE* ofh, LitDynListExpr* statements)
{
    LitAstPrinter apr;
    apr.indent = 0;
    apr.printer = lit_iostream_makeio(ofh, false);
    lit_iostream_puts(apr.printer, "<<<astdump begin>>>\n");
    lit_astprint_printexprlist(&apr, statements);
    lit_iostream_puts(apr.printer, "\n<<<astdump end>>>\n");
    lit_iostream_destroy(apr.printer);
}

void lit_astprint_printbeginone(FILE* ofh, LitAstExpression* expr)
{
    LitAstPrinter apr;
    apr.indent = 0;
    apr.printer = lit_iostream_makeio(ofh, false);
    lit_astprint_printexpression(&apr, expr);
    lit_iostream_destroy(apr.printer);
}


void lit_emitter_resolvestmtlist(LitAstEmitter* emt, LitDynListExpr* statements)
{
    size_t i;
    for(i = 0; i < statements->listcount; i++)
    {
        lit_emitter_resolvestatement(emt, statements->listitems[i]);
    }
}

void lit_emitter_init(LitState* state, LitAstEmitter* emt)
{
    lit_emitter_reset(state, emt);
    lit_dynlistpriv_init(&emt->privlist);
    lit_dynlistuint_init(&emt->breaks);
    lit_dynlistuint_init(&emt->continues);
}

void lit_emitter_reset(LitState* state, LitAstEmitter* emt)
{
    emt->pstate = state;
    emt->loop_start = 0;
    emt->emit_reference = 0;
    emt->class_name = NULL;
    emt->compiler = NULL;
    emt->chunk = NULL;
    emt->module = NULL;
    emt->class_has_super = false;
}

void lit_emitter_destroy(LitAstEmitter* emt)
{
    lit_dynlistuint_destroy(&emt->breaks);
    lit_dynlistuint_destroy(&emt->continues);
}

void lit_emitter_raiseerror(LitAstEmitter* emt, size_t line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_state_raiseerror(emt->pstate, LIT_ERROR_COMPILEERROR, lit_state_errorfmtv(emt->pstate, line, fmt, args)->strbuf.data);
    va_end(args);
}

size_t lit_emitter_emittmp(LitAstEmitter* emt)
{
    lit_chunk_push(emt->chunk, 0, emt->last_line);
    return emt->chunk->compiledcodecount - 1;
}

void lit_emitter_patchinstr(LitAstEmitter* emt, uint64_t position, uint64_t instruction)
{
    emt->chunk->compiledcodechunk[position] = instruction;
}

void lit_emitter_emitabc(LitAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, uint16_t b, uint16_t c)
{
    emt->last_line = fmax(line, emt->last_line);
    lit_chunk_push(emt->chunk, LIT_REG_FORMABCINST(opcode, a, b, c), emt->last_line);
}

void lit_emitter_emitabx(LitAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, uint32_t bx)
{
    emt->last_line = fmax(line, emt->last_line);
    lit_chunk_push(emt->chunk, LIT_REG_FORMABXINST(opcode, a, bx), emt->last_line);
}

void lit_emitter_emitasbx(LitAstEmitter* emt, uint16_t line, uint8_t opcode, uint8_t a, int32_t sbx)
{
    emt->last_line = fmax(line, emt->last_line);
    lit_chunk_push(emt->chunk, LIT_REG_FORMASBXINST(opcode, a, sbx), emt->last_line);
}

/*
 * be very careful with the use of this function:
 * always reserve a register just before using it, do not wait around!
 */
uint64_t lit_emitter_reserveregister(LitAstEmitter* emt)
{
    LitAstCompiler* ccx = emt->compiler;
    if(ccx->registersused == LIT_CONFIG_REGISTERSMAX)
    {
        lit_emitter_raiseerror(emt, emt->last_line, "too many registers required");
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


void lit_emitter_freeregister(LitAstEmitter* emt, uint64_t reg)
{
    if(LIT_BIT_ISSET(reg, LIT_BITFLAG_REGISTER))
    {
        return;
    }
    LitAstCompiler* ccx = emt->compiler;
    if(ccx->registersused == 0)
    {
        return lit_emitter_raiseerror(emt, emt->last_line, "invalid register was freed");
    }
    ccx->registersused--;
}

LitAstLocal lit_emitter_makelocal(const char* name, size_t length, int depth, bool captured, bool constant, uint8_t reg)
{
    LitAstLocal rt;
    rt.name = name;
    rt.length = length;
    rt.depth = depth;
    rt.captured = captured;
    rt.constant = constant;
    rt.reg = reg;
    return rt;
}

void lit_emitter_compilerinit(LitAstEmitter* emt, LitAstCompiler* ccx, LitFuncType type)
{
    lit_dynlistloc_init(&ccx->locals);
    ccx->type = type;
    ccx->scope_depth = -1;
    ccx->enclosing = (struct LitAstCompiler*)emt->compiler;
    ccx->skip_return = false;
    ccx->function = lit_object_makefunction(emt->pstate, emt->module);
    ccx->loop_depth = 0;
    ccx->registersused = 0;
    emt->compiler = ccx;
    const char* name = emt->pstate->lexer->sourcefilename;
    if(emt->compiler == NULL)
    {
        ccx->function->name = lit_string_copylen(emt->pstate, name, strlen(name));
    }
    emt->chunk = &ccx->function->chunk;
    if(type == LIT_FUNCTYPE_METHOD || type == LIT_FUNCTYPE_STATIC_METHOD || type == LIT_FUNCTYPE_CONSTRUCTOR)
    {
        lit_dynlistloc_push(&ccx->locals, lit_emitter_makelocal("this", 4, -1, false, false, lit_emitter_reserveregister(emt)));
    }
    else
    {
        lit_dynlistloc_push(&ccx->locals, lit_emitter_makelocal("", 0, -1, false, false, lit_emitter_reserveregister(emt)));
    }
}

LitFuncScript* lit_emitter_compilerend(LitAstEmitter* emt, LitString* name)
{
    lit_emitter_freeregister(emt, 0);
    if(emt->compiler->registersused > 0)
    {
        lit_emitter_raiseerror(emt, emt->last_line, "not all registers were freed (%i left)", emt->compiler->registersused);
    }
    if(!emt->compiler->skip_return)
    {
        uint8_t reg = lit_emitter_reserveregister(emt);
        LitFuncType type = emt->compiler->type;
        if(type == LIT_FUNCTYPE_CONSTRUCTOR)
        {
            /* load <this> */
            lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, reg, 0, 0);
        }
        else
        {
            lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_LOADNULL, reg, 0, 0);
        }
        lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_RETURN, reg, 1, 0);
        
        lit_emitter_freeregister(emt, reg);
        emt->compiler->skip_return = true;
    }
    LitFuncScript* function = emt->compiler->function;
    lit_dynlistloc_destroy(&emt->compiler->locals);
    emt->compiler = (LitAstCompiler*)emt->compiler->enclosing;
    emt->chunk = emt->compiler == NULL ? NULL : &emt->compiler->function->chunk;
    if(name != NULL)
    {
        function->name = name;
    }
#ifdef LIT_TRACE_CHUNK
    if(!emt->pstate->had_error)
    {
        lit_debug_disaschunk(&function->chunk, function->name->strbuf.data, NULL);
    }
#endif
    return function;
}

void lit_emitter_scopebegin(LitAstEmitter* emt)
{
    emt->compiler->scope_depth++;
}

void lit_emitter_scopeend(LitAstEmitter* emt)
{
    if(emt->compiler->scope_depth == -1)
    {
        lit_emitter_raiseerror(emt, emt->last_line, "invalid scope ending");
    }
    emt->compiler->scope_depth--;
    LitAstCompiler* ccx = emt->compiler;
    LitDynListLoc* locals = &ccx->locals;
    while(locals->listcount > 0 && locals->listitems[locals->listcount - 1].depth > ccx->scope_depth)
    {
        LitAstLocal* local = &locals->listitems[locals->listcount - 1];
        if(local->captured)
        {
            lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_UPVALUECLOSE, local->reg, 0, 0);
        }
        lit_emitter_freeregister(emt, local->reg);
        locals->listcount--;
    }
}

uint16_t lit_emitter_addconst(LitAstEmitter* emt, size_t line, LitValue val)
{
    size_t constant = lit_chunk_addconstant(emt->pstate, emt->chunk, val);
    if(constant >= UINT16_MAX)
    {
        lit_emitter_raiseerror(emt, line, "too many constants for one chunk");
    }
    return constant;
}

int lit_emitter_addprivate(LitAstEmitter* emt, const char* name, size_t length, size_t line, bool constant)
{
    int index;
    LitValue vidx;
    LitAstPrivate priv;
    LitState* state;
    LitString* key;
    LitTable* privnames;
    LitDynListPriv* privates;
    privates = &emt->privlist;
    if(privates->listcount == UINT16_MAX)
    {
        lit_emitter_raiseerror(emt, line, "too many private locals for one module");
    }
    privnames = &emt->module->privatenames->innertable;
    key = lit_table_findstring(privnames, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        lit_emitter_raiseerror(emt, line, "variable '%.*s' was already declared in this scope", length, name);
        if(lit_table_getentry(privnames, key, &vidx))
        {
            return lit_value_asnumber(vidx);
        }
    }
    state = emt->pstate;
    index = (int)privates->listcount;
    priv.initialized = false;
    priv.constant = constant;
    lit_dynlistpriv_push(privates, priv);
    lit_table_set(privnames, lit_string_copylen(state, name, length), lit_value_makenumber(index));
    emt->module->privatecount++;
    return index;
}

int lit_emitter_resolveprivate(LitAstEmitter* emt, const char* name, size_t length, size_t line)
{
    int numberindex;
    LitValue index;
    LitString* key;
    LitTable* privnames;
    privnames = &emt->module->privatenames->innertable;
    key = lit_table_findstring(privnames, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        if(lit_table_getentry(privnames, key, &index))
        {
            numberindex = lit_value_asnumber(index);
            if(!emt->privlist.listitems[numberindex].initialized)
            {
                lit_emitter_raiseerror(emt, line, "variable '%.*s' can't use itself in its initializer", length, name);
            }
            return numberindex;
        }
    }
    return -1;
}

int lit_emitter_addlocal(LitAstEmitter* emt, const char* name, size_t length, size_t line, bool constant, uint8_t reg)
{
    int i;
    LitAstCompiler* ccx = emt->compiler;
    LitDynListLoc* locals = &ccx->locals;
    if(locals->listcount == UINT16_MAX)
    {
        lit_emitter_raiseerror(emt, line, "too many local variables for one function");
    }
    for(i = (int)locals->listcount - 1; i >= 0; i--)
    {
        LitAstLocal* local = &locals->listitems[i];
        if(local->depth != UINT16_MAX && local->depth < ccx->scope_depth)
        {
            break;
        }
        if(length == local->length && memcmp(local->name, name, length) == 0)
        {
            lit_emitter_raiseerror(emt, line, "variable '%.*s' was already declared in this scope", length, name);
        }
    }
    lit_dynlistloc_push(locals, lit_emitter_makelocal(name, length, UINT16_MAX, false, constant, reg));
    return (int)locals->listcount - 1;
}

int lit_emitter_resolvelocal(LitAstEmitter* emt, LitAstCompiler* ccx, const char* name, size_t length, size_t line)
{
    int i;
    LitDynListLoc* locals = &ccx->locals;
    for(i = (int)locals->listcount - 1; i >= 0; i--)
    {
        LitAstLocal* local = &locals->listitems[i];
        if(local->length == length && memcmp(local->name, name, length) == 0)
        {
            if(local->depth == UINT16_MAX)
            {
                lit_emitter_raiseerror(emt, line, "variable '%.*s' can't use itself in its initializer", length, name);
            }
            return i;
        }
    }
    return -1;
}

int lit_emitter_addupvalue(LitAstEmitter* emt, LitAstCompiler* ccx, size_t index, size_t line, bool islocal)
{
    size_t i;
    size_t upvaluecount = ccx->function->upvaluecount;
    for(i = 0; i < upvaluecount; i++)
    {
        LitAstUpvalue* upvalue = &ccx->upvalues[i];
        if(upvalue->index == index && upvalue->isLocal == islocal)
        {
            return i;
        }
    }
    if(upvaluecount == LIT_CONFIG_UINT16COUNT)
    {
        lit_emitter_raiseerror(emt, line, "too many upvalues for one function");
        return 0;
    }
    ccx->upvalues[upvaluecount].isLocal = islocal;
    ccx->upvalues[upvaluecount].index = index;
    return ccx->function->upvaluecount++;
}

int lit_emitter_resolveupvalue(LitAstEmitter* emt, LitAstCompiler* ccx, const char* name, size_t length, size_t line)
{
    if(ccx->enclosing == NULL)
    {
        return -1;
    }
    int local = lit_emitter_resolvelocal(emt, (LitAstCompiler*)ccx->enclosing, name, length, line);
    if(local != -1)
    {
        ((LitAstCompiler*)ccx->enclosing)->locals.listitems[local].captured = true;
        return lit_emitter_addupvalue(emt, ccx, local, line, true);
    }
    int upvalue = lit_emitter_resolveupvalue(emt, (LitAstCompiler*)ccx->enclosing, name, length, line);
    if(upvalue != -1)
    {
        return lit_emitter_addupvalue(emt, ccx, upvalue, line, false);
    }
    return -1;
}

void lit_emitter_marklocalinit(LitAstEmitter* emt, size_t index)
{
    emt->compiler->locals.listitems[index].depth = emt->compiler->scope_depth;
}

void lit_emitter_markprivateinit(LitAstEmitter* emt, size_t index)
{
    emt->privlist.listitems[index].initialized = true;
}

void lit_emitter_resolvestatement(LitAstEmitter* emt, LitAstExpression* topexpr)
{
    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case LIT_ASTEXPRTYP_VARDECL:
        {
            LitAstVarDeclExpr* expr = (LitAstVarDeclExpr*)topexpr;
            lit_emitter_markprivateinit(emt, lit_emitter_addprivate(emt, expr->name, expr->length, topexpr->line, expr->isconstant));
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            LitAstFunctionExpr* expr = (LitAstFunctionExpr*)topexpr;
            if(!expr->exported)
            {
                lit_emitter_markprivateinit(emt, lit_emitter_addprivate(emt, expr->name, expr->length, topexpr->line, false));
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

LitOpCode lit_emitter_translateunaryop(LitAstTokType token)
{
    switch(token)
    {
        case LIT_ASTTOKTYP_MINUS:
            return LIT_OPCODE_NEGATE;
        case LIT_ASTTOKTYP_BANG:
            return LIT_OPCODE_NOT;
        case LIT_ASTTOKTYP_TILDE:
            return LIT_OPCODE_BINNOT;
        default:
            UNREACHABLE
    }
    return LIT_OPCODE_RETURN;
}

LitOpCode lit_emitter_translatebinaryop(LitAstTokType token)
{
    switch(token)
    {
        case LIT_ASTTOKTYP_BANGEQUAL:
        case LIT_ASTTOKTYP_EQUALEQUAL:
            return LIT_OPCODE_EQUAL;
        case LIT_ASTTOKTYP_LESS:
            return LIT_OPCODE_LESSTHAN;
        case LIT_ASTTOKTYP_LESSEQUAL:
            return LIT_OPCODE_LESSEQUAL;
        case LIT_ASTTOKTYP_GREATER:
            return LIT_OPCODE_GREATERTHAN;
        case LIT_ASTTOKTYP_GREATEREQUAL:
            return LIT_OPCODE_GREATEREQUAL;
        case LIT_ASTTOKTYP_PLUS:
            return LIT_OPCODE_MATHADD;
        case LIT_ASTTOKTYP_MINUS:
            return LIT_OPCODE_MATHSUBTRACT;
        case LIT_ASTTOKTYP_STAR:
            return LIT_OPCODE_MATHMULTIPLY;
        case LIT_ASTTOKTYP_STARSTAR:
            return LIT_OPCODE_MATHPOWER;
        case LIT_ASTTOKTYP_SLASH:
            return LIT_OPCODE_MATHDIVIDE;
        case LIT_ASTTOKTYP_SHARP:
            return LIT_OPCODE_MATHFLOORDIVIDE;
        case LIT_ASTTOKTYP_PERCENT:
            return LIT_OPCODE_MATHMOD;
        case LIT_ASTTOKTYP_LESSLESS:
            return LIT_OPCODE_MATHLEFTSHIFT;
        case LIT_ASTTOKTYP_GREATERGREATER:
            return LIT_OPCODE_MATHRIGHTSHIFT;
        case LIT_ASTTOKTYP_CARET:
            return LIT_OPCODE_BINXOR;
        case LIT_ASTTOKTYP_AMPERSAND:
            return LIT_OPCODE_BINAND;
        case LIT_ASTTOKTYP_BAR:
            return LIT_OPCODE_BINOR;
        case LIT_ASTTOKTYP_KWIS:
            return LIT_OPCODE_IS;
        default:
            UNREACHABLE
    }
    return LIT_OPCODE_RETURN;
}

uint16_t lit_emitter_parsearg(LitAstEmitter* emt, LitAstExpression* topexpr, uint8_t reg)
{
    if(topexpr->type == LIT_ASTEXPRTYP_LITERAL)
    {
        LitValue value = ((LitAstLiteralValExpr*)topexpr)->value;
        if(lit_value_isnumber(value) || lit_value_isstring(value))
        {
            uint64_t arg = lit_emitter_addconst(emt, topexpr->line, value);
            /* Mark that this is a constant */
            LIT_BIT_SETBIT(arg, LIT_BITFLAG_CONSTANT);
            return arg;
        }
    }
    else if(topexpr->type == LIT_ASTEXPRTYP_VARGET)
    {
        LitAstVarGetExpr* expr = ((LitAstVarGetExpr*)topexpr);
        int index = lit_emitter_resolvelocal(emt, emt->compiler, expr->name, expr->length, topexpr->line);
        if(index != -1)
        {
            return emt->compiler->locals.listitems[index].reg;
        }
    }
    lit_emitter_emitexpr(emt, topexpr, reg);
    return reg;
}

void lit_emitter_emitbinaryexpr(LitAstEmitter* emt, LitAstBinaryExpr* expr, uint8_t reg, bool swap)
{
    LitAstTokType op = expr->op;
    if(op == LIT_ASTTOKTYP_AMPERSANDAMPERSAND || op == LIT_ASTTOKTYP_BARBAR || op == LIT_ASTTOKTYP_QUESTIONQUESTION)
    {
        lit_emitter_emitexpr(emt, expr->left, reg);
        size_t jump = lit_emitter_emittmp(emt);
        lit_emitter_emitexpr(emt, expr->right, reg);
        lit_emitter_patchinstr(emt, jump, LIT_REG_FORMABXINST(op == LIT_ASTTOKTYP_BARBAR ? LIT_OPCODE_JUMPIFTRUE : (op == LIT_ASTTOKTYP_QUESTIONQUESTION ? LIT_OPCODE_JUMPIFNONNULL : LIT_OPCODE_JUMPIFFALSE), reg, emt->chunk->compiledcodecount - jump - 1));
    }
    else
    {
        uint16_t b = lit_emitter_parsearg(emt, expr->left, reg);
        LitOpCode opcode = lit_emitter_translatebinaryop(op);
        if(opcode == LIT_OPCODE_IS)
        {
            if(expr->right->type != LIT_ASTEXPRTYP_VARGET)
            {
                return lit_emitter_raiseerror(emt, ((LitAstExpression*)expr)->line, "'is' operator is not used with a var expression");
            }
            LitAstVarGetExpr* e = (LitAstVarGetExpr*)expr->right;
            int constant = lit_emitter_addconst(emt, ((LitAstExpression*)expr)->line, lit_value_fromobject(lit_string_copylen(emt->pstate, e->name, e->length)));
            lit_emitter_emitabc(emt, ((LitAstExpression*)expr)->line, opcode, reg, b, constant);
        }
        else
        {
            uint16_t rc = lit_emitter_reserveregister(emt);
            uint16_t c = lit_emitter_parsearg(emt, expr->right, rc);
            lit_emitter_emitabc(emt, ((LitAstExpression*)expr)->line, opcode, reg, swap ? c : b, swap ? b : c);
            lit_emitter_freeregister(emt, rc);
        }
    }
}

bool lit_emitter_emitparams(LitAstEmitter* emt, LitDynListParam* parameters, size_t line)
{
    size_t i;
    for(i = 0; i < parameters->listcount; i++)
    {
        LitAstFuncParamExpr* parameter = &parameters->listitems[i];
        uint8_t reg = lit_emitter_reserveregister(emt);
        parameter->reg = reg;
        int index = lit_emitter_addlocal(emt, parameter->name, parameter->length, line, false, reg);
        lit_emitter_marklocalinit(emt, index);
        /* variadic arg ...  */
        if(parameter->length == 3 && memcmp(parameter->name, "...", 3) == 0)
        {
            return true;
        }
        if(parameter->defaultval != NULL)
        {
            size_t jump = lit_emitter_emittmp(emt);
            lit_emitter_emitexpr(emt, parameter->defaultval, reg);
            lit_emitter_patchinstr(emt, jump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFNONNULL, reg, (int64_t)emt->chunk->compiledcodecount - jump - 1));
        }
    }
    return false;
}

void lit_emitter_emitexprfull(LitAstEmitter* emt, LitAstExpression* topexpr, uint64_t reg, bool ignored);

void lit_emitter_emitexprignoringregister(LitAstEmitter* emt, LitAstExpression* topexpr)
{
    uint8_t reg = lit_emitter_reserveregister(emt);
    lit_emitter_emitexprfull(emt, topexpr, reg, true);
    lit_emitter_freeregister(emt, reg);
}

void lit_emitter_emitexpr(LitAstEmitter* emt, LitAstExpression* topexpr, uint64_t reg)
{
    lit_emitter_emitexprfull(emt, topexpr, reg, false);
}

void lit_emitter_emitexprfull(LitAstEmitter* emt, LitAstExpression* topexpr, uint64_t reg, bool ignored)
{
    if(topexpr == NULL)
    {
        return;
    }
    switch(topexpr->type)
    {
        case LIT_ASTEXPRTYP_LITERAL:
        {
            LitValue value = ((LitAstLiteralValExpr*)topexpr)->value;
            if(lit_value_isnull(value))
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_LOADNULL, reg, 0, 0);
            }
            else if(lit_value_isbool(value))
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_LOADBOOL, reg, (uint8_t)lit_value_asbool(value), 0);
            }
            else
            {
                uint64_t constant = lit_emitter_addconst(emt, topexpr->line, value);
                LIT_BIT_SETBIT(constant, LIT_BITFLAG_CONSTANT);
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, constant, 0);
            }
            break;
        }
        case LIT_ASTEXPRTYP_UNARY:
        {
            LitAstUnaryExpr* expr = (LitAstUnaryExpr*)topexpr;
            uint16_t b = lit_emitter_parsearg(emt, expr->right, reg);
            lit_emitter_emitabc(emt, topexpr->line, lit_emitter_translateunaryop(expr->op), reg, b, 0);
            break;
        }
        case LIT_ASTEXPRTYP_BINARY:
        {
            LitAstBinaryExpr* expr = (LitAstBinaryExpr*)topexpr;
            switch(expr->op)
            {
                case LIT_ASTTOKTYP_GREATER:
                case LIT_ASTTOKTYP_GREATEREQUAL:
                case LIT_ASTTOKTYP_LESS:
                case LIT_ASTTOKTYP_LESSEQUAL:
                case LIT_ASTTOKTYP_EQUALEQUAL:
                {
                    lit_emitter_emitbinaryexpr(emt, expr, reg, false);
                    break;
                }
                case LIT_ASTTOKTYP_BANGEQUAL:
                {
                    lit_emitter_emitbinaryexpr(emt, expr, reg, false);
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_NOT, reg, reg, false);
                    break;
                }
                default:
                {
                    return lit_emitter_emitbinaryexpr(emt, expr, reg, false);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_VARGET:
        {
            LitAstVarGetExpr* expr = (LitAstVarGetExpr*)topexpr;
            bool ref = emt->emit_reference > 0;
            if(ref)
            {
                emt->emit_reference--;
            }
            int index = lit_emitter_resolvelocal(emt, emt->compiler, expr->name, expr->length, topexpr->line);
            if(index == -1)
            {
                index = lit_emitter_resolveupvalue(emt, emt->compiler, expr->name, expr->length, topexpr->line);
                if(index == -1)
                {
                    index = lit_emitter_resolveprivate(emt, expr->name, expr->length, topexpr->line);
                    if(index == -1)
                    {
                        uint64_t constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(lit_string_copylen(emt->pstate, expr->name, expr->length)));
                        if(ref)
                        {
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_REFGLOBAL, reg, constant);
                        }
                        else
                        {
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_GLOBALGET, reg, constant);
                        }
                    }
                    else
                    {
                        if(ref)
                        {
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_REFPRIVATE, reg, index);
                        }
                        else
                        {
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_PRIVATEGET, reg, index);
                        }
                    }
                }
                else
                {
                    if(ref)
                    {
                        lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_REFUPVALUE, reg, index);
                    }
                    else
                    {
                        lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_UPVALUEGET, reg, index);
                    }
                }
            }
            else
            {
                uint64_t r = emt->compiler->locals.listitems[index].reg;
                if(ref)
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_REFLOCAL, reg, r, 0);
                }
                else if(reg != r)
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, r, 0);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_ASSIGN:
        {
            LitAstAssignExpr* expr = (LitAstAssignExpr*)topexpr;
            if(expr->to->type == LIT_ASTEXPRTYP_VARGET)
            {
                LitAstVarGetExpr* e = (LitAstVarGetExpr*)expr->to;
                uint64_t index = lit_emitter_resolvelocal(emt, emt->compiler, e->name, e->length, expr->to->line);
                if(((int64_t)index) == -1)
                {
                    uint16_t r = lit_emitter_parsearg(emt, expr->value, reg);
                    index = lit_emitter_resolveupvalue(emt, emt->compiler, e->name, e->length, expr->to->line);
                    if(((int64_t)index) == -1)
                    {
                        index = lit_emitter_resolveprivate(emt, e->name, e->length, expr->to->line);
                        if(((int64_t)index) == -1)
                        {
                            uint16_t constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(lit_string_copylen(emt->pstate, e->name, e->length)));
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_GLOBALSET, constant, r);
                        }
                        else
                        {
                            if(emt->privlist.listitems[index].constant)
                            {
                                lit_emitter_raiseerror(emt, topexpr->line, "attempt to modify constant '%.*s'", e->length, e->name);
                            }
                            if(LIT_BIT_ISSET(r, LIT_BITFLAG_CONSTANT))
                            {
                                LIT_BIT_SETBIT(index, 16);
                            }
                            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_PRIVATESET, r, index);
                        }
                    }
                    else
                    {
                        lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_UPVALUESET, index, r);
                    }
                    if(!ignored && reg != r)
                    {
                        lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, r, 0);
                    }
                    break;
                }
                else
                {
                    LitAstLocal local = emt->compiler->locals.listitems[index];
                    if(local.constant)
                    {
                        lit_emitter_raiseerror(emt, topexpr->line, "attempt to modify constant '%.*s'", e->length, e->name);
                    }
                    if(expr->value->type == LIT_ASTEXPRTYP_LITERAL || expr->value->type == LIT_ASTEXPRTYP_VARGET)
                    {
                        lit_emitter_emitexpr(emt, expr->value, local.reg);
                    }
                    else
                    {
                        uint16_t r = lit_emitter_reserveregister(emt);
                        lit_emitter_emitexpr(emt, expr->value, r);
                        lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, local.reg, r, 0);
                        lit_emitter_freeregister(emt, r);
                    }
                    if(!ignored && reg != local.reg)
                    {
                        lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, local.reg, 0);
                    }
                    break;
                }
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_SUBSCRIPT)
            {
                LitAstSubscriptExpr* e = (LitAstSubscriptExpr*)expr->to;
                lit_emitter_emitexpr(emt, e->array, reg);
                uint8_t rega = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, e->index, rega);
                uint8_t regb = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, expr->value, regb);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_SUBSCRIPTSET, reg, rega, regb);
                lit_emitter_freeregister(emt, rega);
                lit_emitter_freeregister(emt, regb);
                break;
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_INDEXGET)
            {
                LitAstIndexGetExpr* e = (LitAstIndexGetExpr*)expr->to;
                uint8_t r = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, e->where, r);
                uint8_t rv = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, expr->value, rv);
                int constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(lit_string_copylen(emt->pstate, e->name, e->length)));
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_FIELDSET, r, constant, rv);
                if(!ignored && reg != rv)
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, rv, 0);
                }
                lit_emitter_freeregister(emt, r);
                lit_emitter_freeregister(emt, rv);
                break;
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_REFERENCE)
            {
                uint8_t r = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, ((LitAstRefExpr*)expr->to)->to, r);
                uint8_t rv = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, expr->value, rv);
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_REFSET, r, rv, 0);
                if(!ignored && reg != rv)
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, rv, 0);
                }
                lit_emitter_freeregister(emt, r);
                lit_emitter_freeregister(emt, rv);
                break;
            }
            lit_emitter_raiseerror(emt, topexpr->line, "invalid assigment target");
            break;
        }
        case LIT_ASTEXPRTYP_CALL:
        {
            LitAstCallExpr* expr = (LitAstCallExpr*)topexpr;
            size_t i;
            size_t argc = expr->callargs.listcount;
            uint16_t* argregs = (uint16_t*)lit_sysmem_malloc(argc * sizeof(uint16_t));
            bool method = expr->excallee->type == LIT_ASTEXPRTYP_INDEXGET;
            bool super = expr->excallee->type == LIT_ASTEXPRTYP_SUPER;

            uint64_t original_reg = reg;
            bool move_back = false;

            if(reg != emt->compiler->registersused - 1)
            {
                reg = lit_emitter_reserveregister(emt);
                move_back = true;
            }

            if(method)
            {
                ((LitAstIndexGetExpr*)expr->excallee)->ignore_emit = true;
            }
            else if(super)
            {
                ((LitAstSuperExpr*)expr->excallee)->ignore_emit = true;
            }
            lit_emitter_emitexpr(emt, expr->excallee, reg);
            uint64_t tmpreg = super ? lit_emitter_reserveregister(emt) : 0;
            for(i = 0; i < argc; i++)
            {
                uint64_t supadd;
                uint64_t argreg = lit_emitter_reserveregister(emt);
                LitAstExpression* e = expr->callargs.listitems[i];
                supadd = (super ? 2 : 1);
                if(argreg != ((reg + i) + supadd))
                {
                    /* something went terribly wrong */
                    argreg = (reg + i + supadd);
                    UNREACHABLE
                }
                argregs[i] = argreg;
                lit_emitter_emitexpr(emt, e, argreg);
            }
            if(method)
            {
                if(expr->excallee->type != LIT_ASTEXPRTYP_INDEXGET)
                {
                    /* TODO: replace with a proper error code? */
                    UNREACHABLE
                }
                LitAstIndexGetExpr* e = (LitAstIndexGetExpr*)expr->excallee;
                int constant = lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copylen(emt->pstate, e->name, e->length)));
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_INVOKE, reg, argc + 1, constant);
            }
            else if(super)
            {
                assert(tmpreg == reg + 1);
                LitAstSuperExpr* e = (LitAstSuperExpr*)expr->excallee;
                size_t index = lit_emitter_resolveupvalue(emt, emt->compiler, "super", 5, emt->last_line);
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_UPVALUEGET, tmpreg, index);
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, 0, 0);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_INVOKESUPER, reg, argc + 1, lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(e->methodname)));
                lit_emitter_freeregister(emt, tmpreg);
            }
            else
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_CALLCALLABLE, reg, argc + 1, 1);
            }
            for(i = 0; i < argc; i++)
            {
                lit_emitter_freeregister(emt, argregs[i]);
            }
            lit_sysmem_free(argregs);
            if(method)
            {
                LitAstExpression* get = expr->excallee;
                while(get != NULL)
                {
                    if(get->type == LIT_ASTEXPRTYP_INDEXGET)
                    {
                        LitAstIndexGetExpr* getter = (LitAstIndexGetExpr*)get;
                        if(getter->jump > 0)
                        {
                            lit_emitter_patchinstr(emt, getter->jump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFNULL, reg, (int64_t)emt->chunk->compiledcodecount - getter->jump - 1));
                        }
                        get = getter->where;
                    }
                    else if(get->type == LIT_ASTEXPRTYP_SUBSCRIPT)
                    {
                        get = ((LitAstSubscriptExpr*)get)->array;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(move_back)
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, original_reg, reg, 0);
                lit_emitter_freeregister(emt, reg);
                reg = original_reg;
            }
            if(expr->init == NULL)
            {
                break;
            }
            LitAstLiteralObjectExpr* init = (LitAstLiteralObjectExpr*)expr->init;
            uint8_t r = lit_emitter_reserveregister(emt);
            for(i = 0; i < init->objexvalues.listcount; i++)
            {
                LitAstExpression* e = init->objexvalues.listitems[i];
                emt->last_line = e->line;
                lit_emitter_emitexpr(emt, e, r);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_OBJECTPUSH, reg, lit_emitter_addconst(emt, emt->last_line, init->objexkeys.listitems[i]), r);
            }
            lit_emitter_freeregister(emt, r);
            break;
        }
        case LIT_ASTEXPRTYP_INDEXGET:
        {
            LitAstIndexGetExpr* expr = (LitAstIndexGetExpr*)topexpr;
            bool ref = emt->emit_reference > 0;
            if(ref)
            {
                emt->emit_reference--;
            }
            bool jump = expr->jump == 0;
            bool emit = !expr->ignore_emit;
            lit_emitter_emitexpr(emt, expr->where, reg);
            if(jump)
            {
                expr->jump = lit_emitter_emittmp(emt);
                if(!expr->ignore_emit)
                {
                    int constant = lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copylen(emt->pstate, expr->name, expr->length)));
                    if(ref)
                    {
                        lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_REFFIELD, reg, reg, constant);
                    }
                    else
                    {
                        lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_FIELDGET, reg, reg, constant);
                    }
                }
                lit_emitter_patchinstr(emt, expr->jump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFNULL, reg, (int64_t)emt->chunk->compiledcodecount - expr->jump - 1));
            }
            else if(emit)
            {
                int constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(lit_string_copylen(emt->pstate, expr->name, expr->length)));
                if(ref)
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_REFFIELD, reg, reg, constant);
                }
                else
                {
                    lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_FIELDGET, reg, reg, constant);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_INDEXSET:
        {
            LitAstIndexSetExpr* expr = (LitAstIndexSetExpr*)topexpr;
            uint8_t wherereg = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->where, wherereg);
            uint8_t valuereg = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->value, valuereg);
            int constant = lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copylen(emt->pstate, expr->name, expr->length)));
            lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_FIELDSET, wherereg, constant, valuereg);
            if(!ignored && reg != valuereg)
            {
                /* pains me to do this, but we gotta ensure that the value is after the where */
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, reg, valuereg, 0);
            }
            lit_emitter_freeregister(emt, wherereg);
            lit_emitter_freeregister(emt, valuereg);
            break;
        }
        case LIT_ASTEXPRTYP_SUBSCRIPT:
        {
            LitAstSubscriptExpr* expr = (LitAstSubscriptExpr*)topexpr;
            lit_emitter_emitexpr(emt, expr->array, reg);
            uint8_t r = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->index, r);
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_SUBSCRIPTGET, reg, r, 0);
            lit_emitter_freeregister(emt, r);
            break;
        }
        case LIT_ASTEXPRTYP_ARRAY:
        {
            size_t i;
            LitAstLiteralArrayExpr* expr = (LitAstLiteralArrayExpr*)topexpr;
            uint8_t r = lit_emitter_reserveregister(emt);
            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_MAKEARRAY, reg, expr->exvalues.listcount);
            for(i = 0; i < expr->exvalues.listcount; i++)
            {
                lit_emitter_emitexpr(emt, expr->exvalues.listitems[i], r);
                lit_emitter_emitabx(emt, emt->last_line, LIT_OPCODE_ARRAYPUSH, reg, r);
            }
            lit_emitter_freeregister(emt, r);
            break;
        }
        case LIT_ASTEXPRTYP_OBJECT:
        {
            size_t i;
            LitAstLiteralObjectExpr* expr = (LitAstLiteralObjectExpr*)topexpr;
            uint8_t r = lit_emitter_reserveregister(emt);
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MAKEOBJECT, reg, 0, 0);
            for(i = 0; i < expr->objexvalues.listcount; i++)
            {
                lit_emitter_emitexpr(emt, expr->objexvalues.listitems[i], r);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_OBJECTPUSH, reg, lit_emitter_addconst(emt, emt->last_line, expr->objexkeys.listitems[i]), r);
            }
            lit_emitter_freeregister(emt, r);
            break;
        }
        case LIT_ASTEXPRTYP_FUNCANON:
        {
            size_t i;
            LitAstFunctionExpr* expr = (LitAstFunctionExpr*)topexpr;
            LitString* name = lit_value_asstring(lit_string_valformat(emt->pstate, "lambda @:@", lit_value_fromobject(emt->module->name), lit_string_numbertostring(emt->pstate, topexpr->line)));
            LitAstCompiler stackcc;
            lit_emitter_compilerinit(emt, &stackcc, LIT_FUNCTYPE_REGULAR);
            lit_emitter_scopebegin(emt);
            bool vararg = lit_emitter_emitparams(emt, &expr->parameters, topexpr->line);
            bool ended = false;
            if(expr->body != NULL)
            {
                bool singleexpr = expr->body->type == LIT_ASTEXPRTYP_EXPRESSION;
                if(singleexpr)
                {
                    uint8_t r = lit_emitter_reserveregister(emt);
                    stackcc.skip_return = true;
                    lit_emitter_emitexpr(emt, ((LitAstExprStmtExpr*)expr->body)->exvalue, r);
                    lit_emitter_emitabc(emt, expr->body->line, LIT_OPCODE_RETURN, r, 0, 0);
                    lit_emitter_freeregister(emt, r);
                }
                else
                {
                    ended = lit_emitter_emitstmt(emt, expr->body);
                }
            }
            if(!ended)
            {
                lit_emitter_scopeend(emt);
            }
            LitFuncScript* function = lit_emitter_compilerend(emt, name);
            function->argcount = expr->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint64_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emt);
                LitClsPrototype* clsproto = lit_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitAstUpvalue* upvalue = &stackcc.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_MAKECLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, functionreg, 0);
            if(closure)
            {
                lit_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_RANGE:
        {
            LitAstRangeExpr* expr = (LitAstRangeExpr*)topexpr;
            lit_emitter_emitexpr(emt, expr->to, reg);
            uint8_t regb = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->from, regb);
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MAKERANGE, reg, regb, reg);
            lit_emitter_freeregister(emt, regb);
            break;
        }
        case LIT_ASTEXPRTYP_INTERPOLATION:
        {
            size_t i;
            LitAstStrTemplateExpr* expr = (LitAstStrTemplateExpr*)topexpr;
            lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_MAKEARRAY, reg, expr->expressions.listcount);
            uint8_t r = lit_emitter_reserveregister(emt);
            for(i = 0; i < expr->expressions.listcount; i++)
            {
                lit_emitter_emitexpr(emt, expr->expressions.listitems[i], r);
                lit_emitter_emitabx(emt, emt->last_line, LIT_OPCODE_ARRAYPUSH, reg, r);
            }
            lit_emitter_freeregister(emt, r);
            lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_INVOKE, reg, 2, lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copy(emt->pstate, "join"))));
            break;
        }
        case LIT_ASTEXPRTYP_THIS:
        {
            LitFuncType type = emt->compiler->type;
            if(type == LIT_FUNCTYPE_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "'this' can't be used %s", "in static methods");
            }
            if(type == LIT_FUNCTYPE_CONSTRUCTOR || type == LIT_FUNCTYPE_METHOD)
            {
                /* the instance is always in register 0 */
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, 0, 0);
            }
            else
            {
                if(emt->compiler->enclosing == NULL)
                {
                    lit_emitter_raiseerror(emt, topexpr->line, "'this' can't be used %s", "in functions outside of any class");
                }
                else
                {
                    size_t index = lit_emitter_resolveupvalue(emt, emt->compiler, "this", 4, topexpr->line);
                    lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_UPVALUEGET, reg, index);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_TERNARY:
        {
            LitAstTernaryExpr* expr = (LitAstTernaryExpr*)topexpr;
            uint8_t condreg = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->condition, condreg);
            size_t condbranchskip = lit_emitter_emittmp(emt);
            lit_emitter_freeregister(emt, condreg);
            int64_t start = emt->chunk->compiledcodecount;
            lit_emitter_emitexpr(emt, expr->branchif, reg);
            size_t elseskip = lit_emitter_emittmp(emt);
            lit_emitter_patchinstr(emt, condbranchskip, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - start));
            int64_t elsestart = emt->chunk->compiledcodecount;
            lit_emitter_emitexpr(emt, expr->branchelse, reg);
            lit_emitter_patchinstr(emt, elseskip, LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - elsestart));
            break;
        }
        case LIT_ASTEXPRTYP_SUPER:
        {
            if(emt->compiler->type == LIT_FUNCTYPE_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "'super' can't be used %s", "in static methods");
            }
            else if(!emt->class_has_super)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "'super' can't be used in class '%s', because it doesn't have a super class", emt->class_name->strbuf.data);
            }
            LitAstSuperExpr* expr = (LitAstSuperExpr*)topexpr;
            if(!expr->ignore_emit)
            {
                size_t index = lit_emitter_resolveupvalue(emt, emt->compiler, "super", 5, emt->last_line);
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, 0, 0);
                uint8_t tmpreg = lit_emitter_reserveregister(emt);
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_UPVALUEGET, tmpreg, index);
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_CLASSGETSUPERMETHOD, reg, tmpreg, lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(expr->methodname)));
                lit_emitter_freeregister(emt, tmpreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_REFERENCE:
        {
            LitAstExpression* to = ((LitAstRefExpr*)topexpr)->to;
            if(to->type != LIT_ASTEXPRTYP_VARGET && to->type != LIT_ASTEXPRTYP_INDEXGET && to->type != LIT_ASTEXPRTYP_THIS && to->type != LIT_ASTEXPRTYP_SUPER)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "invalid reference target (cannot reference literals)");
                break;
            }
            int old = emt->emit_reference;
            emt->emit_reference++;
            lit_emitter_emitexpr(emt, to, reg);
            emt->emit_reference = old;
            break;
        }
        default:
        {
            lit_emitter_raiseerror(emt, topexpr->line, "unknown expression with id '%i'", (int)topexpr->type);
            break;
        }
    }
}

void lit_emitter_patchloopjumps(LitAstEmitter* emt, LitDynListUInt* breaks)
{
    size_t i;
    for(i = 0; i < breaks->listcount; i++)
    {
        lit_emitter_patchinstr(emt, breaks->listitems[i], LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - breaks->listitems[i] - 1));
    }
    lit_dynlistuint_destroy(breaks);
}

void lit_emitter_emitstmtscoped(LitAstEmitter* emt, LitAstExpression* expr)
{
    lit_emitter_scopebegin(emt);
    if(!lit_emitter_emitstmt(emt, expr))
    {
        lit_emitter_scopeend(emt);
    }
}

bool lit_emitter_emitstmt(LitAstEmitter* emt, LitAstExpression* topexpr)
{
    if(topexpr == NULL)
    {
        return false;
    }
    switch(topexpr->type)
    {
        case LIT_ASTEXPRTYP_EXPRESSION:
        {
            lit_emitter_emitexprignoringregister(emt, ((LitAstExprStmtExpr*)topexpr)->exvalue);
            break;
        }
        case LIT_ASTEXPRTYP_BLOCK:
        {
            size_t i;
            LitDynListExpr* statements = &((LitAstBlockExpr*)topexpr)->statements;
            bool endedscope = false;
            for(i = 0; i < statements->listcount; i++)
            {
                if(lit_emitter_emitstmt(emt, statements->listitems[i]))
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
        case LIT_ASTEXPRTYP_VARDECL:
        {
            LitAstVarDeclExpr* expr = (LitAstVarDeclExpr*)topexpr;
            bool isprivate = emt->compiler->enclosing == NULL && emt->compiler->scope_depth == 0;
            int index = 0;
            uint16_t reg = lit_emitter_reserveregister(emt);
            if(!isprivate)
            {
                index = lit_emitter_addlocal(emt, expr->name, expr->length, topexpr->line, expr->isconstant, reg);
            }
            else
            {
                index = lit_emitter_resolveprivate(emt, expr->name, expr->length, topexpr->line);
            }
            if(expr->init == NULL)
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_LOADNULL, reg, 0, 0);
            }
            else
            {
                lit_emitter_emitexpr(emt, expr->init, reg);
            }
            if(isprivate)
            {
                lit_emitter_markprivateinit(emt, index);
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_PRIVATESET, reg, index);
                lit_emitter_freeregister(emt, reg);
            }
            else
            {
                lit_emitter_marklocalinit(emt, index);
            }
            break;
        }
        case LIT_ASTEXPRTYP_IF:
        {
            size_t i;
            LitAstIfExpr* expr = (LitAstIfExpr*)topexpr;
            uint16_t condreg = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->condition, condreg);
            size_t condbranchskip = lit_emitter_emittmp(emt);
            size_t elseskip = 0;
            lit_emitter_freeregister(emt, condreg);
            int64_t start = emt->chunk->compiledcodecount;
            lit_emitter_emitstmtscoped(emt, expr->branchif);
            if(expr->branchelse)
            {
                elseskip = lit_emitter_emittmp(emt);
            }
            lit_emitter_patchinstr(emt, condbranchskip, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - start));
            size_t endjumpcount = expr->branchelseiflist == NULL ? 0 : expr->branchelseiflist->listcount;
            uint64_t* endjumps = (uint64_t*)lit_sysmem_malloc(endjumpcount * sizeof(uint64_t));
            if(expr->branchelseiflist != NULL)
            {
                for(i = 0; i < expr->branchelseiflist->listcount; i++)
                {
                    LitAstExpression* e = expr->elseifcondlist->listitems[i];
                    if(e == NULL)
                    {
                        continue;
                    }
                    uint8_t elseifcondreg = lit_emitter_reserveregister(emt);
                    lit_emitter_emitexpr(emt, e, elseifcondreg);
                    uint64_t nextjump = lit_emitter_emittmp(emt);
                    lit_emitter_freeregister(emt, elseifcondreg);
                    lit_emitter_emitstmtscoped(emt, expr->branchelseiflist->listitems[i]);
                    endjumps[i] = lit_emitter_emittmp(emt);
                    lit_emitter_patchinstr(emt, nextjump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFFALSE, elseifcondreg, (int64_t)emt->chunk->compiledcodecount - nextjump - 1));
                }
            }
            if(expr->branchelse)
            {
                lit_emitter_emitstmtscoped(emt, expr->branchelse);
                lit_emitter_patchinstr(emt, elseskip, LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - elseskip - 1));
            }
            for(i = 0; i < endjumpcount; i++)
            {
                if(expr->elseifcondlist->listitems[i] == NULL)
                {
                    continue;
                }
                lit_emitter_patchinstr(emt, endjumps[i], LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - endjumps[i] - 1));
            }
            lit_sysmem_free(endjumps);
            break;
        }
        case LIT_ASTEXPRTYP_THROW:
        {
            LitAstThrowExpr* expr = (LitAstThrowExpr*)topexpr;
            uint8_t reg = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, expr->exvalue, reg);
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_THROW, reg, 0, 0);
            lit_emitter_freeregister(emt, reg);
            break;
        }
        case LIT_ASTEXPRTYP_TRY:
        {
            LitAstTryExpr* expr = (LitAstTryExpr*)topexpr;
            uint8_t error_reg = 0;

            uint64_t try_instr_idx = lit_emitter_emittmp(emt);

            lit_emitter_emitstmtscoped(emt, expr->try_block);

            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_POP_TRY, 0, 0, 0);

            uint64_t jump_to_finally = lit_emitter_emittmp(emt);

            uint64_t catch_start = emt->chunk->compiledcodecount;
            lit_emitter_patchinstr(emt, try_instr_idx, LIT_REG_FORMABXINST(LIT_OPCODE_PUSH_TRY, error_reg, (int64_t)catch_start - (int64_t)try_instr_idx - 1));

            if(expr->catch_block != NULL)
            {
                lit_emitter_scopebegin(emt);
                if(expr->catch_var != NULL)
                {
                    uint8_t reg = lit_emitter_reserveregister(emt);
                    uint64_t local_idx = lit_emitter_addlocal(emt, expr->catch_var, expr->catch_var_len, topexpr->line, false, reg);
                    error_reg = reg;
                    lit_emitter_patchinstr(emt, try_instr_idx, LIT_REG_FORMABXINST(LIT_OPCODE_PUSH_TRY, error_reg, (int64_t)catch_start - (int64_t)try_instr_idx - 1));
                    lit_emitter_marklocalinit(emt, local_idx);
                }
                lit_emitter_emitstmt(emt, expr->catch_block);
                lit_emitter_scopeend(emt);
                
                uint64_t finally_start = emt->chunk->compiledcodecount;
                lit_emitter_patchinstr(emt, jump_to_finally, LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)finally_start - (int64_t)jump_to_finally - 1));

                if(expr->finally_block != NULL)
                {
                    lit_emitter_emitstmtscoped(emt, expr->finally_block);
                }
            }
            else
            {
                // No catch block. If an error occurs, it jumps here.
                // We MUST run finally and then rethrow.
                if(expr->finally_block != NULL)
                {
                    lit_emitter_emitstmtscoped(emt, expr->finally_block);
                }
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_RETHROW, 0, 0, 0);
                
                uint64_t finally_start = emt->chunk->compiledcodecount;
                lit_emitter_patchinstr(emt, jump_to_finally, LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)finally_start - (int64_t)jump_to_finally - 1));
                
                // Normal path (no error)
                if(expr->finally_block != NULL)
                {
                    lit_emitter_emitstmtscoped(emt, expr->finally_block);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            size_t i;
            bool local;
            bool isexport;
            bool isprivate;
            uint64_t index;
            uint8_t reg;
            LitAstFunctionExpr* stmt;
            index = 0;
            stmt = (LitAstFunctionExpr*)topexpr;
            isexport = stmt->exported;
            isprivate = !isexport && emt->compiler->enclosing == NULL && emt->compiler->scope_depth == 0;
            local = !(isexport || isprivate);
            reg = 0;
            if(!isexport)
            {
                index = isprivate ? lit_emitter_resolveprivate(emt, stmt->name, stmt->length, topexpr->line) : lit_emitter_addlocal(emt, stmt->name, stmt->length, topexpr->line, false, reg = lit_emitter_reserveregister(emt));
            }
            LitString* name = lit_string_copylen(emt->pstate, stmt->name, stmt->length);
            if(local)
            {
                lit_emitter_marklocalinit(emt, index);
            }
            else if(isprivate)
            {
                lit_emitter_markprivateinit(emt, index);
            }
            LitAstCompiler stackcc;
            lit_emitter_compilerinit(emt, &stackcc, LIT_FUNCTYPE_REGULAR);
            lit_emitter_scopebegin(emt);
            bool vararg = lit_emitter_emitparams(emt, &stmt->parameters, topexpr->line);
            if(!lit_emitter_emitstmt(emt, stmt->body))
            {
                lit_emitter_scopeend(emt);
            }
            LitFuncScript* function = lit_emitter_compilerend(emt, name);
            function->argcount = stmt->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint64_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emt);
                LitClsPrototype* clsproto = lit_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitAstUpvalue* upvalue = &stackcc.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_MAKECLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            if(isexport)
            {
                uint16_t nameconst = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(function->name));
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_GLOBALSET, nameconst, functionreg);
            }
            else if(isprivate)
            {
                if(!closure)
                {
                    LIT_BIT_SETBIT(index, 16);
                }
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_PRIVATESET, functionreg, index);
            }
            else
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_MOVE, reg, functionreg, 0);
            }
            if(closure)
            {
                lit_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_RETURN:
        {
            LitAstReturnExpr* stmt = (LitAstReturnExpr*)topexpr;
            uint8_t reg = lit_emitter_reserveregister(emt);
            if(stmt->exvalue == NULL)
            {
                lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_LOADNULL, reg, 0, 0);
            }
            else
            {
                lit_emitter_emitexpr(emt, stmt->exvalue, reg);
            }
            lit_emitter_scopeend(emt);
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_RETURN, reg, 0, 0);
            lit_emitter_freeregister(emt, reg);
            return true;
        }
        case LIT_ASTEXPRTYP_WHILE:
        {
            LitAstWhileExpr* stmt = (LitAstWhileExpr*)topexpr;
            uint8_t reg = lit_emitter_reserveregister(emt);
            size_t beforecond = lit_emitter_emittmp(emt);
            emt->loop_start = beforecond;
            emt->compiler->loop_depth++;
            LitDynListUInt old_breaks = emt->breaks;
            LitDynListUInt old_continues = emt->continues;
            lit_dynlistuint_init(&emt->breaks);
            lit_dynlistuint_init(&emt->continues);
            lit_emitter_emitexpr(emt, stmt->condition, reg);
            size_t tmpinstr = lit_emitter_emittmp(emt);
            lit_emitter_emitstmtscoped(emt, stmt->body);
            lit_emitter_patchloopjumps(emt, &emt->continues);
            lit_emitter_emitasbx(emt, topexpr->line, LIT_OPCODE_JUMP, 0, (int)beforecond - emt->chunk->compiledcodecount - 1);
            lit_emitter_patchinstr(emt, tmpinstr, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFFALSE, reg, emt->chunk->compiledcodecount - tmpinstr - 1));
            lit_emitter_patchloopjumps(emt, &emt->breaks);
            emt->breaks = old_breaks;
            emt->continues = old_continues;
            emt->compiler->loop_depth--;
            lit_emitter_freeregister(emt, reg);
            break;
        }
        case LIT_ASTEXPRTYP_FOR:
        {
            size_t i;
            LitAstForExpr* stmt = (LitAstForExpr*)topexpr;
            emt->compiler->loop_depth++;
            LitDynListUInt old_breaks = emt->breaks;
            LitDynListUInt old_continues = emt->continues;
            lit_dynlistuint_init(&emt->breaks);
            lit_dynlistuint_init(&emt->continues);
            lit_emitter_scopebegin(emt);
            if(stmt->iscstyle)
            {
                if(stmt->var != NULL)
                {
                    lit_emitter_emitstmt(emt, stmt->var);
                }
                else if(stmt->init != NULL)
                {
                    lit_emitter_emitexprignoringregister(emt, stmt->init);
                }
                size_t start = emt->chunk->compiledcodecount;
                size_t exitjump = 0;
                uint8_t condreg = 0;
                if(stmt->condition != NULL)
                {
                    condreg = lit_emitter_reserveregister(emt);
                    lit_emitter_emitexpr(emt, stmt->condition, condreg);
                    exitjump = lit_emitter_emittmp(emt);
                }
                if(stmt->increment != NULL)
                {
                    size_t bodyjump = lit_emitter_emittmp(emt);
                    size_t incrstart = emt->chunk->compiledcodecount;
                    lit_emitter_emitexprignoringregister(emt, stmt->increment);
                    lit_emitter_emitasbx(emt, topexpr->line, LIT_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                    start = incrstart;
                    lit_emitter_patchinstr(emt, bodyjump, LIT_REG_FORMASBXINST(LIT_OPCODE_JUMP, 0, (int64_t)emt->chunk->compiledcodecount - bodyjump - 1));
                }
                emt->loop_start = start;
                bool endedscope = false;
                emt->loop_start = start;
                lit_emitter_scopebegin(emt);
                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_ASTEXPRTYP_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitAstBlockExpr*)stmt->body)->statements;
                        for(i = 0; i < statements->listcount; i++)
                        {
                            if(lit_emitter_emitstmt(emt, statements->listitems[i]))
                            {
                                endedscope = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        endedscope = lit_emitter_emitstmt(emt, stmt->body);
                    }
                }
                lit_emitter_patchloopjumps(emt, &emt->continues);
                if(!endedscope)
                {
                    lit_emitter_scopeend(emt);
                }
                lit_emitter_emitasbx(emt, topexpr->line, LIT_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                if(stmt->condition != NULL)
                {
                    lit_emitter_patchinstr(emt, exitjump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFFALSE, condreg, (int64_t)emt->chunk->compiledcodecount - exitjump - 1));
                    lit_emitter_freeregister(emt, condreg);
                }
            }
            else
            {
                size_t sequence = lit_emitter_reserveregister(emt);
                lit_emitter_marklocalinit(emt, lit_emitter_addlocal(emt, "seq ", 4, topexpr->line, false, sequence));
                uint8_t condreg = lit_emitter_reserveregister(emt);
                lit_emitter_emitexpr(emt, stmt->condition, condreg);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, sequence, condreg, 0);
                size_t iterator = lit_emitter_reserveregister(emt);
                lit_emitter_marklocalinit(emt, lit_emitter_addlocal(emt, "iter ", 5, topexpr->line, false, iterator));
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_LOADNULL, iterator, 0, 0);
                size_t start = emt->chunk->compiledcodecount;
                emt->loop_start = emt->chunk->compiledcodecount;
                /* iter = seq.iterator(iter) */
                uint8_t tmprega = lit_emitter_reserveregister(emt);
                uint8_t tmpregb = lit_emitter_reserveregister(emt);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_INVOKE, tmprega, 2, lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copy(emt->pstate, "iterator"))));
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, iterator, tmprega, 0);
                /* if iter is null, just get out of the loop */
                size_t exitjump = lit_emitter_emittmp(emt);
                lit_emitter_scopebegin(emt);
                /* var i = seq.iteratorValue(iter) */
                LitAstVarDeclExpr* var = (LitAstVarDeclExpr*)stmt->var;
                size_t local = lit_emitter_reserveregister(emt);
                lit_emitter_marklocalinit(emt, lit_emitter_addlocal(emt, var->name, var->length, topexpr->line, false, local));
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_INVOKE, tmprega, 2, lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(lit_string_copy(emt->pstate, "iteratorValue"))));
                lit_emitter_emitabc(emt, emt->last_line, LIT_OPCODE_MOVE, local, tmprega, 0);
                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_ASTEXPRTYP_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitAstBlockExpr*)stmt->body)->statements;
                        for(i = 0; i < statements->listcount; i++)
                        {
                            lit_emitter_emitstmt(emt, statements->listitems[i]);
                        }
                    }
                    else
                    {
                        lit_emitter_emitstmt(emt, stmt->body);
                    }
                }
                lit_emitter_patchloopjumps(emt, &emt->continues);
                lit_emitter_scopeend(emt);
                lit_emitter_emitasbx(emt, topexpr->line, LIT_OPCODE_JUMP, 0, (int)start - emt->chunk->compiledcodecount - 1);
                lit_emitter_patchinstr(emt, exitjump, LIT_REG_FORMABXINST(LIT_OPCODE_JUMPIFNULL, iterator, (int64_t)emt->chunk->compiledcodecount - exitjump - 1));
                lit_emitter_freeregister(emt, tmprega);
                lit_emitter_freeregister(emt, tmpregb);
                lit_emitter_freeregister(emt, condreg);
            }
            lit_emitter_patchloopjumps(emt, &emt->breaks);
            lit_emitter_scopeend(emt);
            emt->breaks = old_breaks;
            emt->continues = old_continues;
            emt->compiler->loop_depth--;
            break;
        }
        case LIT_ASTEXPRTYP_BREAK:
        {
            if(emt->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "cannot use '%s' outside of loops", "break");
            }
            lit_dynlistuint_push(&emt->breaks, lit_emitter_emittmp(emt));
            break;
        }
        case LIT_ASTEXPRTYP_CONTINUE:
        {
            if(emt->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "cannot use '%s' outside of loops", "continue");
            }
            lit_dynlistuint_push(&emt->continues, lit_emitter_emittmp(emt));
            break;
        }
        case LIT_ASTEXPRTYP_CLASS:
        {
            size_t i;
            LitAstClassExpr* stmt = (LitAstClassExpr*)topexpr;
            bool hasparent = stmt->parent != NULL;
            uint16_t b = 0;
            emt->class_name = stmt->name;
            if(hasparent)
            {
                uint16_t constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(stmt->parent));
                b = lit_emitter_reserveregister(emt);
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_GLOBALGET, b, constant);
            }
            int nameconst = lit_emitter_addconst(emt, emt->last_line, lit_value_fromobject(stmt->name));
            uint8_t class_register = lit_emitter_reserveregister(emt);
            emt->class_register = class_register;
            lit_emitter_emitabc(emt, topexpr->line, LIT_OPCODE_CLASSMAKE, nameconst, hasparent ? b + 1 : 0, class_register);
            if(hasparent)
            {
                lit_emitter_freeregister(emt, b);
                emt->class_has_super = true;
                lit_emitter_scopebegin(emt);
                size_t super = lit_emitter_addlocal(emt, "super", 5, emt->last_line, false, lit_emitter_reserveregister(emt));
                lit_emitter_marklocalinit(emt, super);
            }
            for(i = 0; i < stmt->staticfields.listcount; i++)
            {
                LitAstExpression* s = stmt->staticfields.listitems[i];
                if(s->type == LIT_ASTEXPRTYP_VARDECL)
                {
                    LitAstVarDeclExpr* var = (LitAstVarDeclExpr*)s;
                    uint8_t reg = lit_emitter_reserveregister(emt);
                    lit_emitter_emitexpr(emt, var->init, reg);
                    int fieldnameconst = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(lit_string_copylen(emt->pstate, var->name, var->length)));
                    lit_emitter_emitabc(emt, s->line, LIT_OPCODE_CLASSPUTFIELDSTATIC, class_register, fieldnameconst, reg);
                    lit_emitter_freeregister(emt, reg);
                }
                else
                {
                    lit_emitter_emitstmt(emt, s);
                }
            }
            if(stmt->parent != NULL)
            {
                lit_emitter_scopeend(emt);
            }
            lit_emitter_freeregister(emt, class_register);
            emt->class_name = NULL;
            emt->class_has_super = false;
            break;
        }
        case LIT_ASTEXPRTYP_METHOD:
        {
            bool isconstructor;
            size_t i;
            size_t ctorlen;
            const char* ctorstr;
            LitString* clsname;
            LitAstMethodExpr* stmt;
            ctorlen = lit_string_getlength(emt->pstate->strings.strconstructor);
            ctorstr = lit_string_getdata(emt->pstate->strings.strconstructor);
            stmt = (LitAstMethodExpr*)topexpr;
            isconstructor = stmt->name->strbuf.length == ctorlen && memcmp(stmt->name->strbuf.data, ctorstr, ctorlen) == 0;
            if(isconstructor && stmt->is_static)
            {
                lit_emitter_raiseerror(emt, topexpr->line, "constructors cannot be static");
            }
            LitAstCompiler stackcc;
            lit_emitter_compilerinit(emt, &stackcc, isconstructor ? LIT_FUNCTYPE_CONSTRUCTOR : (stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD));
            lit_emitter_scopebegin(emt);
            bool vararg = lit_emitter_emitparams(emt, &stmt->parameters, topexpr->line);
            if(!lit_emitter_emitstmt(emt, stmt->body))
            {
                lit_emitter_scopeend(emt);
            }
            clsname = (LitString*)lit_value_asobject(lit_value_fromobject(emt->class_name));
            LitFuncScript* function = lit_emitter_compilerend(emt, clsname);
            function->argcount = stmt->parameters.listcount;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint64_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emt);
                LitClsPrototype* clsproto = lit_object_makeclsproto(emt->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitAstUpvalue* upvalue = &stackcc.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emt, topexpr->line, LIT_OPCODE_MAKECLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            int fieldnameconst = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(stmt->name));
            lit_emitter_emitabc(emt, topexpr->line, stmt->is_static ? LIT_OPCODE_CLASSPUTFIELDSTATIC : LIT_OPCODE_CLASSPUTMETHOD, emt->class_register, fieldnameconst, functionreg);
            if(closure)
            {
                lit_emitter_freeregister(emt, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_FIELD:
        {
            LitAstFieldExpr* stmt = (LitAstFieldExpr*)topexpr;
            LitFuncScript* getter = NULL;
            LitFuncScript* setter = NULL;
            if(stmt->getter != NULL)
            {
                LitAstCompiler stackcc;
                lit_emitter_compilerinit(emt, &stackcc, stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD);
                lit_emitter_scopebegin(emt);
                if(stmt->getter->type == LIT_ASTEXPRTYP_EXPRESSION)
                {
                    uint8_t r = lit_emitter_reserveregister(emt);
                    stackcc.skip_return = true;
                    lit_emitter_emitexpr(emt, ((LitAstExprStmtExpr*)stmt->getter)->exvalue, r);
                    lit_emitter_emitabc(emt, stmt->getter->line, LIT_OPCODE_RETURN, r, 0, 0);
                    lit_emitter_freeregister(emt, r);
                }
                if(!lit_emitter_emitstmt(emt, stmt->getter))
                {
                    lit_emitter_scopeend(emt);
                }
                getter = lit_emitter_compilerend(emt, lit_value_asstring(lit_string_valformat(emt->pstate, "@:get @", lit_value_fromobject(emt->class_name), stmt->name)));
            }
            if(stmt->setter != NULL)
            {
                LitAstCompiler stackcc;
                lit_emitter_compilerinit(emt, &stackcc, stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD);
                uint8_t reg = lit_emitter_reserveregister(emt);
                lit_emitter_marklocalinit(emt, lit_emitter_addlocal(emt, "value", 5, topexpr->line, false, reg));
                lit_emitter_scopebegin(emt);
                if(!lit_emitter_emitstmt(emt, stmt->setter))
                {
                    lit_emitter_scopeend(emt);
                }
                lit_emitter_freeregister(emt, reg);
                setter = lit_emitter_compilerend(emt, lit_value_asstring(lit_string_valformat(emt->pstate, "@:set @", lit_value_fromobject(emt->class_name), stmt->name)));
                setter->argcount = 1;
                setter->maxregisters++;
            }
            LitField* field = lit_object_makefield(emt->pstate, (LitObject*)getter, (LitObject*)setter);
            uint64_t constant = lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(field));
            LIT_BIT_SETBIT(constant, LIT_BITFLAG_CONSTANT);
            lit_emitter_emitabc(emt, topexpr->line, stmt->is_static ? LIT_OPCODE_CLASSPUTFIELDSTATIC : LIT_OPCODE_CLASSPUTMETHOD, emt->class_register, lit_emitter_addconst(emt, topexpr->line, lit_value_fromobject(stmt->name)), constant);
            break;
        }
        default:
        {
            lit_emitter_raiseerror(emt, topexpr->line, "unknown statement with id '%i'", (int)topexpr->type);
            break;
        }
    }
    return false;
}

LitModule* lit_emitter_emitmod(LitAstEmitter* emt, LitDynListExpr* statements, LitString* modname)
{
    size_t i;
    LitAstPrivate priv;
    emt->last_line = 1;
    emt->emit_reference = 0;
    LitState* state = emt->pstate;
    LitValue modulevalue;
    LitModule* module;
    bool isnew = false;
    if(lit_map_get(emt->pstate->vmstate.modules, modname, &modulevalue))
    {
        module = lit_value_asmodule(modulevalue);
    }
    else
    {
        module = lit_object_makemodule(emt->pstate, modname);
        isnew = true;
    }
    emt->module = module;
    size_t oldprivatescnt = module->privatecount;
    if(oldprivatescnt > 0)
    {
        LitDynListPriv* privates = &emt->privlist;
        privates->listcount = oldprivatescnt - 1;
        priv.initialized = true;
        priv.constant = false;
        lit_dynlistpriv_push(privates, priv);
        for(i = 0; i < oldprivatescnt; i++)
        {
            privates->listitems[i].initialized = true;
        }
    }
    LitAstCompiler stackcc;
    lit_emitter_compilerinit(emt, &stackcc, LIT_FUNCTYPE_SCRIPT);
    emt->chunk = &stackcc.function->chunk;
    lit_emitter_resolvestmtlist(emt, statements);
    lit_emitter_scopebegin(emt);
    bool endedscope = false;
    for(i = 0; i < statements->listcount; i++)
    {
        LitAstExpression* stmt = statements->listitems[i];
        if(i == statements->listcount - 1 && stmt->type == LIT_ASTEXPRTYP_EXPRESSION && !endedscope)
        {
            uint8_t r = lit_emitter_reserveregister(emt);
            lit_emitter_emitexpr(emt, ((LitAstExprStmtExpr*)stmt)->exvalue, r);
            lit_emitter_emitabc(emt, stmt->line, LIT_OPCODE_RETURN, r, 1, 0);
            lit_emitter_freeregister(emt, r);
            emt->compiler->skip_return = true;
            break;
        }
        if(lit_emitter_emitstmt(emt, stmt))
        {
            endedscope = true;
            break;
        }
    }
    if(!endedscope)
    {
        lit_emitter_scopeend(emt);
    }
    module->mainfunction = lit_emitter_compilerend(emt, modname);
    if(isnew)
    {
        size_t total = emt->privlist.listcount;
        module->privatevalues = (LitValue*)lit_sysmem_malloc(total * sizeof(LitValue));
        for(i = 0; i < total; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
        }
    }
    else
    {
        size_t add = (module->privatecount);
        if(add == 0)
        {
            add = 1;
        }
        module->privatevalues = (LitValue*)lit_sysmem_realloc(module->privatevalues, sizeof(LitValue) * add);
        for(i = oldprivatescnt; i < module->privatecount; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
        }
    }
    lit_dynlistpriv_destroy(&emt->privlist);
    if(isnew && !state->had_error)
    {
        lit_map_set(state->vmstate.modules, modname, lit_value_fromobject(module));
    }
    module->ran = true;
    return module;
}

const char* lit_debug_opcname(uint64_t opc)
{
    switch(opc)
    {
                case LIT_OPCODE_MOVE: return "move";
                case LIT_OPCODE_LOADNULL: return "ldnull";
                case LIT_OPCODE_LOADBOOL: return "ldbool";
                case LIT_OPCODE_MAKECLOSURE: return "mkclosure";
                case LIT_OPCODE_MAKEARRAY: return "mkarray";
                case LIT_OPCODE_MAKEOBJECT: return "mkobject";
                case LIT_OPCODE_MAKERANGE: return "mkrange";
                case LIT_OPCODE_RETURN: return "return";
                case LIT_OPCODE_MATHADD: return "m.add";
                case LIT_OPCODE_MATHSUBTRACT: return "m.subtr";
                case LIT_OPCODE_MATHMULTIPLY: return "m.mult";
                case LIT_OPCODE_MATHDIVIDE: return "m.div";
                case LIT_OPCODE_MATHFLOORDIVIDE: return "m.fldiv";
                case LIT_OPCODE_MATHMOD: return "m.mod";
                case LIT_OPCODE_MATHPOWER: return "m.pow";
                case LIT_OPCODE_MATHLEFTSHIFT: return "m.shleft";
                case LIT_OPCODE_MATHRIGHTSHIFT: return "m.shright";
                case LIT_OPCODE_BINXOR: return "m.bxor";
                case LIT_OPCODE_BINAND: return "m.band";
                case LIT_OPCODE_BINOR: return "m.bor";
                case LIT_OPCODE_JUMP: return "jump";
                case LIT_OPCODE_JUMPIFTRUE: return "jumpiftrue";
                case LIT_OPCODE_JUMPIFFALSE: return "jumpiffalse";
                case LIT_OPCODE_JUMPIFNONNULL: return "non_null_jump";
                case LIT_OPCODE_JUMPIFNULL: return "null_jump";
                case LIT_OPCODE_EQUAL: return "eq";
                case LIT_OPCODE_LESSTHAN: return "lessthan";
                case LIT_OPCODE_LESSEQUAL: return "lessequal";
                case LIT_OPCODE_GREATERTHAN: return "greater";
                case LIT_OPCODE_GREATEREQUAL: return "greaterequal";
                case LIT_OPCODE_NEGATE: return "negate";
                case LIT_OPCODE_NOT: return "not";
                case LIT_OPCODE_BINNOT: return "m.bnot";
                case LIT_OPCODE_GLOBALSET: return "globalset";
                case LIT_OPCODE_GLOBALGET: return "globalget";
                case LIT_OPCODE_UPVALUESET: return "upvset";
                case LIT_OPCODE_UPVALUEGET: return "upvget";
                case LIT_OPCODE_PRIVATESET: return "privset";
                case LIT_OPCODE_PRIVATEGET: return "privget";
                case LIT_OPCODE_CALLCALLABLE: return "call";
                case LIT_OPCODE_UPVALUECLOSE: return "upvclose";
                case LIT_OPCODE_CLASSMAKE: return "mkclass";
                case LIT_OPCODE_CLASSPUTFIELDSTATIC: return "classputstaticfield";
                case LIT_OPCODE_CLASSPUTMETHOD: return "classputmethod";
                case LIT_OPCODE_FIELDGET: return "fieldget";
                case LIT_OPCODE_CLASSGETSUPERMETHOD: return "classgetsupermethod";
                case LIT_OPCODE_FIELDSET: return "fieldset";
                case LIT_OPCODE_IS: return "is";
                case LIT_OPCODE_INVOKE: return "classinvoke";
                case LIT_OPCODE_INVOKESUPER: return "classinvokesuper";
                case LIT_OPCODE_SUBSCRIPTGET: return "indexget";
                case LIT_OPCODE_SUBSCRIPTSET: return "indexset";
                case LIT_OPCODE_ARRAYPUSH: return "arraypush";
                case LIT_OPCODE_OBJECTPUSH: return "objectpush";
                case LIT_OPCODE_REFGLOBAL: return "refglobal";
                case LIT_OPCODE_REFPRIVATE: return "refprivate";
                case LIT_OPCODE_REFLOCAL: return "reflocal";
                case LIT_OPCODE_REFUPVALUE: return "refupvalue";
                case LIT_OPCODE_REFFIELD: return "reffield";
                case LIT_OPCODE_REFSET: return "setref";
                case LIT_OPCODE_PUSH_TRY: return "pushtry";
                case LIT_OPCODE_POP_TRY: return "poptry";
                case LIT_OPCODE_THROW: return "throw";
                case LIT_OPCODE_RETHROW: return "rethrow";
    }
    return "?unknown?";
}

void lit_debug_disasmodule_recursive(LitState* state, LitIOStream* pr, LitFuncScript* function, const char* source, LitTable* disassembled)
{
    if(function == NULL || lit_table_getentry(disassembled, function->name, NULL))
    {
        return;
    }
    lit_table_set(disassembled, function->name, lit_value_makenull());
    lit_debug_disaschunk(pr, &function->chunk, function->name->strbuf.data, source);
    size_t i;
    for(i = 0; i < function->chunk.constantlist.listcount; i++)
    {
        LitValue val = function->chunk.constantlist.listitems[i];
        if(lit_value_isfuncscript(val))
        {
            lit_debug_disasmodule_recursive(state, pr, lit_value_asfuncscript(val), source, disassembled);
        }
        else if(lit_value_isobjtype(val, LIT_OBJ_FUNCCLOSURE))
        {
            lit_debug_disasmodule_recursive(state, pr, ((LitFuncClosure*)lit_value_asobject(val))->function, source, disassembled);
        }
        else if(lit_value_isobjtype(val, LIT_OBJ_CLSPROTOTYPE))
        {
            lit_debug_disasmodule_recursive(state, pr, ((LitClsPrototype*)lit_value_asobject(val))->function, source, disassembled);
        }
    }
}

void lit_debug_disasmodule(LitIOStream* pr, LitModule* module, const char* source)
{
    LitTable disassembled;
    LitState* state = module->innerobject.pstate;
    lit_table_init(state, &disassembled);
    lit_debug_disasmodule_recursive(state, pr, module->mainfunction, source, &disassembled);
    lit_free_table(&disassembled);
}

void lit_debug_printconst(LitIOStream* pr, LitValue value)
{
    lit_iostream_writecolor(pr, COLOR_CYAN);
    lit_value_printvalue(pr, value, true);
    lit_iostream_writecolor(pr, COLOR_RESET);
}

void lit_debug_disaschunk(LitIOStream* pr, LitChunk* chunk, const char* name, const char* source)
{
    size_t i;
    size_t offset;
    LitDynListVal* list = &chunk->constantlist;
    lit_iostream_printf(pr, "^^ %s ^^\n", name);
    if(list->listcount > 0)
    {
        lit_iostream_writecolor(pr, COLOR_MAGENTA);
        lit_iostream_printf(pr, "constants:\n");
        lit_iostream_writecolor(pr, COLOR_RESET);
        for(i = 0; i < list->listcount; i++)
        {
            LitValue value = list->listitems[i];
            lit_iostream_printf(pr, "% 4ld ", i);
            lit_debug_printconst(pr, value);
            lit_iostream_printf(pr, "\n");
        }
    }
    lit_iostream_writecolor(pr, COLOR_MAGENTA);
    lit_iostream_printf(pr, "text:\n");
    lit_iostream_writecolor(pr, COLOR_RESET);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        lit_debug_disasinstr(pr, chunk, offset, source, false);
    }
    lit_iostream_writecolor(pr, COLOR_MAGENTA);
    lit_iostream_printf(pr, "hex:\n");
    lit_iostream_writecolor(pr, COLOR_RESET);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        lit_iostream_printf(pr, "%08lX ", chunk->compiledcodechunk[offset]);
    }
    lit_iostream_printf(pr, "\n");
    lit_iostream_printf(pr, "vv %s vv\n", name);
}

void lit_debug_callbackprintabcinstr(LitIOStream* pr, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s %lu \t%lu \t%lu\n", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "", LIT_INST_GETA(instruction), LIT_INST_GETB(instruction), LIT_INST_GETC(instruction));
}

void lit_debug_callbackprintabxinstr(LitIOStream* pr, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s %lu \t%lu\n", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "", LIT_INST_GETA(instruction), LIT_INST_GETBX(instruction));
}

void lit_debug_callbackprintasbxinstr(LitIOStream* pr, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s %lu \t%li\n", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "", LIT_INST_GETA(instruction), LIT_INST_GETSBX(instruction));
}

void lit_debug_printregister(LitIOStream* pr, uint16_t reg)
{
    lit_iostream_printf(pr, " \t%hu", reg);
}

void lit_debug_printconstarg(LitIOStream* pr, LitChunk* chunk, uint32_t arg, bool indent)
{
    lit_iostream_printf(pr, "%sc%u (", indent ? " \t" : "", arg);
    lit_debug_printconst(pr, chunk->constantlist.listitems[arg]);
    lit_iostream_printf(pr, ")");
}

void lit_debug_printconstorregister(LitIOStream* pr, LitChunk* chunk, uint16_t arg)
{
    if(LIT_BIT_ISSET(arg, LIT_BITFLAG_CONSTANT))
    {
        lit_debug_printconstarg(pr, chunk, arg & ~(1UL << LIT_BITFLAG_CONSTANT), true);
    }
    else
    {
        lit_debug_printregister(pr, arg);
    }
}

void lit_debug_printunaryinstr(LitIOStream* pr, LitChunk* chunk, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s %lu", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "", LIT_INST_GETA(instruction));
    lit_debug_printconstorregister(pr, chunk, LIT_INST_GETB(instruction));
    lit_iostream_printf(pr, "\n");
}

void lit_debug_printbinaryinstr(LitIOStream* pr, LitChunk* chunk, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s %lu", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "", LIT_INST_GETA(instruction));
    lit_debug_printconstorregister(pr, chunk, LIT_INST_GETB(instruction));
    lit_debug_printconstorregister(pr, chunk, LIT_INST_GETC(instruction));
    lit_iostream_printf(pr, "\n");
}

void lit_debug_printglobalinstr(LitIOStream* pr, LitChunk* chunk, LitOpCode opc, uint64_t instruction)
{
    const char* name = lit_debug_opcname(opc);
    lit_iostream_writecolor(pr, COLOR_YELLOW);
    lit_iostream_printf(pr, "%s", name);
    lit_iostream_writecolor(pr, COLOR_RESET);
    lit_iostream_printf(pr, "%*s", LIT_CONFIG_LONGESTOPNAME - (int)strlen(name), "");
    if (opc == LIT_OPCODE_GLOBALSET)
    {
        lit_debug_printconstarg(pr, chunk, (uint16_t)LIT_INST_GETA(instruction), false);
        lit_debug_printconstorregister(pr, chunk, (uint16_t)LIT_INST_GETBX(instruction));
    }
    else
    {
        lit_debug_printregister(pr, (uint16_t)LIT_INST_GETA(instruction));
        lit_debug_printconstarg(pr, chunk, (uint16_t)LIT_INST_GETBX(instruction), true);
    }
    lit_iostream_printf(pr, "\n");
}

void lit_debug_disasinstr(LitIOStream* pr, LitChunk* chunk, size_t offset, const char* source, bool forceline)
{
    typedef void (*LitDebugInstructionFn)(LitIOStream*, LitOpCode opc, uint64_t);
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
    static LitDebugInstructionFn debuginstrfuncs[] = {
        lit_debug_callbackprintabcinstr,
        lit_debug_callbackprintabxinstr,
        lit_debug_callbackprintasbxinstr
    };
    line = lit_chunk_getline(chunk, offset);
    same = !chunk->haslineinfo || (offset > 0 && line == lit_chunk_getline(chunk, offset - 1));
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
                lit_iostream_writecolor(pr, COLOR_RED);
                lit_iostream_printf(pr, "        %.*s\n", nextline ? (int)(nextline - outputline) : (int)strlen(prevline), outputline);
                lit_iostream_writecolor(pr, COLOR_RESET);
                break;
            }
        }
    }
    lit_iostream_printf(pr, "%04ld ", offset);
    if(same && !forceline)
    {
        lit_iostream_printf(pr, "   | ");
    }
    else
    {
        lit_iostream_writecolor(pr, COLOR_BLUE);
        lit_iostream_printf(pr, "%4ld ", line);
        lit_iostream_writecolor(pr, COLOR_RESET);
    }
    instruction = chunk->compiledcodechunk[offset];
    opcode = LIT_INST_GETOPCODE(instruction);
    switch(opcode)
    {
        case LIT_OPCODE_MOVE:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_MATHADD:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_MATHSUBTRACT:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_MATHMULTIPLY:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_MATHDIVIDE:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_NEGATE:
            lit_debug_printunaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_NOT:
            lit_debug_printunaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_EQUAL:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_LESSTHAN:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_LESSEQUAL:
            lit_debug_printbinaryinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_GLOBALSET:
            lit_debug_printglobalinstr(pr, chunk, opcode, instruction);
            break;
        case LIT_OPCODE_GLOBALGET:
            lit_debug_printglobalinstr(pr, chunk, opcode, instruction);
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
                handle_opcode(LIT_OPCODE_MOVE, LIT_INSTYP_ABC) /* R(A) := RC(B) */
                handle_opcode(LIT_OPCODE_LOADNULL, LIT_INSTYP_ABC) /* R(A) := null */
                handle_opcode(LIT_OPCODE_LOADBOOL, LIT_INSTYP_ABC) /* R(A) := (bool) B */
                handle_opcode(LIT_OPCODE_MAKECLOSURE, LIT_INSTYP_ABX) /* R(A) := PrC[Bx] */
                handle_opcode(LIT_OPCODE_MAKEARRAY, LIT_INSTYP_ABX) /* R(A) := new Array(Bx) */
                handle_opcode(LIT_OPCODE_MAKEOBJECT, LIT_INSTYP_ABC) /* R(A) = new Object() */
                handle_opcode(LIT_OPCODE_MAKERANGE, LIT_INSTYP_ABC) /* R(A) = new Range(RC(B), RC(C)) */
                handle_opcode(LIT_OPCODE_RETURN, LIT_INSTYP_ABC) /* return R(A) */
                handle_opcode(LIT_OPCODE_MATHADD, LIT_INSTYP_ABC) /* R(A) := RC(B) + RC(C) */
                handle_opcode(LIT_OPCODE_MATHSUBTRACT, LIT_INSTYP_ABC) /* R(A) := RC(B) - RC(C) */
                handle_opcode(LIT_OPCODE_MATHMULTIPLY, LIT_INSTYP_ABC) /* R(A) := RC(B) * RC(C) */
                handle_opcode(LIT_OPCODE_MATHDIVIDE, LIT_INSTYP_ABC) /* R(A) := RC(B) / RC(C) */
                handle_opcode(LIT_OPCODE_MATHFLOORDIVIDE, LIT_INSTYP_ABC) /* R(A) := floor(RC(B) / RC(C)) */
                handle_opcode(LIT_OPCODE_MATHMOD, LIT_INSTYP_ABC) /* R(A) := RC(B) % RC(C) */
                handle_opcode(LIT_OPCODE_MATHPOWER, LIT_INSTYP_ABC) /* R(A) := pow(RC(B), RC(C)) */
                handle_opcode(LIT_OPCODE_MATHLEFTSHIFT, LIT_INSTYP_ABC) /* R(A) := RC(B) << RC(C) */
                handle_opcode(LIT_OPCODE_MATHRIGHTSHIFT, LIT_INSTYP_ABC) /* R(A) := RC(B) >> RC(C) */
                handle_opcode(LIT_OPCODE_BINXOR, LIT_INSTYP_ABC) /* R(A) := RC(B) ^ RC(C) */
                handle_opcode(LIT_OPCODE_BINAND, LIT_INSTYP_ABC) /* R(A) := RC(B) & RC(C) */
                handle_opcode(LIT_OPCODE_BINOR, LIT_INSTYP_ABC) /* R(A) := RC(B) | RC(C) */
                handle_opcode(LIT_OPCODE_JUMP, LIT_INSTYP_ASBX) /* PC += sBx */
                handle_opcode(LIT_OPCODE_JUMPIFTRUE, LIT_INSTYP_ABX) /* if (R(A)) PC += Bx */
                handle_opcode(LIT_OPCODE_JUMPIFFALSE, LIT_INSTYP_ABX) /* if (not R(A)) PC += Bx */
                handle_opcode(LIT_OPCODE_JUMPIFNONNULL, LIT_INSTYP_ABX) /* if (R(A) != null) PC += Bx */
                handle_opcode(LIT_OPCODE_JUMPIFNULL, LIT_INSTYP_ABX) /* if (R(A) == null) PC += Bx */
                handle_opcode(LIT_OPCODE_EQUAL, LIT_INSTYP_ABC) /* R(A) := RC(B) == RC(C) */
                handle_opcode(LIT_OPCODE_LESSTHAN, LIT_INSTYP_ABC) /* R(A) := RC(B) < RC(C) */
                handle_opcode(LIT_OPCODE_LESSEQUAL, LIT_INSTYP_ABC) /* R(A) := RC(B) <= RC(C) */
                handle_opcode(LIT_OPCODE_GREATERTHAN, LIT_INSTYP_ABC) /* R(A) := RC(B) > RC(C) */
                handle_opcode(LIT_OPCODE_GREATEREQUAL, LIT_INSTYP_ABC) /* R(A) := RC(B) >= RC(C) */
                handle_opcode(LIT_OPCODE_NEGATE, LIT_INSTYP_ABC) /* R(A) := -RC(B) */
                handle_opcode(LIT_OPCODE_NOT, LIT_INSTYP_ABC) /* R(A) := !RC(B) */
                handle_opcode(LIT_OPCODE_BINNOT, LIT_INSTYP_ABC) /* R(A) := ~RC(B) */
                handle_opcode(LIT_OPCODE_GLOBALSET, LIT_INSTYP_ABX) /* G[C(A)] := RC(BX) */
                handle_opcode(LIT_OPCODE_GLOBALGET, LIT_INSTYP_ABX) /* R(A) := G[C(Bx)] */
                handle_opcode(LIT_OPCODE_UPVALUESET, LIT_INSTYP_ABX) /* U[A] := RC(Bx) */
                handle_opcode(LIT_OPCODE_UPVALUEGET, LIT_INSTYP_ABX) /* R(A) := U[Bx] */
                handle_opcode(LIT_OPCODE_PRIVATESET, LIT_INSTYP_ABX) /* P[A] := RC(Bx) */
                handle_opcode(LIT_OPCODE_PRIVATEGET, LIT_INSTYP_ABX) /* R(A) := P[C(Bx)] */
                handle_opcode(LIT_OPCODE_CALLCALLABLE, LIT_INSTYP_ABC) /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(LIT_OPCODE_UPVALUECLOSE, LIT_INSTYP_ABC) /* close_upvalue(R(A)) */
                handle_opcode(LIT_OPCODE_CLASSMAKE, LIT_INSTYP_ABC) /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
                handle_opcode(LIT_OPCODE_CLASSPUTFIELDSTATIC, LIT_INSTYP_ABC) /* R(A)[C(B)] = RC(C) */
                handle_opcode(LIT_OPCODE_CLASSPUTMETHOD, LIT_INSTYP_ABC) /* R(A).Methods[C(B)] = RC(C) */
                handle_opcode(LIT_OPCODE_FIELDGET, LIT_INSTYP_ABC) /* R(A) = R(B)[C(C)] */
                handle_opcode(LIT_OPCODE_CLASSGETSUPERMETHOD, LIT_INSTYP_ABC) /* R(A) = R(B).super[C(C)] */
                handle_opcode(LIT_OPCODE_FIELDSET, LIT_INSTYP_ABC) /* R(A)[C(B)] = R(C) */
                handle_opcode(LIT_OPCODE_IS, LIT_INSTYP_ABC) /* R(A) := RC(B) is G[C(C)] */
                handle_opcode(LIT_OPCODE_INVOKE, LIT_INSTYP_ABC) /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(LIT_OPCODE_INVOKESUPER, LIT_INSTYP_ABC) /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
                handle_opcode(LIT_OPCODE_SUBSCRIPTGET, LIT_INSTYP_ABC) /* R(A) := R(A)[RC(B)] */
                handle_opcode(LIT_OPCODE_SUBSCRIPTSET, LIT_INSTYP_ABC) /* R(A)[RC(B)] := R(C) */
                handle_opcode(LIT_OPCODE_ARRAYPUSH, LIT_INSTYP_ABX) /* R(A)[R(A).listcount++] = RC(Bx) */
                handle_opcode(LIT_OPCODE_OBJECTPUSH, LIT_INSTYP_ABC) /* R(A)[R(B)] = RC(C) */
                handle_opcode(LIT_OPCODE_REFGLOBAL, LIT_INSTYP_ABX) /* R(A) := ref G(C[Bx]) */
                handle_opcode(LIT_OPCODE_REFPRIVATE, LIT_INSTYP_ABX) /* R(A) := ref P(Bx) */
                handle_opcode(LIT_OPCODE_REFLOCAL, LIT_INSTYP_ABC) /* R(A) := ref R(B) */
                handle_opcode(LIT_OPCODE_REFUPVALUE, LIT_INSTYP_ABX) /* R(A) := ref U(Bx) */
                handle_opcode(LIT_OPCODE_REFFIELD, LIT_INSTYP_ABC) /* R(A) = ref R(B)[C(C)] */
                handle_opcode(LIT_OPCODE_REFSET, LIT_INSTYP_ABC) /* ref R(A) := R(B) */
                handle_opcode(LIT_OPCODE_PUSH_TRY, LIT_INSTYP_ABX) /* push try handler at PC + Bx */
                handle_opcode(LIT_OPCODE_POP_TRY, LIT_INSTYP_ABC) /* pop try handler */
                handle_opcode(LIT_OPCODE_THROW, LIT_INSTYP_ABC) /* throw R(A) */
                handle_opcode(LIT_OPCODE_RETHROW, LIT_INSTYP_ABC) /* rethrow fiber->error */
                #endif
                #undef handle_opcode
                default:
                {
                    lit_iostream_printf(pr, "unknown opcode %d\n", opcode);
                    break;
                }
            }
        }
    }
}

LitString* lit_state_errorfmtv(LitState* state, size_t line, const char* fmt, va_list args)
{
    LitIOStream pr;
    lit_iostream_makestackstring(&pr);
    lit_iostream_printf(&pr, "[line %ld]: ", line);
    lit_iostream_vwritefmt(&pr, fmt, args);
    return lit_iostream_takestring(state, &pr);
}

LitString* lit_state_errorfmt(LitState* state, size_t line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(state, line, fmt, args);
    va_end(args);
    return result;
}

void lit_state_openlibraries(LitState* state)
{
    lit_corelib_installmath(state);
    lit_corelib_installfile(state);
    lit_corelib_installgc(state);
}

LitValue lit_objfndefault_invalidconstructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    const char* cname;
    LitString* name;
    (void)argc;
    (void)args;
    name = NULL; 
    cname = "?unknown?";
    if(lit_value_isinstance(instance))
    {
        name = lit_value_asinstance(instance)->klass->name;
    }
    else if(lit_value_isclass(instance))
    {
        name = lit_value_asclass(instance)->name;
    }
    if(name != NULL)
    {
        cname = lit_string_getdata(name);
    }
    lit_vm_raisefatalerror(state, "class %s has no constructor", cname);
    return lit_value_makenull();
}

/*
 * Class
 */

LitValue lit_objfnclass_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_valformat(state, "class @", lit_value_fromobject(lit_value_asclass(instance)->name));
}

int lit_coreutil_tableiterator(LitTable* table, int number)
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

LitValue lit_coreutil_tableiterkey(LitTable* table, int index)
{
    if(table->htcapacity <= index)
    {
        return lit_value_makenull();
    }
    return lit_value_fromobject(table->htentries[index].entkey);
}

LitValue lit_objfnclass_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitClass* klass = lit_value_asclass(instance);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int methodsCapacity = (int)klass->mthtable.htcapacity;
    bool fields = index >= methodsCapacity;
    int value = lit_coreutil_tableiterator(fields ? &klass->staticstable : &klass->mthtable, fields ? index - methodsCapacity : index);
    if(value == -1)
    {
        if(fields)
        {
            return lit_value_makenull();
        }
        index++;
        fields = true;
        value = lit_coreutil_tableiterator(&klass->staticstable, index - methodsCapacity);
    }
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(fields ? value + methodsCapacity : value);
}

LitValue lit_objfnclass_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    LitClass* klass = lit_value_asclass(instance);
    size_t methodsCapacity = klass->mthtable.htcapacity;
    bool fields = index >= methodsCapacity;
    return lit_coreutil_tableiterkey(fields ? &klass->staticstable : &klass->mthtable, fields ? index - methodsCapacity : index);
}

LitValue lit_objfnclass_super(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    LitClass* super = NULL;
    if(lit_value_isinstance(instance))
    {
        super = lit_value_asinstance(instance)->klass->super;
    }
    else
    {
        super = lit_value_asclass(instance)->super;
    }
    if(super == NULL)
    {
        return lit_value_makenull();
    }
    return lit_value_fromobject(super);
}

LitValue lit_objfnclass_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitClass* klass = lit_value_asclass(instance);
    if(argc == 2)
    {
        if(!lit_value_isstring(args[0]))
        {
            lit_vm_raisefatalerror(state, "class index must be a string");
        }
        lit_table_set(&klass->staticstable, lit_value_asstring(args[0]), args[1]);
        return args[1];
    }
    if(!lit_value_isstring(args[0]))
    {
        lit_vm_raisefatalerror(state, "class index must be a string");
    }
    LitValue value;
    if(lit_table_getentry(&klass->staticstable, lit_value_asstring(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&klass->mthtable, lit_value_asstring(args[0]), &value))
    {
        return value;
    }
    return lit_value_makenull();
}

LitValue lit_objfnclass_name(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return lit_value_fromobject(lit_value_asclass(instance)->name);
}

/*
 * Object
 */

void lit_objfnutil_tabtoarray(LitState* state, LitArray* arr, LitTable* table)
{
    size_t i;
    LitTabEntry* entry;
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
            lit_array_push(arr, lit_value_fromobject(entry->entkey));
        }
    }
}

LitValue lit_objfnobject_keys(LitState* state, LitValue thisval, size_t argc, LitValue* args)
{
    LitValue val;
    LitArray* arr;
    LitMap* map;
    LitInstance* oinst;
    (void)thisval;
    if(argc == 0)
    {
        lit_vm_raisefatalerror(state, "keys() requires an argument");
    }
    val = args[0];
    arr = lit_array_make(state);
    if(lit_value_ismap(val))
    {
        map = lit_value_asmap(val);
        lit_objfnutil_tabtoarray(state, arr, &map->innertable);
    }
    else if(lit_value_isinstance(val))
    {
        oinst = lit_value_asinstance(val);
        lit_objfnutil_tabtoarray(state, arr, &oinst->fields);
    }
    return lit_value_fromobject(arr);
}

LitValue lit_objfnobject_class(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_fromobject(lit_state_getclassfor(state, instance));
}

LitValue lit_objfnobject_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitIOStream pr;
    LitInstance* self;
    LitString* dest;
    (void)argc;
    (void)args;
    self = lit_value_asinstance(instance);
    LitClass* klass = lit_state_getclassfor(state, instance);
    lit_iostream_makestackstring(&pr);
    lit_value_printobjinstance(&pr, klass, self);
    dest = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(dest);
}

LitValue lit_objfnobject_dump(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitIOStream pr;
    LitString* dest;
    (void)argc;
    (void)args;
    lit_iostream_makestackstring(&pr);
    lit_value_printvalue(&pr, instance, true);
    dest = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(dest);
}

LitValue lit_objfnobject_iscallable(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makebool(lit_value_iscallablefunction(instance));
}

LitValue lit_objfnobject_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitObjType type;
    (void)type;
    if(!lit_value_isinstance(instance))
    {
        type = lit_value_objtype(instance);
        lit_vm_raisefatalerror(state, "cannot modify built-in types");
    }
    LitInstance* inst = lit_value_asinstance(instance);
    if(!lit_value_isstring(args[0]))
    {
        lit_vm_raisefatalerror(state, "object index must be a string");
    }
    if(argc == 2)
    {
        lit_table_set(&inst->fields, lit_value_asstring(args[0]), args[1]);
        return args[1];
    }
    LitValue value;
    if(lit_table_getentry(&inst->fields, lit_value_asstring(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&inst->klass->staticstable, lit_value_asstring(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&inst->klass->mthtable, lit_value_asstring(args[0]), &value))
    {
        return value;
    }
    return lit_value_makenull();
}

LitValue lit_objfnobject_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitInstance* self = lit_value_asinstance(instance);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int value = lit_coreutil_tableiterator(&self->fields, index);
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(value);
}

LitValue lit_objfnobject_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    LitInstance* self = lit_value_asinstance(instance);
    return lit_coreutil_tableiterkey(&self->fields, index);
}

/*
 * Number
 */

LitValue lit_objfnnumber_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_numbertostring(state, lit_value_asnumber(instance));
}

LitValue lit_objfnnumber_chr(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)args;
    (void)argc;
    char c;
    double dn;
    LitString* cs;
    dn = lit_value_asnumber(instance);
    c = dn;
    cs = lit_string_copylen(state, &c, 1);
    return lit_value_fromobject(cs);
}

/*
 * Bool
 */

LitValue lit_objfnbool_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_fromobject(lit_string_copy(state, lit_value_asbool(instance) ? "true" : "false"));
}

/*
 * String
 */


LitValue lit_objfnstring_chr(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    char ch;
    (void)instance;
    (void)argc;
    ch = lit_value_asnumber(args[0]);
    return lit_value_fromobject(lit_string_copylen(state, &ch, 1));
}

LitValue lit_objfnstring_plus(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    //fprintf(stderr, "in objfnstring_plus\n");
    LitString* self;
    LitString* clone;
    LitString* other;
    LitString* result;
    size_t length;
    char* chars;
    (void)argc;
    self = lit_value_asstring(instance);
    clone = lit_string_clone(state, self);
    other = lit_value_tostring(state, args[0], 0);
    lit_string_appendlen(clone, lit_string_getdata(other), lit_string_getlength(other));
    return lit_value_fromobject(clone);

    length = self->strbuf.length + other->strbuf.length;
    chars = (char*)lit_sysmem_malloc(length + 1);
    memcpy(chars, self->strbuf.data, self->strbuf.length);
    memcpy(chars + self->strbuf.length, other->strbuf.data, other->strbuf.length);
    chars[length] = '\0';

    result = lit_string_makestringfrom(state, chars, length, lit_string_hash(chars, length), true);
    return lit_value_fromobject(result);
}


LitValue lit_objfnstring_less(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    return lit_value_makebool(strcmp(lit_value_asstring(instance)->strbuf.data, LIT_CHECK_GETSTRINGDATA(0)) < 0);
}

LitValue lit_objfnstring_greater(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    return lit_value_makebool(strcmp(lit_value_asstring(instance)->strbuf.data, LIT_CHECK_GETSTRINGDATA(0)) > 0);
}

LitValue lit_objfnstring_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return instance;
}

LitValue lit_objfnstring_tonumber(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    double result = strtod(lit_value_asstring(instance)->strbuf.data, NULL);
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_value_makenull();
    }
    return lit_value_makenumber(result);
}


LitValue lit_objfnstring_touppercase(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr;
    LitString* copied;
    (void)argc;
    (void)args;
    selfstr = lit_value_asstring(instance);
    copied = lit_string_clone(state, selfstr);
    lit_util_stringchangecase(copied->strbuf.data, copied->strbuf.length, toupper);
    return lit_value_fromobject(copied);
}

LitValue lit_objfnstring_tolowercase(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr;
    LitString* copied;
    (void)argc;
    (void)args;
    selfstr = lit_value_asstring(instance);
    copied = lit_string_clone(state, selfstr);
    lit_util_stringchangecase(copied->strbuf.data, copied->strbuf.length, tolower);
    return lit_value_fromobject(copied);
}

LitValue lit_objfnstring_contains(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr = lit_value_asstring(instance);
    LitString* sub = LIT_CHECK_GETSTRINGOBJECT(0);
    if(sub == selfstr)
    {
        return lit_value_makebool(true);
    }
    return lit_value_makebool(strstr(selfstr->strbuf.data, sub->strbuf.data) != NULL);
}

LitValue lit_objfnstring_startswith(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LitString* selfstr = lit_value_asstring(instance);
    LitString* sub = LIT_CHECK_GETSTRINGOBJECT(0);
    if(sub == selfstr)
    {
        return lit_value_makebool(true);
    }
    if(sub->strbuf.length > selfstr->strbuf.length)
    {
        return lit_value_makebool(false);
    }
    for(i = 0; i < sub->strbuf.length; i++)
    {
        if(sub->strbuf.data[i] != selfstr->strbuf.data[i])
        {
            return lit_value_makebool(false);
        }
    }
    return lit_value_makebool(true);
}

LitValue lit_objfnstring_endswith(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LitString* selfstr = lit_value_asstring(instance);
    LitString* sub = LIT_CHECK_GETSTRINGOBJECT(0);
    if(sub == selfstr)
    {
        return lit_value_makebool(true);
    }
    if(sub->strbuf.length > selfstr->strbuf.length)
    {
        return lit_value_makebool(false);
    }
    size_t start = selfstr->strbuf.length - sub->strbuf.length;
    for(i = 0; i < sub->strbuf.length; i++)
    {
        if(sub->strbuf.data[i] != selfstr->strbuf.data[i + start])
        {
            return lit_value_makebool(false);
        }
    }
    return lit_value_makebool(true);
}

LitValue lit_objfnstring_replace(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* with;
    LitString* what;
    LitString* clone;
    LitString* selfstr;
    LIT_ENSURE_ARGS(2);
    if(!lit_value_isstring(args[0]) || !lit_value_isstring(args[1]))
    {
        lit_vm_raisefatalerror(state, "expected 2 string arguments");
    }
    selfstr = lit_value_asstring(instance);
    what = lit_value_asstring(args[0]);
    with = lit_value_asstring(args[1]);
    clone = lit_string_makeemptystring(state, 0, false);
    lit_strbuf_fullreplace(&selfstr->strbuf, &clone->strbuf, what->strbuf.data, what->strbuf.length, with->strbuf.data, with->strbuf.length);
    return lit_value_fromobject(clone);
}

LitValue lit_objfnstring_splice(LitState* state, LitString* string, int from, int to)
{
    int length = lit_string_utflength(string);
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
        lit_vm_raisefatalerror(state, "string splice from bound is larger that to bound");
    }
    from = lit_util_stringutfucharoffset(string->strbuf.data, from);
    to = lit_util_stringutfucharoffset(string->strbuf.data, to);
    return lit_value_fromobject(lit_string_fromrange(state, string, from, to - from + 1));
}

LitValue lit_objfnstring_substring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);
    return lit_objfnstring_splice(state, lit_value_asstring(instance), from, to);
}

LitValue lit_objfnstring_indexof(LitState* state, LitValue thisval, size_t argc, LitValue* args)
{
    char findme;
    size_t i;
    LitValue vfind;
    LitString* tmp;
    LitString* selfstr;
    (void)state;
    (void)argc;
    selfstr = lit_value_asstring(thisval);
    findme = -1;
    vfind = args[0];
    if(lit_value_isnumber(vfind))
    {
        findme = lit_value_asnumber(vfind);
    }
    else if(lit_value_isstring(vfind))
    {
        tmp = lit_value_asstring(vfind);
        findme = tmp->strbuf.data[0];
    }
    else
    {
        return lit_value_makenumber(-1);
    }
    for(i=0; i<selfstr->strbuf.length; i++)
    {
        if(selfstr->strbuf.data[i] == findme)
        {
            return lit_value_makenumber(i);
        }
    }
    return lit_value_makenumber(-1);
}

LitValue lit_objfnstring_charcodeat(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int cp;
    int index;
    LitString* selfstr = lit_value_asstring(instance);
    index = lit_value_asnumber(args[0]);
    if(argc != 1)
    {
        lit_vm_raisefatalerror(state, "cannot modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = lit_string_utflength(selfstr) + index;
        if(index < 0)
        {
            return lit_value_makenull();
        }
    }
    cp = lit_string_codepointcodeat(state, selfstr, lit_util_stringutfucharoffset(selfstr->strbuf.data, index));
    return lit_value_makenumber(cp);
}

LitValue lit_objfnstring_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    if(lit_value_isrange(args[0]))
    {
        LitRange* range = lit_value_asrange(args[0]);
        return lit_objfnstring_splice(state, lit_value_asstring(instance), range->from, range->to);
    }
    LitString* selfstr = lit_value_asstring(instance);
    int index = lit_value_asnumber(args[0]);
    if(argc != 1)
    {
        lit_vm_raisefatalerror(state, "cannot modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = lit_string_utflength(selfstr) + index;
        if(index < 0)
        {
            return lit_value_makenull();
        }
    }
    LitString* c = lit_string_codepointstringat(state, selfstr, lit_util_stringutfucharoffset(selfstr->strbuf.data, index));
    return c == NULL ? lit_value_makenull() : lit_value_fromobject(c);
}

LitValue lit_objfnstring_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_string_utflength(lit_value_asstring(instance)));
}

LitValue lit_objfnstring_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr = lit_value_asstring(instance);
    if(lit_value_isnull(args[0]))
    {
        if(selfstr->strbuf.length == 0)
        {
            return lit_value_makenull();
        }
        return lit_value_makenumber(0);
    }
    int index = LIT_CHECK_NUMBER(0);
    if(index < 0)
    {
        return lit_value_makenull();
    }
    do
    {
        index++;
        if(index >= (int)selfstr->strbuf.length)
        {
            return lit_value_makenull();
        }
    } while((selfstr->strbuf.data[index] & 0xc0) == 0x80);
    return lit_value_makenumber(index);
}

LitValue lit_objfnstring_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr = lit_value_asstring(instance);
    uint32_t index = LIT_CHECK_NUMBER(0);
    if(index == UINT32_MAX)
    {
        return lit_value_makebool(false);
    }
    return lit_value_fromobject(lit_string_codepointstringat(state, selfstr, index));
}

/*
 * Function
 */

LitValue lit_objfnfunction_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_function_getname(state, instance);
}

LitValue lit_objfnfunction_name(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_function_getname(state, instance);
}

/*
 * Fiber
 */

LitValue lit_objfnfiber_constructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    LitValue arg;
    printf("Fiber.constructor:argc=%ld\n", argc);
    if((argc == 0) || (!lit_value_iscallablefunction(args[0])))
    {
        fprintf(stderr, "args[0]=");
        lit_value_printvalue(state->streamstderr, args[0], true);
        fprintf(stderr, "\n");
        lit_vm_raisefatalerror(state, "Fiber constructor expects a function as its argument");
    }
    arg = args[0];
    LitModule* module = state->vmstate.fiber->module;

    LitFiber* fiber;

    if(lit_value_isfuncscript(arg))
    {
        fiber = lit_object_makefiber(state, module, lit_value_asfuncscript(arg));
    }
    else
    {
        fiber = lit_object_makefiberclosure(state, module, lit_value_asfuncclosure(arg));
    }

    fiber->parent = state->vmstate.fiber;

    return lit_value_fromobject(fiber);
}

bool lit_coreutil_isfiberdone(LitFiber* fiber)
{
    return fiber->framecount == 0 || fiber->abort;
}

LitValue lit_objfnfiber_done(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makebool(lit_coreutil_isfiberdone(lit_value_asfiber(instance)));
}

LitValue lit_objfnfiber_error(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_asfiber(instance)->error;
}

LitValue lit_objfnfiber_current(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(state->vmstate.fiber);
}

void lit_coreutil_runfiber(LitState* state, LitFiber* fiber, LitValue* args, size_t argc, bool catcher)
{
    size_t i;
    size_t ai;
    if(lit_coreutil_isfiberdone(fiber))
    {
        lit_vm_raisefatalerror(state, "Fiber already finished executing");
    }
    fiber->parent = state->vmstate.fiber;
    fiber->catcher = catcher;
    state->vmstate.fiber = fiber;
    LitCallFrame* frame = &fiber->framevals[fiber->framecount - 1];
    if(frame->ip == frame->function->chunk.compiledcodechunk)
    {
        fiber->argcount = argc;
        LitFuncScript* function = frame->function;
        LitValue* start = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
        lit_fiber_ensureregisters(fiber, start - fiber->registeritems + function->maxregisters);
        frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
        for(i = argc + 1; i < function->maxregisters; i++)
        {
            frame->slots[i] = lit_value_makenull();
        }
        frame->slots[0] = lit_value_fromobject(function);
        for(ai = 0; ai < argc; ai++)
        {
            frame->slots[ai + 1] = args[ai];
        }
        bool vararg = frame->function->vararg;
        size_t functionargcount = function->argcount;
        fiber->argcount = functionargcount;
        if(vararg)
        {
            if(functionargcount == argc && lit_value_isvargarray(*(frame->slots + functionargcount)))
            {
                /* no need to repack the arguments */
            }
            else
            {
                LitArray* array = &lit_object_makevararray(state)->innerarray;
                lit_state_pushroot(state, (LitObject*)array);
                *(frame->slots + functionargcount) = lit_value_fromobject(array);
                size_t varargcount = argc - functionargcount + 1;
                if(varargcount > 0)
                {
                    lit_dynlistval_ensuresize(&array->innerlist, varargcount);
                    for(i = 0; i < varargcount; i++)
                    {
                        array->innerlist.listitems[i] = args[i + functionargcount - 1];
                    }
                }
                lit_state_poproot(state);
            }
        }
    }
    if(LIT_UNLIKELY(state->config.traceexecution))
    {
        fprintf(stderr, "fiber start:\n");
    }
}

LitValue lit_objfnfiber_run(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    lit_coreutil_runfiber(state, lit_value_asfiber(instance), args, argc, false);
    return lit_value_makenull();
}

LitValue lit_objfnfiber_try(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    lit_coreutil_runfiber(state, lit_value_asfiber(instance), args, argc, true);
    return lit_value_makenull();
}

LitValue lit_objfnfiber_yield(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    if(state->vmstate.fiber->parent == NULL)
    {
        lit_vm_handleerror(state, argc == 0 ? lit_string_copy(state, "Fiber was yielded") : lit_value_tostring(state, args[0], 0));
        return lit_value_makenull();
    }
    state->vmstate.fiber = state->vmstate.fiber->parent;
    *state->vmstate.fiber->returnaddress = argc == 0 ? lit_value_makenull() : lit_value_fromobject(lit_value_tostring(state, args[0], 0));
    return lit_value_makenull();
}

LitValue lit_objfnfiber_yeet(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    if(state->vmstate.fiber->parent == NULL)
    {
        lit_vm_handleerror(state, argc == 0 ? lit_string_copy(state, "Fiber was yeeted") : lit_value_tostring(state, args[0], 0));
        return lit_value_makenull();
    }
    state->vmstate.fiber = state->vmstate.fiber->parent;
    *state->vmstate.fiber->returnaddress = argc == 0 ? lit_value_makenull() : lit_value_fromobject(lit_value_tostring(state, args[0], 0));
    return lit_value_makenull();
}

LitValue lit_objfnfiber_abort(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    LitString* value = argc == 0 ? lit_string_copy(state, "Fiber was aborted") : lit_value_tostring(state, args[0], 0);
    lit_vm_handleerror(state, value);
    if(state->vmstate.fiber->returnaddress != NULL)
    {
        *state->vmstate.fiber->returnaddress = lit_value_fromobject(value);
    }
    return lit_value_makenull();
}

/*
 * Module
 */


LitValue lit_objfnmodule_privates(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitModule* module = lit_value_ismodule(instance) ? lit_value_asmodule(instance) : state->vmstate.fiber->module;
    LitMap* map = module->privatenames;
    return lit_value_fromobject(map);
}

LitValue lit_objfnmodule_current(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(state->vmstate.fiber->module);
}

LitValue lit_objfnmodule_toString(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_valformat(state, "Module @", lit_value_fromobject(lit_value_asmodule(instance)->name));
}

LitValue lit_objfnmodule_name(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return lit_value_fromobject(lit_value_asmodule(instance)->name);
}

/*
 * Array
 */

LitValue lit_objfnarray_constructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    size_t count;
    LitValue fill;
    LitArray* arr;
    (void)instance;
    (void)argc;
    (void)args;
    arr = lit_array_make(state);
    if(argc > 0)
    {
        count = lit_value_asnumber(args[0]);
        fill = lit_value_makenull();
        if(argc > 1)
        {
            fill = args[1];
        }
        for(i = 0; i < count; i++)
        {
            lit_array_push(arr, fill);
        }
    }
    return lit_value_fromobject(arr);
}

LitValue lit_objfnarray_splice(LitState* state, LitArray* array, int from, int to)
{
    size_t i;
    size_t length = array->innerlist.listcount;
    if(from < 0)
    {
        from = (int)length + from;
    }
    if(to < 0)
    {
        to = (int)length + to;
    }
    if(from > to)
    {
        lit_vm_raisefatalerror(state, "string splice from bound is larger that to bound");
    }
    from = fmax(from, 0);
    to = fmin(to, (int)length - 1);
    length = fmin(length, to - from + 1);
    LitArray* newarray = lit_array_make(state);
    for(i = 0; i < length; i++)
    {
        lit_dynlistval_push(&newarray->innerlist, array->innerlist.listitems[from + i]);
    }
    return lit_value_fromobject(newarray);
}

LitValue lit_objfnarray_slice(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);
    return lit_objfnarray_splice(state, lit_value_asarray(instance), from, to);
}

LitValue lit_objfnarray_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    if(argc == 2)
    {
        if(!lit_value_isnumber(args[0]))
        {
            lit_vm_raisefatalerror(state, "array index must be a number, got a %s instead", lit_value_valtypename(args[0]));
        }
        LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
        int64_t index = lit_value_asnumber(args[0]);
        if(index < 0)
        {
            index = fmax(0, list->listcount + index);
        }
        //fprintf(stderr, "Array[]: index=%ld\n", index);
        lit_dynlistval_ensuresize(list, index + 1);
        return list->listitems[index] = args[1];
    }
    if(!lit_value_isnumber(args[0]))
    {
        if(lit_value_isrange(args[0]))
        {
            LitRange* range = lit_value_asrange(args[0]);
            return lit_objfnarray_splice(state, lit_value_asarray(instance), (int)range->from, (int)range->to);
        }
        lit_vm_raisefatalerror(state, "array index must be a number, got a %s instead", lit_value_valtypename(args[0]));
        return lit_value_makenull();
    }
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    int index = lit_value_asnumber(args[0]);
    if(index < 0)
    {
        index = fmax(0, list->listcount + index);
    }
    if((size_t)index >= list->listcount)
    {
        return lit_value_makenull();
    }
    return list->listitems[index];
}

LitValue lit_objfnarray_push(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LitArray* self;
    (void)state;
    self = lit_value_asarray(instance);
    for(i = 0; i < argc; i++)
    {
        lit_dynlistval_push(&self->innerlist, args[i]);
    }
    return lit_value_makenull();
}

LitValue lit_objfnarray_insert(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int i;
    LIT_ENSURE_ARGS(2);
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    int index = LIT_CHECK_NUMBER(0);
    if(index < 0)
    {
        index = fmax(0, list->listcount + index);
    }
    LitValue value = args[1];
    if((int)list->listcount <= index)
    {
        lit_dynlistval_ensuresize(list, index + 1);
    }
    else
    {
        lit_dynlistval_ensuresize(list, list->listcount + 1);
        for(i = list->listcount - 1; i > index; i--)
        {
            list->listitems[i] = list->listitems[i - 1];
        }
    }
    list->listitems[index] = value;
    return lit_value_makenull();
}

LitValue lit_objfnarray_addall(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LIT_ENSURE_ARGS(1);
    if(!lit_value_isarray(args[0]))
    {
        lit_vm_raisefatalerror(state, "expected array as the argument");
    }
    LitArray* array = lit_value_asarray(instance);
    LitArray* toAdd = lit_value_asarray(args[0]);
    for(i = 0; i < toAdd->innerlist.listcount; i++)
    {
        lit_dynlistval_push(&array->innerlist, toAdd->innerlist.listitems[i]);
    }
    return lit_value_makenull();
}

int lit_coreutil_indexof(LitState* state, LitArray* array, LitValue value)
{
    size_t i;
    LitValue* ptr;
    for(i = 0; i < array->innerlist.listcount; i++)
    {
        ptr = &array->innerlist.listitems[i];
        if(lit_value_compare(state, *ptr, value))
        {
            return (int)i;
        }
    }
    return -1;
}

LitValue lit_objfnarray_indexof(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    int index = lit_coreutil_indexof(state, lit_value_asarray(instance), args[0]);
    return index == -1 ? lit_value_makenull() : lit_value_makenumber(index);
}

LitValue lit_coreutil_removeat(LitArray* array, size_t index)
{
    size_t i;
    LitDynListVal* list = &array->innerlist;
    size_t count = list->listcount;
    if(index >= count)
    {
        return lit_value_makenull();
    }
    LitValue value = list->listitems[index];
    if(index == count - 1)
    {
        list->listitems[index] = lit_value_makenull();
    }
    else
    {
        for(i = index; i < list->listcount - 1; i++)
        {
            list->listitems[i] = list->listitems[i + 1];
        }
        list->listitems[count - 1] = lit_value_makenull();
    }
    list->listcount--;
    return value;
}

LitValue lit_objfnarray_remove(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitArray* array = lit_value_asarray(instance);
    int index = lit_coreutil_indexof(state, array, args[0]);
    if(index != -1)
    {
        return lit_coreutil_removeat(array, (size_t)index);
    }
    return lit_value_makenull();
}

LitValue lit_objfnarray_pop(LitState* state, LitValue thisval, size_t argc, LitValue* args)
{
    int index;
    LitValue val;
    LitArray* ary;
    (void)state;
    (void)argc;
    (void)args;
    ary = lit_value_asarray(thisval);
    if(lit_array_count(ary) == 0)
    {
        return lit_value_makenull();
    }
    index = lit_array_count(ary) - 1;
    val = lit_array_get(ary, index);
    lit_array_removeat(ary, (size_t)index);
    return val;
}

LitValue lit_objfnarray_removeat(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int index = LIT_CHECK_NUMBER(0);
    if(index < 0)
    {
        return lit_value_makenull();
    }
    return lit_coreutil_removeat(lit_value_asarray(instance), (size_t)index);
}

LitValue lit_objfnarray_contains(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    return lit_value_makebool(lit_coreutil_indexof(state, lit_value_asarray(instance), args[0]) != -1);
}

LitValue lit_objfnarray_clear(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    lit_value_asarray(instance)->innerlist.listcount = 0;
    return lit_value_makenull();
}

LitValue lit_objfnarray_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitArray* array = lit_value_asarray(instance);
    int number = 0;
    if(lit_value_isnumber(args[0]))
    {
        number = lit_value_asnumber(args[0]);
        if(number >= (int)array->innerlist.listcount - 1)
        {
            return lit_value_makenull();
        }
        number++;
    }
    return array->innerlist.listcount == 0 ? lit_value_makenull() : lit_value_makenumber(number);
}

LitValue lit_objfnarray_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    if(list->listcount <= index)
    {
        return lit_value_makenull();
    }
    return list->listitems[index];
}

LitValue lit_objfnarray_foreach(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LIT_ENSURE_ARGS(1);
    LitValue callback = args[0];
    if(!lit_value_iscallablefunction(callback))
    {
        lit_vm_raisefatalerror(state, "expected a function as the callback");
    }
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    for(i = 0; i < list->listcount; i++)
    {
        lit_state_callvalue(state, callback, &list->listitems[i], 1);
    }
    return lit_value_makenull();
}

LitValue lit_objfnarray_join(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    bool havejoinee;
    size_t i;
    LitIOStream pr;
    LitString* res;
    LitValue joinee;
    LitDynListVal* list;
    (void)argc;
    (void)args;
    havejoinee = false;
    if(argc > 0)
    {
        joinee = args[0];
        havejoinee = true;
    }
    lit_iostream_makestackstring(&pr);
    list = &lit_value_asarray(instance)->innerlist;
    for(i = 0; i < list->listcount; i++)
    {
        lit_value_printvalue(&pr, list->listitems[i], false);
        if((i + 1) < list->listcount)
        {
            if(havejoinee)
            {
                lit_value_printvalue(&pr, joinee, false);
            }
        }
    }
    res = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(res);
}

bool lit_callback_sortcompare(LitState* state, LitValue a, LitValue b)
{
    LitValue args[2];
    if(lit_value_isnumber(a) && lit_value_isnumber(b))
    {
        return lit_value_asnumber(a) < lit_value_asnumber(b);
    }
    args[0] = b;
    return !lit_is_falsey(lit_state_findandcallmethod(state, a, lit_string_copy(state, "<"), args, 1).result);
}

void lit_coreutil_basicquicksort(LitState* state, LitValue* l, int length)
{
    if(length < 2)
    {
        return;
    }
    int pivotindex = length / 2;
    int i;
    int j;
    LitValue pivot = l[pivotindex];
    for(i = 0, j = length - 1;; i++, j--)
    {
        while(i < pivotindex && lit_callback_sortcompare(state, l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && lit_callback_sortcompare(state, pivot, l[j]))
        {
            j--;
        }
        if(i >= j)
        {
            break;
        }
        LitValue tmp = l[i];
        l[i] = l[j];
        l[j] = tmp;
    }
    lit_coreutil_basicquicksort(state, l, i);
    lit_coreutil_basicquicksort(state, l + i, length - i);
}

bool lit_coreutil_inlinesortcompare(LitState* state, LitValue callee, LitValue a, LitValue b)
{
    LitResult r;
    LitValue args[3];
    args[0] = a;
    args[1] = b;
    r = lit_state_callvalue(state, callee, args, 2);
    return !lit_is_falsey(r.result);
}

void lit_coreutil_customquicksort(LitState* state, LitValue* l, int length, LitValue callee)
{
    if(length < 2)
    {
        return;
    }
    int pivotindex = length / 2;
    int i;
    int j;
    LitValue pivot = l[pivotindex];

    for(i = 0, j = length - 1;; i++, j--)
    {
        while(i < pivotindex && lit_coreutil_inlinesortcompare(state, callee, l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && lit_coreutil_inlinesortcompare(state, callee, pivot, l[j]))
        {
            j--;
        }
        if(i >= j)
        {
            break;
        }
        LitValue tmp = l[i];
        l[i] = l[j];
        l[j] = tmp;
    }
    lit_coreutil_customquicksort(state, l, i, callee);
    lit_coreutil_customquicksort(state, l + i, length - i, callee);
}

LitValue lit_objfnarray_sort(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    if(argc == 1 && lit_value_iscallablefunction(args[0]))
    {
        lit_coreutil_customquicksort(state, list->listitems, list->listcount, args[0]);
    }
    else
    {
        lit_coreutil_basicquicksort(state, list->listitems, list->listcount);
    }
    return instance;
}

LitValue lit_objfnarray_clone(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    (void)argc;
    (void)args;
    LitDynListVal* list = &lit_value_asarray(instance)->innerlist;
    LitArray* array = lit_array_make(state);
    LitDynListVal* newvalues = &array->innerlist;
    lit_dynlistval_ensuresize(newvalues, list->listcount);
    /* lit_dynlistval_ensuresize sets the count to max of previous count (0 in this case) and new count, so we have to reset it */
    newvalues->listcount = 0;
    for(i = 0; i < list->listcount; i++)
    {
        lit_dynlistval_push(newvalues, list->listitems[i]);
    }
    return lit_value_fromobject(array);
}

LitValue lit_objfnarray_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitIOStream pr;
    LitString* dest;
    LitArray* self;
    (void)argc;
    (void)args;
    self = lit_value_asarray(instance);
    lit_iostream_makestackstring(&pr);
    lit_value_printobjarray(&pr, self);
    dest = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(dest);
}

LitValue lit_objfnarray_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return lit_value_makenumber(lit_value_asarray(instance)->innerlist.listcount);
}

/*
 * Map
 */

LitValue lit_objfnmap_constructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitInstance* inst;
    LitTable* pfields;
    (void)instance;
    (void)argc;
    (void)args;
    pfields = NULL;
    if(argc > 0)
    {
        if(lit_value_isinstance(args[0]))
        {
            inst = lit_value_asinstance(args[0]);
            pfields = &inst->fields;
        }
    }
    return lit_value_fromobject(lit_object_makemap(state, pfields));
}

LitValue lit_objfnmap_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    if(!lit_value_isstring(args[0]))
    {
        lit_vm_raisefatalerror(state, "map index must be a string");
    }
    LitMap* map = lit_value_asmap(instance);
    LitString* index = lit_value_asstring(args[0]);
    if(argc == 2)
    {
        LitValue val = args[1];
        lit_map_set(map, index, val);
        return val;
    }
    LitValue value;
    if(!lit_map_get(map, index, &value))
    {
        return lit_value_makenull();
    }
    return value;
}

LitValue lit_objfnmap_addall(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    if(!lit_value_ismap(args[0]))
    {
        lit_vm_raisefatalerror(state, "expected map as the argument");
    }
    lit_map_addall(lit_value_asmap(args[0]), lit_value_asmap(instance));
    return lit_value_makenull();
}

LitValue lit_objfnmap_clear(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitMap* map;
    (void)state;
    (void)argc;
    (void)args;
    map = lit_value_asmap(instance);
    map->innertable.htcount = 0;
    return lit_value_makenull();
}

LitValue lit_objfnmap_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int value = lit_coreutil_tableiterator(&lit_value_asmap(instance)->innertable, index);
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(value);
}

LitValue lit_objfnmap_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    return lit_coreutil_tableiterkey(&lit_value_asmap(instance)->innertable, index);
}

LitValue lit_objfnmap_foreach(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int i;
    LitValue callargs[3];
    LIT_ENSURE_ARGS(1);
    LitValue callback = args[0];
    if(!lit_value_iscallablefunction(callback))
    {
        lit_vm_raisefatalerror(state, "expected a function as the callback");
    }
    LitTable* tab = &lit_value_asmap(instance)->innertable;
    for(i = 0; i < tab->htcapacity; i++)
    {
        LitTabEntry* entry = &tab->htentries[i];
        if(entry->entkey != NULL)
        {
            callargs[0] = lit_value_fromobject(entry->entkey);
            callargs[1] = entry->entvalue;
            lit_state_callvalue(state, callback, callargs, 2);
        }
    }
    return lit_value_makenull();
}

LitValue lit_objfnmap_clone(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitMap* map = lit_object_makemap(state, NULL);
    lit_table_addall(&lit_value_asmap(instance)->innertable, &map->innertable);
    return lit_value_fromobject(map);
}

LitValue lit_objfnmap_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitIOStream pr;
    LitMap* self;
    LitString* dest;
    (void)argc;
    (void)args;
    self = lit_value_asmap(instance);
    lit_iostream_makestackstring(&pr);
    lit_value_printobjtable(&pr, (LitObject*)self, &self->innertable);
    dest = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(dest);
}

LitValue lit_objfnmap_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_value_asmap(instance)->innertable.htcount);
}

/*
 * Range
 */

LitValue lit_objfnrange_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitRange* range = lit_value_asrange(instance);
    int number = range->from;
    if(lit_value_isnumber(args[0]))
    {
        number = lit_value_asnumber(args[0]);
        if(range->to > range->from ? number >= range->to : number <= range->to)
        {
            return lit_value_makenull();
        }
        number += (range->from - range->to) > 0 ? -1 : 1;
    }
    return lit_value_makenumber(number);
}

LitValue lit_objfnrange_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    LIT_ENSURE_ARGS(1);
    return args[0];
}

LitValue lit_objfnrange_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitRange* range = lit_value_asrange(instance);
    return lit_string_valformat(state, "Range(#, #)", range->from, range->to);
}

LitValue lit_objfnrange_from(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_value_asrange(instance)->from);
}

LitValue lit_objfnrange_setfrom(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    lit_value_asrange(instance)->from = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue lit_objfnrange_to(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_value_asrange(instance)->to);
}

LitValue lit_objfnrange_setto(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    lit_value_asrange(instance)->to = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue lit_objfnrange_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    LitRange* range = lit_value_asrange(instance);
    return lit_value_makenumber(range->to - range->from);
}

/*
 * Natives
 */

LitValue lit_cfn_srand(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    srand(lit_value_asnumber(args[0]));
    return lit_value_makenull();
}

LitValue lit_cfn_random(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    uint64_t iv;
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    iv = random();
    return lit_value_makenumber(iv);
}

LitValue lit_cfn_time(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber((double)clock() / CLOCKS_PER_SEC);
}

LitValue lit_cfn_systemtime(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(time(NULL));
}

LitValue lit_cfn_printvalues(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    (void)instance;
    if(argc == 0)
    {
        return lit_value_makenull();
    }
    for(i = 0; i < argc; i++)
    {
        lit_value_printvalue(state->streamstdout, args[i], false);
    }
    return lit_value_makenull();
}

LitValue lit_cfn_printchar(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    char ch;
    size_t i;
    (void)instance;
    if(argc == 0)
    {
        return lit_value_makenull();
    }
    for(i = 0; i < argc; i++)
    {
        ch = lit_value_asnumber(args[i]);
        lit_iostream_putlen(state->streamstdout, &ch, 1);
    }
    return lit_value_makenull();
}

LitValue lit_cfn_println(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitValue r;
    (void)instance;
    r = lit_cfn_printvalues(state, instance, argc, args);
    fprintf(stdout, "\n");
    return r;
}

LitValue lit_cfn_openlibrary(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    const char* name = LIT_CHECK_GETSTRINGDATA(0);
    /*
    if(strcmp(name, "network") == 0)
    {
        lit_open_network_library(state);
    }
    else
    */
    {
        lit_vm_raisefatalerror(state, "unknown built-in library %s", name);
    }
    return lit_value_makenull();
}

bool lit_evalutil_interpmodule(LitState* state, LitModule* module)
{
    LitFiber* fiber;
    LitFuncScript* function;
    function = module->mainfunction;
    fiber = lit_object_makefiber(state, module, function);
    fiber->parent = state->vmstate.fiber;
    state->vmstate.fiber = fiber;
    return true;
}

bool lit_evalutil_compileandrun(LitState* state, LitString* modname, char* source)
{
    LitModule* module;
    module = lit_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        return false;
    }
    module->ran = true;
    return lit_evalutil_interpmodule(state, module);
}

LitValue lit_cfn_eval(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    char* code = (char*)LIT_CHECK_GETSTRINGDATA(0);
    LitFiber* fiber = state->vmstate.fiber;
    if(lit_evalutil_compileandrun(state, fiber->module->name, code))
    {
        fiber->returnaddress = args - 1;
    }
    return lit_value_makenull();
}

void lit_state_opencorelibrary(LitState* state)
{
    LitClass* klass;
    lit_vmexec_pushgc(state, false);
    {
        klass = lit_class_make(state, "Class", NULL);
        lit_class_bindmethod(klass, "toString", lit_objfnclass_tostring);
        lit_class_bindmethod(klass, "[]", lit_objfnclass_subscript);
        lit_class_bindstaticmethod(klass, "toString", lit_objfnclass_tostring);
        lit_class_bindstaticmethod(klass, "iterator", lit_objfnclass_iterator);
        lit_class_bindstaticmethod(klass, "iteratorValue", lit_objfnclass_itervalue);
        lit_class_bindgetsetter(klass, "super", lit_objfnclass_super, NULL);
        lit_class_bindstaticgetter(klass, "super", lit_objfnclass_super);
        lit_class_bindstaticgetter(klass, "name", lit_objfnclass_name);
        state->stdclassclass = klass;
        lit_state_setglobal(state, klass->name, lit_value_fromobject(klass));
    }
    {
        klass = lit_class_make(state, "Object", NULL);
        lit_class_inherit(klass, state->stdclassclass);
        lit_class_bindstaticmethod(klass, "keys", lit_objfnobject_keys);
        lit_class_bindmethod(klass, "toString", lit_objfnobject_tostring);
        lit_class_bindmethod(klass, "dump", lit_objfnobject_dump);
        lit_class_bindmethod(klass, "isCallable", lit_objfnobject_iscallable);
        lit_class_bindmethod(klass, "[]", lit_objfnobject_subscript);
        lit_class_bindmethod(klass, "iterator", lit_objfnobject_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnobject_itervalue);
        lit_class_bindgetsetter(klass, "class", lit_objfnobject_class, NULL);
        state->stdobjectclass = klass;
        state->stdobjectclass->super = state->stdclassclass;
    }
    {
        klass = lit_class_make(state, "Number", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindgetsetter(klass, "chr", lit_objfnnumber_chr, NULL);
        lit_class_bindmethod(klass, "toString", lit_objfnnumber_tostring);
        state->number_class = klass;
    }
    {
        klass = lit_class_make(state, "String", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindstaticmethod(klass, "chr", lit_objfnstring_chr);
        lit_class_bindstaticmethod(klass, "fromCharCode", lit_objfnstring_chr);
        lit_class_bindmethod(klass, "+", lit_objfnstring_plus);
        /*
        lit_class_bindmethod(klass, "<", lit_objfnstring_less);
        lit_class_bindmethod(klass, ">", lit_objfnstring_greater);
        */
        lit_class_bindmethod(klass, "toString", lit_objfnstring_tostring);
        lit_class_bindmethod(klass, "toNumber", lit_objfnstring_tonumber);
        lit_class_bindmethod(klass, "toUpperCase", lit_objfnstring_touppercase);
        lit_class_bindmethod(klass, "toLowerCase", lit_objfnstring_tolowercase);
        lit_class_bindmethod(klass, "contains", lit_objfnstring_contains);
        lit_class_bindmethod(klass, "startsWith", lit_objfnstring_startswith);
        lit_class_bindmethod(klass, "endsWith", lit_objfnstring_endswith);
        lit_class_bindmethod(klass, "replace", lit_objfnstring_replace);
        lit_class_bindmethod(klass, "substring", lit_objfnstring_substring);
        lit_class_bindmethod(klass, "substr", lit_objfnstring_substring);
        lit_class_bindmethod(klass, "indexOf", lit_objfnstring_indexof);
        lit_class_bindmethod(klass, "iterator", lit_objfnstring_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnstring_itervalue);
        lit_class_bindmethod(klass, "[]", lit_objfnstring_subscript);
        lit_class_bindmethod(klass, "charAt", lit_objfnstring_subscript);
        lit_class_bindmethod(klass, "charCodeAt", lit_objfnstring_charcodeat);
        lit_class_bindmethod(klass, "size", lit_objfnstring_length);
        lit_class_bindgetsetter(klass, "length", lit_objfnstring_length, NULL);
        state->string_class = klass;
    }
    {
        klass = lit_class_make(state, "Bool", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "toString", lit_objfnbool_tostring);
        state->bool_class = klass;
    }
    {
        klass = lit_class_make(state, "Function", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "toString", lit_objfnfunction_tostring);
        lit_class_bindgetsetter(klass, "name", lit_objfnfunction_name, NULL);
        state->function_class = klass;
    }
    {
        klass = lit_class_make(state, "Fiber", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfnfiber_constructor);
        lit_class_bindmethod(klass, "run", lit_objfnfiber_run);
        lit_class_bindmethod(klass, "try", lit_objfnfiber_try);
        lit_class_bindgetsetter(klass, "done", lit_objfnfiber_done, NULL);
        lit_class_bindgetsetter(klass, "error", lit_objfnfiber_error, NULL);
        lit_class_bindstaticmethod(klass, "yield", lit_objfnfiber_yield);
        lit_class_bindstaticmethod(klass, "yeet", lit_objfnfiber_yeet);
        lit_class_bindstaticmethod(klass, "abort", lit_objfnfiber_abort);
        lit_class_bindstaticgetter(klass, "current", lit_objfnfiber_current);
        state->fiber_class = klass;
    }
    {
        klass = lit_class_make(state, "Module", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_setstaticfield(klass, "loaded", lit_value_fromobject(state->vmstate.modules));
        lit_class_bindstaticgetter(klass, "privates", lit_objfnmodule_privates);
        lit_class_bindstaticgetter(klass, "current", lit_objfnmodule_current);
        lit_class_bindmethod(klass, "toString", lit_objfnmodule_toString);
        lit_class_bindgetsetter(klass, "name", lit_objfnmodule_name, NULL);
        lit_class_bindgetsetter(klass, "privates", lit_objfnmodule_privates, NULL);
        state->module_class = klass;
    }
    {
        klass = lit_class_make(state, "Array", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfnarray_constructor);
        lit_class_bindmethod(klass, "[]", lit_objfnarray_subscript);
        lit_class_bindmethod(klass, "add", lit_objfnarray_push);
        lit_class_bindmethod(klass, "push", lit_objfnarray_push);
        lit_class_bindmethod(klass, "insert", lit_objfnarray_insert);
        lit_class_bindmethod(klass, "slice", lit_objfnarray_slice);
        lit_class_bindmethod(klass, "addAll", lit_objfnarray_addall);
        lit_class_bindmethod(klass, "pop", lit_objfnarray_pop);
        lit_class_bindmethod(klass, "remove", lit_objfnarray_remove);
        lit_class_bindmethod(klass, "removeAt", lit_objfnarray_removeat);
        lit_class_bindmethod(klass, "indexOf", lit_objfnarray_indexof);
        lit_class_bindmethod(klass, "contains", lit_objfnarray_contains);
        lit_class_bindmethod(klass, "clear", lit_objfnarray_clear);
        lit_class_bindmethod(klass, "iterator", lit_objfnarray_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnarray_itervalue);
        lit_class_bindmethod(klass, "forEach", lit_objfnarray_foreach);
        lit_class_bindmethod(klass, "join", lit_objfnarray_join);
        lit_class_bindmethod(klass, "sort", lit_objfnarray_sort);
        lit_class_bindmethod(klass, "clone", lit_objfnarray_clone);
        lit_class_bindmethod(klass, "toString", lit_objfnarray_tostring);
        lit_class_bindgetsetter(klass, "length", lit_objfnarray_length, NULL);
        state->array_class = klass;
    }
    {
        klass = lit_class_make(state, "Map", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfnmap_constructor);
        lit_class_bindmethod(klass, "[]", lit_objfnmap_subscript);
        lit_class_bindmethod(klass, "addAll", lit_objfnmap_addall);
        lit_class_bindmethod(klass, "clear", lit_objfnmap_clear);
        lit_class_bindmethod(klass, "iterator", lit_objfnmap_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnmap_itervalue);
        lit_class_bindmethod(klass, "forEach", lit_objfnmap_foreach);
        lit_class_bindmethod(klass, "clone", lit_objfnmap_clone);
        lit_class_bindmethod(klass, "toString", lit_objfnmap_tostring);
        lit_class_bindgetsetter(klass, "length", lit_objfnmap_length, NULL);
        state->map_class = klass;
    }
    {
        klass = lit_class_make(state, "Range", state->stdobjectclass);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "iterator", lit_objfnrange_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnrange_itervalue);
        lit_class_bindmethod(klass, "toString", lit_objfnrange_tostring);
        lit_class_bindgetsetter(klass, "from", lit_objfnrange_from, lit_objfnrange_setfrom);
        lit_class_bindgetsetter(klass, "to", lit_objfnrange_to, lit_objfnrange_setto);
        lit_class_bindgetsetter(klass, "length", lit_objfnrange_length, NULL);
        state->range_class = klass;
    }
    lit_vmexec_popgc(state);
    lit_state_defnative(state, "srand", lit_cfn_srand);
    lit_state_defnative(state, "random", lit_cfn_random);
    lit_state_defnative(state, "time", lit_cfn_time);
    lit_state_defnative(state, "systemTime", lit_cfn_systemtime);
    lit_state_defnative(state, "print", lit_cfn_printvalues);
    lit_state_defnative(state, "printchar", lit_cfn_printchar);
    lit_state_defnative(state, "println", lit_cfn_println);
    lit_state_defnative(state, "openLibrary", lit_cfn_openlibrary);
    lit_state_defnative(state, "eval", lit_cfn_eval);
    lit_state_setglobal(state, lit_string_copy(state, "GLOBALS"), lit_value_fromobject(state->vmstate.globals));
}



void lit_bcemu_initfile(LitEmulatedFile* file, const char* source)
{
    file->source = source;
    file->position = 0;
}

void lit_bcemu_readuint8(LitEmulatedFile* file, uint8_t* dest)
{
    *dest = file->source[file->position++];
}


void lit_bcemu_readuint16(LitEmulatedFile* file, uint16_t* dest)
{
    uint8_t v1;
    uint8_t v2;
    lit_bcemu_readuint8(file, &v1);
    lit_bcemu_readuint8(file, &v2);
    *dest = (uint16_t)(v1 | (v2 << 8u));
}

void lit_bcemu_readuint32(LitEmulatedFile* file, uint32_t* dest)
{
    uint8_t v1;
    uint8_t v2;
    uint8_t v3;
    uint8_t v4;
    lit_bcemu_readuint8(file, &v1);
    lit_bcemu_readuint8(file, &v2);
    lit_bcemu_readuint8(file, &v3);
    lit_bcemu_readuint8(file, &v4);
    *dest = (uint32_t)(v1 | (v2 << 8u) | (v3 << 16u) | (v4 << 24u));
}

void lit_bcemu_readuint64(LitEmulatedFile* file, uint64_t* dest)
{
    uint32_t v1;
    uint32_t v2;
    lit_bcemu_readuint32(file, &v1);
    lit_bcemu_readuint32(file, &v2);
    *dest = (uint64_t)(v1 | ((uint64_t)v2 << 32u));
}

void lit_bcemu_readdouble(LitEmulatedFile* file, double* dest)
{
    size_t i;
    uint8_t buf[8];
    double result;
    for(i = 0; i < 8; i++)
    {
        lit_bcemu_readuint8(file, &buf[i]);
    }
    memcpy(&result, buf, 8);
    *dest = result;
}

void lit_bcfile_writeuint8(FILE* file, uint8_t byte)
{
    fwrite(&byte, sizeof(uint8_t), 1, file);
}

void lit_bcfile_writeuint16(FILE* file, uint16_t byte)
{
    size_t rsz;
    (void)rsz;
    rsz = fwrite(&byte, sizeof(uint16_t), 1, file);
    assert(rsz == 1);
}

void lit_bcfile_writeuint32(FILE* file, uint32_t byte)
{
    size_t rsz;
    (void)rsz;
    rsz = fwrite(&byte, sizeof(uint32_t), 1, file);
    assert(rsz == 1);
}

void lit_bcfile_writeuint64(FILE* file, uint64_t byte)
{
    size_t rsz;
    (void)rsz;
    rsz = fwrite(&byte, sizeof(uint64_t), 1, file);
    assert(rsz == 1);
}

void lit_bcfile_writedouble(FILE* file, double byte)
{
    size_t rsz;
    (void)rsz;
    rsz = fwrite(&byte, sizeof(double), 1, file);
    assert(rsz == 1);
}

void lit_bcfile_writestring(FILE* file, LitString* string)
{
    uint8_t wch;
    uint8_t rch;
    uint32_t i;
    lit_bcfile_writeuint32(file, string->strbuf.length);
    for(i = 0; i < string->strbuf.length; i++)
    {
        rch = (uint8_t)string->strbuf.data[i];
        #if 1
            wch = rch ^ LIT_CONFIG_BCSTRINGKEY;
        #else
            wch = rch;
        #endif
        lit_bcfile_writeuint8(file, wch);
    }
}


LitString* lit_bcemu_readstring(LitState* state, LitEmulatedFile* file)
{
    LitString* res;
    uint8_t tmp;
    uint32_t i;
    LitStrBuffer dummy;
    lit_bcemu_readuint32(file, &dummy.length);
    fprintf(stderr, "readstring: length=%d\n", dummy.length);
    if(dummy.length < 1)
    {
        return NULL;
    }
    res = lit_string_makeemptystring(state, dummy.length, false);
    for(i = 0; i < dummy.length; i++)
    {
        lit_bcemu_readuint8(file, &tmp);
        res->strbuf.data[i] = (char)tmp ^ LIT_CONFIG_BCSTRINGKEY;
    }
    res->strbuf.length = dummy.length;
    return res;
}

/*
    LitObject innerobject;
    LitChunk chunk;
    LitString* name;
    size_t argcount;
    size_t upvaluecount;
    uint64_t maxregisters;
    bool vararg;
    LitModule* module;
*/
void lit_bcfile_savefunction(FILE* file, LitFuncScript* function)
{
    lit_bcfile_savechunk(file, &function->chunk);
    lit_bcfile_writestring(file, function->name);
    lit_bcfile_writeuint32(file, function->argcount);
    lit_bcfile_writeuint32(file, function->upvaluecount);
    lit_bcfile_writeuint8(file, (uint8_t)function->vararg);
    lit_bcfile_writeuint64(file, function->maxregisters);
}

LitFuncScript* lit_bcemu_loadfunction(LitState* state, LitEmulatedFile* file, LitModule* module)
{
    uint8_t tmp;
    LitFuncScript* function;
    function = lit_object_makefunction(state, module);
    lit_bcemu_loadchunk(state, file, module, &function->chunk);
    function->name = lit_bcemu_readstring(state, file);
    lit_bcemu_readuint32(file, &function->argcount);
    lit_bcemu_readuint32(file, &function->upvaluecount);
    lit_bcemu_readuint8(file, &tmp);
    function->vararg = tmp;
    lit_bcemu_readuint64(file, &function->maxregisters);
    return function;
}

/*
    size_t compiledcodecount;
    size_t capacity;
    uint64_t* compiledcodechunk;
    bool haslineinfo;
    size_t linecount;
    size_t linecapacity;
    uint16_t* lines;
    LitDynListVal constantlist;
*/
void lit_bcfile_savechunk(FILE* file, LitChunk* chunk)
{
    size_t i;
    lit_bcfile_writeuint64(file, chunk->compiledcodecount);
    for(i = 0; i < chunk->compiledcodecount; i++)
    {
        lit_bcfile_writeuint64(file, chunk->compiledcodechunk[i]);
    }
    if(chunk->haslineinfo)
    {
        size_t c = chunk->linecount * 2 + 2;
        lit_bcfile_writeuint64(file, c);
        for(i = 0; i < c; i++)
        {
            lit_bcfile_writeuint64(file, chunk->lines[i]);
        }
    }
    else
    {
        lit_bcfile_writeuint64(file, 0);
    }
    lit_bcfile_writeuint64(file, chunk->constantlist.listcount);
    for(i = 0; i < chunk->constantlist.listcount; i++)
    {
        LitValue constant = chunk->constantlist.listitems[i];
        if(lit_value_isobject(constant))
        {
            LitObjType type = lit_value_asobject(constant)->type;
            lit_bcfile_writeuint64(file, (uint8_t)(type + 1));
            switch(type)
            {
                case LIT_OBJ_STRING:
                {
                    lit_bcfile_writestring(file, lit_value_asstring(constant));
                    break;
                }
                case LIT_OBJ_FUNCSCRIPT:
                {
                    lit_bcfile_savefunction(file, lit_value_asfuncscript(constant));
                    break;
                }
                default:
                {
                    fprintf(stderr, "ERROR: serializing type %d (%s) not implemented!\n", type, lit_value_objtypename(type));
                    UNREACHABLE
                    break;
                }
            }
        }
        else
        {
            lit_bcfile_writeuint8(file, 0);
            lit_bcfile_writedouble(file, lit_value_asnumber(constant));
        }
    }
}

void lit_bcemu_loadchunk(LitState* state, LitEmulatedFile* file, LitModule* module, LitChunk* chunk)
{
    size_t i;
    size_t count;
    uint64_t type;
    double dtmp;
    lit_chunk_init(chunk);
    lit_bcemu_readuint64(file, &count);
    chunk->compiledcodechunk = (uint64_t*)lit_sysmem_malloc(sizeof(uint64_t) * count);
    chunk->compiledcodecount = count;
    chunk->capacity = count;
    for(i = 0; i < count; i++)
    {
        lit_bcemu_readuint64(file, &chunk->compiledcodechunk[i]);
    }
    lit_bcemu_readuint64(file, &count);
    if(count > 0)
    {
        chunk->lines = (uint16_t*)lit_sysmem_malloc(sizeof(uint16_t) * count);
        chunk->linecount = count;
        chunk->linecapacity = count;
        for(i = 0; i < count; i++)
        {
            lit_bcemu_readuint16(file, &chunk->lines[i]);
        }
    }
    else
    {
        chunk->haslineinfo = false;
    }
    lit_bcemu_readuint64(file, &count);
    chunk->constantlist.listitems = (LitValue*)lit_sysmem_malloc(sizeof(LitValue) * count);
    chunk->constantlist.listcount = count;
    chunk->constantlist.listcapacity = count;
    for(i = 0; i < count; i++)
    {
        lit_bcemu_readuint64(file, &type);
        if(type == 0)
        {
            lit_bcemu_readdouble(file, &dtmp);
            chunk->constantlist.listitems[i] = lit_value_makenumber(dtmp);
        }
        else
        {
            switch((LitObjType)(type - 1))
            {
                case LIT_OBJ_STRING:
                {
                    chunk->constantlist.listitems[i] = lit_value_fromobject(lit_bcemu_readstring(state, file));
                    break;
                }
                case LIT_OBJ_FUNCSCRIPT:
                {
                    chunk->constantlist.listitems[i] = lit_value_fromobject(lit_bcemu_loadfunction(state, file, module));
                    break;
                }
                default:
                {
                    UNREACHABLE
                    break;
                }
            }
        }
    }
}

void lit_bcfile_savemodule(LitModule* module, FILE* file)
{
    size_t i;
    bool disabled;
    disabled = false;
    lit_bcfile_writestring(file, module->name);
    lit_bcfile_writeuint64(file, module->privatecount);
    lit_bcfile_writeuint8(file, (uint8_t)disabled);
    if(!disabled)
    {
        LitTable* privates = &module->privatenames->innertable;
        for(i = 0; i < module->privatecount; i++)
        {
            if(privates->htentries[i].entkey != NULL)
            {
                lit_bcfile_writestring(file, privates->htentries[i].entkey);
                lit_bcfile_writeuint64(file, (uint16_t)lit_value_asnumber(privates->htentries[i].entvalue));
            }
        }
    }
    lit_bcfile_savefunction(file, module->mainfunction);
}

LitModule* lit_bcemu_initloadmodule(LitState* state, const char* input)
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
    LitModule* module;
    LitTable* privates;
    LitEmulatedFile file;
    lit_bcemu_initfile(&file, input);
    lit_bcemu_readuint32(&file, &utmp);
    if(utmp != LIT_CONFIG_BCMAGICNUMBER)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown magic number");
        return NULL;
    }
    lit_bcemu_readuint32(&file, &bytecodeversion);
    if(bytecodeversion > LIT_BYTECODE_VERSION)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown bytecode version '%i'", (int)bytecodeversion);
        return NULL;
    }
    lit_bcemu_readuint32(&file, &modulecount);
    LitModule* first = NULL;
    for(j = 0; j < modulecount; j++)
    {
        module = lit_object_makemodule(state, lit_bcemu_readstring(state, &file));
        privates = &module->privatenames->innertable;
        lit_bcemu_readuint32(&file, &privatescount);
        lit_bcemu_readuint8(&file, &tmp);
        enabled = !((bool)tmp);
        module->privatevalues = (LitValue*)lit_sysmem_malloc(privatescount * sizeof(LitValue));
        module->privatecount = privatescount;
        for(i = 0; i < privatescount; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
            if(enabled)
            {
                LitString* name = lit_bcemu_readstring(state, &file);
                lit_bcemu_readuint64(&file, &tmp64);
                lit_table_set(privates, name, lit_value_makenumber(tmp64));
            }
        }
        module->mainfunction = lit_bcemu_loadfunction(state, &file, module);
        lit_map_set(state->vmstate.modules, module->name, lit_value_fromobject(module));
        if(j == 0)
        {
            first = module;
        }
    }
#if 1
    lit_bcemu_readuint32(&file, &utmp);
    if(utmp != LIT_CONFIG_BCENDNUMBER)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown end number");
        return NULL;
    }
#endif
    return first;
}

void lit_callback_onfilecleanup(LitState* state, LitUserdata* data, bool mark)
{
    (void)state;
    if(mark)
    {
        return;
    }
    LitFileData* filedata = ((LitFileData*)data->data);
    if(filedata->file != NULL)
    {
        fclose(filedata->file);
        filedata->file = NULL;
    }
}

LitValue lit_objfnfile_constructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    const char* path = LIT_CHECK_GETSTRINGDATA(0);
    const char* mode = LIT_CHECK_GETSTRINGDATAOR(1, "rw");
    FILE* file = fopen(path, mode);
    if(file == NULL)
    {
        lit_vm_raisefatalerror(state, "failed to open file %s with mode %s (C error: %s)", path, mode, strerror(errno));
    }
    LitFileData* data = (LitFileData*)lit_userdata_insertdata(state, instance, sizeof(LitFileData), lit_callback_onfilecleanup);
    data->path = (char*)path;
    data->file = file;
    return instance;
}

LitValue lit_objfnfile_close(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    LitFileData* data = (LitFileData*)lit_userdata_extractdata(instance);
    fclose(data->file);
    data->file = NULL;
    return lit_value_makenull();
}

LitValue lit_objfnfile_exists(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    char* fname;
    (void)instance;
    fname = NULL;
    fname = (char*)LIT_CHECK_GETSTRINGDATA(0);
    return lit_value_makebool(lit_util_fileexists(fname));
}

LitValue lit_objfnfile_create(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    const char* path = LIT_CHECK_GETSTRINGDATA(0);
    FILE* file = fopen(path, "w");
    if(file == NULL)
    {
        lit_vm_raisefatalerror(state, "failed to create file %s", path);
    }
    fclose(file);
    return lit_value_makenull();
}

/*
 * ==
 * File writing
 */

LitValue lit_objfnfile_writevalvalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LitIOStream pr;
    LitFileData* lfd;
    (void)state;
    lfd = (LitFileData*)lit_userdata_extractdata(instance);
    lit_iostream_makestackio(&pr, lfd->file, false);
    for(i = 0; i < argc; i++)
    {
        lit_value_printvalue(&pr, args[i], false);
    }
    return lit_value_makenull();
}

LitValue lit_objfnfile_writevalstring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t wr;
    size_t maxlen;
    LitString* string;
    string = LIT_CHECK_GETSTRINGOBJECT(0);
    if(string == NULL)
    {
        return lit_value_makenull();
    }
    string = lit_value_asstring(args[0]);
    maxlen = lit_string_getlength(string);
    if(argc > 1)
    {
        maxlen = LIT_CHECK_NUMBER(1);
    }
    LitFileData* data = (LitFileData*)lit_userdata_extractdata(instance);
    wr = fwrite(lit_string_getdata(string), sizeof(char), maxlen, data->file);
    return lit_value_makenumber(wr);
}

/*
 * ==
 * File reading
 */

LitString* lit_util_readhandletostring(LitState* state, FILE* hnd, bool havesizeparam, size_t howmuch)
{
    size_t rsz;
    size_t length;
    LitString* result;
    if(havesizeparam)
    {
        length = howmuch;
    }
    else
    {
        fseek(hnd, 0, SEEK_END);
        length = ftell(hnd);
        fseek(hnd, 0, SEEK_SET);
    }
    result = lit_string_makeemptystring(state, length, true);
    result->strbuf.data = (char*)lit_sysmem_malloc((length + 1) * sizeof(char));
    result->strbuf.data[length] = '\0';
    rsz = fread(result->strbuf.data, sizeof(char), length, hnd);
    result->strbuf.length = rsz;
    result->strhash = lit_string_hash(result->strbuf.data, result->strbuf.length);
    lit_string_register(state, result);
    return result;
}

LitValue lit_objfnfile_readallinstance(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    bool havesizeparam;
    size_t howmuch;
    LitFileData* data;
    LitString* result;
    (void)argc;
    (void)args;
    howmuch = 0;
    havesizeparam = false;
    if(argc > 0)
    {
        howmuch = LIT_CHECK_NUMBER(0);
        havesizeparam = true;
    }
    data = (LitFileData*)lit_userdata_extractdata(instance);
    result = lit_util_readhandletostring(state, data->file, havesizeparam, howmuch);
    return lit_value_fromobject(result);
}

LitValue lit_objfnfile_readallstatic(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    bool havesizeparam;
    size_t howmuch;
    const char* filename;
    FILE* hnd;
    LitString* result;
    (void)instance;
    (void)argc;
    (void)args;
    howmuch = 0;
    havesizeparam = false;
    filename = LIT_CHECK_GETSTRINGOBJECT(0)->strbuf.data;
    if(argc > 1)
    {
        howmuch = LIT_CHECK_NUMBER(1);
        havesizeparam = true;
    }
    hnd = fopen(filename, "rb");
    if(!hnd)
    {
        lit_vm_raisefatalerror(state, "cannot open '%s' for reading", filename);
        return lit_value_makenull();
    }
    result = lit_util_readhandletostring(state, hnd, havesizeparam, howmuch);
    fclose(hnd);
    return lit_value_fromobject(result);
}

LitValue lit_objfnfile_readline(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    char ch;
    LitString* res;
    LitFileData* data;
    (void)argc;
    (void)args;
    data = (LitFileData*)lit_userdata_extractdata(instance);
    res = lit_string_makeemptystring(state, 64, false);
    while(true)
    {
        ch = fgetc(data->file);
        if(ch == EOF)
        {
            break;
        }
        if(ch == '\n')
        {
            break;
        }
        lit_string_appendbyte(res, ch);
    }
    return lit_value_fromobject(res);
}

LitValue lit_objfnfile_getlastmodified(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    struct stat buffer;
    char* fname;
    (void)instance;
    fname = (char*)LIT_CHECK_GETSTRINGDATA(0);
    if(stat(fname, &buffer) != 0)
    {
        return lit_value_makenumber(0);
    }
    return lit_value_makenumber(buffer.st_mtime);
}

/*
 * Directory
 */

LitValue lit_objfndirectory_exists(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    const char* directoryname = LIT_CHECK_GETSTRINGDATA(0);
    struct stat buffer;
    return lit_value_makebool(stat(directoryname, &buffer) == 0);
}

LitValue lit_objfndirectory_read(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
#ifdef WIN32
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(lit_array_make(state));
#else
    size_t fnlen;
    size_t topdlen;
    LitString* res;
    LitArray* array;
    const char* topdname;
    const char* fname;
    LitIOStream pr;
    DIR* dhnd;
    struct dirent* ent;
    (void)instance;
    topdname = LIT_CHECK_GETSTRINGDATA(0);
    topdlen = strlen(topdname);
    dhnd = opendir(topdname);
    array = lit_array_make(state);
    if(dhnd == NULL)
    {
        return lit_value_fromobject(array);
    }
    while((ent = readdir(dhnd)))
    {
        lit_iostream_makestackstring(&pr);
        fname = ent->d_name;
        fnlen = strlen(fname);
        if(strcmp(fname, "..") == 0 || strcmp(fname, ".") == 0)
        {
            continue;
        }
        lit_iostream_putlen(&pr, topdname, topdlen);
        lit_iostream_putlen(&pr, "/", 1);
        lit_iostream_putlen(&pr, fname, fnlen);
        res = lit_iostream_takestring(state, &pr);
        struct stat st;
        stat(res->strbuf.data, &st);
        lit_dynlistval_push(&array->innerlist, lit_value_fromobject(res));
    }
    closedir(dhnd);
    return lit_value_fromobject(array);
#endif
}

void lit_corelib_installfile(LitState* state)
{
    lit_vmexec_pushgc(state, false);
    {
        LitClass* klass = lit_class_make(state, "File", state->stdobjectclass);
        lit_class_bindstaticmethod(klass, "exists", lit_objfnfile_exists);
        lit_class_bindstaticmethod(klass, "getLastModified", lit_objfnfile_getlastmodified);
        lit_class_bindstaticmethod(klass, "create", lit_objfnfile_create);
        lit_class_bindstaticmethod(klass, "read", lit_objfnfile_readallstatic);
        lit_class_bindconstructor(klass, lit_objfnfile_constructor);
        lit_class_bindmethod(klass, "close", lit_objfnfile_close);
        lit_class_bindmethod(klass, "write", lit_objfnfile_writevalvalue);
        lit_class_bindmethod(klass, "writeString", lit_objfnfile_writevalstring);
        lit_class_bindmethod(klass, "readAll", lit_objfnfile_readallinstance);
        lit_class_bindmethod(klass, "readLine", lit_objfnfile_readline);
    }
    {
        LitClass* klass = lit_class_make(state, "Directory", state->stdobjectclass);
        lit_class_bindstaticmethod(klass, "exists", lit_objfndirectory_exists);
        lit_class_bindstaticmethod(klass, "read", lit_objfndirectory_read);
    }
    lit_vmexec_popgc(state);
}

LitValue lit_objfngc_memoryused(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(state->bytes_allocated);
}

LitValue lit_objfngc_nextround(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(state->gcnextgc);
}

LitValue lit_objfngc_trigger(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    state->gcallowgc = true;
    int64_t collected = lit_collect_garbage(state);
    state->gcallowgc = false;
    return lit_value_makenumber(collected);
}

void lit_corelib_installgc(LitState* state)
{
    lit_vmexec_pushgc(state, false);
    LitClass* klass = lit_class_make(state, "GC", state->stdobjectclass);
    lit_class_bindstaticgetter(klass, "memoryUsed", lit_objfngc_memoryused);
    lit_class_bindstaticgetter(klass, "nextRound", lit_objfngc_nextround);
    lit_class_bindstaticmethod(klass, "trigger", lit_objfngc_trigger);
    lit_vmexec_popgc(state);
}

LitValue lit_objfnmath_abs(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fabs(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_cos(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(cos(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_sin(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(sin(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_tan(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(tan(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_acos(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(acos(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_asin(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(asin(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_atan(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(atan(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_atan2(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(atan2(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue lit_objfnmath_floor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(floor(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_ceil(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(ceil(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_round(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    double value = LIT_CHECK_NUMBER(0);
    if(argc > 1)
    {
        int places = (int)pow(10, LIT_CHECK_NUMBER(1));
        return lit_value_makenumber(round(value * places) / places);
    }
    return lit_value_makenumber(round(value));
}

LitValue lit_objfnmath_min(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fmin(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue lit_objfnmath_max(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fmax(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue lit_objfnmath_mid(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    double x = LIT_CHECK_NUMBER(0);
    double y = LIT_CHECK_NUMBER(1);
    double z = LIT_CHECK_NUMBER(2);
    if(x > y)
    {
        return lit_value_makenumber(fmax(x, fmin(y, z)));
    }
    else
    {
        return lit_value_makenumber(fmax(y, fmin(x, z)));
    }
}

LitValue lit_objfnmath_toRadians(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(LIT_CHECK_NUMBER(0) * M_PI / 180.0);
}

LitValue lit_objfnmath_toDegrees(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(LIT_CHECK_NUMBER(0) * 180.0 / M_PI);
}

LitValue lit_objfnmath_sqrt(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(sqrt(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_log(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(exp(LIT_CHECK_NUMBER(0)));
}

LitValue lit_objfnmath_exp(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(exp(LIT_CHECK_NUMBER(0)));
}

void lit_corelib_installmath(LitState* state)
{
    LitClass* klass;
    lit_vmexec_pushgc(state, false);
    {
        klass = lit_class_make(state, "Math", state->stdobjectclass);
        lit_class_setstaticfield(klass, "Pi", lit_value_makenumber(M_PI));
        lit_class_setstaticfield(klass, "Tau", lit_value_makenumber(M_PI * 2));
        lit_class_bindstaticmethod(klass, "abs", lit_objfnmath_abs);
        lit_class_bindstaticmethod(klass, "sin", lit_objfnmath_sin);
        lit_class_bindstaticmethod(klass, "cos", lit_objfnmath_cos);
        lit_class_bindstaticmethod(klass, "tan", lit_objfnmath_tan);
        lit_class_bindstaticmethod(klass, "asin", lit_objfnmath_asin);
        lit_class_bindstaticmethod(klass, "acos", lit_objfnmath_acos);
        lit_class_bindstaticmethod(klass, "atan", lit_objfnmath_atan);
        lit_class_bindstaticmethod(klass, "atan2", lit_objfnmath_atan2);
        lit_class_bindstaticmethod(klass, "floor", lit_objfnmath_floor);
        lit_class_bindstaticmethod(klass, "ceil", lit_objfnmath_ceil);
        lit_class_bindstaticmethod(klass, "round", lit_objfnmath_round);
        lit_class_bindstaticmethod(klass, "min", lit_objfnmath_min);
        lit_class_bindstaticmethod(klass, "max", lit_objfnmath_max);
        lit_class_bindstaticmethod(klass, "mid", lit_objfnmath_mid);
        lit_class_bindstaticmethod(klass, "toRadians", lit_objfnmath_toRadians);
        lit_class_bindstaticmethod(klass, "toDegrees", lit_objfnmath_toDegrees);
        lit_class_bindstaticmethod(klass, "sqrt", lit_objfnmath_sqrt);
        lit_class_bindstaticmethod(klass, "log", lit_objfnmath_log);
        lit_class_bindstaticmethod(klass, "exp", lit_objfnmath_exp);
    }
    lit_vmexec_popgc(state);
}



bool lit_fiber_ensureframes(LitState* state, LitFiber* fiber)
{
    size_t incsize;
    size_t oldsize;
    size_t inccap;
    (void)oldsize;
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(state, "no Fiber to run on");
        return true;
    }
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        inccap = (fiber->framecapacity * 2);
        oldsize = (sizeof(LitCallFrame) * fiber->framecapacity);
        incsize = (sizeof(LitCallFrame) * inccap);
        fiber->framevals = (LitCallFrame*)lit_sysmem_realloc(fiber->framevals, incsize);
        if(fiber->framevals == NULL)
        {
            return false;
        }
        fiber->framecapacity = inccap;
    }
    return false;
}

LitCallFrame* lit_state_setupcallonframe(LitState* state, LitFuncScript* callee, LitValue* arguments, size_t argc)
{
    size_t i;
    size_t ai;
    size_t ti;
    LitFiber* fiber = state->vmstate.fiber;
    if(callee == NULL)
    {
        lit_vm_raisefatalerror(state, "attempt to call a null value");
        return NULL;
    }
    if(lit_fiber_ensureframes(state, fiber))
    {
        return NULL;
    }
    LitValue* start = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
    lit_fiber_ensureregisters(fiber, start - fiber->registeritems + callee->maxregisters);
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->maxregisters : fiber->registeritems;
#ifdef LIT_TRACE_NULL_FILL
    printf("filling with nulls\n");
#endif
    for(i = argc + 1; i < callee->maxregisters; i++)
    {
        frame->slots[i] = lit_value_makenull();
    }
    frame->slots[0] = lit_value_fromobject(callee);
    for(ai = 0; ai < argc; ai++)
    {
        frame->slots[ai + 1] = arguments[ai];
    }
    size_t targetargcount = callee->argcount;
    bool vararg = callee->vararg;
    if(targetargcount > argc)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("filling with nulls\n");
#endif
        for(ti = argc; ti < targetargcount; ti++)
        {
            *(frame->slots + ti + 1) = lit_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = lit_value_fromobject(lit_array_make(state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && lit_value_isvargarray(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            LitArray* array = &lit_object_makevararray(state)->innerarray;
            lit_state_pushroot(state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->innerlist, argc - targetargcount + 1);
            size_t j = 0;
            for(ti = targetargcount - 1; ti < argc; ti++)
            {
                array->innerlist.listitems[j++] = *(frame->slots + ti + 1);
            }
            *(frame->slots + targetargcount) = lit_value_fromobject(array);
            lit_state_poproot(state);
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

LitResult lit_state_execcallonframe(LitState* state, LitCallFrame* frame)
{
    if(frame == NULL)
    {
        RETURN_RUNTIME_ERROR()
    }
    LitFiber* fiber = state->vmstate.fiber;
    LitResult result = lit_state_execfiber(state, fiber);
    if(!lit_value_isnull(fiber->error))
    {
        result.result = fiber->error;
    }
    return result;
}

LitResult lit_state_callfunction(LitState* state, LitFuncScript* callee, LitValue* arguments, size_t argc)
{
    return lit_state_execcallonframe(state, lit_state_setupcallonframe(state, callee, arguments, argc));
}

LitResult lit_state_callclosure(LitState* state, LitFuncClosure* callee, LitValue* arguments, size_t argc)
{
    LitCallFrame* frame = lit_state_setupcallonframe(state, callee->function, arguments, argc);
    if(frame == NULL)
    {
        RETURN_RUNTIME_ERROR()
    }
    frame->closure = callee;
    return lit_state_execcallonframe(state, frame);
}

LitResult lit_state_callmethod(LitState* state, LitValue instance, LitValue callee, LitValue* arguments, size_t argc)
{
    size_t i;
    size_t ai;
    if(lit_value_isobject(callee))
    {
        if(lit_set_native_exit_jump())
        {
            RETURN_RUNTIME_ERROR()
        }
        LitObjType type = lit_value_objtype(callee);
        if(type == LIT_OBJ_FUNCSCRIPT)
        {
            return lit_state_callfunction(state, lit_value_asfuncscript(callee), arguments, argc);
        }
        else if(type == LIT_OBJ_FUNCCLOSURE)
        {
            return lit_state_callclosure(state, lit_value_asfuncclosure(callee), arguments, argc);
        }
        LitFiber* fiber = state->vmstate.fiber;
        if(lit_fiber_ensureframes(state, fiber))
        {
            RETURN_RUNTIME_ERROR()
        }
        LitValue* start = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
        lit_fiber_ensureregisters(fiber, start - fiber->registeritems + 3 + argc);
        LitValue* slot = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->maxregisters : fiber->registeritems;
#ifdef LIT_TRACE_NULL_FILL
        printf("filling with nulls\n");
#endif
        for(i = argc; i < argc + 3; i++)
        {
            *(slot + i) = lit_value_makenull();
        }
        *slot = instance;
        if(type != LIT_OBJ_CLASS)
        {
            for(ai = 0; ai < argc; ai++)
            {
                *(slot + ai + 1) = arguments[ai];
            }
        }

        if(LIT_UNLIKELY(state->config.traceexecution))
        {
            if(!state->config.traceinstsonly)
            {
                lit_debug_traceprintvalue(state->config.desttrace, "<vm:slots>", fiber->framecount, argc, slot);
            }
        }
        switch(type)
        {
            case LIT_OBJ_FUNCNATIVE:
            {
                /* for some reason, single line expression doesn't work */
                LitValue value = lit_value_asfuncnative(callee)->natfuncptr(state, lit_value_makenull(), argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_FUNCNATMETHOD:
            {
                LitFuncNative* method = lit_value_asfuncmethod(callee);
                /* For some reason, single line expression doesn't work */
                LitValue value = method->natfuncptr(state, *slot, argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = lit_value_asclass(callee);
                LitInstance* inst = lit_object_makeinstance(state, klass);
                if(klass->mthconstructor != NULL)
                {
                    lit_state_callmethod(state, *slot, lit_value_fromobject(klass->mthconstructor), arguments, argc);
                }
                RETURN_OK(lit_value_fromobject(inst))
            }
            case LIT_OBJ_FUNCBOUNDMETHOD:
            {
                LitFuncBound* boundmethod = lit_value_asfuncboundmethod(callee);
                LitValue method = boundmethod->method;
                if(lit_value_isfuncmethod(method))
                {
                    /* For some reason, single line expression doesn't work */
                    LitValue value = lit_value_asfuncmethod(method)->natfuncptr(state, boundmethod->receiver, argc, slot + 1);
                    RETURN_OK(value)
                }
                else
                {
                    *slot = boundmethod->receiver;
                    return lit_state_callfunction(state, lit_value_asfuncscript(method), arguments, argc);
                }
            }
            default:
            {
                break;
            }
        }
    }
    if(lit_value_isnull(callee))
    {
        lit_vm_raisefatalerror(state, "attempt to call a null value");
    }
    else
    {
        lit_vm_raisefatalerror(state, "can only call functions and classes");
    }
    RETURN_RUNTIME_ERROR()
}

LitResult lit_state_callvalue(LitState* state, LitValue callee, LitValue* arguments, size_t argc)
{
    return lit_state_callmethod(state, callee, callee, arguments, argc);
}

LitResult lit_state_findandcallmethod(LitState* state, LitValue callee, LitString* mthname, LitValue* arguments, size_t argc)
{
    bool ok;
    LitValue method;
    LitFiber* fiber;
    LitClass* klass;
    LitInstance* inst;
    fiber = state->vmstate.fiber;
    ok = false;
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(state, "no Fiber to run on");
        RETURN_RUNTIME_ERROR()
    }
    klass = lit_state_getclassfor(state, callee);
    if(lit_value_isinstance(callee))
    {
        inst = lit_value_asinstance(callee);
        if(lit_table_getentry(&inst->fields, mthname, &method))
        {
            ok = true;
        }
    }
    else if(klass != NULL)
    {
        if(lit_table_getentry(&klass->mthtable, mthname, &method))
        {
            ok = true;
        }
    }
    if(ok)
    {
        return lit_state_callmethod(state, callee, method, arguments, argc);
    }
    return lit_result_make(LIT_STATUS_INVALID, lit_value_makenull());
}

LitString* lit_value_tostrinvoketostring(LitState* state, LitValue object, size_t indentation, bool explicitfail)
{
    size_t needed;
    LitValue tmpv;
    LitValue* tmptr;
    LitFiber* fiber;
    LitInstance* inst;
    fiber = state->vmstate.fiber;
    /*
    * NB: only do this with instances for now.
    */
    if(!lit_value_isinstance(object))
    {
        goto failed;
    }
    if(lit_value_isinstance(object))
    {
        inst = lit_value_asinstance(object);
        if(!lit_table_getentry(&inst->klass->mthtable, state->strings.strtostring, &tmpv))
        {
            return NULL;
        }       
    }
    if(lit_fiber_ensureframes(state, fiber))
    {
        goto failed;
    }
    LitFuncScript* function = state->apifunction;
    if(function == NULL)
    {
        function = state->apifunction = lit_object_makefunction(state, fiber->module);
        function->chunk.haslineinfo = false;
        function->name = state->apiname;
        LitChunk* chunk = &function->chunk;
        chunk->compiledcodecount = 0;
        chunk->constantlist.listcount = 0;
        function->maxregisters = 3;
        int constant = lit_chunk_addconstant(state, chunk, lit_value_fromobject(state->strings.strtostring));
        lit_chunk_push(chunk, LIT_REG_FORMABCINST(LIT_OPCODE_INVOKE, 1, 2, constant), 1);
        lit_chunk_push(chunk, LIT_REG_FORMABCINST(LIT_OPCODE_RETURN, 1, 0, 0), 1);
    }
    tmptr = fiber->registeritems;
    if(fiber->framecount > 0)
    {
        tmptr = (fiber->framevals[fiber->framecount - 1].slots + (int)fiber->framevals[fiber->framecount - 1].function->maxregisters);
    }
    needed = (tmptr - fiber->registeritems + function->maxregisters);
    lit_fiber_ensureregisters(fiber, needed);
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    frame->ip = function->chunk.compiledcodechunk;
    frame->closure = NULL;
    frame->function = function;
    /* "duplicated" code due to lit_fiber_ensureregisters messing with register pointers */
    frame->slots = fiber->registeritems;
    if(fiber->framecount > 1)
    {
        frame->slots = (fiber->framevals[fiber->framecount - 2].slots + (int)fiber->framevals[fiber->framecount - 2].function->maxregisters);
    }
    frame->resultignored = false;
    frame->returntoc = true;
    frame->returnaddress = NULL;
    frame->slots[0] = lit_value_fromobject(function);
    frame->slots[1] = object;
    frame->slots[2] = lit_value_makenumber(indentation);
    LitResult result = lit_state_execfiber(state, fiber);
    if(result.type != LIT_STATUS_OK)
    {
        if(explicitfail)
        {
            return lit_string_copy(state, "null");
        }
        return NULL;
    }
    if(!lit_value_isstring(result.result))
    {
        if(explicitfail)
        {
            return lit_string_copy(state, "invalid toString()");
        }
        return NULL;
    }
    return lit_value_asstring(result.result);
    failed:
        if(explicitfail)
        {
            return lit_string_copy(state, "null");
        }
        return NULL;
}

LitString* lit_value_tostring(LitState* state, LitValue object, size_t indentation)
{
    LitIOStream pr;
    if(lit_value_isstring(object))
    {
        return lit_value_asstring(object);
    }
    else if(!lit_value_isobject(object))
    {
        if(lit_value_isnull(object))
        {
            return lit_string_copy(state, "null");
        }
        else if(lit_value_isnumber(object))
        {
            return lit_value_asstring(lit_string_numbertostring(state, lit_value_asnumber(object)));
        }
        else if(lit_value_isbool(object))
        {
            return lit_string_copy(state, lit_value_asbool(object) ? "true" : "false");
        }
    }
    else if(lit_value_isreference(object))
    {
        LitValue* slot = lit_value_asreference(object)->slot;
        if(slot == NULL)
        {
            return lit_string_copy(state, "null");
        }
        return lit_value_tostring(state, *slot, 0);
    }
    return lit_value_tostrinvoketostring(state, object, indentation, true);
}

LitValue lit_state_callnew(LitState* state, const char* name, LitValue* args, size_t argc)
{
    LitValue value;
    if(!lit_map_get(state->vmstate.globals, lit_string_copy(state, name), &value))
    {
        lit_vm_raisefatalerror(state, "failed to create instance of class %s: class not found", name);
        return lit_value_makenull();
    }
    LitClass* klass = lit_value_asclass(value);
    if(klass->mthconstructor == NULL)
    {
        return lit_value_fromobject(lit_object_makeinstance(state, klass));
    }
    return lit_state_callmethod(state, value, value, args, argc).result;
}

bool lit_value_iscallablefunction(LitValue value)
{
    if(lit_value_isobject(value))
    {
        LitObjType type = lit_value_objtype(value);
        return ((type == LIT_OBJ_FUNCCLOSURE) || (type == LIT_OBJ_FUNCSCRIPT) || (type == LIT_OBJ_FUNCNATIVE) || (type == LIT_OBJ_FUNCNATMETHOD) || (type == LIT_OBJ_FUNCBOUNDMETHOD));
    }
    return false;
}


LitObject* lit_object_allocobject(LitState* state, size_t size, LitObjType type)
{
    LitObject* object = (LitObject*)lit_sysmem_malloc(size);
    object->pstate = state;
    object->type = type;
    object->marked = false;
    object->next = state->vmstate.objects;
    state->vmstate.objects = object;
#ifdef LIT_CONFIG_LOGALLOCATION
    printf("%p allocate %ld for %s\n", (void*)object, size, lit_value_objtypename(type));
#endif
    return object;
}

LitFuncScript* lit_object_makefunction(LitState* state, LitModule* module)
{
    LitFuncScript* function = (LitFuncScript*)lit_object_allocobject(state, sizeof(LitFuncScript), LIT_OBJ_FUNCSCRIPT);
    lit_chunk_init(&function->chunk);
    function->name = NULL;
    function->argcount = 0;
    function->upvaluecount = 0;
    function->maxregisters = 0;
    function->module = module;
    function->vararg = false;
    return function;
}

LitValue lit_function_getname(LitState* state, LitValue instance)
{
    LitString* name = NULL;
    switch(lit_value_objtype(instance))
    {
        case LIT_OBJ_FUNCSCRIPT:
        {
            name = lit_value_asfuncscript(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCCLOSURE:
        {
            name = lit_value_asfuncclosure(instance)->function->name;
            break;
        }
        case LIT_OBJ_CLSPROTOTYPE:
        {
            name = lit_value_asclsproto(instance)->function->name;
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = lit_value_asfield(instance);
            if(field->getter != NULL)
            {
                return lit_function_getname(state, lit_value_fromobject(field->getter));
            }
            return lit_function_getname(state, lit_value_fromobject(field->setter));
        }
        case LIT_OBJ_FUNCNATIVE:
        {
            name = lit_value_asfuncnative(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCNATMETHOD:
        {
            name = lit_value_asfuncmethod(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCBOUNDMETHOD:
        {
            return lit_function_getname(state, lit_value_asfuncboundmethod(instance)->method);
        }
        default:
        {
            break;
        }
    }
    if(name == NULL)
    {
        return lit_string_valformat(state, "function #", *((double*)lit_value_asobject(instance)));
    }
    return lit_string_valformat(state, "function @", lit_value_fromobject(name));
}

LitUpvalue* lit_object_makeupvalue(LitState* state, LitValue* slot)
{
    LitUpvalue* upvalue = (LitUpvalue*)lit_object_allocobject(state, sizeof(LitUpvalue), LIT_OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = lit_value_makenull();
    upvalue->next = NULL;
    return upvalue;
}

LitFuncClosure* lit_object_makeclosure(LitState* state, LitFuncScript* function)
{
    size_t i;
    LitFuncClosure* closure = (LitFuncClosure*)lit_object_allocobject(state, sizeof(LitFuncClosure), LIT_OBJ_FUNCCLOSURE);
    closure->function = function;
    /* to prevent GC crashes */
    closure->upvaluecount = 0;
    lit_state_pushroot(state, (LitObject*)closure);
    LitUpvalue** upvalues = (LitUpvalue**)lit_sysmem_malloc(function->upvaluecount * sizeof(LitUpvalue*));
    lit_state_poproot(state);
    for(i = 0; i < function->upvaluecount; i++)
    {
        upvalues[i] = NULL;
    }
    closure->upvalues = upvalues;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

LitClsPrototype* lit_object_makeclsproto(LitState* state, LitFuncScript* function)
{
    LitClsPrototype* closure = (LitClsPrototype*)lit_object_allocobject(state, sizeof(LitClsPrototype), LIT_OBJ_CLSPROTOTYPE);
    lit_state_pushroot(state, (LitObject*)closure);
    closure->indexes = (uint32_t*)lit_sysmem_malloc(function->upvaluecount * sizeof(uint32_t));
    closure->local = (bool*)lit_sysmem_malloc(function->upvaluecount * sizeof(bool));
    lit_state_poproot(state);
    closure->function = function;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

LitFuncNative* lit_object_makenativefunc(LitState* state, LitNativeFunctionFn function, LitString* name)
{
    LitFuncNative* native = (LitFuncNative*)lit_object_allocobject(state, sizeof(LitFuncNative), LIT_OBJ_FUNCNATIVE);
    native->natfuncptr = function;
    native->name = name;
    return native;
}


LitFuncNative* lit_object_makenativemethod(LitState* state, LitNativeFunctionFn method, LitString* name)
{
    LitFuncNative* native = (LitFuncNative*)lit_object_allocobject(state, sizeof(LitFuncNative), LIT_OBJ_FUNCNATMETHOD);
    native->natfuncptr = method;
    native->name = name;
    return native;
}


LitFiber* lit_object_makefiber(LitState* state, LitModule* module, LitFuncScript* function)
{
    size_t i;
    /* Allocate in advance, just in case GC is triggered */
    size_t registersallocated = function == NULL ? 1 : lit_util_closestpoweroftwo(function->maxregisters);
    LitValue* registers = (LitValue*)lit_sysmem_malloc(registersallocated * sizeof(LitValue));
    LitCallFrame* framevals = (LitCallFrame*)lit_sysmem_malloc(LIT_INITIAL_CALL_FRAMES * sizeof(LitCallFrame));
    LitFiber* fiber = (LitFiber*)lit_object_allocobject(state, sizeof(LitFiber), LIT_OBJ_FIBER);
    if(module->mainfiber == NULL)
    {
        module->mainfiber = fiber;
    }
    fiber->registeritems = registers;
    for(i = 0; i < registersallocated; i++)
    {
        fiber->registeritems[i] = lit_value_makenull();
    }
    fiber->registersallocated = registersallocated;
    fiber->framevals = framevals;
    fiber->framecapacity = LIT_INITIAL_CALL_FRAMES;
    fiber->parent = NULL;
    fiber->framecount = function == NULL ? 0 : 1;
    fiber->argcount = 0;
    fiber->module = module;
    fiber->catcher = false;
    fiber->caught = false;
    fiber->error = lit_value_makenull();
    fiber->openupvalues = NULL;
    fiber->abort = false;
    fiber->returnaddress = NULL;

    fiber->handlers = NULL;
    fiber->handler_count = 0;
    fiber->handler_capacity = 0;

    if(function != NULL)
    {
        LitCallFrame* frame = &fiber->framevals[0];
        frame->closure = NULL;
        frame->function = function;
        frame->slots = fiber->registeritems;
        frame->resultignored = false;
        frame->returntoc = false;
        frame->returnaddress = NULL;
        lit_fiber_ensureregisters(fiber, function->maxregisters);
        frame->ip = function->chunk.compiledcodechunk;
    }
    return fiber;
}

LitFiber* lit_object_makefiberclosure(LitState* state, LitModule* module, LitFuncClosure* closure)
{
    LitFiber* fiber = lit_object_makefiber(state, module, closure->function);
    fiber->framevals[0].closure = closure;
    return fiber;
}

void lit_fiber_ensureregisters(LitFiber* fiber, size_t needed)
{
    size_t i;
    size_t capacity;
    LitUpvalue* upvalue;
    LitValue* oldregisters;
    if(fiber->registersallocated >= needed)
    {
        return;
    }
    capacity = (size_t)lit_util_closestpoweroftwo((int)needed);
    oldregisters = fiber->registeritems;
    fiber->registeritems = (LitValue*)lit_sysmem_realloc(fiber->registeritems, sizeof(LitValue) * capacity);
    for(i = fiber->registersallocated; i < capacity; i++)
    {
        fiber->registeritems[i] = lit_value_makenull();
    }
    fiber->registersallocated = capacity;
    if(fiber->registeritems != oldregisters)
    {
        for(i = 0; i < fiber->framecount; i++)
        {
            LitCallFrame* frame = &fiber->framevals[i];
            int difference = (frame->slots - oldregisters);
            LitValue* oldslots = frame->slots;
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

LitModule* lit_object_makemodule(LitState* state, LitString* name)
{
    LitModule* module = (LitModule*)lit_object_allocobject(state, sizeof(LitModule), LIT_OBJ_MODULE);
    module->name = name;
    module->returnvalue = lit_value_makenull();
    module->mainfunction = NULL;
    module->privatevalues = NULL;
    module->ran = false;
    module->mainfiber = NULL;
    module->privatecount = 0;
    module->privatenames = lit_object_makemap(state, NULL);
    return module;
}

LitClass* lit_object_makeclass(LitState* state, LitString* name)
{
    LitClass* klass = (LitClass*)lit_object_allocobject(state, sizeof(LitClass), LIT_OBJ_CLASS);
    klass->name = name;
    klass->mthconstructor = NULL;
    klass->super = NULL;
    lit_table_init(state, &klass->mthtable);
    lit_table_init(state, &klass->staticstable);
    return klass;
}

LitInstance* lit_object_makeinstance(LitState* state, LitClass* klass)
{
    LitInstance* instance = (LitInstance*)lit_object_allocobject(state, sizeof(LitInstance), LIT_OBJ_INSTANCE);
    instance->klass = klass;
    lit_table_init(state, &instance->fields);
    return instance;
}

LitFuncBound* lit_object_makeboundmethod(LitState* state, LitValue receiver, LitValue method)
{
    LitFuncBound* boundmethod = (LitFuncBound*)lit_object_allocobject(state, sizeof(LitFuncBound), LIT_OBJ_FUNCBOUNDMETHOD);
    boundmethod->receiver = receiver;
    boundmethod->method = method;
    return boundmethod;
}

LitArray* lit_array_make(LitState* state)
{
    LitArray* array = (LitArray*)lit_object_allocobject(state, sizeof(LitArray), LIT_OBJ_ARRAY);
    lit_dynlistval_init(&array->innerlist);
    return array;
}

void lit_array_push(LitArray* array, LitValue val)
{
    lit_dynlistval_push(&array->innerlist, val);
}

size_t lit_array_size(LitArray* array)
{
    return array->innerlist.listcount;
}

size_t lit_array_count(LitArray* array)
{
    return array->innerlist.listcount;
}

LitValue lit_array_get(LitArray* ary, size_t idx)
{
    return lit_dynlistval_get(&ary->innerlist, idx);
}

LitValue lit_array_set(LitArray* ary, size_t idx, LitValue val)
{
    return lit_dynlistval_set(&ary->innerlist, idx, val);
}

LitValue lit_array_removeat(LitArray* array, size_t index)
{
    size_t i;
    size_t count;
    LitValue value;
    LitDynListVal* vl;
    vl = &array->innerlist;
    count = vl->listcount;
    if(index >= count)
    {
        return lit_value_makenull();
    }
    value = lit_dynlistval_get(vl, index);
    if(index == count - 1)
    {
        lit_dynlistval_set(vl, index, lit_value_makenull());
    }
    else
    {
        for(i = index; i < vl->listcount - 1; i++)
        {
            lit_dynlistval_set(vl, i, lit_dynlistval_get(vl, i + 1));
        }
        lit_dynlistval_set(vl, count - 1, lit_value_makenull());
    }
    vl->listcount--;
    return value;
}

LitVarargArray* lit_object_makevararray(LitState* state)
{
    LitVarargArray* array = (LitVarargArray*)lit_object_allocobject(state, sizeof(LitVarargArray), LIT_OBJ_VARARGARRAY);
    lit_dynlistval_init(&array->innerarray.innerlist);
    return array;
}

LitMap* lit_object_makemap(LitState* state, LitTable* fields)
{
    LitMap* map = (LitMap*)lit_object_allocobject(state, sizeof(LitMap), LIT_OBJ_MAP);
    lit_table_init(state, &map->innertable);
    if(fields != NULL)
    {
        lit_table_addall(fields, &map->innertable);
    }
    return map;
}

bool lit_map_set(LitMap* map, LitString* key, LitValue value)
{
    if(lit_value_isnull(value))
    {
        lit_map_delete(map, key);
        return false;
    }
    return lit_table_set(&map->innertable, key, value);
}

bool lit_map_get(LitMap* map, LitString* key, LitValue* value)
{
    return lit_table_getentry(&map->innertable, key, value);
}

bool lit_map_delete(LitMap* map, LitString* key)
{
    return lit_table_delete(&map->innertable, key);
}

void lit_map_addall(LitMap* from, LitMap* to)
{
    int i;
    for(i = 0; i <= from->innertable.htcapacity; i++)
    {
        LitTabEntry* entry = &from->innertable.htentries[i];
        if(entry->entkey != NULL)
        {
            lit_table_set(&to->innertable, entry->entkey, entry->entvalue);
        }
    }
}

LitValue lit_map_getfield(LitMap* map, const char* name)
{
    LitValue value;
    LitState* state;
    state = ((LitObject*)map)->pstate;
    if(!lit_table_getentry(&map->innertable, lit_string_copy(state, name), &value))
    {
        value = lit_value_makenull();
    }
    return value;
}

void lit_map_setfield(LitMap* map, const char* name, LitValue value)
{
    LitState* state;
    state = ((LitObject*)map)->pstate;
    lit_table_set(&map->innertable, lit_string_copy(state, name), value);
}

LitUserdata* lit_userdata_makeuserdata(LitState* state, size_t size)
{
    LitUserdata* userdata = (LitUserdata*)lit_object_allocobject(state, sizeof(LitUserdata), LIT_OBJ_USERDATA);
    if(size > 0)
    {
        userdata->data = lit_sysmem_malloc(size);
    }
    else
    {
        userdata->data = NULL;
    }
    userdata->size = size;
    userdata->oncleanupfn = NULL;
    return userdata;
}

void* lit_userdata_insertdata(LitState* state, LitValue instance, size_t typesz, LitCleanupFn cleanup)
{
    LitUserdata* userdata;
    userdata = lit_userdata_makeuserdata(state, typesz);
    userdata->oncleanupfn = cleanup;
    lit_table_set(&lit_value_asinstance(instance)->fields, lit_string_copy(state, "_data"), lit_value_fromobject(userdata));
    return userdata->data;
}

void* lit_userdata_extractdata(LitValue instance)
{
    LitValue temp;
    LitState* state;
    LitInstance* inst;
    inst = lit_value_asinstance(instance);
    state = ((LitObject*)inst)->pstate;
    if(!lit_table_getentry(&inst->fields, lit_string_copy(state, "_data"), &temp))
    {
        lit_vm_raisefatalerror(state, "failed to extract userdata");
    }
    return lit_value_asuserdata(temp)->data;
}

LitRange* lit_object_makerange(LitState* state, double from, double to)
{
    LitRange* range = (LitRange*)lit_object_allocobject(state, sizeof(LitRange), LIT_OBJ_RANGE);
    range->from = from;
    range->to = to;
    return range;
}

LitField* lit_object_makefield(LitState* state, LitObject* getter, LitObject* setter)
{
    LitField* field = (LitField*)lit_object_allocobject(state, sizeof(LitField), LIT_OBJ_FIELD);
    field->getter = getter;
    field->setter = setter;
    return field;
}

LitReference* lit_object_makereference(LitState* state, LitValue* slot)
{
    LitReference* reference = (LitReference*)lit_object_allocobject(state, sizeof(LitReference), LIT_OBJ_REFERENCE);
    reference->slot = slot;
    return reference;
}

void lit_state_defaultprinterrmsgerror(LitState* state, const char* message)
{
    LitIOStream* pr;
    (void)state;
    pr = state->streamstderr;
    fflush(stdout);
    if(message != NULL)
    {
        lit_iostream_writecolor(pr, COLOR_RED);
        lit_iostream_printf(pr, "unhandled error in state:\n");
        lit_iostream_printf(pr, "  %s\n", message);
        lit_iostream_writecolor(pr, COLOR_RESET);
        fflush(stderr);
    }
    state->had_error = true;

}

LitState* lit_state_make()
{
    LitState* state = (LitState*)lit_sysmem_malloc(sizeof(LitState));
    state->stdclassclass = NULL;
    state->stdobjectclass = NULL;
    state->number_class = NULL;
    state->string_class = NULL;
    state->bool_class = NULL;
    state->function_class = NULL;
    state->fiber_class = NULL;
    state->module_class = NULL;
    state->array_class = NULL;
    state->map_class = NULL;
    state->range_class = NULL;
    state->bytes_allocated = 0;
    state->gcnextgc = 256 * 1024;
    state->gcallowgc = false;
    state->printerrmessagefn = lit_state_defaultprinterrmsgerror;
    state->had_error = false;
    state->roots = NULL;
    state->root_count = 0;
    state->root_capacity = 0;
    state->last_module = NULL;
    state->config.dumpast = false;
    state->config.traceexecution = false;
    state->config.traceinstsonly = false;
    state->config.tracechunk = false;
    state->config.isreplmode = false;
    state->config.havedesttrace = false;
    state->config.quitafterdump = false;
    state->streamstdout = lit_iostream_makeio(stdout, false);
    state->streamstdout->shouldflush = true;
    state->streamstderr = lit_iostream_makeio(stderr, false);
    state->config.desttrace = state->streamstderr;
    lit_init_vm(state);
    lit_api_init(state);
    {
        state->strings.strnull = lit_string_copy(state, "null");
        state->strings.strthis = lit_string_copy(state, "this");
        state->strings.strtostring = lit_string_copy(state, "toString");
        state->strings.strconstructor = lit_string_copy(state, "constructor");
        state->strings.stropequal = lit_string_copy(state, "==");
        state->strings.stropindex = lit_string_copy(state, "[]");
    }
    state->lexer = (LitAstLexer*)lit_sysmem_malloc(sizeof(LitAstLexer));
    state->parser = (LitAstParser*)lit_sysmem_malloc(sizeof(LitAstParser));
    lit_astparser_init(state, (LitAstParser*)state->parser);
    state->emitter = (LitAstEmitter*)lit_sysmem_malloc(sizeof(LitAstEmitter));
    lit_emitter_init(state, state->emitter);

    lit_state_opencorelibrary(state);
    lit_state_openlibraries(state);

    return state;
}

int64_t lit_state_destroy(LitState* state)
{
    if(state->roots != NULL)
    {
        lit_sysmem_free(state->roots);
        state->roots = NULL;
    }
    lit_api_destroy(state);
    lit_iostream_destroy(state->streamstdout);
    lit_iostream_destroy(state->streamstderr);
    if(state->config.havedesttrace)
    {
        lit_iostream_destroy(state->config.desttrace);
    }
    lit_sysmem_free(state->lexer);
    lit_astparser_destroy(state->parser);
    lit_sysmem_free(state->parser);
    lit_emitter_destroy(state->emitter);
    lit_sysmem_free(state->emitter);
    lit_free_vm(state);
    int64_t amount = state->bytes_allocated;
    lit_sysmem_free(state);
    return amount;
}

void lit_state_pushroot(LitState* state, LitObject* object)
{
    lit_state_pushvalueroot(state, lit_value_fromobject(object));
}

void lit_state_pushvalueroot(LitState* state, LitValue value)
{
    if(state->root_count + 1 >= state->root_capacity)
    {
        state->root_capacity = LIT_GROW_CAPACITY(state->root_capacity);
        state->roots = (LitValue*)lit_sysmem_realloc(state->roots, state->root_capacity * sizeof(LitValue));
    }
    state->roots[state->root_count++] = value;
}

LitValue lit_state_peekroot(LitState* state, size_t distance)
{
    assert(state->root_count - distance + 1 > 0);
    return state->roots[state->root_count - distance - 1];
}

void lit_state_poproot(LitState* state)
{
    state->root_count--;
}

void lit_state_poproots(LitState* state, size_t amount)
{
    state->root_count -= amount;
}

LitClass* lit_state_getclassfor(LitState* state, LitValue value)
{
    if(lit_value_isobject(value))
    {
        switch(lit_value_objtype(value))
        {
            case LIT_OBJ_STRING:
                return state->string_class;
            case LIT_OBJ_USERDATA:
                return state->stdobjectclass;
            case LIT_OBJ_FIELD:
            case LIT_OBJ_FUNCSCRIPT:
            case LIT_OBJ_FUNCCLOSURE:
            case LIT_OBJ_CLSPROTOTYPE:
            case LIT_OBJ_FUNCNATIVE:
            case LIT_OBJ_FUNCBOUNDMETHOD:
            case LIT_OBJ_FUNCNATMETHOD:
            {
                return state->function_class;
            }
            case LIT_OBJ_FIBER:
                return state->fiber_class;
            case LIT_OBJ_MODULE:
                return state->module_class;
            case LIT_OBJ_UPVALUE:
            {
                LitUpvalue* upvalue = lit_value_asupvalue(value);
                if(upvalue->location == NULL)
                {
                    return lit_state_getclassfor(state, upvalue->closed);
                }
                return lit_state_getclassfor(state, *upvalue->location);
            }
            case LIT_OBJ_INSTANCE:
                return lit_value_asinstance(value)->klass;
            case LIT_OBJ_CLASS:
                return state->stdclassclass;
            case LIT_OBJ_ARRAY:
            case LIT_OBJ_VARARGARRAY:
                return state->array_class;
            case LIT_OBJ_MAP:
                return state->map_class;
            case LIT_OBJ_RANGE:
                return state->range_class;
            case LIT_OBJ_REFERENCE:
            {
                LitValue* slot = lit_value_asreference(value)->slot;
                if(slot != NULL)
                {
                    return lit_state_getclassfor(state, *slot);
                }
                return state->stdobjectclass;
            }
        }
    }
    else if(lit_value_isnumber(value))
    {
        return state->number_class;
    }
    else if(lit_value_isbool(value))
    {
        return state->bool_class;
    }
    return NULL;
}


LitResult lit_state_interpretsource(LitState* state, const char* modname, const char* code)
{
    return lit_state_interninterpretsource(state, lit_string_copylen(state, modname, strlen(modname)), code);
}

LitModule* lit_state_compilemodulesource(LitState* state, LitString* modname, const char* code)
{
    bool allowedgc = state->gcallowgc;
    state->gcallowgc = false;
    state->had_error = false;
    LitModule* module = NULL;
    /* this is a lbc format */
    if((code[1] << 8 | code[0]) == LIT_CONFIG_BCMAGICNUMBER)
    {
        module = lit_bcemu_initloadmodule(state, code);
    }
    else
    {
        LitDynListExpr statements;
        lit_dynlistexpr_init(&statements);
        if(lit_astparser_parsesource(state->parser, modname->strbuf.data, code, &statements))
        {
            lit_ast_destroyexprlist(state, &statements);
            return NULL;
        }
        if(state->config.dumpast)
        {
            lit_astprint_printbeginlist(stderr, &statements);
            if(state->config.quitafterdump)
            {
                lit_ast_destroyexprlist(state, &statements);
                return NULL;
            }
        }
        module = lit_emitter_emitmod(state->emitter, &statements, modname);
        lit_ast_destroyexprlist(state, &statements);
    }
    state->gcallowgc = allowedgc;
    return state->had_error ? NULL : module;
}

LitResult lit_state_interninterpretsource(LitState* state, LitString* modname, const char* code)
{
    LitModule* module = lit_state_compilemodulesource(state, modname, code);
    if(module == NULL)
    {
        return lit_result_make(LIT_STATUS_COMPILEERROR, lit_value_makenull());
    }
    LitResult result = lit_interpret_module(state, module);
    state->last_module = module;
    return result;
}


bool lit_state_compileandsavefile(LitState* state, const char* inputfile, const char* outputfile)
{
    size_t flen;
    LitModule* compiledmodule;
    (void)flen;
    {
        char* filename = lit_util_dupstring(inputfile);
        char* source = lit_util_readfile(filename, &flen);
        if(source == NULL)
        {
            lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to open file '%s'", filename);
            return false;
        }
        LitString* modname = lit_string_copylen(state, filename, strlen(filename));
        LitModule* module = lit_state_compilemodulesource(state, modname, source);
        compiledmodule = module;
        lit_sysmem_free((void*)source);
        if(module == NULL)
        {
            return false;
        }
    }
    FILE* hnd = fopen(outputfile, "w+b");
    if(hnd == NULL)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to open for writing file '%s'", outputfile);
        return false;
    }
    lit_bcfile_writeuint32(hnd, LIT_CONFIG_BCMAGICNUMBER);
    lit_bcfile_writeuint32(hnd, LIT_BYTECODE_VERSION);
    //lit_bcfile_writeuint16(hnd, 1);
    {
        lit_bcfile_savemodule(compiledmodule, hnd);
    }
    lit_bcfile_writeuint32(hnd, LIT_CONFIG_BCENDNUMBER);
    fclose(hnd);
    return true;
}

char* lit_util_readsource(LitState* state, const char* filename)
{
    size_t flen;
    char* source;
    (void)flen;
    source = lit_util_readfile(filename, &flen);
    if(source == NULL)
    {
        lit_state_raiseerror(state, LIT_ERROR_RUNTIMEERROR, "failed to open file '%s'", filename);
    }
    return source;
}

LitResult lit_state_interpretfile(LitState* state, const char* file)
{
    char* source = lit_util_readsource(state, file);
    if(source == NULL)
    {
        return LIT_STATUS_RUNTIME_FAIL;
    }
    LitResult result = lit_state_interpretsource(state, file, source);
    lit_sysmem_free((void*)source);
    return result;
}

LitResult lit_state_dumpfile(LitState* state, LitIOStream* pr, const char* file)
{
    char* source = lit_util_readsource(state, file);
    if(source == NULL)
    {
        return LIT_STATUS_RUNTIME_FAIL;
    }
    LitResult result;
    LitString* modname = lit_string_copylen(state, file, strlen(file));
    LitModule* module = lit_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        result = LIT_STATUS_RUNTIME_FAIL;
    }
    else
    {
        lit_debug_disasmodule(pr, module, source);
        result = lit_result_make(LIT_STATUS_OK, lit_value_makenull());
    }
    lit_sysmem_free((void*)source);
    return result;
}

void lit_state_raiseerror(LitState* state, LitErrorType type, const char* fmt, ...)
{
    va_list args;
    LitIOStream pr;
    (void)type;
    lit_iostream_makestackstring(&pr);
    va_start(args, fmt);
    lit_iostream_vwritefmt(&pr, fmt, args);
    va_end(args);
    state->printerrmessagefn(state, pr.psbuf.data);
    lit_iostream_destroy(&pr);
}

void lit_vmexec_traceframe(LitState* state, LitFiber* fiber)
{
    #ifdef LIT_CONFIG_TRACESTACK
        bool frisexit;
        size_t frcnt;
        size_t frargc;
        size_t frmaxreg;
        size_t fradded;
        size_t frcap;
        size_t frcap;
        const char* frname;
        LitIOStream* pr;
        LitCallFrame* frame;
    #endif
    (void)state;
    (void)fiber;
    #ifdef LIT_CONFIG_TRACESTACK
        if(fiber == NULL)
        {
            return;
        }
        pr = state->config.desttrace;
        frame = &fiber->framevals[fiber->framecount - 1];
        frcnt = fiber->framecount - 1;
        frname = lit_string_getdata(frame->function->name);
        frargc = frame->function->argcount;
        frmaxreg = frame->function->maxregisters;
        fradded = frmaxreg + (int)(fiber->stack_top - fiber->stack);
        frcap = fiber->stack_capacity;
        frisexit = frame->returnaddress == NULL;
        lit_iostream_printf(pr, "== fiber %p f%i %s (expects %i, max %i, added %i, current %i, exits %i) ==\n", fiber, frcnt, frname, frargc, frmaxreg, fradded, frcap, frisexit);
    #endif
}

void lit_debug_traceprintvalue(LitIOStream* pr, const char* prefix, size_t framecount, size_t argc, LitValue* vals)
{
    size_t i;
    lit_iostream_printf(pr, "-> f%ld %s{\n", framecount, prefix);
    for(i = 0; i <= argc; i++)
    {
        lit_iostream_printf(pr, "  [%ld]: ", i);
        lit_value_printvalue(pr, *(vals + i), true);
        lit_iostream_printf(pr, "\n");
    }
    lit_iostream_printf(pr, "}\n");
}

void lit_vmexec_resetvm(LitState* state)
{
    state->vmstate.objects = NULL;
    state->vmstate.fiber = NULL;
    state->vmstate.gcgraystack = NULL;
    state->vmstate.gcgraycount = 0;
    state->vmstate.gcgraycapacity = 0;
    lit_table_init(state, &state->vmstate.strings);
    state->vmstate.globals = NULL;
    state->vmstate.modules = NULL;
}

void lit_init_vm(LitState* state)
{
    lit_vmexec_resetvm(state);
    state->vmstate.globals = lit_object_makemap(state, NULL);
    state->vmstate.modules = lit_object_makemap(state, NULL);
}

void lit_free_vm(LitState* state)
{
    lit_free_table(&state->vmstate.strings);
    lit_free_objects(state, state->vmstate.objects);
    lit_vmexec_resetvm(state);
}

bool lit_vm_handleerror(LitState* state, LitString* errorstring)
{
    int i;
    LitIOStream* pr;
    pr = state->streamstderr;
    LitValue error = lit_value_fromobject(errorstring);
    LitFiber* fiber = state->vmstate.fiber;
    while(fiber != NULL)
    {
        fiber->error = error;
        if(fiber->handler_count > 0)
        {
            LitHandler* handler = &fiber->handlers[--fiber->handler_count];
            fiber->framecount = handler->frame_count;
            state->vmstate.frame = &fiber->framevals[fiber->framecount - 1];
            state->vmstate.currentchunk = &state->vmstate.frame->function->chunk;
            state->vmstate.ip = handler->handler_ip;
            state->vmstate.vmregisteritems = fiber->registeritems + handler->register_count;
            state->vmstate.vmregisteritems[handler->error_reg] = error;
            return true;
        }
        if(fiber->catcher)
        {
            fiber->caught = true;
            state->vmstate.fiber = fiber->parent;
            if(state->vmstate.fiber->returnaddress != NULL)
            {
                *state->vmstate.fiber->returnaddress = error;
            }
            return true;
        }
        LitFiber* caller = fiber->parent;
        fiber->parent = NULL;
        fiber = caller;
    }
    fiber = state->vmstate.fiber;
    fiber->abort = true;
    fiber->error = error;
    if(fiber->parent != NULL)
    {
        fiber->parent->abort = true;
    }
    int count = (int)fiber->framecount - 1;
    lit_iostream_writecolor(pr, COLOR_RED);
    lit_iostream_printf(pr, "unhandled error in vm:\n");
    lit_iostream_printf(pr, "%s\n", errorstring->strbuf.data);
    for(i = count; i >= 0; i--)
    {
        LitCallFrame* frame = &fiber->framevals[i];
        LitFuncScript* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->strbuf.data;
        if(chunk->haslineinfo)
        {
            size_t line = lit_chunk_getline(chunk, frame->ip - chunk->compiledcodechunk - 1);
            lit_iostream_printf(pr, "  [line %ld] in %s()\n", line, name);
        }
        else
        {
            lit_iostream_printf(pr, "\tin %s()\n", name);
        }
    }
    lit_iostream_writecolor(pr, COLOR_RESET);
    state->printerrmessagefn(state, NULL);
    lit_iostream_destroy(pr);
    return false;
}

bool lit_vm_raiseerrorva(LitState* state, const char* format, va_list args)
{
    LitIOStream pr;
    LitString* str;
    lit_iostream_makestackstring(&pr);
    lit_iostream_vwritefmt(&pr, format, args);
    str = lit_iostream_takestring(state, &pr);
    return lit_vm_handleerror(state, str);
}

bool lit_vm_raiseerror(LitState* state, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = lit_vm_raiseerrorva(state, format, args);
    va_end(args);
    return result;
}

bool lit_vm_raisefatalerror(LitState* state, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = lit_vm_raiseerrorva(state, format, args);
    va_end(args);
    lit_state_nativeexitjump();
    return result;
}

bool lit_vmexec_callcallable(LitState* state, LitFuncScript* function, LitFuncClosure* closure, size_t argc, size_t calleeregister)
{
    size_t i;
    LitFiber* fiber = state->vmstate.fiber;
    assert(fiber->framecount > 0);
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        size_t newcapacity = ((fiber->framecapacity + 1) * 2);
        fiber->framevals = (LitCallFrame*)lit_sysmem_realloc(fiber->framevals, sizeof(LitCallFrame) * newcapacity);
        fiber->framecapacity = newcapacity;
    }
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    LitCallFrame* previousframe = &fiber->framevals[fiber->framecount - 2];
    frame->function = function;
    frame->closure = closure;
    frame->ip = function->chunk.compiledcodechunk;
    frame->slots = previousframe->slots + calleeregister;
    frame->resultignored = false;
    frame->returntoc = false;
    frame->returnaddress = previousframe->slots + (int)calleeregister;
    lit_fiber_ensureregisters(fiber, frame->slots - fiber->registeritems + function->maxregisters);
    size_t targetargcount = function->argcount;
    bool vararg = function->vararg;
    if(targetargcount > argc)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("filling with nulls\n");
#endif
        for(i = argc; i < targetargcount; i++)
        {
            *(frame->slots + i + 1) = lit_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = lit_value_fromobject(lit_array_make(state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && lit_value_isvargarray(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            LitArray* array = &lit_object_makevararray(state)->innerarray;
            lit_state_pushroot(state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->innerlist, argc - targetargcount + 1);
            size_t j = 0;
            for(i = targetargcount - 1; i < argc; i++)
            {
                array->innerlist.listitems[j++] = *(frame->slots + i + 1);
            }
            *(frame->slots + targetargcount) = lit_value_fromobject(array);
            lit_state_poproot(state);
        }
    }
    return true;
}

bool lit_vmexec_actualcallvalue(LitState* state, size_t calleeregister, size_t argc, const LitValue alternatecallee)
{
    LitCallFrame* frame = &state->vmstate.fiber->framevals[state->vmstate.fiber->framecount - 1];
    LitValue callee = lit_value_isnull(alternatecallee) ? frame->slots[calleeregister] : alternatecallee;
    if(lit_value_isobject(callee))
    {
        if(lit_set_native_exit_jump())
        {
            bool caught = state->vmstate.fiber->caught;
            state->vmstate.fiber->caught = false;
            return !caught;
        }
        switch(lit_value_objtype(callee))
        {
            case LIT_OBJ_FUNCSCRIPT:
            {
                return lit_vmexec_callcallable(state, lit_value_asfuncscript(callee), NULL, argc, calleeregister);
            }
            case LIT_OBJ_FUNCCLOSURE:
            {
                LitFuncClosure* closure = lit_value_asfuncclosure(callee);
                return lit_vmexec_callcallable(state, closure->function, closure, argc, calleeregister);
            }
            case LIT_OBJ_FUNCNATIVE:
            {
                LitValue value = lit_value_asfuncnative(callee)->natfuncptr(state, lit_value_makenull(), argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                return !state->vmstate.fiber->abort;
            }
            case LIT_OBJ_FUNCNATMETHOD:
            {
                lit_vmexec_pushgc(state, false);
                LitFuncNative* method = lit_value_asfuncmethod(callee);
                LitFiber* fiber = state->vmstate.fiber;
                LitValue value = method->natfuncptr(state, *(frame->slots + calleeregister), argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                lit_vmexec_popgc(state);
                return !fiber->abort;
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = lit_value_asclass(callee);
                LitInstance* instance = lit_object_makeinstance(state, klass);
                frame->slots[calleeregister] = lit_value_fromobject(instance);
                if(klass->mthconstructor != NULL)
                {
                    return lit_vmexec_actualcallvalue(state, calleeregister, argc, lit_value_fromobject(klass->mthconstructor));
                }
                return true;
            }
            case LIT_OBJ_FUNCBOUNDMETHOD:
            {
                LitFuncBound* boundmethod = lit_value_asfuncboundmethod(callee);
                LitValue method = boundmethod->method;
                if(lit_value_isfuncmethod(method))
                {
                    lit_vmexec_pushgc(state, false);
                    LitValue value = lit_value_asfuncmethod(method)->natfuncptr(state, boundmethod->receiver, argc, frame->slots + calleeregister + 1);
                    frame->slots[calleeregister] = value;
                    lit_vmexec_popgc(state);
                    return !state->vmstate.fiber->abort;
                }
                else
                {
                    frame->slots[calleeregister] = boundmethod->receiver;
                    return lit_vmexec_callcallable(state, lit_value_asfuncscript(method), NULL, argc, calleeregister);
                }
                return !state->vmstate.fiber->abort;
            }
            default:
            {
                break;
            }
        }
    }
    if(lit_value_isnull(callee))
    {
        return lit_vm_raiseerror(state, "attempt to call a null value");
    }
    else
    {
        return lit_vm_raiseerror(state, "can only call functions and classes, got %s", lit_value_valtypename(callee));
    }
    return true;
}

LitUpvalue* lit_vmexec_captureupvalue(LitState* state, LitValue* local)
{
    LitUpvalue* previousupvalue = NULL;
    LitUpvalue* upvalue = state->vmstate.fiber->openupvalues;
    while(upvalue != NULL && upvalue->location > local)
    {
        previousupvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    LitUpvalue* createdupvalue = lit_object_makeupvalue(state, local);
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

void lit_vmexec_closeupvalues(LitState* state, const LitValue* last)
{
    LitFiber* fiber = state->vmstate.fiber;
    while(fiber->openupvalues != NULL && fiber->openupvalues->location >= last)
    {
        LitUpvalue* upvalue = fiber->openupvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        fiber->openupvalues = upvalue->next;
    }
}

LitResult lit_interpret_module(LitState* state, LitModule* module)
{
    LitFiber* fiber = lit_object_makefiber(state, module, module->mainfunction);
    state->vmstate.fiber = fiber;
    LitResult result = lit_state_execfiber(state, fiber);
    return result;
}

#define LIT_CONF_USECOMPUTEDGOTO 0

#if defined(__CPPCHECK__) || (!defined(__GNUC__))
    #define LIT_CONF_USECOMPUTEDGOTO 0
#endif

#define lit_vmmac_dispatchnext() goto dispatch;

#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    #define LABELNAME(nm) label_##nm
    #define CASE_CODE(name) label_##name:
#else
    #define CASE_CODE(name) case name:
#endif

LIT_INLINE void lit_vmmac_readframe(LitState* state, LitFiber** destfiber)
{
    *destfiber = state->vmstate.fiber;
    state->vmstate.frame = &(*destfiber)->framevals[(*destfiber)->framecount - 1];
    state->vmstate.currentchunk = &state->vmstate.frame->function->chunk;
    state->vmstate.vmconstantvalues = state->vmstate.currentchunk->constantlist.listitems;
    state->vmstate.ip = state->vmstate.frame->ip;
    (*destfiber)->module = state->vmstate.frame->function->module;
    state->vmstate.vmregisteritems = state->vmstate.frame->slots;
    state->vmstate.vmprivatevalues = (*destfiber)->module->privatevalues;
    state->vmstate.upvalues = state->vmstate.frame->closure == NULL ? NULL : state->vmstate.frame->closure->upvalues;
}

LIT_INLINE void lit_vmmac_writeframe(LitState* state)
{
    state->vmstate.frame->ip = state->vmstate.ip;
}

LIT_INLINE bool lit_vmmac_recoverstate(LitState* state, LitFiber** fiber, LitResult* result)
{
    lit_vmmac_writeframe(state);
    (*fiber) = state->vmstate.fiber;
    if((*fiber) == NULL)
    {
        *result = lit_result_make(LIT_STATUS_OK, lit_value_makenull());
        return false;
    }
    if((*fiber)->abort)
    {
        lit_vmexec_popgc(state);
        *result = lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());
        return false;
    }
    lit_vmmac_readframe(state, fiber);
    if(state->config.traceexecution)
    {
        lit_vmexec_traceframe(state, *fiber);
    }
    return true;
}

LIT_INLINE LitValue lit_vmmac_getrc(LitState* state, int64_t r)
{
    if(LIT_BIT_ISSET(r, LIT_BITFLAG_CONSTANT))
    {
        return state->vmstate.vmconstantvalues[r & ~(1UL << LIT_BITFLAG_CONSTANT)];
    }
    return state->vmstate.vmregisteritems[r];
}

LIT_INLINE bool lit_vmmac_callvalue(LitState* state, LitFiber** fiber, LitValue callee, size_t reg, size_t argc, LitResult* res)
{
    if(!lit_vmexec_actualcallvalue(state, reg, argc, callee))
    {
        if(!lit_vmmac_recoverstate(state, fiber, res))
        {
            return false;
        }
    }
    return true;
}

#define lit_vmmac_fail(state, ...) \
    { \
        LitResult tmprecoverres; \
        if(lit_vm_raiseerror(state, __VA_ARGS__)) \
        { \
            if(!lit_vmmac_recoverstate(state, &fiber, &tmprecoverres)) \
            { \
                return tmprecoverres; \
            } \
            lit_vmmac_dispatchnext(); \
        } \
        else \
        { \
            lit_vmexec_popgc(state); \
            return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull()); \
        } \
    }

LIT_INLINE LitResult lit_vmmac_invokeoperatormethoddefault(LitState* state, LitFiber** fiber, size_t reg, LitValue bv, const char* m, size_t argc)
{
    LitValue method;
    LitResult tmpres;
    LitString* mthname;
    LitClass* klass;
    lit_vmmac_writeframe(state);
    klass = lit_state_getclassfor(state, bv);
    if(klass == NULL)
    {
        if(lit_vm_raiseerror(state, "use of method '%s' on a null value", m))
        {
            if(!lit_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return lit_result_make(LIT_STATUS_INVALID, lit_value_makenull());
        }
        else
        {
            lit_vmexec_popgc(state);
            return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());
        }
    }
    mthname = lit_string_copy(state, m);
    if((lit_value_isinstance(bv) && (lit_table_getentry(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_getentry(&klass->mthtable, mthname, &method))
    {
        if(!lit_vmmac_callvalue(state, fiber, method, reg, argc, &tmpres))
        {
            return tmpres;
        }
    }
    else
    {
        if(lit_vm_raiseerror(state, "attempt to invoke undefined operator method '%s#operator %s'", klass->name->strbuf.data, mthname->strbuf.data))
        {
            if(!lit_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return lit_result_make(LIT_STATUS_INVALID, lit_value_makenull());
        }
        else
        {
            lit_vmexec_popgc(state);
            return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());
        }
    }
    lit_vmmac_readframe(state, fiber);
    return lit_result_make(LIT_STATUS_OK, lit_value_makenull());
}

LIT_INLINE LitResult lit_vmmac_invokeoperatormethodandcontinue(LitState* state, LitFiber** fiber, size_t reg, LitValue bv, const char* m, size_t argc)
{
    LitResult tmpres;
    LitResult invmcres;
    lit_vmmac_writeframe(state);
    LitClass* klass = lit_state_getclassfor(state, bv);
    if(klass == NULL)
    {
        if(lit_vm_raiseerror(state, "only instances and classes have methods"))
        {
            if(!lit_vmmac_recoverstate(state, fiber, &tmpres))
            {
                return tmpres;
            }
            return lit_result_make(LIT_STATUS_INVALID, lit_value_makenull());
        }
        else
        {
            lit_vmexec_popgc(state);
            return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());
        }
    }
    LitString* mthname = lit_string_copy(state, m);
    LitValue method;
    if((lit_value_isinstance(bv) && (lit_table_getentry(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_getentry(&klass->mthtable, mthname, &method))
    {
        if(!lit_vmmac_callvalue(state, fiber, method, reg, argc, &invmcres))
        {
           return invmcres;
        }
        lit_vmmac_readframe(state, fiber);
        return lit_result_make(LIT_STATUS_INVALID, lit_value_makenull());
    }
    return lit_result_make(LIT_STATUS_OK, lit_value_makenull());
}

#define lit_vmmac_dobinaryop(state, fiber, typefn, opcode, opstring) \
    LitValue cv; \
    LitValue bv; \
    LitValue res; \
    LitValue tmpb; \
    LitValue tmpval; \
    LitResult invres; \
    bool isinst; \
    double dnbv; \
    double dncv; \
    uint64_t ra; \
    uint64_t rb; \
    uint64_t rc; \
    const char* snbv; \
    const char* sncv; \
    ra = LIT_INST_GETA(state->vmstate.instruction); \
    rb = LIT_INST_GETB(state->vmstate.instruction); \
    rc = LIT_INST_GETC(state->vmstate.instruction); \
    bv = lit_vmmac_getrc(state, rb); \
    cv = lit_vmmac_getrc(state, rc); \
    if(lit_value_isnumber(bv) && !lit_value_isnumber(cv)) \
    { \
        snbv = lit_value_valtypename(bv); \
        sncv = lit_value_valtypename(cv); \
        lit_vmmac_fail(state, "attempt to use the operator %s with a %s and a %s", opstring, snbv, sncv); \
    } \
    isinst = (lit_value_isinstance(bv) && lit_value_asinstance(bv)->klass == state->number_class); \
    if(lit_value_isnumber(bv) || isinst) \
    { \
        if(isinst) \
        { \
            tmpval = lit_instance_getthis(lit_value_asinstance(bv)); \
            if(lit_value_isnull(tmpval)) \
            { \
                lit_vmmac_fail(state, "failed to extract 'this' value from Number instance"); \
            } \
            dnbv = lit_value_asnumber(tmpval); \
        } \
        else \
        { \
            dnbv = lit_value_asnumber(bv); \
        } \
        dncv = lit_value_asnumber(cv); \
        res = lit_value_makenull(); \
        switch(opcode) \
        { \
            case LIT_OPCODE_MATHADD: \
                { \
                    res = typefn(dnbv + dncv); \
                } \
                break; \
            case LIT_OPCODE_MATHSUBTRACT: \
                { \
                    res = typefn(dnbv - dncv); \
                } \
                break; \
            case LIT_OPCODE_MATHMULTIPLY: \
                { \
                    res = typefn(dnbv * dncv); \
                } \
                break; \
            case LIT_OPCODE_MATHDIVIDE: \
                { \
                    res = typefn(dnbv / dncv); \
                } \
                break; \
            default: \
                { \
                    lit_vmmac_fail(state, "INTERNAL ERROR: no case clause for op %d (%s)", opcode, opstring); \
                } \
                break; \
        } \
        state->vmstate.vmregisteritems[ra] = res; \
    } \
    else \
    { \
        if(lit_value_isnull(bv)) \
        { \
            lit_vmmac_fail(state, "attempt to use the operator %s on a null value", opstring); \
        } \
        state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb); \
        tmpb = state->vmstate.vmregisteritems[ra + 1]; \
        state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc); \
        invres = lit_vmmac_invokeoperatormethoddefault(state, (fiber), ra, state->vmstate.vmregisteritems[ra], opstring, 1); \
        if(invres.type != LIT_STATUS_OK) \
        { \
            if(invres.type == LIT_STATUS_INVALID) \
            { \
                lit_vmmac_dispatchnext(); \
            } \
            return invres; \
        } \
        state->vmstate.vmregisteritems[ra + 1] = tmpb; \
    }

#define lit_vmmac_docomparisonop(typefn, opcode, opstring) \
    LitValue cv; \
    LitValue bv; \
    LitValue res; \
    LitValue tmpb; \
    LitResult invres; \
    uint64_t ra; \
    uint64_t rc; \
    uint64_t rb; \
    const char* snbv; \
    const char* sncv; \
    ra = LIT_INST_GETA(state->vmstate.instruction); \
    rb = LIT_INST_GETB(state->vmstate.instruction); \
    rc = LIT_INST_GETC(state->vmstate.instruction); \
    bv = lit_vmmac_getrc(state, rb); \
    cv = lit_vmmac_getrc(state, rc); \
    if(lit_value_isnumber(bv)) \
    { \
        if(!lit_value_isnumber(cv)) \
        { \
            snbv = lit_value_valtypename(bv); \
            sncv = lit_value_valtypename(cv); \
            lit_vmmac_fail(state, "attempt to use the operator %s with a %s and a %s", opstring, snbv, sncv); \
        } \
        res = lit_value_makenull();\
        switch(opcode) \
        { \
            case LIT_OPCODE_LESSTHAN: \
                { \
                    res = typefn(lit_value_asnumber(bv) < lit_value_asnumber(cv)); \
                } \
                break; \
            case LIT_OPCODE_LESSEQUAL: \
                { \
                    res = typefn(lit_value_asnumber(bv) <= lit_value_asnumber(cv)); \
                } \
                break; \
            case LIT_OPCODE_GREATERTHAN: \
                { \
                    res = typefn(lit_value_asnumber(bv) > lit_value_asnumber(cv)); \
                } \
                break; \
            case LIT_OPCODE_GREATEREQUAL: \
                { \
                    res = typefn(lit_value_asnumber(bv) >= lit_value_asnumber(cv)); \
                } \
                break; \
            default: \
                { \
                    lit_vmmac_fail(state, "INTERNAL ERROR: no case clause for op %d (%s)", opcode, opstring); \
                } \
                break; \
        } \
        state->vmstate.vmregisteritems[ra] = res; \
    } \
    else if(lit_value_isnull(bv)) \
    { \
        lit_vmmac_fail(state, "attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb); \
        tmpb = state->vmstate.vmregisteritems[ra + 1]; \
        state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc); \
        invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], opstring, 1); \
        if(invres.type != LIT_STATUS_OK) \
        { \
            if(invres.type == LIT_STATUS_INVALID) \
            { \
                lit_vmmac_dispatchnext(); \
            } \
            return invres; \
        } \
        state->vmstate.vmregisteritems[ra + 1] = tmpb; \
    }

LitResult lit_state_execfiber(LitState* state, LitFiber* fiber)
{
    assert(fiber->framecount > 0);
    state->vmstate.fiber = fiber;

#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    static void* dispatchtable[] = {
        &&LABELNAME(LIT_OPCODE_MOVE),
        &&LABELNAME(LIT_OPCODE_LOADNULL),
        &&LABELNAME(LIT_OPCODE_LOADBOOL),
        &&LABELNAME(LIT_OPCODE_MAKECLOSURE),
        &&LABELNAME(LIT_OPCODE_MAKEARRAY),
        &&LABELNAME(LIT_OPCODE_MAKEOBJECT),
        &&LABELNAME(LIT_OPCODE_MAKERANGE),
        &&LABELNAME(LIT_OPCODE_RETURN),
        &&LABELNAME(LIT_OPCODE_MATHADD),
        &&LABELNAME(LIT_OPCODE_MATHSUBTRACT),
        &&LABELNAME(LIT_OPCODE_MATHMULTIPLY),
        &&LABELNAME(LIT_OPCODE_MATHDIVIDE),
        &&LABELNAME(LIT_OPCODE_MATHFLOORDIVIDE),
        &&LABELNAME(LIT_OPCODE_MATHMOD),
        &&LABELNAME(LIT_OPCODE_MATHPOWER),
        &&LABELNAME(LIT_OPCODE_MATHLEFTSHIFT),
        &&LABELNAME(LIT_OPCODE_MATHRIGHTSHIFT),
        &&LABELNAME(LIT_OPCODE_BINXOR),
        &&LABELNAME(LIT_OPCODE_BINAND),
        &&LABELNAME(LIT_OPCODE_BINOR),
        &&LABELNAME(LIT_OPCODE_JUMP),
        &&LABELNAME(LIT_OPCODE_JUMPIFTRUE),
        &&LABELNAME(LIT_OPCODE_JUMPIFFALSE),
        &&LABELNAME(LIT_OPCODE_JUMPIFNONNULL),
        &&LABELNAME(LIT_OPCODE_JUMPIFNULL),
        &&LABELNAME(LIT_OPCODE_EQUAL),
        &&LABELNAME(LIT_OPCODE_LESSTHAN),
        &&LABELNAME(LIT_OPCODE_LESSEQUAL),
        &&LABELNAME(LIT_OPCODE_GREATERTHAN),
        &&LABELNAME(LIT_OPCODE_GREATEREQUAL),
        &&LABELNAME(LIT_OPCODE_NEGATE),
        &&LABELNAME(LIT_OPCODE_NOT),
        &&LABELNAME(LIT_OPCODE_BINNOT),
        &&LABELNAME(LIT_OPCODE_GLOBALSET),
        &&LABELNAME(LIT_OPCODE_GLOBALGET),
        &&LABELNAME(LIT_OPCODE_UPVALUESET),
        &&LABELNAME(LIT_OPCODE_UPVALUEGET),
        &&LABELNAME(LIT_OPCODE_PRIVATESET),
        &&LABELNAME(LIT_OPCODE_PRIVATEGET),
        &&LABELNAME(LIT_OPCODE_CALLCALLABLE),
        &&LABELNAME(LIT_OPCODE_UPVALUECLOSE),
        &&LABELNAME(LIT_OPCODE_CLASSMAKE),
        &&LABELNAME(LIT_OPCODE_CLASSPUTFIELDSTATIC),
        &&LABELNAME(LIT_OPCODE_CLASSPUTMETHOD),
        &&LABELNAME(LIT_OPCODE_FIELDGET),
        &&LABELNAME(LIT_OPCODE_CLASSGETSUPERMETHOD),
        &&LABELNAME(LIT_OPCODE_FIELDSET),
        &&LABELNAME(LIT_OPCODE_IS),
        &&LABELNAME(LIT_OPCODE_INVOKE),
        &&LABELNAME(LIT_OPCODE_INVOKESUPER),
        &&LABELNAME(LIT_OPCODE_SUBSCRIPTGET),
        &&LABELNAME(LIT_OPCODE_SUBSCRIPTSET),
        &&LABELNAME(LIT_OPCODE_ARRAYPUSH),
        &&LABELNAME(LIT_OPCODE_OBJECTPUSH),
        &&LABELNAME(LIT_OPCODE_REFGLOBAL),
        &&LABELNAME(LIT_OPCODE_REFPRIVATE),
        &&LABELNAME(LIT_OPCODE_REFLOCAL),
        &&LABELNAME(LIT_OPCODE_REFUPVALUE),
        &&LABELNAME(LIT_OPCODE_REFFIELD),
        &&LABELNAME(LIT_OPCODE_REFSET),
        &&LABELNAME(LIT_OPCODE_PUSH_TRY),
        &&LABELNAME(LIT_OPCODE_POP_TRY),
        &&LABELNAME(LIT_OPCODE_THROW),
        &&LABELNAME(LIT_OPCODE_RETHROW),
    };
#endif
    bool traceforcenl;
    size_t traceofs;
    size_t tracemaxreg;
    LitCallFrame* previousframe;
    LitTable* globals;
    globals = &state->vmstate.globals->innertable;
    lit_vmexec_pushgc(state, true) fiber->abort = false;
    lit_vmmac_readframe(state, &fiber);
    state->vmstate.fiber = fiber;
    state->vmstate.vmregisteritems[0] = lit_value_fromobject(state->vmstate.frame->function);
    if(LIT_UNLIKELY(state->config.traceexecution))
    {
        lit_vmexec_traceframe(state, fiber);
        lit_iostream_printf(state->config.desttrace, "fiber start:\n");
    }

dispatch:
    state->vmstate.instruction = *state->vmstate.ip++;
    if(LIT_UNLIKELY(state->config.traceexecution))
    {
        previousframe = state->vmstate.frame;
        traceofs = (size_t)(state->vmstate.ip - state->vmstate.currentchunk->compiledcodechunk - 1);
        traceforcenl = state->vmstate.frame != previousframe;
        lit_debug_disasinstr(state->config.desttrace, state->vmstate.currentchunk, traceofs, NULL, traceforcenl);
        if(!state->config.traceinstsonly)
        {
            if(LIT_LIKELY(state->vmstate.frame->function->maxregisters > 0))
            {
                tracemaxreg = state->vmstate.frame->function->maxregisters;
                lit_debug_traceprintvalue(state->config.desttrace, "<vm:registers>", fiber->framecount, tracemaxreg, state->vmstate.vmregisteritems);
            }
        }
        previousframe = state->vmstate.frame;
    }
#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    goto* dispatchtable[LIT_INST_GETOPCODE(state->vmstate.instruction)];
#else
    switch(LIT_INST_GETOPCODE(state->vmstate.instruction))
#endif
    {
        CASE_CODE(LIT_OPCODE_MOVE)
        {
            uint64_t ra;
            uint64_t rb;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb);
            lit_vmmac_dispatchnext()
        }
        CASE_CODE(LIT_OPCODE_LOADNULL)
        {
            uint64_t ra;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_makenull();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_LOADBOOL)
        {
            uint64_t ra;
            uint64_t rb;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_makebool(rb != 0);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MAKECLOSURE)
        {
            size_t i;
            uint64_t index;
            uint64_t rbx;
            uint64_t ra;
            LitFuncClosure* closure;
            LitClsPrototype* clsproto;
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            ra = LIT_INST_GETA(state->vmstate.instruction);
            clsproto = lit_value_asclsproto(state->vmstate.vmconstantvalues[rbx]);
            closure = lit_object_makeclosure(state, clsproto->function);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(closure);
            for(i = 0; i < closure->function->upvaluecount; i++)
            {
                index = clsproto->indexes[i];
                if(clsproto->local[i])
                {
                    closure->upvalues[i] = lit_vmexec_captureupvalue(state, state->vmstate.vmregisteritems + index);
                }
                else
                {
                    closure->upvalues[i] = state->vmstate.upvalues[index];
                }
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MAKEARRAY)
        {
            size_t sz;
            uint64_t ra;
            uint64_t rb;
            LitArray* array;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            array = lit_array_make(state);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(array);
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
            lit_dynlistval_ensureactualsize(&array->innerlist, sz);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MAKEOBJECT)
        {
            uint64_t ra;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(lit_object_makeinstance(state, state->stdobjectclass));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MAKERANGE)
        {
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            LitValue first;
            LitValue second;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            rc = LIT_INST_GETC(state->vmstate.instruction);
            first = lit_vmmac_getrc(state, rb);
            second = lit_vmmac_getrc(state, rc);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(lit_object_makerange(state, lit_value_asnumber(first), lit_value_asnumber(second)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_RETURN)
        {
            LitValue value;
            while(fiber->handler_count > 0 && fiber->handlers[fiber->handler_count - 1].frame_count >= fiber->framecount)
            {
                fiber->handler_count--;
            }
            value = state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)];
            lit_vmexec_closeupvalues(state, state->vmstate.vmregisteritems);
            fiber->framecount--;
            if(state->vmstate.frame->returntoc)
            {
                state->vmstate.frame->returntoc = false;
                fiber->module->returnvalue = value;
                return lit_result_make(LIT_STATUS_OK, value);
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
                    if(LIT_UNLIKELY(state->config.traceexecution))
                    {
                        lit_iostream_printf(state->config.desttrace, "fiber continue:\n");
                    }
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                return lit_result_make(LIT_STATUS_OK, value);
            }
            *state->vmstate.frame->returnaddress = value;
            lit_vmmac_readframe(state, &fiber);
            if(state->config.traceexecution)
            {
                lit_vmexec_traceframe(state, fiber);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHADD)
        {
            lit_vmmac_dobinaryop(state, &fiber, lit_value_makenumber, LIT_OPCODE_MATHADD, "+");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHSUBTRACT)
        {
            lit_vmmac_dobinaryop(state, &fiber, lit_value_makenumber, LIT_OPCODE_MATHSUBTRACT, "-");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHMULTIPLY)
        {
            lit_vmmac_dobinaryop(state, &fiber, lit_value_makenumber, LIT_OPCODE_MATHMULTIPLY, "*");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHDIVIDE)
        {
            lit_vmmac_dobinaryop(state, &fiber, lit_value_makenumber, LIT_OPCODE_MATHDIVIDE, "/");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHFLOORDIVIDE)
        {
            uint64_t ra = LIT_INST_GETA(state->vmstate.instruction);
            uint64_t rb = LIT_INST_GETB(state->vmstate.instruction);
            uint64_t rc = LIT_INST_GETC(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, rb);
            LitValue cv = lit_vmmac_getrc(state, rc);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[ra] = lit_value_makenumber(floor(lit_value_asnumber(bv) / lit_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb);
                LitValue tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc);
                {
                    LitResult invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], "#", 1);
                    if(invres.type != LIT_STATUS_OK)
                    {
                        if(invres.type == LIT_STATUS_INVALID)
                        {
                            lit_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHMOD)
        {
            LitValue bv;
            LitValue cv;
            LitValue res;
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            int64_t nintbv;
            int64_t nintbc;
            double nddbv;
            double nddbc;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            rc = LIT_INST_GETC(state->vmstate.instruction);
            bv = lit_vmmac_getrc(state, rb);
            cv = lit_vmmac_getrc(state, rc);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                nddbv = lit_value_asnumber(bv);
                nddbc = lit_value_asnumber(cv);
                nintbv = ((int64_t)nddbv);
                nintbc = ((int64_t)nddbc);
                if((nintbv == nddbv) && (nintbc == nddbc))
                {
                    res = lit_value_makenumber(nintbv % nintbc);
                }
                else
                {
                    res = lit_value_makenumber(fmod(nddbv, nddbc));
                }
                state->vmstate.vmregisteritems[ra] = res;
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb);
                LitValue tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc);
                {
                    LitResult invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], "%", 1);
                    if(invres.type != LIT_STATUS_OK)
                    {
                        if(invres.type == LIT_STATUS_INVALID)
                        {
                            lit_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHPOWER)
        {
            uint64_t ra = LIT_INST_GETA(state->vmstate.instruction);
            uint64_t rb = LIT_INST_GETB(state->vmstate.instruction);
            uint64_t rc = LIT_INST_GETC(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, rb);
            LitValue cv = lit_vmmac_getrc(state, rc);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[ra] = lit_value_makenumber(pow(lit_value_asnumber(bv), lit_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb);
                LitValue tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc);
                {
                    LitResult invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, state->vmstate.vmregisteritems[ra], "**", 1);
                    if(invres.type != LIT_STATUS_OK)
                    {
                        if(invres.type == LIT_STATUS_INVALID)
                        {
                            lit_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHLEFTSHIFT)
        {
            LitValue res;
            uint32_t ivbv;
            uint32_t ivbc;
            LitValue bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail(state, "operands of '%s' must be two numbers, got %s and %s", "<<", lit_value_valtypename(bv), lit_value_valtypename(cv));
            }
            ivbv = (uint32_t)lit_value_asnumber(bv);
            ivbc = (uint32_t)lit_value_asnumber(cv);
            {
                res = lit_value_makenumber(ivbv << ivbc);
            }
            state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = res;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_MATHRIGHTSHIFT)
        {
            LitValue res;
            uint32_t ivbv;
            uint32_t ivbc;
            LitValue bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail(state, "operands of '%s' must be two numbers, got %s and %s", ">>", lit_value_valtypename(bv), lit_value_valtypename(cv));
            }
            ivbv = (uint32_t)lit_value_asnumber(bv);
            ivbc = (uint32_t)lit_value_asnumber(cv);
            {
                res = lit_value_makenumber(ivbv >> ivbc);
            }
            state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = res;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_BINXOR)
        {
            uint64_t ra;
            uint32_t nbv;
            uint32_t ncv;
            LitValue bv;
            LitValue cv;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            cv = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "^", lit_value_valtypename(bv), lit_value_valtypename(cv));
            }
            nbv = (uint32_t)lit_value_asnumber(bv);
            ncv = (uint32_t)lit_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (lit_value_makenumber(nbv ^ ncv));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_BINAND)
        {
            uint64_t ra;
            uint32_t nbv;
            uint32_t ncv;
            LitValue bv;
            LitValue cv;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            cv = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "&", lit_value_valtypename(bv), lit_value_valtypename(cv));
            }
            nbv = (uint32_t)lit_value_asnumber(bv);
            ncv = (uint32_t)lit_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (lit_value_makenumber(nbv & ncv));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_BINOR)
        {
            uint64_t ra;
            uint32_t nbv;
            uint32_t ncv;
            LitValue bv;
            LitValue cv;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            cv = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail(state, "operator '%s' cannot be used with %s and %s", "|", lit_value_valtypename(bv), lit_value_valtypename(cv));
            }
            nbv = (uint32_t)lit_value_asnumber(bv);
            ncv = (uint32_t)lit_value_asnumber(cv);
            state->vmstate.vmregisteritems[ra] = (lit_value_makenumber(nbv | ncv));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_JUMP)
        {
            state->vmstate.ip += LIT_INST_GETSBX(state->vmstate.instruction);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_JUMPIFTRUE)
        {
            if(!lit_is_falsey(state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INST_GETBX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_JUMPIFFALSE)
        {
            if(lit_is_falsey(state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INST_GETBX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_JUMPIFNONNULL)
        {
            if(!lit_value_isnull(state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INST_GETBX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_JUMPIFNULL)
        {
            uint64_t ra;
            uint64_t rbx;
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            ra = LIT_INST_GETA(state->vmstate.instruction);
            if(lit_value_isnull(state->vmstate.vmregisteritems[ra]))
            {
                state->vmstate.ip += rbx;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_EQUAL)
        {
            LitValue bv;
            LitValue ptmp;
            LitValue tmpb;
            uint64_t ra;
            uint64_t rb;
            uint64_t rc;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            rc = LIT_INST_GETC(state->vmstate.instruction);
            bv = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            if(lit_value_isinstance(bv))
            {
                state->vmstate.vmregisteritems[ra] = lit_vmmac_getrc(state, rb);
                tmpb = state->vmstate.vmregisteritems[ra + 1];
                state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rc);
                {
                    LitResult invres = lit_vmmac_invokeoperatormethodandcontinue(state, &fiber, ra, state->vmstate.vmregisteritems[ra], "==", 1);
                    if(invres.type != LIT_STATUS_OK)
                    {
                        if(invres.type == LIT_STATUS_INVALID)
                        {
                            lit_vmmac_dispatchnext();
                        }
                        return invres;
                    }
                }
                state->vmstate.vmregisteritems[ra + 1] = tmpb;
            }
            ptmp = lit_vmmac_getrc(state, rc);
            state->vmstate.vmregisteritems[ra] = lit_value_makebool(lit_value_compare(state, bv, ptmp));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_LESSTHAN)
        {
            lit_vmmac_docomparisonop(lit_value_makebool, LIT_OPCODE_LESSTHAN, "<");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_LESSEQUAL)
        {
            lit_vmmac_docomparisonop(lit_value_makebool, LIT_OPCODE_LESSEQUAL, "<=");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_GREATERTHAN)
        {
            lit_vmmac_docomparisonop(lit_value_makebool, LIT_OPCODE_GREATERTHAN, ">");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_GREATEREQUAL)
        {
            lit_vmmac_docomparisonop(lit_value_makebool, LIT_OPCODE_GREATEREQUAL, ">=");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_NEGATE)
        {
            uint64_t ra;
            uint64_t rb;
            double dn;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            LitValue value = lit_vmmac_getrc(state, rb);
            if(!lit_value_isnumber(value))
            {
                lit_vmmac_fail(state, "operand must be a number");
            }
            dn = lit_value_asnumber(value);
            state->vmstate.vmregisteritems[ra] = lit_value_makenumber(-dn);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_NOT)
        {
            uint64_t ra;
            uint64_t rb;
            LitValue value;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            value = lit_vmmac_getrc(state, rb);
            if(lit_value_isinstance(value))
            {
                LitResult invres = lit_vmmac_invokeoperatormethodandcontinue(state, &fiber, rb, value, "!", 0);
                if(invres.type != LIT_STATUS_OK)
                {
                    if(invres.type == LIT_STATUS_INVALID)
                    {
                        lit_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra] = lit_value_makebool(lit_is_falsey(value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_BINNOT)
        {
            uint64_t rb;
            uint64_t ra;
            double dn;
            uint32_t cn;
            LitValue value;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            value = lit_vmmac_getrc(state, rb);
            if(!lit_value_isnumber(value))
            {
                lit_vmmac_fail(state, "operand must be a number");
            }
            dn = lit_value_asnumber(value);
            cn = (uint32_t)dn;
            state->vmstate.vmregisteritems[ra] = lit_value_makenumber(~cn);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_GLOBALSET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            lit_table_set(globals, lit_value_asstring(state->vmstate.vmconstantvalues[ra]), lit_vmmac_getrc(state, rbx));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_GLOBALGET)
        {
            uint64_t ra;
            uint64_t rbx;
            LitValue* reg;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            reg = &state->vmstate.vmregisteritems[ra];
            if(!lit_table_getentry(globals, lit_value_asstring(state->vmstate.vmconstantvalues[rbx]), reg))
            {
                *reg = lit_value_makenull();
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_UPVALUESET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            *state->vmstate.frame->closure->upvalues[ra]->location = lit_vmmac_getrc(state, rbx);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_UPVALUEGET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = *state->vmstate.frame->closure->upvalues[rbx]->location;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_PRIVATESET)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            state->vmstate.vmprivatevalues[(uint16_t)rbx] = LIT_BIT_ISSET(rbx, 16) ? state->vmstate.vmconstantvalues[ra] : state->vmstate.vmregisteritems[ra];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_PRIVATEGET)
        {
            state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = state->vmstate.vmprivatevalues[LIT_INST_GETBX(state->vmstate.instruction)];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_CALLCALLABLE)
        {
            lit_vmmac_writeframe(state);
            if(!lit_vmexec_actualcallvalue(state, LIT_INST_GETA(state->vmstate.instruction), LIT_INST_GETB(state->vmstate.instruction) - 1, lit_value_makenull()))
            {
                lit_vmexec_popgc(state);
                return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());                
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_UPVALUECLOSE)
        {
            lit_vmexec_closeupvalues(state, &state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] - 1);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_CLASSMAKE)
        {
            uint16_t rb;
            LitValue super;
            LitString* name;
            LitClass* klass;
            LitClass* superklass;
            name = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETA(state->vmstate.instruction)]);
            klass = lit_object_makeclass(state, name);
            state->vmstate.vmregisteritems[LIT_INST_GETC(state->vmstate.instruction)] = lit_value_fromobject(klass);
            lit_map_set(state->vmstate.globals, name, lit_value_fromobject(klass));
            rb = LIT_INST_GETB(state->vmstate.instruction);
            if(rb == 0)
            {
                klass->super = state->stdobjectclass;
                lit_table_addall(&klass->super->mthtable, &klass->mthtable);
                lit_table_addall(&klass->super->staticstable, &klass->staticstable);
            }
            else
            {
                super = state->vmstate.vmregisteritems[--rb];
                if(!lit_value_isclass(super))
                {
                    lit_vmmac_fail(state, "superclass must be a class");
                }
                superklass = lit_value_asclass(super);
                klass->super = superklass;
                klass->mthconstructor = superklass->mthconstructor;
                lit_table_addall(&superklass->mthtable, &klass->mthtable);
                lit_table_addall(&klass->super->staticstable, &klass->staticstable);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_CLASSPUTFIELDSTATIC)
        {
            LitClass* klass;
            LitValue vklass;
            LitValue setkey;
            LitValue setval;
            vklass = state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)];
            setkey = state->vmstate.vmconstantvalues[LIT_INST_GETB(state->vmstate.instruction)];
            setval = lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction));
            klass = lit_value_asclass(vklass);
            lit_table_set(&klass->staticstable, lit_value_asstring(setkey), setval);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_CLASSPUTMETHOD)
        {
            size_t ctorlen;
            size_t mthlen;
            const char* mthstr;
            const char* ctorstr;
            LitClass* klass;
            LitString* name;
            ctorlen = lit_string_getlength(state->strings.strconstructor);
            ctorstr = lit_string_getdata(state->strings.strconstructor);
            klass = lit_value_asclass(state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)]);
            name = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETB(state->vmstate.instruction)]);
            mthlen = name->strbuf.length;
            mthstr = name->strbuf.data;
            if((klass->mthconstructor == NULL || (klass->super != NULL && klass->mthconstructor == ((LitClass*)klass->super)->mthconstructor)) && mthlen == ctorlen && memcmp(mthstr, ctorstr, ctorlen) == 0)
            {
                klass->mthconstructor = lit_value_asobject(lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction)));
            }
            lit_table_set(&klass->mthtable, name, lit_vmmac_getrc(state, LIT_INST_GETC(state->vmstate.instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_FIELDGET)
        {
            uint8_t ra;
            LitValue value;
            LitResult tmpres;
            LitValue object;
            LitString* name;
            LitField* field;
            LitClass* klass;
            LitInstance* instance;
            object = state->vmstate.vmregisteritems[LIT_INST_GETB(state->vmstate.instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail(state, "attempt to index a null value");
            }
            name = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]);
            ra = LIT_INST_GETA(state->vmstate.instruction);
            if(lit_value_isinstance(object))
            {
                instance = lit_value_asinstance(object);
                if(!lit_table_getentry(&instance->fields, name, &value))
                {
                    if(lit_table_getentry(&instance->klass->mthtable, name, &value))
                    {
                        if(lit_value_isfield(value))
                        {
                            field = lit_value_asfield(value);
                            if(field->getter == NULL)
                            {
                                lit_vmmac_fail(state, "class %s does not have a getter for the field %s", instance->klass->name->strbuf.data, name->strbuf.data);
                            }
                            lit_vmmac_writeframe(state);
                            if(!lit_vmmac_callvalue(state, &fiber, lit_value_fromobject(lit_value_asfield(value)->getter), ra, 0, &tmpres))
                            {
                                return tmpres;
                            }
                            lit_vmmac_readframe(state, &fiber);
                            lit_vmmac_dispatchnext();
                        }
                        else
                        {
                            value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                        }
                    }
                    else
                    {
                        value = lit_value_makenull();
                    }
                }
            }
            else if(lit_value_isclass(object))
            {
                klass = lit_value_asclass(object);
                if(lit_table_getentry(&klass->staticstable, name, &value))
                {
                    if(lit_value_isfuncmethod(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                    else if(lit_value_isfield(value))
                    {
                        field = lit_value_asfield(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail(state, "class %s does not have a getter for the field %s", klass->name->strbuf.data, name->strbuf.data);
                        }
                        lit_vmmac_writeframe(state);
                        if(!lit_vmmac_callvalue(state, &fiber, lit_value_fromobject(field->getter), ra, 0, &tmpres))
                        {
                            return tmpres;
                        }
                        lit_vmmac_readframe(state, &fiber);
                        lit_vmmac_dispatchnext();
                    }
                }
                else
                {
                    value = lit_value_makenull();
                }
            }
            else
            {
                klass = lit_state_getclassfor(state, object);
                if(klass == NULL)
                {
                    lit_vmmac_fail(state, "only instances and classes have fields");
                }
                if(lit_table_getentry(&klass->mthtable, name, &value))
                {
                    if(lit_value_isfield(value))
                    {
                        field = lit_value_asfield(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail(state, "class %s does not have a getter for the field %s", klass->name->strbuf.data, name->strbuf.data);
                        }
                        lit_vmmac_writeframe(state);
                        if(!lit_vmmac_callvalue(state, &fiber, lit_value_fromobject(lit_value_asfield(value)->getter), ra, 0, &tmpres))
                        {
                            return tmpres;
                        }
                        lit_vmmac_readframe(state, &fiber);
                        lit_vmmac_dispatchnext();
                    }
                    else if(lit_value_isfuncmethod(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                }
                else
                {
                    value = lit_value_makenull();
                }
            }
            state->vmstate.vmregisteritems[ra] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_CLASSGETSUPERMETHOD)
        {
            LitValue value;
            LitValue instance;
            LitClass* klass;
            LitString* mthname;
            instance = state->vmstate.vmregisteritems[LIT_INST_GETB(state->vmstate.instruction)];
            klass = lit_value_asclass(instance);
            mthname = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]);
            if(lit_table_getentry(&klass->mthtable, mthname, &value) || lit_table_getentry(&klass->staticstable, mthname, &value))
            {
                value = lit_value_fromobject(lit_object_makeboundmethod(state, state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)], value));
            }
            else
            {
                value = lit_value_makenull();
            }
            state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_FIELDSET)
        {
            LitResult tmpres;
            LitValue value;
            LitValue instance;
            LitValue setter;
            LitClass* klass;
            LitField* field;
            LitInstance* inst;
            uint8_t ra;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            instance = state->vmstate.vmregisteritems[ra];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail(state, "attempt to index a null value");
            }
            value = state->vmstate.vmregisteritems[LIT_INST_GETC(state->vmstate.instruction)];
            LitString* fieldname = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETB(state->vmstate.instruction)]);
            if(lit_value_isclass(instance))
            {
                klass = lit_value_asclass(instance);
                if(lit_table_getentry(&klass->staticstable, fieldname, &setter) && lit_value_isfield(setter))
                {
                    field = lit_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail(state, "class %s does not have a setter for the field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe(state);
                    if(!lit_vmmac_callvalue(state, &fiber, lit_value_fromobject(field->setter), ra, 1, &tmpres))
                    {
                        return tmpres;
                    }
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                if(lit_value_isnull(value))
                {
                    lit_table_delete(&klass->staticstable, fieldname);
                }
                else
                {
                    lit_table_set(&klass->staticstable, fieldname, value);
                }
            }
            else if(lit_value_isinstance(instance))
            {
                inst = lit_value_asinstance(instance);
                if(lit_table_getentry(&inst->klass->mthtable, fieldname, &setter) && lit_value_isfield(setter))
                {
                    field = lit_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail(state, "class %s does not have a setter for the field %s", inst->klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe(state);
                    if(!lit_vmmac_callvalue(state, &fiber,lit_value_fromobject(field->setter), ra, 1, &tmpres))
                    {
                        return tmpres;
                    }
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                if(lit_value_isnull(value))
                {
                    lit_table_delete(&inst->fields, fieldname);
                }
                else
                {
                    lit_table_set(&inst->fields, fieldname, value);
                }
            }
            else
            {
                klass = lit_state_getclassfor(state, instance);
                if(klass == NULL)
                {
                    lit_vmmac_fail(state, "only instances and classes have fields");
                }
                if(lit_table_getentry(&klass->mthtable, fieldname, &setter) && lit_value_isfield(setter))
                {
                    field = lit_value_asfield(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail(state, "class %s does not have a setter for the field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe(state);
                    if(!lit_vmmac_callvalue(state, &fiber, lit_value_fromobject(field->setter), ra, 1, &tmpres))
                    {
                        return tmpres;
                    }
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                else
                {
                    lit_vmmac_fail(state, "class %s does not contain field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                }
            }
            state->vmstate.vmregisteritems[ra] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_IS)
        {
            uint8_t ra = LIT_INST_GETA(state->vmstate.instruction);
            LitValue instance = lit_vmmac_getrc(state, LIT_INST_GETB(state->vmstate.instruction));
            if(lit_value_isnull(instance))
            {
                state->vmstate.vmregisteritems[ra] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            LitClass* instanceklass = lit_state_getclassfor(state, instance);
            LitValue klass;
            if(!lit_table_getentry(globals, lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]), &klass))
            {
                state->vmstate.vmregisteritems[ra] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            if(instanceklass == NULL || !lit_value_isclass(klass))
            {
                lit_vmmac_fail(state, "operands must be an instance and a class");
            }
            LitClass* type = lit_value_asclass(klass);
            bool found = false;
            while(instanceklass != NULL)
            {
                if(instanceklass == type)
                {
                    found = true;
                    break;
                }
                instanceklass = (LitClass*)instanceklass->super;
            }
            state->vmstate.vmregisteritems[ra] = lit_value_makebool(found);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_INVOKE)
        {
            LitResult tmpres;
            lit_vmmac_writeframe(state);
            uint8_t ra = LIT_INST_GETA(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[ra];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail(state, "attempt to index a null value");
            }
            LitClass* klass = lit_value_isclass(instance) ? lit_value_asclass(instance) : lit_state_getclassfor(state, instance);
            if(klass == NULL)
            {
                lit_vmmac_fail(state, "only instances and classes have methods");
            }
            LitString* mthname = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]);
            int argc = LIT_INST_GETB(state->vmstate.instruction) - 1;
            LitValue method;
            if(lit_value_isinstance(instance) && (lit_table_getentry(&lit_value_asinstance(instance)->fields, mthname, &method)))
            {
                if(!lit_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres))
                {
                    return tmpres;
                }
            }
            else if(lit_value_isclass(instance) && lit_table_getentry(&klass->staticstable, mthname, &method))
            {
                if(!lit_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres))
                {
                    return tmpres;
                }
            }
            else if(lit_table_getentry(&klass->mthtable, mthname, &method))
            {
                if(!lit_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres))
                {
                    return tmpres;
                }
            }
            else
            {
                lit_vmmac_fail(state, "attempt to invoke undefined method '%s#%s'", klass->name->strbuf.data, mthname->strbuf.data);
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_INVOKESUPER)
        {
            size_t i;
            LitResult tmpres;
            lit_vmmac_writeframe(state);
            uint8_t ra = LIT_INST_GETA(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[ra + 1];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail(state, "attempt to index a null value");
            }
            LitClass* klass = lit_value_asclass(instance);
            if(klass == NULL)
            {
                lit_vmmac_fail(state, "only instances and classes have methods");
            }
            LitString* mthname = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]);
            int argc = LIT_INST_GETB(state->vmstate.instruction) - 1;
            LitValue method;
            if(lit_table_getentry(&klass->mthtable, mthname, &method) || lit_table_getentry(&klass->staticstable, mthname, &method))
            {
                for(i = ra + 1; i <= ra + (size_t)argc; i++)
                {
                    state->vmstate.vmregisteritems[i] = state->vmstate.vmregisteritems[i + 1];
                }
                if(!lit_vmmac_callvalue(state, &fiber, method, ra, argc, &tmpres))
                {
                    return tmpres;
                }
            }
            else
            {
                lit_vmmac_fail(state, "attempt to invoke undefined method '%s#%s' of super class", klass->name->strbuf.data, mthname->strbuf.data);
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_SUBSCRIPTGET)
        {
            uint8_t ra = LIT_INST_GETA(state->vmstate.instruction);
            uint16_t rb = LIT_INST_GETB(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[ra];
            LitValue tmpb = state->vmstate.vmregisteritems[ra + 1];
            state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rb);
            {
                LitResult invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, instance, "[]", 1);
                if(invres.type != LIT_STATUS_OK)
                {
                    if(invres.type == LIT_STATUS_INVALID)
                    {
                        lit_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra + 1] = tmpb;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_SUBSCRIPTSET)
        {
            uint8_t ra = LIT_INST_GETA(state->vmstate.instruction);
            uint16_t rb = LIT_INST_GETB(state->vmstate.instruction);
            uint16_t rc = LIT_INST_GETC(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[ra];
            LitValue tmpb = state->vmstate.vmregisteritems[ra + 1];
            LitValue tmpc = state->vmstate.vmregisteritems[ra + 2];
            state->vmstate.vmregisteritems[ra + 1] = lit_vmmac_getrc(state, rb);
            state->vmstate.vmregisteritems[ra + 2] = lit_vmmac_getrc(state, rc);
            {
                LitResult invres = lit_vmmac_invokeoperatormethoddefault(state, &fiber, ra, instance, "[]", 2);
                if(invres.type != LIT_STATUS_OK)
                {
                    if(invres.type == LIT_STATUS_INVALID)
                    {
                        lit_vmmac_dispatchnext();
                    }
                    return invres;
                }
            }
            state->vmstate.vmregisteritems[ra + 1] = tmpb;
            state->vmstate.vmregisteritems[ra + 2] = tmpc;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_ARRAYPUSH)
        {
            uint64_t ra;
            LitValue pval;
            LitArray* array;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            array = lit_value_asarray(state->vmstate.vmregisteritems[ra]);
            pval = lit_vmmac_getrc(state, LIT_INST_GETBX(state->vmstate.instruction));
            lit_array_push(array, pval);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_OBJECTPUSH)
        {
            LitValue operand = state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)];
            LitString* key = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETB(state->vmstate.instruction)]);
            LitValue value = state->vmstate.vmregisteritems[LIT_INST_GETC(state->vmstate.instruction)];
            if(lit_value_ismap(operand))
            {
                lit_map_set(lit_value_asmap(operand), key, value);
            }
            else if(lit_value_isinstance(operand))
            {
                lit_table_set(&lit_value_asinstance(operand)->fields, key, value);
            }
            else
            {
                lit_vmmac_fail(state, "slotted an object or a map as the operand, got %s", lit_value_valtypename(operand));
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFGLOBAL)
        {
            LitString* name = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETBX(state->vmstate.instruction)]);
            LitValue* value;
            if(lit_table_getslot(&state->vmstate.globals->innertable, name, &value))
            {
                state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            }
            else
            {
                lit_vmmac_fail(state, "attempt to reference a null value");
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFPRIVATE)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(lit_object_makereference(state, &state->vmstate.vmprivatevalues[rbx]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFLOCAL)
        {
            uint64_t ra;
            uint64_t rb;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rb = LIT_INST_GETB(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(lit_object_makereference(state, &state->vmstate.vmregisteritems[rb]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFUPVALUE)
        {
            uint64_t ra;
            uint64_t rbx;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            state->vmstate.vmregisteritems[ra] = lit_value_fromobject(lit_object_makereference(state, state->vmstate.upvalues[rbx]->location));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFFIELD)
        {
            LitValue object = state->vmstate.vmregisteritems[LIT_INST_GETB(state->vmstate.instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail(state, "attempt to index a null value");
            }
            LitValue* value;
            LitString* name = lit_value_asstring(state->vmstate.vmconstantvalues[LIT_INST_GETC(state->vmstate.instruction)]);
            if(lit_value_isinstance(object))
            {
                if(!lit_table_getslot(&lit_value_asinstance(object)->fields, name, &value))
                {
                    lit_vmmac_fail(state, "attempt to reference a null value");
                }
            }
            else
            {
                lit_vmmac_fail(state, "can only reference fields of real instances");
            }
            state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_REFSET)
        {
            LitValue reference = state->vmstate.vmregisteritems[LIT_INST_GETA(state->vmstate.instruction)];
            if(!lit_value_isreference(reference))
            {
                lit_vmmac_fail(state, "provided value is not a reference");
            }
            *lit_value_asreference(reference)->slot = state->vmstate.vmregisteritems[LIT_INST_GETB(state->vmstate.instruction)];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_PUSH_TRY)
        {
            uint64_t rbx;
            rbx = LIT_INST_GETBX(state->vmstate.instruction);
            if(fiber->handler_count == fiber->handler_capacity)
            {
                fiber->handler_capacity = LIT_GROW_CAPACITY(fiber->handler_capacity);
                fiber->handlers = (LitHandler*)lit_sysmem_realloc(fiber->handlers, fiber->handler_capacity * sizeof(LitHandler));
            }
            LitHandler* handler = &fiber->handlers[fiber->handler_count++];
            handler->handler_ip = state->vmstate.ip + (int)rbx;
            handler->register_count = (uint32_t)(state->vmstate.vmregisteritems - fiber->registeritems);
            handler->frame_count = fiber->framecount;
            handler->error_reg = LIT_INST_GETA(state->vmstate.instruction);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_POP_TRY)
        {
            fiber->handler_count--;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_THROW)
        {
            uint64_t ra;
            ra = LIT_INST_GETA(state->vmstate.instruction);
            if(!lit_vm_handleerror(state, lit_value_tostring(state, state->vmstate.vmregisteritems[ra], 0)))
            {
                return lit_result_make(LIT_STATUS_RUNTIMEERROR, fiber->error);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(LIT_OPCODE_RETHROW)
        {
            if(!lit_vm_handleerror(state, lit_value_asstring(fiber->error)))
            {
                return lit_result_make(LIT_STATUS_RUNTIMEERROR, fiber->error);
            }
            lit_vmmac_dispatchnext();
        }
#if !defined(LIT_CONF_USECOMPUTEDGOTO) || (LIT_CONF_USECOMPUTEDGOTO == 0)
        default:
#endif
        {
            lit_vmmac_fail(state, "unknown opcode %i", state->vmstate.instruction);
            lit_vmexec_popgc(state);
            return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());            
        }
    }
}

void lit_state_nativeexitjump()
{
    longjmp(g_vmglobaljumpbuf, 1);
}

static LitState* g_replstate;

void lit_cli_interupthandler(int signalid)
{
    lit_state_destroy(g_replstate);
    fprintf(stderr, "\nExiting (signalid=%d).\n", signalid);
    exit(0);
}


static char* nn_cli_getinput(linocontext_t* lictx, const char* prompt)
{
    return lino_context_readline(lictx, prompt);
}

static void nn_cli_addhistoryline(linocontext_t* lictx, const char* line)
{
    lino_context_historyadd(lictx, line);
}

static void nn_cli_freeline(linocontext_t* lictx, char* line)
{
    lino_context_freeline(lictx, line);
}

void lit_cli_runrepl(LitState* state, linocontext_t* lictx)
{
    char* line;
    LitResult result;
    LitValue value;
    LitIOStream* pr;
    pr = state->streamstdout;
    g_replstate = state;
    signal(SIGINT, lit_cli_interupthandler);
#ifndef _WIN32
    signal(SIGTSTP, lit_cli_interupthandler);
#endif
    while(true)
    {
        line = nn_cli_getinput(lictx, "> ");
        if(line == NULL)
        {
            break;
        }
        nn_cli_addhistoryline(lictx, line);
        result = lit_state_interpretsource(state, "repl", line);
        if(result.type == LIT_STATUS_OK)
        {
            lit_iostream_writecolor(pr, COLOR_GREEN);
            lit_iostream_puts(pr, "\n");
            value = result.result;
            value = state->vmstate.frame->slots[1];
            lit_value_printvalue(pr, value, true);
            lit_iostream_writecolor(pr, COLOR_RESET);
            lit_iostream_puts(pr, "\n");
        }
        nn_cli_freeline(lictx, line);
    }
}


static void lit_cli_parseenv(LitState* state, char** envp)
{
    enum { kMaxKeyLen = 40 };
    size_t i;
    int len;
    int pos;
    char* raw;
    char* valbuf;
    char keybuf[kMaxKeyLen];
    LitString* oskey;
    LitString* osval;
    LitMap* envmap;
    envmap = lit_object_makemap(state, NULL);
    if(envp == NULL)
    {
        return;
    }
    for(i=0; envp[i] != NULL; i++)
    {
        raw = envp[i];
        len = strlen(raw);
        pos = lit_util_findfirstpos(raw, len, '=');
        if(pos == -1)
        {
            fprintf(stderr, "malformed environment string '%s'\n", raw);
        }
        else
        {
            memset(keybuf, 0, kMaxKeyLen);
            memcpy(keybuf, raw, pos);
            valbuf = &raw[pos+1];
            oskey = lit_string_copy(state, keybuf);
            osval = lit_string_copy(state, valbuf);
            lit_map_set(envmap, oskey, lit_value_fromobject(osval));
            
        }
    }
    lit_state_setglobal(state, lit_string_copy(state, "ENV"), lit_value_fromobject(envmap));

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

static void optprs_fprintusage(FILE* out, optlongflags_t* flags)
{
    size_t i;
    char ch;
    bool needval;
    bool maybeval;
    bool hadshort;
    optlongflags_t* flag;
    for(i=0; flags[i].longname != NULL; i++)
    {
        flag = &flags[i];
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

static void lit_cli_showusage(char* argv[], optlongflags_t* flags, bool fail)
{
    FILE* out;
    out = fail ? stderr : stdout;
    fprintf(out, "usage: %s [<options>] [<filename> | -e <code>]\n", argv[0]);
    optprs_fprintusage(out, flags);
}

int main(int argc, char* argv[], char** envp)
{
    int i;
    int co;
    int opt;
    int scriptargc;
    int longindex;
    bool dumpbccode;
    bool wasusage;
    char *arg;
    char* source;
    const char* bytecodefile;
    const char* filename;
    FILE* tmpfh;
    char* scriptargv[128];
    optcontext_t options;
    linocontext_t lictx;
    LitState* state;
    LitArray* argarray;
    LitStatusCode result;
    result = LIT_STATUS_OK;
    static optlongflags_t longopts[] =
    {
        {"help", 'h', OPTPARSE_NONE, "this help"},
        {"dump", 'd', OPTPARSE_NONE, "dump instructions"},
        {"ast", 'a', OPTPARSE_NONE, "dump AST"},
        {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
        {"trace", 't', OPTPARSE_NONE, "trace execution"},
        {"instsonly", 'i', OPTPARSE_NONE, "when '-t' is specified, trace instructions only, skipping printing values"},
        {"dest", 'T', OPTPARSE_REQUIRED, "when '-t' is specified, write trace output to file. defaults to stderr"},
        {"output", 'o', OPTPARSE_REQUIRED, "compile a script to bytecode"},
        {"quit", 'q', OPTPARSE_NONE, "when dumping flags (like '-a' or '-d') are specified, quit immediately after"},
        {0, 0, (optargtype_t)0, NULL}
    };
    #if defined(LIT_PLATFORM_WINDOWS) || defined(_MSC_VER)
        _setmode(fileno(stdin), _O_BINARY);
        _setmode(fileno(stdout), _O_BINARY);
        _setmode(fileno(stderr), _O_BINARY);
    #endif
    wasusage = false;
    dumpbccode = false;
    source = NULL;
    bytecodefile = NULL;
    state = lit_state_make();
    if(state == NULL)
    {
        fprintf(stderr, "failed to create state\n");
        return 0;
    }
    scriptargc = 0;
    optprs_init(&options, argc, argv);
    options.permute = 0;
    while ((opt = optprs_nextlongflag(&options, longopts, &longindex)) != -1)
    {
        co = longopts[longindex].shortname;
        if(opt == '?')
        {
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
            result = LIT_STATUS_RUNTIMEERROR;
            goto endmain;
        }
        else if(co == 'h')
        {
            lit_cli_showusage(argv, longopts, false);
            wasusage = true;
        }
        else if(co == 'a')
        {
            state->config.dumpast = true;
        }
        else if(co == 'd')
        {
            dumpbccode = true;
        }
        else if(co == 'o')
        {
            bytecodefile = options.optarg;
        }
        else if(co == 'e')
        {
            source = options.optarg;
        }
        else if(co == 'q')
        {
            state->config.quitafterdump = true;
        }
        else if(co == 't')
        {
            state->config.traceexecution = true;
        }
        else if(co == 'i')
        {
            state->config.traceinstsonly = true;
        }
        else if(co == 'T')
        {
            tmpfh = fopen(options.optarg, "wb");
            if(tmpfh == NULL)
            {
                fprintf(stderr, "cannot open trace destination file '%s' for writing\n", options.optarg);
                goto endmain;
            }
            state->config.desttrace = lit_iostream_makeio(tmpfh, true);
            state->config.havedesttrace = true;
        }
    }
    if(wasusage)
    {
        goto endmain;
    }
    lit_cli_parseenv(state, envp);
    while(true)
    {
        arg = optprs_nextpositional(&options);
        if(arg == NULL)
        {
            break;
        }
        scriptargv[scriptargc] = arg;
        scriptargc++;
    }
    if(bytecodefile != NULL)
    {
        if(!lit_state_compileandsavefile(state, scriptargv[0], bytecodefile))
        {
            result = LIT_STATUS_COMPILEERROR;
        }
        goto endmain;
    }
    {
        argarray = lit_array_make(state);
        lit_state_setglobal(state, lit_string_copy(state, "ARGV"), lit_value_fromobject(argarray));
        for(i = 0; i < (int)scriptargc; i++)
        {
            arg = scriptargv[i];

        }
    }
    if(source != NULL)
    {
        const char* modname = "<-e>";
        if(dumpbccode)
        {
            LitModule* module = lit_state_compilemodulesource(state, lit_string_copy(state, modname), source);
            if(module == NULL)
            {
                goto endmain;
            }
            lit_debug_disasmodule(state->streamstdout, module, source);
            if(state->config.quitafterdump)
            {
                goto endmain;
            }
        }
        else
        {
            result = lit_state_interpretsource(state, modname, source).type;
            if(result != LIT_STATUS_OK)
            {
                goto endmain;
            }
        }
    }
    else if(scriptargc > 0)
    {
        filename = scriptargv[0];
        if(dumpbccode)
        {
            result = lit_state_dumpfile(state, state->streamstdout, filename).type;
            if(state->config.quitafterdump)
            {
                goto endmain;
            }
        }
        else
        {
            result = lit_state_interpretfile(state, filename).type;
        }
        if(result != LIT_STATUS_OK)
        {
            goto endmain;
        }
    }
    else
    {
        state->config.isreplmode = true;
        lino_context_init(&lictx);
        lit_cli_runrepl(state, &lictx);
    }
    endmain:
    lit_state_destroy(state);
    if(result != LIT_STATUS_OK)
    {
        return 1;
    }
    return 0;
}

