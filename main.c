
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
#include "regtypes.h"
#include "optparse.h"
#include "oslib.h"
#include "lino.h"

#if !defined(_WIN32) && !defined(__CYGWIN__)
    #include <dirent.h>
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

/* can't be over 255 */
#define LIT_REGISTERS_MAX 255

#define LIT_GC_HEAP_GROW_FACTOR 2
#define LIT_INITIAL_CALL_FRAMES 1024

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
#define UINT8_COUNT UINT8_MAX + 1
#define UINT16_COUNT UINT16_MAX + 1

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
        lit_vm_raisefatalerror(state, "expected %i arguments, got %i", count, argc); \
        return lit_value_makenull();                                                \
    }

/* do not change these, or old bytecode files will break! */
#define LIT_BYTECODE_MAGIC_NUMBER 6932
#define LIT_BYTECODE_END_NUMBER 2942
#define LIT_STRING_KEY 48

#define lit_set_native_exit_jump() setjmp(g_vmglobaljumpbuf)

#define OBJECT_TYPE(value) (lit_value_asobject(value)->type)

#define TABLE_MAX_LOAD 0.75

#define LIT_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define LIT_GC_FREEOBJECT(state, type, pointer) lit_reallocate(state, pointer, sizeof(type), 0)

#define LIT_BIT_SETBIT(number, n) number |= 1UL << n
#define LIT_BIT_ISSET(number, n) (((number >> n) & 1U) != 0)

#define LIT_LONGEST_OP_NAME 13

#define LIT_OPCODE_SIZE 0x3f
#define LIT_A_ARG_SIZE 0xff
#define LIT_B_ARG_SIZE 0x1ff
#define LIT_C_ARG_SIZE 0x1ff
/* 18 bits max */
#define LIT_BX_ARG_SIZE 0x3ffff
/* 17 bits max */
#define LIT_SBX_ARG_SIZE 0x1ffff

#define LIT_A_ARG_POSITION 6
#define LIT_B_ARG_POSITION 14
#define LIT_C_ARG_POSITION 23
#define LIT_BX_ARG_POSITION 14
#define LIT_SBX_ARG_POSITION 15
#define LIT_SBX_FLAG_POSITION 14

/*
 * Instruction can follow one of the three formats:
 *
 * ABC  opcode:6 bits (starting from bit 0), A:8 bits, B:9 bits, C:9 bits
 * ABx  opcode:6 bits (starting from bit 0), A:8 bits, Bx:18 bits
 * AsBx opcode:6 bits (starting from bit 0), A:8 bits, sBx:18 bits (signed)
 */

#define LIT_INSTRUCTION_OPCODE(instruction) (instruction & LIT_OPCODE_SIZE)
#define LIT_INSTRUCTION_A(instruction) (((int64_t)(instruction >> LIT_A_ARG_POSITION)) & LIT_A_ARG_SIZE)
#define LIT_INSTRUCTION_B(instruction) (((int64_t)(instruction >> LIT_B_ARG_POSITION)) & LIT_B_ARG_SIZE)
#define LIT_INSTRUCTION_C(instruction) (((int64_t)(instruction >> LIT_C_ARG_POSITION)) & LIT_C_ARG_SIZE)
#define LIT_INSTRUCTION_BX(instruction) (((int64_t)(instruction >> LIT_BX_ARG_POSITION)) & LIT_BX_ARG_SIZE)
#define LIT_INSTRUCTION_SBX(instruction) ((((int64_t)(instruction >> LIT_SBX_ARG_POSITION)) & LIT_SBX_ARG_SIZE) * (((((int64_t)instruction) >> LIT_SBX_FLAG_POSITION) & 0x1) == 1 ? -1 : 1))

#define LIT_FORM_ABC_INSTRUCTION(opcode, a, b, c) \
    ( \
        ((opcode) & LIT_OPCODE_SIZE) | \
        ((((int64_t)(a)) & LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | \
        ((((int64_t)(b)) & LIT_B_ARG_SIZE) << LIT_B_ARG_POSITION) | \
        ((((int64_t)(c)) & LIT_C_ARG_SIZE) << LIT_C_ARG_POSITION) \
    )

#define LIT_FORM_ABX_INSTRUCTION(opcode, a, bx) \
    ( \
        ((opcode) & LIT_OPCODE_SIZE) | \
        ((((int64_t)(a)) & LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | \
        ((((int64_t)(bx)) & LIT_BX_ARG_SIZE) << LIT_BX_ARG_POSITION) \
    )

#define LIT_FORM_ASBX_INSTRUCTION(opcode, a, sbx) \
    ( \
        ((opcode) & LIT_OPCODE_SIZE) | \
        (((a) & LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | \
        ((labs((int64_t)(sbx)) & LIT_SBX_ARG_SIZE) << LIT_SBX_ARG_POSITION)) | \
        (((((int64_t)(sbx)) < 0 ? 1 : 0) << LIT_SBX_FLAG_POSITION) \
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

enum LitFunctionType
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

enum LitTokenType
{
    LIT_ASTTOKTYP_NEW_LINE,

    /* single-character tokens */
    LIT_ASTTOKTYP_LEFT_PAREN,
    LIT_ASTTOKTYP_RIGHT_PAREN,
    LIT_ASTTOKTYP_LEFT_BRACE,
    LIT_ASTTOKTYP_RIGHT_BRACE,
    LIT_ASTTOKTYP_LEFT_BRACKET,
    LIT_ASTTOKTYP_RIGHT_BRACKET,
    LIT_ASTTOKTYP_COMMA,
    LIT_ASTTOKTYP_SEMICOLON,
    LIT_ASTTOKTYP_COLON,

    /* one or two character tokens */
    LIT_ASTTOKTYP_BAR_EQUAL,
    LIT_ASTTOKTYP_BAR,
    LIT_ASTTOKTYP_BAR_BAR,
    LIT_ASTTOKTYP_AMPERSAND_EQUAL,
    LIT_ASTTOKTYP_AMPERSAND,
    LIT_ASTTOKTYP_AMPERSAND_AMPERSAND,
    LIT_ASTTOKTYP_BANG,
    LIT_ASTTOKTYP_BANG_EQUAL,
    LIT_ASTTOKTYP_EQUAL,
    LIT_ASTTOKTYP_EQUAL_EQUAL,
    LIT_ASTTOKTYP_GREATER,
    LIT_ASTTOKTYP_GREATER_EQUAL,
    LIT_ASTTOKTYP_GREATER_GREATER,
    LIT_ASTTOKTYP_LESS,
    LIT_ASTTOKTYP_LESS_EQUAL,
    LIT_ASTTOKTYP_LESS_LESS,
    LIT_ASTTOKTYP_PLUS,
    LIT_ASTTOKTYP_PLUS_EQUAL,
    LIT_ASTTOKTYP_PLUS_PLUS,
    LIT_ASTTOKTYP_MINUS,
    LIT_ASTTOKTYP_MINUS_EQUAL,
    LIT_ASTTOKTYP_MINUS_MINUS,
    LIT_ASTTOKTYP_STAR,
    LIT_ASTTOKTYP_STAR_EQUAL,
    LIT_ASTTOKTYP_STAR_STAR,
    LIT_ASTTOKTYP_SLASH,
    LIT_ASTTOKTYP_SLASH_EQUAL,
    LIT_ASTTOKTYP_QUESTION,
    LIT_ASTTOKTYP_QUESTION_QUESTION,
    LIT_ASTTOKTYP_PERCENT,
    LIT_ASTTOKTYP_PERCENT_EQUAL,
    LIT_ASTTOKTYP_ARROW,
    LIT_ASTTOKTYP_SMALL_ARROW,
    LIT_ASTTOKTYP_TILDE,
    LIT_ASTTOKTYP_CARET,
    LIT_ASTTOKTYP_CARET_EQUAL,
    LIT_ASTTOKTYP_DOT,
    LIT_ASTTOKTYP_DOT_DOT,
    LIT_ASTTOKTYP_DOT_DOT_DOT,
    LIT_ASTTOKTYP_SHARP,
    LIT_ASTTOKTYP_SHARP_EQUAL,

    /* literals */
    LIT_ASTTOKTYP_IDENTIFIER,
    LIT_ASTTOKTYP_STRING,
    LIT_ASTTOKTYP_INTERPOLATION,
    LIT_ASTTOKTYP_NUMBER,

    /* keywords */
    LIT_ASTTOKTYP_CLASS,
    LIT_ASTTOKTYP_ELSE,
    LIT_ASTTOKTYP_FALSE,
    LIT_ASTTOKTYP_FOR,
    LIT_ASTTOKTYP_FUNCTION,
    LIT_ASTTOKTYP_IF,
    LIT_ASTTOKTYP_NULL,
    LIT_ASTTOKTYP_RETURN,
    LIT_ASTTOKTYP_SUPER,
    LIT_ASTTOKTYP_THIS,
    LIT_ASTTOKTYP_TRUE,
    LIT_ASTTOKTYP_VAR,
    LIT_ASTTOKTYP_WHILE,
    LIT_ASTTOKTYP_CONTINUE,
    LIT_ASTTOKTYP_BREAK,
    LIT_ASTTOKTYP_NEW,
    LIT_ASTTOKTYP_EXPORT,
    LIT_ASTTOKTYP_IS,
    LIT_ASTTOKTYP_STATIC,
    LIT_ASTTOKTYP_OPERATOR,
    LIT_ASTTOKTYP_IN,
    LIT_ASTTOKTYP_CONST,
    LIT_ASTTOKTYP_REF,

    LIT_ASTTOKTYP_ERROR,
    LIT_ASTTOKTYP_EOF
};

enum LitPrecedence
{
    LIT_ASTPREC_NONE,
    LIT_ASTPREC_ASSIGNMENT, /* = */
    LIT_ASTPREC_OR, /* || */
    LIT_ASTPREC_AND, /* && */
    LIT_ASTPREC_BOR, /* | ^ */
    LIT_ASTPREC_BAND, /* & */
    LIT_ASTPREC_SHIFT, /* << >> */
    LIT_ASTPREC_EQUALITY, /* == != */
    LIT_ASTPREC_COMPARISON, /* < > <= >= */
    LIT_ASTPREC_COMPOUND, /* += -= *= /= ++ -- */
    LIT_ASTPREC_TERM, /* + - */
    LIT_ASTPREC_FACTOR, /* * / */
    LIT_ASTPREC_IS, /* is */
    LIT_ASTPREC_RANGE, /* .. */
    LIT_ASTPREC_UNARY, /* ! - ~ */
    LIT_ASTPREC_NULL, /* ?? */
    LIT_ASTPREC_CALL, /* . () */
    LIT_ASTPREC_PRIMARY
};

enum LitExpressionType
{
    LIT_ASTEXPRTYP_LITERAL,
    LIT_ASTEXPRTYP_BINARY,
    LIT_ASTEXPRTYP_UNARY,
    LIT_ASTEXPRTYP_VAR,
    LIT_ASTEXPRTYP_ASSIGN,
    LIT_ASTEXPRTYP_CALL,
    LIT_ASTEXPRTYP_SET,
    LIT_ASTEXPRTYP_GET,
    LIT_ASTEXPRTYP_LAMBDA,
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
    LIT_ASTEXPRTYP_FIELD
};

enum LitInstructionType
{
    LIT_INSTYP_ABC,
    LIT_INSTYP_ABX,
    LIT_INSTYP_ASBX,
};

enum LitOpCode
{
    OP_MOVE, /* R(A) := RC(B) */
    OP_LOAD_NULL, /* R(A) := null */
    OP_LOAD_BOOL, /* R(A) := (bool) B */
    OP_CLOSURE, /* R(A) := PrC[Bx] */
    OP_ARRAY, /* R(A) := new Array(Bx) */
    OP_OBJECT, /* R(A) = new Object() */
    OP_RANGE, /* R(A) = new Range(RC(B), RC(C)) */
    OP_RETURN, /* return R(A) */
    OP_ADD, /* R(A) := RC(B) + RC(C) */
    OP_SUBTRACT, /* R(A) := RC(B) - RC(C) */
    OP_MULTIPLY, /* R(A) := RC(B) * RC(C) */
    OP_DIVIDE, /* R(A) := RC(B) / RC(C) */
    OP_FLOOR_DIVIDE, /* R(A) := floor(RC(B) / RC(C)) */
    OP_MOD, /* R(A) := RC(B) % RC(C) */
    OP_POWER, /* R(A) := pow(RC(B), RC(C)) */
    OP_LSHIFT, /* R(A) := RC(B) << RC(C) */
    OP_RSHIFT, /* R(A) := RC(B) >> RC(C) */
    OP_BXOR, /* R(A) := RC(B) ^ RC(C) */
    OP_BAND, /* R(A) := RC(B) & RC(C) */
    OP_BOR, /* R(A) := RC(B) | RC(C) */
    OP_JUMP, /* PC += sBx */
    OP_TRUE_JUMP, /* if (R(A)) PC += Bx */
    OP_FALSE_JUMP, /* if (not R(A)) PC += Bx */
    OP_NON_NULL_JUMP, /* if (R(A) != null) PC += Bx */
    OP_NULL_JUMP, /* if (R(A) == null) PC += Bx */
    OP_EQUAL, /* R(A) := RC(B) == RC(C) */
    OP_LESS, /* R(A) := RC(B) < RC(C) */
    OP_LESS_EQUAL, /* R(A) := RC(B) <= RC(C) */
    OP_GREATER, /* R(A) := RC(B) > RC(C) */
    OP_GREATER_EQUAL, /* R(A) := RC(B) >= RC(C) */
    OP_NEGATE, /* R(A) := -RC(B) */
    OP_NOT, /* R(A) := !RC(B) */
    OP_BNOT, /* R(A) := ~RC(B) */
    OP_SET_GLOBAL, /* G[C(A)] := RC(BX) */
    OP_GET_GLOBAL, /* R(A) := G[C(Bx)] */
    OP_SET_UPVALUE, /* U[A] := RC(Bx) */
    OP_GET_UPVALUE, /* R(A) := U[Bx] */
    OP_SET_PRIVATE, /* P[A] := RC(Bx) */
    OP_GET_PRIVATE, /* R(A) := P[C(Bx)] */
    OP_CALL, /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
    OP_CLOSE_UPVALUE, /* close_upvalue(R(A)) */
    OP_CLASS, /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
    OP_STATIC_FIELD, /* R(A)[C(B)] = RC(C) */
    OP_METHOD, /* R(A).Methods[C(B)] = RC(C) */
    OP_GET_FIELD, /* R(A) = R(B)[C(C)] */
    OP_GET_SUPER_METHOD, /* R(A) = R(B).super[C(C)] */
    OP_SET_FIELD, /* R(A)[C(B)] = R(C) */
    OP_IS, /* R(A) := RC(B) is G[C(C)] */
    OP_INVOKE, /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
    OP_INVOKE_SUPER, /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
    OP_SUBSCRIPT_GET, /* R(A) := R(A)[RC(B)] */
    OP_SUBSCRIPT_SET, /* R(A)[RC(B)] := R(C) */
    OP_PUSH_ARRAY_ELEMENT, /* R(A)[R(A).count++] = RC(Bx) */
    OP_PUSH_OBJECT_ELEMENT, /* R(A)[R(B)] = RC(C) */
    OP_REFERENCE_GLOBAL, /* R(A) := ref G(C[Bx]) */
    OP_REFERENCE_PRIVATE, /* R(A) := ref P(Bx) */
    OP_REFERENCE_LOCAL, /* R(A) := ref R(B) */
    OP_REFERENCE_UPVALUE, /* R(A) := ref U(Bx) */
    OP_REFERENCE_FIELD, /* R(A) = ref R(B)[C(C)] */
    OP_SET_REFERENCE, /* ref R(A) := R(B) */
};

enum LitIOStrMode
{
    LIT_IOSTRMODE_UNDEFINED,
    LIT_IOSTRMODE_STRING,
    LIT_IOSTRMODE_FILE
};

typedef enum LitIOStrMode LitIOStrMode;
typedef enum /**/ LitObjType LitObjType;
typedef enum /**/ LitFunctionType LitFunctionType;
typedef enum /**/ LitErrorType LitErrorType;
typedef enum /**/ LitStatusCode LitStatusCode;
typedef enum /**/ LitTokenType LitTokenType;
typedef enum /**/ LitPrecedence LitPrecedence;
typedef enum LitValType LitValType;
typedef enum LitExpressionType LitExpressionType;
typedef enum LitInstructionType LitInstructionType;
typedef enum LitOpCode LitOpCode;

typedef struct /**/ LitScanner LitScanner;
typedef struct /**/ LitState LitState;
typedef struct /**/ LitParser LitParser;
typedef struct /**/ LitEmitter LitEmitter;
typedef struct /**/ LitState LitState;
typedef struct /**/ LitResult LitResult;
typedef struct /**/ LitObject LitObject;
typedef struct /**/ LitMap LitMap;
typedef struct /**/ LitString LitString;
typedef struct /**/ LitModule LitModule;
typedef struct /**/ LitFiber LitFiber;
typedef struct /**/ LitUserdata LitUserdata;
typedef struct /**/ LitExpression LitExpression;
typedef struct /**/ LitUpvalue LitUpvalue;
typedef struct /**/ LitClass LitClass;
typedef struct /**/ LitLiteralExpression LitLiteralExpression;
typedef struct /**/ LitBinaryExpression LitBinaryExpression;
typedef struct /**/ LitUnaryExpression LitUnaryExpression;
typedef struct /**/ LitVarExpression LitVarExpression;
typedef struct /**/ LitAssignExpression LitAssignExpression;
typedef struct /**/ LitCallExpression LitCallExpression;
typedef struct /**/ LitGetExpression LitGetExpression;
typedef struct /**/ LitSetExpression LitSetExpression;
typedef struct /**/ LitParameter LitParameter;
typedef struct /**/ LitDynListParam LitDynListParam;
typedef struct /**/ LitArrayExpression LitArrayExpression;
typedef struct /**/ LitObjectExpression LitObjectExpression;
typedef struct /**/ LitSubscriptExpression LitSubscriptExpression;
typedef struct /**/ LitThisExpression LitThisExpression;
typedef struct /**/ LitSuperExpression LitSuperExpression;
typedef struct /**/ LitRangeExpression LitRangeExpression;
typedef struct /**/ LitTernaryExpression LitTernaryExpression;
typedef struct /**/ LitInterpolationExpression LitInterpolationExpression;
typedef struct /**/ LitReferenceExpression LitReferenceExpression;
typedef struct /**/ LitExpressionStatement LitExpressionStatement;
typedef struct /**/ LitBlockStatement LitBlockStatement;
typedef struct /**/ LitVarStatement LitVarStatement;
typedef struct /**/ LitIfStatement LitIfStatement;
typedef struct /**/ LitWhileStatement LitWhileStatement;
typedef struct /**/ LitForStatement LitForStatement;
typedef struct /**/ LitContinueStatement LitContinueStatement;
typedef struct /**/ LitBreakStatement LitBreakStatement;
typedef struct /**/ LitFunctionStatement LitFunctionStatement;
typedef struct /**/ LitReturnStatement LitReturnStatement;
typedef struct /**/ LitMethodStatement LitMethodStatement;
typedef struct /**/ LitClassStatement LitClassStatement;
typedef struct /**/ LitFieldStatement LitFieldStatement;
typedef struct /**/ LitPrivate LitPrivate;
typedef struct /**/ LitDynListPriv LitDynListPriv;
typedef struct /**/ LitLocal LitLocal;
typedef struct /**/ LitDynListLoc LitDynListLoc;
typedef struct /**/ LitCompilerUpvalue LitCompilerUpvalue;
typedef struct /**/ LitCompiler LitCompiler;
typedef struct /**/ LitEmitter LitEmitter;
typedef struct /**/ LitParseRule LitParseRule;
typedef struct /**/ LitParser LitParser;
typedef struct /**/ LitEmulatedFile LitEmulatedFile;
typedef struct /**/ LitScanner LitScanner;
typedef struct LitFileData LitFileData;
typedef struct LitResult LitResult;
typedef struct LitToken LitToken;
typedef struct LitDynListExpr LitDynListExpr;
typedef struct LitIOStream LitIOStream;
typedef struct LitStrBuffer LitStrBuffer;
typedef struct LitValue LitValue;

typedef struct LitDynListUInt LitDynListUInt;
typedef struct LitDynListByte LitDynListByte;
typedef struct LitDynListVal LitDynListVal;
typedef struct LitChunk LitChunk;
typedef struct LitTabEntry LitTabEntry;
typedef struct LitTable LitTable;
typedef struct LitObject LitObject;
typedef struct LitString LitString;
typedef struct LitFunction LitFunction;
typedef struct LitUpvalue LitUpvalue;
typedef struct LitFuncClosure LitFuncClosure;
typedef struct LitClosurePrototype LitClosurePrototype;
typedef struct LitFuncNative LitFuncNative;
typedef struct LitCallFrame LitCallFrame;
typedef struct LitMap LitMap;
typedef struct LitModule LitModule;
typedef struct LitFiber LitFiber;
typedef struct LitClass LitClass;
typedef struct LitInstance LitInstance;
typedef struct LitBoundMethod LitBoundMethod;
typedef struct LitArray LitArray;
typedef struct LitVarargArray LitVarargArray;
typedef struct LitUserdata LitUserdata;
typedef struct LitRange LitRange;
typedef struct LitField LitField;
typedef struct LitReference LitReference;
typedef struct LitState LitState;

typedef void (*LitDebugInstructionFn)(LitIOStream*, uint64_t, const char*);
typedef void (*LitErrorFn)(LitState* state, const char* message);
typedef void (*LitPrintFn)(LitState* state, const char* message);
typedef LitExpression* (*LitPrefixParseFn)(LitParser*, bool);
typedef LitExpression* (*LitInfixParseFn)(LitParser*, LitExpression*, bool);
typedef void (*LitCleanupFn)(LitState*, LitUserdata*, bool);
typedef LitValue (*LitMapIndexFn)(LitState*, LitObject*, LitString*, LitValue*);

typedef LitValue (*LitNativeFunctionFn)(LitState*, LitValue, size_t, LitValue*);

struct LitStrBuffer
{
    uint8_t isintern;
    /* capacity should be >= length+1 to allow for \0 */
    size_t capacity;
    size_t length;
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
    size_t maxvallength;
    /* the mode that determines what writer actually does */
    LitIOStrMode wrmode;
    LitStrBuffer psbuf;
    FILE* handle;
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
    size_t capacity;
    size_t count;
    size_t* values;
};

struct LitDynListByte
{
    size_t capacity;
    size_t count;
    uint8_t* values;
};

struct LitDynListVal
{
    size_t capacity;
    size_t count;
    LitValue* values;
};

struct LitChunk
{
    size_t compiledcodecount;
    size_t capacity;
    uint64_t* compiledcodechunk;
    bool haslineinfo;
    size_t linecount;
    size_t linecapacity;
    uint16_t* lines;
    LitDynListVal constantlist;
};

struct LitTabEntry
{
    LitString* key;
    LitValue value;
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

struct LitFunction
{
    LitObject innerobject;
    LitChunk chunk;
    LitString* name;
    size_t argcount;
    size_t upvaluecount;
    uint32_t maxregisters;
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
    LitFunction* function;
    LitUpvalue** upvalues;
    size_t upvaluecount;
};

struct LitClosurePrototype
{
    LitObject innerobject;
    LitFunction* function;
    bool* local;
    uint32_t* indexes;
    size_t upvaluecount;
};

struct LitFuncNative
{
    LitObject innerobject;
    LitNativeFunctionFn natfuncptr;
    LitString* name;
};

struct LitCallFrame
{
    LitFunction* function;
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
    LitTable values;
};

struct LitModule
{
    LitObject innerobject;
    LitValue returnvalue;
    LitString* name;
    LitValue* privatevalues;
    LitMap* privatenames;
    size_t privatecount;
    LitFunction* mainfunction;
    LitFiber* mainfiber;
    bool ran;
};

struct LitFiber
{
    LitObject innerobject;
    LitFiber* parent;
    LitValue* registeritems;
    size_t registersallocated;
    LitCallFrame* framevals;
    size_t framecapacity;
    size_t framecount;
    size_t argcount;
    LitValue* returnaddress;
    LitUpvalue* open_upvalues;
    LitModule* module;
    LitValue error;
    bool abort;
    bool catcher;
    bool caught;
};

struct LitClass
{
    LitObject innerobject;
    LitString* name;
    LitObject* init_method;
    LitTable methods;
    LitTable static_fields;
    LitClass* super;
};

struct LitInstance
{
    LitObject innerobject;
    LitClass* klass;
    LitTable fields;
};

struct LitBoundMethod
{
    LitObject innerobject;
    LitValue receiver;
    LitValue method;
};

struct LitArray
{
    LitObject innerobject;
    LitDynListVal values;
};

struct LitVarargArray
{
    LitArray array;
};

struct LitUserdata
{
    LitObject innerobject;
    void* data;
    size_t size;
    LitCleanupFn cleanup_fn;
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

struct LitState
{
    struct
    {
        bool traceexecution;
        bool tracechunk;
        bool isreplmode;
        bool havedesttrace;
        LitIOStream* desttrace;
    } config;

    struct
    {
        LitObject* objects;
        LitTable strings;
        LitMap* modules;
        LitMap* globals;
        LitFiber* fiber;
        /* For garbage collection */
        size_t gray_count;
        size_t gray_capacity;
        LitObject** gray_stack;
        /* exec */
        LitCallFrame* frame;
        LitChunk* current_chunk;
        LitValue* vmregisteritems;
        LitValue* vmconstantvalues;
        LitValue* vmprivatevalues;
        LitUpvalue** upvalues;
        uint64_t* ip;
        uint64_t instruction;
    } vmstate;

    int64_t bytes_allocated;
    int64_t next_gc;
    bool allow_gc;
    LitIOStream* streamstdout;
    LitIOStream* streamstderr;
    LitErrorFn error_fn;
    LitValue* roots;
    size_t root_count;
    size_t root_capacity;
    LitScanner* scanner;
    LitParser* parser;
    LitEmitter* emitter;
    bool had_error;
    LitFunction* api_function;
    LitString* api_name;
    /* Mental note: */
    /* When adding another class here, DO NOT forget to mark it or it will be GC-ed */
    LitClass* class_class;
    LitClass* object_class;
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
};

struct LitResult
{
    LitStatusCode type;
    LitValue result;
};

struct LitToken
{
    const char* start;
    LitTokenType type;
    size_t length;
    size_t line;
    LitValue value;
};

/*
 * Expressions
 */
struct LitDynListExpr
{
    size_t capacity;
    size_t count;
    LitExpression** values;
};

struct LitPrivate
{
    bool initialized;
    bool constant;
};

struct LitDynListPriv
{
    size_t capacity;
    size_t count;
    LitPrivate* values;
};

struct LitLocal
{
    const char* name;
    size_t length;
    int depth;
    bool captured;
    bool constant;
    uint32_t reg;
};

struct LitDynListLoc
{
    size_t capacity;
    size_t count;
    LitLocal* values;
};

struct LitCompilerUpvalue
{
    uint32_t index;
    bool isLocal;
};

struct LitCompiler
{
    LitDynListLoc locals;
    int scope_depth;
    LitFunction* function;
    LitFunctionType type;
    LitCompilerUpvalue upvalues[UINT8_COUNT];
    uint64_t registers_used;
    LitCompiler* enclosing;
    bool skip_return;
    size_t loop_depth;
};

struct LitEmitter
{
    LitState* pstate;
    LitChunk* chunk;
    LitCompiler* compiler;
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

struct LitParseRule
{
    LitPrefixParseFn prefix;
    LitInfixParseFn infix;
    LitPrecedence precedence;
};

struct LitParser
{
    LitState* pstate;
    bool had_error;
    bool panic_mode;
    LitToken previous;
    LitToken current;
    LitCompiler* compiler;
    uint32_t exprrootcnt;
    uint32_t stmtrootcnt;
};

struct LitEmulatedFile
{
    const char* source;
    size_t position;
};

struct LitScanner
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

struct LitExpression
{
    LitExpressionType type;
    size_t line;
};

struct LitLiteralExpression
{
    LitExpression exprbase;
    LitValue value;
};

struct LitBinaryExpression
{
    LitExpression exprbase;
    LitExpression* left;
    LitExpression* right;
    LitTokenType op;
    bool ignore_left;
};

struct LitUnaryExpression
{
    LitExpression exprbase;
    LitExpression* right;
    LitTokenType op;
};

struct LitVarExpression
{
    LitExpression exprbase;
    const char* name;
    size_t length;
};

struct LitAssignExpression
{
    LitExpression exprbase;
    LitExpression* to;
    LitExpression* value;
};

struct LitCallExpression
{
    LitExpression exprbase;
    LitExpression* excallee;
    LitDynListExpr args;
    LitExpression* init;
};

struct LitGetExpression
{
    LitExpression exprbase;
    LitExpression* where;
    const char* name;
    size_t length;
    int jump;
    bool ignore_emit;
    bool ignore_result;
};

struct LitSetExpression
{
    LitExpression exprbase;
    LitExpression* where;
    const char* name;
    size_t length;
    LitExpression* value;
};

struct LitParameter
{
    const char* name;
    size_t length;
    uint32_t reg;
    LitExpression* default_value;
};

struct LitDynListParam
{
    size_t capacity;
    size_t count;
    LitParameter* values;
};

struct LitArrayExpression
{
    LitExpression exprbase;
    LitDynListExpr values;
};

struct LitObjectExpression
{
    LitExpression exprbase;
    LitDynListVal keys;
    LitDynListExpr values;
};

struct LitSubscriptExpression
{
    LitExpression exprbase;
    LitExpression* array;
    LitExpression* index;
};

struct LitThisExpression
{
    LitExpression exprbase;
};

struct LitSuperExpression
{
    LitExpression exprbase;
    LitString* method;
    bool ignore_emit;
    bool ignore_result;
};

struct LitRangeExpression
{
    LitExpression exprbase;
    LitExpression* from;
    LitExpression* to;
};

struct LitTernaryExpression
{
    LitExpression exprbase;
    LitExpression* condition;
    LitExpression* if_branch;
    LitExpression* else_branch;
};

struct LitInterpolationExpression
{
    LitExpression exprbase;
    LitDynListExpr expressions;
};

struct LitReferenceExpression
{
    LitExpression exprbase;
    LitExpression* to;
};

struct LitExpressionStatement
{
    LitExpression exprbase;
    LitExpression* expression;
};

struct LitBlockStatement
{
    LitExpression exprbase;
    LitDynListExpr statements;
};

struct LitVarStatement
{
    LitExpression exprbase;
    const char* name;
    size_t length;
    bool constant;
    LitExpression* init;
};

struct LitIfStatement
{
    LitExpression exprbase;
    LitExpression* condition;
    LitExpression* if_branch;
    LitExpression* else_branch;
    LitDynListExpr* elseif_conditions;
    LitDynListExpr* elseif_branches;
};

struct LitWhileStatement
{
    LitExpression exprbase;
    LitExpression* condition;
    LitExpression* body;
};

struct LitForStatement
{
    LitExpression exprbase;
    LitExpression* init;
    LitExpression* var;
    LitExpression* condition;
    LitExpression* increment;
    LitExpression* body;
    bool c_style;
};

struct LitContinueStatement
{
    LitExpression exprbase;
};

struct LitBreakStatement
{
    LitExpression exprbase;
};

struct LitFunctionStatement
{
    LitExpression exprbase;
    const char* name;
    size_t length;
    LitDynListParam parameters;
    LitExpression* body;
    bool exported;
};

struct LitReturnStatement
{
    LitExpression exprbase;
    LitExpression* expression;
};

struct LitMethodStatement
{
    LitExpression exprbase;
    LitString* name;
    LitDynListParam parameters;
    LitExpression* body;
    bool is_static;
};

struct LitClassStatement
{
    LitExpression exprbase;
    LitString* name;
    LitString* parent;
    LitDynListExpr fields;
};

struct LitFieldStatement
{
    LitExpression exprbase;
    LitString* name;
    LitExpression* getter;
    LitExpression* setter;
    bool is_static;
};

struct LitFileData
{
    char* path;
    FILE* file;
};

#include "prot.inc"

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
            fprintf(stderr, "[%s:%i] Out of memory\n", __FILE__, __LINE__);
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

void lit_iostream_initvars(LitIOStream* pr, LitIOStrMode mode)
{
    pr->fromstack = false;
    pr->wrmode = LIT_IOSTRMODE_UNDEFINED;
    pr->shouldclose = false;
    pr->shouldflush = false;
    pr->stringtaken = false;
    pr->shortenvalues = false;
    pr->jsonmode = false;
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

bool lit_iostream_writestringl(LitIOStream* pr, const char* estr, size_t elen)
{
    /* fprintf(stderr, "writestringl: (%d) <<<%.*s>>>\n", elen, elen, estr); */
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

bool lit_iostream_writestring(LitIOStream* pr, const char* estr)
{
    return lit_iostream_writestringl(pr, estr, strlen(estr));
}

bool lit_iostream_writechar(LitIOStream* pr, int b)
{
    char ch;
    if(pr->wrmode == LIT_IOSTRMODE_STRING)
    {
        ch = b;
        lit_iostream_writestringl(pr, &ch, 1);
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
            lit_iostream_writestring(pr, "\\\'");
        }
        break;
        case '\"':
        {
            lit_iostream_writestring(pr, "\\\"");
        }
        break;
        case '\\':
        {
            lit_iostream_writestring(pr, "\\\\");
        }
        break;
        case '\b':
        {
            lit_iostream_writestring(pr, "\\b");
        }
        break;
        case '\f':
        {
            lit_iostream_writestring(pr, "\\f");
        }
        break;
        case '\n':
        {
            lit_iostream_writestring(pr, "\\n");
        }
        break;
        case '\r':
        {
            lit_iostream_writestring(pr, "\\r");
        }
        break;
        case '\t':
        {
            lit_iostream_writestring(pr, "\\t");
        }
        break;
        case 0:
        {
            lit_iostream_writestring(pr, "\\0");
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

bool lit_iostream_writequotedstring(LitIOStream* pr, const char* str, size_t len, bool withquot)
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

bool lit_iostream_printf(LitIOStream* pr, const char* fmt, ...)
{
    bool b;
    va_list va;
    va_start(va, fmt);
    b = lit_iostream_vwritefmt(pr, fmt, va);
    va_end(va);
    return b;
}

bool lit_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool lit_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

void* lit_reallocate(LitState* state, void* pointer, size_t oldsize, size_t newsize)
{
    state->bytes_allocated += (int64_t)newsize - (int64_t)oldsize;
    if(newsize > oldsize)
    {
#ifdef LIT_CONFIG_STRESSTESTGC
        lit_collect_garbage(state);
#endif
        if(state->bytes_allocated > state->next_gc)
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
    fprintf(stderr, "(%s) %p free %s\n", lit_tostring_typename(object->type), (void*)object, lit_tostring_typename(object->type));
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
            LitFunction* function = (LitFunction*)object;
            lit_chunk_destroy(&function->chunk);
            LIT_GC_FREEOBJECT(state, LitFunction, object);
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
            LitClosurePrototype* clsproto = (LitClosurePrototype*)object;
            lit_sysmem_free(clsproto->indexes);
            lit_sysmem_free(clsproto->local);
            LIT_GC_FREEOBJECT(state, LitClosurePrototype, object);
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
            lit_free_table(&klass->methods);
            lit_free_table(&klass->static_fields);
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
            LIT_GC_FREEOBJECT(state, LitBoundMethod, object);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            lit_vallist_destroy(&((LitArray*)object)->values);
            LIT_GC_FREEOBJECT(state, LitArray, object);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            lit_vallist_destroy(&((LitVarargArray*)object)->array.values);
            LIT_GC_FREEOBJECT(state, LitVarargArray, object);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_free_table(&((LitMap*)object)->values);
            LIT_GC_FREEOBJECT(state, LitMap, object);
            break;
        }
        case LIT_OBJ_USERDATA:
        {
            LitUserdata* data = (LitUserdata*)object;
            if(data->cleanup_fn != NULL)
            {
                data->cleanup_fn(state, data, false);
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
    lit_sysmem_free(state->vmstate.gray_stack);
    state->vmstate.gray_capacity = 0;
}

void lit_mark_object(LitState* state, LitObject* object)
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
    if(state->vmstate.gray_capacity < state->vmstate.gray_count + 1)
    {
        state->vmstate.gray_capacity = LIT_GROW_CAPACITY(state->vmstate.gray_capacity);
        state->vmstate.gray_stack = (LitObject**)lit_sysmem_realloc(state->vmstate.gray_stack, sizeof(LitObject*) * state->vmstate.gray_capacity);
    }
    state->vmstate.gray_stack[state->vmstate.gray_count++] = object;
}

void lit_mark_value(LitState* state, LitValue value)
{
    if(lit_value_isobject(value))
    {
        lit_mark_object(state, lit_value_asobject(value));
    }
}

void mark_roots(LitState* state)
{
    size_t i;
    for(i = 0; i < state->root_count; i++)
    {
        lit_mark_value(state, state->roots[i]);
    }
    lit_mark_object(state, (LitObject*)state->vmstate.fiber);
    lit_mark_object(state, (LitObject*)state->class_class);
    lit_mark_object(state, (LitObject*)state->object_class);
    lit_mark_object(state, (LitObject*)state->number_class);
    lit_mark_object(state, (LitObject*)state->string_class);
    lit_mark_object(state, (LitObject*)state->bool_class);
    lit_mark_object(state, (LitObject*)state->function_class);
    lit_mark_object(state, (LitObject*)state->fiber_class);
    lit_mark_object(state, (LitObject*)state->module_class);
    lit_mark_object(state, (LitObject*)state->array_class);
    lit_mark_object(state, (LitObject*)state->map_class);
    lit_mark_object(state, (LitObject*)state->range_class);
    lit_mark_object(state, (LitObject*)state->api_name);
    lit_mark_object(state, (LitObject*)state->api_function);
    lit_table_markentries(&state->vmstate.modules->values);
    lit_table_markentries(&state->vmstate.globals->values);
}

void mark_array(LitState* state, LitDynListVal* array)
{
    size_t i;
    for(i = 0; i < array->count; i++)
    {
        lit_mark_value(state, array->values[i]);
    }
}

void blacken_object(LitState* state, LitObject* object)
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
            if(data->cleanup_fn != NULL)
            {
                data->cleanup_fn(state, data, true);
            }
            break;
        }
        case LIT_OBJ_FUNCSCRIPT:
        {
            LitFunction* function = (LitFunction*)object;
            lit_mark_object(state, (LitObject*)function->name);
            mark_array(state, &function->chunk.constantlist);
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
                lit_mark_value(state, fiber->registeritems[i]);
            }
            for(i = 0; i < fiber->framecount; i++)
            {
                LitCallFrame* frame = &fiber->framevals[i];
                if(frame->closure != NULL)
                {
                    lit_mark_object(state, (LitObject*)frame->closure);
                }
                else
                {
                    lit_mark_object(state, (LitObject*)frame->function);
                }
            }
            for(upvalue = fiber->open_upvalues; upvalue != NULL; upvalue = upvalue->next)
            {
                lit_mark_object(state, (LitObject*)upvalue);
            }
            lit_mark_value(state, fiber->error);
            lit_mark_object(state, (LitObject*)fiber->module);
            lit_mark_object(state, (LitObject*)fiber->parent);
            break;
        }
        case LIT_OBJ_MODULE:
        {
            size_t i;
            LitModule* module;
            module = (LitModule*)object;
            lit_mark_value(state, module->returnvalue);
            lit_mark_object(state, (LitObject*)module->name);
            lit_mark_object(state, (LitObject*)module->mainfunction);
            lit_mark_object(state, (LitObject*)module->mainfiber);
            lit_mark_object(state, (LitObject*)module->privatenames);
            for(i = 0; i < module->privatecount; i++)
            {
                lit_mark_value(state, module->privatevalues[i]);
            }
            break;
        }
        case LIT_OBJ_FUNCCLOSURE:
        {
            size_t i;
            LitFuncClosure* closure = (LitFuncClosure*)object;
            lit_mark_object(state, (LitObject*)closure->function);
            /* Check for NULL is needed for a really specific gc-case */
            if(closure->upvalues != NULL)
            {
                for(i = 0; i < closure->upvaluecount; i++)
                {
                    lit_mark_object(state, (LitObject*)closure->upvalues[i]);
                }
            }
            break;
        }
        case LIT_OBJ_CLSPROTOTYPE:
        {
            lit_mark_object(state, (LitObject*)((LitFuncClosure*)object)->function);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            lit_mark_value(state, ((LitUpvalue*)object)->closed);
            break;
        }
        case LIT_OBJ_CLASS:
        {
            LitClass* klass = (LitClass*)object;
            lit_mark_object(state, (LitObject*)klass->name);
            lit_mark_object(state, (LitObject*)klass->super);
            lit_table_markentries(&klass->methods);
            lit_table_markentries(&klass->static_fields);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            LitInstance* instance = (LitInstance*)object;
            lit_mark_object(state, (LitObject*)instance->klass);
            lit_table_markentries(&instance->fields);
            break;
        }
        case LIT_OBJ_FUNCBOUNDMETHOD:
        {
            LitBoundMethod* boundmethod = (LitBoundMethod*)object;
            lit_mark_value(state, boundmethod->receiver);
            lit_mark_value(state, boundmethod->method);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            mark_array(state, &((LitArray*)object)->values);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            mark_array(state, &((LitVarargArray*)object)->array.values);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_table_markentries(&((LitMap*)object)->values);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = (LitField*)object;
            lit_mark_object(state, (LitObject*)field->getter);
            lit_mark_object(state, (LitObject*)field->setter);
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            lit_mark_value(state, *((LitReference*)object)->slot);
            break;
        }
        default:
        {
            lit_vm_raisefatalerror(state, "unknown object with type %i", object->type);
            break;
        }
    }
}

void trace_references(LitState* state)
{
    while(state->vmstate.gray_count > 0)
    {
        LitObject* object = state->vmstate.gray_stack[--state->vmstate.gray_count];
        blacken_object(state, object);
    }
}

void sweep(LitState* state)
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
    if(!state->allow_gc)
    {
        return 0;
    }
    state->allow_gc = false;
    uint64_t before = state->bytes_allocated;
#ifdef LIT_CONFIG_LOGGC
    fprintf(stderr, "-- gc begin\n");
    clock_t t = clock();
#endif
    mark_roots(state);
    trace_references(state);
    lit_table_remove_white(&state->vmstate.strings);
    sweep(state);
    state->next_gc = state->bytes_allocated * LIT_GC_HEAP_GROW_FACTOR;
    state->allow_gc = true;
    uint64_t collected = before - state->bytes_allocated;
#ifdef LIT_CONFIG_LOGGC
    fprintf(stderr, "-- gc end. Collected %imb (%ib) in %gms\n", ((int)((collected / 1024.0 + 0.5) / 10)) * 10, collected, (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
#endif
    return collected;
}

/* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float */
int lit_closest_power_of_two(int n)
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

int lit_strutil_utfgetcountdecode(uint8_t byte)
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

int lit_strutil_utfgetcountencode(int value)
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

int lit_strutil_utfencode(int value, uint8_t* bytes)
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

int lit_strutil_utfdecode(const uint8_t* bytes, uint32_t length)
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

int lit_strutil_utfucharoffset(const char* str, int index)
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

bool IS_STRING(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_STRING);
}

#define IS_FUNCTION(value) lit_value_isobjtype(value, LIT_OBJ_FUNCSCRIPT)
#define IS_NATIVE_METHOD(value) lit_value_isobjtype(value, LIT_OBJ_FUNCNATMETHOD)
#define IS_MODULE(value) lit_value_isobjtype(value, LIT_OBJ_MODULE)
#define IS_CLOSURE(value) lit_value_isobjtype(value, LIT_OBJ_FUNCCLOSURE)
#define IS_CLOSURE_PROTOTYPE(value) lit_value_isobjtype(value, LIT_OBJ_CLSPROTOTYPE)
#define IS_CLASS(value) lit_value_isobjtype(value, LIT_OBJ_CLASS)
bool lit_value_isinstance(LitValue value)
{
    return lit_value_isobjtype(value, LIT_OBJ_INSTANCE);
}

#define IS_ARRAY(value) (lit_value_isobjtype(value, LIT_OBJ_ARRAY) || lit_value_isobjtype(value, LIT_OBJ_VARARGARRAY))
#define IS_VARARG_ARRAY(value) lit_value_isobjtype(value, LIT_OBJ_VARARGARRAY)
#define IS_RANGE(value) lit_value_isobjtype(value, LIT_OBJ_RANGE)
#define IS_FIELD(value) lit_value_isobjtype(value, LIT_OBJ_FIELD)
#define IS_REFERENCE(value) lit_value_isobjtype(value, LIT_OBJ_REFERENCE)

double lit_value_asnumber(LitValue value)
{
    return value.as.numval;
}

bool lit_value_asbool(LitValue v)
{
    return (v.as.boolval);
}

LitObject* lit_value_asobject(LitValue v)
{
    return (v.as.obj);
}

LitString* AS_STRING(LitValue value)
{
    return ((LitString*)lit_value_asobject(value));
}

LitFunction* AS_FUNCTION(LitValue value)
{
    return ((LitFunction*)lit_value_asobject(value));
}

LitFuncNative* AS_NATIVE_FUNCTION(LitValue value)
{
    return ((LitFuncNative*)lit_value_asobject(value));
}


LitFuncNative* AS_NATIVE_METHOD(LitValue value)
{
    return ((LitFuncNative*)lit_value_asobject(value));
}


LitModule* AS_MODULE(LitValue value)
{
    return ((LitModule*)lit_value_asobject(value));
}

LitFuncClosure* AS_CLOSURE(LitValue value)
{
    return ((LitFuncClosure*)lit_value_asobject(value));
}

LitClosurePrototype* AS_CLOSURE_PROTOTYPE(LitValue value)
{
    return ((LitClosurePrototype*)lit_value_asobject(value));
}

LitUpvalue* AS_UPVALUE(LitValue value)
{
    return ((LitUpvalue*)lit_value_asobject(value));
}

LitClass* AS_CLASS(LitValue value)
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

LitBoundMethod* AS_BOUND_METHOD(LitValue value)
{
    return ((LitBoundMethod*)lit_value_asobject(value));
}

LitUserdata* AS_USERDATA(LitValue value)
{
    return ((LitUserdata*)lit_value_asobject(value));
}

LitRange* AS_RANGE(LitValue value)
{
    return ((LitRange*)lit_value_asobject(value));
}

LitField* AS_FIELD(LitValue value)
{
    return ((LitField*)lit_value_asobject(value));
}

LitFiber* AS_FIBER(LitValue value)
{
    return ((LitFiber*)lit_value_asobject(value));
}

LitReference* AS_REFERENCE(LitValue value)
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
    LitString* interned = lit_table_find_string(&state->vmstate.strings, chars, length, hash);
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
    res = lit_table_find_string(&state->vmstate.strings, chars, length, hash);
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
        i += lit_strutil_utfgetcountdecode(string->strbuf.data[i]);
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
    codepoint = lit_strutil_utfdecode((uint8_t*)string->strbuf.data + index, string->strbuf.length - index);
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
    int codepoint = lit_strutil_utfdecode((uint8_t*)string->strbuf.data + index, string->strbuf.length - index);
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
    length = lit_strutil_utfgetcountencode(value);
    res = lit_string_makeemptystring(state, length, false);
    lit_strutil_utfencode(value, (uint8_t*)res->strbuf.data);
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
        length += lit_strutil_utfgetcountdecode(from[start + i]);
    }
    res = lit_string_makeemptystring(state, length, false);
    to = (uint8_t*)res->strbuf.data;
    for(i = 0; i < count; i++)
    {
        index = start + i;
        codepoint = lit_strutil_utfdecode(from + index, source->strbuf.length - index);
        if(codepoint != -1)
        {
            to += lit_strutil_utfencode(codepoint, to);
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

void lit_string_numbertostream(LitIOStream* pr, double dn)
{
    if(isnan(dn))
    {
        lit_iostream_writestring(pr, "nan");
        return;
    }
    if(isinf(dn))
    {
        if(dn > 0.0)
        {
            lit_iostream_writestring(pr, "infinity");
            return;
        }
        else
        {
            lit_iostream_writestring(pr, "-infinity");
            return;
        }
    }
    lit_iostream_printf(pr, "%g", dn);
}


LitValue lit_string_numbertostring(LitState* state, double value)
{
    LitIOStream pr;
    lit_iostream_makestackstring(&pr);
    lit_string_numbertostream(&pr, value);
    return lit_value_fromobject(lit_iostream_takestring(state, &pr));
}

LitValue lit_string_valformat(LitState* state, const char* format, ...)
{
    va_list arglist;    
    bool wasallowed;
    const char* c;
    LitString* result;
    wasallowed = state->allow_gc;
    state->allow_gc = false;
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
                LitString* string = AS_STRING(va_arg(arglist, LitValue));
                if(string != NULL)
                {
                    lit_string_appendlen(result, string->strbuf.data, string->strbuf.length);
                    break;
                }
                goto defaultendingcopying;
            }
            case '#':
            {
                LitString* string = AS_STRING(lit_string_numbertostring(state, va_arg(arglist, double)));
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
    state->allow_gc = wasallowed;
    return lit_value_fromobject(result);
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
        if(IS_STRING(a) && IS_STRING(b))
        {
            LitString* as = AS_STRING(a);
            LitString* bs = AS_STRING(b);
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
        if(IS_STRING(a) || IS_STRING(b))
        {
            return false;
        }
    }
    tmpargs[0] = b;
    return !lit_is_falsey(lit_state_findandcallmethod(state, a, lit_string_copy(state, "=="), tmpargs, 1).result);
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

void lit_value_printobjmap(LitIOStream* pr, LitObject* self, LitTable* tab)
{
    bool didprint;
    size_t i;
    size_t index;
    size_t valueamount;
    LitValue field;
    LitTable* values;
    LitTabEntry* entry;
    values = tab;
    valueamount = values->htcount;
    lit_iostream_writestring(pr, "{");
    if(valueamount > 0)
    {
        i = 0;
        index = 0;
        do
        {
            entry = &values->htentries[index];
            index++;
            didprint =false;
            if(entry->key != NULL)
            {
                /* Special hidden key */
                field = entry->value;
                lit_iostream_writestringl(pr, entry->key->strbuf.data, entry->key->strbuf.length);
                lit_iostream_writestring(pr, ": ");
                if((lit_value_ismap(field) && (lit_value_asobject(field) == self)))
                {
                    lit_iostream_writestring(pr, "<recursion>");
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
                    lit_iostream_writestring(pr, ",");
                }
            }
        } while(i < valueamount);
    }
    lit_iostream_writestring(pr, "}");
}

void lit_value_printobjinstance(LitIOStream* pr, LitClass* klass, LitInstance* self)
{
    lit_iostream_printf(pr, "<instance of %s: ", klass->name->strbuf.data);
    lit_value_printobjmap(pr, (LitObject*)self, &self->fields);
    lit_iostream_printf(pr, "  >", klass->name->strbuf.data);
}

void lit_value_printobjarray(LitIOStream* pr, LitArray* self)
{
    size_t i;
    size_t valueamount;
    LitValue field;
    LitDynListVal* values;
    valueamount = self->values.count;
    values = &self->values;
    lit_iostream_writestring(pr, "[");
    if(values->count > 0)
    {
        for(i = 0; i < valueamount; i++)
        {
            field = values->values[i];
            if(IS_ARRAY(field) && lit_value_asarray(field) == self)
            {
                lit_iostream_writestring(pr, "<recursion>");
            }
            else
            {
                lit_value_printvalue(pr, field, true);
            }
            if((i + 1) < valueamount)
            {
                lit_iostream_writestring(pr, ", ");
            }
        }
    }
    lit_iostream_writestring(pr, "]");
}

void lit_value_printobject(LitIOStream* pr, LitValue value, bool reprmode)
{
    switch(OBJECT_TYPE(value))
    {
        case LIT_OBJ_STRING:
        {
            LitString* str;
            str = AS_STRING(value);
            if(reprmode)
            {
                lit_iostream_writequotedstring(pr, str->strbuf.data, str->strbuf.length, true);
            }
            else
            {
                lit_iostream_writestringl(pr, str->strbuf.data, str->strbuf.length);
            }
        }
        break;
        case LIT_OBJ_FUNCSCRIPT:
        {
            lit_iostream_printf(pr, "function %s", AS_FUNCTION(value)->name->strbuf.data);
        }
        break;
        case LIT_OBJ_FUNCCLOSURE:
        {
            lit_iostream_printf(pr, "closure %s", AS_CLOSURE(value)->function->name->strbuf.data);
        }
        break;
        case LIT_OBJ_CLSPROTOTYPE:
        {
            lit_iostream_printf(pr, "closure %s", AS_CLOSURE_PROTOTYPE(value)->function->name->strbuf.data);
        }
        break;
        case LIT_OBJ_FUNCNATIVE:
        {
            lit_iostream_printf(pr, "function %s", AS_NATIVE_FUNCTION(value)->name->strbuf.data);
        }
        break;
        case LIT_OBJ_FUNCNATMETHOD:
        {
            lit_iostream_printf(pr, "function %s", AS_NATIVE_METHOD(value)->name->strbuf.data);
        }
        break;
        case LIT_OBJ_FIBER:
        {
            lit_iostream_printf(pr, "fiber");
        }
        break;
        case LIT_OBJ_MODULE:
        {
            lit_iostream_printf(pr, "module %s", AS_MODULE(value)->name->strbuf.data);
        }
        break;
        case LIT_OBJ_UPVALUE:
        {
            LitUpvalue* upvalue = AS_UPVALUE(value);
            if(upvalue->location == NULL)
            {
                lit_value_printvalue(pr, upvalue->closed, reprmode);
            }
            else
            {
                lit_value_printobject(pr, *upvalue->location, reprmode);
            }
        }
        break;
        case LIT_OBJ_CLASS:
        {
            lit_iostream_printf(pr, "class %s", AS_CLASS(value)->name->strbuf.data);
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
            lit_value_printvalue(pr, AS_BOUND_METHOD(value)->method, reprmode);
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
            LitMap* map = lit_value_asmap(value);
            lit_value_printobjmap(pr, (LitObject*)map, &map->values);
        }
        break;

        case LIT_OBJ_USERDATA:
        {
            lit_iostream_printf(pr, "userdata");
        }
        break;

        case LIT_OBJ_RANGE:
        {
            LitRange* range = AS_RANGE(value);
            lit_iostream_printf(pr, "%g .. %g", range->from, range->to);
        }
        break;
        case LIT_OBJ_FIELD:
        {
            lit_iostream_printf(pr, "field");
        }
        break;
        case LIT_OBJ_REFERENCE:
        {
            lit_iostream_printf(pr, "reference => ");
            LitValue* slot = AS_REFERENCE(value)->slot;
            if(slot == NULL)
            {
                lit_iostream_printf(pr, "null");
            }
            else
            {
                lit_value_printvalue(pr, *slot, reprmode);
            }
        }
        break;
        default:
        {
            lit_iostream_printf(pr, "[unknown object %p %i]", &value, OBJECT_TYPE(value));
        }
        break;
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
        lit_string_numbertostream(pr, lit_value_asnumber(value));
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

void lit_dynlistval_ensuresize(LitDynListVal* values, size_t size)
{
    lit_dynlistval_ensureactualsize(values, size);
    if(values->count < size)
    {
        values->count = size;
    }
}

void lit_dynlistval_ensureactualsize(LitDynListVal* values, size_t size)
{
    size_t i;
    if(values->capacity < size)
    {
        size_t oldcapacity = values->capacity;
        values->capacity = size;
        values->values = (LitValue*)lit_sysmem_realloc(values->values, sizeof(LitValue) * (size));
        for(i = oldcapacity; i < size; i++)
        {
            values->values[i] = lit_value_makenull();
        }
    }
}

const char* lit_get_value_type(LitValue value)
{
    if(lit_value_isbool(value))
    {
        return "bool";
    }
    else if(lit_value_isnull(value))
    {
        return "null";
    }
    else if(lit_value_isnumber(value))
    {
        return "number";
    }
    else if(lit_value_isobject(value))
    {
        return lit_tostring_typename(OBJECT_TYPE(value));
    }
    return "unknown";
}

void lit_api_init(LitState* state)
{
    state->api_name = lit_string_copylen(state, "c", 1);
    state->api_function = NULL;
}

void lit_api_destroy(LitState* state)
{
    state->api_name = NULL;
    state->api_function = NULL;
}

bool lit_state_getglobaltovalue(LitState* state, LitString* name, LitValue* dest)
{
    return lit_table_getentry(&state->vmstate.globals->values, name, dest);
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

LitFunction* lit_state_getglobalfunction(LitState* state, LitString* name)
{
    LitValue val;
    if(!lit_state_getglobaltovalue(state, name, &val))
    {
        return NULL;
    }
    if(IS_FUNCTION(val))
    {
        return AS_FUNCTION(val);
    }
    return NULL;
}

void lit_state_setglobal(LitState* state, LitString* name, LitValue value)
{
    lit_state_pushroot(state, (LitObject*)name);
    lit_state_pushvalueroot(state, value);
    lit_table_set(&state->vmstate.globals->values, name, value);
    lit_state_poproots(state, 2);
}

void lit_state_defnative(LitState* state, const char* name, LitNativeFunctionFn native)
{
    lit_state_pushroot(state, (LitObject*)lit_string_copy(state, name));
    lit_state_pushroot(state, (LitObject*)lit_object_makenativefunc(state, native, AS_STRING(lit_state_peekroot(state, 0))));
    lit_table_set(&state->vmstate.globals->values, AS_STRING(lit_state_peekroot(state, 1)), lit_state_peekroot(state, 0));
    lit_state_poproots(state, 2);
}


double lit_args_checknumber(LitState* state, const char* sourcefname, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !lit_value_isnumber(args[id]))
    {
        lit_vm_raisefatalerror(state, "in %s: Expected a number as argument #%i, got a %s", sourcefname, (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return lit_value_asnumber(args[id]);
}

const char* lit_args_checkstring(LitState* state, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(state, "expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return AS_STRING(args[id])->strbuf.data;
}

const char* lit_args_getstring(LitValue* args, size_t argc, size_t id, const char* def)
{
    if(argc <= id || !IS_STRING(args[id]))
    {
        return def;
    }
    return AS_STRING(args[id])->strbuf.data;
}

LitString* lit_args_checkobjstring(LitState* state, LitValue* args, size_t argc, size_t id)
{
    if(argc <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(state, "expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return AS_STRING(args[id]);
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

LitTabEntry* find_entry(LitTabEntry* entries, int capacity, LitString* key)
{
    uint32_t index = key->strhash % capacity;
    LitTabEntry* tombstone = NULL;
    while(true)
    {
        LitTabEntry* entry = &entries[index];
        if(entry->key == NULL)
        {
            if(lit_value_isnull(entry->value))
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else if(tombstone == NULL)
            {
                tombstone = entry;
            }
        }
        if(entry->key == key)
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
        entries[i].key = NULL;
        entries[i].value = lit_value_makenull();
    }
    table->htcount = 0;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->key == NULL)
        {
            continue;
        }
        destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->value = entry->value;
        table->htcount++;
    }
    lit_sysmem_free(table->htentries);
    table->htcapacity = capacity;
    table->htentries = entries;
}

bool lit_table_set(LitTable* table, LitString* key, LitValue value)
{
    if(table->htcount + 1 > (table->htcapacity + 1) * TABLE_MAX_LOAD)
    {
        int capacity = LIT_GROW_CAPACITY(table->htcapacity + 1) - 1;
        lit_table_adjustcapacity(table, capacity);
    }
    LitTabEntry* entry = find_entry(table->htentries, table->htcapacity, key);
    bool isnew = entry->key == NULL;
    if(isnew && lit_value_isnull(entry->value))
    {
        table->htcount++;
    }
    entry->key = key;
    entry->value = value;
    return isnew;
}

bool lit_table_getentry(LitTable* table, LitString* key, LitValue* value)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->htentries, table->htcapacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    *value = entry->value;
    return true;
}

bool lit_table_getslot(LitTable* table, LitString* key, LitValue** value)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->htentries, table->htcapacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    *value = &entry->value;
    return true;
}

bool lit_table_delete(LitTable* table, LitString* key)
{
    if(table->htcount == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->htentries, table->htcapacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    entry->key = NULL;
    entry->value = lit_value_makebool(true);
    return true;
}

LitString* lit_table_find_string(LitTable* table, const char* chars, size_t length, uint32_t hash)
{
    size_t entlen;
    uint32_t enthash;
    const char* entstr;
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
        if(entry->key == NULL)
        {
            if(lit_value_isnull(entry->value))
            {
                return NULL;
            }
        }
        else
        {
            entlen = entry->key->strbuf.length;
            enthash = entry->key->strhash;
            entstr = entry->key->strbuf.data;
            if(entlen == length && /* enthash == hash && */ memcmp(entstr, chars, length) == 0)
            {
                return entry->key;
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
        if(entry->key != NULL)
        {
            lit_table_set(to, entry->key, entry->value);
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
        if(entry->key != NULL && !lit_table_getentry(to, entry->key, &fake))
        {
            lit_table_set(to, entry->key, entry->value);
        }
    }
}

void lit_table_remove_white(LitTable* table)
{
    int i;
    LitObject* obj;
    LitTabEntry* entry;
    for(i = 0; i <= table->htcapacity; i++)
    {
        entry = &table->htentries[i];
        if(entry->key != NULL)
        {
            obj = (LitObject*)entry->key;
            if(!obj->marked)
            {
                lit_table_delete(table, entry->key);
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
        lit_mark_object(state, (LitObject*)entry->key);
        lit_mark_value(state, entry->value);
    }
}

void lit_chunk_init(LitChunk* chunk)
{
    lit_chunk_reset(chunk);
    lit_vallist_init(&chunk->constantlist);
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
    lit_vallist_destroy(&chunk->constantlist);
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
    lit_vallist_push(&chunk->constantlist, constant);
    lit_state_poproot(state);
    return chunk->constantlist.count - 1;
}

size_t lit_chunk_getline(LitChunk* chunk, size_t offset)
{
    size_t i;
    if(!chunk->haslineinfo)
    {
        return 0;
    }
    size_t rle = 0;
    size_t line = 0;
    size_t index = 0;
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
            rle = chunk->lines[index + 1];
            if(rle > 0)
            {
                rle--;
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

void lit_exprlist_init(LitDynListExpr* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_exprlist_destroy(LitDynListExpr* array)
{
    lit_sysmem_free(array->values);
    lit_exprlist_init(array);
}

void lit_exprlist_push(LitDynListExpr* array, LitExpression* value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitExpression**)lit_sysmem_realloc(array->values, sizeof(LitExpression*) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_paramlist_init(LitDynListParam* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_paramlist_destroy(LitDynListParam* array)
{
    lit_sysmem_free(array->values);
    lit_paramlist_init(array);
}

void lit_paramlist_push(LitDynListParam* array, LitParameter value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitParameter*)lit_sysmem_realloc(array->values, sizeof(LitParameter) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
};

void lit_ast_destroyparamlist(LitState* state, LitDynListParam* parameters)
{
    size_t i;
    for(i = 0; i < parameters->count; i++)
    {
        lit_ast_destroyexpression(state, parameters->values[i].default_value);
    }
    lit_paramlist_destroy(parameters);
}

void lit_ast_destroyexprlist(LitState* state, LitDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->count; i++)
    {
        lit_ast_destroyexpression(state, expressions->values[i]);
    }
    lit_exprlist_destroy(expressions);
}

void lit_ast_destroystmtlist(LitState* state, LitDynListExpr* statements)
{
    size_t i;
    if(statements == NULL)
    {
        return;
    }
    for(i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }
    lit_exprlist_destroy(statements);
}

void lit_ast_destroyexpression(LitState* state, LitExpression* expression)
{
    if(expression == NULL)
    {
        return;
    }
    switch(expression->type)
    {
        case LIT_ASTEXPRTYP_LITERAL:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_BINARY:
        {
            LitBinaryExpression* expr = (LitBinaryExpression*)expression;
            if(!expr->ignore_left)
            {
                lit_ast_destroyexpression(state, expr->left);
            }
            lit_ast_destroyexpression(state, expr->right);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_UNARY:
        {
            lit_ast_destroyexpression(state, ((LitUnaryExpression*)expression)->right);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_VAR:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_ASSIGN:
        {
            LitAssignExpression* expr = (LitAssignExpression*)expression;
            lit_ast_destroyexpression(state, expr->to);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_CALL:
        {
            LitCallExpression* expr = (LitCallExpression*)expression;
            lit_ast_destroyexpression(state, expr->excallee);
            lit_ast_destroyexpression(state, expr->init);
            lit_ast_destroyexprlist(state, &expr->args);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_GET:
        {
            lit_ast_destroyexpression(state, ((LitGetExpression*)expression)->where);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_SET:
        {
            LitSetExpression* expr = (LitSetExpression*)expression;
            lit_ast_destroyexpression(state, expr->where);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_LAMBDA:
        {
            LitFunctionStatement* expr = (LitFunctionStatement*)expression;
            lit_ast_destroyparamlist(state, &expr->parameters);
            lit_ast_destroystmt(state, expr->body);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_ARRAY:
        {
            lit_ast_destroyexprlist(state, &((LitArrayExpression*)expression)->values);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_OBJECT:
        {
            LitObjectExpression* map = (LitObjectExpression*)expression;
            lit_vallist_destroy(&map->keys);
            lit_ast_destroyexprlist(state, &map->values);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_SUBSCRIPT:
        {
            LitSubscriptExpression* expr = (LitSubscriptExpression*)expression;
            lit_ast_destroyexpression(state, expr->array);
            lit_ast_destroyexpression(state, expr->index);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_THIS:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_SUPER:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_RANGE:
        {
            LitRangeExpression* expr = (LitRangeExpression*)expression;
            lit_ast_destroyexpression(state, expr->from);
            lit_ast_destroyexpression(state, expr->to);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_TERNARY:
        {
            LitTernaryExpression* expr = (LitTernaryExpression*)expression;
            lit_ast_destroyexpression(state, expr->condition);
            lit_ast_destroyexpression(state, expr->if_branch);
            lit_ast_destroyexpression(state, expr->else_branch);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_INTERPOLATION:
        {
            lit_ast_destroyexprlist(state, &((LitInterpolationExpression*)expression)->expressions);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_ASTEXPRTYP_REFERENCE:
        {
            lit_ast_destroyexpression(state, ((LitReferenceExpression*)expression)->to);
            lit_sysmem_free(expression);
            break;
        }
        default:
        {
            lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "unknown expression type %d", (int)expression->type);
            break;
        }
    }
}

LitExpression* lit_ast_allocexpression(uint64_t line, size_t size, LitExpressionType type)
{
    LitExpression* object = (LitExpression*)lit_sysmem_malloc(size);
    object->type = type;
    object->line = line;
    return object;
}

LitLiteralExpression* lit_ast_makeliteralexpr(size_t line, LitValue value)
{
    LitLiteralExpression* expression = (LitLiteralExpression*)lit_ast_allocexpression(line, sizeof(LitLiteralExpression), LIT_ASTEXPRTYP_LITERAL);
    expression->value = value;
    return expression;
}

LitBinaryExpression* lit_ast_makebinaryexpr(size_t line, LitExpression* left, LitExpression* right, LitTokenType op)
{
    LitBinaryExpression* expression = (LitBinaryExpression*)lit_ast_allocexpression(line, sizeof(LitBinaryExpression), LIT_ASTEXPRTYP_BINARY);
    expression->left = left;
    expression->right = right;
    expression->op = op;
    expression->ignore_left = false;
    return expression;
}

LitUnaryExpression* lit_ast_makeunaryexpr(size_t line, LitExpression* right, LitTokenType op)
{
    LitUnaryExpression* expression = (LitUnaryExpression*)lit_ast_allocexpression(line, sizeof(LitUnaryExpression), LIT_ASTEXPRTYP_UNARY);
    expression->right = right;
    expression->op = op;
    return expression;
}

LitVarExpression* lit_ast_makevarexpr(size_t line, const char* name, size_t length)
{
    LitVarExpression* expression = (LitVarExpression*)lit_ast_allocexpression(line, sizeof(LitVarExpression), LIT_ASTEXPRTYP_VAR);
    expression->name = name;
    expression->length = length;
    return expression;
}

LitAssignExpression* lit_ast_makeassignexpr(size_t line, LitExpression* to, LitExpression* value)
{
    LitAssignExpression* expression = (LitAssignExpression*)lit_ast_allocexpression(line, sizeof(LitAssignExpression), LIT_ASTEXPRTYP_ASSIGN);
    expression->to = to;
    expression->value = value;
    return expression;
}

LitCallExpression* lit_ast_makecallexpr(size_t line, LitExpression* callee)
{
    LitCallExpression* expression = (LitCallExpression*)lit_ast_allocexpression(line, sizeof(LitCallExpression), LIT_ASTEXPRTYP_CALL);
    expression->excallee = callee;
    expression->init = NULL;
    lit_exprlist_init(&expression->args);
    return expression;
}

LitGetExpression* lit_ast_makegetexpr(size_t line, LitExpression* where, const char* name, size_t length, bool questionable, bool ignore_result)
{
    LitGetExpression* expression = (LitGetExpression*)lit_ast_allocexpression(line, sizeof(LitGetExpression), LIT_ASTEXPRTYP_GET);
    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->ignore_emit = false;
    expression->jump = questionable ? 0 : -1;
    expression->ignore_result = ignore_result;
    return expression;
}

LitSetExpression* lit_ast_makesetexpr(size_t line, LitExpression* where, const char* name, size_t length, LitExpression* value)
{
    LitSetExpression* expression = (LitSetExpression*)lit_ast_allocexpression(line, sizeof(LitSetExpression), LIT_ASTEXPRTYP_SET);
    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->value = value;
    return expression;
}

LitFunctionStatement* lit_ast_makelambdaexpr(size_t line)
{
    LitFunctionStatement* expression = (LitFunctionStatement*)lit_ast_allocexpression(line, sizeof(LitFunctionStatement), LIT_ASTEXPRTYP_LAMBDA);
    expression->body = NULL;
    lit_paramlist_init(&expression->parameters);
    return expression;
}

LitArrayExpression* lit_ast_makearrayexpr(size_t line)
{
    LitArrayExpression* expression = (LitArrayExpression*)lit_ast_allocexpression(line, sizeof(LitArrayExpression), LIT_ASTEXPRTYP_ARRAY);
    lit_exprlist_init(&expression->values);
    return expression;
}

LitObjectExpression* lit_ast_makeobjectexpr(size_t line)
{
    LitObjectExpression* expression = (LitObjectExpression*)lit_ast_allocexpression(line, sizeof(LitObjectExpression), LIT_ASTEXPRTYP_OBJECT);
    lit_vallist_init(&expression->keys);
    lit_exprlist_init(&expression->values);
    return expression;
}

LitSubscriptExpression* lit_ast_makesubscriptexpr(size_t line, LitExpression* array, LitExpression* index)
{
    LitSubscriptExpression* expression = (LitSubscriptExpression*)lit_ast_allocexpression(line, sizeof(LitSubscriptExpression), LIT_ASTEXPRTYP_SUBSCRIPT);
    expression->array = array;
    expression->index = index;
    return expression;
}

LitThisExpression* lit_ast_makethisexpr(size_t line)
{
    return (LitThisExpression*)lit_ast_allocexpression(line, sizeof(LitThisExpression), LIT_ASTEXPRTYP_THIS);
}

LitSuperExpression* lit_ast_makesuperexpr(size_t line, LitString* method, bool ignore_result)
{
    LitSuperExpression* expression = (LitSuperExpression*)lit_ast_allocexpression(line, sizeof(LitSuperExpression), LIT_ASTEXPRTYP_SUPER);
    expression->method = method;
    expression->ignore_emit = false;
    expression->ignore_result = ignore_result;
    return expression;
}

LitRangeExpression* lit_ast_makerangeexpr(size_t line, LitExpression* from, LitExpression* to)
{
    LitRangeExpression* expression = (LitRangeExpression*)lit_ast_allocexpression(line, sizeof(LitRangeExpression), LIT_ASTEXPRTYP_RANGE);
    expression->from = from;
    expression->to = to;
    return expression;
}

LitTernaryExpression* lit_ast_maketernaryexpr(size_t line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch)
{
    LitTernaryExpression* expression = (LitTernaryExpression*)lit_ast_allocexpression(line, sizeof(LitTernaryExpression), LIT_ASTEXPRTYP_TERNARY);
    expression->condition = condition;
    expression->if_branch = if_branch;
    expression->else_branch = else_branch;
    return expression;
}

LitInterpolationExpression* lit_ast_makeinterpolationexpr(size_t line)
{
    LitInterpolationExpression* expression = (LitInterpolationExpression*)lit_ast_allocexpression(line, sizeof(LitInterpolationExpression), LIT_ASTEXPRTYP_INTERPOLATION);
    lit_exprlist_init(&expression->expressions);
    return expression;
}

LitReferenceExpression* lit_ast_makerefexpr(size_t line, LitExpression* to)
{
    LitReferenceExpression* expression = (LitReferenceExpression*)lit_ast_allocexpression(line, sizeof(LitReferenceExpression), LIT_ASTEXPRTYP_REFERENCE);
    expression->to = to;
    return expression;
}

void lit_ast_destroystmt(LitState* state, LitExpression* statement)
{
    if(statement == NULL)
    {
        return;
    }
    switch(statement->type)
    {
        case LIT_ASTEXPRTYP_EXPRESSION:
        {
            lit_ast_destroyexpression(state, ((LitExpressionStatement*)statement)->expression);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_BLOCK:
        {
            lit_ast_destroystmtlist(state, &((LitBlockStatement*)statement)->statements);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_VARDECL:
        {
            lit_ast_destroyexpression(state, ((LitVarStatement*)statement)->init);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_IF:
        {
            LitIfStatement* stmt = (LitIfStatement*)statement;
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroystmt(state, stmt->if_branch);
            lit_ast_destroyallocatedexprlist(state, stmt->elseif_conditions);
            lit_ast_destroyallocatedstmtlist(state, stmt->elseif_branches);
            lit_ast_destroystmt(state, stmt->else_branch);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_WHILE:
        {
            LitWhileStatement* stmt = (LitWhileStatement*)statement;
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroystmt(state, stmt->body);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_FOR:
        {
            LitForStatement* stmt = (LitForStatement*)statement;
            lit_ast_destroyexpression(state, stmt->increment);
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroyexpression(state, stmt->init);
            lit_ast_destroystmt(state, stmt->var);
            lit_ast_destroystmt(state, stmt->body);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_CONTINUE:
        {
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_BREAK:
        {
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            LitFunctionStatement* stmt = (LitFunctionStatement*)statement;
            lit_ast_destroystmt(state, stmt->body);
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_RETURN:
        {
            lit_ast_destroyexpression(state, ((LitReturnStatement*)statement)->expression);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_METHOD:
        {
            LitMethodStatement* stmt = (LitMethodStatement*)statement;
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_ast_destroystmt(state, stmt->body);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_CLASS:
        {
            lit_ast_destroystmtlist(state, &((LitClassStatement*)statement)->fields);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_ASTEXPRTYP_FIELD:
        {
            LitFieldStatement* stmt = (LitFieldStatement*)statement;
            lit_ast_destroystmt(state, stmt->getter);
            lit_ast_destroystmt(state, stmt->setter);
            lit_sysmem_free(statement);
            break;
        }
        default:
        {
            lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "unknown statement type %d", (int)statement->type);
            break;
        }
    }
}

LitExpressionStatement* lit_ast_makeexprstmt(size_t line, LitExpression* expression)
{
    LitExpressionStatement* statement = (LitExpressionStatement*)lit_ast_allocexpression(line, sizeof(LitExpressionStatement), LIT_ASTEXPRTYP_EXPRESSION);
    statement->expression = expression;
    return statement;
}

LitBlockStatement* lit_ast_makeblockstmt(size_t line)
{
    LitBlockStatement* statement = (LitBlockStatement*)lit_ast_allocexpression(line, sizeof(LitBlockStatement), LIT_ASTEXPRTYP_BLOCK);
    lit_exprlist_init(&statement->statements);
    return statement;
}

LitVarStatement* lit_ast_makevardefstmt(size_t line, const char* name, size_t length, LitExpression* init, bool constant)
{
    LitVarStatement* statement = (LitVarStatement*)lit_ast_allocexpression(line, sizeof(LitVarStatement), LIT_ASTEXPRTYP_VARDECL);
    statement->name = name;
    statement->length = length;
    statement->init = init;
    statement->constant = constant;
    return statement;
}

LitIfStatement* lit_ast_makeifstatement(size_t line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch, LitDynListExpr* elseif_conditions, LitDynListExpr* elseif_branches)
{
    LitIfStatement* statement = (LitIfStatement*)lit_ast_allocexpression(line, sizeof(LitIfStatement), LIT_ASTEXPRTYP_IF);
    statement->condition = condition;
    statement->if_branch = if_branch;
    statement->else_branch = else_branch;
    statement->elseif_conditions = elseif_conditions;
    statement->elseif_branches = elseif_branches;
    return statement;
}

LitWhileStatement* lit_ast_makewhilestmt(size_t line, LitExpression* condition, LitExpression* body)
{
    LitWhileStatement* statement = (LitWhileStatement*)lit_ast_allocexpression(line, sizeof(LitWhileStatement), LIT_ASTEXPRTYP_WHILE);
    statement->condition = condition;
    statement->body = body;
    return statement;
}

LitForStatement* lit_ast_makeforstmt(size_t line, LitExpression* init, LitExpression* var, LitExpression* condition, LitExpression* increment, LitExpression* body, bool c_style)
{
    LitForStatement* statement = (LitForStatement*)lit_ast_allocexpression(line, sizeof(LitForStatement), LIT_ASTEXPRTYP_FOR);
    statement->init = init;
    statement->var = var;
    statement->condition = condition;
    statement->increment = increment;
    statement->body = body;
    statement->c_style = c_style;
    return statement;
}

LitContinueStatement* lit_ast_makecontinuestmt(size_t line)
{
    return (LitContinueStatement*)lit_ast_allocexpression(line, sizeof(LitContinueStatement), LIT_ASTEXPRTYP_CONTINUE);
}

LitBreakStatement* lit_ast_makebreakstmt(size_t line)
{
    return (LitBreakStatement*)lit_ast_allocexpression(line, sizeof(LitBreakStatement), LIT_ASTEXPRTYP_BREAK);
}

LitFunctionStatement* lit_ast_makefuncdefstmt(size_t line, const char* name, size_t length)
{
    LitFunctionStatement* function = (LitFunctionStatement*)lit_ast_allocexpression(line, sizeof(LitFunctionStatement), LIT_ASTEXPRTYP_FUNCTION);
    function->name = name;
    function->length = length;
    function->body = NULL;
    lit_paramlist_init(&function->parameters);
    return function;
}

LitReturnStatement* lit_ast_makereturnstmt(size_t line, LitExpression* expression)
{
    LitReturnStatement* statement = (LitReturnStatement*)lit_ast_allocexpression(line, sizeof(LitReturnStatement), LIT_ASTEXPRTYP_RETURN);
    statement->expression = expression;
    return statement;
}

LitMethodStatement* lit_ast_makemethoddefstmt(size_t line, LitString* name, bool is_static)
{
    LitMethodStatement* statement = (LitMethodStatement*)lit_ast_allocexpression(line, sizeof(LitMethodStatement), LIT_ASTEXPRTYP_METHOD);
    statement->name = name;
    statement->body = NULL;
    statement->is_static = is_static;
    lit_paramlist_init(&statement->parameters);
    return statement;
}

LitClassStatement* lit_ast_makeclassdefstmt(size_t line, LitString* name, LitString* parent)
{
    LitClassStatement* statement = (LitClassStatement*)lit_ast_allocexpression(line, sizeof(LitClassStatement), LIT_ASTEXPRTYP_CLASS);
    statement->name = name;
    statement->parent = parent;
    lit_exprlist_init(&statement->fields);
    return statement;
}

LitFieldStatement* lit_ast_makefieldstmt(size_t line, LitString* name, LitExpression* getter, LitExpression* setter, bool is_static)
{
    LitFieldStatement* statement = (LitFieldStatement*)lit_ast_allocexpression(line, sizeof(LitFieldStatement), LIT_ASTEXPRTYP_FIELD);
    statement->name = name;
    statement->getter = getter;
    statement->setter = setter;
    statement->is_static = is_static;
    return statement;
}

LitDynListExpr* lit_ast_allocexprlist()
{
    LitDynListExpr* expressions = (LitDynListExpr*)lit_sysmem_malloc(sizeof(LitDynListExpr));
    lit_exprlist_init(expressions);
    return expressions;
}

void lit_ast_destroyallocatedexprlist(LitState* state, LitDynListExpr* expressions)
{
    size_t i;
    if(expressions == NULL)
    {
        return;
    }
    for(i = 0; i < expressions->count; i++)
    {
        lit_ast_destroyexpression(state, expressions->values[i]);
    }
    lit_exprlist_destroy(expressions);
    lit_sysmem_free(expressions);
}

LitDynListExpr* lit_ast_allocstmtlist()
{
    LitDynListExpr* statements = (LitDynListExpr*)lit_sysmem_malloc(sizeof(LitDynListExpr));
    lit_exprlist_init(statements);
    return statements;
}

void lit_ast_destroyallocatedstmtlist(LitState* state, LitDynListExpr* statements)
{
    size_t i;
    if(statements == NULL)
    {
        return;
    }
    for(i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }
    lit_exprlist_destroy(statements);
    lit_sysmem_free(statements);
}

void lit_scanner_init(LitState* state, LitScanner* scanner, const char* filename, const char* source)
{
    scanner->sourcecurrentline = 1;
    scanner->sourcedatastart = source;
    scanner->sourcedatacurrent = source;
    scanner->sourcefilename = filename;
    scanner->pstate = state;
    scanner->bracecount = 0;
    scanner->had_error = false;
}

LitToken lit_scanner_maketoken(LitScanner* scanner, LitTokenType type)
{
    LitToken token;
    token.type = type;
    token.start = scanner->sourcedatastart;
    token.length = (size_t)(scanner->sourcedatacurrent - scanner->sourcedatastart);
    token.line = scanner->sourcecurrentline;
    return token;
}

LitToken lit_scanner_makeerrortoken(LitScanner* scanner, const char* fmt, ...)
{
    scanner->had_error = true;
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(scanner->pstate, scanner->sourcecurrentline, fmt, args);
    va_end(args);
    LitToken token;
    token.type = LIT_ASTTOKTYP_ERROR;
    token.start = result->strbuf.data;
    token.length = result->strbuf.length;
    token.line = scanner->sourcecurrentline;
    return token;
}

bool lit_scanner_isatend(LitScanner* scanner)
{
    return *scanner->sourcedatacurrent == '\0';
}

char lit_scanner_advance(LitScanner* scanner)
{
    scanner->sourcedatacurrent++;
    return scanner->sourcedatacurrent[-1];
}

bool lit_scanner_match(LitScanner* scanner, char expected)
{
    if(lit_scanner_isatend(scanner))
    {
        return false;
    }
    if(*scanner->sourcedatacurrent != expected)
    {
        return false;
    }
    scanner->sourcedatacurrent++;
    return true;
}

LitToken lit_scanner_matchtoken(LitScanner* scanner, char c, LitTokenType a, LitTokenType b)
{
    return lit_scanner_maketoken(scanner, lit_scanner_match(scanner, c) ? a : b);
}

LitToken lit_scanner_matchtokens(LitScanner* scanner, char cr, char cb, LitTokenType a, LitTokenType b, LitTokenType c)
{
    return lit_scanner_maketoken(scanner, lit_scanner_match(scanner, cr) ? a : (lit_scanner_match(scanner, cb) ? b : c));
}

char lit_scanner_peek(LitScanner* scanner)
{
    return *scanner->sourcedatacurrent;
}

char lit_scanner_peeknext(LitScanner* scanner)
{
    if(lit_scanner_isatend(scanner))
    {
        return '\0';
    }
    return scanner->sourcedatacurrent[1];
}

bool lit_scanner_skipwhitespace(LitScanner* scanner)
{
    while(true)
    {
        char c = lit_scanner_peek(scanner);
        switch(c)
        {
            case 1:
            case 2:
            case 3:
            case ' ':
            case '\r':
            case '\t':
            {
                lit_scanner_advance(scanner);
                break;
            }
            case '\n':
            {
                scanner->sourcedatastart = scanner->sourcedatacurrent;
                lit_scanner_advance(scanner);
                return true;
            }
            case '/':
            {
                if(lit_scanner_peeknext(scanner) == '/')
                {
                    while(lit_scanner_peek(scanner) != '\n' && !lit_scanner_isatend(scanner))
                    {
                        lit_scanner_advance(scanner);
                    }
                    return lit_scanner_skipwhitespace(scanner);
                }
                else if(lit_scanner_peeknext(scanner) == '*')
                {
                    lit_scanner_advance(scanner);
                    lit_scanner_advance(scanner);
                    while((lit_scanner_peek(scanner) != '*' || lit_scanner_peeknext(scanner) != '/') && !lit_scanner_isatend(scanner))
                    {
                        if(lit_scanner_peek(scanner) == '\n')
                        {
                            scanner->sourcecurrentline++;
                        }
                        lit_scanner_advance(scanner);
                    }
                    lit_scanner_advance(scanner);
                    lit_scanner_advance(scanner);
                    return lit_scanner_skipwhitespace(scanner);
                }
                return false;
            }
            default:
                return false;
        }
    }
}

LitToken lit_scanner_scanstring(LitScanner* lex, bool interpolation, bool useescapes, char endch)
{
    char currch;
    char nextch;
    const char* cstr;
    LitDynListByte bytes;
    LitTokenType stringtype;
    LitState* state;
    state = lex->pstate;
    stringtype = LIT_ASTTOKTYP_STRING;
    lit_bytelist_init(&bytes);
    while(true)
    {
        currch = lit_scanner_advance(lex);
        if(currch == endch)
        {
            break;
        }
        else if(interpolation && currch == '{')
        {
            if(lex->bracecount >= LIT_INTERPOLATION_NESTING_MAX)
            {
                lit_bytelist_destroy(&bytes);
                return lit_scanner_makeerrortoken(lex, "interpolation nesting is too deep, maximum is %i", LIT_INTERPOLATION_NESTING_MAX);
            }
            stringtype = LIT_ASTTOKTYP_INTERPOLATION;
            lex->bracevalues[lex->bracecount++] = 1;
            break;
        }
        if(useescapes)
        {
            switch(currch)
            {
                case '\0':
                {
                    lit_bytelist_destroy(&bytes);
                    return lit_scanner_makeerrortoken(lex, "unterminated string");
                }
                break;
                case '\n':
                {
                    lex->sourcecurrentline++;
                    lit_bytelist_push(&bytes, currch);
                }
                break;
                case '\\':
                {
                    nextch = lit_scanner_advance(lex);
                    if(nextch == '\n')
                    {
                        continue;
                    }
                    if(nextch == endch)
                    {
                        lit_bytelist_push(&bytes, endch);
                    }
                    else
                    {
                        switch(nextch)
                        {
                            case '\"':
                                lit_bytelist_push(&bytes, '\"');
                                break;
                            case '\\':
                                lit_bytelist_push(&bytes, '\\');
                                break;
                            case '0':
                                lit_bytelist_push(&bytes, '\0');
                                break;
                            case '{':
                                lit_bytelist_push(&bytes, '{');
                                break;
                            case 'a':
                                lit_bytelist_push(&bytes, '\a');
                                break;
                            case 'b':
                                lit_bytelist_push(&bytes, '\b');
                                break;
                            case 'f':
                                lit_bytelist_push(&bytes, '\f');
                                break;
                            case 'n':
                                lit_bytelist_push(&bytes, '\n');
                                break;
                            case 'r':
                                lit_bytelist_push(&bytes, '\r');
                                break;
                            case 't':
                                lit_bytelist_push(&bytes, '\t');
                                break;
                            case 'v':
                                lit_bytelist_push(&bytes, '\v');
                                break;
                            case 'e':
                                lit_bytelist_push(&bytes, 27);
                                break;
                            default:
                            {
                                lit_bytelist_destroy(&bytes);
                                return lit_scanner_makeerrortoken(lex, "invalid escape character '%c'", lex->sourcedatacurrent[-1]);
                            }
                            break;
                        }
                    }
                }
                break;
                default:
                {
                    lit_bytelist_push(&bytes, currch);
                }
                break;
            }
        }
        else
        {
            lit_bytelist_push(&bytes, currch);
        }
    }
    LitToken token = lit_scanner_maketoken(lex, stringtype);
    /*if(bytes.count == 0)
    {
        token.value = lit_value_makenull();
    }
    else
    */
    {
        cstr = bytes.values;
        if(bytes.count == 0)
        {
            cstr = "";
        }
        token.value = lit_value_fromobject(lit_string_copylen(state, cstr, bytes.count));
    }
    lit_bytelist_destroy(&bytes);
    return token;
}

int lit_scanner_scanhexdigit(LitScanner* scanner)
{
    char c = lit_scanner_advance(scanner);
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
    scanner->sourcedatacurrent--;
    return -1;
}

int lit_scanner_scanbinarydigit(LitScanner* scanner)
{
    char c = lit_scanner_advance(scanner);
    if(c >= '0' && c <= '1')
    {
        return c - '0';
    }
    scanner->sourcedatacurrent--;
    return -1;
}

LitToken lit_scanner_makenumbertoken(LitScanner* scanner, bool ishex, bool isbinary)
{
    errno = 0;
    LitValue value;
    if(ishex)
    {
        value = lit_value_makenumber((double)strtoll(scanner->sourcedatastart, NULL, 16));
    }
    else if(isbinary)
    {
        value = lit_value_makenumber((int)strtoll(scanner->sourcedatastart + 2, NULL, 2));
    }
    else
    {
        value = lit_value_makenumber(strtod(scanner->sourcedatastart, NULL));
    }
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_scanner_makeerrortoken(scanner, "number is too big to be represented by a single literal");
    }
    LitToken token = lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_NUMBER);
    token.value = value;
    return token;
}

LitToken lit_scanner_scannumber(LitScanner* scanner)
{
    if(lit_scanner_match(scanner, 'x'))
    {
        while(lit_scanner_scanhexdigit(scanner) != -1)
        {
            continue;
        }
        return lit_scanner_makenumbertoken(scanner, true, false);
    }
    if(lit_scanner_match(scanner, 'b'))
    {
        while(lit_scanner_scanbinarydigit(scanner) != -1)
        {
            continue;
        }
        return lit_scanner_makenumbertoken(scanner, false, true);
    }
    while(lit_is_digit(lit_scanner_peek(scanner)))
    {
        lit_scanner_advance(scanner);
    }
    /* look for a fractional part */
    if(lit_scanner_peek(scanner) == '.' && lit_is_digit(lit_scanner_peeknext(scanner)))
    {
        /* consume the '.' */
        lit_scanner_advance(scanner);
        while(lit_is_digit(lit_scanner_peek(scanner)))
        {
            lit_scanner_advance(scanner);
        }
    }
    return lit_scanner_makenumbertoken(scanner, false, false);
}

LitTokenType lit_scanner_scanidenttype(LitScanner* lex)
{
    static struct
    {
        LitTokenType type;
        const char* kw;
    } keywords[] = {
        { LIT_ASTTOKTYP_CLASS, "class" }, { LIT_ASTTOKTYP_ELSE, "else" }, { LIT_ASTTOKTYP_FALSE, "false" }, { LIT_ASTTOKTYP_FOR, "for" }, { LIT_ASTTOKTYP_FUNCTION, "function" }, { LIT_ASTTOKTYP_IF, "if" }, { LIT_ASTTOKTYP_NULL, "null" }, { LIT_ASTTOKTYP_RETURN, "return" }, { LIT_ASTTOKTYP_SUPER, "super" }, { LIT_ASTTOKTYP_THIS, "this" }, { LIT_ASTTOKTYP_TRUE, "true" }, { LIT_ASTTOKTYP_VAR, "var" }, { LIT_ASTTOKTYP_WHILE, "while" }, { LIT_ASTTOKTYP_CONTINUE, "continue" }, { LIT_ASTTOKTYP_BREAK, "break" }, { LIT_ASTTOKTYP_NEW, "new" }, { LIT_ASTTOKTYP_EXPORT, "export" }, { LIT_ASTTOKTYP_IS, "is" }, { LIT_ASTTOKTYP_STATIC, "static" }, { LIT_ASTTOKTYP_OPERATOR, "operator" }, { LIT_ASTTOKTYP_IN, "in" }, { LIT_ASTTOKTYP_CONST, "const" }, { LIT_ASTTOKTYP_REF, "ref" }, { (LitTokenType)0, NULL },
    };
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

LitToken lit_scanner_scanident(LitScanner* scanner)
{
    while(lit_is_alpha(lit_scanner_peek(scanner)) || lit_is_digit(lit_scanner_peek(scanner)))
    {
        lit_scanner_advance(scanner);
    }
    return lit_scanner_maketoken(scanner, lit_scanner_scanidenttype(scanner));
}

LitToken lit_scanner_scantoken(LitScanner* scanner)
{
    if(lit_scanner_skipwhitespace(scanner))
    {
        LitToken token = lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_NEW_LINE);
        scanner->sourcecurrentline++;
        return token;
    }
    scanner->sourcedatastart = scanner->sourcedatacurrent;
    if(lit_scanner_isatend(scanner))
    {
        return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_EOF);
    }
    char c = lit_scanner_advance(scanner);
    if(lit_is_digit(c))
    {
        return lit_scanner_scannumber(scanner);
    }
    if(lit_is_alpha(c))
    {
        return lit_scanner_scanident(scanner);
    }
    switch(c)
    {
        case '(':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_LEFT_PAREN);
        case ')':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_RIGHT_PAREN);
        case '{':
        {
            if(scanner->bracecount > 0)
            {
                scanner->bracevalues[scanner->bracecount - 1]++;
            }
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_LEFT_BRACE);
        }
        case '}':
        {
            if(scanner->bracecount > 0 && --scanner->bracevalues[scanner->bracecount - 1] == 0)
            {
                scanner->bracecount--;
                return lit_scanner_scanstring(scanner, true, true, '`');
            }
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_RIGHT_BRACE);
        }
        case '[':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_LEFT_BRACKET);
        case ']':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_RIGHT_BRACKET);
        case ';':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_SEMICOLON);
        case ',':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_COMMA);
        case ':':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_COLON);
        case '~':
            return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_TILDE);
        case '+':
            return lit_scanner_matchtokens(scanner, '=', '+', LIT_ASTTOKTYP_PLUS_EQUAL, LIT_ASTTOKTYP_PLUS_PLUS, LIT_ASTTOKTYP_PLUS);
        case '-':
            return lit_scanner_match(scanner, '>') ? lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_SMALL_ARROW) : lit_scanner_matchtokens(scanner, '=', '-', LIT_ASTTOKTYP_MINUS_EQUAL, LIT_ASTTOKTYP_MINUS_MINUS, LIT_ASTTOKTYP_MINUS);
        case '/':
            return lit_scanner_matchtoken(scanner, '=', LIT_ASTTOKTYP_SLASH_EQUAL, LIT_ASTTOKTYP_SLASH);
        case '#':
            return lit_scanner_matchtoken(scanner, '=', LIT_ASTTOKTYP_SHARP_EQUAL, LIT_ASTTOKTYP_SHARP);
        case '!':
            return lit_scanner_matchtoken(scanner, '=', LIT_ASTTOKTYP_BANG_EQUAL, LIT_ASTTOKTYP_BANG);
        case '?':
            return lit_scanner_matchtoken(scanner, '?', LIT_ASTTOKTYP_QUESTION_QUESTION, LIT_ASTTOKTYP_QUESTION);
        case '%':
            return lit_scanner_matchtoken(scanner, '=', LIT_ASTTOKTYP_PERCENT_EQUAL, LIT_ASTTOKTYP_PERCENT);
        case '^':
            return lit_scanner_matchtoken(scanner, '=', LIT_ASTTOKTYP_CARET_EQUAL, LIT_ASTTOKTYP_CARET);
        case '>':
            return lit_scanner_matchtokens(scanner, '=', '>', LIT_ASTTOKTYP_GREATER_EQUAL, LIT_ASTTOKTYP_GREATER_GREATER, LIT_ASTTOKTYP_GREATER);
        case '<':
            return lit_scanner_matchtokens(scanner, '=', '<', LIT_ASTTOKTYP_LESS_EQUAL, LIT_ASTTOKTYP_LESS_LESS, LIT_ASTTOKTYP_LESS);
        case '*':
            return lit_scanner_matchtokens(scanner, '=', '*', LIT_ASTTOKTYP_STAR_EQUAL, LIT_ASTTOKTYP_STAR_STAR, LIT_ASTTOKTYP_STAR);
        case '=':
            return lit_scanner_matchtokens(scanner, '=', '>', LIT_ASTTOKTYP_EQUAL_EQUAL, LIT_ASTTOKTYP_ARROW, LIT_ASTTOKTYP_EQUAL);
        case '|':
            return lit_scanner_matchtokens(scanner, '=', '|', LIT_ASTTOKTYP_BAR_EQUAL, LIT_ASTTOKTYP_BAR_BAR, LIT_ASTTOKTYP_BAR);
        case '&':
            return lit_scanner_matchtokens(scanner, '=', '&', LIT_ASTTOKTYP_AMPERSAND_EQUAL, LIT_ASTTOKTYP_AMPERSAND_AMPERSAND, LIT_ASTTOKTYP_AMPERSAND);
        case '.':
        {
            if(!lit_scanner_match(scanner, '.'))
            {
                return lit_scanner_maketoken(scanner, LIT_ASTTOKTYP_DOT);
            }
            return lit_scanner_matchtoken(scanner, '.', LIT_ASTTOKTYP_DOT_DOT_DOT, LIT_ASTTOKTYP_DOT_DOT);
        }
        case '`':
        {
            return lit_scanner_scanstring(scanner, true, true, '`');
        }
        case '"':
            return lit_scanner_scanstring(scanner, false, true, '"');
    }
    fprintf(stderr, "current=%s\n", scanner->sourcedatacurrent);
    return lit_scanner_makeerrortoken(scanner, "unexpected character '%c'", c);
}

static jmp_buf jumpbuffer;
static LitParseRule rules[LIT_ASTTOKTYP_EOF + 1];
static bool didsetuprules;

void lit_parser_compilerinit(LitParser* parser, LitCompiler* compiler)
{
    compiler->scope_depth = 0;
    compiler->function = NULL;
    compiler->enclosing = (struct LitCompiler*)parser->compiler;
    parser->compiler = compiler;
}

void lit_parser_compilerend(LitParser* parser, LitCompiler* compiler)
{
    parser->compiler = (LitCompiler*)compiler->enclosing;
}

void lit_parser_scopebegin(LitParser* parser)
{
    parser->compiler->scope_depth++;
}

void lit_parser_scopeend(LitParser* parser)
{
    parser->compiler->scope_depth--;
}

LitParseRule* lit_parser_getrule(LitTokenType type)
{
    return &rules[type];
}

bool lit_parser_isatend(LitParser* parser)
{
    return parser->current.type == LIT_ASTTOKTYP_EOF;
}

void lit_parser_init(LitState* state, LitParser* parser)
{
    if(!didsetuprules)
    {
        didsetuprules = true;
        lit_parser_setuprules();
    }
    parser->pstate = state;
    parser->had_error = false;
    parser->panic_mode = false;
}

void lit_parser_destroy(LitParser* parser)
{
    (void)parser;
}

void lit_parser_failactual(LitParser* parser, LitToken* token, const char* message)
{
    (void)token;
    if(parser->panic_mode)
    {
        return;
    }
    lit_state_raiseerror(parser->pstate, LIT_ERROR_COMPILEERROR, message);
    parser->had_error = true;
    lit_parser_sync(parser);
}

void lit_parser_failatv(LitParser* parser, LitToken* token, const char* fmt, va_list args)
{
    lit_parser_failactual(parser, token, lit_state_errorfmtv(parser->pstate, token->line, fmt, args)->strbuf.data);
}

void lit_parser_failherefmt(LitParser* parser, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_parser_failatv(parser, &parser->current, fmt, args);
    va_end(args);
}

void lit_parser_failfmt(LitParser* parser, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_parser_failatv(parser, &parser->previous, fmt, args);
    va_end(args);
}

void lit_parser_advance(LitParser* parser)
{
    parser->previous = parser->current;
    while(true)
    {
        parser->current = lit_scanner_scantoken(parser->pstate->scanner);
        if(parser->current.type != LIT_ASTTOKTYP_ERROR)
        {
            break;
        }
        lit_parser_failactual(parser, &parser->current, parser->current.start);
    }
}

bool lit_parser_check(LitParser* parser, LitTokenType type)
{
    return parser->current.type == type;
}

bool lit_parser_match(LitParser* parser, LitTokenType type)
{
    if(parser->current.type == type)
    {
        lit_parser_advance(parser);
        return true;
    }
    return false;
}

bool lit_parser_matchident(LitParser* parser, const char* type)
{
    if(parser->current.type == LIT_ASTTOKTYP_IDENTIFIER || parser->current.type == LIT_ASTTOKTYP_CLASS)
    {
        if(memcmp(parser->previous.start, type, fmax(strlen(type), parser->previous.length)))
        {
            lit_parser_advance(parser);
            return true;
        }
    }
    return false;
}

void lit_parser_consume(LitParser* parser, LitTokenType type, const char* error)
{
    if(parser->current.type == type)
    {
        lit_parser_advance(parser);
        return;
    }
    bool line = parser->previous.type == LIT_ASTTOKTYP_NEW_LINE;
    lit_parser_failactual(parser, &parser->current, lit_state_errorfmt(parser->pstate, parser->current.line, "expected %s, got '%.*s'", error, line ? 8 : parser->previous.length, line ? "new line" : parser->previous.start)->strbuf.data);
}

bool lit_parser_matchlinefeed(LitParser* parser)
{
    if(!lit_parser_match(parser, LIT_ASTTOKTYP_NEW_LINE))
    {
        return false;
    }
    while(lit_parser_match(parser, LIT_ASTTOKTYP_NEW_LINE))
    {
    }
    return true;
}

void lit_parser_ignorelinefeeds(LitParser* parser)
{
    lit_parser_matchlinefeed(parser);
}

LitExpression* lit_parser_parseblock(LitParser* parser)
{
    lit_parser_scopebegin(parser);
    LitBlockStatement* statement = lit_ast_makeblockstmt(parser->previous.line);
    lit_parser_ignorelinefeeds(parser);
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_BRACE) && !lit_parser_check(parser, LIT_ASTTOKTYP_EOF))
    {
        lit_exprlist_push(&statement->statements, lit_parser_parsestmt(parser));
        lit_parser_ignorelinefeeds(parser);
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACE, "'}'");
    lit_parser_scopeend(parser);
    return (LitExpression*)statement;
}

LitExpression* lit_parser_parseprec(LitParser* parser, LitPrecedence precedence, bool err)
{
    LitToken previous = parser->previous;
    lit_parser_advance(parser);
    LitPrefixParseFn prefix_rule = lit_parser_getrule(parser->previous.type)->prefix;
    if(prefix_rule == NULL)
    {
        /* todo: file start */
        bool prevnewline = ((previous.start != NULL) && (*previous.start == '\n'));
        bool parserprevnewline = ((parser->previous.start != NULL) && (*parser->previous.start == '\n'));
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
            gotlen = parser->previous.length;
            gotstr = parser->previous.start;
        }
        lit_parser_failfmt(parser, "expected expression after '%.*s', got '%.*s'", exlen, exstr, gotlen, gotstr);
        return NULL;
    }
    bool canassign = precedence <= LIT_ASTPREC_ASSIGNMENT;
    LitExpression* expr = prefix_rule(parser, canassign);
    lit_parser_ignorelinefeeds(parser);
    while(precedence <= lit_parser_getrule(parser->current.type)->precedence)
    {
        lit_parser_advance(parser);
        LitInfixParseFn infix_rule = lit_parser_getrule(parser->previous.type)->infix;
        expr = infix_rule(parser, expr, canassign);
    }
    if(err && canassign && lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        lit_parser_failfmt(parser, "invalid assigment target");
    }
    return expr;
}

LitExpression* lit_parser_rulenumber(LitParser* parser, bool canassign)
{
    (void)canassign;
    return (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value);
}

LitExpression* lit_parser_parselambda(LitParser* parser, LitFunctionStatement* lambda)
{
    lambda->body = lit_parser_parsestmt(parser);
    return (LitExpression*)lambda;
}

LitParameter lit_parser_makeparameter(const char* name, size_t length, uint8_t reg, LitExpression* defval)
{
    LitParameter pm;
    pm.name = name;
    pm.length = length;
    pm.reg = reg;
    pm.default_value = defval;
    return pm;
}

void lit_parser_parseparams(LitParser* parser, LitDynListParam* parameters)
{
    bool haddefault;
    haddefault = false;
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_PAREN))
    {
        /* variadic argument ... */
        if(lit_parser_match(parser, LIT_ASTTOKTYP_DOT_DOT_DOT))
        {
            lit_paramlist_push(parameters, lit_parser_makeparameter("...", 3, 0, NULL));
            return;
        }
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "argument name");
        const char* argname = parser->previous.start;
        size_t arglength = parser->previous.length;
        LitExpression* default_value = NULL;
        if(lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
        {
            haddefault = true;
            default_value = lit_parser_parseexpr(parser);
        }
        else if(haddefault)
        {
            lit_parser_failfmt(parser, "default arguments must always be in the end of the argument list.");
        }
        lit_paramlist_push(parameters, lit_parser_makeparameter(argname, arglength, 0, default_value));
        if(!lit_parser_match(parser, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
    }
}

LitExpression* lit_parser_rulegroupingorlambda(LitParser* parser, bool canassign)
{
    (void)canassign;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_RIGHT_PAREN))
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_ARROW, "=> after lambda arguments");
        return lit_parser_parselambda(parser, lit_ast_makelambdaexpr(parser->previous.line));
    }
    const char* start = parser->previous.start;
    size_t line = parser->previous.line;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_IDENTIFIER) || lit_parser_match(parser, LIT_ASTTOKTYP_DOT_DOT_DOT))
    {
        LitState* state = parser->pstate;
        const char* firstargstart = parser->previous.start;
        size_t firstarglength = parser->previous.length;
        if(lit_parser_match(parser, LIT_ASTTOKTYP_COMMA) || (lit_parser_match(parser, LIT_ASTTOKTYP_RIGHT_PAREN) && lit_parser_match(parser, LIT_ASTTOKTYP_ARROW)))
        {
            bool hadarrow = parser->previous.type == LIT_ASTTOKTYP_ARROW;
            bool hadvararg = parser->previous.type == LIT_ASTTOKTYP_DOT_DOT_DOT;
            /* this is a lambda */
            LitFunctionStatement* lambda = lit_ast_makelambdaexpr(line);
            LitExpression* defvalue = NULL;
            bool haddefault = lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL);
            if(haddefault)
            {
                defvalue = lit_parser_parseexpr(parser);
            }
            lit_paramlist_push(&lambda->parameters, lit_parser_makeparameter(firstargstart, firstarglength, 0, defvalue));
            if(!hadvararg && parser->previous.type == LIT_ASTTOKTYP_COMMA)
            {
                do
                {
                    bool stop = false;
                    if(lit_parser_match(parser, LIT_ASTTOKTYP_DOT_DOT_DOT))
                    {
                        stop = true;
                    }
                    else
                    {
                        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "argument name");
                    }
                    const char* argname = parser->previous.start;
                    size_t arglength = parser->previous.length;
                    LitExpression* default_value = NULL;
                    if(lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
                    {
                        default_value = lit_parser_parseexpr(parser);
                        haddefault = true;
                    }
                    else if(haddefault)
                    {
                        lit_parser_failfmt(parser, "default arguments must always be in the end of the argument list.");
                    }
                    lit_paramlist_push(&lambda->parameters, lit_parser_makeparameter(argname, arglength, 0, default_value));
                    if(stop)
                    {
                        break;
                    }
                } while(lit_parser_match(parser, LIT_ASTTOKTYP_COMMA));
            }
            if(!hadarrow)
            {
                lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after lambda parameters");
                lit_parser_consume(parser, LIT_ASTTOKTYP_ARROW, "=> after lambda parameters");
            }
            return lit_parser_parselambda(parser, lambda);
        }
        else
        {
            /* ouch, this was a grouping with a single identifier */
            LitScanner* scanner = state->scanner;
            scanner->sourcedatacurrent = start;

            scanner->sourcecurrentline = line;
            parser->current = lit_scanner_scantoken(scanner);
            lit_parser_advance(parser);
        }
    }
    LitExpression* expression = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after grouping expression");
    return expression;
}

LitExpression* lit_parser_parsecall(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitCallExpression* expression = lit_ast_makecallexpr(parser->previous.line, prev);
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_PAREN))
    {
        LitExpression* e = lit_parser_parseexpr(parser);
        lit_exprlist_push(&expression->args, e);
        if(!lit_parser_match(parser, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
        if(e->type == LIT_ASTEXPRTYP_VAR)
        {
            LitVarExpression* ee = (LitVarExpression*)e;
            /* variadic arg ... */
            if(ee->length == 3 && memcmp(ee->name, "...", 3) == 0)
            {
                break;
            }
        }
    }
    if(expression->args.count > 255)
    {
        lit_parser_failfmt(parser, "function can't have more than 255 arguments, got %i", (int)expression->args.count);
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after arguments");
    return (LitExpression*)expression;
}

LitExpression* lit_parser_ruleunary(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    LitExpression* expression = lit_parser_parseprec(parser, LIT_ASTPREC_UNARY, true);
    return (LitExpression*)lit_ast_makeunaryexpr(line, expression, op);
}

LitExpression* lit_parser_rulebinary(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    bool invert = parser->previous.type == LIT_ASTTOKTYP_BANG;
    if(invert)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IS, "'is' after '!'");
    }
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);
    expression = (LitExpression*)lit_ast_makebinaryexpr(line, prev, expression, op);
    if(invert)
    {
        expression = (LitExpression*)lit_ast_makeunaryexpr(line, expression, LIT_ASTTOKTYP_BANG);
    }
    return expression;
}

LitExpression* lit_parser_rulelogicaland(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, LIT_ASTPREC_AND, true), op);
}

LitExpression* lit_parser_rulelogicalor(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, LIT_ASTPREC_OR, true), op);
}

LitExpression* lit_parser_rulenullfilter(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, LIT_ASTPREC_NULL, true), op);
}

LitTokenType lit_parser_convertcompoundop(LitTokenType op)
{
    switch(op)
    {
        case LIT_ASTTOKTYP_PLUS_EQUAL:
            return LIT_ASTTOKTYP_PLUS;
        case LIT_ASTTOKTYP_MINUS_EQUAL:
            return LIT_ASTTOKTYP_MINUS;
        case LIT_ASTTOKTYP_STAR_EQUAL:
            return LIT_ASTTOKTYP_STAR;
        case LIT_ASTTOKTYP_SLASH_EQUAL:
            return LIT_ASTTOKTYP_SLASH;
        case LIT_ASTTOKTYP_SHARP_EQUAL:
            return LIT_ASTTOKTYP_SHARP;
        case LIT_ASTTOKTYP_PERCENT_EQUAL:
            return LIT_ASTTOKTYP_PERCENT;
        case LIT_ASTTOKTYP_CARET_EQUAL:
            return LIT_ASTTOKTYP_CARET;
        case LIT_ASTTOKTYP_BAR_EQUAL:
            return LIT_ASTTOKTYP_BAR;
        case LIT_ASTTOKTYP_AMPERSAND_EQUAL:
            return LIT_ASTTOKTYP_AMPERSAND;
        case LIT_ASTTOKTYP_PLUS_PLUS:
            return LIT_ASTTOKTYP_PLUS;
        case LIT_ASTTOKTYP_MINUS_MINUS:
            return LIT_ASTTOKTYP_MINUS;
        default:
        {
            UNREACHABLE
        }
    }
    return LIT_ASTTOKTYP_EOF;
}

LitExpression* lit_parser_rulecompound(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    size_t line = parser->previous.line;
    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression;
    if(op == LIT_ASTTOKTYP_PLUS_PLUS || op == LIT_ASTTOKTYP_MINUS_MINUS)
    {
        expression = (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makenumber(1));
    }
    else
    {
        expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);
    }
    LitBinaryExpression* binary = lit_ast_makebinaryexpr(line, prev, expression, lit_parser_convertcompoundop(op));
    /* to make sure we don't free it twice */
    binary->ignore_left = true;
    return (LitExpression*)lit_ast_makeassignexpr(line, prev, (LitExpression*)binary);
}

LitExpression* lit_parser_ruleliteral(LitParser* parser, bool canassign)
{
    (void)canassign;
    size_t line = parser->previous.line;
    switch(parser->previous.type)
    {
        case LIT_ASTTOKTYP_TRUE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(true));
        }
        case LIT_ASTTOKTYP_FALSE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(false));
        }
        case LIT_ASTTOKTYP_NULL:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makenull());
        }
        default:
            UNREACHABLE
    }
    return NULL;
}

LitExpression* lit_parser_rulestring(LitParser* parser, bool canassign)
{
    LitExpression* expression = (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value);
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    return expression;
}

LitExpression* lit_parser_ruleinterpolation(LitParser* parser, bool canassign)
{
    LitString* str;
    LitInterpolationExpression* expression = lit_ast_makeinterpolationexpr(parser->previous.line);
    do
    {
        str = AS_STRING(parser->previous.value);
        if(str != NULL)
        {
            if(str->strbuf.length > 0)
            {
                lit_exprlist_push(&expression->expressions, (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value));
            }
        }
        lit_exprlist_push(&expression->expressions, lit_parser_parseexpr(parser));
    } while(lit_parser_match(parser, LIT_ASTTOKTYP_INTERPOLATION));
    lit_parser_consume(parser, LIT_ASTTOKTYP_STRING, "end of interpolation");
    str = AS_STRING(parser->previous.value);
    if(str != NULL)
    {
        if(str->strbuf.length > 0)
        {
            lit_exprlist_push(&expression->expressions, (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value));
        }
    }
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, (LitExpression*)expression, canassign);
    }
    return (LitExpression*)expression;
}

LitExpression* lit_parser_ruleobject(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitObjectExpression* object = lit_ast_makeobjectexpr(parser->previous.line);
    lit_parser_ignorelinefeeds(parser);
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_BRACE))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "key string after '{'");
        lit_vallist_push(&object->keys, lit_value_fromobject(lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length)));
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LIT_ASTTOKTYP_COLON, "':' after key string");
        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(&object->values, lit_parser_parseexpr(parser));
        if(!lit_parser_match(parser, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
    }
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACE, "'}' after object");
    return (LitExpression*)object;
}

LitExpression* lit_parser_parsevarexprbase(LitParser* parser, bool canassign, bool isnew)
{
    LitExpression* expression = (LitExpression*)lit_ast_makevarexpr(parser->previous.line, parser->previous.start, parser->previous.length);
    if(isnew)
    {
        bool hadargs = lit_parser_check(parser, LIT_ASTTOKTYP_LEFT_PAREN);
        LitCallExpression* call = NULL;
        if(hadargs)
        {
            lit_parser_advance(parser);
            call = (LitCallExpression*)lit_parser_parsecall(parser, expression, false);
        }
        if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACE))
        {
            if(call == NULL)
            {
                call = lit_ast_makecallexpr(expression->line, expression);
            }
            call->init = lit_parser_ruleobject(parser, false);
        }
        else if(!hadargs)
        {
            lit_parser_failherefmt(parser, "expected %s, got '%.*s'", "argument list for instance creation", parser->previous.length, parser->previous.start);
        }
        return (LitExpression*)call;
    }
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    if(canassign && lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->previous.line, expression, lit_parser_parseexpr(parser));
    }
    return expression;
}

LitExpression* lit_parser_rulevarexpr(LitParser* parser, bool canassign)
{
    return lit_parser_parsevarexprbase(parser, canassign, false);
}

LitExpression* lit_parser_rulenewexpr(LitParser* parser, bool canassign)
{
    (void)canassign;
    lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "class name after 'new'");
    return lit_parser_parsevarexprbase(parser, false, true);
}

LitExpression* lit_parser_ruledot(LitParser* parser, LitExpression* previous, bool canassign)
{
    size_t line = parser->previous.line;
    bool ignored = parser->previous.type == LIT_ASTTOKTYP_SMALL_ARROW;
    /* class and super are allowed field names */
    if(!(lit_parser_match(parser, LIT_ASTTOKTYP_CLASS) || lit_parser_match(parser, LIT_ASTTOKTYP_SUPER)))
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
    }
    const char* name = parser->previous.start;
    size_t length = parser->previous.length;
    if(!ignored && canassign && lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitExpression*)lit_ast_makesetexpr(line, previous, name, length, lit_parser_parseexpr(parser));
    }
    else
    {
        LitExpression* expression = (LitExpression*)lit_ast_makegetexpr(line, previous, name, length, false, ignored);
        if(!ignored && lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
        {
            return lit_parser_parsesubscript(parser, expression, canassign);
        }
        return expression;
    }
}

LitExpression* lit_parser_rulerange(LitParser* parser, LitExpression* previous, bool canassign)
{
    (void)canassign;
    size_t line = parser->previous.line;
    return (LitExpression*)lit_ast_makerangeexpr(line, previous, lit_parser_parseexpr(parser));
}

LitExpression* lit_parser_ruleternaryorquestion(LitParser* parser, LitExpression* previous, bool canassign)
{
    (void)canassign;
    size_t line = parser->previous.line;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_DOT) /* || lit_parser_match(parser, LIT_ASTTOKTYP_SMALL_ARROW)*/)
    {
        bool ignored = parser->previous.type == LIT_ASTTOKTYP_SMALL_ARROW;
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
        return (LitExpression*)lit_ast_makegetexpr(line, previous, parser->previous.start, parser->previous.length, true, ignored);
    }
    LitExpression* if_branch = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_COLON, "':' after expression");
    LitExpression* else_branch = lit_parser_parseexpr(parser);
    return (LitExpression*)lit_ast_maketernaryexpr(line, previous, if_branch, else_branch);
}

LitExpression* lit_parser_rulearray(LitParser* parser, bool canassign)
{
    LitArrayExpression* array = lit_ast_makearrayexpr(parser->previous.line);
    lit_parser_ignorelinefeeds(parser);
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_BRACKET))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(&array->values, lit_parser_parseexpr(parser));
        lit_parser_ignorelinefeeds(parser);
        if(!lit_parser_match(parser, LIT_ASTTOKTYP_COMMA))
        {
            break;
        }
        lit_parser_ignorelinefeeds(parser);
    }
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACKET, "']' after array");
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, (LitExpression*)array, canassign);
    }
    return (LitExpression*)array;
}

LitExpression* lit_parser_parsesubscript(LitParser* parser, LitExpression* previous, bool canassign)
{
    size_t line = parser->previous.line;
    LitExpression* index = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACKET, "']' after subscript");
    LitExpression* expression = (LitExpression*)lit_ast_makesubscriptexpr(line, previous, index);
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    else if(canassign && lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->previous.line, expression, lit_parser_parseexpr(parser));
    }
    return expression;
}

LitExpression* lit_parser_rulethis(LitParser* parser, bool canassign)
{
    LitExpression* expression = (LitExpression*)lit_ast_makethisexpr(parser->previous.line);
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    return expression;
}

LitExpression* lit_parser_rulesuper(LitParser* parser, bool canassign)
{
    (void)canassign;
    size_t line = parser->previous.line;
    if(!(lit_parser_match(parser, LIT_ASTTOKTYP_DOT) || lit_parser_match(parser, LIT_ASTTOKTYP_SMALL_ARROW)))
    {
        LitExpression* expression = (LitExpression*)lit_ast_makesuperexpr(line, lit_string_copylen(parser->pstate, "constructor", 11), false);
        lit_parser_consume(parser, LIT_ASTTOKTYP_LEFT_PAREN, "'(' after 'super'");
        return lit_parser_parsecall(parser, expression, false);
    }
    bool ignoring = parser->previous.type == LIT_ASTTOKTYP_SMALL_ARROW;
    lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, ignoring ? "super method name after '->'" : "super method name after '.'");
    LitExpression* expression = (LitExpression*)lit_ast_makesuperexpr(line, lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length), ignoring);
    if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_PAREN))
    {
        return lit_parser_parsecall(parser, expression, false);
    }
    return expression;
}

LitExpression* lit_parser_rulenothing(LitParser* parser, bool canassign)
{
    (void)parser;
    (void)canassign;
    return NULL;
}

LitExpression* lit_parser_rulefunction(LitParser* parser, bool canassign)
{
    (void)canassign;
    return lit_parser_parsefunction(parser);
}

LitExpression* lit_parser_rulereference(LitParser* parser, bool canassign)
{
    (void)canassign;
    size_t line = parser->previous.line;
    lit_parser_ignorelinefeeds(parser);
    LitReferenceExpression* expression = lit_ast_makerefexpr(line, lit_parser_parseprec(parser, LIT_ASTPREC_CALL, false));
    if(lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(line, (LitExpression*)expression, lit_parser_parseexpr(parser));
    }
    return (LitExpression*)expression;
}

LitExpression* lit_parser_parseexpr(LitParser* parser)
{
    lit_parser_ignorelinefeeds(parser);
    return lit_parser_parseprec(parser, LIT_ASTPREC_ASSIGNMENT, true);
}

LitExpression* lit_parser_parsevardecl(LitParser* parser)
{
    bool constant = parser->previous.type == LIT_ASTTOKTYP_CONST;
    size_t line = parser->previous.line;
    lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "variable name");
    const char* name = parser->previous.start;
    size_t length = parser->previous.length;
    LitExpression* init = NULL;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_EQUAL))
    {
        init = lit_parser_parseexpr(parser);
    }
    return (LitExpression*)lit_ast_makevardefstmt(line, name, length, init, constant);
}

LitExpression* lit_parser_parseif(LitParser* parser)
{
    size_t line = parser->previous.line;
    bool invert = lit_parser_match(parser, LIT_ASTTOKTYP_BANG);
    bool hadparen = lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);
    if(hadparen)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')'");
    }
    if(invert)
    {
        condition = (LitExpression*)lit_ast_makeunaryexpr(condition->line, condition, LIT_ASTTOKTYP_BANG);
    }
    lit_parser_ignorelinefeeds(parser);
    LitExpression* if_branch = lit_parser_parsestmt(parser);
    LitDynListExpr* elseif_conditions = NULL;
    LitDynListExpr* elseif_branches = NULL;
    LitExpression* else_branch = NULL;
    lit_parser_ignorelinefeeds(parser);
    while(lit_parser_match(parser, LIT_ASTTOKTYP_ELSE))
    {
        /* else if */
        if(lit_parser_match(parser, LIT_ASTTOKTYP_IF))
        {
            if(elseif_conditions == NULL)
            {
                elseif_conditions = lit_ast_allocexprlist();
                elseif_branches = lit_ast_allocstmtlist();
            }
            invert = lit_parser_match(parser, LIT_ASTTOKTYP_BANG);
            hadparen = lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_PAREN);
            LitExpression* e = lit_parser_parseexpr(parser);
            if(hadparen)
            {
                lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')'");
            }
            if(invert)
            {
                e = (LitExpression*)lit_ast_makeunaryexpr(condition->line, e, LIT_ASTTOKTYP_BANG);
            }
            lit_exprlist_push(elseif_conditions, e);
            lit_parser_ignorelinefeeds(parser);
            lit_exprlist_push(elseif_branches, lit_parser_parsestmt(parser));
            lit_parser_ignorelinefeeds(parser);
            continue;
        }
        /* else */
        if(else_branch != NULL)
        {
            lit_parser_failfmt(parser, "if-statement can have only one else-branch");
        }
        else_branch = lit_parser_parsestmt(parser);
    }
    return (LitExpression*)lit_ast_makeifstatement(line, condition, if_branch, else_branch, elseif_conditions, elseif_branches);
}

LitExpression* lit_parser_parsefor(LitParser* parser)
{
    size_t line = parser->previous.line;
    bool hadparen = lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_PAREN);
    LitExpression* var = NULL;
    LitExpression* init = NULL;
    if(!lit_parser_check(parser, LIT_ASTTOKTYP_SEMICOLON))
    {
        if(lit_parser_match(parser, LIT_ASTTOKTYP_VAR))
        {
            var = lit_parser_parsevardecl(parser);
        }
        else
        {
            init = lit_parser_parseexpr(parser);
        }
    }
    bool c_style = !lit_parser_match(parser, LIT_ASTTOKTYP_IN);
    LitExpression* condition = NULL;
    LitExpression* increment = NULL;
    if(c_style)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_SEMICOLON, "';'");
        condition = lit_parser_check(parser, LIT_ASTTOKTYP_SEMICOLON) ? NULL : lit_parser_parseexpr(parser);
        lit_parser_consume(parser, LIT_ASTTOKTYP_SEMICOLON, "';'");
        increment = lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_PAREN) ? NULL : lit_parser_parseexpr(parser);
    }
    else
    {
        condition = lit_parser_parseexpr(parser);
        if(var == NULL)
        {
            lit_parser_failfmt(parser, "for-loops using in-iteration must declare a new variable");
        }
    }
    if(hadparen)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')'");
    }
    return (LitExpression*)lit_ast_makeforstmt(line, init, var, condition, increment, lit_parser_parsestmt(parser), c_style);
}

LitExpression* lit_parser_parsewhile(LitParser* parser)
{
    size_t line = parser->previous.line;
    bool hadparen = lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);
    if(hadparen)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')'");
    }
    LitExpression* body = lit_parser_parsestmt(parser);
    return (LitExpression*)lit_ast_makewhilestmt(line, condition, body);
}

LitExpression* lit_parser_parsefunction(LitParser* parser)
{
    size_t line;
    size_t namelen;
    bool isexport;
    bool noname;
    const char* fnname;
    noname = false;
    fnname = "anonymous";
    namelen = strlen(fnname);
    isexport = parser->previous.type == LIT_ASTTOKTYP_EXPORT;
    if(isexport)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_FUNCTION, "'function' after 'export'");
    }
    line = parser->previous.line;
    if(lit_parser_check(parser, LIT_ASTTOKTYP_IDENTIFIER))
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "function name");
        fnname = parser->previous.start;
        namelen = parser->previous.length;
    }
    else
    {
        noname = true;
    }
    if(lit_parser_match(parser, LIT_ASTTOKTYP_DOT))
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "function name");
        LitFunctionStatement* lambda = lit_ast_makelambdaexpr(line);
        LitSetExpression* to = lit_ast_makesetexpr(line, (LitExpression*)lit_ast_makevarexpr(line, fnname, namelen), parser->previous.start, parser->previous.length, (LitExpression*)lambda);
        lit_parser_consume(parser, LIT_ASTTOKTYP_LEFT_PAREN, "'(' after function name");
        LitCompiler compiler;
        lit_parser_compilerinit(parser, &compiler);
        lit_parser_scopebegin(parser);
        lit_parser_parseparams(parser, &lambda->parameters);
        if(lambda->parameters.count > 255)
        {
            lit_parser_failfmt(parser, "function can't have more than 255 arguments, got %i", (int)lambda->parameters.count);
        }
        lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after function arguments");
        lambda->body = lit_parser_parsestmt(parser);
        lit_parser_scopeend(parser);
        lit_parser_compilerend(parser, &compiler);
        return (LitExpression*)lit_ast_makeexprstmt(line, (LitExpression*)to);
    }
    LitFunctionStatement* function;

    if(noname)
    {
        function = lit_ast_makelambdaexpr(line);
    }
    else
    {
        function = lit_ast_makefuncdefstmt(line, fnname, namelen);
    }
    function->exported = isexport;
    lit_parser_consume(parser, LIT_ASTTOKTYP_LEFT_PAREN, "'(' after function name");
    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);
    lit_parser_parseparams(parser, &function->parameters);
    if(function->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "function can't have more than 255 arguments, got %i", (int)function->parameters.count);
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after function arguments");
    function->body = lit_parser_parsestmt(parser);
    lit_parser_scopeend(parser);
    lit_parser_compilerend(parser, &compiler);
    return (LitExpression*)function;
}

LitExpression* lit_parser_parsereturn(LitParser* parser)
{
    size_t line = parser->previous.line;
    LitExpression* expression = NULL;
    if(!lit_parser_check(parser, LIT_ASTTOKTYP_NEW_LINE) && !lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_BRACE))
    {
        expression = lit_parser_parseexpr(parser);
    }
    return (LitExpression*)lit_ast_makereturnstmt(line, expression);
}

LitExpression* lit_parser_parsefield(LitParser* parser, LitString* name, bool is_static)
{
    size_t line = parser->previous.line;
    LitExpression* getter = NULL;
    LitExpression* setter = NULL;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_ARROW))
    {
        getter = lit_parser_parsestmt(parser);
    }
    else
    {
        /* will be LIT_ASTTOKTYP_LEFT_BRACE, otherwise this method won't be called */
        lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACE);
        lit_parser_ignorelinefeeds(parser);
        if(lit_parser_matchident(parser, "get"))
        {
            /* ignore it if it's present */
            lit_parser_match(parser, LIT_ASTTOKTYP_ARROW);
            getter = lit_parser_parsestmt(parser);
        }
        lit_parser_ignorelinefeeds(parser);
        if(lit_parser_matchident(parser, "set"))
        {
            /* Ignore it if it's present */
            lit_parser_match(parser, LIT_ASTTOKTYP_ARROW);
            setter = lit_parser_parsestmt(parser);
        }
        if(getter == NULL && setter == NULL)
        {
            lit_parser_failfmt(parser, "expected declaration of either getter or setter, got none");
        }
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACE, "'}' after field declaration");
    }
    return (LitExpression*)lit_ast_makefieldstmt(line, name, getter, setter, is_static);
}

static LitTokenType operators[] = { LIT_ASTTOKTYP_PLUS,         LIT_ASTTOKTYP_MINUS, LIT_ASTTOKTYP_STAR,       LIT_ASTTOKTYP_PERCENT, LIT_ASTTOKTYP_SLASH,         LIT_ASTTOKTYP_SHARP,

                                    LIT_ASTTOKTYP_BANG,         LIT_ASTTOKTYP_LESS,  LIT_ASTTOKTYP_LESS_EQUAL, LIT_ASTTOKTYP_GREATER, LIT_ASTTOKTYP_GREATER_EQUAL, LIT_ASTTOKTYP_EQUAL_EQUAL,

                                    LIT_ASTTOKTYP_LEFT_BRACKET,

                                    LIT_ASTTOKTYP_EOF };

LitExpression* lit_parser_parsemethod(LitParser* parser, bool is_static)
{
    if(lit_parser_match(parser, LIT_ASTTOKTYP_STATIC))
    {
        is_static = true;
    }
    LitString* name = NULL;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_OPERATOR))
    {
        if(is_static)
        {
            lit_parser_failfmt(parser, "operator methods can't be static or defined in static classes");
        }
        size_t i = 0;
        while(operators[i] != LIT_ASTTOKTYP_EOF)
        {
            if(lit_parser_match(parser, operators[i]))
            {
                break;
            }
            i++;
        }
        if(parser->previous.type == LIT_ASTTOKTYP_LEFT_BRACKET)
        {
            lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACKET, "']' after '[' in op method declaration");
            name = lit_string_copylen(parser->pstate, "[]", 2);
        }
        else
        {
            name = lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length);
        }
    }
    else
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "method name");
        name = lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length);
        if(lit_parser_check(parser, LIT_ASTTOKTYP_LEFT_BRACE) || lit_parser_check(parser, LIT_ASTTOKTYP_ARROW))
        {
            return lit_parser_parsefield(parser, name, is_static);
        }
    }
    LitMethodStatement* method = lit_ast_makemethoddefstmt(parser->previous.line, name, is_static);
    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_LEFT_PAREN, "'(' after method name");
    lit_parser_parseparams(parser, &method->parameters);
    if(method->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "function can't have more than 255 arguments, got %i", (int)method->parameters.count);
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_PAREN, "')' after method arguments");
    method->body = lit_parser_parsestmt(parser);
    lit_parser_scopeend(parser);
    lit_parser_compilerend(parser, &compiler);
    return (LitExpression*)method;
}

LitExpression* lit_parser_parseclass(LitParser* parser)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }
    size_t line = parser->previous.line;
    bool is_static = parser->previous.type == LIT_ASTTOKTYP_STATIC;
    if(is_static)
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_CLASS, "'class' after 'static'");
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "class name after 'class'");
    LitString* name = lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length);
    LitString* super = NULL;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_COLON))
    {
        lit_parser_consume(parser, LIT_ASTTOKTYP_IDENTIFIER, "super class name after ':'");
        super = lit_string_copylen(parser->pstate, parser->previous.start, parser->previous.length);
        if(super == name)
        {
            lit_parser_failfmt(parser, "class cannot inherit itself");
        }
    }
    LitClassStatement* klass = lit_ast_makeclassdefstmt(line, name, super);
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LIT_ASTTOKTYP_LEFT_BRACE, "'{' before class body");
    lit_parser_ignorelinefeeds(parser);
    bool finishedparsingfields = false;
    while(!lit_parser_check(parser, LIT_ASTTOKTYP_RIGHT_BRACE))
    {
        bool fieldisstatic = false;
        if(lit_parser_match(parser, LIT_ASTTOKTYP_STATIC))
        {
            fieldisstatic = true;
            if(lit_parser_match(parser, LIT_ASTTOKTYP_VAR))
            {
                if(finishedparsingfields)
                {
                    lit_parser_failfmt(parser, "all static fields must be defined before the methods");
                }
                LitExpression* var = lit_parser_parsevardecl(parser);
                if(var != NULL)
                {
                    lit_exprlist_push(&klass->fields, var);
                }
                lit_parser_ignorelinefeeds(parser);
                continue;
            }
            else
            {
                finishedparsingfields = true;
            }
        }
        LitExpression* method = lit_parser_parsemethod(parser, is_static || fieldisstatic);
        if(method != NULL)
        {
            lit_exprlist_push(&klass->fields, method);
        }
        lit_parser_ignorelinefeeds(parser);
    }
    lit_parser_consume(parser, LIT_ASTTOKTYP_RIGHT_BRACE, "'}' after class body");
    return (LitExpression*)klass;
}

void lit_parser_sync(LitParser* parser)
{
    parser->panic_mode = false;
    while(parser->current.type != LIT_ASTTOKTYP_EOF)
    {
        if(parser->previous.type == LIT_ASTTOKTYP_NEW_LINE)
        {
            longjmp(jumpbuffer, 1);
            return;
        }
        switch(parser->current.type)
        {
            case LIT_ASTTOKTYP_CLASS:
            case LIT_ASTTOKTYP_FUNCTION:
            case LIT_ASTTOKTYP_EXPORT:
            case LIT_ASTTOKTYP_VAR:
            case LIT_ASTTOKTYP_CONST:
            case LIT_ASTTOKTYP_FOR:
            case LIT_ASTTOKTYP_STATIC:
            case LIT_ASTTOKTYP_IF:
            case LIT_ASTTOKTYP_WHILE:
            case LIT_ASTTOKTYP_RETURN:
            {
                longjmp(jumpbuffer, 1);
                return;
            }
            default:
            {
                lit_parser_advance(parser);
            }
        }
    }
}

LitExpression* lit_parser_parsestmt(LitParser* parser)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }
    lit_parser_ignorelinefeeds(parser);
    if(lit_parser_match(parser, LIT_ASTTOKTYP_VAR) || lit_parser_match(parser, LIT_ASTTOKTYP_CONST))
    {
        return lit_parser_parsevardecl(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_IF))
    {
        return lit_parser_parseif(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_FOR))
    {
        return lit_parser_parsefor(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_WHILE))
    {
        return lit_parser_parsewhile(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_CONTINUE))
    {
        return (LitExpression*)lit_ast_makecontinuestmt(parser->previous.line);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_BREAK))
    {
        return (LitExpression*)lit_ast_makebreakstmt(parser->previous.line);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_FUNCTION) || lit_parser_match(parser, LIT_ASTTOKTYP_EXPORT))
    {
        return lit_parser_parsefunction(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_RETURN))
    {
        return lit_parser_parsereturn(parser);
    }
    else if(lit_parser_match(parser, LIT_ASTTOKTYP_LEFT_BRACE))
    {
        lit_parser_ignorelinefeeds(parser);
        return lit_parser_parseblock(parser);
    }
    LitExpression* expression = lit_parser_parseexpr(parser);
    return expression == NULL ? NULL : (LitExpression*)lit_ast_makeexprstmt(parser->previous.line, expression);
}

LitExpression* lit_parser_parsedecl(LitParser* parser)
{
    LitExpression* statement = NULL;
    if(lit_parser_match(parser, LIT_ASTTOKTYP_CLASS) || lit_parser_match(parser, LIT_ASTTOKTYP_STATIC))
    {
        statement = lit_parser_parseclass(parser);
    }
    else
    {
        statement = lit_parser_parsestmt(parser);
    }
    return statement;
}

bool lit_parser_parsesource(LitParser* parser, const char* filename, const char* source, LitDynListExpr* statements)
{
    parser->had_error = false;
    parser->panic_mode = false;
    lit_scanner_init(parser->pstate, parser->pstate->scanner, filename, source);
    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_advance(parser);
    lit_parser_ignorelinefeeds(parser);
    if(!lit_parser_isatend(parser))
    {
        do
        {
            LitExpression* statement = lit_parser_parsedecl(parser);
            if(statement != NULL)
            {
                lit_exprlist_push(statements, statement);
            }
            if(!lit_parser_matchlinefeed(parser))
            {
                if(lit_parser_match(parser, LIT_ASTTOKTYP_EOF))
                {
                    break;
                }
            }
        } while(!lit_parser_isatend(parser));
    }
    return parser->had_error || parser->pstate->scanner->had_error;
}


LitParseRule lit_parser_makerule(LitPrefixParseFn prefix, LitInfixParseFn infix, LitPrecedence precedence)
{
    LitParseRule pr;
    pr.prefix = prefix;
    pr.infix = infix;
    pr.precedence = precedence;
    return pr;
}

void lit_parser_setuprules()
{
    rules[LIT_ASTTOKTYP_LEFT_PAREN] = lit_parser_makerule(lit_parser_rulegroupingorlambda, lit_parser_parsecall, LIT_ASTPREC_CALL);
    rules[LIT_ASTTOKTYP_PLUS] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_TERM);
    rules[LIT_ASTTOKTYP_MINUS] = lit_parser_makerule(lit_parser_ruleunary, lit_parser_rulebinary, LIT_ASTPREC_TERM);
    rules[LIT_ASTTOKTYP_BANG] = lit_parser_makerule(lit_parser_ruleunary, lit_parser_rulebinary, LIT_ASTPREC_TERM);
    rules[LIT_ASTTOKTYP_STAR] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_STAR_STAR] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_SLASH] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_SHARP] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_STAR] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_STAR] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_BAR] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_BOR);
    rules[LIT_ASTTOKTYP_AMPERSAND] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_BAND);
    rules[LIT_ASTTOKTYP_TILDE] = lit_parser_makerule(lit_parser_ruleunary, NULL, LIT_ASTPREC_UNARY);
    rules[LIT_ASTTOKTYP_CARET] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_BOR);
    rules[LIT_ASTTOKTYP_LESS_LESS] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_SHIFT);
    rules[LIT_ASTTOKTYP_GREATER_GREATER] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_SHIFT);
    rules[LIT_ASTTOKTYP_PERCENT] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_FACTOR);
    rules[LIT_ASTTOKTYP_IS] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_IS);
    rules[LIT_ASTTOKTYP_NUMBER] = lit_parser_makerule(lit_parser_rulenumber, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_TRUE] = lit_parser_makerule(lit_parser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_FALSE] = lit_parser_makerule(lit_parser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_NULL] = lit_parser_makerule(lit_parser_ruleliteral, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_BANG_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_EQUALITY);
    rules[LIT_ASTTOKTYP_EQUAL_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_EQUALITY);
    rules[LIT_ASTTOKTYP_GREATER] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_COMPARISON);
    rules[LIT_ASTTOKTYP_GREATER_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_COMPARISON);
    rules[LIT_ASTTOKTYP_LESS] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_COMPARISON);
    rules[LIT_ASTTOKTYP_LESS_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulebinary, LIT_ASTPREC_COMPARISON);
    rules[LIT_ASTTOKTYP_STRING] = lit_parser_makerule(lit_parser_rulestring, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_INTERPOLATION] = lit_parser_makerule(lit_parser_ruleinterpolation, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_IDENTIFIER] = lit_parser_makerule(lit_parser_rulevarexpr, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_NEW] = lit_parser_makerule(lit_parser_rulenewexpr, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_PLUS_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_MINUS_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_STAR_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_SLASH_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_SHARP_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_PERCENT_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_CARET_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_BAR_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_AMPERSAND_EQUAL] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_PLUS_PLUS] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_MINUS_MINUS] = lit_parser_makerule(NULL, lit_parser_rulecompound, LIT_ASTPREC_COMPOUND);
    rules[LIT_ASTTOKTYP_AMPERSAND_AMPERSAND] = lit_parser_makerule(NULL, lit_parser_rulelogicaland, LIT_ASTPREC_AND);
    rules[LIT_ASTTOKTYP_BAR_BAR] = lit_parser_makerule(NULL, lit_parser_rulelogicalor, LIT_ASTPREC_AND);
    rules[LIT_ASTTOKTYP_QUESTION_QUESTION] = lit_parser_makerule(NULL, lit_parser_rulenullfilter, LIT_ASTPREC_NULL);
    rules[LIT_ASTTOKTYP_DOT] = lit_parser_makerule(NULL, lit_parser_ruledot, LIT_ASTPREC_CALL);
#if 0
        rules[LIT_ASTTOKTYP_SMALL_ARROW] = lit_parser_makerule(NULL, lit_parser_ruledot, LIT_ASTPREC_CALL);
#endif
    rules[LIT_ASTTOKTYP_DOT_DOT] = lit_parser_makerule(NULL, lit_parser_rulerange, LIT_ASTPREC_RANGE);
    rules[LIT_ASTTOKTYP_DOT_DOT_DOT] = lit_parser_makerule(lit_parser_rulevarexpr, NULL, LIT_ASTPREC_ASSIGNMENT);
    rules[LIT_ASTTOKTYP_LEFT_BRACKET] = lit_parser_makerule(lit_parser_rulearray, lit_parser_parsesubscript, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_LEFT_BRACE] = lit_parser_makerule(lit_parser_ruleobject, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_THIS] = lit_parser_makerule(lit_parser_rulethis, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_SUPER] = lit_parser_makerule(lit_parser_rulesuper, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_QUESTION] = lit_parser_makerule(NULL, lit_parser_ruleternaryorquestion, LIT_ASTPREC_EQUALITY);
    rules[LIT_ASTTOKTYP_REF] = lit_parser_makerule(lit_parser_rulereference, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_SEMICOLON] = lit_parser_makerule(lit_parser_rulenothing, NULL, LIT_ASTPREC_NONE);
    rules[LIT_ASTTOKTYP_FUNCTION] = lit_parser_makerule(lit_parser_rulefunction, NULL, LIT_ASTPREC_NONE);
}

void lit_privlist_init(LitDynListPriv* array)
{
    lit_privlist_reset(array);
}

void lit_privlist_reset(LitDynListPriv* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_privlist_destroy(LitDynListPriv* array)
{
    lit_sysmem_free(array->values);
    lit_privlist_reset(array);
}

void lit_privlist_push(LitDynListPriv* array, LitPrivate value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitPrivate*)lit_sysmem_realloc(array->values, sizeof(LitPrivate) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_loclist_init(LitDynListLoc* array)
{
    lit_loclist_reset(array);
}

void lit_loclist_reset(LitDynListLoc* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_loclist_destroy(LitDynListLoc* array)
{
    lit_sysmem_free(array->values);
    lit_loclist_reset(array);
}

void lit_loclist_push(LitDynListLoc* array, LitLocal value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitLocal*)lit_sysmem_realloc(array->values, sizeof(LitLocal) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_emitter_resolvestatements(LitEmitter* emitter, LitDynListExpr* statements)
{
    size_t i;
    for(i = 0; i < statements->count; i++)
    {
        resolve_statement(emitter, statements->values[i]);
    }
}

void lit_emitter_init(LitState* state, LitEmitter* emitter)
{
    lit_emitter_reset(state, emitter);
    lit_privlist_init(&emitter->privlist);
    lit_uintlist_init(&emitter->breaks);
    lit_uintlist_init(&emitter->continues);
}

void lit_emitter_reset(LitState* state, LitEmitter* emitter)
{
    emitter->pstate = state;
    emitter->loop_start = 0;
    emitter->emit_reference = 0;
    emitter->class_name = NULL;
    emitter->compiler = NULL;
    emitter->chunk = NULL;
    emitter->module = NULL;
    emitter->class_has_super = false;
}

void lit_emitter_destroy(LitEmitter* emitter)
{
    lit_uintlist_destroy(&emitter->breaks);
    lit_uintlist_destroy(&emitter->continues);
}

void lit_emitter_raiseerror(LitEmitter* emitter, size_t line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_state_raiseerror(emitter->pstate, LIT_ERROR_COMPILEERROR, lit_state_errorfmtv(emitter->pstate, line, fmt, args)->strbuf.data);
    va_end(args);
}

size_t lit_emitter_emittmp(LitEmitter* emitter)
{
    lit_chunk_push(emitter->chunk, 0, emitter->last_line);
    return emitter->chunk->compiledcodecount - 1;
}

void lit_emitter_patchinstr(LitEmitter* emitter, uint64_t position, uint64_t instruction)
{
    emitter->chunk->compiledcodechunk[position] = instruction;
}

void lit_emitter_emitabc(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, uint16_t b, uint16_t c)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->chunk, LIT_FORM_ABC_INSTRUCTION(opcode, a, b, c), emitter->last_line);
}

void lit_emitter_emitabx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, LITREGUINT32 bx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->chunk, LIT_FORM_ABX_INSTRUCTION(opcode, a, bx), emitter->last_line);
}

void lit_emitter_emitasbx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, LITREGINT32 sbx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->chunk, LIT_FORM_ASBX_INSTRUCTION(opcode, a, sbx), emitter->last_line);
}

/*
 * be very careful with the use of this function:
 * always reserve a register just before using it, do not wait around!
 */
uint32_t lit_emitter_reserveregister(LitEmitter* emitter)
{
    LitCompiler* compiler = emitter->compiler;
    if(compiler->registers_used == LIT_REGISTERS_MAX)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "too many registers required");
        return 0;
    }
    compiler->function->maxregisters = ((compiler->function->maxregisters) > (compiler->registers_used+1) ? (compiler->function->maxregisters) : (compiler->registers_used+1));
    ++compiler->registers_used;
    return compiler->registers_used - 1;
}


void lit_emitter_freeregister(LitEmitter* emitter, uint16_t reg)
{
    if(LIT_BIT_ISSET(reg, LIT_BITFLAG_REGISTER))
    {
        return;
    }
    LitCompiler* compiler = emitter->compiler;
    if(compiler->registers_used == 0)
    {
        return lit_emitter_raiseerror(emitter, emitter->last_line, "invalid register was freed");
    }
    compiler->registers_used--;
}

LitLocal lit_emitter_makelocal(const char* name, size_t length, int depth, bool captured, bool constant, uint8_t reg)
{
    LitLocal rt;
    rt.name = name;
    rt.length = length;
    rt.depth = depth;
    rt.captured = captured;
    rt.constant = constant;
    rt.reg = reg;
    return rt;
}

void lit_emitter_compilerinit(LitEmitter* emitter, LitCompiler* compiler, LitFunctionType type)
{
    lit_loclist_init(&compiler->locals);
    compiler->type = type;
    compiler->scope_depth = -1;
    compiler->enclosing = (struct LitCompiler*)emitter->compiler;
    compiler->skip_return = false;
    compiler->function = lit_object_makefunction(emitter->pstate, emitter->module);
    compiler->loop_depth = 0;
    compiler->registers_used = 0;
    emitter->compiler = compiler;
    const char* name = emitter->pstate->scanner->sourcefilename;
    if(emitter->compiler == NULL)
    {
        compiler->function->name = lit_string_copylen(emitter->pstate, name, strlen(name));
    }
    emitter->chunk = &compiler->function->chunk;
    if(type == LIT_FUNCTYPE_METHOD || type == LIT_FUNCTYPE_STATIC_METHOD || type == LIT_FUNCTYPE_CONSTRUCTOR)
    {
        lit_loclist_push(&compiler->locals, lit_emitter_makelocal("this", 4, -1, false, false, lit_emitter_reserveregister(emitter)));
    }
    else
    {
        lit_loclist_push(&compiler->locals, lit_emitter_makelocal("", 0, -1, false, false, lit_emitter_reserveregister(emitter)));
    }
}

LitFunction* lit_emitter_compilerend(LitEmitter* emitter, LitString* name)
{
    lit_emitter_freeregister(emitter, 0);
    if(emitter->compiler->registers_used > 0)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "not all registers were freed (%i left)", emitter->compiler->registers_used);
    }
    if(!emitter->compiler->skip_return)
    {
        uint8_t reg = lit_emitter_reserveregister(emitter);
        LitFunctionType type = emitter->compiler->type;
        if(type == LIT_FUNCTYPE_CONSTRUCTOR)
        {
            /* load <this> */
            lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, reg, 0, 0);
        }
        else
        {
            lit_emitter_emitabc(emitter, emitter->last_line, OP_LOAD_NULL, reg, 0, 0);
        }
        lit_emitter_emitabc(emitter, emitter->last_line, OP_RETURN, reg, 1, 0);
        
        lit_emitter_freeregister(emitter, reg);
        emitter->compiler->skip_return = true;
    }
    LitFunction* function = emitter->compiler->function;
    lit_loclist_destroy(&emitter->compiler->locals);
    emitter->compiler = (LitCompiler*)emitter->compiler->enclosing;
    emitter->chunk = emitter->compiler == NULL ? NULL : &emitter->compiler->function->chunk;
    if(name != NULL)
    {
        function->name = name;
    }
#ifdef LIT_TRACE_CHUNK
    if(!emitter->pstate->had_error)
    {
        lit_debug_disaschunk(&function->chunk, function->name->strbuf.data, NULL);
    }
#endif
    return function;
}

void lit_emitter_scopebegin(LitEmitter* emitter)
{
    emitter->compiler->scope_depth++;
}

void lit_emitter_scopeend(LitEmitter* emitter)
{
    if(emitter->compiler->scope_depth == -1)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "invalid scope ending");
    }
    emitter->compiler->scope_depth--;
    LitCompiler* compiler = emitter->compiler;
    LitDynListLoc* locals = &compiler->locals;
    while(locals->count > 0 && locals->values[locals->count - 1].depth > compiler->scope_depth)
    {
        LitLocal* local = &locals->values[locals->count - 1];
        if(local->captured)
        {
            lit_emitter_emitabc(emitter, emitter->last_line, OP_CLOSE_UPVALUE, local->reg, 0, 0);
        }
        lit_emitter_freeregister(emitter, local->reg);
        locals->count--;
    }
}

uint16_t lit_emitter_addconst(LitEmitter* emitter, size_t line, LitValue value)
{
    size_t constant = lit_chunk_addconstant(emitter->pstate, emitter->chunk, value);
    if(constant >= UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "too many constants for one chunk");
    }
    return constant;
}

int lit_emitter_addprivate(LitEmitter* emitter, const char* name, size_t length, size_t line, bool constant)
{
    int index;
    LitValue vidx;
    LitPrivate priv;
    LitState* state;
    LitString* key;
    LitTable* privnames;
    LitDynListPriv* privates;
    privates = &emitter->privlist;
    if(privates->count == UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "too many private locals for one module");
    }
    privnames = &emitter->module->privatenames->values;
    key = lit_table_find_string(privnames, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        lit_emitter_raiseerror(emitter, line, "variable '%.*s' was already declared in this scope", length, name);
        if(lit_table_getentry(privnames, key, &vidx))
        {
            return lit_value_asnumber(vidx);
        }
    }
    state = emitter->pstate;
    index = (int)privates->count;
    priv.initialized = false;
    priv.constant = constant;
    lit_privlist_push(privates, priv);
    lit_table_set(privnames, lit_string_copylen(state, name, length), lit_value_makenumber(index));
    emitter->module->privatecount++;
    return index;
}

int lit_emitter_resolveprivate(LitEmitter* emitter, const char* name, size_t length, size_t line)
{
    int numberindex;
    LitValue index;
    LitString* key;
    LitTable* privnames;
    privnames = &emitter->module->privatenames->values;
    key = lit_table_find_string(privnames, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        if(lit_table_getentry(privnames, key, &index))
        {
            numberindex = lit_value_asnumber(index);
            if(!emitter->privlist.values[numberindex].initialized)
            {
                lit_emitter_raiseerror(emitter, line, "variable '%.*s' can't use itself in its initializer", length, name);
            }
            return numberindex;
        }
    }
    return -1;
}

int lit_emitter_addlocal(LitEmitter* emitter, const char* name, size_t length, size_t line, bool constant, uint8_t reg)
{
    int i;
    LitCompiler* compiler = emitter->compiler;
    LitDynListLoc* locals = &compiler->locals;
    if(locals->count == UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "too many local variables for one function");
    }
    for(i = (int)locals->count - 1; i >= 0; i--)
    {
        LitLocal* local = &locals->values[i];
        if(local->depth != UINT16_MAX && local->depth < compiler->scope_depth)
        {
            break;
        }
        if(length == local->length && memcmp(local->name, name, length) == 0)
        {
            lit_emitter_raiseerror(emitter, line, "variable '%.*s' was already declared in this scope", length, name);
        }
    }
    lit_loclist_push(locals, lit_emitter_makelocal(name, length, UINT16_MAX, false, constant, reg));
    return (int)locals->count - 1;
}

int lit_emitter_resolvelocal(LitEmitter* emitter, LitCompiler* compiler, const char* name, size_t length, size_t line)
{
    int i;
    LitDynListLoc* locals = &compiler->locals;
    for(i = (int)locals->count - 1; i >= 0; i--)
    {
        LitLocal* local = &locals->values[i];
        if(local->length == length && memcmp(local->name, name, length) == 0)
        {
            if(local->depth == UINT16_MAX)
            {
                lit_emitter_raiseerror(emitter, line, "variable '%.*s' can't use itself in its initializer", length, name);
            }
            return i;
        }
    }
    return -1;
}

int lit_emitter_addupvalue(LitEmitter* emitter, LitCompiler* compiler, size_t index, size_t line, bool islocal)
{
    size_t i;
    size_t upvaluecount = compiler->function->upvaluecount;
    for(i = 0; i < upvaluecount; i++)
    {
        LitCompilerUpvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->isLocal == islocal)
        {
            return i;
        }
    }
    if(upvaluecount == UINT16_COUNT)
    {
        lit_emitter_raiseerror(emitter, line, "too many upvalues for one function");
        return 0;
    }
    compiler->upvalues[upvaluecount].isLocal = islocal;
    compiler->upvalues[upvaluecount].index = index;
    return compiler->function->upvaluecount++;
}

int lit_emitter_resolveupvalue(LitEmitter* emitter, LitCompiler* compiler, const char* name, size_t length, size_t line)
{
    if(compiler->enclosing == NULL)
    {
        return -1;
    }
    int local = lit_emitter_resolvelocal(emitter, (LitCompiler*)compiler->enclosing, name, length, line);
    if(local != -1)
    {
        ((LitCompiler*)compiler->enclosing)->locals.values[local].captured = true;
        return lit_emitter_addupvalue(emitter, compiler, local, line, true);
    }
    int upvalue = lit_emitter_resolveupvalue(emitter, (LitCompiler*)compiler->enclosing, name, length, line);
    if(upvalue != -1)
    {
        return lit_emitter_addupvalue(emitter, compiler, upvalue, line, false);
    }
    return -1;
}

void lit_emitter_marklocalinit(LitEmitter* emitter, size_t index)
{
    emitter->compiler->locals.values[index].depth = emitter->compiler->scope_depth;
}

void lit_emitter_markprivateinit(LitEmitter* emitter, size_t index)
{
    emitter->privlist.values[index].initialized = true;
}

void resolve_statement(LitEmitter* emitter, LitExpression* statement)
{
    if(statement == NULL)
    {
        return;
    }
    switch(statement->type)
    {
        case LIT_ASTEXPRTYP_VARDECL:
        {
            LitVarStatement* stmt = (LitVarStatement*)statement;
            lit_emitter_markprivateinit(emitter, lit_emitter_addprivate(emitter, stmt->name, stmt->length, statement->line, stmt->constant));
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            LitFunctionStatement* stmt = (LitFunctionStatement*)statement;
            if(!stmt->exported)
            {
                lit_emitter_markprivateinit(emitter, lit_emitter_addprivate(emitter, stmt->name, stmt->length, statement->line, false));
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

LitOpCode lit_emitter_translateunaryop(LitTokenType token)
{
    switch(token)
    {
        case LIT_ASTTOKTYP_MINUS:
            return OP_NEGATE;
        case LIT_ASTTOKTYP_BANG:
            return OP_NOT;
        case LIT_ASTTOKTYP_TILDE:
            return OP_BNOT;
        default:
            UNREACHABLE
    }
    return OP_RETURN;
}

LitOpCode lit_emitter_translatebinaryop(LitTokenType token)
{
    switch(token)
    {
        case LIT_ASTTOKTYP_BANG_EQUAL:
        case LIT_ASTTOKTYP_EQUAL_EQUAL:
            return OP_EQUAL;
        case LIT_ASTTOKTYP_LESS:
            return OP_LESS;
        case LIT_ASTTOKTYP_LESS_EQUAL:
            return OP_LESS_EQUAL;
        case LIT_ASTTOKTYP_GREATER:
            return OP_GREATER;
        case LIT_ASTTOKTYP_GREATER_EQUAL:
            return OP_GREATER_EQUAL;
        case LIT_ASTTOKTYP_PLUS:
            return OP_ADD;
        case LIT_ASTTOKTYP_MINUS:
            return OP_SUBTRACT;
        case LIT_ASTTOKTYP_STAR:
            return OP_MULTIPLY;
        case LIT_ASTTOKTYP_STAR_STAR:
            return OP_POWER;
        case LIT_ASTTOKTYP_SLASH:
            return OP_DIVIDE;
        case LIT_ASTTOKTYP_SHARP:
            return OP_FLOOR_DIVIDE;
        case LIT_ASTTOKTYP_PERCENT:
            return OP_MOD;
        case LIT_ASTTOKTYP_LESS_LESS:
            return OP_LSHIFT;
        case LIT_ASTTOKTYP_GREATER_GREATER:
            return OP_RSHIFT;
        case LIT_ASTTOKTYP_CARET:
            return OP_BXOR;
        case LIT_ASTTOKTYP_AMPERSAND:
            return OP_BAND;
        case LIT_ASTTOKTYP_BAR:
            return OP_BOR;
        case LIT_ASTTOKTYP_IS:
            return OP_IS;
        default:
            UNREACHABLE
    }
    return OP_RETURN;
}

uint16_t lit_emitter_parsearg(LitEmitter* emitter, LitExpression* expression, uint8_t reg)
{
    if(expression->type == LIT_ASTEXPRTYP_LITERAL)
    {
        LitValue value = ((LitLiteralExpression*)expression)->value;
        if(lit_value_isnumber(value) || IS_STRING(value))
        {
            uint16_t arg = lit_emitter_addconst(emitter, expression->line, value);
            /* Mark that this is a constant */
            LIT_BIT_SETBIT(arg, LIT_BITFLAG_CONSTANT);
            return arg;
        }
    }
    else if(expression->type == LIT_ASTEXPRTYP_VAR)
    {
        LitVarExpression* expr = ((LitVarExpression*)expression);
        int index = lit_emitter_resolvelocal(emitter, emitter->compiler, expr->name, expr->length, expression->line);
        if(index != -1)
        {
            return emitter->compiler->locals.values[index].reg;
        }
    }
    lit_emitter_emitexpr(emitter, expression, reg);
    return reg;
}

void lit_emitter_emitbinaryexpr(LitEmitter* emitter, LitBinaryExpression* expr, uint8_t reg, bool swap)
{
    LitTokenType op = expr->op;
    if(op == LIT_ASTTOKTYP_AMPERSAND_AMPERSAND || op == LIT_ASTTOKTYP_BAR_BAR || op == LIT_ASTTOKTYP_QUESTION_QUESTION)
    {
        lit_emitter_emitexpr(emitter, expr->left, reg);
        size_t jump = lit_emitter_emittmp(emitter);
        lit_emitter_emitexpr(emitter, expr->right, reg);
        lit_emitter_patchinstr(emitter, jump, LIT_FORM_ABX_INSTRUCTION(op == LIT_ASTTOKTYP_BAR_BAR ? OP_TRUE_JUMP : (op == LIT_ASTTOKTYP_QUESTION_QUESTION ? OP_NON_NULL_JUMP : OP_FALSE_JUMP), reg, emitter->chunk->compiledcodecount - jump - 1));
    }
    else
    {
        uint16_t b = lit_emitter_parsearg(emitter, expr->left, reg);
        LitOpCode opcode = lit_emitter_translatebinaryop(op);
        if(opcode == OP_IS)
        {
            if(expr->right->type != LIT_ASTEXPRTYP_VAR)
            {
                return lit_emitter_raiseerror(emitter, ((LitExpression*)expr)->line, "'is' operator is not used with a var expression");
            }
            LitVarExpression* e = (LitVarExpression*)expr->right;
            int constant = lit_emitter_addconst(emitter, ((LitExpression*)expr)->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, e->name, e->length)));
            lit_emitter_emitabc(emitter, ((LitExpression*)expr)->line, opcode, reg, b, constant);
        }
        else
        {
            uint16_t rc = lit_emitter_reserveregister(emitter);
            uint16_t c = lit_emitter_parsearg(emitter, expr->right, rc);
            lit_emitter_emitabc(emitter, ((LitExpression*)expr)->line, opcode, reg, swap ? c : b, swap ? b : c);
            lit_emitter_freeregister(emitter, rc);
        }
    }
}

bool lit_emitter_emitparams(LitEmitter* emitter, LitDynListParam* parameters, size_t line)
{
    size_t i;
    for(i = 0; i < parameters->count; i++)
    {
        LitParameter* parameter = &parameters->values[i];
        uint8_t reg = lit_emitter_reserveregister(emitter);
        parameter->reg = reg;
        int index = lit_emitter_addlocal(emitter, parameter->name, parameter->length, line, false, reg);
        lit_emitter_marklocalinit(emitter, index);
        /* variadic arg ...  */
        if(parameter->length == 3 && memcmp(parameter->name, "...", 3) == 0)
        {
            return true;
        }
        if(parameter->default_value != NULL)
        {
            size_t jump = lit_emitter_emittmp(emitter);
            lit_emitter_emitexpr(emitter, parameter->default_value, reg);
            lit_emitter_patchinstr(emitter, jump, LIT_FORM_ABX_INSTRUCTION(OP_NON_NULL_JUMP, reg, (int64_t)emitter->chunk->compiledcodecount - jump - 1));
        }
    }
    return false;
}

void lit_emitter_emitexprfull(LitEmitter* emitter, LitExpression* expression, uint8_t reg, bool ignored);

void lit_emitter_emitexprignoringregister(LitEmitter* emitter, LitExpression* expression)
{
    uint8_t reg = lit_emitter_reserveregister(emitter);
    lit_emitter_emitexprfull(emitter, expression, reg, true);
    lit_emitter_freeregister(emitter, reg);
}

void lit_emitter_emitexpr(LitEmitter* emitter, LitExpression* expression, uint8_t reg)
{
    lit_emitter_emitexprfull(emitter, expression, reg, false);
}

void lit_emitter_emitexprfull(LitEmitter* emitter, LitExpression* expression, uint8_t reg, bool ignored)
{
    if(expression == NULL)
    {
        return;
    }
    switch(expression->type)
    {
        case LIT_ASTEXPRTYP_LITERAL:
        {
            LitValue value = ((LitLiteralExpression*)expression)->value;
            if(lit_value_isnull(value))
            {
                lit_emitter_emitabc(emitter, expression->line, OP_LOAD_NULL, reg, 0, 0);
            }
            else if(lit_value_isbool(value))
            {
                lit_emitter_emitabc(emitter, expression->line, OP_LOAD_BOOL, reg, (uint8_t)lit_value_asbool(value), 0);
            }
            else
            {
                uint16_t constant = lit_emitter_addconst(emitter, expression->line, value);
                LIT_BIT_SETBIT(constant, LIT_BITFLAG_CONSTANT);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, constant, 0);
            }
            break;
        }
        case LIT_ASTEXPRTYP_UNARY:
        {
            LitUnaryExpression* expr = (LitUnaryExpression*)expression;
            uint16_t b = lit_emitter_parsearg(emitter, expr->right, reg);
            lit_emitter_emitabc(emitter, expression->line, lit_emitter_translateunaryop(expr->op), reg, b, 0);
            break;
        }
        case LIT_ASTEXPRTYP_BINARY:
        {
            LitBinaryExpression* expr = (LitBinaryExpression*)expression;
            switch(expr->op)
            {
                case LIT_ASTTOKTYP_GREATER:
                case LIT_ASTTOKTYP_GREATER_EQUAL:
                case LIT_ASTTOKTYP_LESS:
                case LIT_ASTTOKTYP_LESS_EQUAL:
                case LIT_ASTTOKTYP_EQUAL_EQUAL:
                {
                    lit_emitter_emitbinaryexpr(emitter, expr, reg, false);
                    break;
                }
                case LIT_ASTTOKTYP_BANG_EQUAL:
                {
                    lit_emitter_emitbinaryexpr(emitter, expr, reg, false);
                    lit_emitter_emitabc(emitter, expression->line, OP_NOT, reg, reg, false);
                    break;
                }
                default:
                {
                    return lit_emitter_emitbinaryexpr(emitter, expr, reg, false);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_VAR:
        {
            LitVarExpression* expr = (LitVarExpression*)expression;
            bool ref = emitter->emit_reference > 0;
            if(ref)
            {
                emitter->emit_reference--;
            }
            int index = lit_emitter_resolvelocal(emitter, emitter->compiler, expr->name, expr->length, expression->line);
            if(index == -1)
            {
                index = lit_emitter_resolveupvalue(emitter, emitter->compiler, expr->name, expr->length, expression->line);
                if(index == -1)
                {
                    index = lit_emitter_resolveprivate(emitter, expr->name, expr->length, expression->line);
                    if(index == -1)
                    {
                        uint16_t constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, expr->name, expr->length)));
                        if(ref)
                        {
                            lit_emitter_emitabx(emitter, expression->line, OP_REFERENCE_GLOBAL, reg, constant);
                        }
                        else
                        {
                            lit_emitter_emitabx(emitter, expression->line, OP_GET_GLOBAL, reg, constant);
                        }
                    }
                    else
                    {
                        if(ref)
                        {
                            lit_emitter_emitabx(emitter, expression->line, OP_REFERENCE_PRIVATE, reg, index);
                        }
                        else
                        {
                            lit_emitter_emitabx(emitter, expression->line, OP_GET_PRIVATE, reg, index);
                        }
                    }
                }
                else
                {
                    if(ref)
                    {
                        lit_emitter_emitabx(emitter, expression->line, OP_REFERENCE_UPVALUE, reg, index);
                    }
                    else
                    {
                        lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, reg, index);
                    }
                }
            }
            else
            {
                uint16_t r = emitter->compiler->locals.values[index].reg;
                if(ref)
                {
                    lit_emitter_emitabc(emitter, expression->line, OP_REFERENCE_LOCAL, reg, r, 0);
                }
                else if(reg != r)
                {
                    lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, r, 0);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_ASSIGN:
        {
            LitAssignExpression* expr = (LitAssignExpression*)expression;
            if(expr->to->type == LIT_ASTEXPRTYP_VAR)
            {
                LitVarExpression* e = (LitVarExpression*)expr->to;
                int index = lit_emitter_resolvelocal(emitter, emitter->compiler, e->name, e->length, expr->to->line);
                if(index == -1)
                {
                    uint16_t r = lit_emitter_parsearg(emitter, expr->value, reg);
                    index = lit_emitter_resolveupvalue(emitter, emitter->compiler, e->name, e->length, expr->to->line);
                    if(index == -1)
                    {
                        index = lit_emitter_resolveprivate(emitter, e->name, e->length, expr->to->line);
                        if(index == -1)
                        {
                            uint16_t constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, e->name, e->length)));
                            lit_emitter_emitabx(emitter, expression->line, OP_SET_GLOBAL, constant, r);
                        }
                        else
                        {
                            if(emitter->privlist.values[index].constant)
                            {
                                lit_emitter_raiseerror(emitter, expression->line, "attempt to modify constant '%.*s'", e->length, e->name);
                            }
                            if(LIT_BIT_ISSET(r, LIT_BITFLAG_CONSTANT))
                            {
                                LIT_BIT_SETBIT(index, 16);
                            }
                            lit_emitter_emitabx(emitter, expression->line, OP_SET_PRIVATE, r, index);
                        }
                    }
                    else
                    {
                        lit_emitter_emitabx(emitter, expression->line, OP_SET_UPVALUE, index, r);
                    }
                    if(!ignored && reg != r)
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, r, 0);
                    }
                    break;
                }
                else
                {
                    LitLocal local = emitter->compiler->locals.values[index];
                    if(local.constant)
                    {
                        lit_emitter_raiseerror(emitter, expression->line, "attempt to modify constant '%.*s'", e->length, e->name);
                    }
                    lit_emitter_emitexpr(emitter, expr->value, local.reg);
                    if(!ignored && reg != local.reg)
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, local.reg, 0);
                    }
                    break;
                }
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_SUBSCRIPT)
            {
                LitSubscriptExpression* e = (LitSubscriptExpression*)expr->to;
                lit_emitter_emitexpr(emitter, e->array, reg);
                uint8_t rega = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, e->index, rega);
                uint8_t regb = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, expr->value, regb);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_SUBSCRIPT_SET, reg, rega, regb);
                lit_emitter_freeregister(emitter, rega);
                lit_emitter_freeregister(emitter, regb);
                break;
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_GET)
            {
                LitGetExpression* e = (LitGetExpression*)expr->to;
                uint8_t r = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, e->where, r);
                lit_emitter_emitexpr(emitter, expr->value, reg);
                int constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, e->name, e->length)));
                lit_emitter_emitabc(emitter, expression->line, OP_SET_FIELD, r, constant, reg);
                lit_emitter_freeregister(emitter, r);
                break;
            }
            else if(expr->to->type == LIT_ASTEXPRTYP_REFERENCE)
            {
                uint8_t r = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, ((LitReferenceExpression*)expr->to)->to, r);
                lit_emitter_emitexpr(emitter, expr->value, reg);
                lit_emitter_emitabc(emitter, expression->line, OP_SET_REFERENCE, r, reg, 0);
                lit_emitter_freeregister(emitter, r);
                break;
            }
            lit_emitter_raiseerror(emitter, expression->line, "invalid assigment target");
            break;
        }
        case LIT_ASTEXPRTYP_CALL:
        {
            size_t i;
            LitCallExpression* expr = (LitCallExpression*)expression;
            size_t argc = expr->args.count;
            uint16_t* argregs = (uint16_t*)lit_sysmem_malloc(argc * sizeof(uint16_t));
            bool method = expr->excallee->type == LIT_ASTEXPRTYP_GET;
            bool super = expr->excallee->type == LIT_ASTEXPRTYP_SUPER;
            if(method)
            {
                ((LitGetExpression*)expr->excallee)->ignore_emit = true;
            }
            else if(super)
            {
                ((LitSuperExpression*)expr->excallee)->ignore_emit = true;
            }
            lit_emitter_emitexpr(emitter, expr->excallee, reg);
            uint8_t tmpreg = super ? lit_emitter_reserveregister(emitter) : 0;
            for(i = 0; i < argc; i++)
            {
                uint16_t supadd;
                uint16_t argreg = lit_emitter_reserveregister(emitter);
                LitExpression* e = expr->args.values[i];
                supadd = (super ? 2 : 1);
                if(argreg != ((reg + i) + supadd))
                {
                    /* something went terribly wrong */
                    
                    fprintf(stderr, "callexpression error on line %ld: argreg (%d) != ((reg (%d) + i (%ld)) + supadd (%d)) == %ld\n", expression->line, argreg, reg, i, supadd, ((reg + i) + supadd));
                    argreg = (reg + i + supadd) - 0;
#if 0
                        UNREACHABLE
#endif
                }
                argregs[i] = argreg;
                lit_emitter_emitexpr(emitter, e, argreg);
            }
            if(method)
            {
                if(expr->excallee->type != LIT_ASTEXPRTYP_GET)
                {
                    /* TODO: replace with a proper error code? */
                    UNREACHABLE
                }
                LitGetExpression* e = (LitGetExpression*)expr->excallee;
                int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copylen(emitter->pstate, e->name, e->length)));
                lit_emitter_emitabc(emitter, expression->line, OP_INVOKE, reg, argc + 1, constant);
            }
            else if(super)
            {
                assert(tmpreg == reg + 1);
                LitSuperExpression* e = (LitSuperExpression*)expr->excallee;
                size_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "super", 5, emitter->last_line);
                lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, tmpreg, index);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE_SUPER, reg, argc + 1, lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(e->method)));
                lit_emitter_freeregister(emitter, tmpreg);
            }
            else
            {
                lit_emitter_emitabc(emitter, expression->line, OP_CALL, reg, argc + 1, 1);
            }
            for(i = 0; i < argc; i++)
            {
                lit_emitter_freeregister(emitter, argregs[i]);
            }
            lit_sysmem_free(argregs);
            if(method)
            {
                LitExpression* get = expr->excallee;
                while(get != NULL)
                {
                    if(get->type == LIT_ASTEXPRTYP_GET)
                    {
                        LitGetExpression* getter = (LitGetExpression*)get;
                        if(getter->jump > 0)
                        {
                            lit_emitter_patchinstr(emitter, getter->jump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, reg, (int64_t)emitter->chunk->compiledcodecount - getter->jump - 1));
                        }
                        get = getter->where;
                    }
                    else if(get->type == LIT_ASTEXPRTYP_SUBSCRIPT)
                    {
                        get = ((LitSubscriptExpression*)get)->array;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(expr->init == NULL)
            {
                break;
            }
            LitObjectExpression* init = (LitObjectExpression*)expr->init;
            uint8_t r = lit_emitter_reserveregister(emitter);
            for(i = 0; i < init->values.count; i++)
            {
                LitExpression* e = init->values.values[i];
                emitter->last_line = e->line;
                lit_emitter_emitexpr(emitter, e, r);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_PUSH_OBJECT_ELEMENT, reg, lit_emitter_addconst(emitter, emitter->last_line, init->keys.values[i]), r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_ASTEXPRTYP_GET:
        {
            LitGetExpression* expr = (LitGetExpression*)expression;
            bool ref = emitter->emit_reference > 0;
            if(ref)
            {
                emitter->emit_reference--;
            }
            bool jump = expr->jump == 0;
            bool emit = !expr->ignore_emit;
            lit_emitter_emitexpr(emitter, expr->where, reg);
            if(jump)
            {
                expr->jump = lit_emitter_emittmp(emitter);
                if(!expr->ignore_emit)
                {
                    int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copylen(emitter->pstate, expr->name, expr->length)));
                    if(ref)
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_REFERENCE_FIELD, reg, reg, constant);
                    }
                    else
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_GET_FIELD, reg, reg, constant);
                    }
                }
                lit_emitter_patchinstr(emitter, expr->jump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, reg, (int64_t)emitter->chunk->compiledcodecount - expr->jump - 1));
            }
            else if(emit)
            {
                int constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, expr->name, expr->length)));
                if(ref)
                {
                    lit_emitter_emitabc(emitter, expression->line, OP_REFERENCE_FIELD, reg, reg, constant);
                }
                else
                {
                    lit_emitter_emitabc(emitter, expression->line, OP_GET_FIELD, reg, reg, constant);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_SET:
        {
            LitSetExpression* expr = (LitSetExpression*)expression;
            uint8_t wherereg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->where, wherereg);
            uint8_t valuereg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->value, valuereg);
            int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copylen(emitter->pstate, expr->name, expr->length)));
            lit_emitter_emitabc(emitter, emitter->last_line, OP_SET_FIELD, wherereg, constant, valuereg);
            if(!ignored && reg != valuereg)
            {
                /* pains me to do this, but we gotta ensure that the value is after the where */
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, reg, valuereg, 0);
            }
            lit_emitter_freeregister(emitter, wherereg);
            lit_emitter_freeregister(emitter, valuereg);
            break;
        }
        case LIT_ASTEXPRTYP_SUBSCRIPT:
        {
            LitSubscriptExpression* expr = (LitSubscriptExpression*)expression;
            lit_emitter_emitexpr(emitter, expr->array, reg);
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->index, r);
            lit_emitter_emitabc(emitter, expression->line, OP_SUBSCRIPT_GET, reg, r, 0);
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_ASTEXPRTYP_ARRAY:
        {
            size_t i;
            LitArrayExpression* expr = (LitArrayExpression*)expression;
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitabx(emitter, expression->line, OP_ARRAY, reg, expr->values.count);
            for(i = 0; i < expr->values.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->values.values[i], r);
                lit_emitter_emitabx(emitter, emitter->last_line, OP_PUSH_ARRAY_ELEMENT, reg, r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_ASTEXPRTYP_OBJECT:
        {
            size_t i;
            LitObjectExpression* expr = (LitObjectExpression*)expression;
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitabc(emitter, expression->line, OP_OBJECT, reg, 0, 0);
            for(i = 0; i < expr->values.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->values.values[i], r);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_PUSH_OBJECT_ELEMENT, reg, lit_emitter_addconst(emitter, emitter->last_line, expr->keys.values[i]), r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_ASTEXPRTYP_LAMBDA:
        {
            size_t i;
            LitFunctionStatement* expr = (LitFunctionStatement*)expression;
            LitString* name = AS_STRING(lit_string_valformat(emitter->pstate, "lambda @:@", lit_value_fromobject(emitter->module->name), lit_string_numbertostring(emitter->pstate, expression->line)));
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, LIT_FUNCTYPE_REGULAR);
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &expr->parameters, expression->line);
            bool ended = false;
            if(expr->body != NULL)
            {
                bool singleexpr = expr->body->type == LIT_ASTEXPRTYP_EXPRESSION;
                if(singleexpr)
                {
                    uint8_t r = lit_emitter_reserveregister(emitter);
                    compiler.skip_return = true;
                    lit_emitter_emitexpr(emitter, ((LitExpressionStatement*)expr->body)->expression, r);
                    lit_emitter_emitabc(emitter, expr->body->line, OP_RETURN, r, 0, 0);
                    lit_emitter_freeregister(emitter, r);
                }
                else
                {
                    ended = lit_emitter_emitstmt(emitter, expr->body);
                }
            }
            if(!ended)
            {
                lit_emitter_scopeend(emitter);
            }
            LitFunction* function = lit_emitter_compilerend(emitter, name);
            function->argcount = expr->parameters.count;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emitter, expression->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, functionreg, 0);
            if(closure)
            {
                lit_emitter_freeregister(emitter, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_RANGE:
        {
            LitRangeExpression* expr = (LitRangeExpression*)expression;
            lit_emitter_emitexpr(emitter, expr->to, reg);
            uint8_t regb = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->from, regb);
            lit_emitter_emitabc(emitter, expression->line, OP_RANGE, reg, regb, reg);
            lit_emitter_freeregister(emitter, regb);
            break;
        }
        case LIT_ASTEXPRTYP_INTERPOLATION:
        {
            size_t i;
            LitInterpolationExpression* expr = (LitInterpolationExpression*)expression;
            lit_emitter_emitabx(emitter, expression->line, OP_ARRAY, reg, expr->expressions.count);
            uint8_t r = lit_emitter_reserveregister(emitter);
            for(i = 0; i < expr->expressions.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->expressions.values[i], r);
                lit_emitter_emitabx(emitter, emitter->last_line, OP_PUSH_ARRAY_ELEMENT, reg, r);
            }
            lit_emitter_freeregister(emitter, r);
            lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, reg, 2, lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->pstate, "join"))));
            break;
        }
        case LIT_ASTEXPRTYP_THIS:
        {
            LitFunctionType type = emitter->compiler->type;
            if(type == LIT_FUNCTYPE_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'this' can't be used %s", "in static methods");
            }
            if(type == LIT_FUNCTYPE_CONSTRUCTOR || type == LIT_FUNCTYPE_METHOD)
            {
                /* the instance is always in register 0 */
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
            }
            else
            {
                if(emitter->compiler->enclosing == NULL)
                {
                    lit_emitter_raiseerror(emitter, expression->line, "'this' can't be used %s", "in functions outside of any class");
                }
                else
                {
                    size_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "this", 4, expression->line);
                    lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, reg, index);
                }
            }
            break;
        }
        case LIT_ASTEXPRTYP_TERNARY:
        {
            LitTernaryExpression* expr = (LitTernaryExpression*)expression;
            uint8_t condreg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->condition, condreg);
            size_t condbranchskip = lit_emitter_emittmp(emitter);
            lit_emitter_freeregister(emitter, condreg);
            int64_t start = emitter->chunk->compiledcodecount;
            lit_emitter_emitexpr(emitter, expr->if_branch, reg);
            size_t elseskip = lit_emitter_emittmp(emitter);
            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->compiledcodecount - start));
            int64_t elsestart = emitter->chunk->compiledcodecount;
            lit_emitter_emitexpr(emitter, expr->else_branch, reg);
            lit_emitter_patchinstr(emitter, elseskip, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - elsestart));
            break;
        }
        case LIT_ASTEXPRTYP_SUPER:
        {
            if(emitter->compiler->type == LIT_FUNCTYPE_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'super' can't be used %s", "in static methods");
            }
            else if(!emitter->class_has_super)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'super' can't be used in class '%s', because it doesn't have a super class", emitter->class_name->strbuf.data);
            }
            LitSuperExpression* expr = (LitSuperExpression*)expression;
            if(!expr->ignore_emit)
            {
                size_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "super", 5, emitter->last_line);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
                uint8_t tmpreg = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, tmpreg, index);
                lit_emitter_emitabc(emitter, expression->line, OP_GET_SUPER_METHOD, reg, tmpreg, lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(expr->method)));
                lit_emitter_freeregister(emitter, tmpreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_REFERENCE:
        {
            LitExpression* to = ((LitReferenceExpression*)expression)->to;
            if(to->type != LIT_ASTEXPRTYP_VAR && to->type != LIT_ASTEXPRTYP_GET && to->type != LIT_ASTEXPRTYP_THIS && to->type != LIT_ASTEXPRTYP_SUPER)
            {
                lit_emitter_raiseerror(emitter, expression->line, "invalid refence target");
                break;
            }
            int old = emitter->emit_reference;
            emitter->emit_reference++;
            lit_emitter_emitexpr(emitter, to, reg);
            emitter->emit_reference = old;
            break;
        }
        default:
        {
            lit_emitter_raiseerror(emitter, expression->line, "unknown expression with id '%i'", (int)expression->type);
            break;
        }
    }
}

void lit_emitter_patchloopjumps(LitEmitter* emitter, LitDynListUInt* breaks)
{
    size_t i;
    for(i = 0; i < breaks->count; i++)
    {
        lit_emitter_patchinstr(emitter, breaks->values[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - breaks->values[i] - 1));
    }
    lit_uintlist_destroy(breaks);
}

void lit_emitter_emitstmtscoped(LitEmitter* emitter, LitExpression* statement)
{
    lit_emitter_scopebegin(emitter);
    if(!lit_emitter_emitstmt(emitter, statement))
    {
        lit_emitter_scopeend(emitter);
    }
}

bool lit_emitter_emitstmt(LitEmitter* emitter, LitExpression* statement)
{
    if(statement == NULL)
    {
        return false;
    }
    switch(statement->type)
    {
        case LIT_ASTEXPRTYP_EXPRESSION:
        {
            lit_emitter_emitexprignoringregister(emitter, ((LitExpressionStatement*)statement)->expression);
            break;
        }
        case LIT_ASTEXPRTYP_BLOCK:
        {
            size_t i;
            LitDynListExpr* statements = &((LitBlockStatement*)statement)->statements;
            bool endedscope = false;
            for(i = 0; i < statements->count; i++)
            {
                if(lit_emitter_emitstmt(emitter, statements->values[i]))
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
            LitVarStatement* stmt = (LitVarStatement*)statement;
            uint16_t reg = lit_emitter_reserveregister(emitter);
            bool isprivate = emitter->compiler->enclosing == NULL && emitter->compiler->scope_depth == 0;
            if(stmt->init == NULL)
            {
                lit_emitter_emitabc(emitter, statement->line, OP_LOAD_NULL, reg, 0, 0);
            }
            else
            {
                lit_emitter_emitexpr(emitter, stmt->init, reg);
            }
            int index = isprivate ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) : lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, stmt->constant, reg);
            if(isprivate)
            {
                lit_emitter_markprivateinit(emitter, index);
                lit_emitter_emitabx(emitter, statement->line, OP_SET_PRIVATE, reg, index);
                lit_emitter_freeregister(emitter, reg);
            }
            else
            {
                lit_emitter_marklocalinit(emitter, index);
            }
            break;
        }
        case LIT_ASTEXPRTYP_IF:
        {
            size_t i;
            LitIfStatement* stmt = (LitIfStatement*)statement;
            uint16_t condreg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, stmt->condition, condreg);
            size_t condbranchskip = lit_emitter_emittmp(emitter);
            size_t elseskip = 0;
            lit_emitter_freeregister(emitter, condreg);
            int64_t start = emitter->chunk->compiledcodecount;
            lit_emitter_emitstmtscoped(emitter, stmt->if_branch);
            if(stmt->else_branch)
            {
                elseskip = lit_emitter_emittmp(emitter);
            }
            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->compiledcodecount - start));
            size_t endjumpcount = stmt->elseif_branches == NULL ? 0 : stmt->elseif_branches->count;
            uint64_t* endjumps = (uint64_t*)lit_sysmem_malloc(endjumpcount * sizeof(uint64_t));
            if(stmt->elseif_branches != NULL)
            {
                for(i = 0; i < stmt->elseif_branches->count; i++)
                {
                    LitExpression* e = stmt->elseif_conditions->values[i];
                    if(e == NULL)
                    {
                        continue;
                    }
                    uint8_t elseifcondreg = lit_emitter_reserveregister(emitter);
                    lit_emitter_emitexpr(emitter, e, elseifcondreg);
                    uint64_t nextjump = lit_emitter_emittmp(emitter);
                    lit_emitter_freeregister(emitter, elseifcondreg);
                    lit_emitter_emitstmtscoped(emitter, stmt->elseif_branches->values[i]);
                    endjumps[i] = lit_emitter_emittmp(emitter);
                    lit_emitter_patchinstr(emitter, nextjump, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, elseifcondreg, (int64_t)emitter->chunk->compiledcodecount - nextjump - 1));
                }
            }
            if(stmt->else_branch)
            {
                lit_emitter_emitstmtscoped(emitter, stmt->else_branch);
                lit_emitter_patchinstr(emitter, elseskip, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - elseskip - 1));
            }
            for(i = 0; i < endjumpcount; i++)
            {
                if(stmt->elseif_conditions->values[i] == NULL)
                {
                    continue;
                }
                lit_emitter_patchinstr(emitter, endjumps[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - endjumps[i] - 1));
            }
            lit_sysmem_free(endjumps);
            break;
        }
        case LIT_ASTEXPRTYP_FUNCTION:
        {
            size_t i;
            bool local;
            bool isexport;
            bool isprivate;
            int index;
            uint8_t reg;
            LitFunctionStatement* stmt;
            index = 0;
            stmt = (LitFunctionStatement*)statement;
            isexport = stmt->exported;
            isprivate = !isexport && emitter->compiler->enclosing == NULL && emitter->compiler->scope_depth == 0;
            local = !(isexport || isprivate);
            reg = 0;
            if(!isexport)
            {
                index = isprivate ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) : lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, false, reg = lit_emitter_reserveregister(emitter));
            }
            LitString* name = lit_string_copylen(emitter->pstate, stmt->name, stmt->length);
            if(local)
            {
                lit_emitter_marklocalinit(emitter, index);
            }
            else if(isprivate)
            {
                lit_emitter_markprivateinit(emitter, index);
            }
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, LIT_FUNCTYPE_REGULAR);
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &stmt->parameters, statement->line);
            if(!lit_emitter_emitstmt(emitter, stmt->body))
            {
                lit_emitter_scopeend(emitter);
            }
            LitFunction* function = lit_emitter_compilerend(emitter, name);
            function->argcount = stmt->parameters.count;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emitter, statement->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            if(isexport)
            {
                uint16_t nameconst = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(function->name));
                lit_emitter_emitabx(emitter, statement->line, OP_SET_GLOBAL, nameconst, functionreg);
            }
            else if(isprivate)
            {
                if(!closure)
                {
                    LIT_BIT_SETBIT(index, 16);
                }
                lit_emitter_emitabx(emitter, statement->line, OP_SET_PRIVATE, functionreg, index);
            }
            else
            {
                lit_emitter_emitabc(emitter, statement->line, OP_MOVE, reg, functionreg, 0);
            }
            if(closure)
            {
                lit_emitter_freeregister(emitter, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_RETURN:
        {
            LitReturnStatement* stmt = (LitReturnStatement*)statement;
            uint8_t reg = lit_emitter_reserveregister(emitter);
            if(stmt->expression == NULL)
            {
                lit_emitter_emitabc(emitter, statement->line, OP_LOAD_NULL, reg, 0, 0);
            }
            else
            {
                lit_emitter_emitexpr(emitter, stmt->expression, reg);
            }
            lit_emitter_scopeend(emitter);
            lit_emitter_emitabc(emitter, statement->line, OP_RETURN, reg, 0, 0);
            lit_emitter_freeregister(emitter, reg);
            return true;
        }
        case LIT_ASTEXPRTYP_WHILE:
        {
            LitWhileStatement* stmt = (LitWhileStatement*)statement;
            uint8_t reg = lit_emitter_reserveregister(emitter);
            size_t beforecond = lit_emitter_emittmp(emitter);
            emitter->loop_start = beforecond;
            emitter->compiler->loop_depth++;
            LitDynListUInt old_breaks = emitter->breaks;
            LitDynListUInt old_continues = emitter->continues;
            lit_uintlist_init(&emitter->breaks);
            lit_uintlist_init(&emitter->continues);
            lit_emitter_emitexpr(emitter, stmt->condition, reg);
            size_t tmpinstr = lit_emitter_emittmp(emitter);
            lit_emitter_emitstmtscoped(emitter, stmt->body);
            lit_emitter_patchloopjumps(emitter, &emitter->continues);
            lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)beforecond - emitter->chunk->compiledcodecount - 1);
            lit_emitter_patchinstr(emitter, tmpinstr, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, reg, emitter->chunk->compiledcodecount - tmpinstr - 1));
            lit_emitter_patchloopjumps(emitter, &emitter->breaks);
            emitter->breaks = old_breaks;
            emitter->continues = old_continues;
            emitter->compiler->loop_depth--;
            lit_emitter_freeregister(emitter, reg);
            break;
        }
        case LIT_ASTEXPRTYP_FOR:
        {
            size_t i;
            LitForStatement* stmt = (LitForStatement*)statement;
            emitter->compiler->loop_depth++;
            LitDynListUInt old_breaks = emitter->breaks;
            LitDynListUInt old_continues = emitter->continues;
            lit_uintlist_init(&emitter->breaks);
            lit_uintlist_init(&emitter->continues);
            lit_emitter_scopebegin(emitter);
            if(stmt->c_style)
            {
                if(stmt->var != NULL)
                {
                    lit_emitter_emitstmt(emitter, stmt->var);
                }
                else if(stmt->init != NULL)
                {
                    lit_emitter_emitexprignoringregister(emitter, stmt->init);
                }
                size_t start = emitter->chunk->compiledcodecount;
                size_t exitjump = 0;
                uint8_t condreg = 0;
                if(stmt->condition != NULL)
                {
                    condreg = lit_emitter_reserveregister(emitter);
                    lit_emitter_emitexpr(emitter, stmt->condition, condreg);
                    exitjump = lit_emitter_emittmp(emitter);
                }
                if(stmt->increment != NULL)
                {
                    size_t bodyjump = lit_emitter_emittmp(emitter);
                    size_t incrstart = emitter->chunk->compiledcodecount;
                    lit_emitter_emitexprignoringregister(emitter, stmt->increment);
                    lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->compiledcodecount - 1);
                    start = incrstart;
                    lit_emitter_patchinstr(emitter, bodyjump, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - bodyjump - 1));
                }
                emitter->loop_start = start;
                bool endedscope = false;
                emitter->loop_start = start;
                lit_emitter_scopebegin(emitter);
                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_ASTEXPRTYP_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitBlockStatement*)stmt->body)->statements;
                        for(i = 0; i < statements->count; i++)
                        {
                            if(lit_emitter_emitstmt(emitter, statements->values[i]))
                            {
                                endedscope = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        endedscope = lit_emitter_emitstmt(emitter, stmt->body);
                    }
                }
                lit_emitter_patchloopjumps(emitter, &emitter->continues);
                if(!endedscope)
                {
                    lit_emitter_scopeend(emitter);
                }
                lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->compiledcodecount - 1);
                if(stmt->condition != NULL)
                {
                    lit_emitter_patchinstr(emitter, exitjump, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->compiledcodecount - exitjump - 1));
                    lit_emitter_freeregister(emitter, condreg);
                }
            }
            else
            {
                size_t sequence = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "seq ", 4, statement->line, false, sequence));
                uint8_t condreg = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, stmt->condition, condreg);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, sequence, condreg, 0);
                size_t iterator = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "iter ", 5, statement->line, false, iterator));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_LOAD_NULL, iterator, 0, 0);
                size_t start = emitter->chunk->compiledcodecount;
                emitter->loop_start = emitter->chunk->compiledcodecount;
                /* iter = seq.iterator(iter) */
                uint8_t tmprega = lit_emitter_reserveregister(emitter);
                uint8_t tmpregb = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, tmprega, 2, lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->pstate, "iterator"))));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, iterator, tmprega, 0);
                /* if iter is null, just get out of the loop */
                size_t exitjump = lit_emitter_emittmp(emitter);
                lit_emitter_scopebegin(emitter);
                /* var i = seq.iteratorValue(iter) */
                LitVarStatement* var = (LitVarStatement*)stmt->var;
                size_t local = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, var->name, var->length, statement->line, false, local));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, tmprega, 2, lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->pstate, "iteratorValue"))));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, local, tmprega, 0);
                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_ASTEXPRTYP_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitBlockStatement*)stmt->body)->statements;
                        for(i = 0; i < statements->count; i++)
                        {
                            lit_emitter_emitstmt(emitter, statements->values[i]);
                        }
                    }
                    else
                    {
                        lit_emitter_emitstmt(emitter, stmt->body);
                    }
                }
                lit_emitter_patchloopjumps(emitter, &emitter->continues);
                lit_emitter_scopeend(emitter);
                lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->compiledcodecount - 1);
                lit_emitter_patchinstr(emitter, exitjump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, iterator, (int64_t)emitter->chunk->compiledcodecount - exitjump - 1));
                lit_emitter_freeregister(emitter, tmprega);
                lit_emitter_freeregister(emitter, tmpregb);
                lit_emitter_freeregister(emitter, condreg);
            }
            lit_emitter_patchloopjumps(emitter, &emitter->breaks);
            lit_emitter_scopeend(emitter);
            emitter->breaks = old_breaks;
            emitter->continues = old_continues;
            emitter->compiler->loop_depth--;
            break;
        }
        case LIT_ASTEXPRTYP_BREAK:
        {
            if(emitter->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emitter, statement->line, "cannot use '%s' outside of loops", "break");
            }
            lit_uintlist_push(&emitter->breaks, lit_emitter_emittmp(emitter));
            break;
        }
        case LIT_ASTEXPRTYP_CONTINUE:
        {
            if(emitter->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emitter, statement->line, "cannot use '%s' outside of loops", "continue");
            }
            lit_uintlist_push(&emitter->continues, lit_emitter_emittmp(emitter));
            break;
        }
        case LIT_ASTEXPRTYP_CLASS:
        {
            size_t i;
            LitClassStatement* stmt = (LitClassStatement*)statement;
            bool hasparent = stmt->parent != NULL;
            uint16_t b = 0;
            emitter->class_name = stmt->name;
            if(hasparent)
            {
                uint16_t constant = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(stmt->parent));
                b = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabx(emitter, statement->line, OP_GET_GLOBAL, b, constant);
            }
            int nameconst = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(stmt->name));
            uint8_t class_register = lit_emitter_reserveregister(emitter);
            emitter->class_register = class_register;
            lit_emitter_emitabc(emitter, statement->line, OP_CLASS, nameconst, hasparent ? b + 1 : 0, class_register);
            if(hasparent)
            {
                lit_emitter_freeregister(emitter, b);
                emitter->class_has_super = true;
                lit_emitter_scopebegin(emitter);
                size_t super = lit_emitter_addlocal(emitter, "super", 5, emitter->last_line, false, lit_emitter_reserveregister(emitter));
                lit_emitter_marklocalinit(emitter, super);
            }
            for(i = 0; i < stmt->fields.count; i++)
            {
                LitExpression* s = stmt->fields.values[i];
                if(s->type == LIT_ASTEXPRTYP_VARDECL)
                {
                    LitVarStatement* var = (LitVarStatement*)s;
                    uint8_t reg = lit_emitter_reserveregister(emitter);
                    lit_emitter_emitexpr(emitter, var->init, reg);
                    int fieldnameconst = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(lit_string_copylen(emitter->pstate, var->name, var->length)));
                    lit_emitter_emitabc(emitter, s->line, OP_STATIC_FIELD, class_register, fieldnameconst, reg);
                    lit_emitter_freeregister(emitter, reg);
                }
                else
                {
                    lit_emitter_emitstmt(emitter, s);
                }
            }
            if(stmt->parent != NULL)
            {
                lit_emitter_scopeend(emitter);
            }
            lit_emitter_freeregister(emitter, class_register);
            emitter->class_name = NULL;
            emitter->class_has_super = false;
            break;
        }
        case LIT_ASTEXPRTYP_METHOD:
        {
            size_t i;
            LitString* clsname;
            LitMethodStatement* stmt = (LitMethodStatement*)statement;
            bool constructor = stmt->name->strbuf.length == 11 && memcmp(stmt->name->strbuf.data, "constructor", 11) == 0;
            if(constructor && stmt->is_static)
            {
                lit_emitter_raiseerror(emitter, statement->line, "constructors cannot be static");
            }
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, constructor ? LIT_FUNCTYPE_CONSTRUCTOR : (stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD));
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &stmt->parameters, statement->line);
            if(!lit_emitter_emitstmt(emitter, stmt->body))
            {
                lit_emitter_scopeend(emitter);
            }
            clsname = (LitString*)lit_value_asobject(lit_value_fromobject(emitter->class_name));
            {
#if 0
                fprintf(stderr, "emitter->class_name=%.*s\n", clsname->strbuf.length, clsname->strbuf.data);
#endif
            }
            LitFunction* function = lit_emitter_compilerend(emitter, clsname);
            function->argcount = stmt->parameters.count;
            function->maxregisters += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->pstate, function);
                for(i = 0; i < function->upvaluecount; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];
                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }
                uint16_t constidx = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(clsproto));
                lit_emitter_emitabx(emitter, statement->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(function));
                LIT_BIT_SETBIT(functionreg, LIT_BITFLAG_CONSTANT);
            }
            int fieldnameconst = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(stmt->name));
            lit_emitter_emitabc(emitter, statement->line, stmt->is_static ? OP_STATIC_FIELD : OP_METHOD, emitter->class_register, fieldnameconst, functionreg);
            if(closure)
            {
                lit_emitter_freeregister(emitter, functionreg);
            }
            break;
        }
        case LIT_ASTEXPRTYP_FIELD:
        {
            LitFieldStatement* stmt = (LitFieldStatement*)statement;
            LitFunction* getter = NULL;
            LitFunction* setter = NULL;
            if(stmt->getter != NULL)
            {
                LitCompiler compiler;
                lit_emitter_compilerinit(emitter, &compiler, stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD);
                lit_emitter_scopebegin(emitter);
                if(stmt->getter->type == LIT_ASTEXPRTYP_EXPRESSION)
                {
                    uint8_t r = lit_emitter_reserveregister(emitter);
                    compiler.skip_return = true;
                    lit_emitter_emitexpr(emitter, ((LitExpressionStatement*)stmt->getter)->expression, r);
                    lit_emitter_emitabc(emitter, stmt->getter->line, OP_RETURN, r, 0, 0);
                    lit_emitter_freeregister(emitter, r);
                }
                if(!lit_emitter_emitstmt(emitter, stmt->getter))
                {
                    lit_emitter_scopeend(emitter);
                }
                getter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_valformat(emitter->pstate, "@:get @", lit_value_fromobject(emitter->class_name), stmt->name)));
            }
            if(stmt->setter != NULL)
            {
                LitCompiler compiler;
                lit_emitter_compilerinit(emitter, &compiler, stmt->is_static ? LIT_FUNCTYPE_STATIC_METHOD : LIT_FUNCTYPE_METHOD);
                uint8_t reg = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "value", 5, statement->line, false, reg));
                lit_emitter_scopebegin(emitter);
                if(!lit_emitter_emitstmt(emitter, stmt->setter))
                {
                    lit_emitter_scopeend(emitter);
                }
                lit_emitter_freeregister(emitter, reg);
                setter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_valformat(emitter->pstate, "@:set @", lit_value_fromobject(emitter->class_name), stmt->name)));
                setter->argcount = 1;
                setter->maxregisters++;
            }
            LitField* field = lit_object_makefield(emitter->pstate, (LitObject*)getter, (LitObject*)setter);
            int constant = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(field));
            LIT_BIT_SETBIT(constant, LIT_BITFLAG_CONSTANT);
            lit_emitter_emitabc(emitter, statement->line, stmt->is_static ? OP_STATIC_FIELD : OP_METHOD, emitter->class_register, lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(stmt->name)), constant);
            break;
        }
        default:
        {
            lit_emitter_raiseerror(emitter, statement->line, "unknown statement with id '%i'", (int)statement->type);
            break;
        }
    }
    return false;
}

LitModule* lit_emitter_emitmod(LitEmitter* emitter, LitDynListExpr* statements, LitString* modname)
{
    size_t i;
    LitPrivate priv;
    emitter->last_line = 1;
    emitter->emit_reference = 0;
    LitState* state = emitter->pstate;
    LitValue modulevalue;
    LitModule* module;
    bool isnew = false;
    if(lit_table_getentry(&emitter->pstate->vmstate.modules->values, modname, &modulevalue))
    {
        module = AS_MODULE(modulevalue);
    }
    else
    {
        module = lit_object_makemodule(emitter->pstate, modname);
        isnew = true;
    }
    emitter->module = module;
    size_t oldprivatescnt = module->privatecount;
    if(oldprivatescnt > 0)
    {
        LitDynListPriv* privates = &emitter->privlist;
        privates->count = oldprivatescnt - 1;
        priv.initialized = true;
        priv.constant = false;
        lit_privlist_push(privates, priv);
        for(i = 0; i < oldprivatescnt; i++)
        {
            privates->values[i].initialized = true;
        }
    }
    LitCompiler compiler;
    lit_emitter_compilerinit(emitter, &compiler, LIT_FUNCTYPE_SCRIPT);
    emitter->chunk = &compiler.function->chunk;
    lit_emitter_resolvestatements(emitter, statements);
    lit_emitter_scopebegin(emitter);
    bool endedscope = false;
    for(i = 0; i < statements->count; i++)
    {
        LitExpression* stmt = statements->values[i];
        if(lit_emitter_emitstmt(emitter, stmt))
        {
            endedscope = true;
            break;
        }
    }
    if(!endedscope)
    {
        lit_emitter_scopeend(emitter);
    }
    module->mainfunction = lit_emitter_compilerend(emitter, modname);
    if(isnew)
    {
        size_t total = emitter->privlist.count;
        module->privatevalues = (LitValue*)lit_sysmem_malloc(total * sizeof(LitValue));
        for(i = 0; i < total; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
        }
    }
    else
    {
        module->privatevalues = (LitValue*)lit_sysmem_realloc(module->privatevalues, sizeof(LitValue) * (module->privatecount));
        for(i = oldprivatescnt; i < module->privatecount; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
        }
    }
    lit_privlist_destroy(&emitter->privlist);
    if(isnew && !state->had_error)
    {
        lit_table_set(&state->vmstate.modules->values, modname, lit_value_fromobject(module));
    }
    module->ran = true;
    return module;
}

void lit_debug_disasmodule(LitIOStream* pr, LitModule* module, const char* source)
{
    lit_debug_disaschunk(pr, &module->mainfunction->chunk, module->mainfunction->name->strbuf.data, source);
}

void lit_debug_printconst(LitIOStream* pr, LitValue value)
{
    lit_iostream_writestring(pr, COLOR_CYAN);
    lit_value_printvalue(pr, value, true);
    lit_iostream_writestring(pr, COLOR_RESET);
}

void lit_debug_disaschunk(LitIOStream* pr, LitChunk* chunk, const char* name, const char* source)
{
    size_t i;
    size_t offset;
    LitDynListVal* values = &chunk->constantlist;
    lit_iostream_printf(pr, "^^ %s ^^\n", name);
    if(values->count > 0)
    {
        lit_iostream_printf(pr, "%sconstants:%s\n", COLOR_MAGENTA, COLOR_RESET);
        for(i = 0; i < values->count; i++)
        {
            LitValue value = values->values[i];
            lit_iostream_printf(pr, "% 4d ", i);
            lit_debug_printconst(pr, value);
            lit_iostream_printf(pr, "\n");
        }
    }
    lit_iostream_printf(pr, "%stext:%s\n", COLOR_MAGENTA, COLOR_RESET);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        lit_debug_disasinstr(pr, chunk, offset, source, false);
    }
    lit_iostream_printf(pr, "%shex:%s\n", COLOR_MAGENTA, COLOR_RESET);
    for(offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        lit_iostream_printf(pr, "%08lX ", chunk->compiledcodechunk[offset]);
    }
    lit_iostream_printf(pr, "\n");
    lit_iostream_printf(pr, "vv %s vv\n", name);
}

void lit_debug_printabcinstr(LitIOStream* pr, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s %lu \t%lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction), LIT_INSTRUCTION_B(instruction), LIT_INSTRUCTION_C(instruction));
}

void lit_debug_printabxinstr(LitIOStream* pr, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s %lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction), LIT_INSTRUCTION_BX(instruction));
}

void lit_debug_printasbxinstr(LitIOStream* pr, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s %lu \t%li\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction), LIT_INSTRUCTION_SBX(instruction));
}

void lit_debug_printregister(LitIOStream* pr, uint16_t reg)
{
    lit_iostream_printf(pr, " \t%hu", reg);
}

void lit_debug_printconstarg(LitIOStream* pr, LitChunk* chunk, uint16_t arg, bool indent)
{
    arg &= 0xff;
    lit_iostream_printf(pr, "%sc%hu (", indent ? " \t" : "", arg);
    lit_debug_printconst(pr, chunk->constantlist.values[arg]);
    lit_iostream_printf(pr, ")");
}

void lit_debug_printconstorregister(LitIOStream* pr, LitChunk* chunk, uint16_t arg)
{
    if(LIT_BIT_ISSET(arg, LIT_BITFLAG_CONSTANT))
    {
        lit_debug_printconstarg(pr, chunk, arg, true);
    }
    else
    {
        lit_debug_printregister(pr, arg);
    }
}

void lit_debug_printunaryinstr(LitIOStream* pr, LitChunk* chunk, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));
    lit_debug_printconstorregister(pr, chunk, LIT_INSTRUCTION_B(instruction));
    lit_iostream_printf(pr, "\n");
}

void lit_debug_printbinaryinstr(LitIOStream* pr, LitChunk* chunk, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));

    lit_debug_printconstorregister(pr, chunk, LIT_INSTRUCTION_B(instruction));
    lit_debug_printconstorregister(pr, chunk, LIT_INSTRUCTION_C(instruction));

    lit_iostream_printf(pr, "\n");
}

void lit_debug_printglobalinstr(LitIOStream* pr, LitChunk* chunk, uint64_t instruction, const char* name)
{
    lit_iostream_printf(pr, "%s%s%s%*s", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "");

    lit_debug_printconstarg(pr, chunk, LIT_INSTRUCTION_BX(instruction), false);
    lit_debug_printconstorregister(pr, chunk, LIT_INSTRUCTION_A(instruction));

    lit_iostream_printf(pr, "\n");
}

static LitDebugInstructionFn debuginstrfuncs[] = { lit_debug_printabcinstr, lit_debug_printabxinstr, lit_debug_printasbxinstr };

void lit_debug_disasinstr(LitIOStream* pr, LitChunk* chunk, size_t offset, const char* source, bool forceline)
{
    size_t line = lit_chunk_getline(chunk, offset);
    bool same = !chunk->haslineinfo || (offset > 0 && line == lit_chunk_getline(chunk, offset - 1));
    if(!same && source != NULL)
    {
        size_t index = 0;
        char* currentline = (char*)source;
        while(currentline)
        {
            char* nextline = strchr(currentline, '\n');
            char* prevline = currentline;
            index++;
            currentline = nextline ? (nextline + 1) : NULL;
            if(index == line)
            {
                char* outputline = prevline ? prevline : nextline;
                char c;
                while((c = *outputline) && (c == '\t' || c == ' '))
                {
                    outputline++;
                }
                lit_iostream_printf(pr, "%s        %.*s%s\n", COLOR_RED, nextline ? (int)(nextline - outputline) : (int)strlen(prevline), outputline, COLOR_RESET);
                break;
            }
        }
    }
    lit_iostream_printf(pr, "%04d ", offset);
    if(same && !forceline)
    {
        lit_iostream_printf(pr, "   | ");
    }
    else
    {
        lit_iostream_printf(pr, "%s%4d%s ", COLOR_BLUE, line, COLOR_RESET);
    }
    uint64_t instruction = chunk->compiledcodechunk[offset];
    uint8_t opcode = LIT_INSTRUCTION_OPCODE(instruction);
    switch(opcode)
    {
        case OP_MOVE:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "MOVE");
            break;
        case OP_ADD:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "ADD");
            break;
        case OP_SUBTRACT:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "SUBTRACT");
            break;
        case OP_MULTIPLY:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "MULTIPLY");
            break;
        case OP_DIVIDE:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "DIVIDE");
            break;
        case OP_NEGATE:
            lit_debug_printunaryinstr(pr, chunk, instruction, "NEGATE");
            break;
        case OP_NOT:
            lit_debug_printunaryinstr(pr, chunk, instruction, "NOT");
            break;
        case OP_EQUAL:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "EQUAL");
            break;
        case OP_LESS:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "LESS");
            break;
        case OP_LESS_EQUAL:
            lit_debug_printbinaryinstr(pr, chunk, instruction, "LESS_EQUAL");
            break;
        case OP_SET_GLOBAL:
            lit_debug_printglobalinstr(pr, chunk, instruction, "SET_GLOBAL");
            break;
        case OP_GET_GLOBAL:
            lit_debug_printglobalinstr(pr, chunk, instruction, "GET_GLOBAL");
            break;
        default:
        {
            switch(opcode)
            {
                /* A simple way to automatically generate case printers for all the opcodes */
                #define OPCODE(name, stringname, type)                           \
                    case OP_##name:                                              \
                    {                                                            \
                        debuginstrfuncs[(int)type](pr, instruction, stringname); \
                        break;                                                   \
                    }
                #if 1
                OPCODE(MOVE, "MOVE", LIT_INSTYP_ABC) /* R(A) := RC(B) */
                OPCODE(LOAD_NULL, "LOAD_NULL", LIT_INSTYP_ABC) /* R(A) := null */
                OPCODE(LOAD_BOOL, "LOAD_BOOL", LIT_INSTYP_ABC) /* R(A) := (bool) B */
                OPCODE(CLOSURE, "CLOSURE", LIT_INSTYP_ABX) /* R(A) := PrC[Bx] */
                OPCODE(ARRAY, "ARRAY", LIT_INSTYP_ABX) /* R(A) := new Array(Bx) */
                OPCODE(OBJECT, "OBJECT", LIT_INSTYP_ABC) /* R(A) = new Object() */
                OPCODE(RANGE, "RANGE", LIT_INSTYP_ABC) /* R(A) = new Range(RC(B), RC(C)) */
                OPCODE(RETURN, "RETURN", LIT_INSTYP_ABC) /* return R(A) */
                OPCODE(ADD, "ADD", LIT_INSTYP_ABC) /* R(A) := RC(B) + RC(C) */
                OPCODE(SUBTRACT, "SUBTRACT", LIT_INSTYP_ABC) /* R(A) := RC(B) - RC(C) */
                OPCODE(MULTIPLY, "MULTIPLY", LIT_INSTYP_ABC) /* R(A) := RC(B) * RC(C) */
                OPCODE(DIVIDE, "DIVIDE", LIT_INSTYP_ABC) /* R(A) := RC(B) / RC(C) */
                OPCODE(FLOOR_DIVIDE, "FLOOR_DIVIDE", LIT_INSTYP_ABC) /* R(A) := floor(RC(B) / RC(C)) */
                OPCODE(MOD, "MOD", LIT_INSTYP_ABC) /* R(A) := RC(B) % RC(C) */
                OPCODE(POWER, "POWER", LIT_INSTYP_ABC) /* R(A) := pow(RC(B), RC(C)) */
                OPCODE(LSHIFT, "LSHIFT", LIT_INSTYP_ABC) /* R(A) := RC(B) << RC(C) */
                OPCODE(RSHIFT, "RSHIFT", LIT_INSTYP_ABC) /* R(A) := RC(B) >> RC(C) */
                OPCODE(BXOR, "BXOR", LIT_INSTYP_ABC) /* R(A) := RC(B) ^ RC(C) */
                OPCODE(BAND, "BAND", LIT_INSTYP_ABC) /* R(A) := RC(B) & RC(C) */
                OPCODE(BOR, "BOR", LIT_INSTYP_ABC) /* R(A) := RC(B) | RC(C) */
                OPCODE(JUMP, "JUMP", LIT_INSTYP_ASBX) /* PC += sBx */
                OPCODE(TRUE_JUMP, "TRUE_JUMP", LIT_INSTYP_ABX) /* if (R(A)) PC += Bx */
                OPCODE(FALSE_JUMP, "FALSE_JUMP", LIT_INSTYP_ABX) /* if (not R(A)) PC += Bx */
                OPCODE(NON_NULL_JUMP, "NON_NULL_JUMP", LIT_INSTYP_ABX) /* if (R(A) != null) PC += Bx */
                OPCODE(NULL_JUMP, "NULL_JUMP", LIT_INSTYP_ABX) /* if (R(A) == null) PC += Bx */
                OPCODE(EQUAL, "EQUAL", LIT_INSTYP_ABC) /* R(A) := RC(B) == RC(C) */
                OPCODE(LESS, "LESS", LIT_INSTYP_ABC) /* R(A) := RC(B) < RC(C) */
                OPCODE(LESS_EQUAL, "LESS_EQUAL", LIT_INSTYP_ABC) /* R(A) := RC(B) <= RC(C) */
                OPCODE(GREATER, "GREATER", LIT_INSTYP_ABC) /* R(A) := RC(B) > RC(C) */
                OPCODE(GREATER_EQUAL, "GREATER_EQUAL", LIT_INSTYP_ABC) /* R(A) := RC(B) >= RC(C) */
                OPCODE(NEGATE, "NEGATE", LIT_INSTYP_ABC) /* R(A) := -RC(B) */
                OPCODE(NOT, "NOT", LIT_INSTYP_ABC) /* R(A) := !RC(B) */
                OPCODE(BNOT, "BNOT", LIT_INSTYP_ABC) /* R(A) := ~RC(B) */
                OPCODE(SET_GLOBAL, "SET_GLOBAL", LIT_INSTYP_ABX) /* G[C(A)] := RC(BX) */
                OPCODE(GET_GLOBAL, "GET_GLOBAL", LIT_INSTYP_ABX) /* R(A) := G[C(Bx)] */
                OPCODE(SET_UPVALUE, "SET_UPVALUE", LIT_INSTYP_ABX) /* U[A] := RC(Bx) */
                OPCODE(GET_UPVALUE, "GET_UPVALUE", LIT_INSTYP_ABX) /* R(A) := U[Bx] */
                OPCODE(SET_PRIVATE, "SET_PRIVATE", LIT_INSTYP_ABX) /* P[A] := RC(Bx) */
                OPCODE(GET_PRIVATE, "GET_PRIVATE", LIT_INSTYP_ABX) /* R(A) := P[C(Bx)] */
                OPCODE(CALL, "CALL", LIT_INSTYP_ABC) /* R(A) := R(A)(R(A + 1), ..., R(A + B - 1)) */
                OPCODE(CLOSE_UPVALUE, "CLOSE_UPVALUE", LIT_INSTYP_ABC) /* close_upvalue(R(A)) */
                OPCODE(CLASS, "CLASS", LIT_INSTYP_ABC) /* G[C(A)] = R[C] = new_class(C(A), C(B - 1)) */
                OPCODE(STATIC_FIELD, "STATIC_FIELD", LIT_INSTYP_ABC) /* R(A)[C(B)] = RC(C) */
                OPCODE(METHOD, "METHOD", LIT_INSTYP_ABC) /* R(A).Methods[C(B)] = RC(C) */
                OPCODE(GET_FIELD, "GET_FIELD", LIT_INSTYP_ABC) /* R(A) = R(B)[C(C)] */
                OPCODE(GET_SUPER_METHOD, "GET_SUPER_METHOD", LIT_INSTYP_ABC) /* R(A) = R(B).super[C(C)] */
                OPCODE(SET_FIELD, "SET_FIELD", LIT_INSTYP_ABC) /* R(A)[C(B)] = R(C) */
                OPCODE(IS, "IS", LIT_INSTYP_ABC) /* R(A) := RC(B) is G[C(C)] */
                OPCODE(INVOKE, "INVOKE", LIT_INSTYP_ABC) /* R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1)) */
                OPCODE(INVOKE_SUPER, "INVOKE_SUPER", LIT_INSTYP_ABC) /* R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1)) */
                OPCODE(SUBSCRIPT_GET, "SUBSCRIPT_GET", LIT_INSTYP_ABC) /* R(A) := R(A)[RC(B)] */
                OPCODE(SUBSCRIPT_SET, "SUBSCRIPT_SET", LIT_INSTYP_ABC) /* R(A)[RC(B)] := R(C) */
                OPCODE(PUSH_ARRAY_ELEMENT, "PUSH_ARRAY_ELEMENT", LIT_INSTYP_ABX) /* R(A)[R(A).count++] = RC(Bx) */
                OPCODE(PUSH_OBJECT_ELEMENT, "PUSH_OBJECT_ELEMENT", LIT_INSTYP_ABC) /* R(A)[R(B)] = RC(C) */
                OPCODE(REFERENCE_GLOBAL, "REFERENCE_GLOBAL", LIT_INSTYP_ABX) /* R(A) := ref G(C[Bx]) */
                OPCODE(REFERENCE_PRIVATE, "REFERENCE_PRIVATE", LIT_INSTYP_ABX) /* R(A) := ref P(Bx) */
                OPCODE(REFERENCE_LOCAL, "REFERENCE_LOCAL", LIT_INSTYP_ABC) /* R(A) := ref R(B) */
                OPCODE(REFERENCE_UPVALUE, "REFERENCE_UPVALUE", LIT_INSTYP_ABX) /* R(A) := ref U(Bx) */
                OPCODE(REFERENCE_FIELD, "REFERENCE_FIELD", LIT_INSTYP_ABC) /* R(A) = ref R(B)[C(C)] */
                OPCODE(SET_REFERENCE, "SET_REFERENCE", LIT_INSTYP_ABC) /* ref R(A) := R(B) */
                #endif
                #undef OPCODE
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
    lit_open_math_library(state);
    lit_open_file_library(state);
    lit_open_gc_library(state);
}

LitValue lit_objfndefault_invalidconstructor(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    lit_vm_raisefatalerror(state, "cannot create an instance of built-in type", lit_value_asinstance(instance)->klass->name);
    return lit_value_makenull();
}

/*
 * Class
 */

LitValue lit_objfnclass_tostring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_valformat(state, "class @", lit_value_fromobject(AS_CLASS(instance)->name));
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
        if(table->htentries[number].key != NULL)
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
    return lit_value_fromobject(table->htentries[index].key);
}

LitValue lit_objfnclass_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitClass* klass = AS_CLASS(instance);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int methodsCapacity = (int)klass->methods.htcapacity;
    bool fields = index >= methodsCapacity;
    int value = lit_coreutil_tableiterator(fields ? &klass->static_fields : &klass->methods, fields ? index - methodsCapacity : index);
    if(value == -1)
    {
        if(fields)
        {
            return lit_value_makenull();
        }
        index++;
        fields = true;
        value = lit_coreutil_tableiterator(&klass->static_fields, index - methodsCapacity);
    }
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(fields ? value + methodsCapacity : value);
}

LitValue lit_objfnclass_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    LitClass* klass = AS_CLASS(instance);
    size_t methodsCapacity = klass->methods.htcapacity;
    bool fields = index >= methodsCapacity;
    return lit_coreutil_tableiterkey(fields ? &klass->static_fields : &klass->methods, fields ? index - methodsCapacity : index);
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
        super = AS_CLASS(instance)->super;
    }
    if(super == NULL)
    {
        return lit_value_makenull();
    }
    return lit_value_fromobject(super);
}

LitValue lit_objfnclass_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitClass* klass = AS_CLASS(instance);
    if(argc == 2)
    {
        if(!IS_STRING(args[0]))
        {
            lit_vm_raisefatalerror(state, "class index must be a string");
        }
        lit_table_set(&klass->static_fields, AS_STRING(args[0]), args[1]);
        return args[1];
    }
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(state, "class index must be a string");
    }
    LitValue value;
    if(lit_table_getentry(&klass->static_fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&klass->methods, AS_STRING(args[0]), &value))
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
    return lit_value_fromobject(AS_CLASS(instance)->name);
}

/*
 * Object
 */

void lit_util_tabtoarray(LitState* state, LitArray* arr, LitTable* table)
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
        if(entry->key != NULL)
        {
            lit_array_push(arr, lit_value_fromobject(entry->key));
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
        lit_util_tabtoarray(state, arr, &map->values);
    }
    else if(lit_value_isinstance(val))
    {
        oinst = lit_value_asinstance(val);
        lit_util_tabtoarray(state, arr, &oinst->fields);
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
        type = OBJECT_TYPE(instance);
        lit_vm_raisefatalerror(state, "cannot modify built-in types");
    }
    LitInstance* inst = lit_value_asinstance(instance);
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(state, "object index must be a string");
    }
    if(argc == 2)
    {
        lit_table_set(&inst->fields, AS_STRING(args[0]), args[1]);
        return args[1];
    }
    LitValue value;
    if(lit_table_getentry(&inst->fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&inst->klass->static_fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_getentry(&inst->klass->methods, AS_STRING(args[0]), &value))
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
    LitString* self;
    LitString* other;
    LitString* result;
    size_t length;
    char* chars;

    (void)argc;
    self = AS_STRING(instance);
    other = lit_value_tostring(state, args[0], 0);

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
    return lit_value_makebool(strcmp(AS_STRING(instance)->strbuf.data, LIT_CHECK_GETSTRINGDATA(0)) < 0);
}

LitValue lit_objfnstring_greater(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    return lit_value_makebool(strcmp(AS_STRING(instance)->strbuf.data, LIT_CHECK_GETSTRINGDATA(0)) > 0);
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
    double result = strtod(AS_STRING(instance)->strbuf.data, NULL);
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_value_makenull();
    }
    return lit_value_makenumber(result);
}

void lit_util_stringchangecase(LitString* instr, int (*func)(int))
{
    size_t i;
    for(i = 0; i < instr->strbuf.length; i++)
    {
        instr->strbuf.data[i] = func(instr->strbuf.data[i]);
    }
}

LitValue lit_objfnstring_touppercase(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr;
    LitString* copied;
    (void)argc;
    (void)args;
    selfstr = AS_STRING(instance);
    copied = lit_string_clone(state, selfstr);
    lit_util_stringchangecase(copied, toupper);
    return lit_value_fromobject(copied);
}

LitValue lit_objfnstring_tolowercase(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr;
    LitString* copied;
    (void)argc;
    (void)args;
    selfstr = AS_STRING(instance);
    copied = lit_string_clone(state, selfstr);
    lit_util_stringchangecase(copied, tolower);
    return lit_value_fromobject(copied);
}

LitValue lit_objfnstring_contains(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr = AS_STRING(instance);
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
    LitString* selfstr = AS_STRING(instance);
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
    LitString* selfstr = AS_STRING(instance);
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
    if(!IS_STRING(args[0]) || !IS_STRING(args[1]))
    {
        lit_vm_raisefatalerror(state, "expected 2 string arguments");
    }
    selfstr = AS_STRING(instance);
    what = AS_STRING(args[0]);
    with = AS_STRING(args[1]);
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
    from = lit_strutil_utfucharoffset(string->strbuf.data, from);
    to = lit_strutil_utfucharoffset(string->strbuf.data, to);
    return lit_value_fromobject(lit_string_fromrange(state, string, from, to - from + 1));
}

LitValue lit_objfnstring_substring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);
    return lit_objfnstring_splice(state, AS_STRING(instance), from, to);
}


LitValue lit_objfnstring_charcodeat(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int cp;
    int index;
    LitString* selfstr = AS_STRING(instance);
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
    cp = lit_string_codepointcodeat(state, selfstr, lit_strutil_utfucharoffset(selfstr->strbuf.data, index));
    return lit_value_makenumber(cp);
}

LitValue lit_objfnstring_subscript(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    if(IS_RANGE(args[0]))
    {
        LitRange* range = AS_RANGE(args[0]);
        return lit_objfnstring_splice(state, AS_STRING(instance), range->from, range->to);
    }
    LitString* selfstr = AS_STRING(instance);
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
    LitString* c = lit_string_codepointstringat(state, selfstr, lit_strutil_utfucharoffset(selfstr->strbuf.data, index));
    return c == NULL ? lit_value_makenull() : lit_value_fromobject(c);
}

LitValue lit_objfnstring_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_string_utflength(AS_STRING(instance)));
}

LitValue lit_objfnstring_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitString* selfstr = AS_STRING(instance);
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
    LitString* selfstr = AS_STRING(instance);
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

    if(IS_FUNCTION(arg))
    {
        fiber = lit_object_makefiber(state, module, AS_FUNCTION(arg));
    }
    else
    {
        fiber = lit_object_makefiberclosure(state, module, AS_CLOSURE(arg));
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
    return lit_value_makebool(lit_coreutil_isfiberdone(AS_FIBER(instance)));
}

LitValue lit_objfnfiber_error(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return AS_FIBER(instance)->error;
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
    int i;
    uint8_t ai;
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
        LitFunction* function = frame->function;
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
            if(functionargcount == argc && IS_VARARG_ARRAY(*(frame->slots + functionargcount)))
            {
                /* no need to repack the arguments */
            }
            else
            {
                LitArray* array = &lit_object_makevararray(state)->array;
                lit_state_pushroot(state, (LitObject*)array);
                *(frame->slots + functionargcount) = lit_value_fromobject(array);
                int varargcount = argc - functionargcount + 1;
                if(varargcount > 0)
                {
                    lit_dynlistval_ensuresize(&array->values, varargcount);
                    for(i = 0; i < varargcount; i++)
                    {
                        array->values.values[i] = args[i + functionargcount - 1];
                    }
                }
                lit_state_poproot(state);
            }
        }
    }
    if(state->config.traceexecution)
    {
        fprintf(stderr, "fiber start:\n");
    }
}

LitValue lit_objfnfiber_run(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    lit_coreutil_runfiber(state, AS_FIBER(instance), args, argc, false);
    return lit_value_makenull();
}

LitValue lit_objfnfiber_try(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    state->vmstate.fiber->returnaddress = args - 1;
    lit_coreutil_runfiber(state, AS_FIBER(instance), args, argc, true);
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
    LitModule* module = IS_MODULE(instance) ? AS_MODULE(instance) : state->vmstate.fiber->module;
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
    return lit_string_valformat(state, "Module @", lit_value_fromobject(AS_MODULE(instance)->name));
}

LitValue lit_objfnmodule_name(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)state;
    return lit_value_fromobject(AS_MODULE(instance)->name);
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
    size_t length = array->values.count;
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
        lit_vallist_push(&newarray->values, array->values.values[from + i]);
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
        LitDynListVal* values = &lit_value_asarray(instance)->values;
        int64_t index = lit_value_asnumber(args[0]);
        if(index < 0)
        {
            index = fmax(0, values->count + index);
        }
        //fprintf(stderr, "Array[]: index=%ld\n", index);
        lit_dynlistval_ensuresize(values, index + 1);
        return values->values[index] = args[1];
    }
    if(!lit_value_isnumber(args[0]))
    {
        if(IS_RANGE(args[0]))
        {
            LitRange* range = AS_RANGE(args[0]);
            return lit_objfnarray_splice(state, lit_value_asarray(instance), (int)range->from, (int)range->to);
        }
        lit_vm_raisefatalerror(state, "array index must be a number, got a %s instead", lit_value_valtypename(args[0]));
        return lit_value_makenull();
    }
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    int index = lit_value_asnumber(args[0]);
    if(index < 0)
    {
        index = fmax(0, values->count + index);
    }
    if(values->capacity <= (size_t)index)
    {
        return lit_value_makenull();
    }
    return values->values[index];
}

LitValue lit_objfnarray_push(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LitArray* self;
    (void)state;
    self = lit_value_asarray(instance);
    for(i = 0; i < argc; i++)
    {
        lit_vallist_push(&self->values, args[i]);
    }
    return lit_value_makenull();
}

LitValue lit_objfnarray_insert(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    int i;
    LIT_ENSURE_ARGS(2);
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    int index = LIT_CHECK_NUMBER(0);
    if(index < 0)
    {
        index = fmax(0, values->count + index);
    }
    LitValue value = args[1];
    if((int)values->count <= index)
    {
        lit_dynlistval_ensuresize(values, index + 1);
    }
    else
    {
        lit_dynlistval_ensuresize(values, values->count + 1);
        for(i = values->count - 1; i > index; i--)
        {
            values->values[i] = values->values[i - 1];
        }
    }
    values->values[index] = value;
    return lit_value_makenull();
}

LitValue lit_objfnarray_addall(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    LIT_ENSURE_ARGS(1);
    if(!IS_ARRAY(args[0]))
    {
        lit_vm_raisefatalerror(state, "expected array as the argument");
    }
    LitArray* array = lit_value_asarray(instance);
    LitArray* toAdd = lit_value_asarray(args[0]);
    for(i = 0; i < toAdd->values.count; i++)
    {
        lit_vallist_push(&array->values, toAdd->values.values[i]);
    }
    return lit_value_makenull();
}

int lit_coreutil_indexof(LitState* state, LitArray* array, LitValue value)
{
    size_t i;
    LitValue* ptr;
    for(i = 0; i < array->values.count; i++)
    {
        ptr = &array->values.values[i];
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
    LitDynListVal* values = &array->values;
    size_t count = values->count;
    if(index >= count)
    {
        return lit_value_makenull();
    }
    LitValue value = values->values[index];
    if(index == count - 1)
    {
        values->values[index] = lit_value_makenull();
    }
    else
    {
        for(i = index; i < values->count - 1; i++)
        {
            values->values[i] = values->values[i + 1];
        }
        values->values[count - 1] = lit_value_makenull();
    }
    values->count--;
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
    lit_value_asarray(instance)->values.count = 0;
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
        if(number >= (int)array->values.count - 1)
        {
            return lit_value_makenull();
        }
        number++;
    }
    return array->values.count == 0 ? lit_value_makenull() : lit_value_makenumber(number);
}

LitValue lit_objfnarray_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    if(values->count <= index)
    {
        return lit_value_makenull();
    }
    return values->values[index];
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
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    for(i = 0; i < values->count; i++)
    {
        lit_state_callvalue(state, callback, &values->values[i], 1);
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
    LitDynListVal* values;
    (void)argc;
    (void)args;
    havejoinee = false;
    if(argc > 0)
    {
        joinee = args[0];
        havejoinee = true;
    }
    lit_iostream_makestackstring(&pr);
    values = &lit_value_asarray(instance)->values;
    for(i = 0; i < values->count; i++)
    {
        lit_value_printvalue(&pr, values->values[i], false);
        if((i + 1) < values->count)
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

bool sort_compare(LitState* state, LitValue a, LitValue b)
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
        while(i < pivotindex && sort_compare(state, l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && sort_compare(state, pivot, l[j]))
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
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    if(argc == 1 && lit_value_iscallablefunction(args[0]))
    {
        lit_coreutil_customquicksort(state, values->values, values->count, args[0]);
    }
    else
    {
        lit_coreutil_basicquicksort(state, values->values, values->count);
    }
    return instance;
}

LitValue lit_objfnarray_clone(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t i;
    (void)argc;
    (void)args;
    LitDynListVal* values = &lit_value_asarray(instance)->values;
    LitArray* array = lit_array_make(state);
    LitDynListVal* newvalues = &array->values;
    lit_dynlistval_ensuresize(newvalues, values->count);
    /* lit_dynlistval_ensuresize sets the count to max of previous count (0 in this case) and new count, so we have to reset it */
    newvalues->count = 0;
    for(i = 0; i < values->count; i++)
    {
        lit_vallist_push(newvalues, values->values[i]);
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
    return lit_value_makenumber(lit_value_asarray(instance)->values.count);
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
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(state, "map index must be a string");
    }
    LitMap* map = lit_value_asmap(instance);
    LitString* index = AS_STRING(args[0]);
    if(argc == 2)
    {
        LitValue val = args[1];
        lit_map_set(map, index, val);
        return val;
    }
    LitValue value;
    if(!lit_table_getentry(&map->values, index, &value))
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
    (void)state;
    (void)argc;
    (void)args;
    lit_value_asmap(instance)->values.htcount = 0;
    return lit_value_makenull();
}

LitValue lit_objfnmap_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int value = lit_coreutil_tableiterator(&lit_value_asmap(instance)->values, index);
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(value);
}

LitValue lit_objfnmap_itervalue(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t index = LIT_CHECK_NUMBER(0);
    return lit_coreutil_tableiterkey(&lit_value_asmap(instance)->values, index);
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
    LitTable* values = &lit_value_asmap(instance)->values;
    for(i = 0; i < values->htcapacity; i++)
    {
        LitTabEntry* entry = &values->htentries[i];
        if(entry->key != NULL)
        {
            callargs[0] = lit_value_fromobject(entry->key);
            callargs[1] = entry->value;
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
    lit_table_addall(&lit_value_asmap(instance)->values, &map->values);
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
    lit_value_printobjmap(&pr, (LitObject*)self, &self->values);
    dest = lit_iostream_takestring(state, &pr);
    return lit_value_fromobject(dest);
}

LitValue lit_objfnmap_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_value_asmap(instance)->values.htcount);
}

/*
 * Range
 */

LitValue lit_objfnrange_iterator(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitRange* range = AS_RANGE(instance);
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
    LitRange* range = AS_RANGE(instance);
    return lit_string_valformat(state, "Range(#, #)", range->from, range->to);
}

LitValue lit_objfnrange_from(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(AS_RANGE(instance)->from);
}

LitValue lit_objfnrange_setfrom(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    AS_RANGE(instance)->from = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue lit_objfnrange_to(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    return lit_value_makenumber(AS_RANGE(instance)->to);
}

LitValue lit_objfnrange_setto(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    AS_RANGE(instance)->to = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue lit_objfnrange_length(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)argc;
    (void)args;
    LitRange* range = AS_RANGE(instance);
    return lit_value_makenumber(range->to - range->from);
}

/*
 * Natives
 */

LitValue lit_corefn_srand(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    srand(lit_value_asnumber(args[0]));
    return lit_value_makenull();
}

LitValue lit_corefn_random(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    uint64_t iv;
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    iv = random();
    return lit_value_makenumber(iv);
}

LitValue lit_corefn_time(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber((double)clock() / CLOCKS_PER_SEC);
}

LitValue lit_corefn_systemtime(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)state;
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(time(NULL));
}

LitValue lit_corefn_printvalues(LitState* state, LitValue instance, size_t argc, LitValue* args)
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

LitValue lit_corefn_printchar(LitState* state, LitValue instance, size_t argc, LitValue* args)
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
        lit_iostream_writestringl(state->streamstdout, &ch, 1);
    }
    return lit_value_makenull();
}

LitValue lit_corefn_println(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    LitValue r;
    (void)instance;
    r = lit_corefn_printvalues(state, instance, argc, args);
    fprintf(stdout, "\n");
    return r;
}

LitValue lit_corefn_openlibrary(LitState* state, LitValue instance, size_t argc, LitValue* args)
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

bool interpret(LitState* state, LitModule* module)
{
    LitFunction* function = module->mainfunction;
    LitFiber* fiber = lit_object_makefiber(state, module, function);
    fiber->parent = state->vmstate.fiber;
    state->vmstate.fiber = fiber;
    return true;
}

bool lit_coreutil_compileandinterpret(LitState* state, LitString* modname, char* source)
{
    LitModule* module = lit_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        return false;
    }
    module->ran = true;
    return interpret(state, module);
}

LitValue lit_corefn_eval(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    char* code = (char*)LIT_CHECK_GETSTRINGDATA(0);
    //return lit_coreutil_compileandinterpret(state, state->vmstate.fiber->module->name, code);
    return lit_state_interpretsource(state, "<eval>", code).result;
}

void lit_state_opencorelibrary(LitState* state)
{
    LitClass* klass;
    bool wasallowed = state->allow_gc;
    state->allow_gc = false;
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
        state->class_class = klass;
        lit_state_setglobal(state, klass->name, lit_value_fromobject(klass));
        state->allow_gc = wasallowed;
    }
    {
        klass = lit_class_make(state, "Object", NULL);
        lit_class_inherit(klass, state->class_class);
        lit_class_bindstaticmethod(klass, "keys", lit_objfnobject_keys);
        lit_class_bindmethod(klass, "toString", lit_objfnobject_tostring);
        lit_class_bindmethod(klass, "isCallable", lit_objfnobject_iscallable);
        lit_class_bindmethod(klass, "[]", lit_objfnobject_subscript);
        lit_class_bindmethod(klass, "iterator", lit_objfnobject_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnobject_itervalue);
        lit_class_bindgetsetter(klass, "class", lit_objfnobject_class, NULL);
        state->object_class = klass;
        state->object_class->super = state->class_class;
    }
    {
        klass = lit_class_make(state, "Number", state->object_class);
        lit_class_inherit(klass, state->object_class);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindgetsetter(klass, "chr", lit_objfnnumber_chr, NULL);
        lit_class_bindmethod(klass, "toString", lit_objfnnumber_tostring);
        state->number_class = klass;
    }
    {
        klass = lit_class_make(state, "String", state->object_class);
        lit_class_inherit(klass, state->object_class);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindstaticmethod(klass, "chr", lit_objfnstring_chr);
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
        lit_class_bindmethod(klass, "iterator", lit_objfnstring_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnstring_itervalue);
        lit_class_bindmethod(klass, "[]", lit_objfnstring_subscript);
        lit_class_bindmethod(klass, "charAt", lit_objfnstring_subscript);
        lit_class_bindmethod(klass, "charCodeAt", lit_objfnstring_charcodeat);
        lit_class_bindgetsetter(klass, "length", lit_objfnstring_length, NULL);
        state->string_class = klass;
    }
    {
        klass = lit_class_make(state, "Bool", state->object_class);
        lit_class_inherit(klass, state->object_class);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "toString", lit_objfnbool_tostring);
        state->bool_class = klass;
    }
    {
        klass = lit_class_make(state, "Function", state->object_class);
        lit_class_inherit(klass, state->object_class);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "toString", lit_objfnfunction_tostring);
        lit_class_bindgetsetter(klass, "name", lit_objfnfunction_name, NULL);
        state->function_class = klass;
    }
    {
        klass = lit_class_make(state, "Fiber", state->object_class);
        lit_class_inherit(klass, state->object_class);
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
        klass = lit_class_make(state, "Module", state->object_class);
        lit_class_inherit(klass, state->object_class);
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
        klass = lit_class_make(state, "Array", state->object_class);
        lit_class_inherit(klass, state->object_class);
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
        klass = lit_class_make(state, "Map", state->object_class);
        lit_class_inherit(klass, state->object_class);
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
        klass = lit_class_make(state, "Range", state->object_class);
        lit_class_inherit(klass, state->object_class);
        lit_class_bindconstructor(klass, lit_objfndefault_invalidconstructor);
        lit_class_bindmethod(klass, "iterator", lit_objfnrange_iterator);
        lit_class_bindmethod(klass, "iteratorValue", lit_objfnrange_itervalue);
        lit_class_bindmethod(klass, "toString", lit_objfnrange_tostring);
        lit_class_bindgetsetter(klass, "from", lit_objfnrange_from, lit_objfnrange_setfrom);
        lit_class_bindgetsetter(klass, "to", lit_objfnrange_to, lit_objfnrange_setto);
        lit_class_bindgetsetter(klass, "length", lit_objfnrange_length, NULL);
        state->range_class = klass;
    }
    state->allow_gc = wasallowed;
    lit_state_defnative(state, "srand", lit_corefn_srand);
    lit_state_defnative(state, "random", lit_corefn_random);
    lit_state_defnative(state, "time", lit_corefn_time);
    lit_state_defnative(state, "systemTime", lit_corefn_systemtime);
    lit_state_defnative(state, "print", lit_corefn_printvalues);
    lit_state_defnative(state, "printchar", lit_corefn_printchar);
    lit_state_defnative(state, "println", lit_corefn_println);
    lit_state_defnative(state, "openLibrary", lit_corefn_openlibrary);
    lit_state_defnative(state, "eval", lit_corefn_eval);
    lit_state_setglobal(state, lit_string_copy(state, "globals"), lit_value_fromobject(state->vmstate.globals));
}

char* lit_read_file(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(file == NULL)
    {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)lit_sysmem_malloc(fileSize + 1);
    size_t bytesread = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesread] = '\0';
    fclose(file);
    return buffer;
}

bool lit_file_exists(const char* path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

void lit_write_uint8_t(FILE* file, uint8_t byte)
{
    fwrite(&byte, sizeof(uint8_t), 1, file);
}

void lit_write_uint16_t(FILE* file, uint16_t byte)
{
    fwrite(&byte, sizeof(uint16_t), 1, file);
}

void lit_write_uint32_t(FILE* file, uint32_t byte)
{
    fwrite(&byte, sizeof(uint32_t), 1, file);
}

void lit_util_writeuint64(FILE* file, uint64_t byte)
{
    fwrite(&byte, sizeof(uint64_t), 1, file);
}

void lit_util_writedouble(FILE* file, double byte)
{
    fwrite(&byte, sizeof(double), 1, file);
}

void lit_util_writestring(FILE* file, LitString* string)
{
    uint16_t i;
    uint16_t c = string->strbuf.length;
    fwrite(&c, sizeof(uint16_t), 1, file);
    for(i = 0; i < c; i++)
    {
        lit_write_uint8_t(file, (uint8_t)string->strbuf.data[i] ^ LIT_STRING_KEY);
    }
}

uint8_t lit_util_readuint8(FILE* file)
{
    size_t rsz;
    static uint8_t btmp;
    (void)rsz;
    rsz = fread(&btmp, sizeof(uint8_t), 1, file);
    return btmp;
}

LitString* lit_util_readstring(LitState* state, FILE* file)
{
    size_t i;
    size_t rsz;
    size_t length;
    LitString* res;
    (void)rsz;
    rsz = fread(&length, sizeof(uint16_t), 1, file);
    if(length < 1)
    {
        return NULL;
    }
    res = lit_string_makeemptystring(state, length, false);
    for(i = 0; i < length; i++)
    {
        res->strbuf.data[i] = (char)lit_util_readuint8(file) ^ LIT_STRING_KEY;
    }
    res->strbuf.length = length;
    return res;
}

void lit_init_emulated_file(LitEmulatedFile* file, const char* source)
{
    file->source = source;
    file->position = 0;
}

uint8_t lit_read_euint8_t(LitEmulatedFile* file)
{
    return (uint8_t)file->source[file->position++];
}

uint16_t lit_read_euint16_t(LitEmulatedFile* file)
{
    return (uint16_t)(lit_read_euint8_t(file) | (lit_read_euint8_t(file) << 8u));
}

uint32_t lit_read_euint32_t(LitEmulatedFile* file)
{
    return (uint32_t)(lit_read_euint8_t(file) | (lit_read_euint8_t(file) << 8u) | (lit_read_euint8_t(file) << 16u) | (lit_read_euint8_t(file) << 24u));
}

uint64_t lit_read_euint64_t(LitEmulatedFile* file)
{
    return (uint64_t)(lit_read_euint32_t(file) | ((uint64_t)lit_read_euint32_t(file) << 32u));
}

double lit_read_edouble(LitEmulatedFile* file)
{
    size_t i;
    uint8_t values[8];
    double result;
    for(i = 0; i < 8; i++)
    {
        values[i] = lit_read_euint8_t(file);
    }
    memcpy(&result, values, 8);
    return result;
}

LitString* lit_read_estring(LitState* state, LitEmulatedFile* file)
{
    LitString* res;
    size_t i;
    size_t length;
    length = lit_read_euint16_t(file);
    if(length < 1)
    {
        return NULL;
    }
    res = lit_string_makeemptystring(state, length, false);
    for(i = 0; i < length; i++)
    {
        res->strbuf.data[i] = (char)lit_read_euint8_t(file) ^ LIT_STRING_KEY;
    }
    res->strbuf.length = length;
    return res;
}

void save_function(FILE* file, LitFunction* function)
{
    save_chunk(file, &function->chunk);
    lit_util_writestring(file, function->name);
    lit_write_uint32_t(file, function->argcount);
    lit_write_uint32_t(file, function->upvaluecount);
    lit_write_uint8_t(file, (uint8_t)function->vararg);
    lit_write_uint8_t(file, (uint16_t)function->maxregisters);
}

LitFunction* load_function(LitState* state, LitEmulatedFile* file, LitModule* module)
{
    LitFunction* function = lit_object_makefunction(state, module);
    load_chunk(state, file, module, &function->chunk);
    function->name = lit_read_estring(state, file);
    function->argcount = lit_read_euint32_t(file);
    function->upvaluecount = lit_read_euint32_t(file);
    function->vararg = (bool)lit_read_euint8_t(file);
    function->maxregisters = lit_read_euint8_t(file);
    return function;
}

void save_chunk(FILE* file, LitChunk* chunk)
{
    size_t i;
    lit_write_uint32_t(file, chunk->compiledcodecount);
    for(i = 0; i < chunk->compiledcodecount; i++)
    {
        lit_util_writeuint64(file, chunk->compiledcodechunk[i]);
    }
    if(chunk->haslineinfo)
    {
        size_t c = chunk->linecount * 2 + 2;
        lit_write_uint32_t(file, c);
        for(i = 0; i < c; i++)
        {
            lit_write_uint16_t(file, chunk->lines[i]);
        }
    }
    else
    {
        lit_write_uint32_t(file, 0);
    }
    lit_write_uint32_t(file, chunk->constantlist.count);
    for(i = 0; i < chunk->constantlist.count; i++)
    {
        LitValue constant = chunk->constantlist.values[i];
        if(lit_value_isobject(constant))
        {
            LitObjType type = lit_value_asobject(constant)->type;
            lit_write_uint8_t(file, (uint8_t)(type + 1));
            switch(type)
            {
                case LIT_OBJ_STRING:
                {
                    lit_util_writestring(file, AS_STRING(constant));
                    break;
                }
                case LIT_OBJ_FUNCSCRIPT:
                {
                    save_function(file, AS_FUNCTION(constant));
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
            lit_write_uint8_t(file, 0);
            lit_util_writedouble(file, lit_value_asnumber(constant));
        }
    }
}

void load_chunk(LitState* state, LitEmulatedFile* file, LitModule* module, LitChunk* chunk)
{
    size_t i;
    lit_chunk_init(chunk);
    size_t count = lit_read_euint32_t(file);
    chunk->compiledcodechunk = (uint64_t*)lit_sysmem_malloc(sizeof(uint64_t) * count);
    chunk->compiledcodecount = count;
    chunk->capacity = count;
    for(i = 0; i < count; i++)
    {
        chunk->compiledcodechunk[i] = lit_read_euint64_t(file);
    }
    count = lit_read_euint32_t(file);
    if(count > 0)
    {
        chunk->lines = (uint16_t*)lit_sysmem_malloc(sizeof(uint16_t) * count);
        chunk->linecount = count;
        chunk->linecapacity = count;
        for(i = 0; i < count; i++)
        {
            chunk->lines[i] = lit_read_euint16_t(file);
        }
    }
    else
    {
        chunk->haslineinfo = false;
    }
    count = lit_read_euint32_t(file);
    chunk->constantlist.values = (LitValue*)lit_sysmem_malloc(sizeof(LitValue) * count);
    chunk->constantlist.count = count;
    chunk->constantlist.capacity = count;
    for(i = 0; i < count; i++)
    {
        uint8_t type = lit_read_euint8_t(file);
        if(type == 0)
        {
            chunk->constantlist.values[i] = lit_value_makenumber(lit_read_edouble(file));
        }
        else
        {
            switch((LitObjType)(type - 1))
            {
                case LIT_OBJ_STRING:
                {
                    chunk->constantlist.values[i] = lit_value_fromobject(lit_read_estring(state, file));
                    break;
                }
                case LIT_OBJ_FUNCSCRIPT:
                {
                    chunk->constantlist.values[i] = lit_value_fromobject(load_function(state, file, module));
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

void lit_save_module(LitModule* module, FILE* file)
{
    size_t i;
    bool disabled;
    disabled = false;
    lit_util_writestring(file, module->name);
    lit_write_uint16_t(file, module->privatecount);
    lit_write_uint8_t(file, (uint8_t)disabled);
    if(!disabled)
    {
        LitTable* privates = &module->privatenames->values;
        for(i = 0; i < module->privatecount; i++)
        {
            if(privates->htentries[i].key != NULL)
            {
                lit_util_writestring(file, privates->htentries[i].key);
                lit_write_uint16_t(file, (uint16_t)lit_value_asnumber(privates->htentries[i].value));
            }
        }
    }
    save_function(file, module->mainfunction);
}

LitModule* lit_load_module(LitState* state, const char* input)
{
    uint16_t j;
    uint16_t i;
    LitEmulatedFile file;
    lit_init_emulated_file(&file, input);
    if(lit_read_euint16_t(&file) != LIT_BYTECODE_MAGIC_NUMBER)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown magic number");
        return NULL;
    }
    uint8_t bytecodeversion = lit_read_euint8_t(&file);
    if(bytecodeversion > LIT_BYTECODE_VERSION)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown bytecode version '%i'", (int)bytecodeversion);
        return NULL;
    }
    uint16_t modulecount = lit_read_euint16_t(&file);
    LitModule* first = NULL;
    for(j = 0; j < modulecount; j++)
    {
        LitModule* module = lit_object_makemodule(state, lit_read_estring(state, &file));
        LitTable* privates = &module->privatenames->values;
        uint16_t privatescount = lit_read_euint16_t(&file);
        bool enabled = !((bool)lit_read_euint8_t(&file));
        module->privatevalues = (LitValue*)lit_sysmem_malloc(privatescount * sizeof(LitValue));
        module->privatecount = privatescount;
        for(i = 0; i < privatescount; i++)
        {
            module->privatevalues[i] = lit_value_makenull();
            if(enabled)
            {
                LitString* name = lit_read_estring(state, &file);
                lit_table_set(privates, name, lit_value_makenumber(lit_read_euint16_t(&file)));
            }
        }
        module->mainfunction = load_function(state, &file, module);
        lit_table_set(&state->vmstate.modules->values, module->name, lit_value_fromobject(module));
        if(j == 0)
        {
            first = module;
        }
    }
#if 0
    if(lit_read_euint16_t(&file) != LIT_BYTECODE_END_NUMBER)
    {
        lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to read compiled code, unknown end number");
        return NULL;
    }
#endif
    return first;
}

LitClass* lit_class_make(LitState* state, const char* name, LitClass* super)
{
    LitClass* klass = lit_object_makeclass(state, lit_string_copylen(state, name, strlen(name)));
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
    lit_table_set(&selfclass->static_fields, nm, lit_value_fromobject(lit_object_makefield(state, (LitObject*)lit_object_makenativemethod(state, getter, nm), NULL)));
}

void lit_class_inherit(LitClass* selfclass, LitClass* other)
{
    selfclass->super = (LitClass*)other;
    if(selfclass->init_method == NULL)
    {
        selfclass->init_method = other->init_method;
    }
    lit_table_addallignoring(&other->methods, &selfclass->methods);
    lit_table_addallignoring(&other->static_fields, &selfclass->static_fields);
}

void lit_class_bindmethod(LitClass* selfclass, const char* name, LitNativeFunctionFn fn)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->methods, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}


void lit_class_bindconstructor(LitClass* selfclass, LitNativeFunctionFn fn)
{
    const char* fname;
    LitString* nm;
    LitFuncNative* meth;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    fname = "constructor";
    nm = lit_string_copylen(state, fname, strlen(fname));
    meth = lit_object_makenativemethod(state, fn, nm);
    selfclass->init_method = (LitObject*)meth;
    lit_table_set(&selfclass->methods, nm, lit_value_fromobject(meth));
}

void lit_class_bindstaticmethod(LitClass* selfclass, const char* name, LitNativeFunctionFn fn)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->static_fields, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}

void lit_class_setstaticfield(LitClass* selfclass, const char* name, LitValue val)
{
    LitString* nm;
    LitState* state;
    state = ((LitObject*)selfclass)->pstate;
    nm = lit_string_copylen(state, name, strlen(name));
    lit_table_set(&selfclass->static_fields, nm, val);
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
    lit_table_set(&selfclass->methods, nm, lit_value_fromobject(lit_object_makefield(state, mthget, mthset)));
}

void cleanup_file(LitState* state, LitUserdata* data, bool mark)
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
    LitFileData* data = (LitFileData*)lit_userdata_insertdata(state, instance, sizeof(LitFileData), cleanup_file);
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
    return lit_value_makebool(lit_file_exists(fname));
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

LitValue lit_objfnfile_writevalues(LitState* state, LitValue instance, size_t argc, LitValue* args)
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

LitValue lit_objfnfile_writestring(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    size_t wr;
    size_t maxlen;
    LitString* string;
    string = LIT_CHECK_GETSTRINGOBJECT(0);
    if(string == NULL)
    {
        return lit_value_makenull();
    }
    string = AS_STRING(args[0]);
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
        lit_iostream_writestringl(&pr, topdname, topdlen);
        lit_iostream_writestringl(&pr, "/", 1);
        lit_iostream_writestringl(&pr, fname, fnlen);
        res = lit_iostream_takestring(state, &pr);
        struct stat st;
        stat(res->strbuf.data, &st);
        lit_vallist_push(&array->values, lit_value_fromobject(res));
    }
    closedir(dhnd);
    return lit_value_fromobject(array);
#endif
}

void lit_open_file_library(LitState* state)
{
    bool wasallowed = state->allow_gc;
    state->allow_gc = false;
    {
        LitClass* klass = lit_class_make(state, "File", state->object_class);
        lit_class_bindstaticmethod(klass, "exists", lit_objfnfile_exists);
        lit_class_bindstaticmethod(klass, "getLastModified", lit_objfnfile_getlastmodified);
        lit_class_bindstaticmethod(klass, "create", lit_objfnfile_create);
        lit_class_bindstaticmethod(klass, "read", lit_objfnfile_readallstatic);
        lit_class_bindconstructor(klass, lit_objfnfile_constructor);
        lit_class_bindmethod(klass, "close", lit_objfnfile_close);
        lit_class_bindmethod(klass, "write", lit_objfnfile_writevalues);
        lit_class_bindmethod(klass, "writeString", lit_objfnfile_writestring);
        lit_class_bindmethod(klass, "readAll", lit_objfnfile_readallinstance);
        lit_class_bindmethod(klass, "readLine", lit_objfnfile_readline);
    }
    {
        LitClass* klass = lit_class_make(state, "Directory", state->object_class);
        lit_class_bindstaticmethod(klass, "exists", lit_objfndirectory_exists);
        lit_class_bindstaticmethod(klass, "read", lit_objfndirectory_read);
    }
    state->allow_gc = wasallowed;
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
    return lit_value_makenumber(state->next_gc);
}

LitValue lit_objfngc_trigger(LitState* state, LitValue instance, size_t argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    state->allow_gc = true;
    int64_t collected = lit_collect_garbage(state);
    state->allow_gc = false;
    return lit_value_makenumber(collected);
}

void lit_open_gc_library(LitState* state)
{
    bool wasallowed = state->allow_gc;
    state->allow_gc = false;
    LitClass* klass = lit_class_make(state, "GC", state->object_class);
    lit_class_bindstaticgetter(klass, "memoryUsed", lit_objfngc_memoryused);
    lit_class_bindstaticgetter(klass, "nextRound", lit_objfngc_nextround);
    lit_class_bindstaticmethod(klass, "trigger", lit_objfngc_trigger);
    state->allow_gc = wasallowed;
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

void lit_open_math_library(LitState* state)
{
    LitClass* klass;
    bool wasallowed = state->allow_gc;
    state->allow_gc = false;
    {
        klass = lit_class_make(state, "Math", state->object_class);
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
    state->allow_gc = wasallowed;
}

void lit_uintlist_init(LitDynListUInt* array)
{
    lit_uintlist_reset(array);
}

void lit_uintlist_reset(LitDynListUInt* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_uintlist_destroy(LitDynListUInt* array)
{
    lit_sysmem_free(array->values);
    lit_uintlist_reset(array);
}

void lit_uintlist_push(LitDynListUInt* array, size_t value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (size_t*)lit_sysmem_realloc(array->values, sizeof(size_t) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_bytelist_init(LitDynListByte* array)
{
    lit_bytelist_reset(array);
}

void lit_bytelist_reset(LitDynListByte* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_bytelist_destroy(LitDynListByte* array)
{
    lit_sysmem_free(array->values);
    lit_bytelist_reset(array);
}

void lit_bytelist_push(LitDynListByte* array, uint8_t value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (uint8_t*)lit_sysmem_realloc(array->values, sizeof(uint8_t) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_vallist_init(LitDynListVal* array)
{
    lit_vallist_reset(array);
}

void lit_vallist_reset(LitDynListVal* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void lit_vallist_destroy(LitDynListVal* array)
{
    lit_sysmem_free(array->values);
    lit_vallist_reset(array);
}

void lit_vallist_push(LitDynListVal* array, LitValue value)
{
    if(array->capacity < array->count + 1)
    {
        size_t oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitValue*)lit_sysmem_realloc(array->values, sizeof(LitValue) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}

LitValue lit_vallist_get(LitDynListVal* list, size_t idx)
{
    return list->values[idx];
}

LitValue lit_vallist_set(LitDynListVal* list, size_t idx, LitValue val)
{
    list->values[idx] = val;
    return list->values[idx];
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

LitCallFrame* setup_call(LitState* state, LitFunction* callee, LitValue* arguments, uint8_t argc)
{
    int i;
    uint8_t ai;
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
        if(targetargcount == argc && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            LitArray* array = &lit_object_makevararray(state)->array;
            lit_state_pushroot(state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->values, argc - targetargcount + 1);
            size_t j = 0;
            for(ti = targetargcount - 1; ti < argc; ti++)
            {
                array->values.values[j++] = *(frame->slots + ti + 1);
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

LitResult execute_call(LitState* state, LitCallFrame* frame)
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

LitResult lit_state_callfunction(LitState* state, LitFunction* callee, LitValue* arguments, uint8_t argc)
{
    return execute_call(state, setup_call(state, callee, arguments, argc));
}

LitResult lit_state_callclosure(LitState* state, LitFuncClosure* callee, LitValue* arguments, uint8_t argc)
{
    LitCallFrame* frame = setup_call(state, callee->function, arguments, argc);
    if(frame == NULL)
    {
        RETURN_RUNTIME_ERROR()
    }
    frame->closure = callee;
    return execute_call(state, frame);
}

LitResult lit_state_callmethod(LitState* state, LitValue instance, LitValue callee, LitValue* arguments, uint8_t argc)
{
    int i;
    uint8_t ai;
    if(lit_value_isobject(callee))
    {
        if(lit_set_native_exit_jump())
        {
            RETURN_RUNTIME_ERROR()
        }
        LitObjType type = OBJECT_TYPE(callee);
        if(type == LIT_OBJ_FUNCSCRIPT)
        {
            return lit_state_callfunction(state, AS_FUNCTION(callee), arguments, argc);
        }
        else if(type == LIT_OBJ_FUNCCLOSURE)
        {
            return lit_state_callclosure(state, AS_CLOSURE(callee), arguments, argc);
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

        if(state->config.traceexecution)
        {
            lit_debug_traceprintvalue(state->config.desttrace, "<vm:slots>", fiber->framecount, argc, slot);
        }
        switch(type)
        {
            case LIT_OBJ_FUNCNATIVE:
            {
                /* for some reason, single line expression doesn't work */
                LitValue value = AS_NATIVE_FUNCTION(callee)->natfuncptr(state, lit_value_makenull(), argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_FUNCNATMETHOD:
            {
                LitFuncNative* method = AS_NATIVE_METHOD(callee);
                /* For some reason, single line expression doesn't work */
                LitValue value = method->natfuncptr(state, *slot, argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* inst = lit_object_makeinstance(state, klass);
                if(klass->init_method != NULL)
                {
                    lit_state_callmethod(state, *slot, lit_value_fromobject(klass->init_method), arguments, argc);
                }
                RETURN_OK(lit_value_fromobject(inst))
            }
            case LIT_OBJ_FUNCBOUNDMETHOD:
            {
                LitBoundMethod* boundmethod = AS_BOUND_METHOD(callee);
                LitValue method = boundmethod->method;
                if(IS_NATIVE_METHOD(method))
                {
                    /* For some reason, single line expression doesn't work */
                    LitValue value = AS_NATIVE_METHOD(method)->natfuncptr(state, boundmethod->receiver, argc, slot + 1);
                    RETURN_OK(value)
                }
                else
                {
                    *slot = boundmethod->receiver;
                    return lit_state_callfunction(state, AS_FUNCTION(method), arguments, argc);
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

LitResult lit_state_callvalue(LitState* state, LitValue callee, LitValue* arguments, uint8_t argc)
{
    return lit_state_callmethod(state, callee, callee, arguments, argc);
}

LitResult lit_state_findandcallmethod(LitState* state, LitValue callee, LitString* mthname, LitValue* arguments, uint8_t argc)
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
        if(lit_table_getentry(&klass->methods, mthname, &method))
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

LitString* lit_value_tostring(LitState* state, LitValue object, size_t indentation)
{
    size_t needed;
    LitValue* tmptr;
    LitIOStream pr;
    if(IS_STRING(object))
    {
        return AS_STRING(object);
    }
    else if(!lit_value_isobject(object))
    {
        if(lit_value_isnull(object))
        {
            return lit_string_copy(state, "null");
        }
        else if(lit_value_isnumber(object))
        {
            return AS_STRING(lit_string_numbertostring(state, lit_value_asnumber(object)));
        }
        else if(lit_value_isbool(object))
        {
            return lit_string_copy(state, lit_value_asbool(object) ? "true" : "false");
        }
    }
    else if(IS_REFERENCE(object))
    {
        LitValue* slot = AS_REFERENCE(object)->slot;
        if(slot == NULL)
        {
            return lit_string_copy(state, "null");
        }
        return lit_value_tostring(state, *slot, 0);
    }
    LitFiber* fiber = state->vmstate.fiber;
    if(lit_fiber_ensureframes(state, fiber))
    {
        return lit_string_copy(state, "null");
    }
    LitFunction* function = state->api_function;
    if(function == NULL)
    {
        function = state->api_function = lit_object_makefunction(state, fiber->module);
        function->chunk.haslineinfo = false;
        function->name = state->api_name;
        LitChunk* chunk = &function->chunk;
        chunk->compiledcodecount = 0;
        chunk->constantlist.count = 0;
        function->maxregisters = 3;
        int constant = lit_chunk_addconstant(state, chunk, lit_value_fromobject(lit_string_copy(state, "toString")));
        lit_chunk_push(chunk, LIT_FORM_ABC_INSTRUCTION(OP_INVOKE, 1, 2, constant), 1);
        lit_chunk_push(chunk, LIT_FORM_ABC_INSTRUCTION(OP_RETURN, 1, 0, 0), 1);
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
        return lit_string_copy(state, "null");
    }
    if(!IS_STRING(result.result))
    {
        return lit_string_copy(state, "invalid toString()");
    }
    return AS_STRING(result.result);
}

LitValue lit_state_callnew(LitState* state, const char* name, LitValue* args, size_t argc)
{
    LitValue value;
    if(!lit_table_getentry(&state->vmstate.globals->values, lit_string_copy(state, name), &value))
    {
        lit_vm_raisefatalerror(state, "failed to create instance of class %s: class not found", name);
        return lit_value_makenull();
    }
    LitClass* klass = AS_CLASS(value);
    if(klass->init_method == NULL)
    {
        return lit_value_fromobject(lit_object_makeinstance(state, klass));
    }
    return lit_state_callmethod(state, value, value, args, argc).result;
}

bool lit_value_iscallablefunction(LitValue value)
{
    if(lit_value_isobject(value))
    {
        LitObjType type = OBJECT_TYPE(value);
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
    printf("%p allocate %ld for %s\n", (void*)object, size, lit_tostring_typename(type));
#endif
    return object;
}

LitFunction* lit_object_makefunction(LitState* state, LitModule* module)
{
    LitFunction* function = (LitFunction*)lit_object_allocobject(state, sizeof(LitFunction), LIT_OBJ_FUNCSCRIPT);
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
    switch(OBJECT_TYPE(instance))
    {
        case LIT_OBJ_FUNCSCRIPT:
        {
            name = AS_FUNCTION(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCCLOSURE:
        {
            name = AS_CLOSURE(instance)->function->name;
            break;
        }
        case LIT_OBJ_CLSPROTOTYPE:
        {
            name = AS_CLOSURE_PROTOTYPE(instance)->function->name;
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = AS_FIELD(instance);
            if(field->getter != NULL)
            {
                return lit_function_getname(state, lit_value_fromobject(field->getter));
            }
            return lit_function_getname(state, lit_value_fromobject(field->setter));
        }
        case LIT_OBJ_FUNCNATIVE:
        {
            name = AS_NATIVE_FUNCTION(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCNATMETHOD:
        {
            name = AS_NATIVE_METHOD(instance)->name;
            break;
        }
        case LIT_OBJ_FUNCBOUNDMETHOD:
        {
            return lit_function_getname(state, AS_BOUND_METHOD(instance)->method);
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

LitFuncClosure* lit_object_makeclosure(LitState* state, LitFunction* function)
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

LitClosurePrototype* lit_object_makeclosureproto(LitState* state, LitFunction* function)
{
    LitClosurePrototype* closure = (LitClosurePrototype*)lit_object_allocobject(state, sizeof(LitClosurePrototype), LIT_OBJ_CLSPROTOTYPE);
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


LitFiber* lit_object_makefiber(LitState* state, LitModule* module, LitFunction* function)
{
    uint8_t i;
    /* Allocate in advance, just in case GC is triggered */
    uint8_t registersallocated = function == NULL ? 1 : (uint8_t)lit_closest_power_of_two(function->maxregisters);
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
    fiber->open_upvalues = NULL;
    fiber->abort = false;
    fiber->returnaddress = NULL;
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
    capacity = (size_t)lit_closest_power_of_two((int)needed);
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
        for(upvalue = fiber->open_upvalues; upvalue != NULL; upvalue = upvalue->next)
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
    klass->init_method = NULL;
    klass->super = NULL;
    lit_table_init(state, &klass->methods);
    lit_table_init(state, &klass->static_fields);
    return klass;
}

LitInstance* lit_object_makeinstance(LitState* state, LitClass* klass)
{
    LitInstance* instance = (LitInstance*)lit_object_allocobject(state, sizeof(LitInstance), LIT_OBJ_INSTANCE);
    instance->klass = klass;
    lit_table_init(state, &instance->fields);
    return instance;
}

LitBoundMethod* lit_object_makeboundmethod(LitState* state, LitValue receiver, LitValue method)
{
    LitBoundMethod* boundmethod = (LitBoundMethod*)lit_object_allocobject(state, sizeof(LitBoundMethod), LIT_OBJ_FUNCBOUNDMETHOD);
    boundmethod->receiver = receiver;
    boundmethod->method = method;
    return boundmethod;
}

LitArray* lit_array_make(LitState* state)
{
    LitArray* array = (LitArray*)lit_object_allocobject(state, sizeof(LitArray), LIT_OBJ_ARRAY);
    lit_vallist_init(&array->values);
    return array;
}

void lit_array_push(LitArray* array, LitValue val)
{
    lit_vallist_push(&array->values, val);
}

size_t lit_array_size(LitArray* array)
{
    return array->values.count;
}

size_t lit_array_count(LitArray* array)
{
    return array->values.count;
}

LitValue lit_array_get(LitArray* ary, size_t idx)
{
    return lit_vallist_get(&ary->values, idx);
}

LitValue lit_array_set(LitArray* ary, size_t idx, LitValue val)
{
    return lit_vallist_set(&ary->values, idx, val);
}

LitValue lit_array_removeat(LitArray* array, size_t index)
{
    size_t i;
    size_t count;
    LitValue value;
    LitDynListVal* vl;
    vl = &array->values;
    count = vl->count;
    if(index >= count)
    {
        return lit_value_makenull();
    }
    value = lit_vallist_get(vl, index);
    if(index == count - 1)
    {
        lit_vallist_set(vl, index, lit_value_makenull());
    }
    else
    {
        for(i = index; i < vl->count - 1; i++)
        {
            lit_vallist_set(vl, i, lit_vallist_get(vl, i + 1));
        }
        lit_vallist_set(vl, count - 1, lit_value_makenull());
    }
    vl->count--;
    return value;
}

LitVarargArray* lit_object_makevararray(LitState* state)
{
    LitVarargArray* array = (LitVarargArray*)lit_object_allocobject(state, sizeof(LitVarargArray), LIT_OBJ_VARARGARRAY);
    lit_vallist_init(&array->array.values);
    return array;
}

LitMap* lit_object_makemap(LitState* state, LitTable* fields)
{
    LitMap* map = (LitMap*)lit_object_allocobject(state, sizeof(LitMap), LIT_OBJ_MAP);
    lit_table_init(state, &map->values);
    if(fields != NULL)
    {
        lit_table_addall(fields, &map->values);
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
    return lit_table_set(&map->values, key, value);
}

bool lit_map_get(LitMap* map, LitString* key, LitValue* value)
{
    return lit_table_getentry(&map->values, key, value);
}

bool lit_map_delete(LitMap* map, LitString* key)
{
    return lit_table_delete(&map->values, key);
}

void lit_map_addall(LitMap* from, LitMap* to)
{
    int i;
    for(i = 0; i <= from->values.htcapacity; i++)
    {
        LitTabEntry* entry = &from->values.htentries[i];
        if(entry->key != NULL)
        {
            lit_table_set(&to->values, entry->key, entry->value);
        }
    }
}

LitValue lit_map_getfield(LitMap* map, const char* name)
{
    LitValue value;
    LitState* state;
    state = ((LitObject*)map)->pstate;
    if(!lit_table_getentry(&map->values, lit_string_copy(state, name), &value))
    {
        value = lit_value_makenull();
    }
    return value;
}

void lit_map_setfield(LitMap* map, const char* name, LitValue value)
{
    LitState* state;
    state = ((LitObject*)map)->pstate;
    lit_table_set(&map->values, lit_string_copy(state, name), value);
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
    userdata->cleanup_fn = NULL;
    return userdata;
}

void* lit_userdata_insertdata(LitState* state, LitValue instance, size_t typesz, LitCleanupFn cleanup)
{
    LitUserdata* userdata;
    userdata = lit_userdata_makeuserdata(state, typesz);
    userdata->cleanup_fn = cleanup;
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
    return AS_USERDATA(temp)->data;
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

void lit_state_defaulthandleerror(LitState* state, const char* message)
{
    (void)state;
    fflush(stdout);
    fprintf(stderr, "%s%s%s\n", COLOR_RED, message, COLOR_RESET);
    fflush(stderr);
}

LitState* lit_state_make()
{
    LitState* state = (LitState*)lit_sysmem_malloc(sizeof(LitState));
    state->class_class = NULL;
    state->object_class = NULL;
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
    state->next_gc = 256 * 1024;
    state->allow_gc = false;
    state->error_fn = lit_state_defaulthandleerror;
    state->had_error = false;
    state->roots = NULL;
    state->root_count = 0;
    state->root_capacity = 0;
    state->last_module = NULL;
    state->config.traceexecution = false;
    state->config.tracechunk = false;
    state->config.isreplmode = false;
    state->config.havedesttrace = false;
    state->streamstdout = lit_iostream_makeio(stdout, false);
    state->streamstdout->shouldflush = true;
    state->streamstderr = lit_iostream_makeio(stderr, false);
    state->config.desttrace = state->streamstderr;
    state->scanner = (LitScanner*)lit_sysmem_malloc(sizeof(LitScanner));
    state->parser = (LitParser*)lit_sysmem_malloc(sizeof(LitParser));
    lit_parser_init(state, (LitParser*)state->parser);
    state->emitter = (LitEmitter*)lit_sysmem_malloc(sizeof(LitEmitter));
    lit_emitter_init(state, state->emitter);
    lit_init_vm(state);
    lit_api_init(state);
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
    lit_sysmem_free(state->scanner);
    lit_parser_destroy(state->parser);
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

LitValue lit_state_peekroot(LitState* state, uint8_t distance)
{
    assert(state->root_count - distance + 1 > 0);
    return state->roots[state->root_count - distance - 1];
}

void lit_state_poproot(LitState* state)
{
    state->root_count--;
}

void lit_state_poproots(LitState* state, uint8_t amount)
{
    state->root_count -= amount;
}

LitClass* lit_state_getclassfor(LitState* state, LitValue value)
{
    if(lit_value_isobject(value))
    {
        switch(OBJECT_TYPE(value))
        {
            case LIT_OBJ_STRING:
                return state->string_class;
            case LIT_OBJ_USERDATA:
                return state->object_class;
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
                LitUpvalue* upvalue = AS_UPVALUE(value);
                if(upvalue->location == NULL)
                {
                    return lit_state_getclassfor(state, upvalue->closed);
                }
                return lit_state_getclassfor(state, *upvalue->location);
            }
            case LIT_OBJ_INSTANCE:
                return lit_value_asinstance(value)->klass;
            case LIT_OBJ_CLASS:
                return state->class_class;
            case LIT_OBJ_ARRAY:
            case LIT_OBJ_VARARGARRAY:
                return state->array_class;
            case LIT_OBJ_MAP:
                return state->map_class;
            case LIT_OBJ_RANGE:
                return state->range_class;
            case LIT_OBJ_REFERENCE:
            {
                LitValue* slot = AS_REFERENCE(value)->slot;
                if(slot != NULL)
                {
                    return lit_state_getclassfor(state, *slot);
                }
                return state->object_class;
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

void relstmts(LitState* state, LitDynListExpr* statements)
{
    size_t i;
    for(i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }
    lit_exprlist_destroy(statements);
}

LitResult lit_state_interpretsource(LitState* state, const char* modname, const char* code)
{
    return lit_state_interninterpretsource(state, lit_string_copylen(state, modname, strlen(modname)), code);
}

LitModule* lit_state_compilemodulesource(LitState* state, LitString* modname, const char* code)
{
    bool allowedgc = state->allow_gc;
    state->allow_gc = false;
    state->had_error = false;
    LitModule* module = NULL;
    /* this is a lbc format */
    if((code[1] << 8 | code[0]) == LIT_BYTECODE_MAGIC_NUMBER)
    {
        module = lit_load_module(state, code);
    }
    else
    {
        LitDynListExpr statements;
        lit_exprlist_init(&statements);
        if(lit_parser_parsesource(state->parser, modname->strbuf.data, code, &statements))
        {
            lit_ast_destroystmtlist(state, &statements);
            return NULL;
        }
        module = lit_emitter_emitmod(state->emitter, &statements, modname);
        lit_ast_destroystmtlist(state, &statements);
    }
    state->allow_gc = allowedgc;
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

char* lit_util_patchfilename(char* filename)
{
    int i;
    int namelength = strlen(filename);
    /* check, if our filename ends with .lit or lbc, and remove it */
    if(namelength > 4 && (memcmp(filename + namelength - 4, ".lit", 4) == 0 || memcmp(filename + namelength - 4, ".lbc", 4) == 0))
    {
        filename[namelength - 4] = '\0';
        namelength -= 4;
    }
    /* check if our filename starts with ./ and remove it (useless, and makes the module name be ..main) */
    if(namelength > 2 && memcmp(filename, "./", 2) == 0)
    {
        filename += 2;
        namelength -= 2;
    }
    for(i = 0; i < namelength; i++)
    {
        char c = filename[i];
        if(c == '/' || c == '\\')
        {
            filename[i] = '.';
        }
    }
    return filename;
}

char* lit_util_dupstring(const char* string)
{
    size_t length = strlen(string) + 1;
    char* newstring = (char*)lit_sysmem_malloc(length);
    memcpy(newstring, string, length);
    return newstring;
}

bool lit_state_compileandsavefile(LitState* state, const char* inputfile, const char* outputfile)
{
    LitModule* compiledmodule;
    {
        char* filename = lit_util_dupstring(inputfile);
        char* source = lit_read_file(filename);
        if(source == NULL)
        {
            lit_state_raiseerror(state, LIT_ERROR_COMPILEERROR, "failed to open file '%s'", filename);
            return false;
        }
        filename = lit_util_patchfilename(filename);
        LitString* modname = lit_string_copylen(state, filename, strlen(filename));
        LitModule* module = lit_state_compilemodulesource(state, modname, source);
        compiledmodule = module;
        lit_sysmem_free((void*)source);
        lit_sysmem_free((void*)filename);
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
    lit_write_uint16_t(hnd, LIT_BYTECODE_MAGIC_NUMBER);
    lit_write_uint8_t(hnd, LIT_BYTECODE_VERSION);
    lit_write_uint16_t(hnd, 1);
    {
        lit_save_module(compiledmodule, hnd);
    }
    lit_write_uint16_t(hnd, LIT_BYTECODE_END_NUMBER);
    fclose(hnd);
    return true;
}

char* lit_util_readsource(LitState* state, const char* filename)
{
    char* source = lit_read_file(filename);
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
    state->error_fn(state, pr.psbuf.data);
    lit_iostream_destroy(&pr);
    state->had_error = true;
}

const char* lit_tostring_typename(LitObjType t)
{
    switch(t)
    {
        case LIT_OBJ_STRING: return "string";
        case LIT_OBJ_FUNCSCRIPT: return "funcscript";
        case LIT_OBJ_FUNCNATIVE: return "funcnative";
        case LIT_OBJ_FUNCNATMETHOD: return "funcnatmethod";
        case LIT_OBJ_FIBER: return "fiber";
        case LIT_OBJ_MODULE: return "module";
        case LIT_OBJ_FUNCCLOSURE: return "funcclosure";
        case LIT_OBJ_CLSPROTOTYPE: return "clsprototype";
        case LIT_OBJ_UPVALUE: return "upvalue";
        case LIT_OBJ_CLASS: return "class";
        case LIT_OBJ_INSTANCE: return "instance";
        case LIT_OBJ_FUNCBOUNDMETHOD: return "funcboundmethod";
        case LIT_OBJ_ARRAY: return "array";
        case LIT_OBJ_VARARGARRAY: return "varargarray";
        case LIT_OBJ_MAP: return "map";
        case LIT_OBJ_USERDATA: return "userdata";
        case LIT_OBJ_RANGE: return "range";
        case LIT_OBJ_FIELD: return "field";
        case LIT_OBJ_REFERENCE: return "reference";
    }
    return "?unknown?";
}

#define lit_vmexec_pushgc(state, allow) \
    bool wasallowed = state->allow_gc;  \
    state->allow_gc = allow;

#define lit_vmexec_popgc(state) state->allow_gc = wasallowed;

void lit_vmexec_traceframe(LitState* state, LitFiber* fiber)
{
    (void)state;
    (void)fiber;
    if(state->config.traceexecution)
    {
#ifdef LIT_TRACE_STACK
        if(fiber == NULL)
        {
            return;
        }
        LitCallFrame* frame = &fiber->framevals[fiber->framecount - 1];
        lit_iostream_printf(state->config.desttrace, "== fiber %p f%i %s (expects %i, max %i, added %i, current %i, exits %i) ==\n", fiber, fiber->framecount - 1, frame->function->name->strbuf.data, frame->function->argcount, frame->function->maxregisters, frame->function->maxregisters + (int)(fiber->stack_top - fiber->stack), fiber->stack_capacity, frame->returnaddress == NULL);
#endif
    }
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
    state->vmstate.gray_stack = NULL;
    state->vmstate.gray_count = 0;
    state->vmstate.gray_capacity = 0;
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
    LitIOStream pr;
    LitValue error = lit_value_fromobject(errorstring);
    LitFiber* fiber = state->vmstate.fiber;
    while(fiber != NULL)
    {
        fiber->error = error;
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
    lit_iostream_makestackstring(&pr);
    int count = (int)fiber->framecount - 1;
    lit_iostream_printf(&pr, "%s%s\n", COLOR_RED, errorstring->strbuf.data);
    for(i = count; i >= 0; i--)
    {
        LitCallFrame* frame = &fiber->framevals[i];
        LitFunction* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->strbuf.data;
        if(chunk->haslineinfo)
        {
            lit_iostream_printf(&pr, "[line %d] in %s()\n", lit_chunk_getline(chunk, frame->ip - chunk->compiledcodechunk - 1), name);
        }
        else
        {
            lit_iostream_printf(&pr, "\tin %s()\n", name);
        }
    }
    lit_iostream_printf(&pr, "%s", COLOR_RESET);
    lit_state_raiseerror(state, LIT_ERROR_RUNTIMEERROR, pr.psbuf.data);
    lit_iostream_destroy(&pr);
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
    lit_native_exit_jump();
    return result;
}

bool lit_vmexec_callcallable(LitState* state, LitFunction* function, LitFuncClosure* closure, uint8_t argc, size_t calleeregister)
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
        if(targetargcount == argc && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            /* no need to repack the arguments */
        }
        else
        {
            LitArray* array = &lit_object_makevararray(state)->array;
            lit_state_pushroot(state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->values, argc - targetargcount + 1);
            size_t j = 0;
            for(i = targetargcount - 1; i < argc; i++)
            {
                array->values.values[j++] = *(frame->slots + i + 1);
            }
            *(frame->slots + targetargcount) = lit_value_fromobject(array);
            lit_state_poproot(state);
        }
    }
    return true;
}

bool lit_vmexec_actualcallvalue(LitState* state, size_t calleeregister, uint8_t argc, const LitValue alternatecallee)
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
        switch(OBJECT_TYPE(callee))
        {
            case LIT_OBJ_FUNCSCRIPT:
            {
                return lit_vmexec_callcallable(state, AS_FUNCTION(callee), NULL, argc, calleeregister);
            }
            case LIT_OBJ_FUNCCLOSURE:
            {
                LitFuncClosure* closure = AS_CLOSURE(callee);
                return lit_vmexec_callcallable(state, closure->function, closure, argc, calleeregister);
            }
            case LIT_OBJ_FUNCNATIVE:
            {
                LitValue value = AS_NATIVE_FUNCTION(callee)->natfuncptr(state, lit_value_makenull(), argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                return !state->vmstate.fiber->abort;
            }
            case LIT_OBJ_FUNCNATMETHOD:
            {
                lit_vmexec_pushgc(state, false);
                LitFuncNative* method = AS_NATIVE_METHOD(callee);
                LitFiber* fiber = state->vmstate.fiber;
                LitValue value = method->natfuncptr(state, *(frame->slots + calleeregister), argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                lit_vmexec_popgc(state);
                return !fiber->abort;
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* instance = lit_object_makeinstance(state, klass);
                frame->slots[calleeregister] = lit_value_fromobject(instance);
                if(klass->init_method != NULL)
                {
                    return lit_vmexec_actualcallvalue(state, calleeregister, argc, lit_value_fromobject(klass->init_method));
                }
                return true;
            }
            case LIT_OBJ_FUNCBOUNDMETHOD:
            {
                LitBoundMethod* boundmethod = AS_BOUND_METHOD(callee);
                LitValue method = boundmethod->method;
                if(IS_NATIVE_METHOD(method))
                {
                    lit_vmexec_pushgc(state, false);
                    LitValue value = AS_NATIVE_METHOD(method)->natfuncptr(state, boundmethod->receiver, argc, frame->slots + calleeregister + 1);
                    frame->slots[calleeregister] = value;
                    lit_vmexec_popgc(state);
                    return !state->vmstate.fiber->abort;
                }
                else
                {
                    frame->slots[calleeregister] = boundmethod->receiver;
                    return lit_vmexec_callcallable(state, AS_FUNCTION(method), NULL, argc, calleeregister);
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
        return lit_vm_raiseerror(state, "can only call functions and classes, got %s", lit_get_value_type(callee));
    }
    return true;
}

LitUpvalue* lit_vmexec_captureupvalue(LitState* state, LitValue* local)
{
    LitUpvalue* previousupvalue = NULL;
    LitUpvalue* upvalue = state->vmstate.fiber->open_upvalues;
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
        state->vmstate.fiber->open_upvalues = createdupvalue;
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
    while(fiber->open_upvalues != NULL && fiber->open_upvalues->location >= last)
    {
        LitUpvalue* upvalue = fiber->open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        fiber->open_upvalues = upvalue->next;
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

void lit_vmmac_readframe(LitState* state, LitFiber** destfiber)
{
    *destfiber = state->vmstate.fiber;
    state->vmstate.frame = &(*destfiber)->framevals[(*destfiber)->framecount - 1];
    state->vmstate.current_chunk = &state->vmstate.frame->function->chunk;
    state->vmstate.vmconstantvalues = state->vmstate.current_chunk->constantlist.values;
    state->vmstate.ip = state->vmstate.frame->ip;
    (*destfiber)->module = state->vmstate.frame->function->module;
    state->vmstate.vmregisteritems = state->vmstate.frame->slots;
    state->vmstate.vmprivatevalues = (*destfiber)->module->privatevalues;
    state->vmstate.upvalues = state->vmstate.frame->closure == NULL ? NULL : state->vmstate.frame->closure->upvalues;
}

#define lit_vmmac_writeframe() \
    state->vmstate.frame->ip = state->vmstate.ip;

#define lit_vmmac_returnerror() \
    lit_vmexec_popgc(state); \
    return lit_result_make(LIT_STATUS_RUNTIMEERROR, lit_value_makenull());

#define lit_vmmac_recoverstate() \
    lit_vmmac_writeframe(); \
    fiber = state->vmstate.fiber; \
    if(fiber == NULL) \
    { \
        return lit_result_make(LIT_STATUS_OK, lit_value_makenull()); \
    } \
    if(fiber->abort) \
    { \
        lit_vmmac_returnerror(); \
    } \
    lit_vmmac_readframe(state, &fiber); \
    lit_vmexec_traceframe(state, fiber);

#define lit_vmmac_fail(...) \
    if(lit_vm_raiseerror(state, __VA_ARGS__)) \
    { \
        lit_vmmac_recoverstate(); \
        lit_vmmac_dispatchnext(); \
    } \
    else \
    { \
        lit_vmmac_returnerror(); \
    }

#define lit_vmmac_callvalue(callee, reg, argc) \
    if(!lit_vmexec_actualcallvalue(state, reg, argc, callee)) \
    { \
        lit_vmmac_recoverstate(); \
    }


LitValue lit_vmmac_getrc(LitState* state, int64_t r)
{
    if(LIT_BIT_ISSET(r, LIT_BITFLAG_CONSTANT))
    {
        return state->vmstate.vmconstantvalues[r & 0xff];
    }
    return state->vmstate.vmregisteritems[r];
}

#define lit_vmmac_invokemethoddefault(reg, bv, m, argc) \
    lit_vmmac_writeframe(); \
    LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vmmac_fail("use of method '%s' on a null value", m) \
    } \
    LitString* mthname = lit_string_copy(state, m); \
    LitValue method; \
    if((lit_value_isinstance(bv) && (lit_table_getentry(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_getentry(&klass->methods, mthname, &method)) \
    { \
        lit_vmmac_callvalue(method, reg, argc) \
    } \
    else \
    { \
        lit_vmmac_fail("attempt to call method '%s', that is not defined in class %s", mthname->strbuf.data, klass->name->strbuf.data) \
    } \
    lit_vmmac_readframe(state, &fiber)

#define lit_vmmac_invokemethodandcontinue(reg, bv, m, argc) \
    lit_vmmac_writeframe() LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vmmac_fail("only instances and classes have methods"); \
    } \
    LitString* mthname = lit_string_copy(state, m); \
    LitValue method; \
    if((lit_value_isinstance(bv) && (lit_table_getentry(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_getentry(&klass->methods, mthname, &method)) \
    { \
        lit_vmmac_callvalue(method, reg, argc); \
        lit_vmmac_readframe(state, &fiber); \
        lit_vmmac_dispatchnext(); \
    }

#define BINARY_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction); \
    uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction); \
    uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction); \
    LitValue bv = lit_vmmac_getrc(state, b); \
    LitValue cv = lit_vmmac_getrc(state, c); \
    if(lit_value_isnumber(bv)) \
    { \
        if(!lit_value_isnumber(cv)) \
        { \
            lit_vmmac_fail("attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        state->vmstate.vmregisteritems[a] = type(lit_value_asnumber(bv) op lit_value_asnumber(cv)); \
    } \
    else \
    { \
        if(lit_value_isnull(bv)) \
        { \
            lit_vmmac_fail("attempt to use the operator %s on a null value", opstring); \
        } \
        state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b); \
        LitValue tmpb = state->vmstate.vmregisteritems[a + 1]; \
        state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c); \
        lit_vmmac_invokemethoddefault(a, state->vmstate.vmregisteritems[a], opstring, 1); \
        state->vmstate.vmregisteritems[a + 1] = tmpb; \
    }

#define COMPARISON_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction); \
    uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction); \
    uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction); \
    LitValue bv = lit_vmmac_getrc(state, b); \
    LitValue cv = lit_vmmac_getrc(state, c); \
    if(lit_value_isnumber(bv)) \
    { \
        if(!lit_value_isnumber(cv)) \
        { \
            lit_vmmac_fail("attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        state->vmstate.vmregisteritems[a] = type(lit_value_asnumber(bv) op lit_value_asnumber(cv)); \
    } \
    else if(lit_value_isnull(bv)) \
    { \
        lit_vmmac_fail("attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b); \
        LitValue tmpb = state->vmstate.vmregisteritems[a + 1]; \
        state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c); \
        lit_vmmac_invokemethoddefault(a, state->vmstate.vmregisteritems[a], opstring, 1); \
        state->vmstate.vmregisteritems[a + 1] = tmpb; \
    }

LitResult lit_state_execfiber(LitState* state, LitFiber* fiber)
{
    assert(fiber->framecount > 0);
    state->vmstate.fiber = fiber;

#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    static void* dispatchtable[] = {
        &&LABELNAME(OP_MOVE),
        &&LABELNAME(OP_LOAD_NULL),
        &&LABELNAME(OP_LOAD_BOOL),
        &&LABELNAME(OP_CLOSURE),
        &&LABELNAME(OP_ARRAY),
        &&LABELNAME(OP_OBJECT),
        &&LABELNAME(OP_RANGE),
        &&LABELNAME(OP_RETURN),
        &&LABELNAME(OP_ADD),
        &&LABELNAME(OP_SUBTRACT),
        &&LABELNAME(OP_MULTIPLY),
        &&LABELNAME(OP_DIVIDE),
        &&LABELNAME(OP_FLOOR_DIVIDE),
        &&LABELNAME(OP_MOD),
        &&LABELNAME(OP_POWER),
        &&LABELNAME(OP_LSHIFT),
        &&LABELNAME(OP_RSHIFT),
        &&LABELNAME(OP_BXOR),
        &&LABELNAME(OP_BAND),
        &&LABELNAME(OP_BOR),
        &&LABELNAME(OP_JUMP),
        &&LABELNAME(OP_TRUE_JUMP),
        &&LABELNAME(OP_FALSE_JUMP),
        &&LABELNAME(OP_NON_NULL_JUMP),
        &&LABELNAME(OP_NULL_JUMP),
        &&LABELNAME(OP_EQUAL),
        &&LABELNAME(OP_LESS),
        &&LABELNAME(OP_LESS_EQUAL),
        &&LABELNAME(OP_GREATER),
        &&LABELNAME(OP_GREATER_EQUAL),
        &&LABELNAME(OP_NEGATE),
        &&LABELNAME(OP_NOT),
        &&LABELNAME(OP_BNOT),
        &&LABELNAME(OP_SET_GLOBAL),
        &&LABELNAME(OP_GET_GLOBAL),
        &&LABELNAME(OP_SET_UPVALUE),
        &&LABELNAME(OP_GET_UPVALUE),
        &&LABELNAME(OP_SET_PRIVATE),
        &&LABELNAME(OP_GET_PRIVATE),
        &&LABELNAME(OP_CALL),
        &&LABELNAME(OP_CLOSE_UPVALUE),
        &&LABELNAME(OP_CLASS),
        &&LABELNAME(OP_STATIC_FIELD),
        &&LABELNAME(OP_METHOD),
        &&LABELNAME(OP_GET_FIELD),
        &&LABELNAME(OP_GET_SUPER_METHOD),
        &&LABELNAME(OP_SET_FIELD),
        &&LABELNAME(OP_IS),
        &&LABELNAME(OP_INVOKE),
        &&LABELNAME(OP_INVOKE_SUPER),
        &&LABELNAME(OP_SUBSCRIPT_GET),
        &&LABELNAME(OP_SUBSCRIPT_SET),
        &&LABELNAME(OP_PUSH_ARRAY_ELEMENT),
        &&LABELNAME(OP_PUSH_OBJECT_ELEMENT),
        &&LABELNAME(OP_REFERENCE_GLOBAL),
        &&LABELNAME(OP_REFERENCE_PRIVATE),
        &&LABELNAME(OP_REFERENCE_LOCAL),
        &&LABELNAME(OP_REFERENCE_UPVALUE),
        &&LABELNAME(OP_REFERENCE_FIELD),
        &&LABELNAME(OP_SET_REFERENCE),
    };
#endif
    LitCallFrame* previousframe;
    LitTable* globals;
    globals = &state->vmstate.globals->values;
    lit_vmexec_pushgc(state, true) fiber->abort = false;
    lit_vmmac_readframe(state, &fiber);
    state->vmstate.fiber = fiber;
    state->vmstate.vmregisteritems[0] = lit_value_fromobject(state->vmstate.frame->function);
    lit_vmexec_traceframe(state, fiber);
    if(state->config.traceexecution)
    {
        lit_iostream_printf(state->config.desttrace, "fiber start:\n");
    }

dispatch:
    state->vmstate.instruction = *state->vmstate.ip++;
    if(state->config.traceexecution)
    {
        previousframe = state->vmstate.frame;
        if(state->vmstate.frame->function->maxregisters > 0)
        {
            lit_debug_traceprintvalue(state->config.desttrace, "<vm:registers>", fiber->framecount, state->vmstate.frame->function->maxregisters, state->vmstate.vmregisteritems);
        }
        lit_debug_disasinstr(state->config.desttrace, state->vmstate.current_chunk, (size_t)(state->vmstate.ip - state->vmstate.current_chunk->compiledcodechunk - 1), NULL, state->vmstate.frame != previousframe);
        previousframe = state->vmstate.frame;
    }
#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    goto* dispatchtable[LIT_INSTRUCTION_OPCODE(state->vmstate.instruction)];
#else
    switch(LIT_INSTRUCTION_OPCODE(state->vmstate.instruction))
#endif
    {
        CASE_CODE(OP_MOVE)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            lit_vmmac_dispatchnext()
        }
        CASE_CODE(OP_LOAD_NULL)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_makenull();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LOAD_BOOL)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_makebool(LIT_INSTRUCTION_B(state->vmstate.instruction) != 0);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLOSURE)
        {
            size_t i;
            LitClosurePrototype* clsproto = AS_CLOSURE_PROTOTYPE(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]);
            LitFuncClosure* closure = lit_object_makeclosure(state, clsproto->function);
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(closure);
            for(i = 0; i < closure->function->upvaluecount; i++)
            {
                uint32_t index = clsproto->indexes[i];
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
        CASE_CODE(OP_ARRAY)
        {
            size_t sz;
            LitArray* array = lit_array_make(state);
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(array);
            sz = LIT_INSTRUCTION_B(state->vmstate.instruction);
            if(sz > array->values.capacity)
            {
                if(array->values.capacity > 0)
                {
                    sz = array->values.capacity - 1;
                }
                else
                {
                    sz = 0;
                }
            }
            lit_dynlistval_ensureactualsize(&array->values, sz);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_OBJECT)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makeinstance(state, state->object_class));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_RANGE)
        {
            LITREGUINT8 rxa = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LITREGUINT8 rxb = LIT_INSTRUCTION_B(state->vmstate.instruction);
            LITREGUINT8 rxc = LIT_INSTRUCTION_C(state->vmstate.instruction);
            state->vmstate.vmregisteritems[rxa] = lit_value_fromobject(lit_object_makerange(state, lit_value_asnumber(lit_vmmac_getrc(state, rxb)), lit_value_asnumber(lit_vmmac_getrc(state, rxc))));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_RETURN)
        {
            LitValue value = state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)];
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
                    if(state->config.traceexecution)
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
            lit_vmexec_traceframe(state, fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_ADD)
        {
            BINARY_INSTRUCTION(lit_value_makenumber, +, "+");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SUBTRACT)
        {
            BINARY_INSTRUCTION(lit_value_makenumber, -, "-");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_MULTIPLY)
        {
            BINARY_INSTRUCTION(lit_value_makenumber, *, "*");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_DIVIDE)
        {
            BINARY_INSTRUCTION(lit_value_makenumber, /, "/");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_FLOOR_DIVIDE)
        {
            uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction);
            uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, b);
            LitValue cv = lit_vmmac_getrc(state, c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[a] = lit_value_makenumber(floor(lit_value_asnumber(bv) / lit_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b);
                ;
                LitValue tmpb = state->vmstate.vmregisteritems[a + 1];
                state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c);
                ;
                lit_vmmac_invokemethoddefault(a, state->vmstate.vmregisteritems[a], "#", 1);
                state->vmstate.vmregisteritems[a + 1] = tmpb;
                ;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_MOD)
        {
            uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction);
            uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, b);
            LitValue cv = lit_vmmac_getrc(state, c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[a] = lit_value_makenumber(fmod(lit_value_asnumber(bv), lit_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b);
                ;
                LitValue tmpb = state->vmstate.vmregisteritems[a + 1];
                state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c);
                ;
                lit_vmmac_invokemethoddefault(a, state->vmstate.vmregisteritems[a], "%", 1);
                state->vmstate.vmregisteritems[a + 1] = tmpb;
                ;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_POWER)
        {
            uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction);
            uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, b);
            LitValue cv = lit_vmmac_getrc(state, c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                state->vmstate.vmregisteritems[a] = lit_value_makenumber(pow(lit_value_asnumber(bv), lit_value_asnumber(cv)));
            }
            else
            {
                state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b);
                ;
                LitValue tmpb = state->vmstate.vmregisteritems[a + 1];
                state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c);
                ;
                lit_vmmac_invokemethoddefault(a, state->vmstate.vmregisteritems[a], "**", 1);
                state->vmstate.vmregisteritems[a + 1] = tmpb;
                ;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LSHIFT)
        {
            LitValue res;
            int64_t ivbv;
            int64_t ivbc;
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail("operands of bitwise op %s must be two numbers, got %s and %s", "<<", lit_get_value_type(bv), lit_get_value_type(cv));
            }
            ivbv = (int64_t)lit_value_asnumber(bv);
            ivbc = (int64_t)lit_value_asnumber(cv);
            if((ivbv < 0) || (ivbc < 0))
            {
                res = lit_value_makenumber(0);
            }
            else
            {
                res = lit_value_makenumber(ivbv << ivbc);
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = res;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_RSHIFT)
        {
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail("operands of bitwise op %s must be two numbers, got %s and %s", ">>", lit_get_value_type(bv), lit_get_value_type(cv));
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = (lit_value_makenumber((int64_t)lit_value_asnumber(bv) >> (int64_t)lit_value_asnumber(cv)));
            ;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BXOR)
        {
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail("operands of bitwise op %s must be two numbers, got %s and %s", "^", lit_get_value_type(bv), lit_get_value_type(cv));
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = (lit_value_makenumber((int64_t)lit_value_asnumber(bv) ^ (int64_t)lit_value_asnumber(cv)));
            ;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BAND)
        {
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail("operands of bitwise op %s must be two numbers, got %s and %s", "&", lit_get_value_type(bv), lit_get_value_type(cv));
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = (lit_value_makenumber((int64_t)lit_value_asnumber(bv) & (int64_t)lit_value_asnumber(cv)));
            ;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BOR)
        {
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            LitValue cv = lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction));
            if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv))
            {
                lit_vmmac_fail("operands of bitwise op %s must be two numbers, got %s and %s", "|", lit_get_value_type(bv), lit_get_value_type(cv));
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = (lit_value_makenumber((int64_t)lit_value_asnumber(bv) | (int64_t)lit_value_asnumber(cv)));
            ;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_JUMP)
        {
            state->vmstate.ip += LIT_INSTRUCTION_SBX(state->vmstate.instruction);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_TRUE_JUMP)
        {
            if(!lit_is_falsey(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INSTRUCTION_BX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_FALSE_JUMP)
        {
            if(lit_is_falsey(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INSTRUCTION_BX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NON_NULL_JUMP)
        {
            if(!lit_value_isnull(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INSTRUCTION_BX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NULL_JUMP)
        {
            if(lit_value_isnull(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)]))
            {
                state->vmstate.ip += LIT_INSTRUCTION_BX(state->vmstate.instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_EQUAL)
        {
            LitValue ptmp;
            uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction);
            uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            uint16_t c = LIT_INSTRUCTION_C(state->vmstate.instruction);
            LitValue bv = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            if(lit_value_isinstance(bv))
            {
                state->vmstate.vmregisteritems[a] = lit_vmmac_getrc(state, b);
                ;
                LitValue tmpb = state->vmstate.vmregisteritems[a + 1];
                state->vmstate.vmregisteritems[a + 1] = lit_vmmac_getrc(state, c);
                ;
                lit_vmmac_invokemethodandcontinue(a, state->vmstate.vmregisteritems[a], "==", 1);
                state->vmstate.vmregisteritems[a + 1] = tmpb;
            }
            ptmp = lit_vmmac_getrc(state, c);
            state->vmstate.vmregisteritems[a] = lit_value_makebool(lit_value_compare(state, bv, ptmp));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LESS)
        {
            COMPARISON_INSTRUCTION(lit_value_makebool, <, "<");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LESS_EQUAL)
        {
            COMPARISON_INSTRUCTION(lit_value_makebool, <=, "<=");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GREATER)
        {
            COMPARISON_INSTRUCTION(lit_value_makebool, >, ">");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GREATER_EQUAL)
        {
            COMPARISON_INSTRUCTION(lit_value_makebool, >=, ">=");
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NEGATE)
        {
            LitValue value = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            if(!lit_value_isnumber(value))
            {
                lit_vmmac_fail("operand must be a number");
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_makenumber(-lit_value_asnumber(value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NOT)
        {
            uint8_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            LitValue value = lit_vmmac_getrc(state, b);
            if(lit_value_isinstance(value))
            {
                lit_vmmac_invokemethodandcontinue(b, value, "!", 0);
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_makebool(lit_is_falsey(value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BNOT)
        {
            LitValue value = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            if(!lit_value_isnumber(value))
            {
                lit_vmmac_fail("operand must be a number");
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_makenumber(~((int64_t)lit_value_asnumber(value)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_GLOBAL)
        {
            lit_table_set(globals, AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_A(state->vmstate.instruction)]), lit_vmmac_getrc(state, LIT_INSTRUCTION_BX(state->vmstate.instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_GLOBAL)
        {
            LitValue* reg = &state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)];
            if(!lit_table_getentry(globals, AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]), reg))
            {
                *reg = lit_value_makenull();
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_UPVALUE)
        {
            *state->vmstate.frame->closure->upvalues[LIT_INSTRUCTION_A(state->vmstate.instruction)]->location = lit_vmmac_getrc(state, LIT_INSTRUCTION_BX(state->vmstate.instruction));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_UPVALUE)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = *state->vmstate.frame->closure->upvalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]->location;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_PRIVATE)
        {
            uint8_t a = LIT_INSTRUCTION_A(state->vmstate.instruction);
            uint32_t b = LIT_INSTRUCTION_BX(state->vmstate.instruction);
            state->vmstate.vmprivatevalues[(uint16_t)b] = LIT_BIT_ISSET(b, 16) ? state->vmstate.vmconstantvalues[a] : state->vmstate.vmregisteritems[a];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_PRIVATE)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = state->vmstate.vmprivatevalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CALL)
        {
            lit_vmmac_writeframe();
            if(!lit_vmexec_actualcallvalue(state, LIT_INSTRUCTION_A(state->vmstate.instruction), LIT_INSTRUCTION_B(state->vmstate.instruction) - 1, lit_value_makenull()))
            {
                lit_vmmac_returnerror();
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLOSE_UPVALUE)
        {
            lit_vmexec_closeupvalues(state, &state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] - 1);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLASS)
        {
            LitString* name = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_A(state->vmstate.instruction)]);
            LitClass* klass = lit_object_makeclass(state, name);
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_C(state->vmstate.instruction)] = lit_value_fromobject(klass);
            lit_table_set(&state->vmstate.globals->values, name, lit_value_fromobject(klass));
            uint16_t b = LIT_INSTRUCTION_B(state->vmstate.instruction);
            if(b == 0)
            {
                klass->super = state->object_class;
                lit_table_addall(&klass->super->methods, &klass->methods);
                lit_table_addall(&klass->super->static_fields, &klass->static_fields);
            }
            else
            {
                LitValue super = state->vmstate.vmregisteritems[--b];
                if(!IS_CLASS(super))
                {
                    lit_vmmac_fail("superclass must be a class");
                }
                LitClass* superklass = AS_CLASS(super);
                klass->super = superklass;
                klass->init_method = superklass->init_method;
                lit_table_addall(&superklass->methods, &klass->methods);
                lit_table_addall(&klass->super->static_fields, &klass->static_fields);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_STATIC_FIELD)
        {
            lit_table_set(&AS_CLASS(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)])->static_fields, AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_B(state->vmstate.instruction)]), lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_METHOD)
        {
            LitClass* klass = AS_CLASS(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)]);
            LitString* name = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_B(state->vmstate.instruction)]);
            if((klass->init_method == NULL || (klass->super != NULL && klass->init_method == ((LitClass*)klass->super)->init_method)) && name->strbuf.length == 11 && memcmp(name->strbuf.data, "constructor", 11) == 0)
            {
                klass->init_method = lit_value_asobject(lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction)));
            }
            lit_table_set(&klass->methods, name, lit_vmmac_getrc(state, LIT_INSTRUCTION_C(state->vmstate.instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_FIELD)
        {
            LitValue object = state->vmstate.vmregisteritems[LIT_INSTRUCTION_B(state->vmstate.instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail("attempt to index a null value");
            }
            LitValue value;
            LitString* name = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]);
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            if(lit_value_isinstance(object))
            {
                LitInstance* instance = lit_value_asinstance(object);
                if(!lit_table_getentry(&instance->fields, name, &value))
                {
                    if(lit_table_getentry(&instance->klass->methods, name, &value))
                    {
                        if(IS_FIELD(value))
                        {
                            LitField* field = AS_FIELD(value);
                            if(field->getter == NULL)
                            {
                                lit_vmmac_fail("class %s does not have a getter for the field %s", instance->klass->name->strbuf.data, name->strbuf.data);
                            }
                            lit_vmmac_writeframe();
                            lit_vmmac_callvalue(lit_value_fromobject(AS_FIELD(value)->getter), resultreg, 0);
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
            else if(IS_CLASS(object))
            {
                LitClass* klass = AS_CLASS(object);
                if(lit_table_getentry(&klass->static_fields, name, &value))
                {
                    if(IS_NATIVE_METHOD(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                    else if(IS_FIELD(value))
                    {
                        LitField* field = AS_FIELD(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail("class %s does not have a getter for the field %s", klass->name->strbuf.data, name->strbuf.data);
                        }
                        lit_vmmac_writeframe();
                        lit_vmmac_callvalue(lit_value_fromobject(field->getter), resultreg, 0);
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
                LitClass* klass = lit_state_getclassfor(state, object);
                if(klass == NULL)
                {
                    lit_vmmac_fail("only instances and classes have fields");
                }
                if(lit_table_getentry(&klass->methods, name, &value))
                {
                    if(IS_FIELD(value))
                    {
                        LitField* field = AS_FIELD(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail("class %s does not have a getter for the field %s", klass->name->strbuf.data, name->strbuf.data);
                        }
                        lit_vmmac_writeframe();
                        lit_vmmac_callvalue(lit_value_fromobject(AS_FIELD(value)->getter), resultreg, 0);
                        lit_vmmac_readframe(state, &fiber);
                        lit_vmmac_dispatchnext();
                    }
                    else if(IS_NATIVE_METHOD(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                }
                else
                {
                    value = lit_value_makenull();
                }
            }
            state->vmstate.vmregisteritems[resultreg] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_SUPER_METHOD)
        {
            LitValue instance = state->vmstate.vmregisteritems[LIT_INSTRUCTION_B(state->vmstate.instruction)];
            LitClass* klass = AS_CLASS(instance);
            LitString* mthname = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]);
            LitValue value;
            if(lit_table_getentry(&klass->methods, mthname, &value) || lit_table_getentry(&klass->static_fields, mthname, &value))
            {
                value = lit_value_fromobject(lit_object_makeboundmethod(state, state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)], value));
            }
            else
            {
                value = lit_value_makenull();
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_FIELD)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[resultreg];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("attempt to index a null value");
            }
            LitValue value = state->vmstate.vmregisteritems[LIT_INSTRUCTION_C(state->vmstate.instruction)];
            LitString* fieldname = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_B(state->vmstate.instruction)]);
            if(IS_CLASS(instance))
            {
                LitClass* klass = AS_CLASS(instance);
                LitValue setter;
                if(lit_table_getentry(&klass->static_fields, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("class %s does not have a setter for the field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                if(lit_value_isnull(value))
                {
                    lit_table_delete(&klass->static_fields, fieldname);
                }
                else
                {
                    lit_table_set(&klass->static_fields, fieldname, value);
                }
            }
            else if(lit_value_isinstance(instance))
            {
                LitInstance* inst = lit_value_asinstance(instance);
                LitValue setter;
                if(lit_table_getentry(&inst->klass->methods, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("class %s does not have a setter for the field %s", inst->klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
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
                LitClass* klass = lit_state_getclassfor(state, instance);
                if(klass == NULL)
                {
                    lit_vmmac_fail("only instances and classes have fields");
                }
                LitValue setter;
                if(lit_table_getentry(&klass->methods, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("class %s does not have a setter for the field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
                    lit_vmmac_readframe(state, &fiber);
                    lit_vmmac_dispatchnext();
                }
                else
                {
                    lit_vmmac_fail("class %s does not contain field %s", klass->name->strbuf.data, fieldname->strbuf.data);
                }
            }
            state->vmstate.vmregisteritems[resultreg] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_IS)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = lit_vmmac_getrc(state, LIT_INSTRUCTION_B(state->vmstate.instruction));
            if(lit_value_isnull(instance))
            {
                state->vmstate.vmregisteritems[resultreg] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            LitClass* instanceklass = lit_state_getclassfor(state, instance);
            LitValue klass;
            if(!lit_table_getentry(globals, AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]), &klass))
            {
                state->vmstate.vmregisteritems[resultreg] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            if(instanceklass == NULL || !IS_CLASS(klass))
            {
                lit_vmmac_fail("operands must be an instance and a class");
            }
            LitClass* type = AS_CLASS(klass);
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
            state->vmstate.vmregisteritems[resultreg] = lit_value_makebool(found);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_INVOKE)
        {
            lit_vmmac_writeframe();
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[resultreg];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("attempt to index a null value");
            }
            LitClass* klass = IS_CLASS(instance) ? AS_CLASS(instance) : lit_state_getclassfor(state, instance);
            if(klass == NULL)
            {
                lit_vmmac_fail("only instances and classes have methods");
            }
            LitString* mthname = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]);
            int argc = LIT_INSTRUCTION_B(state->vmstate.instruction) - 1;
            LitValue method;
            if(lit_value_isinstance(instance) && (lit_table_getentry(&lit_value_asinstance(instance)->fields, mthname, &method)))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else if(IS_CLASS(instance) && lit_table_getentry(&klass->static_fields, mthname, &method))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else if(lit_table_getentry(&klass->methods, mthname, &method))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else
            {
                lit_vmmac_fail("attempt to call method '%s', that is not defined in class %s", mthname->strbuf.data, klass->name->strbuf.data);
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_INVOKE_SUPER)
        {
            size_t i;
            lit_vmmac_writeframe();
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = state->vmstate.vmregisteritems[resultreg + 1];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("attempt to index a null value");
            }
            LitClass* klass = AS_CLASS(instance);
            if(klass == NULL)
            {
                lit_vmmac_fail("only instances and classes have methods");
            }
            LitString* mthname = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]);
            int argc = LIT_INSTRUCTION_B(state->vmstate.instruction) - 1;
            LitValue method;
            if(lit_table_getentry(&klass->methods, mthname, &method) || lit_table_getentry(&klass->static_fields, mthname, &method))
            {
                for(i = resultreg + 1; i <= resultreg + (size_t)argc; i++)
                {
                    state->vmstate.vmregisteritems[i] = state->vmstate.vmregisteritems[i + 1];
                }
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else
            {
                lit_vmmac_fail("attempt to call method '%s', that is not defined in class %s", mthname->strbuf.data, klass->name->strbuf.data);
            }
            lit_vmmac_readframe(state, &fiber);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SUBSCRIPT_GET)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = lit_vmmac_getrc(state, resultreg);
            lit_vmmac_invokemethoddefault(resultreg, instance, "[]", 1);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SUBSCRIPT_SET)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(state->vmstate.instruction);
            LitValue instance = lit_vmmac_getrc(state, resultreg);
            lit_vmmac_invokemethoddefault(resultreg, instance, "[]", 2);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_PUSH_ARRAY_ELEMENT)
        {
            LitDynListVal* array = &lit_value_asarray(state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)])->values;
            lit_vallist_push(array, lit_vmmac_getrc(state, LIT_INSTRUCTION_BX(state->vmstate.instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_PUSH_OBJECT_ELEMENT)
        {
            LitValue operand = state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)];
            LitString* key = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_B(state->vmstate.instruction)]);
            LitValue value = state->vmstate.vmregisteritems[LIT_INSTRUCTION_C(state->vmstate.instruction)];
            if(lit_value_ismap(operand))
            {
                lit_table_set(&lit_value_asmap(operand)->values, key, value);
            }
            else if(lit_value_isinstance(operand))
            {
                lit_table_set(&lit_value_asinstance(operand)->fields, key, value);
            }
            else
            {
                lit_vmmac_fail("slotted an object or a map as the operand, got %s", lit_get_value_type(operand));
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_GLOBAL)
        {
            LitString* name = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]);
            LitValue* value;
            if(lit_table_getslot(&state->vmstate.globals->values, name, &value))
            {
                state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            }
            else
            {
                lit_vmmac_fail("attempt to reference a null value");
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_PRIVATE)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, &state->vmstate.vmprivatevalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_LOCAL)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, &state->vmstate.vmregisteritems[LIT_INSTRUCTION_B(state->vmstate.instruction)]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_UPVALUE)
        {
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, state->vmstate.upvalues[LIT_INSTRUCTION_BX(state->vmstate.instruction)]->location));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_FIELD)
        {
            LitValue object = state->vmstate.vmregisteritems[LIT_INSTRUCTION_B(state->vmstate.instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail("attempt to index a null value");
            }
            LitValue* value;
            LitString* name = AS_STRING(state->vmstate.vmconstantvalues[LIT_INSTRUCTION_C(state->vmstate.instruction)]);
            if(lit_value_isinstance(object))
            {
                if(!lit_table_getslot(&lit_value_asinstance(object)->fields, name, &value))
                {
                    lit_vmmac_fail("attempt to reference a null value");
                }
            }
            else
            {
                lit_vmmac_fail("can only reference fields of real instances");
            }
            state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_REFERENCE)
        {
            LitValue reference = state->vmstate.vmregisteritems[LIT_INSTRUCTION_A(state->vmstate.instruction)];
            if(!IS_REFERENCE(reference))
            {
                lit_vmmac_fail("provided value is not a reference");
            }
            *AS_REFERENCE(reference)->slot = state->vmstate.vmregisteritems[LIT_INSTRUCTION_B(state->vmstate.instruction)];
            lit_vmmac_dispatchnext();
        }
#if !defined(LIT_CONF_USECOMPUTEDGOTO) || (LIT_CONF_USECOMPUTEDGOTO == 0)
        default:
#endif
        {
            lit_vmmac_fail("unknown op %i", state->vmstate.instruction);
            lit_vmmac_returnerror();
        }
    }
}

void lit_native_exit_jump()
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
    g_replstate = state;
    signal(SIGINT, lit_cli_interupthandler);
#ifndef _WIN32
    signal(SIGTSTP, lit_cli_interupthandler);
#endif
    while(true)
    {
        line = nn_cli_getinput(lictx, "> ");
        fprintf(stderr, "line=<%p>\n", line);
        if(line == NULL)
        {
            break;
        }
        nn_cli_addhistoryline(lictx, line);
        result = lit_state_interpretsource(state, "repl", line);
        if(result.type == LIT_STATUS_OK)
        {
            printf("%s\n", COLOR_GREEN);
            value = result.result;
            value = state->vmstate.frame->slots[1];
            lit_value_printvalue(state->streamstdout, value, true);
            printf("%s\n", COLOR_RESET);
        }
        nn_cli_freeline(lictx, line);
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
    bool dump;
    bool wasusage;
    bool quitafterinit;
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
        {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
        {"trace", 't', OPTPARSE_NONE, "trace execution"},
        {"dest", 'T', OPTPARSE_REQUIRED, "when '-t' is specified, write trace output to file. defaults to stderr"},
        {"output", 'o', OPTPARSE_REQUIRED, "compile a script to bytecode"},
        {0, 0, (optargtype_t)0, NULL}
    };
    #if defined(LIT_PLATFORM_WINDOWS) || defined(_MSC_VER)
        _setmode(fileno(stdin), _O_BINARY);
        _setmode(fileno(stdout), _O_BINARY);
        _setmode(fileno(stderr), _O_BINARY);
    #endif
    wasusage = false;
    quitafterinit = false;
    dump = false;
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
            printf("%s: %s\n", argv[0], options.errmsg);
        }
        else if(co == 'h')
        {
            lit_cli_showusage(argv, longopts, false);
            wasusage = true;
        }
        else if(co == 'd')
        {
            dump = true;
        }
        else if(co == 'o')
        {
            bytecodefile = options.optarg;
        }
        else if(co == 'e')
        {
            source = options.optarg;
        }
        else if(co == 't')
        {
            state->config.traceexecution = true;
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
    if(wasusage || quitafterinit)
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
        if(dump)
        {
            LitModule* module = lit_state_compilemodulesource(state, lit_string_copy(state, modname), source);
            if(module == NULL)
            {
                goto endmain;
            }
            lit_debug_disasmodule(state->streamstdout, module, source);
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
        if(dump)
        {
            result = lit_state_dumpfile(state, state->streamstdout, filename).type;
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

