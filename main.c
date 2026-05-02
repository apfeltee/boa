
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
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <memory.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "allocator.h"


    #define LIT_VERSION_STRING "0.4"
    #define LIT_BYTECODE_VERSION 0

// #define TESTING

    #ifndef TESTING
        #ifndef RELEASE
            #define DEBUG
        #endif
    #endif

    #ifdef DEBUG
        // #define LIT_TRACE_EXECUTION
        //#define LIT_TRACE_CHUNK
    // #define LIT_TRACE_NULL_FILL
    // #define LIT_MINIMIZE_CONTAINERS
    // #define LIT_LOG_GC
    // #define LIT_LOG_ALLOCATION
    // #define LIT_LOG_MARKING
    // #define LIT_LOG_BLACKING
    // #define LIT_STRESS_TEST_GC
    #endif

    #ifdef TESTING
        // So that we can actually test the map contents with a single-line expression
        #define LIT_SINGLE_LINE_MAPS
        #define LIT_SINGLE_LINE_MAPS_ENABLED true

        // Make sure that we did not break anything
        #define LIT_STRESS_TEST_GC
    #else
        #define LIT_SINGLE_LINE_MAPS_ENABLED false
    #endif

    #define LIT_INTERPOLATION_NESTING_MAX 4
    #define LIT_REGISTERS_MAX 250// Can't be over 255

    #define LIT_GC_HEAP_GROW_FACTOR 2
    #define LIT_CALL_FRAMES_MAX (1024)
    #define LIT_INITIAL_CALL_FRAMES 1024
    #define LIT_CONTAINER_OUTPUT_MAX 10

    #if defined(__ANDROID__) || defined(_ANDROID_)
    #elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        #define LIT_OS_WINDOWS
    #endif

    #define LIT_USE_LIBREADLINE

    #if !defined(va_copy)
        #if defined(__GNUC__) || defined(__CLANG__)
            #define va_copy(d,s) __builtin_va_copy(d,s)
        #else
            #define va_copy(dest, src) memcpy(dest, src, sizeof(va_list))
        #endif
    #endif

    #if defined(__STRICT_ANSI__)
        void *memccpy(void *dest, const void *src, int c, size_t n);
        int vsnprintf(char *str, size_t size, const char *format, va_list ap);
    #endif

    #define STRBUF_MIN(x, y) ((x) < (y) ? (x) : (y))
    #define STRBUF_MAX(x, y) ((x) > (y) ? (x) : (y))

    #ifdef LIT_USE_LIBREADLINE
    #else
        #define LIT_REPL_INPUT_MAX 1024
    #endif

    #if !defined(LIT_DISABLE_COLOR) && !defined(LIT_ENABLE_COLOR) && !(defined(LIT_OS_WINDOWS))
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
        fprintf(stderr, "Unreachable code was reached at %s:%i\n", __FILE__, __LINE__); \
        assert(false);
    #define UINT8_COUNT UINT8_MAX + 1
    #define UINT16_COUNT UINT16_MAX + 1

    #define RETURN_RUNTIME_ERROR() return (LitResult){ INTERPRET_RUNTIME_ERROR, lit_value_makenull() };
    #define INTERPRET_RUNTIME_FAIL ((LitResult){ INTERPRET_INVALID, lit_value_makenull() })

    #define RETURN_OK(r) return (LitResult){ INTERPRET_OK, r };

    #define LIT_CHECK_NUMBER(id) lit_args_checknumber(vm, __FUNCTION__, args, argc, id)
    #define LIT_GET_NUMBER(id, def) lit_args_getnumber(vm, args, argc, id, def)

    #define LIT_CHECK_BOOL(id) lit_args_checkbool(vm, args, argc, id)

    #define LIT_CHECK_STRING(id) lit_args_checkstring(vm, args, argc, id)
    #define LIT_GET_STRING(id, def) lit_args_getstring(vm, args, argc, id, def)

    #define LIT_CHECK_OBJECT_STRING(id) lit_args_checkobjstring(vm, args, argc, id)

    #define LIT_ENSURE_ARGS(count)                                                           \
        if(argc != count)                                                               \
        {                                                                                    \
            lit_vm_raisefatalerror(vm, "Expected %i argument, got %i", count, argc); \
            return lit_value_makenull();                                                               \
        }


    #define LIT_INSERT_DATA(type, cleanup)                                                                                      \
        ({                                                                                                                      \
            LitUserdata* userdata = lit_object_makeuserdata(vm->state, sizeof(type));                                               \
            userdata->cleanup_fn = cleanup;                                                                                     \
            lit_table_set(vm->state, &lit_value_asinstance(instance)->fields, CONST_STRING(vm->state, "_data"), lit_value_fromobject(userdata)); \
            (type*)userdata->data;                                                                                              \
        })

    #define LIT_EXTRACT_DATA(type)                                                                    \
        ({                                                                                            \
            LitValue _d;                                                                              \
            if(!lit_table_get(&lit_value_asinstance(instance)->fields, CONST_STRING(vm->state, "_data"), &_d)) \
            {                                                                                         \
                lit_vm_raisefatalerror(vm, "Failed to extract userdata");                          \
            }                                                                                         \
            (type*)AS_USERDATA(_d)->data;                                                             \
        })

    // Do not change these, or old bytecode files will break!
    #define LIT_BYTECODE_MAGIC_NUMBER 6932
    #define LIT_BYTECODE_END_NUMBER 2942
    #define LIT_STRING_KEY 48

    #define lit_set_native_exit_jump() setjmp(g_vmglobaljumpbuf)

    #define OBJECT_TYPE(value) (lit_value_asobject(value)->type)





    #define ALLOCATE_OBJECT(state, type, objectType) (type*)lit_object_allocobject(state, sizeof(type), objectType)
    #define OBJECT_CONST_STRING(state, text) lit_value_fromobject(lit_string_copy((state), (text), strlen(text)))
    #define CONST_STRING(state, text) lit_string_copy((state), (text), strlen(text))

    #define TABLE_MAX_LOAD 0.75

    #define LIT_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

    #define LIT_FREE_ARRAY(state, type, pointer, oldcount) lit_reallocate(state, pointer, sizeof(type) * (oldcount), 0)

    #define LIT_FREE(state, type, pointer) lit_reallocate(state, pointer, sizeof(type), 0)




    #define SET_BIT(number, n) number |= 1UL << n;
    #define IS_BIT_SET(number, n) (((number >> n) & 1U) != 0)

    #define LIT_LONGEST_OP_NAME 13

    #define LIT_OPCODE_SIZE 0x3f
    #define LIT_A_ARG_SIZE 0xff
    #define LIT_B_ARG_SIZE 0x1ff
    #define LIT_C_ARG_SIZE 0x1ff
    #define LIT_BX_ARG_SIZE 0x3ffff// 18 bits max
    #define LIT_SBX_ARG_SIZE 0x1ffff// 17 bits max

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
    #define LIT_INSTRUCTION_A(instruction) ((instruction >> LIT_A_ARG_POSITION) & LIT_A_ARG_SIZE)
    #define LIT_INSTRUCTION_B(instruction) ((instruction >> LIT_B_ARG_POSITION) & LIT_B_ARG_SIZE)
    #define LIT_INSTRUCTION_C(instruction) ((instruction >> LIT_C_ARG_POSITION) & LIT_C_ARG_SIZE)
    #define LIT_INSTRUCTION_BX(instruction) ((instruction >> LIT_BX_ARG_POSITION) & LIT_BX_ARG_SIZE)
    #define LIT_INSTRUCTION_SBX(instruction) \
        (((instruction >> LIT_SBX_ARG_POSITION) & LIT_SBX_ARG_SIZE) * (((instruction >> LIT_SBX_FLAG_POSITION) & 0x1) == 1 ? -1 : 1))


    #define LIT_FORM_ABC_INSTRUCTION(opcode, a, b, c) \
        (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | (((b)&LIT_B_ARG_SIZE) << LIT_B_ARG_POSITION) | (((c)&LIT_C_ARG_SIZE) << LIT_C_ARG_POSITION))

    #define LIT_FORM_ABX_INSTRUCTION(opcode, a, bx) \
        (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | (((bx)&LIT_BX_ARG_SIZE) << LIT_BX_ARG_POSITION))

uint64_t LIT_FORM_ASBX_INSTRUCTION(int opcode, uint8_t a, int sbx)
{
    return (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | ((abs((int)(sbx)) & LIT_SBX_ARG_SIZE) << LIT_SBX_ARG_POSITION))
        | ((((sbx) < 0 ? 1 : 0) << LIT_SBX_FLAG_POSITION));
}


enum LitObjType
{
    LIT_OBJ_STRING,
    LIT_OBJ_FUNCTION,
    LIT_OBJ_NATIVEFUNCTION,
    LIT_OBJ_NATIVEPRIMITIVE,
    LIT_OBJ_NATIVEMETHOD,
    LIT_OBJ_PRIMITIVEMETHOD,
    LIT_OBJ_FIBER,
    LIT_OBJ_MODULE,
    LIT_OBJ_CLOSURE,
    LIT_OBJ_CLOSUREPROTOTYPE,
    LIT_OBJ_UPVALUE,
    LIT_OBJ_CLASS,
    LIT_OBJ_INSTANCE,
    LIT_OBJ_BOUNDMETHOD,
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
    TINVAL_NULL,
    TINVAL_BOOL,
    TINVAL_NUMBER,
    TINVAL_OBJECT,
};


enum LitFunctionType
{
    FUNCTION_REGULAR,
    FUNCTION_SCRIPT,
    FUNCTION_METHOD,
    FUNCTION_STATIC_METHOD,
    FUNCTION_CONSTRUCTOR
};

enum LitErrorType
{
    COMPILE_ERROR,
    RUNTIME_ERROR
};

enum LitStatusCode
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_INVALID
};

enum LitTokenType
{
    LTOKEN_NEW_LINE,

    // Single-character tokens.
    LTOKEN_LEFT_PAREN,
    LTOKEN_RIGHT_PAREN,
    LTOKEN_LEFT_BRACE,
    LTOKEN_RIGHT_BRACE,
    LTOKEN_LEFT_BRACKET,
    LTOKEN_RIGHT_BRACKET,
    LTOKEN_COMMA,
    LTOKEN_SEMICOLON,
    LTOKEN_COLON,

    // One or two character tokens.
    LTOKEN_BAR_EQUAL,
    LTOKEN_BAR,
    LTOKEN_BAR_BAR,
    LTOKEN_AMPERSAND_EQUAL,
    LTOKEN_AMPERSAND,
    LTOKEN_AMPERSAND_AMPERSAND,
    LTOKEN_BANG,
    LTOKEN_BANG_EQUAL,
    LTOKEN_EQUAL,
    LTOKEN_EQUAL_EQUAL,
    LTOKEN_GREATER,
    LTOKEN_GREATER_EQUAL,
    LTOKEN_GREATER_GREATER,
    LTOKEN_LESS,
    LTOKEN_LESS_EQUAL,
    LTOKEN_LESS_LESS,
    LTOKEN_PLUS,
    LTOKEN_PLUS_EQUAL,
    LTOKEN_PLUS_PLUS,
    LTOKEN_MINUS,
    LTOKEN_MINUS_EQUAL,
    LTOKEN_MINUS_MINUS,
    LTOKEN_STAR,
    LTOKEN_STAR_EQUAL,
    LTOKEN_STAR_STAR,
    LTOKEN_SLASH,
    LTOKEN_SLASH_EQUAL,
    LTOKEN_QUESTION,
    LTOKEN_QUESTION_QUESTION,
    LTOKEN_PERCENT,
    LTOKEN_PERCENT_EQUAL,
    LTOKEN_ARROW,
    LTOKEN_SMALL_ARROW,
    LTOKEN_TILDE,
    LTOKEN_CARET,
    LTOKEN_CARET_EQUAL,
    LTOKEN_DOT,
    LTOKEN_DOT_DOT,
    LTOKEN_DOT_DOT_DOT,
    LTOKEN_SHARP,
    LTOKEN_SHARP_EQUAL,

    // Literals.
    LTOKEN_IDENTIFIER,
    LTOKEN_STRING,
    LTOKEN_INTERPOLATION,
    LTOKEN_NUMBER,

    // Keywords.
    LTOKEN_CLASS,
    LTOKEN_ELSE,
    LTOKEN_FALSE,
    LTOKEN_FOR,
    LTOKEN_FUNCTION,
    LTOKEN_IF,
    LTOKEN_NULL,
    LTOKEN_RETURN,
    LTOKEN_SUPER,
    LTOKEN_THIS,
    LTOKEN_TRUE,
    LTOKEN_VAR,
    LTOKEN_WHILE,
    LTOKEN_CONTINUE,
    LTOKEN_BREAK,
    LTOKEN_NEW,
    LTOKEN_EXPORT,
    LTOKEN_IS,
    LTOKEN_STATIC,
    LTOKEN_OPERATOR,
    LTOKEN_IN,
    LTOKEN_CONST,
    LTOKEN_REF,

    LTOKEN_ERROR,
    LTOKEN_EOF
};


enum LitPrecedence
{
    PREC_NONE,
    PREC_ASSIGNMENT,// =
    PREC_OR,// ||
    PREC_AND,// &&
    PREC_BOR,// | ^
    PREC_BAND,// &
    PREC_SHIFT,// << >>
    PREC_EQUALITY,// == !=
    PREC_COMPARISON,// < > <= >=
    PREC_COMPOUND,// += -= *= /= ++ --
    PREC_TERM,// + -
    PREC_FACTOR,// * /
    PREC_IS,// is
    PREC_RANGE,// ..
    PREC_UNARY,// ! - ~
    PREC_NULL,// ??
    PREC_CALL,// . ()
    PREC_PRIMARY
};


enum LitExpressionType
{
    LIT_EXPR_LITERAL,
    LIT_EXPR_BINARY,
    LIT_EXPR_UNARY,
    LIT_EXPR_VAR,
    LIT_EXPR_ASSIGN,
    LIT_EXPR_CALL,
    LIT_EXPR_SET,
    LIT_EXPR_GET,
    LIT_EXPR_LAMBDA,
    LIT_EXPR_ARRAY,
    LIT_EXPR_OBJECT,
    LIT_EXPR_SUBSCRIPT,
    LIT_EXPR_THIS,
    LIT_EXPR_SUPER,
    LIT_EXPR_RANGE,
    LIT_EXPR_TERNARY,
    LIT_EXPR_INTERPOLATION,
    LIT_EXPR_REFERENCE,
    LIT_EXPR_EXPRESSION,
    LIT_EXPR_BLOCK,
    LIT_EXPR_IF,
    LIT_EXPR_WHILE,
    LIT_EXPR_FOR,
    LIT_EXPR_VARDECL,
    LIT_EXPR_CONTINUE,
    LIT_EXPR_BREAK,
    LIT_EXPR_FUNCTION,
    LIT_EXPR_RETURN,
    LIT_EXPR_METHOD,
    LIT_EXPR_CLASS,
    LIT_EXPR_FIELD
};

enum LitInstructionType
{
    LIT_INSTRUCTION_ABC,
    LIT_INSTRUCTION_ABX,
    LIT_INSTRUCTION_ASBX,
};

enum LitOpCode
{


OP_MOVE, // R(A) := RC(B)
OP_LOAD_NULL, // R(A) := null
OP_LOAD_BOOL, // R(A) := (bool) B
OP_CLOSURE, // R(A) := PrC[Bx]
OP_ARRAY, // R(A) := new Array(Bx)
OP_OBJECT, // R(A) = new Object()
OP_RANGE, // R(A) = new Range(RC(B), RC(C))

OP_RETURN, // return R(A)

OP_ADD, // R(A) := RC(B) + RC(C)
OP_SUBTRACT, // R(A) := RC(B) - RC(C)
OP_MULTIPLY, // R(A) := RC(B) * RC(C)
OP_DIVIDE, // R(A) := RC(B) / RC(C)
OP_FLOOR_DIVIDE, // R(A) := floor(RC(B) / RC(C))
OP_MOD, // R(A) := RC(B) % RC(C)
OP_POWER, // R(A) := pow(RC(B), RC(C))

OP_LSHIFT, // R(A) := RC(B) << RC(C)
OP_RSHIFT, // R(A) := RC(B) >> RC(C)
OP_BXOR, // R(A) := RC(B) ^ RC(C)
OP_BAND, // R(A) := RC(B) & RC(C)
OP_BOR, // R(A) := RC(B) | RC(C)

OP_JUMP, // PC += sBx
OP_TRUE_JUMP, // if (R(A)) PC += Bx
OP_FALSE_JUMP, // if (not R(A)) PC += Bx
OP_NON_NULL_JUMP, // if (R(A) != null) PC += Bx
OP_NULL_JUMP, // if (R(A) == null) PC += Bx

OP_EQUAL, // R(A) := RC(B) == RC(C)
OP_LESS, // R(A) := RC(B) < RC(C)
OP_LESS_EQUAL, // R(A) := RC(B) <= RC(C)
OP_GREATER, // R(A) := RC(B) > RC(C)
OP_GREATER_EQUAL, // R(A) := RC(B) >= RC(C)

OP_NEGATE, // R(A) := -RC(B)
OP_NOT, // R(A) := !RC(B)
OP_BNOT, // R(A) := ~RC(B)

OP_SET_GLOBAL, // G[C(A)] := RC(BX)
OP_GET_GLOBAL, // R(A) := G[C(Bx)]
OP_SET_UPVALUE, // U[A] := RC(Bx)
OP_GET_UPVALUE, // R(A) := U[Bx]
OP_SET_PRIVATE, // P[A] := RC(Bx)
OP_GET_PRIVATE, // R(A) := P[C(Bx)]

OP_CALL, // R(A) := R(A)(R(A + 1), ..., R(A + B - 1))
OP_CLOSE_UPVALUE, // close_upvalue(R(A))

OP_CLASS, // G[C(A)] = R[C] = new_class(C(A), C(B - 1))
OP_STATIC_FIELD, // R(A)[C(B)] = RC(C)
OP_METHOD, // R(A).Methods[C(B)] = RC(C)
OP_GET_FIELD, // R(A) = R(B)[C(C)]
OP_GET_SUPER_METHOD, // R(A) = R(B).super[C(C)]
OP_SET_FIELD, // R(A)[C(B)] = R(C)
OP_IS, // R(A) := RC(B) is G[C(C)]
OP_INVOKE, // R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1))
OP_INVOKE_SUPER, // R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1))
OP_SUBSCRIPT_GET, // R(A) := R(A)[RC(B)]
OP_SUBSCRIPT_SET, // R(A)[RC(B)] := R(C)

OP_PUSH_ARRAY_ELEMENT, // R(A)[R(A).count++] = RC(Bx)
OP_PUSH_OBJECT_ELEMENT, // R(A)[R(B)] = RC(C)

OP_REFERENCE_GLOBAL, // R(A) := ref G(C[Bx])
OP_REFERENCE_PRIVATE, // R(A) := ref P(Bx)
OP_REFERENCE_LOCAL, // R(A) := ref R(B)
OP_REFERENCE_UPVALUE, // R(A) := ref U(Bx)
OP_REFERENCE_FIELD, // R(A) = ref R(B)[C(C)]
OP_SET_REFERENCE, // ref R(A) := R(B)
};

enum NNPrMode
{
    NEON_PRMODE_UNDEFINED,
    NEON_PRMODE_STRING,
    NEON_PRMODE_FILE
};

typedef uint32_t LitUInt;
typedef enum NNPrMode NNPrMode;
typedef enum /**/LitObjType LitObjType;
typedef enum /**/LitFunctionType LitFunctionType;
typedef enum /**/LitErrorType LitErrorType;
typedef enum /**/LitStatusCode LitStatusCode;
typedef enum /**/LitTokenType LitTokenType;
typedef enum /**/LitPrecedence LitPrecedence;
typedef enum LitValType LitValType;
typedef enum LitExpressionType LitExpressionType;
typedef enum LitInstructionType LitInstructionType;
typedef enum LitOpCode LitOpCode;

typedef struct /**/LitScanner LitScanner;
typedef struct /**/LitVm LitVm;
typedef struct /**/LitParser LitParser;
typedef struct /**/LitEmitter LitEmitter;
typedef struct /**/LitState LitState;
typedef struct /**/LitResult LitResult;
typedef struct /**/LitObject LitObject;
typedef struct /**/LitMap LitMap;
typedef struct /**/LitString LitString;
typedef struct /**/LitModule LitModule;
typedef struct /**/LitFiber LitFiber;
typedef struct /**/LitUserdata LitUserdata;
typedef struct /**/LitExpression LitExpression;
typedef struct /**/LitUpvalue LitUpvalue;
typedef struct /**/LitClass LitClass;
typedef struct /**/LitLiteralExpression LitLiteralExpression;
typedef struct /**/LitBinaryExpression LitBinaryExpression;
typedef struct /**/LitUnaryExpression LitUnaryExpression;
typedef struct /**/LitVarExpression LitVarExpression;
typedef struct /**/LitAssignExpression LitAssignExpression;
typedef struct /**/LitCallExpression LitCallExpression;
typedef struct /**/LitGetExpression LitGetExpression;
typedef struct /**/LitSetExpression LitSetExpression;
typedef struct /**/LitParameter LitParameter;
typedef struct /**/LitDynListParam LitDynListParam;
typedef struct /**/LitArrayExpression LitArrayExpression;
typedef struct /**/LitObjectExpression LitObjectExpression;
typedef struct /**/LitSubscriptExpression LitSubscriptExpression;
typedef struct /**/LitThisExpression LitThisExpression;
typedef struct /**/LitSuperExpression LitSuperExpression;
typedef struct /**/LitRangeExpression LitRangeExpression;
typedef struct /**/LitTernaryExpression LitTernaryExpression;
typedef struct /**/LitInterpolationExpression LitInterpolationExpression;
typedef struct /**/LitReferenceExpression LitReferenceExpression;
typedef struct /**/LitExpressionStatement LitExpressionStatement;
typedef struct /**/LitBlockStatement LitBlockStatement;
typedef struct /**/LitVarStatement LitVarStatement;
typedef struct /**/LitIfStatement LitIfStatement;
typedef struct /**/LitWhileStatement LitWhileStatement;
typedef struct /**/LitForStatement LitForStatement;
typedef struct /**/LitContinueStatement LitContinueStatement;
typedef struct /**/LitBreakStatement LitBreakStatement;
typedef struct /**/LitFunctionStatement LitFunctionStatement;
typedef struct /**/LitReturnStatement LitReturnStatement;
typedef struct /**/LitMethodStatement LitMethodStatement;
typedef struct /**/LitClassStatement LitClassStatement;
typedef struct /**/LitFieldStatement LitFieldStatement;
typedef struct /**/LitPrivate LitPrivate;
typedef struct /**/LitDynListPriv LitDynListPriv;
typedef struct /**/LitLocal LitLocal;
typedef struct /**/LitDynListLoc LitDynListLoc;
typedef struct /**/LitCompilerUpvalue LitCompilerUpvalue;
typedef struct /**/LitCompiler LitCompiler;
typedef struct /**/LitEmitter LitEmitter;
typedef struct /**/LitParseRule LitParseRule;
typedef struct /**/LitParser LitParser;
typedef struct /**/LitEmulatedFile LitEmulatedFile;
typedef struct /**/LitScanner LitScanner;
typedef struct LitFileData LitFileData;
typedef struct LitResult LitResult;
typedef struct LitToken LitToken;
typedef struct LitDynListExpr LitDynListExpr;
typedef struct NNIOStream NNIOStream;
typedef struct NNStringBuffer NNStringBuffer;
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
typedef struct LitClosure LitClosure;
typedef struct LitClosurePrototype LitClosurePrototype;
typedef struct LitNativeFunction LitNativeFunction;
typedef struct LitNativePrimitive LitNativePrimitive;
typedef struct LitNativeMethod LitNativeMethod;
typedef struct LitPrimitiveMethod LitPrimitiveMethod;
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
typedef struct LitEvent LitEvent;
typedef struct LitEventSystem LitEventSystem;
typedef struct LitState LitState;
typedef struct LitVm LitVm;


typedef void (*LitDebugInstructionFn)(uint64_t instruction, const char* name);
typedef void (*LitErrorFn)(LitState* state, const char* message);
typedef void (*LitPrintFn)(LitState* state, const char* message);
typedef LitExpression* (*LitPrefixParseFn)(LitParser*, bool);
typedef LitExpression* (*LitInfixParseFn)(LitParser*, LitExpression*, bool);
typedef void (*LitCleanupFn)(LitState*, LitUserdata*, bool);
typedef LitValue (*LitMapIndexFn)(LitVm*, LitMap*, LitString*, LitValue*);
typedef bool (*LitPrimitiveMethodFn)(LitVm*, LitValue, LitUInt, LitValue*);
typedef LitValue (*LitNativeMethodFn)(LitVm*, LitValue, LitUInt, LitValue*);
typedef bool (*LitNativePrimitiveFn)(LitVm*, LitUInt, LitValue*);
typedef LitValue (*LitNativeFunctionFn)(LitVm*, LitUInt, LitValue*);


struct NNStringBuffer
{
    uint8_t isintern;
    /* capacity should be >= length+1 to allow for \0 */
    size_t capacity;
    size_t storedlength;
    char* longstrdata;
};

struct NNIOStream
{
    /* if file: should be closed when writer is destroyed? */
    uint8_t shouldclose;
    /* if file: should write operations be flushed via fflush()? */
    uint8_t shouldflush;
    /* if string: true if $strbuf was taken via nn_iostream_take() */
    uint8_t stringtaken;
    /* was this writer instance created on stack? */
    uint8_t fromstack;
    uint8_t shortenvalues;
    uint8_t jsonmode;
    size_t maxvallength;
    /* the mode that determines what writer actually does */
    NNPrMode wrmode;
    LitState* pstate;
    NNStringBuffer strbuf;
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
    LitUInt capacity;
    LitUInt count;
    LitUInt* values;
} ;

struct LitDynListByte
{
    LitUInt capacity;
    LitUInt count;
    uint8_t* values;
};

struct LitDynListVal
{
    LitUInt capacity;
    LitUInt count;
    LitValue* values;
};

struct LitChunk
{
    LitUInt compiledcodecount;
    LitUInt capacity;
    uint64_t* compiledcodechunk;
    bool haslineinfo;
    LitUInt linecount;
    LitUInt linecapacity;
    uint16_t* lines;
    LitDynListVal constants;
};

struct LitTabEntry
{
    LitString* key;
    LitValue value;
};

struct LitTable
{
    int count;
    int capacity;
    LitTabEntry* entries;
};

struct LitObject
{
    LitObjType type;
    LitObject* next;
    bool marked;
};

struct LitString
{
    LitObject innerobject;
    LitUInt length;
    uint32_t hash;
    char* chars;
};

struct LitFunction
{
    LitObject innerobject;
    LitChunk chunk;
    LitString* name;
    uint8_t argcount;
    LitUInt upvaluecount;
    uint8_t max_registers;
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

struct LitClosure
{
    LitObject innerobject;
    LitFunction* function;
    LitUpvalue** upvalues;
    LitUInt upvaluecount;
};

struct LitClosurePrototype
{
    LitObject innerobject;
    LitFunction* function;
    bool* local;
    uint8_t* indexes;
    LitUInt upvaluecount;
};

struct LitNativeFunction
{
    LitObject innerobject;
    LitNativeFunctionFn function;
    LitString* name;
};

struct LitNativePrimitive
{
    LitObject innerobject;
    LitNativePrimitiveFn function;
    LitString* name;
};

struct LitNativeMethod
{
    LitObject innerobject;
    LitNativeMethodFn method;
    LitString* name;
};

struct LitPrimitiveMethod
{
    LitObject innerobject;
    LitPrimitiveMethodFn method;
    LitString* name;
};

struct LitCallFrame
{
    LitFunction* function;
    LitClosure* closure;
    uint64_t* ip;
    LitValue* slots;
    LitValue* return_address;
    bool result_ignored;
    bool return_to_c;
};

struct LitMap
{
    LitObject innerobject;
    LitTable values;
    LitMapIndexFn index_fn;
};

struct LitModule
{
    LitObject innerobject;
    LitValue return_value;
    LitString* name;
    LitValue* privates;
    LitMap* private_names;
    LitUInt private_count;
    LitFunction* main_function;
    LitFiber* main_fiber;
    bool ran;
};

struct LitFiber
{
    LitObject innerobject;
    LitFiber* parent;
    LitValue* registers;
    LitUInt registers_allocated;
    LitCallFrame* framevals;
    LitUInt framecapacity;
    LitUInt framecount;
    LitUInt argcount;
    LitValue* return_address;
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

struct LitEvent
{
    uint64_t expire_time;
    LitValue callback;
    struct LitEvent* next;
    struct LitEvent* previous;
};

struct LitEventSystem
{
    LitEvent* events;
    LitEvent* last_event;
};

struct LitState
{
    struct
    {
        bool traceexecution;
        bool tracechunk;
    } config;
    int64_t bytes_allocated;
    int64_t next_gc;
    bool allow_gc;
    LitErrorFn error_fn;
    LitPrintFn print_fn;
    LitValue* roots;
    LitUInt root_count;
    LitUInt root_capacity;
    LitScanner* scanner;
    LitParser* parser;
    LitEmitter* emitter;
    LitVm* vm;
    LitEventSystem* event_system;
    bool had_error;
    LitFunction* api_function;
    LitString* api_name;
    // Mental note:
    // When adding another class here, DO NOT forget to mark it or it will be GC-ed
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

struct LitVm
{
    LitState* state;
    LitObject* objects;
    LitTable strings;
    LitMap* modules;
    LitMap* globals;
    LitFiber* fiber;
    // For garbage collection
    LitUInt gray_count;
    LitUInt gray_capacity;
    LitObject** gray_stack;
    // exec
    LitCallFrame* frame;
    LitChunk* current_chunk;
    LitValue* registers;
    LitValue* constants;
    LitValue* privates;
    LitUpvalue** upvalues;
    uint64_t* ip;
    uint64_t instruction;
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
    LitUInt length;
    LitUInt line;
    LitValue value;
};

/*
 * Expressions
 */
struct LitDynListExpr
{
    LitUInt capacity;
    LitUInt count;
    LitExpression** values;
};

struct LitPrivate
{
    bool initialized;
    bool constant;
};

struct LitDynListPriv
{
    LitUInt capacity;
    LitUInt count;
    LitPrivate* values;
};

struct LitLocal
{
    const char* name;
    LitUInt length;
    int depth;
    bool captured;
    bool constant;
    uint8_t reg;
};

struct LitDynListLoc 
{
    LitUInt capacity;
    LitUInt count;
    LitLocal* values;
};

struct LitCompilerUpvalue
{
    uint8_t index;
    bool isLocal;
};

struct LitCompiler
{
    LitDynListLoc locals;
    int scope_depth;
    LitFunction* function;
    LitFunctionType type;
    LitCompilerUpvalue upvalues[UINT8_COUNT];
    uint8_t registers_used;
    LitCompiler* enclosing;
    bool skip_return;
    LitUInt loop_depth;
};

struct LitEmitter
{
    LitState* state;
    LitChunk* chunk;
    LitCompiler* compiler;
    LitUInt last_line;
    LitUInt loop_start;
    LitDynListPriv privates;
    LitDynListUInt breaks;
    LitDynListUInt continues;
    LitModule* module;
    LitString* class_name;
    uint8_t class_register;
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
    LitState* state;
    bool had_error;
    bool panic_mode;
    LitToken previous;
    LitToken current;
    LitCompiler* compiler;
    uint8_t exprrootcnt;
    uint8_t stmtrootcnt;
};

struct LitEmulatedFile
{
    const char* source;
    LitUInt position;
};

struct LitScanner
{
    LitUInt line;
    const char* start;
    const char* current;
    const char* file_name;
    LitState* state;
    LitUInt braces[LIT_INTERPOLATION_NESTING_MAX];
    LitUInt num_braces;
    bool had_error;
};

struct LitExpression
{
    LitExpressionType type;
    LitUInt line;
};

struct LitLiteralExpression
{
    LitExpression expression;
    LitValue value;
};

struct LitBinaryExpression
{
    LitExpression expression;
    LitExpression* left;
    LitExpression* right;
    LitTokenType op;
    bool ignore_left;
};

struct LitUnaryExpression
{
    LitExpression expression;
    LitExpression* right;
    LitTokenType op;
};

struct LitVarExpression
{
    LitExpression expression;
    const char* name;
    LitUInt length;
};

struct LitAssignExpression
{
    LitExpression expression;
    LitExpression* to;
    LitExpression* value;
};

struct LitCallExpression
{
    LitExpression expression;
    LitExpression* callee;
    LitDynListExpr args;
    LitExpression* init;
};

struct LitGetExpression
{
    LitExpression expression;
    LitExpression* where;
    const char* name;
    LitUInt length;
    int jump;
    bool ignore_emit;
    bool ignore_result;
};

struct LitSetExpression
{
    LitExpression expression;
    LitExpression* where;
    const char* name;
    LitUInt length;
    LitExpression* value;
};

struct LitParameter
{
    const char* name;
    LitUInt length;
    uint8_t reg;
    LitExpression* default_value;
};

struct LitDynListParam
{
    LitUInt capacity;
    LitUInt count;
    LitParameter* values;
};

struct LitArrayExpression
{
    LitExpression expression;
    LitDynListExpr values;
};

struct LitObjectExpression
{
    LitExpression expression;
    LitDynListVal keys;
    LitDynListExpr values;
};

struct LitSubscriptExpression
{
    LitExpression expression;
    LitExpression* array;
    LitExpression* index;
};

struct LitThisExpression
{
    LitExpression expression;
};

struct LitSuperExpression
{
    LitExpression expression;
    LitString* method;
    bool ignore_emit;
    bool ignore_result;
};

struct LitRangeExpression
{
    LitExpression expression;
    LitExpression* from;
    LitExpression* to;
};

struct LitTernaryExpression
{
    LitExpression statement;
    LitExpression* condition;
    LitExpression* if_branch;
    LitExpression* else_branch;
};

struct LitInterpolationExpression
{
    LitExpression expression;
    LitDynListExpr expressions;
};

struct LitReferenceExpression
{
    LitExpression expression;
    LitExpression* to;
};

struct LitExpressionStatement
{
    LitExpression statement;
    LitExpression* expression;
};

struct LitBlockStatement
{
    LitExpression statement;
    LitDynListExpr statements;
};

struct LitVarStatement
{
    LitExpression statement;
    const char* name;
    LitUInt length;
    bool constant;
    LitExpression* init;
};

struct LitIfStatement
{
    LitExpression statement;
    LitExpression* condition;
    LitExpression* if_branch;
    LitExpression* else_branch;
    LitDynListExpr* elseif_conditions;
    LitDynListExpr* elseif_branches;
};

struct LitWhileStatement
{
    LitExpression statement;
    LitExpression* condition;
    LitExpression* body;
};

struct LitForStatement
{
    LitExpression statement;
    LitExpression* init;
    LitExpression* var;
    LitExpression* condition;
    LitExpression* increment;
    LitExpression* body;
    bool c_style;
};

struct LitContinueStatement
{
    LitExpression statement;
};

struct LitBreakStatement
{
    LitExpression statement;
};

struct LitFunctionStatement
{
    LitExpression statement;
    const char* name;
    LitUInt length;
    LitDynListParam parameters;
    LitExpression* body;
    bool exported;
};

struct LitReturnStatement
{
    LitExpression statement;
    LitExpression* expression;
};

struct LitMethodStatement
{
    LitExpression statement;
    LitString* name;
    LitDynListParam parameters;
    LitExpression* body;
    bool is_static;
};

struct LitClassStatement
{
    LitExpression statement;
    LitString* name;
    LitString* parent;
    LitDynListExpr fields;
};

struct LitFieldStatement
{
    LitExpression statement;
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

bool lit_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool lit_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

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

void* lit_reallocate(LitState* state, void* pointer, size_t oldsize, size_t newsize)
{
    state->bytes_allocated += (int64_t)newsize - (int64_t)oldsize;
    if(newsize > oldsize)
    {
#ifdef LIT_STRESS_TEST_GC
        lit_collect_garbage(state->vm);
#endif
        if(state->bytes_allocated > state->next_gc)
        {
            lit_collect_garbage(state->vm);
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
        lit_state_raiseerror(state, RUNTIME_ERROR, "Fatal error:\nOut of memory\nProgram terminated");
        exit(111);
    }
    return ptr;
}

void lit_free_object(LitState* state, LitObject* object)
{
#ifdef LIT_LOG_ALLOCATION
    printf("(%s) %p free %s\n", lit_tostring_typename(object->type), (void*)object, lit_tostring_typename(object->type));
#endif
    switch(object->type)
    {
        case LIT_OBJ_STRING:
        {
            LitString* string = (LitString*)object;
            lit_sysmem_free(string->chars);
            LIT_FREE(state, LitString, object);
            break;
        }
        case LIT_OBJ_FUNCTION:
        {
            LitFunction* function = (LitFunction*)object;
            lit_chunk_destroy(state, &function->chunk);
            LIT_FREE(state, LitFunction, object);
            break;
        }
        case LIT_OBJ_NATIVEFUNCTION:
        {
            LIT_FREE(state, LitNativeFunction, object);
            break;
        }
        case LIT_OBJ_NATIVEPRIMITIVE:
        {
            LIT_FREE(state, LitNativePrimitive, object);
            break;
        }
        case LIT_OBJ_NATIVEMETHOD:
        {
            LIT_FREE(state, LitNativeMethod, object);
            break;
        }
        case LIT_OBJ_PRIMITIVEMETHOD:
        {
            LIT_FREE(state, LitPrimitiveMethod, object);
            break;
        }
        case LIT_OBJ_FIBER:
        {
            LitFiber* fiber = (LitFiber*)object;
            lit_sysmem_free(fiber->framevals);
            lit_sysmem_free(fiber->registers);
            LIT_FREE(state, LitFiber, object);
            break;
        }
        case LIT_OBJ_MODULE:
        {
            LitModule* module = (LitModule*)object;
            LIT_FREE_ARRAY(state, LitValue, module->privates, module->private_count);
            LIT_FREE(state, LitModule, object);
            break;
        }
        case LIT_OBJ_CLOSURE:
        {
            LitClosure* closure = (LitClosure*)object;
            LIT_FREE_ARRAY(state, LitUpvalue*, closure->upvalues, closure->upvaluecount);
            LIT_FREE(state, LitClosure, object);
            break;
        }
        case LIT_OBJ_CLOSUREPROTOTYPE:
        {
            LitClosurePrototype* clsproto = (LitClosurePrototype*)object;
            LIT_FREE_ARRAY(state, uint8_t, clsproto->indexes, clsproto->upvaluecount);
            LIT_FREE_ARRAY(state, bool, clsproto->local, clsproto->upvaluecount);
            LIT_FREE(state, LitClosurePrototype, object);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            LIT_FREE(state, LitUpvalue, object);
            break;
        }
        case LIT_OBJ_CLASS:
        {
            LitClass* klass = (LitClass*)object;
            lit_free_table(state, &klass->methods);
            lit_free_table(state, &klass->static_fields);
            LIT_FREE(state, LitClass, object);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            lit_free_table(state, &((LitInstance*)object)->fields);
            LIT_FREE(state, LitInstance, object);
            break;
        }
        case LIT_OBJ_BOUNDMETHOD:
        {
            LIT_FREE(state, LitBoundMethod, object);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            lit_vallist_destroy(state, &((LitArray*)object)->values);
            LIT_FREE(state, LitArray, object);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            lit_vallist_destroy(state, &((LitVarargArray*)object)->array.values);
            LIT_FREE(state, LitVarargArray, object);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_free_table(state, &((LitMap*)object)->values);
            LIT_FREE(state, LitMap, object);
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
            LIT_FREE(state, LitUserdata, object);
            break;
        }
        case LIT_OBJ_RANGE:
        {
            LIT_FREE(state, LitRange, object);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LIT_FREE(state, LitField, object);
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            LIT_FREE(state, LitReference, object);
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
    lit_sysmem_free(state->vm->gray_stack);
    state->vm->gray_capacity = 0;
}

void lit_mark_object(LitVm* vm, LitObject* object)
{
    if(object == NULL || object->marked)
    {
        return;
    }
    object->marked = true;
#ifdef LIT_LOG_MARKING
    printf("%p mark ", (void*)object);
    lit_value_printvalue(lit_value_fromobject(object));
    printf("\n");
#endif
    if(vm->gray_capacity < vm->gray_count + 1)
    {
        vm->gray_capacity = LIT_GROW_CAPACITY(vm->gray_capacity);
        vm->gray_stack = (LitObject**)lit_sysmem_realloc(vm->gray_stack, sizeof(LitObject*) * vm->gray_capacity);
    }
    vm->gray_stack[vm->gray_count++] = object;
}

void lit_mark_value(LitVm* vm, LitValue value)
{
    if(lit_value_isobject(value))
    {
        lit_mark_object(vm, lit_value_asobject(value));
    }
}

void mark_roots(LitVm* vm)
{
    LitState* state = vm->state;
    for(LitUInt i = 0; i < state->root_count; i++)
    {
        lit_mark_value(vm, state->roots[i]);
    }
    lit_mark_object(vm, (LitObject*)vm->fiber);
    lit_mark_object(vm, (LitObject*)state->class_class);
    lit_mark_object(vm, (LitObject*)state->object_class);
    lit_mark_object(vm, (LitObject*)state->number_class);
    lit_mark_object(vm, (LitObject*)state->string_class);
    lit_mark_object(vm, (LitObject*)state->bool_class);
    lit_mark_object(vm, (LitObject*)state->function_class);
    lit_mark_object(vm, (LitObject*)state->fiber_class);
    lit_mark_object(vm, (LitObject*)state->module_class);
    lit_mark_object(vm, (LitObject*)state->array_class);
    lit_mark_object(vm, (LitObject*)state->map_class);
    lit_mark_object(vm, (LitObject*)state->range_class);
    lit_mark_object(vm, (LitObject*)state->api_name);
    lit_mark_object(vm, (LitObject*)state->api_function);
    lit_mark_table(vm, &vm->modules->values);
    lit_mark_table(vm, &vm->globals->values);
    LitEvent* event = state->event_system->events;
    while(event != NULL)
    {
        lit_mark_value(vm, event->callback);
        event = event->next;
    }
}

void mark_array(LitVm* vm, LitDynListVal* array)
{
    for(LitUInt i = 0; i < array->count; i++)
    {
        lit_mark_value(vm, array->values[i]);
    }
}

void blacken_object(LitVm* vm, LitObject* object)
{
#ifdef LIT_LOG_BLACKING
    printf("%p blacken ", (void*)object);
    lit_value_printvalue(lit_value_fromobject(object));
    printf("\n");
#endif
    switch(object->type)
    {
        case LIT_OBJ_NATIVEFUNCTION:
        case LIT_OBJ_NATIVEPRIMITIVE:
        case LIT_OBJ_NATIVEMETHOD:
        case LIT_OBJ_PRIMITIVEMETHOD:
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
                data->cleanup_fn(vm->state, data, true);
            }
            break;
        }
        case LIT_OBJ_FUNCTION:
        {
            LitFunction* function = (LitFunction*)object;
            lit_mark_object(vm, (LitObject*)function->name);
            mark_array(vm, &function->chunk.constants);
            break;
        }
        case LIT_OBJ_FIBER:
        {
            LitFiber* fiber = (LitFiber*)object;
            for(LitUInt i = 0; i < fiber->registers_allocated; i++)
            {
                lit_mark_value(vm, fiber->registers[i]);
            }
            for(LitUInt i = 0; i < fiber->framecount; i++)
            {
                LitCallFrame* frame = &fiber->framevals[i];
                if(frame->closure != NULL)
                {
                    lit_mark_object(vm, (LitObject*)frame->closure);
                }
                else
                {
                    lit_mark_object(vm, (LitObject*)frame->function);
                }
            }
            for(LitUpvalue* upvalue = fiber->open_upvalues; upvalue != NULL; upvalue = upvalue->next)
            {
                lit_mark_object(vm, (LitObject*)upvalue);
            }
            lit_mark_value(vm, fiber->error);
            lit_mark_object(vm, (LitObject*)fiber->module);
            lit_mark_object(vm, (LitObject*)fiber->parent);
            break;
        }
        case LIT_OBJ_MODULE:
        {
            LitModule* module = (LitModule*)object;
            lit_mark_value(vm, module->return_value);
            lit_mark_object(vm, (LitObject*)module->name);
            lit_mark_object(vm, (LitObject*)module->main_function);
            lit_mark_object(vm, (LitObject*)module->main_fiber);
            lit_mark_object(vm, (LitObject*)module->private_names);
            for(LitUInt i = 0; i < module->private_count; i++)
            {
                lit_mark_value(vm, module->privates[i]);
            }
            break;
        }
        case LIT_OBJ_CLOSURE:
        {
            LitClosure* closure = (LitClosure*)object;
            lit_mark_object(vm, (LitObject*)closure->function);
            // Check for NULL is needed for a really specific gc-case
            if(closure->upvalues != NULL)
            {
                for(LitUInt i = 0; i < closure->upvaluecount; i++)
                {
                    lit_mark_object(vm, (LitObject*)closure->upvalues[i]);
                }
            }
            break;
        }
        case LIT_OBJ_CLOSUREPROTOTYPE:
        {
            lit_mark_object(vm, (LitObject*)((LitClosure*)object)->function);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            lit_mark_value(vm, ((LitUpvalue*)object)->closed);
            break;
        }
        case LIT_OBJ_CLASS:
        {
            LitClass* klass = (LitClass*)object;
            lit_mark_object(vm, (LitObject*)klass->name);
            lit_mark_object(vm, (LitObject*)klass->super);
            lit_mark_table(vm, &klass->methods);
            lit_mark_table(vm, &klass->static_fields);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            LitInstance* instance = (LitInstance*)object;
            lit_mark_object(vm, (LitObject*)instance->klass);
            lit_mark_table(vm, &instance->fields);
            break;
        }
        case LIT_OBJ_BOUNDMETHOD:
        {
            LitBoundMethod* boundmethod = (LitBoundMethod*)object;
            lit_mark_value(vm, boundmethod->receiver);
            lit_mark_value(vm, boundmethod->method);
            break;
        }
        case LIT_OBJ_ARRAY:
        {
            mark_array(vm, &((LitArray*)object)->values);
            break;
        }
        case LIT_OBJ_VARARGARRAY:
        {
            mark_array(vm, &((LitVarargArray*)object)->array.values);
            break;
        }
        case LIT_OBJ_MAP:
        {
            lit_mark_table(vm, &((LitMap*)object)->values);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = (LitField*)object;
            lit_mark_object(vm, (LitObject*)field->getter);
            lit_mark_object(vm, (LitObject*)field->setter);
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            lit_mark_value(vm, *((LitReference*)object)->slot);
            break;
        }
        default:
        {
            lit_vm_raisefatalerror(vm, "Unknown object with type %i", object->type);
            break;
        }
    }
}

void trace_references(LitVm* vm)
{
    while(vm->gray_count > 0)
    {
        LitObject* object = vm->gray_stack[--vm->gray_count];
        blacken_object(vm, object);
    }
}

void sweep(LitVm* vm)
{
    LitObject* previous = NULL;
    LitObject* object = vm->objects;
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
                vm->objects = object;
            }
            lit_free_object(vm->state, unreached);
        }
    }
}

uint64_t lit_collect_garbage(LitVm* vm)
{
    if(!vm->state->allow_gc)
    {
        return 0;
    }
    vm->state->allow_gc = false;
    uint64_t before = vm->state->bytes_allocated;
#ifdef LIT_LOG_GC
    printf("-- gc begin\n");
    clock_t t = clock();
#endif
    mark_roots(vm);
    trace_references(vm);
    lit_table_remove_white(&vm->strings);
    sweep(vm);
    vm->state->next_gc = vm->state->bytes_allocated * LIT_GC_HEAP_GROW_FACTOR;
    vm->state->allow_gc = true;
    uint64_t collected = before - vm->state->bytes_allocated;
#ifdef LIT_LOG_GC
    printf("-- gc end. Collected %imb (%ib) in %gms\n", ((int)((collected / 1024.0 + 0.5) / 10)) * 10, collected, (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
#endif
    return collected;
}

// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
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


int lit_decode_num_bytes(uint8_t byte)
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

int lit_ustring_length(LitString* string)
{
    int length = 0;
    for(uint32_t i = 0; i < string->length;)
    {
        i += lit_decode_num_bytes(string->chars[i]);
        length++;
    }
    return length;
}

LitString* lit_ustring_code_point_at(LitState* state, LitString* string, uint32_t index)
{
    if(index >= string->length)
    {
        return NULL;
    }
    int codepoint = lit_ustring_decode((uint8_t*)string->chars + index, string->length - index);
    if(codepoint == -1)
    {
        char bytes[2];
        bytes[0] = string->chars[index];
        bytes[1] = '\0';
        return lit_string_copy(state, bytes, 1);
    }
    return lit_ustring_from_code_point(state, codepoint);
}

LitString* lit_ustring_from_code_point(LitState* state, int value)
{
    int length = lit_encode_num_bytes(value);
    char bytes[length + 1];
    lit_ustring_encode(value, (uint8_t*)bytes);
    return lit_string_copy(state, bytes, length);
}

LitString* lit_ustring_from_range(LitState* state, LitString* source, int start, uint32_t count)
{
    uint8_t* from = (uint8_t*)source->chars;
    int length = 0;
    for(uint32_t i = 0; i < count; i++)
    {
        length += lit_decode_num_bytes(from[start + i]);
    }
    char bytes[length];
    uint8_t* to = (uint8_t*)bytes;
    for(uint32_t i = 0; i < count; i++)
    {
        int index = start + i;
        int codepoint = lit_ustring_decode(from + index, source->length - index);
        if(codepoint != -1)
        {
            to += lit_ustring_encode(codepoint, to);
        }
    }
    return lit_string_copy(state, bytes, length);
}

int lit_encode_num_bytes(int value)
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

int lit_ustring_encode(int value, uint8_t* bytes)
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

int lit_ustring_decode(const uint8_t* bytes, uint32_t length)
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

int lit_uchar_offset(char* str, int index)
{
#define is_utf(c) (((c)&0xC0) != 0x80)
    int offset = 0;
    while(index > 0 && str[offset])
    {
        (void)(is_utf(str[++offset]) || is_utf(str[++offset]) || is_utf(str[++offset]) || ++offset);
        index--;
    }
    return offset;
#undef is_utf
}

bool lit_value_isbool(LitValue v)
{
    return (v.type == TINVAL_BOOL);
}

bool lit_value_isnull(LitValue v)
{
    return (v.type == TINVAL_NULL);
}

bool lit_value_isnumber(LitValue v)
{
    return (v.type == TINVAL_NUMBER);
}

bool lit_value_isobject(LitValue v)
{
    return (v.type == TINVAL_OBJECT);
}

bool IS_OBJECTS_TYPE(LitValue value, LitObjType t)
{
    if(lit_value_isobject(value))
    {
        return (lit_value_asobject(value)->type == t);
    }
    return false;
}

bool lit_value_ismap(LitValue value)
{
    return IS_OBJECTS_TYPE(value, LIT_OBJ_MAP);
}

#define IS_STRING(value) IS_OBJECTS_TYPE(value, LIT_OBJ_STRING)
#define IS_FUNCTION(value) IS_OBJECTS_TYPE(value, LIT_OBJ_FUNCTION)
#define IS_NATIVE_METHOD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_NATIVEMETHOD)
#define IS_PRIMITIVE_METHOD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_PRIMITIVEMETHOD)
#define IS_MODULE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_MODULE)
#define IS_CLOSURE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLOSURE)
#define IS_CLOSURE_PROTOTYPE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLOSUREPROTOTYPE)
#define IS_CLASS(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLASS)
bool lit_value_isinstance(LitValue value)
{
    return IS_OBJECTS_TYPE(value, LIT_OBJ_INSTANCE);
}

#define IS_ARRAY(value) (IS_OBJECTS_TYPE(value, LIT_OBJ_ARRAY) || IS_OBJECTS_TYPE(value, LIT_OBJ_VARARGARRAY))
#define IS_VARARG_ARRAY(value) IS_OBJECTS_TYPE(value, LIT_OBJ_VARARGARRAY)
#define IS_RANGE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_RANGE)
#define IS_FIELD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_FIELD)
#define IS_REFERENCE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_REFERENCE)


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


#define AS_STRING(value) ((LitString*)lit_value_asobject(value))
#define AS_CSTRING(value) (((LitString*)lit_value_asobject(value))->chars)
#define AS_FUNCTION(value) ((LitFunction*)lit_value_asobject(value))
#define AS_NATIVE_FUNCTION(value) ((LitNativeFunction*)lit_value_asobject(value))
#define AS_NATIVE_PRIMITIVE(value) ((LitNativePrimitive*)lit_value_asobject(value))
#define AS_NATIVE_METHOD(value) ((LitNativeMethod*)lit_value_asobject(value))
#define AS_PRIMITIVE_METHOD(value) ((LitPrimitiveMethod*)lit_value_asobject(value))
#define AS_MODULE(value) ((LitModule*)lit_value_asobject(value))
#define AS_CLOSURE(value) ((LitClosure*)lit_value_asobject(value))
#define AS_CLOSURE_PROTOTYPE(value) ((LitClosurePrototype*)lit_value_asobject(value))
#define AS_UPVALUE(value) ((LitUpvalue*)lit_value_asobject(value))
#define AS_CLASS(value) ((LitClass*)lit_value_asobject(value))
LitInstance* lit_value_asinstance(LitValue value)
{
    return ((LitInstance*)lit_value_asobject(value));
}

#define AS_ARRAY(value) ((LitArray*)lit_value_asobject(value))
#define AS_MAP(value) ((LitMap*)lit_value_asobject(value))
#define AS_BOUND_METHOD(value) ((LitBoundMethod*)lit_value_asobject(value))
#define AS_USERDATA(value) ((LitUserdata*)lit_value_asobject(value))
#define AS_RANGE(value) ((LitRange*)lit_value_asobject(value))
#define AS_FIELD(value) ((LitField*)lit_value_asobject(value))
#define AS_FIBER(value) ((LitFiber*)lit_value_asobject(value))
#define AS_REFERENCE(value) ((LitReference*)lit_value_asobject(value))

#define memset(...)

LitValue lit_value_makenull()
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = TINVAL_NULL;
    rt.as.numval = 0;
    return rt;    
}

LitValue lit_value_makebool(bool b)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = TINVAL_BOOL;
    rt.as.boolval = b;
    return rt;    
}

LitValue lit_value_makenumber(double num)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = TINVAL_NUMBER;
    rt.as.numval = num;
    return rt;
}

#define lit_value_fromobject(obj) lit_value_fromobject_actual((LitObject*)(obj))

LitValue lit_value_fromobject_actual(LitObject* obj)
{
    LitValue rt;
    memset(&rt, 0, sizeof(LitValue));
    rt.type = TINVAL_OBJECT;
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
    if(lit_value_isnull(a) && lit_value_isnull(b))
    {
        return true;
    }
    if(lit_value_isnumber(a) && lit_value_isnumber(b))
    {
        return lit_value_asnumber(a) == lit_value_asnumber(b);
    }
    if(lit_value_isbool(a) && lit_value_isbool(b))
    {
        return lit_value_asbool(a) == lit_value_asbool(b);
    }
    if(lit_value_isbool(a) && lit_value_isnumber(b))
    {
        return lit_value_asbool(a) == lit_value_asnumber(b);
    }
    if(lit_value_isnumber(a) && lit_value_isbool(b))
    {
        return lit_value_asnumber(a) == lit_value_asbool(b);
    }
    return !lit_is_falsey(lit_state_findandcallmethod(state, a, CONST_STRING(state, "=="), (LitValue[1]){ b }, 1).result);
}


void lit_value_printobject(LitValue value)
{
    switch(OBJECT_TYPE(value))
    {
        case LIT_OBJ_STRING:
        {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case LIT_OBJ_FUNCTION:
        {
            printf("function %s", AS_FUNCTION(value)->name->chars);
            break;
        }
        case LIT_OBJ_CLOSURE:
        {
#ifdef LIT_TRACE_STACK
            printf("closure %s", AS_CLOSURE(value)->function->name->chars);
#else
            printf("function %s", AS_CLOSURE(value)->function->name->chars);
#endif
            break;
        }
        case LIT_OBJ_CLOSUREPROTOTYPE:
        {
#ifdef LIT_TRACE_STACK
            printf("closure %s", AS_CLOSURE_PROTOTYPE(value)->function->name->chars);
#else
            printf("function %s", AS_CLOSURE_PROTOTYPE(value)->function->name->chars);
#endif
            break;
        }
        case LIT_OBJ_NATIVEPRIMITIVE:
        {
            printf("function %s", AS_NATIVE_PRIMITIVE(value)->name->chars);
            break;
        }
        case LIT_OBJ_NATIVEFUNCTION:
        {
            printf("function %s", AS_NATIVE_FUNCTION(value)->name->chars);
            break;
        }
        case LIT_OBJ_PRIMITIVEMETHOD:
        {
            printf("function %s", AS_PRIMITIVE_METHOD(value)->name->chars);
            break;
        }
        case LIT_OBJ_NATIVEMETHOD:
        {
            printf("function %s", AS_NATIVE_METHOD(value)->name->chars);
            break;
        }
        case LIT_OBJ_FIBER:
        {
            printf("fiber");
            break;
        }
        case LIT_OBJ_MODULE:
        {
            printf("module %s", AS_MODULE(value)->name->chars);
            break;
        }
        case LIT_OBJ_UPVALUE:
        {
            LitUpvalue* upvalue = AS_UPVALUE(value);
            if(upvalue->location == NULL)
            {
                lit_value_printvalue(upvalue->closed);
            }
            else
            {
                lit_value_printobject(*upvalue->location);
            }
            break;
        }
        case LIT_OBJ_CLASS:
        {
            printf("class %s", AS_CLASS(value)->name->chars);
            break;
        }
        case LIT_OBJ_INSTANCE:
        {
            printf("%s instance", lit_value_asinstance(value)->klass->name->chars);
            break;
        }
        case LIT_OBJ_BOUNDMETHOD:
        {
            lit_value_printvalue(AS_BOUND_METHOD(value)->method);
            return;
        }
        case LIT_OBJ_VARARGARRAY:
        case LIT_OBJ_ARRAY:
        {
#ifdef LIT_MINIMIZE_CONTAINERS
            printf("array");
#else
            LitArray* array = AS_ARRAY(value);
            LitUInt size = array->values.count;
            printf("[");
            if(size > 32)
            {
                printf(" (too big to be displayed) ");
            }
            else if(size > 0)
            {
                printf(" ");
                for(LitUInt i = 0; i < size; i++)
                {
                    lit_value_printvalue(array->values.values[i]);
                    if(i + 1 < size)
                    {
                        printf(", ");
                    }
                    else
                    {
                        printf(" ");
                    }
                }
            }
            printf("]");
#endif
            break;
        }
        case LIT_OBJ_MAP:
        {
#ifdef LIT_MINIMIZE_CONTAINERS
            printf("map");
#else
            LitMap* map = AS_MAP(value);
            LitUInt size = map->values.count;
            printf("{");
            bool hadbefore = false;
            if(size > 16)
            {
                printf(" (too big to be displayed) ");
            }
            else if(size > 0)
            {
                for(int i = 0; i < map->values.capacity; i++)
                {
                    LitTabEntry* entry = &map->values.entries[i];
                    if(entry->key != NULL)
                    {
                        if(hadbefore)
                        {
                            printf(", ");
                        }
                        else
                        {
                            printf(" ");
                        }
                        printf("%s: ", entry->key->chars);
                        lit_value_printvalue(entry->value);
                        hadbefore = true;
                    }
                }
            }
            if(hadbefore)
            {
                printf(" }");
            }
            else
            {
                printf("}");
            }
#endif
            break;
        }
        case LIT_OBJ_USERDATA:
        {
            printf("userdata");
            break;
        }
        case LIT_OBJ_RANGE:
        {
            LitRange* range = AS_RANGE(value);
            printf("%g .. %g", range->from, range->to);
            break;
        }
        case LIT_OBJ_FIELD:
        {
            printf("field");
            break;
        }
        case LIT_OBJ_REFERENCE:
        {
            printf("reference => ");
            LitValue* slot = AS_REFERENCE(value)->slot;
            if(slot == NULL)
            {
                printf("null");
            }
            else
            {
                lit_value_printvalue(*slot);
            }
            break;
        }
        default:
        {
            printf("[unknown object %p %i]", &value, OBJECT_TYPE(value));
            break;
        }
    }
}

void lit_value_printvalue(LitValue value)
{
    if(lit_value_isbool(value))
    {
        printf(lit_value_asbool(value) ? "true" : "false");
    }
    else if(lit_value_isnull(value))
    {
        printf("null");
    }
    else if(lit_value_isnumber(value))
    {
        printf("%g", lit_value_asnumber(value));
    }
    else if(lit_value_isobject(value))
    {
        lit_value_printobject(value);
    }
    else
    {
        printf("[unknown value %p]", &value);
    }
}

void lit_dynlistval_ensuresize(LitDynListVal* values, LitUInt size)
{
    lit_dynlistval_ensureactualsize(values, size);
    if(values->count < size)
    {
        values->count = size;
    }
}

void lit_dynlistval_ensureactualsize(LitDynListVal* values, LitUInt size)
{
    if(values->capacity < size)
    {
        LitUInt oldcapacity = values->capacity;
        values->capacity = size;
        values->values = (LitValue*)lit_sysmem_realloc(values->values, sizeof(LitValue) * (size));
        for(LitUInt i = oldcapacity; i < size; i++)
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
    state->api_name = lit_string_copy(state, "c", 1);
    state->api_function = NULL;
}

void lit_api_destroy(LitState* state)
{
    state->api_name = NULL;
    state->api_function = NULL;
}

LitValue lit_state_getglobal(LitState* state, LitString* name)
{
    LitValue global;
    if(!lit_table_get(&state->vm->globals->values, name, &global))
    {
        return lit_value_makenull();
    }
    return global;
}

LitFunction* lit_state_getglobalfunction(LitState* state, LitString* name)
{
    LitValue function = lit_state_getglobal(state, name);
    if(IS_FUNCTION(function))
    {
        return AS_FUNCTION(function);
    }
    return NULL;
}

void lit_state_setglobal(LitState* state, LitString* name, LitValue value)
{
    lit_state_pushroot(state, (LitObject*)name);
    lit_state_pushvalueroot(state, value);
    lit_table_set(state, &state->vm->globals->values, name, value);
    lit_state_poproots(state, 2);
}

bool lit_state_globalexists(LitState* state, LitString* name)
{
    LitValue global;
    return lit_table_get(&state->vm->globals->values, name, &global);
}

void lit_state_defnative(LitState* state, const char* name, LitNativeFunctionFn native)
{
    lit_state_pushroot(state, (LitObject*)CONST_STRING(state, name));
    lit_state_pushroot(state, (LitObject*)lit_object_makenativefunc(state, native, AS_STRING(lit_state_peekroot(state, 0))));
    lit_table_set(state, &state->vm->globals->values, AS_STRING(lit_state_peekroot(state, 1)), lit_state_peekroot(state, 0));
    lit_state_poproots(state, 2);
}

void lit_state_defnativeprimitive(LitState* state, const char* name, LitNativePrimitiveFn native)
{
    lit_state_pushroot(state, (LitObject*)CONST_STRING(state, name));
    lit_state_pushroot(state, (LitObject*)lit_object_makenativeprimitive(state, native, AS_STRING(lit_state_peekroot(state, 0))));
    lit_table_set(state, &state->vm->globals->values, AS_STRING(lit_state_peekroot(state, 1)), lit_state_peekroot(state, 0));
    lit_state_poproots(state, 2);
}

double lit_args_checknumber(LitVm* vm, const char* sourcefname, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !lit_value_isnumber(args[id]))
    {
        lit_vm_raisefatalerror(vm, "in %s: Expected a number as argument #%i, got a %s", sourcefname, (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return lit_value_asnumber(args[id]);
}

double lit_args_getnumber(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id, double def)
{
    (void)vm;
    if(argc <= id || !lit_value_isnumber(args[id]))
    {
        return def;
    }
    return lit_value_asnumber(args[id]);
}

bool lit_args_checkbool(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !lit_value_isbool(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a boolean as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return lit_value_asbool(args[id]);
}

bool lit_args_getbool(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id, bool def)
{
    (void)vm;
    if(argc <= id || !lit_value_isbool(args[id]))
    {
        return def;
    }
    return lit_value_asbool(args[id]);
}

const char* lit_args_checkstring(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return AS_STRING(args[id])->chars;
}

const char* lit_args_getstring(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id, const char* def)
{
    (void)vm;
    if(argc <= id || !IS_STRING(args[id]))
    {
        return def;
    }
    return AS_STRING(args[id])->chars;
}

LitString* lit_args_checkobjstring(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a string as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return AS_STRING(args[id]);
}

LitInstance* lit_args_checkinstance(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !lit_value_isinstance(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected an instance as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return lit_value_asinstance(args[id]);
}

LitValue* lit_check_reference(LitVm* vm, LitValue* args, uint8_t argc, uint8_t id)
{
    if(argc <= id || !IS_REFERENCE(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a reference as argument #%i, got a %s", (int)id, id >= argc ? "null" : lit_get_value_type(args[id]));
    }
    return AS_REFERENCE(args[id])->slot;
}

void lit_args_ensurebool(LitVm* vm, LitValue value, const char* error)
{
    if(!lit_value_isbool(value))
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

void lit_args_ensurestring(LitVm* vm, LitValue value, const char* error)
{
    if(!IS_STRING(value))
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

void lit_args_ensurenumber(LitVm* vm, LitValue value, const char* error)
{
    if(!lit_value_isnumber(value))
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

void lit_args_ensureobjtype(LitVm* vm, LitValue value, LitObjType type, const char* error)
{
    if(!lit_value_isobject(value) || OBJECT_TYPE(value) != type)
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

LitValue lit_table_getfield(LitState* state, LitTable* table, const char* name)
{
    LitValue value;
    if(!lit_table_get(table, CONST_STRING(state, name), &value))
    {
        value = lit_value_makenull();
    }
    return value;
}

LitValue lit_map_getfield(LitState* state, LitMap* map, const char* name)
{
    LitValue value;
    if(!lit_table_get(&map->values, CONST_STRING(state, name), &value))
    {
        value = lit_value_makenull();
    }
    return value;
}

void lit_table_setfield(LitState* state, LitTable* table, const char* name, LitValue value)
{
    lit_table_set(state, table, CONST_STRING(state, name), value);
}

void lit_map_setfield(LitState* state, LitMap* map, const char* name, LitValue value)
{
    lit_table_set(state, &map->values, CONST_STRING(state, name), value);
}

void lit_chunk_init(LitChunk* chunk)
{
    lit_chunk_reset(chunk);
    lit_vallist_init(&chunk->constants);
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

void lit_chunk_destroy(LitState* state, LitChunk* chunk)
{
    lit_sysmem_free(chunk->compiledcodechunk);
    lit_sysmem_free(chunk->lines);
    lit_vallist_destroy(state, &chunk->constants);
    lit_chunk_reset(chunk);
}

void lit_chunk_push(LitChunk* chunk, uint64_t word, uint16_t line)
{
    if(chunk->capacity < chunk->compiledcodecount + 1)
    {
        LitUInt oldcapacity = chunk->capacity;
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
        LitUInt oldcapacity = chunk->linecapacity;
        chunk->linecapacity = LIT_GROW_CAPACITY(chunk->linecapacity);
        chunk->lines = (uint16_t*)lit_sysmem_realloc(chunk->lines, sizeof(uint16_t) * (chunk->linecapacity));
        if(oldcapacity == 0)
        {
            chunk->lines[0] = 0;
            chunk->lines[1] = 0;
        }
    }
    LitUInt lineindex = chunk->linecount;
    LitUInt value = chunk->lines[lineindex];
    if(value != 0 && value != line)
    {
        chunk->linecount += 2;
        lineindex = chunk->linecount;
        chunk->lines[lineindex + 1] = 0;
    }
    chunk->lines[lineindex] = line;
    chunk->lines[lineindex + 1]++;
}

LitUInt lit_chunk_addconstant(LitState* state, LitChunk* chunk, LitValue constant)
{
    lit_state_pushvalueroot(state, constant);
    lit_vallist_push(&chunk->constants, constant);
    lit_state_poproot(state);
    return chunk->constants.count - 1;
}

LitUInt lit_chunk_getline(LitChunk* chunk, LitUInt offset)
{
    if(!chunk->haslineinfo)
    {
        return 0;
    }
    LitUInt rle = 0;
    LitUInt line = 0;
    LitUInt index = 0;
    for(LitUInt i = 0; i <= offset; i++)
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
    LitUInt oldcapacity;
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
        LitUInt oldcapacity = array->capacity;
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

void lit_paramlist_destroy(LitState* state, LitDynListParam* array)
{
    LIT_FREE_ARRAY(state, LitParameter, array->values, array->capacity);
    lit_paramlist_init(array);
}

void lit_paramlist_push(LitDynListParam* array, LitParameter value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitParameter*)lit_sysmem_realloc(array->values, sizeof(LitParameter) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
};


void lit_ast_destroyparamlist(LitState* state, LitDynListParam* parameters)
{
    for(LitUInt i = 0; i < parameters->count; i++)
    {
        lit_ast_destroyexpression(state, parameters->values[i].default_value);
    }
    lit_paramlist_destroy(state, parameters);
}

void lit_ast_destroyexprlist(LitState* state, LitDynListExpr* expressions)
{
    if(expressions == NULL)
    {
        return;
    }
    for(LitUInt i = 0; i < expressions->count; i++)
    {
        lit_ast_destroyexpression(state, expressions->values[i]);
    }
    lit_exprlist_destroy(expressions);
}

void lit_ast_destroystmtlist(LitState* state, LitDynListExpr* statements)
{
    if(statements == NULL)
    {
        return;
    }
    for(LitUInt i = 0; i < statements->count; i++)
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
        case LIT_EXPR_LITERAL:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_BINARY:
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
        case LIT_EXPR_UNARY:
        {
            lit_ast_destroyexpression(state, ((LitUnaryExpression*)expression)->right);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_VAR:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_ASSIGN:
        {
            LitAssignExpression* expr = (LitAssignExpression*)expression;
            lit_ast_destroyexpression(state, expr->to);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_CALL:
        {
            LitCallExpression* expr = (LitCallExpression*)expression;
            lit_ast_destroyexpression(state, expr->callee);
            lit_ast_destroyexpression(state, expr->init);
            lit_ast_destroyexprlist(state, &expr->args);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_GET:
        {
            lit_ast_destroyexpression(state, ((LitGetExpression*)expression)->where);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_SET:
        {
            LitSetExpression* expr = (LitSetExpression*)expression;
            lit_ast_destroyexpression(state, expr->where);
            lit_ast_destroyexpression(state, expr->value);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_LAMBDA:
        {
            LitFunctionStatement* expr = (LitFunctionStatement*)expression;
            lit_ast_destroyparamlist(state, &expr->parameters);
            lit_ast_destroystmt(state, expr->body);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_ARRAY:
        {
            lit_ast_destroyexprlist(state, &((LitArrayExpression*)expression)->values);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_OBJECT:
        {
            LitObjectExpression* map = (LitObjectExpression*)expression;
            lit_vallist_destroy(state, &map->keys);
            lit_ast_destroyexprlist(state, &map->values);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_SUBSCRIPT:
        {
            LitSubscriptExpression* expr = (LitSubscriptExpression*)expression;
            lit_ast_destroyexpression(state, expr->array);
            lit_ast_destroyexpression(state, expr->index);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_THIS:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_SUPER:
        {
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_RANGE:
        {
            LitRangeExpression* expr = (LitRangeExpression*)expression;
            lit_ast_destroyexpression(state, expr->from);
            lit_ast_destroyexpression(state, expr->to);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_TERNARY:
        {
            LitTernaryExpression* expr = (LitTernaryExpression*)expression;
            lit_ast_destroyexpression(state, expr->condition);
            lit_ast_destroyexpression(state, expr->if_branch);
            lit_ast_destroyexpression(state, expr->else_branch);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_INTERPOLATION:
        {
            lit_ast_destroyexprlist(state, &((LitInterpolationExpression*)expression)->expressions);
            lit_sysmem_free(expression);
            break;
        }
        case LIT_EXPR_REFERENCE:
        {
            lit_ast_destroyexpression(state, ((LitReferenceExpression*)expression)->to);
            lit_sysmem_free(expression);
            break;
        }
        default:
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Unknown expression type %d", (int)expression->type);
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

LitLiteralExpression* lit_ast_makeliteralexpr(LitUInt line, LitValue value)
{
    LitLiteralExpression* expression = (LitLiteralExpression*)lit_ast_allocexpression(line, sizeof(LitLiteralExpression), LIT_EXPR_LITERAL);
    expression->value = value;
    return expression;
}

LitBinaryExpression* lit_ast_makebinaryexpr(LitUInt line, LitExpression* left, LitExpression* right, LitTokenType op)
{
    LitBinaryExpression* expression = (LitBinaryExpression*)lit_ast_allocexpression(line, sizeof(LitBinaryExpression), LIT_EXPR_BINARY);
    expression->left = left;
    expression->right = right;
    expression->op = op;
    expression->ignore_left = false;
    return expression;
}

LitUnaryExpression* lit_ast_makeunaryexpr(LitUInt line, LitExpression* right, LitTokenType op)
{
    LitUnaryExpression* expression = (LitUnaryExpression*)lit_ast_allocexpression(line, sizeof(LitUnaryExpression), LIT_EXPR_UNARY);
    expression->right = right;
    expression->op = op;
    return expression;
}

LitVarExpression* lit_ast_makevarexpr(LitUInt line, const char* name, LitUInt length)
{
    LitVarExpression* expression = (LitVarExpression*)lit_ast_allocexpression(line, sizeof(LitVarExpression), LIT_EXPR_VAR);
    expression->name = name;
    expression->length = length;
    return expression;
}

LitAssignExpression* lit_ast_makeassignexpr(LitUInt line, LitExpression* to, LitExpression* value)
{
    LitAssignExpression* expression = (LitAssignExpression*)lit_ast_allocexpression(line, sizeof(LitAssignExpression), LIT_EXPR_ASSIGN);
    expression->to = to;
    expression->value = value;
    return expression;
}

LitCallExpression* lit_ast_makecallexpr(LitUInt line, LitExpression* callee)
{
    LitCallExpression* expression = (LitCallExpression*)lit_ast_allocexpression(line, sizeof(LitCallExpression), LIT_EXPR_CALL);
    expression->callee = callee;
    expression->init = NULL;
    lit_exprlist_init(&expression->args);
    return expression;
}

LitGetExpression* lit_ast_makegetexpr(LitUInt line, LitExpression* where, const char* name, LitUInt length, bool questionable, bool ignore_result)
{
    LitGetExpression* expression = (LitGetExpression*)lit_ast_allocexpression(line, sizeof(LitGetExpression), LIT_EXPR_GET);
    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->ignore_emit = false;
    expression->jump = questionable ? 0 : -1;
    expression->ignore_result = ignore_result;
    return expression;
}

LitSetExpression* lit_ast_makesetexpr(LitUInt line, LitExpression* where, const char* name, LitUInt length, LitExpression* value)
{
    LitSetExpression* expression = (LitSetExpression*)lit_ast_allocexpression(line, sizeof(LitSetExpression), LIT_EXPR_SET);
    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->value = value;
    return expression;
}

LitFunctionStatement* lit_ast_makelambdaexpr(LitUInt line)
{
    LitFunctionStatement* expression = (LitFunctionStatement*)lit_ast_allocexpression(line, sizeof(LitFunctionStatement), LIT_EXPR_LAMBDA);
    expression->body = NULL;
    lit_paramlist_init(&expression->parameters);
    return expression;
}

LitArrayExpression* lit_ast_makearrayexpr(LitUInt line)
{
    LitArrayExpression* expression = (LitArrayExpression*)lit_ast_allocexpression(line, sizeof(LitArrayExpression), LIT_EXPR_ARRAY);
    lit_exprlist_init(&expression->values);
    return expression;
}

LitObjectExpression* lit_ast_makeobjectexpr(LitUInt line)
{
    LitObjectExpression* expression = (LitObjectExpression*)lit_ast_allocexpression(line, sizeof(LitObjectExpression), LIT_EXPR_OBJECT);
    lit_vallist_init(&expression->keys);
    lit_exprlist_init(&expression->values);
    return expression;
}

LitSubscriptExpression* lit_ast_makesubscriptexpr(LitUInt line, LitExpression* array, LitExpression* index)
{
    LitSubscriptExpression* expression = (LitSubscriptExpression*)lit_ast_allocexpression(line, sizeof(LitSubscriptExpression), LIT_EXPR_SUBSCRIPT);
    expression->array = array;
    expression->index = index;
    return expression;
}

LitThisExpression* lit_ast_makethisexpr(LitUInt line)
{
    return (LitThisExpression*)lit_ast_allocexpression(line, sizeof(LitThisExpression), LIT_EXPR_THIS);
}

LitSuperExpression* lit_ast_makesuperexpr(LitUInt line, LitString* method, bool ignore_result)
{
    LitSuperExpression* expression = (LitSuperExpression*)lit_ast_allocexpression(line, sizeof(LitSuperExpression), LIT_EXPR_SUPER);
    expression->method = method;
    expression->ignore_emit = false;
    expression->ignore_result = ignore_result;
    return expression;
}

LitRangeExpression* lit_ast_makerangeexpr(LitUInt line, LitExpression* from, LitExpression* to)
{
    LitRangeExpression* expression = (LitRangeExpression*)lit_ast_allocexpression(line, sizeof(LitRangeExpression), LIT_EXPR_RANGE);
    expression->from = from;
    expression->to = to;
    return expression;
}

LitTernaryExpression* lit_ast_maketernaryexpr(LitUInt line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch)
{
    LitTernaryExpression* expression = (LitTernaryExpression*)lit_ast_allocexpression(line, sizeof(LitTernaryExpression), LIT_EXPR_TERNARY);
    expression->condition = condition;
    expression->if_branch = if_branch;
    expression->else_branch = else_branch;
    return expression;
}

LitInterpolationExpression* lit_ast_makeinterpolationexpr(LitUInt line)
{
    LitInterpolationExpression* expression = (LitInterpolationExpression*)lit_ast_allocexpression(line, sizeof(LitInterpolationExpression), LIT_EXPR_INTERPOLATION);
    lit_exprlist_init(&expression->expressions);
    return expression;
}

LitReferenceExpression* lit_ast_makerefexpr(LitUInt line, LitExpression* to)
{
    LitReferenceExpression* expression = (LitReferenceExpression*)lit_ast_allocexpression(line, sizeof(LitReferenceExpression), LIT_EXPR_REFERENCE);
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
        case LIT_EXPR_EXPRESSION:
        {
            lit_ast_destroyexpression(state, ((LitExpressionStatement*)statement)->expression);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_BLOCK:
        {
            lit_ast_destroystmtlist(state, &((LitBlockStatement*)statement)->statements);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_VARDECL:
        {
            lit_ast_destroyexpression(state, ((LitVarStatement*)statement)->init);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_IF:
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
        case LIT_EXPR_WHILE:
        {
            LitWhileStatement* stmt = (LitWhileStatement*)statement;
            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroystmt(state, stmt->body);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_FOR:
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
        case LIT_EXPR_CONTINUE:
        {
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_BREAK:
        {
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_FUNCTION:
        {
            LitFunctionStatement* stmt = (LitFunctionStatement*)statement;
            lit_ast_destroystmt(state, stmt->body);
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_RETURN:
        {
            lit_ast_destroyexpression(state, ((LitReturnStatement*)statement)->expression);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_METHOD:
        {
            LitMethodStatement* stmt = (LitMethodStatement*)statement;
            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_ast_destroystmt(state, stmt->body);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_CLASS:
        {
            lit_ast_destroystmtlist(state, &((LitClassStatement*)statement)->fields);
            lit_sysmem_free(statement);
            break;
        }
        case LIT_EXPR_FIELD:
        {
            LitFieldStatement* stmt = (LitFieldStatement*)statement;
            lit_ast_destroystmt(state, stmt->getter);
            lit_ast_destroystmt(state, stmt->setter);
            lit_sysmem_free(statement);
            break;
        }
        default:
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Unknown statement type %d", (int)statement->type);
            break;
        }
    }
}

LitExpressionStatement* lit_ast_makeexprstmt(LitUInt line, LitExpression* expression)
{
    LitExpressionStatement* statement = (LitExpressionStatement*)lit_ast_allocexpression(line, sizeof(LitExpressionStatement), LIT_EXPR_EXPRESSION);
    statement->expression = expression;
    return statement;
}

LitBlockStatement* lit_ast_makeblockstmt(LitUInt line)
{
    LitBlockStatement* statement = (LitBlockStatement*)lit_ast_allocexpression(line, sizeof(LitBlockStatement), LIT_EXPR_BLOCK);
    lit_exprlist_init(&statement->statements);
    return statement;
}

LitVarStatement* lit_ast_makevardefstmt(LitUInt line, const char* name, LitUInt length, LitExpression* init, bool constant)
{
    LitVarStatement* statement = (LitVarStatement*)lit_ast_allocexpression(line, sizeof(LitVarStatement), LIT_EXPR_VARDECL);
    statement->name = name;
    statement->length = length;
    statement->init = init;
    statement->constant = constant;
    return statement;
}

LitIfStatement* lit_ast_makeifstatement(LitUInt line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch, LitDynListExpr* elseif_conditions, LitDynListExpr* elseif_branches)
{
    LitIfStatement* statement = (LitIfStatement*)lit_ast_allocexpression(line, sizeof(LitIfStatement), LIT_EXPR_IF);
    statement->condition = condition;
    statement->if_branch = if_branch;
    statement->else_branch = else_branch;
    statement->elseif_conditions = elseif_conditions;
    statement->elseif_branches = elseif_branches;
    return statement;
}

LitWhileStatement* lit_ast_makewhilestmt(LitUInt line, LitExpression* condition, LitExpression* body)
{
    LitWhileStatement* statement = (LitWhileStatement*)lit_ast_allocexpression(line, sizeof(LitWhileStatement), LIT_EXPR_WHILE);
    statement->condition = condition;
    statement->body = body;
    return statement;
}

LitForStatement* lit_ast_makeforstmt(LitUInt line, LitExpression* init, LitExpression* var, LitExpression* condition, LitExpression* increment, LitExpression* body, bool c_style)
{
    LitForStatement* statement = (LitForStatement*)lit_ast_allocexpression(line, sizeof(LitForStatement), LIT_EXPR_FOR);
    statement->init = init;
    statement->var = var;
    statement->condition = condition;
    statement->increment = increment;
    statement->body = body;
    statement->c_style = c_style;
    return statement;
}

LitContinueStatement* lit_ast_makecontinuestmt(LitUInt line)
{
    return (LitContinueStatement*)lit_ast_allocexpression(line, sizeof(LitContinueStatement), LIT_EXPR_CONTINUE);
}

LitBreakStatement* lit_ast_makebreakstmt(LitUInt line)
{
    return (LitBreakStatement*)lit_ast_allocexpression(line, sizeof(LitBreakStatement), LIT_EXPR_BREAK);
}

LitFunctionStatement* lit_ast_makefuncdefstmt(LitUInt line, const char* name, LitUInt length)
{
    LitFunctionStatement* function = (LitFunctionStatement*)lit_ast_allocexpression(line, sizeof(LitFunctionStatement), LIT_EXPR_FUNCTION);
    function->name = name;
    function->length = length;
    function->body = NULL;
    lit_paramlist_init(&function->parameters);
    return function;
}

LitReturnStatement* lit_ast_makereturnstmt(LitUInt line, LitExpression* expression)
{
    LitReturnStatement* statement = (LitReturnStatement*)lit_ast_allocexpression(line, sizeof(LitReturnStatement), LIT_EXPR_RETURN);
    statement->expression = expression;
    return statement;
}

LitMethodStatement* lit_ast_makemethoddefstmt(LitUInt line, LitString* name, bool is_static)
{
    LitMethodStatement* statement = (LitMethodStatement*)lit_ast_allocexpression(line, sizeof(LitMethodStatement), LIT_EXPR_METHOD);
    statement->name = name;
    statement->body = NULL;
    statement->is_static = is_static;
    lit_paramlist_init(&statement->parameters);
    return statement;
}

LitClassStatement* lit_ast_makeclassdefstmt(LitUInt line, LitString* name, LitString* parent)
{
    LitClassStatement* statement = (LitClassStatement*)lit_ast_allocexpression(line, sizeof(LitClassStatement), LIT_EXPR_CLASS);
    statement->name = name;
    statement->parent = parent;
    lit_exprlist_init(&statement->fields);
    return statement;
}

LitFieldStatement* lit_ast_makefieldstmt(LitUInt line, LitString* name, LitExpression* getter, LitExpression* setter, bool is_static)
{
    LitFieldStatement* statement = (LitFieldStatement*)lit_ast_allocexpression(line, sizeof(LitFieldStatement), LIT_EXPR_FIELD);
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
    if(expressions == NULL)
    {
        return;
    }
    for(LitUInt i = 0; i < expressions->count; i++)
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
    if(statements == NULL)
    {
        return;
    }
    for(LitUInt i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }
    lit_exprlist_destroy(statements);
    lit_sysmem_free(statements);
}

void lit_scanner_init(LitState* state, LitScanner* scanner, const char* file_name, const char* source)
{
    scanner->line = 1;
    scanner->start = source;
    scanner->current = source;
    scanner->file_name = file_name;
    scanner->state = state;
    scanner->num_braces = 0;
    scanner->had_error = false;
}

LitToken lit_scanner_maketoken(LitScanner* scanner, LitTokenType type)
{
    LitToken token;
    token.type = type;
    token.start = scanner->start;
    token.length = (LitUInt)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

LitToken lit_scanner_makeerrortoken(LitScanner* scanner, const char* fmt, ...)
{
    scanner->had_error = true;
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(scanner->state, scanner->line, fmt, args);
    va_end(args);
    LitToken token;
    token.type = LTOKEN_ERROR;
    token.start = result->chars;
    token.length = result->length;
    token.line = scanner->line;
    return token;
}

bool lit_scanner_isatend(LitScanner* scanner)
{
    return *scanner->current == '\0';
}

char lit_scanner_advance(LitScanner* scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

bool lit_scanner_match(LitScanner* scanner, char expected)
{
    if(lit_scanner_isatend(scanner))
    {
        return false;
    }
    if(*scanner->current != expected)
    {
        return false;
    }
    scanner->current++;
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
    return *scanner->current;
}

char lit_scanner_peeknext(LitScanner* scanner)
{
    if(lit_scanner_isatend(scanner))
    {
        return '\0';
    }
    return scanner->current[1];
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
                scanner->start = scanner->current;
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
                            scanner->line++;
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
    LitDynListByte bytes;
    LitTokenType stringtype;
    LitState* state;
    state = lex->state;
    stringtype = LTOKEN_STRING;
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
            if(lex->num_braces >= LIT_INTERPOLATION_NESTING_MAX)
            {
                lit_bytelist_destroy(state, &bytes);
                return lit_scanner_makeerrortoken(lex, "Interpolation nesting is too deep, maximum is %i", LIT_INTERPOLATION_NESTING_MAX);
            }
            stringtype = LTOKEN_INTERPOLATION;
            lex->braces[lex->num_braces++] = 1;
            break;
        }
        if(useescapes)
        {
            switch(currch)
            {
                case '\0':
                    {
                        lit_bytelist_destroy(state, &bytes);
                        return lit_scanner_makeerrortoken(lex, "Unterminated string");
                    }
                    break;
                case '\n':
                    {
                        lex->line++;
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
                                        lit_bytelist_destroy(state, &bytes);
                                        return lit_scanner_makeerrortoken(lex, "Invalid escape character '%c'", lex->current[-1]);
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
    token.value = lit_value_fromobject(lit_string_copy(state, (const char*)bytes.values, bytes.count));
    lit_bytelist_destroy(state, &bytes);
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
    scanner->current--;
    return -1;
}

int lit_scanner_scanbinarydigit(LitScanner* scanner)
{
    char c = lit_scanner_advance(scanner);
    if(c >= '0' && c <= '1')
    {
        return c - '0';
    }
    scanner->current--;
    return -1;
}

LitToken lit_scanner_makenumbertoken(LitScanner* scanner, bool ishex, bool isbinary)
{
    errno = 0;
    LitValue value;
    if(ishex)
    {
        value = lit_value_makenumber((double)strtoll(scanner->start, NULL, 16));
    }
    else if(isbinary)
    {
        value = lit_value_makenumber((int)strtoll(scanner->start + 2, NULL, 2));
    }
    else
    {
        value = lit_value_makenumber(strtod(scanner->start, NULL));
    }
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_scanner_makeerrortoken(scanner, "Number is too big to be represented by a single literal");
    }
    LitToken token = lit_scanner_maketoken(scanner, LTOKEN_NUMBER);
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
    // Look for a fractional part.
    if(lit_scanner_peek(scanner) == '.' && lit_is_digit(lit_scanner_peeknext(scanner)))
    {
        // Consume the '.'
        lit_scanner_advance(scanner);
        while(lit_is_digit(lit_scanner_peek(scanner)))
        {
            lit_scanner_advance(scanner);
        }
    }
    return lit_scanner_makenumbertoken(scanner, false, false);
}

LitTokenType lit_scanner_checkkeyword(LitScanner* scanner, int start, int length, const char* rest, LitTokenType type)
{
    if(scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0)
    {
        return type;
    }
    return LTOKEN_IDENTIFIER;
}

LitTokenType lit_scanner_scanidenttype(LitScanner* scanner)
{
    switch(scanner->start[0])
    {
        case 'b':
            return lit_scanner_checkkeyword(scanner, 1, 4, "reak", LTOKEN_BREAK);
        case 'c':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'l':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "ass", LTOKEN_CLASS);
                    case 'o':
                    {
                        if(scanner->current - scanner->start > 3)
                        {
                            switch(scanner->start[3])
                            {
                                case 's':
                                    return lit_scanner_checkkeyword(scanner, 2, 3, "nst", LTOKEN_CONST);
                                case 't':
                                    return lit_scanner_checkkeyword(scanner, 2, 6, "ntinue", LTOKEN_CONTINUE);
                            }
                        }
                    }
                }
            }
            break;
        }
        case 'e':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'l':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "se", LTOKEN_ELSE);
                    case 'x':
                        return lit_scanner_checkkeyword(scanner, 2, 4, "port", LTOKEN_EXPORT);
                }
            }
            break;
        }
        case 'f':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'a':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "lse", LTOKEN_FALSE);
                    case 'o':
                        return lit_scanner_checkkeyword(scanner, 2, 1, "r", LTOKEN_FOR);
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 6, "nction", LTOKEN_FUNCTION);
                }
            }
            break;
        }
        case 'i':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 's':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IS);
                    case 'f':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IF);
                    case 'n':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IN);
                }
            }
            break;
        }
        case 'n':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "ll", LTOKEN_NULL);
                    case 'e':
                        return lit_scanner_checkkeyword(scanner, 2, 1, "w", LTOKEN_NEW);
                }
            }
            break;
        }
        case 'r':
        {
            if(scanner->current - scanner->start > 2)
            {
                switch(scanner->start[2])
                {
                    case 'f':
                        return lit_scanner_checkkeyword(scanner, 3, 0, "", LTOKEN_REF);
                    case 't':
                        return lit_scanner_checkkeyword(scanner, 3, 3, "urn", LTOKEN_RETURN);
                }
            }
            break;
        }
        case 'o':
            return lit_scanner_checkkeyword(scanner, 1, 7, "perator", LTOKEN_OPERATOR);
        case 's':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "per", LTOKEN_SUPER);
                    case 't':
                        return lit_scanner_checkkeyword(scanner, 2, 4, "atic", LTOKEN_STATIC);
                }
            }
            break;
        }
        case 't':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'h':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "is", LTOKEN_THIS);
                    case 'r':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "ue", LTOKEN_TRUE);
                }
            }
            break;
        }
        case 'v':
            return lit_scanner_checkkeyword(scanner, 1, 2, "ar", LTOKEN_VAR);
        case 'w':
            return lit_scanner_checkkeyword(scanner, 1, 4, "hile", LTOKEN_WHILE);
    }
    return LTOKEN_IDENTIFIER;
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
        LitToken token = lit_scanner_maketoken(scanner, LTOKEN_NEW_LINE);
        scanner->line++;
        return token;
    }
    scanner->start = scanner->current;
    if(lit_scanner_isatend(scanner))
    {
        return lit_scanner_maketoken(scanner, LTOKEN_EOF);
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
            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_PAREN);
        case ')':
            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_PAREN);
        case '{':
        {
            if(scanner->num_braces > 0)
            {
                scanner->braces[scanner->num_braces - 1]++;
            }
            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_BRACE);
        }
        case '}':
        {
            if(scanner->num_braces > 0 && --scanner->braces[scanner->num_braces - 1] == 0)
            {
                scanner->num_braces--;
                return lit_scanner_scanstring(scanner, true, true, '`');
            }
            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_BRACE);
        }
        case '[':
            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_BRACKET);
        case ']':
            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_BRACKET);
        case ';':
            return lit_scanner_maketoken(scanner, LTOKEN_SEMICOLON);
        case ',':
            return lit_scanner_maketoken(scanner, LTOKEN_COMMA);
        case ':':
            return lit_scanner_maketoken(scanner, LTOKEN_COLON);
        case '~':
            return lit_scanner_maketoken(scanner, LTOKEN_TILDE);
        case '+':
            return lit_scanner_matchtokens(scanner, '=', '+', LTOKEN_PLUS_EQUAL, LTOKEN_PLUS_PLUS, LTOKEN_PLUS);
        case '-':
            return lit_scanner_match(scanner, '>') ? lit_scanner_maketoken(scanner, LTOKEN_SMALL_ARROW) : lit_scanner_matchtokens(scanner, '=', '-', LTOKEN_MINUS_EQUAL, LTOKEN_MINUS_MINUS, LTOKEN_MINUS);
        case '/':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_SLASH_EQUAL, LTOKEN_SLASH);
        case '#':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_SHARP_EQUAL, LTOKEN_SHARP);
        case '!':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_BANG_EQUAL, LTOKEN_BANG);
        case '?':
            return lit_scanner_matchtoken(scanner, '?', LTOKEN_QUESTION_QUESTION, LTOKEN_QUESTION);
        case '%':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_PERCENT_EQUAL, LTOKEN_PERCENT);
        case '^':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_CARET_EQUAL, LTOKEN_CARET);
        case '>':
            return lit_scanner_matchtokens(scanner, '=', '>', LTOKEN_GREATER_EQUAL, LTOKEN_GREATER_GREATER, LTOKEN_GREATER);
        case '<':
            return lit_scanner_matchtokens(scanner, '=', '<', LTOKEN_LESS_EQUAL, LTOKEN_LESS_LESS, LTOKEN_LESS);
        case '*':
            return lit_scanner_matchtokens(scanner, '=', '*', LTOKEN_STAR_EQUAL, LTOKEN_STAR_STAR, LTOKEN_STAR);
        case '=':
            return lit_scanner_matchtokens(scanner, '=', '>', LTOKEN_EQUAL_EQUAL, LTOKEN_ARROW, LTOKEN_EQUAL);
        case '|':
            return lit_scanner_matchtokens(scanner, '=', '|', LTOKEN_BAR_EQUAL, LTOKEN_BAR_BAR, LTOKEN_BAR);
        case '&':
            return lit_scanner_matchtokens(scanner, '=', '&', LTOKEN_AMPERSAND_EQUAL, LTOKEN_AMPERSAND_AMPERSAND, LTOKEN_AMPERSAND);
        case '.':
        {
            if(!lit_scanner_match(scanner, '.'))
            {
                return lit_scanner_maketoken(scanner, LTOKEN_DOT);
            }
            return lit_scanner_matchtoken(scanner, '.', LTOKEN_DOT_DOT_DOT, LTOKEN_DOT_DOT);
        }
        case '`':
        {
            return lit_scanner_scanstring(scanner, true, true, '`');
        }
        case '"':
            return lit_scanner_scanstring(scanner, false, true, '"');
    }
    printf("%s\n", scanner->current);
    return lit_scanner_makeerrortoken(scanner, "Unexpected character '%c'", c);
}

static jmp_buf jumpbuffer;
static LitParseRule rules[LTOKEN_EOF + 1];
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
    return parser->current.type == LTOKEN_EOF;
}

void lit_parser_init(LitState* state, LitParser* parser)
{
    if(!didsetuprules)
    {
        didsetuprules = true;
        lit_parser_setuprules();
    }
    parser->state = state;
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
    lit_state_raiseerror(parser->state, COMPILE_ERROR, message);
    parser->had_error = true;
    lit_parser_sync(parser);
}

void lit_parser_failatv(LitParser* parser, LitToken* token, const char* fmt, va_list args)
{
    lit_parser_failactual(parser, token, lit_state_errorfmtv(parser->state, token->line, fmt, args)->chars);
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
        parser->current = lit_scanner_scantoken(parser->state->scanner);
        if(parser->current.type != LTOKEN_ERROR)
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
    if(parser->current.type == LTOKEN_IDENTIFIER && memcmp(parser->previous.start, type, fmax(strlen(type), parser->previous.length)))
    {
        lit_parser_advance(parser);
        return true;
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
    bool line = parser->previous.type == LTOKEN_NEW_LINE;
    lit_parser_failactual(parser, &parser->current,
                 lit_state_errorfmt(parser->state, parser->current.line, "Expected %s, got '%.*s'", error, line ? 8 : parser->previous.length,
                                  line ? "new line" : parser->previous.start)
                 ->chars);
}

bool lit_parser_matchlinefeed(LitParser* parser)
{
    if(!lit_parser_match(parser, LTOKEN_NEW_LINE))
    {
        return false;
    }
    while(lit_parser_match(parser, LTOKEN_NEW_LINE))
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
    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE) && !lit_parser_check(parser, LTOKEN_EOF))
    {
        lit_exprlist_push(&statement->statements, lit_parser_parsestmt(parser));
        lit_parser_ignorelinefeeds(parser);
    }
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}'");
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
        // todo: file start
        bool prevnewline = previous.start != NULL && *previous.start == '\n';
        bool parserprevnewline = parser->previous.start != NULL && *parser->previous.start == '\n';
        lit_parser_failfmt(parser, "Expected expression after '%.*s', got '%.*s'", prevnewline ? 8 : previous.length, prevnewline ? "new line" : previous.start,
              parserprevnewline ? 8 : parser->previous.length, parserprevnewline ? "new line" : parser->previous.start);
        return NULL;
    }
    bool canassign = precedence <= PREC_ASSIGNMENT;
    LitExpression* expr = prefix_rule(parser, canassign);
    lit_parser_ignorelinefeeds(parser);
    while(precedence <= lit_parser_getrule(parser->current.type)->precedence)
    {
        lit_parser_advance(parser);
        LitInfixParseFn infix_rule = lit_parser_getrule(parser->previous.type)->infix;
        expr = infix_rule(parser, expr, canassign);
    }
    if(err && canassign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        lit_parser_failfmt(parser, "Invalid assigment target");
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

void lit_parser_parseparams(LitParser* parser, LitDynListParam* parameters)
{
    bool haddefault = false;
    while(!lit_parser_check(parser, LTOKEN_RIGHT_PAREN))
    {
        // Vararg ...
        if(lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
        {
            lit_paramlist_push(parameters, (LitParameter){ "...", 3, 0, NULL });
            return;
        }
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "argument name");
        const char* argname = parser->previous.start;
        LitUInt arglength = parser->previous.length;
        LitExpression* default_value = NULL;
        if(lit_parser_match(parser, LTOKEN_EQUAL))
        {
            haddefault = true;
            default_value = lit_parser_parseexpr(parser);
        }
        else if(haddefault)
        {
            lit_parser_failfmt(parser, "Default arguments must always be in the end of the argument list.");
        }
        lit_paramlist_push(parameters, (LitParameter){ argname, arglength, 0, default_value });
        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }
}

LitExpression* lit_parser_rulegroupingorlambda(LitParser* parser, bool canassign)
{
    (void)canassign;
    if(lit_parser_match(parser, LTOKEN_RIGHT_PAREN))
    {
        lit_parser_consume(parser, LTOKEN_ARROW, "=> after lambda arguments");
        return lit_parser_parselambda(parser, lit_ast_makelambdaexpr(parser->previous.line));
    }
    const char* start = parser->previous.start;
    LitUInt line = parser->previous.line;
    if(lit_parser_match(parser, LTOKEN_IDENTIFIER) || lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
    {
        LitState* state = parser->state;
        const char* firstargstart = parser->previous.start;
        LitUInt firstarglength = parser->previous.length;
        if(lit_parser_match(parser, LTOKEN_COMMA) || (lit_parser_match(parser, LTOKEN_RIGHT_PAREN) && lit_parser_match(parser, LTOKEN_ARROW)))
        {
            bool hadarrow = parser->previous.type == LTOKEN_ARROW;
            bool hadvararg = parser->previous.type == LTOKEN_DOT_DOT_DOT;
            // This is a lambda
            LitFunctionStatement* lambda = lit_ast_makelambdaexpr(line);
            LitExpression* defvalue = NULL;
            bool haddefault = lit_parser_match(parser, LTOKEN_EQUAL);
            if(haddefault)
            {
                defvalue = lit_parser_parseexpr(parser);
            }
            lit_paramlist_push(&lambda->parameters, (LitParameter){ firstargstart, firstarglength, 0, defvalue });
            if(!hadvararg && parser->previous.type == LTOKEN_COMMA)
            {
                do
                {
                    bool stop = false;
                    if(lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
                    {
                        stop = true;
                    }
                    else
                    {
                        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "argument name");
                    }
                    const char* argname = parser->previous.start;
                    LitUInt arglength = parser->previous.length;
                    LitExpression* default_value = NULL;
                    if(lit_parser_match(parser, LTOKEN_EQUAL))
                    {
                        default_value = lit_parser_parseexpr(parser);
                        haddefault = true;
                    }
                    else if(haddefault)
                    {
                        lit_parser_failfmt(parser, "Default arguments must always be in the end of the argument list.");
                    }
                    lit_paramlist_push(&lambda->parameters, (LitParameter){ argname, arglength, 0, default_value });
                    if(stop)
                    {
                        break;
                    }
                } while(lit_parser_match(parser, LTOKEN_COMMA));
            }
            if(!hadarrow)
            {
                lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after lambda parameters");
                lit_parser_consume(parser, LTOKEN_ARROW, "=> after lambda parameters");
            }
            return lit_parser_parselambda(parser, lambda);
        }
        else
        {
            // Ouch, this was a grouping with a single identifier
            LitScanner* scanner = state->scanner;
            scanner->current = start;
            scanner->line = line;
            parser->current = lit_scanner_scantoken(scanner);
            lit_parser_advance(parser);
        }
    }
    LitExpression* expression = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after grouping expression");
    return expression;
}

LitExpression* lit_parser_parsecall(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitCallExpression* expression = lit_ast_makecallexpr(parser->previous.line, prev);
    while(!lit_parser_check(parser, LTOKEN_RIGHT_PAREN))
    {
        LitExpression* e = lit_parser_parseexpr(parser);
        lit_exprlist_push(&expression->args, e);
        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
        if(e->type == LIT_EXPR_VAR)
        {
            LitVarExpression* ee = (LitVarExpression*)e;
            // Vararg ...
            if(ee->length == 3 && memcmp(ee->name, "...", 3) == 0)
            {
                break;
            }
        }
    }
    if(expression->args.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)expression->args.count);
    }
    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after arguments");
    return (LitExpression*)expression;
}

LitExpression* lit_parser_ruleunary(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    LitExpression* expression = lit_parser_parseprec(parser, PREC_UNARY, true);
    return (LitExpression*)lit_ast_makeunaryexpr(line, expression, op);
}

LitExpression* lit_parser_rulebinary(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    bool invert = parser->previous.type == LTOKEN_BANG;
    if(invert)
    {
        lit_parser_consume(parser, LTOKEN_IS, "'is' after '!'");
    }
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);
    expression = (LitExpression*)lit_ast_makebinaryexpr(line, prev, expression, op);
    if(invert)
    {
        expression = (LitExpression*)lit_ast_makeunaryexpr(line, expression, LTOKEN_BANG);
    }
    return expression;
}

LitExpression* lit_parser_rulelogicaland(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, PREC_AND, true), op);
}

LitExpression* lit_parser_rulelogicalor(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, PREC_OR, true), op);
}

LitExpression* lit_parser_rulenullfilter(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    return (LitExpression*)lit_ast_makebinaryexpr(line, prev, lit_parser_parseprec(parser, PREC_NULL, true), op);
}

LitTokenType lit_parser_convertcompoundop(LitTokenType op)
{
    switch(op)
    {
        case LTOKEN_PLUS_EQUAL:
            return LTOKEN_PLUS;
        case LTOKEN_MINUS_EQUAL:
            return LTOKEN_MINUS;
        case LTOKEN_STAR_EQUAL:
            return LTOKEN_STAR;
        case LTOKEN_SLASH_EQUAL:
            return LTOKEN_SLASH;
        case LTOKEN_SHARP_EQUAL:
            return LTOKEN_SHARP;
        case LTOKEN_PERCENT_EQUAL:
            return LTOKEN_PERCENT;
        case LTOKEN_CARET_EQUAL:
            return LTOKEN_CARET;
        case LTOKEN_BAR_EQUAL:
            return LTOKEN_BAR;
        case LTOKEN_AMPERSAND_EQUAL:
            return LTOKEN_AMPERSAND;
        case LTOKEN_PLUS_PLUS:
            return LTOKEN_PLUS;
        case LTOKEN_MINUS_MINUS:
            return LTOKEN_MINUS;
        default:
        {
            UNREACHABLE
        }
    }
    return LTOKEN_EOF;
}

LitExpression* lit_parser_rulecompound(LitParser* parser, LitExpression* prev, bool canassign)
{
    (void)canassign;
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression;
    if(op == LTOKEN_PLUS_PLUS || op == LTOKEN_MINUS_MINUS)
    {
        expression = (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makenumber(1));
    }
    else
    {
        expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);
    }
    LitBinaryExpression* binary = lit_ast_makebinaryexpr(line, prev, expression, lit_parser_convertcompoundop(op));
    binary->ignore_left = true;// To make sure we don't free it twice
    return (LitExpression*)lit_ast_makeassignexpr(line, prev, (LitExpression*)binary);
}

LitExpression* lit_parser_ruleliteral(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitUInt line = parser->previous.line;
    switch(parser->previous.type)
    {
        case LTOKEN_TRUE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(true));
        }
        case LTOKEN_FALSE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(line, lit_value_makebool(false));
        }
        case LTOKEN_NULL:
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
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    return expression;
}

LitExpression* lit_parser_ruleinterpolation(LitParser* parser, bool canassign)
{
    LitInterpolationExpression* expression = lit_ast_makeinterpolationexpr(parser->previous.line);
    do
    {
        if(AS_STRING(parser->previous.value)->length > 0)
        {
            lit_exprlist_push(&expression->expressions,
                                  (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value));
        }
        lit_exprlist_push(&expression->expressions, lit_parser_parseexpr(parser));
    } while(lit_parser_match(parser, LTOKEN_INTERPOLATION));
    lit_parser_consume(parser, LTOKEN_STRING, "end of interpolation");
    if(AS_STRING(parser->previous.value)->length > 0)
    {
        lit_exprlist_push(&expression->expressions,
                              (LitExpression*)lit_ast_makeliteralexpr(parser->previous.line, parser->previous.value));
    }
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
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
    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "key string after '{'");
        lit_vallist_push(&object->keys, lit_value_fromobject(lit_string_copy(parser->state, parser->previous.start, parser->previous.length)));
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_COLON, "':' after key string");
        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(&object->values, lit_parser_parseexpr(parser));
        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after object");
    return (LitExpression*)object;
}

LitExpression* lit_parser_parsevarexprbase(LitParser* parser, bool canassign, bool isnew)
{
    LitExpression* expression = (LitExpression*)lit_ast_makevarexpr(parser->previous.line, parser->previous.start, parser->previous.length);
    if(isnew)
    {
        bool hadargs = lit_parser_check(parser, LTOKEN_LEFT_PAREN);
        LitCallExpression* call = NULL;
        if(hadargs)
        {
            lit_parser_advance(parser);
            call = (LitCallExpression*)lit_parser_parsecall(parser, expression, false);
        }
        if(lit_parser_match(parser, LTOKEN_LEFT_BRACE))
        {
            if(call == NULL)
            {
                call = lit_ast_makecallexpr(expression->line, expression);
            }
            call->init = lit_parser_ruleobject(parser, false);
        }
        else if(!hadargs)
        {
            lit_parser_failherefmt(parser, "Expected %s, got '%.*s'", "argument list for instance creation", parser->previous.length, parser->previous.start);
        }
        return (LitExpression*)call;
    }
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    if(canassign && lit_parser_match(parser, LTOKEN_EQUAL))
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
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "class name after 'new'");
    return lit_parser_parsevarexprbase(parser, false, true);
}

LitExpression* lit_parser_ruledot(LitParser* parser, LitExpression* previous, bool canassign)
{
    LitUInt line = parser->previous.line;
    bool ignored = parser->previous.type == LTOKEN_SMALL_ARROW;
    if(!(lit_parser_match(parser, LTOKEN_CLASS) || lit_parser_match(parser, LTOKEN_SUPER)))
    {// class and super are allowed field names
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignored ? "propety name after '->'" : "property name after '.'");
    }
    const char* name = parser->previous.start;
    LitUInt length = parser->previous.length;
    if(!ignored && canassign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makesetexpr(line, previous, name, length, lit_parser_parseexpr(parser));
    }
    else
    {
        LitExpression* expression = (LitExpression*)lit_ast_makegetexpr(line, previous, name, length, false, ignored);
        if(!ignored && lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
        {
            return lit_parser_parsesubscript(parser, expression, canassign);
        }
        return expression;
    }
}

LitExpression* lit_parser_rulerange(LitParser* parser, LitExpression* previous, bool canassign)
{
    (void)canassign;
    LitUInt line = parser->previous.line;
    return (LitExpression*)lit_ast_makerangeexpr(line, previous, lit_parser_parseexpr(parser));
}

LitExpression* lit_parser_ruleternaryorquestion(LitParser* parser, LitExpression* previous, bool canassign)
{
    (void)canassign;
    LitUInt line = parser->previous.line;
    if(lit_parser_match(parser, LTOKEN_DOT) /* || lit_parser_match(parser, LTOKEN_SMALL_ARROW)*/)
    {
        bool ignored = parser->previous.type == LTOKEN_SMALL_ARROW;
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
        return (LitExpression*)lit_ast_makegetexpr(line, previous, parser->previous.start, parser->previous.length, true, ignored);
    }
    LitExpression* if_branch = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_COLON, "':' after expression");
    LitExpression* else_branch = lit_parser_parseexpr(parser);
    return (LitExpression*)lit_ast_maketernaryexpr(line, previous, if_branch, else_branch);
}

LitExpression* lit_parser_rulearray(LitParser* parser, bool canassign)
{
    LitArrayExpression* array = lit_ast_makearrayexpr(parser->previous.line);
    lit_parser_ignorelinefeeds(parser);
    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACKET))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(&array->values, lit_parser_parseexpr(parser));
        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after array");
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, (LitExpression*)array, canassign);
    }
    return (LitExpression*)array;
}

LitExpression* lit_parser_parsesubscript(LitParser* parser, LitExpression* previous, bool canassign)
{
    LitUInt line = parser->previous.line;
    LitExpression* index = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after subscript");
    LitExpression* expression = (LitExpression*)lit_ast_makesubscriptexpr(line, previous, index);
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    else if(canassign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->previous.line, expression, lit_parser_parseexpr(parser));
    }
    return expression;
}

LitExpression* lit_parser_rulethis(LitParser* parser, bool canassign)
{
    LitExpression* expression = (LitExpression*)lit_ast_makethisexpr(parser->previous.line);
    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, canassign);
    }
    return expression;
}

LitExpression* lit_parser_rulesuper(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitUInt line = parser->previous.line;
    if(!(lit_parser_match(parser, LTOKEN_DOT) || lit_parser_match(parser, LTOKEN_SMALL_ARROW)))
    {
        LitExpression* expression = (LitExpression*)lit_ast_makesuperexpr(line, lit_string_copy(parser->state, "constructor", 11), false);
        lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after 'super'");
        return lit_parser_parsecall(parser, expression, false);
    }
    bool ignoring = parser->previous.type == LTOKEN_SMALL_ARROW;
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignoring ? "super method name after '->'" : "super method name after '.'");
    LitExpression* expression
    = (LitExpression*)lit_ast_makesuperexpr(line, lit_string_copy(parser->state, parser->previous.start, parser->previous.length), ignoring);
    if(lit_parser_match(parser, LTOKEN_LEFT_PAREN))
    {
        return lit_parser_parsecall(parser, expression, false);
    }
    return expression;
}


LitExpression *lit_parser_rulenothing(LitParser *parser, bool canassign)
{
    (void)parser;
    (void)canassign;
    return NULL;
}


LitExpression *lit_parser_rulefunction(LitParser *parser, bool canassign)
{
    (void)canassign;
    return lit_parser_parsefunction(parser);
}


LitExpression* lit_parser_rulereference(LitParser* parser, bool canassign)
{
    (void)canassign;
    LitUInt line = parser->previous.line;
    lit_parser_ignorelinefeeds(parser);
    LitReferenceExpression* expression = lit_ast_makerefexpr(line, lit_parser_parseprec(parser, PREC_CALL, false));
    if(lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(line, (LitExpression*)expression, lit_parser_parseexpr(parser));
    }
    return (LitExpression*)expression;
}

LitExpression* lit_parser_parseexpr(LitParser* parser)
{
    lit_parser_ignorelinefeeds(parser);
    return lit_parser_parseprec(parser, PREC_ASSIGNMENT, true);
}

LitExpression* lit_parser_parsevardecl(LitParser* parser)
{
    bool constant = parser->previous.type == LTOKEN_CONST;
    LitUInt line = parser->previous.line;
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "variable name");
    const char* name = parser->previous.start;
    LitUInt length = parser->previous.length;
    LitExpression* init = NULL;
    if(lit_parser_match(parser, LTOKEN_EQUAL))
    {
        init = lit_parser_parseexpr(parser);
    }
    return (LitExpression*)lit_ast_makevardefstmt(line, name, length, init, constant);
}

LitExpression* lit_parser_parseif(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    bool invert = lit_parser_match(parser, LTOKEN_BANG);
    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);
    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }
    if(invert)
    {
        condition = (LitExpression*)lit_ast_makeunaryexpr(condition->line, condition, LTOKEN_BANG);
    }
    lit_parser_ignorelinefeeds(parser);
    LitExpression* if_branch = lit_parser_parsestmt(parser);
    LitDynListExpr* elseif_conditions = NULL;
    LitDynListExpr* elseif_branches = NULL;
    LitExpression* else_branch = NULL;
    lit_parser_ignorelinefeeds(parser);
    while(lit_parser_match(parser, LTOKEN_ELSE))
    {
        // else if
        if(lit_parser_match(parser, LTOKEN_IF))
        {
            if(elseif_conditions == NULL)
            {
                elseif_conditions = lit_ast_allocexprlist();
                elseif_branches = lit_ast_allocstmtlist();
            }
            invert = lit_parser_match(parser, LTOKEN_BANG);
            hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
            LitExpression* e = lit_parser_parseexpr(parser);
            if(hadparen)
            {
                lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
            }
            if(invert)
            {
                e = (LitExpression*)lit_ast_makeunaryexpr(condition->line, e, LTOKEN_BANG);
            }
            lit_exprlist_push(elseif_conditions, e);
            lit_parser_ignorelinefeeds(parser);
            lit_exprlist_push(elseif_branches, lit_parser_parsestmt(parser));
            lit_parser_ignorelinefeeds(parser);
            continue;
        }
        // else
        if(else_branch != NULL)
        {
            lit_parser_failfmt(parser, "If-statement can have only one else-branch");
        }
        else_branch = lit_parser_parsestmt(parser);
    }
    return (LitExpression*)lit_ast_makeifstatement(line, condition, if_branch, else_branch, elseif_conditions, elseif_branches);
}

LitExpression* lit_parser_parsefor(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
    LitExpression* var = NULL;
    LitExpression* init = NULL;
    if(!lit_parser_check(parser, LTOKEN_SEMICOLON))
    {
        if(lit_parser_match(parser, LTOKEN_VAR))
        {
            var = lit_parser_parsevardecl(parser);
        }
        else
        {
            init = lit_parser_parseexpr(parser);
        }
    }
    bool c_style = !lit_parser_match(parser, LTOKEN_IN);
    LitExpression* condition = NULL;
    LitExpression* increment = NULL;
    if(c_style)
    {
        lit_parser_consume(parser, LTOKEN_SEMICOLON, "';'");
        condition = lit_parser_check(parser, LTOKEN_SEMICOLON) ? NULL : lit_parser_parseexpr(parser);
        lit_parser_consume(parser, LTOKEN_SEMICOLON, "';'");
        increment = lit_parser_check(parser, LTOKEN_RIGHT_PAREN) ? NULL : lit_parser_parseexpr(parser);
    }
    else
    {
        condition = lit_parser_parseexpr(parser);
        if(var == NULL)
        {
            lit_parser_failfmt(parser, "For-loops using in-iteration must declare a new variable");
        }
    }
    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }
    return (LitExpression*)lit_ast_makeforstmt(line, init, var, condition, increment, lit_parser_parsestmt(parser), c_style);
}

LitExpression* lit_parser_parsewhile(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);
    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }
    LitExpression* body = lit_parser_parsestmt(parser);
    return (LitExpression*)lit_ast_makewhilestmt(line, condition, body);
}

LitExpression* lit_parser_parsefunction(LitParser* parser)
{
    LitUInt line;
    LitUInt namelen;
    bool isexport;
    bool noname;
    const char* fnname;
    noname = false;
    fnname = "anonymous";
    namelen = strlen(fnname);
    isexport = parser->previous.type == LTOKEN_EXPORT;
    if(isexport)
    {
        lit_parser_consume(parser, LTOKEN_FUNCTION, "'function' after 'export'");
    }
    line = parser->previous.line;
    if(lit_parser_check(parser, LTOKEN_IDENTIFIER))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "function name");
        fnname = parser->previous.start;
        namelen = parser->previous.length;
    }
    else
    {
        noname = true;
    }
    if(lit_parser_match(parser, LTOKEN_DOT))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "function name");
        LitFunctionStatement* lambda = lit_ast_makelambdaexpr(line);
        LitSetExpression* to = lit_ast_makesetexpr(line, (LitExpression*)lit_ast_makevarexpr(line, fnname, namelen),
                                                         parser->previous.start, parser->previous.length, (LitExpression*)lambda);
        lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after function name");
        LitCompiler compiler;
        lit_parser_compilerinit(parser, &compiler);
        lit_parser_scopebegin(parser);
        lit_parser_parseparams(parser, &lambda->parameters);
        if(lambda->parameters.count > 255)
        {
            lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)lambda->parameters.count);
        }
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after function arguments");
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
    lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after function name");
    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);
    lit_parser_parseparams(parser, &function->parameters);
    if(function->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)function->parameters.count);
    }
    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after function arguments");
    function->body = lit_parser_parsestmt(parser);
    lit_parser_scopeend(parser);
    lit_parser_compilerend(parser, &compiler);
    return (LitExpression*)function;
}

LitExpression* lit_parser_parsereturn(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    LitExpression* expression = NULL;
    if(!lit_parser_check(parser, LTOKEN_NEW_LINE) && !lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        expression = lit_parser_parseexpr(parser);
    }
    return (LitExpression*)lit_ast_makereturnstmt(line, expression);
}


LitExpression* lit_parser_parsefield(LitParser* parser, LitString* name, bool is_static)
{
    LitUInt line = parser->previous.line;
    LitExpression* getter = NULL;
    LitExpression* setter = NULL;
    if(lit_parser_match(parser, LTOKEN_ARROW))
    {
        getter = lit_parser_parsestmt(parser);
    }
    else
    {
        lit_parser_match(parser, LTOKEN_LEFT_BRACE);// Will be LTOKEN_LEFT_BRACE, otherwise this method won't be called
        lit_parser_ignorelinefeeds(parser);
        if(lit_parser_matchident(parser, "get"))
        {
            lit_parser_match(parser, LTOKEN_ARROW);// Ignore it if it's present
            getter = lit_parser_parsestmt(parser);
        }
        lit_parser_ignorelinefeeds(parser);
        if(lit_parser_matchident(parser, "set"))
        {
            lit_parser_match(parser, LTOKEN_ARROW);// Ignore it if it's present
            setter = lit_parser_parsestmt(parser);
        }
        if(getter == NULL && setter == NULL)
        {
            lit_parser_failfmt(parser, "Expected declaration of either getter or setter, got none");
        }
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after field declaration");
    }
    return (LitExpression*)lit_ast_makefieldstmt(line, name, getter, setter, is_static);
}

static LitTokenType operators[] = { LTOKEN_PLUS,         LTOKEN_MINUS, LTOKEN_STAR,       LTOKEN_PERCENT, LTOKEN_SLASH,         LTOKEN_SHARP,

                                    LTOKEN_BANG,         LTOKEN_LESS,  LTOKEN_LESS_EQUAL, LTOKEN_GREATER, LTOKEN_GREATER_EQUAL, LTOKEN_EQUAL_EQUAL,

                                    LTOKEN_LEFT_BRACKET,

                                    LTOKEN_EOF };

LitExpression* lit_parser_parsemethod(LitParser* parser, bool is_static)
{
    if(lit_parser_match(parser, LTOKEN_STATIC))
    {
        is_static = true;
    }
    LitString* name = NULL;
    if(lit_parser_match(parser, LTOKEN_OPERATOR))
    {
        if(is_static)
        {
            lit_parser_failfmt(parser, "Operator methods can't be static or defined in static classes");
        }
        LitUInt i = 0;
        while(operators[i] != LTOKEN_EOF)
        {
            if(lit_parser_match(parser, operators[i]))
            {
                break;
            }
            i++;
        }
        if(parser->previous.type == LTOKEN_LEFT_BRACKET)
        {
            lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after '[' in op method declaration");
            name = lit_string_copy(parser->state, "[]", 2);
        }
        else
        {
            name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
        }
    }
    else
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "method name");
        name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
        if(lit_parser_check(parser, LTOKEN_LEFT_BRACE) || lit_parser_check(parser, LTOKEN_ARROW))
        {
            return lit_parser_parsefield(parser, name, is_static);
        }
    }
    LitMethodStatement* method = lit_ast_makemethoddefstmt(parser->previous.line, name, is_static);
    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);
    lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after method name");
    lit_parser_parseparams(parser, &method->parameters);
    if(method->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)method->parameters.count);
    }
    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after method arguments");
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
    LitUInt line = parser->previous.line;
    bool is_static = parser->previous.type == LTOKEN_STATIC;
    if(is_static)
    {
        lit_parser_consume(parser, LTOKEN_CLASS, "'class' after 'static'");
    }
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "class name after 'class'");
    LitString* name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
    LitString* super = NULL;
    if(lit_parser_match(parser, LTOKEN_COLON))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "super class name after ':'");
        super = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
        if(super == name)
        {
            lit_parser_failfmt(parser, "Class can't inherit itself");
        }
    }
    LitClassStatement* klass = lit_ast_makeclassdefstmt(line, name, super);
    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_LEFT_BRACE, "'{' before class body");
    lit_parser_ignorelinefeeds(parser);
    bool finishedparsingfields = false;
    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        bool fieldisstatic = false;
        if(lit_parser_match(parser, LTOKEN_STATIC))
        {
            fieldisstatic = true;
            if(lit_parser_match(parser, LTOKEN_VAR))
            {
                if(finishedparsingfields)
                {
                    lit_parser_failfmt(parser, "All static fields must be defined before the methods");
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
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after class body");
    return (LitExpression*)klass;
}

void lit_parser_sync(LitParser* parser)
{
    parser->panic_mode = false;
    while(parser->current.type != LTOKEN_EOF)
    {
        if(parser->previous.type == LTOKEN_NEW_LINE)
        {
            longjmp(jumpbuffer, 1);
            return;
        }
        switch(parser->current.type)
        {
            case LTOKEN_CLASS:
            case LTOKEN_FUNCTION:
            case LTOKEN_EXPORT:
            case LTOKEN_VAR:
            case LTOKEN_CONST:
            case LTOKEN_FOR:
            case LTOKEN_STATIC:
            case LTOKEN_IF:
            case LTOKEN_WHILE:
            case LTOKEN_RETURN:
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
    if(lit_parser_match(parser, LTOKEN_VAR) || lit_parser_match(parser, LTOKEN_CONST))
    {
        return lit_parser_parsevardecl(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_IF))
    {
        return lit_parser_parseif(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_FOR))
    {
        return lit_parser_parsefor(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_WHILE))
    {
        return lit_parser_parsewhile(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_CONTINUE))
    {
        return (LitExpression*)lit_ast_makecontinuestmt(parser->previous.line);
    }
    else if(lit_parser_match(parser, LTOKEN_BREAK))
    {
        return (LitExpression*)lit_ast_makebreakstmt(parser->previous.line);
    }
    else if(lit_parser_match(parser, LTOKEN_FUNCTION) || lit_parser_match(parser, LTOKEN_EXPORT))
    {
        return lit_parser_parsefunction(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_RETURN))
    {
        return lit_parser_parsereturn(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_LEFT_BRACE))
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
    if(lit_parser_match(parser, LTOKEN_CLASS) || lit_parser_match(parser, LTOKEN_STATIC))
    {
        statement = lit_parser_parseclass(parser);
    }
    else
    {
        statement = lit_parser_parsestmt(parser);
    }
    return statement;
}

bool lit_parser_parsesource(LitParser* parser, const char* file_name, const char* source, LitDynListExpr* statements)
{
    parser->had_error = false;
    parser->panic_mode = false;
    lit_scanner_init(parser->state, parser->state->scanner, file_name, source);
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
                if(lit_parser_match(parser, LTOKEN_EOF))
                {
                    break;
                }
            }
        } while(!lit_parser_isatend(parser));
    }
    return parser->had_error || parser->state->scanner->had_error;
}

void lit_parser_setuprules()
{
    rules[LTOKEN_LEFT_PAREN] = (LitParseRule){ lit_parser_rulegroupingorlambda, lit_parser_parsecall, PREC_CALL };
    rules[LTOKEN_PLUS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_MINUS] = (LitParseRule){ lit_parser_ruleunary, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_BANG] = (LitParseRule){ lit_parser_ruleunary, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_SLASH] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_SHARP] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_BAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BOR };
    rules[LTOKEN_AMPERSAND] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BAND };
    rules[LTOKEN_TILDE] = (LitParseRule){ lit_parser_ruleunary, NULL, PREC_UNARY };
    rules[LTOKEN_CARET] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BOR };
    rules[LTOKEN_LESS_LESS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_SHIFT };
    rules[LTOKEN_GREATER_GREATER] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_SHIFT };
    rules[LTOKEN_PERCENT] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_IS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_IS };
    rules[LTOKEN_NUMBER] = (LitParseRule){ lit_parser_rulenumber, NULL, PREC_NONE };
    rules[LTOKEN_TRUE] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_FALSE] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_NULL] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_BANG_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_EQUALITY };
    rules[LTOKEN_EQUAL_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_EQUALITY };
    rules[LTOKEN_GREATER] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_GREATER_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_LESS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_LESS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_STRING] = (LitParseRule){ lit_parser_rulestring, NULL, PREC_NONE };
    rules[LTOKEN_INTERPOLATION] = (LitParseRule){ lit_parser_ruleinterpolation, NULL, PREC_NONE };
    rules[LTOKEN_IDENTIFIER] = (LitParseRule){ lit_parser_rulevarexpr, NULL, PREC_NONE };
    rules[LTOKEN_NEW] = (LitParseRule){ lit_parser_rulenewexpr, NULL, PREC_NONE };
    rules[LTOKEN_PLUS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_MINUS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_STAR_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_SLASH_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_SHARP_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_PERCENT_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_CARET_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_BAR_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_AMPERSAND_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_PLUS_PLUS] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_MINUS_MINUS] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_AMPERSAND_AMPERSAND] = (LitParseRule){ NULL, lit_parser_rulelogicaland, PREC_AND };
    rules[LTOKEN_BAR_BAR] = (LitParseRule){ NULL, lit_parser_rulelogicalor, PREC_AND };
    rules[LTOKEN_QUESTION_QUESTION] = (LitParseRule){ NULL, lit_parser_rulenullfilter, PREC_NULL };
    rules[LTOKEN_DOT] = (LitParseRule){ NULL, lit_parser_ruledot, PREC_CALL };
    // rules[LTOKEN_SMALL_ARROW] = (LitParseRule) { NULL, lit_parser_ruledot, PREC_CALL };
    rules[LTOKEN_DOT_DOT] = (LitParseRule){ NULL, lit_parser_rulerange, PREC_RANGE };
    rules[LTOKEN_DOT_DOT_DOT] = (LitParseRule){ lit_parser_rulevarexpr, NULL, PREC_ASSIGNMENT };
    rules[LTOKEN_LEFT_BRACKET] = (LitParseRule){ lit_parser_rulearray, lit_parser_parsesubscript, PREC_NONE };
    rules[LTOKEN_LEFT_BRACE] = (LitParseRule){ lit_parser_ruleobject, NULL, PREC_NONE };
    rules[LTOKEN_THIS] = (LitParseRule){ lit_parser_rulethis, NULL, PREC_NONE };
    rules[LTOKEN_SUPER] = (LitParseRule){ lit_parser_rulesuper, NULL, PREC_NONE };
    rules[LTOKEN_QUESTION] = (LitParseRule){ NULL, lit_parser_ruleternaryorquestion, PREC_EQUALITY };
    rules[LTOKEN_REF] = (LitParseRule){ lit_parser_rulereference, NULL, PREC_NONE };
    rules[LTOKEN_SEMICOLON] = (LitParseRule){lit_parser_rulenothing, NULL, PREC_NONE};
    rules[LTOKEN_FUNCTION] = (LitParseRule){lit_parser_rulefunction, NULL, PREC_NONE};
 
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
        LitUInt oldcapacity = array->capacity;
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

void lit_loclist_destroy(LitState* state, LitDynListLoc* array)
{
    LIT_FREE_ARRAY(state, LitLocal, array->values, array->capacity);
    lit_loclist_reset(array);
}
void lit_loclist_push(LitDynListLoc* array, LitLocal value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitLocal*)lit_sysmem_realloc(array->values, sizeof(LitLocal) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}


void lit_emitter_resolvestatements(LitEmitter* emitter, LitDynListExpr* statements)
{
    for(LitUInt i = 0; i < statements->count; i++)
    {
        resolve_statement(emitter, statements->values[i]);
    }
}

void lit_emitter_init(LitState* state, LitEmitter* emitter)
{
    lit_emitter_reset(state, emitter);
    lit_privlist_init(&emitter->privates);
    lit_uintlist_init(&emitter->breaks);
    lit_uintlist_init(&emitter->continues);
}

void lit_emitter_reset(LitState* state, LitEmitter* emitter)
{
    emitter->state = state;
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
    lit_uintlist_destroy(emitter->state, &emitter->breaks);
    lit_uintlist_destroy(emitter->state, &emitter->continues);
}

void lit_emitter_raiseerror(LitEmitter* emitter, LitUInt line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_state_raiseerror(emitter->state, COMPILE_ERROR, lit_state_errorfmtv(emitter->state, line, fmt, args)->chars);
    va_end(args);
}

LitUInt lit_emitter_emittmp(LitEmitter* emitter)
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

void lit_emitter_emitabx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, uint32_t bx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->chunk, LIT_FORM_ABX_INSTRUCTION(opcode, a, bx), emitter->last_line);
}

void lit_emitter_emitasbx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, int32_t sbx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->chunk, LIT_FORM_ASBX_INSTRUCTION(opcode, a, sbx), emitter->last_line);
}

// Be very careful with the use of this function, always reserve a register just before using it, do not wait around!
uint8_t lit_emitter_reserveregister(LitEmitter* emitter)
{
    LitCompiler* compiler = emitter->compiler;
    if(compiler->registers_used == LIT_REGISTERS_MAX)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "Too many registers required");
        return 0;
    }
    compiler->function->max_registers = fmax(compiler->function->max_registers, ++compiler->registers_used);
    return compiler->registers_used - 1;
}

void lit_emitter_freeregister(LitEmitter* emitter, uint16_t reg)
{
    if(IS_BIT_SET(reg, 9))
    {
        return;
    }
    LitCompiler* compiler = emitter->compiler;
    if(compiler->registers_used == 0)
    {
        return lit_emitter_raiseerror(emitter, emitter->last_line, "Invalid register was freed");
    }
    compiler->registers_used--;
}

void lit_emitter_compilerinit(LitEmitter* emitter, LitCompiler* compiler, LitFunctionType type)
{
    lit_loclist_init(&compiler->locals);
    compiler->type = type;
    compiler->scope_depth = -1;
    compiler->enclosing = (struct LitCompiler*)emitter->compiler;
    compiler->skip_return = false;
    compiler->function = lit_object_makefunction(emitter->state, emitter->module);
    compiler->loop_depth = 0;
    compiler->registers_used = 0;
    emitter->compiler = compiler;
    const char* name = emitter->state->scanner->file_name;
    if(emitter->compiler == NULL)
    {
        compiler->function->name = lit_string_copy(emitter->state, name, strlen(name));
    }
    emitter->chunk = &compiler->function->chunk;
    if(type == FUNCTION_METHOD || type == FUNCTION_STATIC_METHOD || type == FUNCTION_CONSTRUCTOR)
    {
        lit_loclist_push(&compiler->locals, (LitLocal){ "this", 4, -1, false, false, lit_emitter_reserveregister(emitter) });
    }
    else
    {
        lit_loclist_push(&compiler->locals, (LitLocal){ "", 0, -1, false, false, lit_emitter_reserveregister(emitter) });
    }
}

LitFunction* lit_emitter_compilerend(LitEmitter* emitter, LitString* name)
{
    lit_emitter_freeregister(emitter, 0);
    if(emitter->compiler->registers_used > 0)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "Not all registers were freed (%i left)", emitter->compiler->registers_used);
    }
    if(!emitter->compiler->skip_return)
    {
        uint8_t reg = lit_emitter_reserveregister(emitter);
        LitFunctionType type = emitter->compiler->type;
        if(type == FUNCTION_CONSTRUCTOR)
        {
            // Load this
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
    lit_loclist_destroy(emitter->state, &emitter->compiler->locals);
    emitter->compiler = (LitCompiler*)emitter->compiler->enclosing;
    emitter->chunk = emitter->compiler == NULL ? NULL : &emitter->compiler->function->chunk;
    if(name != NULL)
    {
        function->name = name;
    }
#ifdef LIT_TRACE_CHUNK
    if(!emitter->state->had_error)
    {
        lit_debug_disaschunk(&function->chunk, function->name->chars, NULL);
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
        lit_emitter_raiseerror(emitter, emitter->last_line, "Invalid scope ending");
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

uint16_t lit_emitter_addconst(LitEmitter* emitter, LitUInt line, LitValue value)
{
    LitUInt constant = lit_chunk_addconstant(emitter->state, emitter->chunk, value);
    if(constant >= UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "Too many constants for one chunk");
    }
    return constant;
}

int lit_emitter_addprivate(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line, bool constant)
{
    LitDynListPriv* privates = &emitter->privates;
    if(privates->count == UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "Too many private locals for one module");
    }
    LitTable* private_names = &emitter->module->private_names->values;
    LitString* key = lit_table_find_string(private_names, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        lit_emitter_raiseerror(emitter, line, "Variable '%.*s' was already declared in this scope", length, name);
        LitValue index;
        lit_table_get(private_names, key, &index);
        return lit_value_asnumber(index);
    }
    LitState* state = emitter->state;
    int index = (int)privates->count;
    lit_privlist_push(privates, (LitPrivate){ false, constant });
    lit_table_set(state, private_names, lit_string_copy(state, name, length), lit_value_makenumber(index));
    emitter->module->private_count++;
    return index;
}

int lit_emitter_resolveprivate(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line)
{
    LitTable* private_names = &emitter->module->private_names->values;
    LitString* key = lit_table_find_string(private_names, name, length, lit_string_hash(name, length));
    if(key != NULL)
    {
        LitValue index;
        lit_table_get(private_names, key, &index);
        int numberindex = lit_value_asnumber(index);
        if(!emitter->privates.values[numberindex].initialized)
        {
            lit_emitter_raiseerror(emitter, line, "Variable '%.*s' can't use itself in its initializer", length, name);
        }
        return numberindex;
    }
    return -1;
}

int lit_emitter_addlocal(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line, bool constant, uint8_t reg)
{
    LitCompiler* compiler = emitter->compiler;
    LitDynListLoc* locals = &compiler->locals;
    if(locals->count == UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "Too many local variables for one function");
    }
    for(int i = (int)locals->count - 1; i >= 0; i--)
    {
        LitLocal* local = &locals->values[i];
        if(local->depth != UINT16_MAX && local->depth < compiler->scope_depth)
        {
            break;
        }
        if(length == local->length && memcmp(local->name, name, length) == 0)
        {
            lit_emitter_raiseerror(emitter, line, "Variable '%.*s' was already declared in this scope", length, name);
        }
    }
    lit_loclist_push(locals, (LitLocal){ name, length, UINT16_MAX, false, constant, reg });
    return (int)locals->count - 1;
}

int lit_emitter_resolvelocal(LitEmitter* emitter, LitCompiler* compiler, const char* name, LitUInt length, LitUInt line)
{
    LitDynListLoc* locals = &compiler->locals;
    for(int i = (int)locals->count - 1; i >= 0; i--)
    {
        LitLocal* local = &locals->values[i];
        if(local->length == length && memcmp(local->name, name, length) == 0)
        {
            if(local->depth == UINT16_MAX)
            {
                lit_emitter_raiseerror(emitter, line, "Variable '%.*s' can't use itself in its initializer", length, name);
            }
            return i;
        }
    }
    return -1;
}

int lit_emitter_addupvalue(LitEmitter* emitter, LitCompiler* compiler, uint8_t index, LitUInt line, bool islocal)
{
    LitUInt upvaluecount = compiler->function->upvaluecount;
    for(LitUInt i = 0; i < upvaluecount; i++)
    {
        LitCompilerUpvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->isLocal == islocal)
        {
            return i;
        }
    }
    if(upvaluecount == UINT16_COUNT)
    {
        lit_emitter_raiseerror(emitter, line, "Too many upvalues for one function");
        return 0;
    }
    compiler->upvalues[upvaluecount].isLocal = islocal;
    compiler->upvalues[upvaluecount].index = index;
    return compiler->function->upvaluecount++;
}

int lit_emitter_resolveupvalue(LitEmitter* emitter, LitCompiler* compiler, const char* name, LitUInt length, LitUInt line)
{
    if(compiler->enclosing == NULL)
    {
        return -1;
    }
    int local = lit_emitter_resolvelocal(emitter, (LitCompiler*)compiler->enclosing, name, length, line);
    if(local != -1)
    {
        ((LitCompiler*)compiler->enclosing)->locals.values[local].captured = true;
        return lit_emitter_addupvalue(emitter, compiler, (uint8_t)local, line, true);
    }
    int upvalue = lit_emitter_resolveupvalue(emitter, (LitCompiler*)compiler->enclosing, name, length, line);
    if(upvalue != -1)
    {
        return lit_emitter_addupvalue(emitter, compiler, (uint8_t)upvalue, line, false);
    }
    return -1;
}

void lit_emitter_marklocalinit(LitEmitter* emitter, LitUInt index)
{
    emitter->compiler->locals.values[index].depth = emitter->compiler->scope_depth;
}

void lit_emitter_markprivateinit(LitEmitter* emitter, LitUInt index)
{
    emitter->privates.values[index].initialized = true;
}

void resolve_statement(LitEmitter* emitter, LitExpression* statement)
{
    if(statement == NULL)
    {
        return;
    }
    switch(statement->type)
    {
        case LIT_EXPR_VARDECL:
        {
            LitVarStatement* stmt = (LitVarStatement*)statement;
            lit_emitter_markprivateinit(emitter, lit_emitter_addprivate(emitter, stmt->name, stmt->length, statement->line, stmt->constant));
            break;
        }
        case LIT_EXPR_FUNCTION:
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
        case LTOKEN_MINUS:
            return OP_NEGATE;
        case LTOKEN_BANG:
            return OP_NOT;
        case LTOKEN_TILDE:
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
        case LTOKEN_BANG_EQUAL:
        case LTOKEN_EQUAL_EQUAL:
            return OP_EQUAL;
        case LTOKEN_LESS:
            return OP_LESS;
        case LTOKEN_LESS_EQUAL:
            return OP_LESS_EQUAL;
        case LTOKEN_GREATER:
            return OP_GREATER;
        case LTOKEN_GREATER_EQUAL:
            return OP_GREATER_EQUAL;
        case LTOKEN_PLUS:
            return OP_ADD;
        case LTOKEN_MINUS:
            return OP_SUBTRACT;
        case LTOKEN_STAR:
            return OP_MULTIPLY;
        case LTOKEN_STAR_STAR:
            return OP_POWER;
        case LTOKEN_SLASH:
            return OP_DIVIDE;
        case LTOKEN_SHARP:
            return OP_FLOOR_DIVIDE;
        case LTOKEN_PERCENT:
            return OP_MOD;
        case LTOKEN_LESS_LESS:
            return OP_LSHIFT;
        case LTOKEN_GREATER_GREATER:
            return OP_RSHIFT;
        case LTOKEN_CARET:
            return OP_BXOR;
        case LTOKEN_AMPERSAND:
            return OP_BAND;
        case LTOKEN_BAR:
            return OP_BOR;
        case LTOKEN_IS:
            return OP_IS;
        default:
            UNREACHABLE
    }
    return OP_RETURN;
}

uint16_t lit_emitter_parsearg(LitEmitter* emitter, LitExpression* expression, uint8_t reg)
{
    if(expression->type == LIT_EXPR_LITERAL)
    {
        LitValue value = ((LitLiteralExpression*)expression)->value;
        if(lit_value_isnumber(value) || IS_STRING(value))
        {
            uint16_t arg = lit_emitter_addconst(emitter, expression->line, value);
            SET_BIT(arg, 8)// Mark that this is a constant
            return arg;
        }
    }
    else if(expression->type == LIT_EXPR_VAR)
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
    if(op == LTOKEN_AMPERSAND_AMPERSAND || op == LTOKEN_BAR_BAR || op == LTOKEN_QUESTION_QUESTION)
    {
        lit_emitter_emitexpr(emitter, expr->left, reg);
        LitUInt jump = lit_emitter_emittmp(emitter);
        lit_emitter_emitexpr(emitter, expr->right, reg);
        lit_emitter_patchinstr(emitter, jump,
                          LIT_FORM_ABX_INSTRUCTION(op == LTOKEN_BAR_BAR ? OP_TRUE_JUMP : (op == LTOKEN_QUESTION_QUESTION ? OP_NON_NULL_JUMP : OP_FALSE_JUMP),
                                                   reg, emitter->chunk->compiledcodecount - jump - 1));
    }
    else
    {
        uint16_t b = lit_emitter_parsearg(emitter, expr->left, reg);
        LitOpCode opcode = lit_emitter_translatebinaryop(op);
        if(opcode == OP_IS)
        {
            if(expr->right->type != LIT_EXPR_VAR)
            {
                return lit_emitter_raiseerror(emitter, expr->expression.line, "'is' operator is not used with a var expression");
            }
            LitVarExpression* e = (LitVarExpression*)expr->right;
            int constant = lit_emitter_addconst(emitter, expr->expression.line, lit_value_fromobject(lit_string_copy(emitter->state, e->name, e->length)));
            lit_emitter_emitabc(emitter, expr->expression.line, opcode, reg, b, constant);
        }
        else
        {
            uint16_t rc = lit_emitter_reserveregister(emitter);
            uint16_t c = lit_emitter_parsearg(emitter, expr->right, rc);
            lit_emitter_emitabc(emitter, expr->expression.line, opcode, reg, swap ? c : b, swap ? b : c);
            lit_emitter_freeregister(emitter, rc);
        }
    }
}

bool lit_emitter_emitparams(LitEmitter* emitter, LitDynListParam* parameters, LitUInt line)
{
    for(LitUInt i = 0; i < parameters->count; i++)
    {
        LitParameter* parameter = &parameters->values[i];
        uint8_t reg = lit_emitter_reserveregister(emitter);
        parameter->reg = reg;
        int index = lit_emitter_addlocal(emitter, parameter->name, parameter->length, line, false, reg);
        lit_emitter_marklocalinit(emitter, index);
        // Vararg ...
        if(parameter->length == 3 && memcmp(parameter->name, "...", 3) == 0)
        {
            return true;
        }
        if(parameter->default_value != NULL)
        {
            LitUInt jump = lit_emitter_emittmp(emitter);
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
        case LIT_EXPR_LITERAL:
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
                SET_BIT(constant, 8);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, constant, 0);
            }
            break;
        }
        case LIT_EXPR_UNARY:
        {
            LitUnaryExpression* expr = (LitUnaryExpression*)expression;
            uint16_t b = lit_emitter_parsearg(emitter, expr->right, reg);
            lit_emitter_emitabc(emitter, expression->line, lit_emitter_translateunaryop(expr->op), reg, b, 0);
            break;
        }
        case LIT_EXPR_BINARY:
        {
            LitBinaryExpression* expr = (LitBinaryExpression*)expression;
            switch(expr->op)
            {
                case LTOKEN_GREATER:
                case LTOKEN_GREATER_EQUAL:
                case LTOKEN_LESS:
                case LTOKEN_LESS_EQUAL:
                case LTOKEN_EQUAL_EQUAL:
                {
                    lit_emitter_emitbinaryexpr(emitter, expr, reg, false);
                    break;
                }
                case LTOKEN_BANG_EQUAL:
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
        case LIT_EXPR_VAR:
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
                        uint16_t constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copy(emitter->state, expr->name, expr->length)));
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
        case LIT_EXPR_ASSIGN:
        {
            LitAssignExpression* expr = (LitAssignExpression*)expression;
            if(expr->to->type == LIT_EXPR_VAR)
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
                            uint16_t constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copy(emitter->state, e->name, e->length)));
                            lit_emitter_emitabx(emitter, expression->line, OP_SET_GLOBAL, constant, r);
                        }
                        else
                        {
                            if(emitter->privates.values[index].constant)
                            {
                                lit_emitter_raiseerror(emitter, expression->line, "Attempt to modify constant '%.*s'", e->length, e->name);
                            }
                            if(IS_BIT_SET(r, 8))
                            {
                                SET_BIT(index, 16);
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
                        lit_emitter_raiseerror(emitter, expression->line, "Attempt to modify constant '%.*s'", e->length, e->name);
                    }
                    lit_emitter_emitexpr(emitter, expr->value, local.reg);
                    if(!ignored && reg != local.reg)
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, local.reg, 0);
                    }
                    break;
                }
            }
            else if(expr->to->type == LIT_EXPR_SUBSCRIPT)
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
            else if(expr->to->type == LIT_EXPR_GET)
            {
                LitGetExpression* e = (LitGetExpression*)expr->to;
                uint8_t r = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, e->where, r);
                lit_emitter_emitexpr(emitter, expr->value, reg);
                int constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copy(emitter->state, e->name, e->length)));
                lit_emitter_emitabc(emitter, expression->line, OP_SET_FIELD, r, constant, reg);
                lit_emitter_freeregister(emitter, r);
                break;
            }
            else if(expr->to->type == LIT_EXPR_REFERENCE)
            {
                uint8_t r = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, ((LitReferenceExpression*)expr->to)->to, r);
                lit_emitter_emitexpr(emitter, expr->value, reg);
                lit_emitter_emitabc(emitter, expression->line, OP_SET_REFERENCE, r, reg, 0);
                lit_emitter_freeregister(emitter, r);
                break;
            }
            lit_emitter_raiseerror(emitter, expression->line, "Invalid assigment target");
            break;
        }
        case LIT_EXPR_CALL:
        {
            LitCallExpression* expr = (LitCallExpression*)expression;
            LitUInt argc = expr->args.count;
            uint16_t argregs[argc];
            bool method = expr->callee->type == LIT_EXPR_GET;
            bool super = expr->callee->type == LIT_EXPR_SUPER;
            if(method)
            {
                ((LitGetExpression*)expr->callee)->ignore_emit = true;
            }
            else if(super)
            {
                ((LitSuperExpression*)expr->callee)->ignore_emit = true;
            }
            lit_emitter_emitexpr(emitter, expr->callee, reg);
            uint8_t tmpreg = super ? lit_emitter_reserveregister(emitter) : 0;
            for(LitUInt i = 0; i < argc; i++)
            {
                uint16_t supadd;
                uint16_t arg_reg = lit_emitter_reserveregister(emitter);
                LitExpression* e = expr->args.values[i];
                supadd = (super ? 2 : 1);
                if(arg_reg != reg + i + supadd)
                {
                    // Something went terribly wrong
                    fprintf(stderr, "arg_reg (%d) != reg (%d) + i (%d) + supadd (%d)\n", arg_reg, reg, i, supadd);
                    //UNREACHABLE
                }
                argregs[i] = arg_reg;
                lit_emitter_emitexpr(emitter, e, arg_reg);
            }
            if(method)
            {
                if(expr->callee->type != LIT_EXPR_GET)
                {
                    UNREACHABLE// TODO: replace with a proper error code?
                }
                LitGetExpression* e = (LitGetExpression*)expr->callee;
                int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->state, e->name, e->length)));
                lit_emitter_emitabc(emitter, expression->line, OP_INVOKE, reg, argc + 1, constant);
            }
            else if(super)
            {
                assert(tmpreg == reg + 1);
                LitSuperExpression* e = (LitSuperExpression*)expr->callee;
                uint8_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "super", 5, emitter->last_line);
                lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, tmpreg, index);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE_SUPER, reg, argc + 1, lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(e->method)));
                lit_emitter_freeregister(emitter, tmpreg);
            }
            else
            {
                lit_emitter_emitabc(emitter, expression->line, OP_CALL, reg, argc + 1, 1);
            }
            for(LitUInt i = 0; i < argc; i++)
            {
                lit_emitter_freeregister(emitter, argregs[i]);
            }
            if(method)
            {
                LitExpression* get = expr->callee;
                while(get != NULL)
                {
                    if(get->type == LIT_EXPR_GET)
                    {
                        LitGetExpression* getter = (LitGetExpression*)get;
                        if(getter->jump > 0)
                        {
                            lit_emitter_patchinstr(emitter, getter->jump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, reg, (int64_t)emitter->chunk->compiledcodecount - getter->jump - 1));
                        }
                        get = getter->where;
                    }
                    else if(get->type == LIT_EXPR_SUBSCRIPT)
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
            for(LitUInt i = 0; i < init->values.count; i++)
            {
                LitExpression* e = init->values.values[i];
                emitter->last_line = e->line;
                lit_emitter_emitexpr(emitter, e, r);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_PUSH_OBJECT_ELEMENT, reg, lit_emitter_addconst(emitter, emitter->last_line, init->keys.values[i]), r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_EXPR_GET:
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
                    int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->state, expr->name, expr->length)));
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
                int constant = lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(lit_string_copy(emitter->state, expr->name, expr->length)));
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
        case LIT_EXPR_SET:
        {
            LitSetExpression* expr = (LitSetExpression*)expression;
            uint8_t wherereg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->where, wherereg);
            uint8_t valuereg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->value, valuereg);
            int constant = lit_emitter_addconst(emitter, emitter->last_line, lit_value_fromobject(lit_string_copy(emitter->state, expr->name, expr->length)));
            lit_emitter_emitabc(emitter, emitter->last_line, OP_SET_FIELD, wherereg, constant, valuereg);
            if(!ignored && reg != valuereg)
            {
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, reg, valuereg, 0);// Pains me to do this, but we gotta ensure that the value is after the where
            }
            lit_emitter_freeregister(emitter, wherereg);
            lit_emitter_freeregister(emitter, valuereg);
            break;
        }
        case LIT_EXPR_SUBSCRIPT:
        {
            LitSubscriptExpression* expr = (LitSubscriptExpression*)expression;
            lit_emitter_emitexpr(emitter, expr->array, reg);
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->index, r);
            lit_emitter_emitabc(emitter, expression->line, OP_SUBSCRIPT_GET, reg, r, 0);
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_EXPR_ARRAY:
        {
            LitArrayExpression* expr = (LitArrayExpression*)expression;
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitabx(emitter, expression->line, OP_ARRAY, reg, expr->values.count);
            for(LitUInt i = 0; i < expr->values.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->values.values[i], r);
                lit_emitter_emitabx(emitter, emitter->last_line, OP_PUSH_ARRAY_ELEMENT, reg, r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_EXPR_OBJECT:
        {
            LitObjectExpression* expr = (LitObjectExpression*)expression;
            uint8_t r = lit_emitter_reserveregister(emitter);
            lit_emitter_emitabc(emitter, expression->line, OP_OBJECT, reg, 0, 0);
            for(LitUInt i = 0; i < expr->values.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->values.values[i], r);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_PUSH_OBJECT_ELEMENT, reg, lit_emitter_addconst(emitter, emitter->last_line, expr->keys.values[i]), r);
            }
            lit_emitter_freeregister(emitter, r);
            break;
        }
        case LIT_EXPR_LAMBDA:
        {
            LitFunctionStatement* expr = (LitFunctionStatement*)expression;
            LitString* name
            = AS_STRING(lit_string_format(emitter->state, "lambda @:@", lit_value_fromobject(emitter->module->name), lit_string_numbertostring(emitter->state, expression->line)));
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, FUNCTION_REGULAR);
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &expr->parameters, expression->line);
            bool ended = false;
            if(expr->body != NULL)
            {
                bool singleexpr = expr->body->type == LIT_EXPR_EXPRESSION;
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
            function->max_registers += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);
                for(LitUInt i = 0; i < function->upvaluecount; i++)
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
                SET_BIT(functionreg, 8);
            }
            lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, functionreg, 0);
            if(closure)
            {
                lit_emitter_freeregister(emitter, functionreg);
            }
            break;
        }
        case LIT_EXPR_RANGE:
        {
            LitRangeExpression* expr = (LitRangeExpression*)expression;
            lit_emitter_emitexpr(emitter, expr->to, reg);
            uint8_t regb = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->from, regb);
            lit_emitter_emitabc(emitter, expression->line, OP_RANGE, reg, regb, reg);
            lit_emitter_freeregister(emitter, regb);
            break;
        }
        case LIT_EXPR_INTERPOLATION:
        {
            LitInterpolationExpression* expr = (LitInterpolationExpression*)expression;
            lit_emitter_emitabx(emitter, expression->line, OP_ARRAY, reg, expr->expressions.count);
            uint8_t r = lit_emitter_reserveregister(emitter);
            for(LitUInt i = 0; i < expr->expressions.count; i++)
            {
                lit_emitter_emitexpr(emitter, expr->expressions.values[i], r);
                lit_emitter_emitabx(emitter, emitter->last_line, OP_PUSH_ARRAY_ELEMENT, reg, r);
            }
            lit_emitter_freeregister(emitter, r);
            lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, reg, 2, lit_emitter_addconst(emitter, emitter->last_line, OBJECT_CONST_STRING(emitter->state, "join")));
            break;
        }
        case LIT_EXPR_THIS:
        {
            LitFunctionType type = emitter->compiler->type;
            if(type == FUNCTION_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'this' can't be used %s", "in static methods");
            }
            if(type == FUNCTION_CONSTRUCTOR || type == FUNCTION_METHOD)
            {
                // The instance is always in register 0
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
                    uint8_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "this", 4, expression->line);
                    lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, reg, index);
                }
            }
            break;
        }
        case LIT_EXPR_TERNARY:
        {
            LitTernaryExpression* expr = (LitTernaryExpression*)expression;
            uint8_t condreg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, expr->condition, condreg);
            LitUInt condbranchskip = lit_emitter_emittmp(emitter);
            lit_emitter_freeregister(emitter, condreg);
            int64_t start = emitter->chunk->compiledcodecount;
            lit_emitter_emitexpr(emitter, expr->if_branch, reg);
            LitUInt elseskip = lit_emitter_emittmp(emitter);
            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->compiledcodecount - start));
            int64_t elsestart = emitter->chunk->compiledcodecount;
            lit_emitter_emitexpr(emitter, expr->else_branch, reg);
            lit_emitter_patchinstr(emitter, elseskip, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - elsestart));
            break;
        }
        case LIT_EXPR_SUPER:
        {
            if(emitter->compiler->type == FUNCTION_STATIC_METHOD)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'super' can't be used %s", "in static methods");
            }
            else if(!emitter->class_has_super)
            {
                lit_emitter_raiseerror(emitter, expression->line, "'super' can't be used in class '%s', because it doesn't have a super class", emitter->class_name->chars);
            }
            LitSuperExpression* expr = (LitSuperExpression*)expression;
            if(!expr->ignore_emit)
            {
                uint8_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "super", 5, emitter->last_line);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
                uint8_t tmpreg = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, tmpreg, index);
                lit_emitter_emitabc(emitter, expression->line, OP_GET_SUPER_METHOD, reg, tmpreg, lit_emitter_addconst(emitter, expression->line, lit_value_fromobject(expr->method)));
                lit_emitter_freeregister(emitter, tmpreg);
            }
            break;
        }
        case LIT_EXPR_REFERENCE:
        {
            LitExpression* to = ((LitReferenceExpression*)expression)->to;
            if(to->type != LIT_EXPR_VAR && to->type != LIT_EXPR_GET && to->type != LIT_EXPR_THIS && to->type != LIT_EXPR_SUPER)
            {
                lit_emitter_raiseerror(emitter, expression->line, "Invalid refence target");
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
            lit_emitter_raiseerror(emitter, expression->line, "Unknown expression with id '%i'", (int)expression->type);
            break;
        }
    }
}

void lit_emitter_patchloopjumps(LitEmitter* emitter, LitDynListUInt* breaks)
{
    for(LitUInt i = 0; i < breaks->count; i++)
    {
        lit_emitter_patchinstr(emitter, breaks->values[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - breaks->values[i] - 1));
    }
    lit_uintlist_destroy(emitter->state, breaks);
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
        case LIT_EXPR_EXPRESSION:
        {
            lit_emitter_emitexprignoringregister(emitter, ((LitExpressionStatement*)statement)->expression);
            break;
        }
        case LIT_EXPR_BLOCK:
        {
            LitDynListExpr* statements = &((LitBlockStatement*)statement)->statements;
            bool endedscope = false;
            for(LitUInt i = 0; i < statements->count; i++)
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
        case LIT_EXPR_VARDECL:
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
            int index = isprivate ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) :
                                  lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, stmt->constant, reg);
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
        case LIT_EXPR_IF:
        {
            LitIfStatement* stmt = (LitIfStatement*)statement;
            uint16_t condreg = lit_emitter_reserveregister(emitter);
            lit_emitter_emitexpr(emitter, stmt->condition, condreg);
            LitUInt condbranchskip = lit_emitter_emittmp(emitter);
            LitUInt elseskip = 0;
            lit_emitter_freeregister(emitter, condreg);
            int64_t start = emitter->chunk->compiledcodecount;
            lit_emitter_emitstmtscoped(emitter, stmt->if_branch);
            if(stmt->else_branch)
            {
                elseskip = lit_emitter_emittmp(emitter);
            }
            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->compiledcodecount - start));
            LitUInt endjumpcount = stmt->elseif_branches == NULL ? 0 : stmt->elseif_branches->count;
            uint64_t endjumps[endjumpcount];
            if(stmt->elseif_branches != NULL)
            {
                for(LitUInt i = 0; i < stmt->elseif_branches->count; i++)
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
            for(LitUInt i = 0; i < endjumpcount; i++)
            {
                if(stmt->elseif_conditions->values[i] == NULL)
                {
                    continue;
                }
                lit_emitter_patchinstr(emitter, endjumps[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->compiledcodecount - endjumps[i] - 1));
            }
            break;
        }
        case LIT_EXPR_FUNCTION:
        {
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
                index = isprivate ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) :
                                  lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, false, reg = lit_emitter_reserveregister(emitter));
            }
            LitString* name = lit_string_copy(emitter->state, stmt->name, stmt->length);
            if(local)
            {
                lit_emitter_marklocalinit(emitter, index);
            }
            else if(isprivate)
            {
                lit_emitter_markprivateinit(emitter, index);
            }
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, FUNCTION_REGULAR);
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &stmt->parameters, statement->line);
            if(!lit_emitter_emitstmt(emitter, stmt->body))
            {
                lit_emitter_scopeend(emitter);
            }
            LitFunction* function = lit_emitter_compilerend(emitter, name);
            function->argcount = stmt->parameters.count;
            function->max_registers += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);
                for(LitUInt i = 0; i < function->upvaluecount; i++)
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
                SET_BIT(functionreg, 8);
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
                    SET_BIT(index, 16);
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
        case LIT_EXPR_RETURN:
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
        case LIT_EXPR_WHILE:
        {
            LitWhileStatement* stmt = (LitWhileStatement*)statement;
            uint8_t reg = lit_emitter_reserveregister(emitter);
            LitUInt beforecond = lit_emitter_emittmp(emitter);
            emitter->loop_start = beforecond;
            emitter->compiler->loop_depth++;
            LitDynListUInt old_breaks = emitter->breaks;
            LitDynListUInt old_continues = emitter->continues;
            lit_uintlist_init(&emitter->breaks);
            lit_uintlist_init(&emitter->continues);
            lit_emitter_emitexpr(emitter, stmt->condition, reg);
            LitUInt tmpinstr = lit_emitter_emittmp(emitter);
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
        case LIT_EXPR_FOR:
        {
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
                LitUInt start = emitter->chunk->compiledcodecount;
                LitUInt exitjump = 0;
                uint8_t condreg = 0;
                if(stmt->condition != NULL)
                {
                    condreg = lit_emitter_reserveregister(emitter);
                    lit_emitter_emitexpr(emitter, stmt->condition, condreg);
                    exitjump = lit_emitter_emittmp(emitter);
                }
                if(stmt->increment != NULL)
                {
                    LitUInt bodyjump = lit_emitter_emittmp(emitter);
                    LitUInt incrstart = emitter->chunk->compiledcodecount;
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
                    if(stmt->body->type == LIT_EXPR_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitBlockStatement*)stmt->body)->statements;
                        for(LitUInt i = 0; i < statements->count; i++)
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
                LitUInt sequence = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "seq ", 4, statement->line, false, sequence));
                uint8_t condreg = lit_emitter_reserveregister(emitter);
                lit_emitter_emitexpr(emitter, stmt->condition, condreg);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, sequence, condreg, 0);
                LitUInt iterator = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "iter ", 5, statement->line, false, iterator));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_LOAD_NULL, iterator, 0, 0);
                LitUInt start = emitter->chunk->compiledcodecount;
                emitter->loop_start = emitter->chunk->compiledcodecount;
                // iter = seq.iterator(iter)
                uint8_t tmprega = lit_emitter_reserveregister(emitter);
                uint8_t tmpregb = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, tmprega, 2,
                                     lit_emitter_addconst(emitter, emitter->last_line, OBJECT_CONST_STRING(emitter->state, "iterator")));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, iterator, tmprega, 0);
                // If iter is null, just get out of the loop
                LitUInt exitjump = lit_emitter_emittmp(emitter);
                lit_emitter_scopebegin(emitter);
                // var i = seq.iteratorValue(iter)
                LitVarStatement* var = (LitVarStatement*)stmt->var;
                LitUInt local = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, var->name, var->length, statement->line, false, local));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmprega, sequence, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, tmpregb, iterator, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE, tmprega, 2,
                                     lit_emitter_addconst(emitter, emitter->last_line, OBJECT_CONST_STRING(emitter->state, "iteratorValue")));
                lit_emitter_emitabc(emitter, emitter->last_line, OP_MOVE, local, tmprega, 0);
                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_EXPR_BLOCK)
                    {
                        LitDynListExpr* statements = &((LitBlockStatement*)stmt->body)->statements;
                        for(LitUInt i = 0; i < statements->count; i++)
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
        case LIT_EXPR_BREAK:
        {
            if(emitter->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emitter, statement->line, "Can't use '%s' outside of loops", "break");
            }
            lit_uintlist_push(&emitter->breaks, lit_emitter_emittmp(emitter));
            break;
        }
        case LIT_EXPR_CONTINUE:
        {
            if(emitter->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emitter, statement->line, "Can't use '%s' outside of loops", "continue");
            }
            lit_uintlist_push(&emitter->continues, lit_emitter_emittmp(emitter));
            break;
        }
        case LIT_EXPR_CLASS:
        {
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
                uint8_t super = lit_emitter_addlocal(emitter, "super", 5, emitter->last_line, false, lit_emitter_reserveregister(emitter));
                lit_emitter_marklocalinit(emitter, super);
            }
            for(LitUInt i = 0; i < stmt->fields.count; i++)
            {
                LitExpression* s = stmt->fields.values[i];
                if(s->type == LIT_EXPR_VARDECL)
                {
                    LitVarStatement* var = (LitVarStatement*)s;
                    uint8_t reg = lit_emitter_reserveregister(emitter);
                    lit_emitter_emitexpr(emitter, var->init, reg);
                    int fieldnameconst = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(lit_string_copy(emitter->state, var->name, var->length)));
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
        case LIT_EXPR_METHOD:
        {
            LitString* clsname;
            LitMethodStatement* stmt = (LitMethodStatement*)statement;
            bool constructor = stmt->name->length == 11 && memcmp(stmt->name->chars, "constructor", 11) == 0;
            if(constructor && stmt->is_static)
            {
                lit_emitter_raiseerror(emitter, statement->line, "Constructors can't be static (at least for now)");
            }
            LitCompiler compiler;
            lit_emitter_compilerinit(emitter, &compiler, constructor ? FUNCTION_CONSTRUCTOR : (stmt->is_static ? FUNCTION_STATIC_METHOD : FUNCTION_METHOD));
            lit_emitter_scopebegin(emitter);
            bool vararg = lit_emitter_emitparams(emitter, &stmt->parameters, statement->line);
            if(!lit_emitter_emitstmt(emitter, stmt->body))
            {
                lit_emitter_scopeend(emitter);
            }
            clsname = (LitString*)lit_value_asobject(lit_value_fromobject(emitter->class_name));
            {
            #if 0
                fprintf(stderr, "emitter->class_name=%.*s\n", clsname->length, clsname->chars);
            #endif
            }
            LitFunction* function = lit_emitter_compilerend(emitter, clsname);
            function->argcount = stmt->parameters.count;
            function->max_registers += function->argcount;
            function->vararg = vararg;
            uint16_t functionreg;
            bool closure = function->upvaluecount > 0;
            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);
                for(LitUInt i = 0; i < function->upvaluecount; i++)
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
                SET_BIT(functionreg, 8);
            }
            int fieldnameconst = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(stmt->name));
            lit_emitter_emitabc(emitter, statement->line, stmt->is_static ? OP_STATIC_FIELD : OP_METHOD, emitter->class_register, fieldnameconst, functionreg);
            if(closure)
            {
                lit_emitter_freeregister(emitter, functionreg);
            }
            break;
        }
        case LIT_EXPR_FIELD:
        {
            LitFieldStatement* stmt = (LitFieldStatement*)statement;
            LitFunction* getter = NULL;
            LitFunction* setter = NULL;
            if(stmt->getter != NULL)
            {
                LitCompiler compiler;
                lit_emitter_compilerinit(emitter, &compiler, stmt->is_static ? FUNCTION_STATIC_METHOD : FUNCTION_METHOD);
                lit_emitter_scopebegin(emitter);
                if(stmt->getter->type == LIT_EXPR_EXPRESSION)
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
                getter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_format(emitter->state, "@:get @", lit_value_fromobject(emitter->class_name), stmt->name)));
            }
            if(stmt->setter != NULL)
            {
                LitCompiler compiler;
                lit_emitter_compilerinit(emitter, &compiler, stmt->is_static ? FUNCTION_STATIC_METHOD : FUNCTION_METHOD);
                uint8_t reg = lit_emitter_reserveregister(emitter);
                lit_emitter_marklocalinit(emitter, lit_emitter_addlocal(emitter, "value", 5, statement->line, false, reg));
                lit_emitter_scopebegin(emitter);
                if(!lit_emitter_emitstmt(emitter, stmt->setter))
                {
                    lit_emitter_scopeend(emitter);
                }
                lit_emitter_freeregister(emitter, reg);
                setter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_format(emitter->state, "@:set @", lit_value_fromobject(emitter->class_name), stmt->name)));
                setter->argcount = 1;
                setter->max_registers++;
            }
            LitField* field = lit_object_makefield(emitter->state, (LitObject*)getter, (LitObject*)setter);
            int constant = lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(field));
            SET_BIT(constant, 8);
            lit_emitter_emitabc(emitter, statement->line, stmt->is_static ? OP_STATIC_FIELD : OP_METHOD, emitter->class_register,
                                 lit_emitter_addconst(emitter, statement->line, lit_value_fromobject(stmt->name)), constant);
            break;
        }
        default:
        {
            lit_emitter_raiseerror(emitter, statement->line, "Unknown statement with id '%i'", (int)statement->type);
            break;
        }
    }
    return false;
}

LitModule* lit_emitter_emitmod(LitEmitter* emitter, LitDynListExpr* statements, LitString* modname)
{
    emitter->last_line = 1;
    emitter->emit_reference = 0;
    LitState* state = emitter->state;
    LitValue modulevalue;
    LitModule* module;
    bool isnew = false;
    if(lit_table_get(&emitter->state->vm->modules->values, modname, &modulevalue))
    {
        module = AS_MODULE(modulevalue);
    }
    else
    {
        module = lit_object_makemodule(emitter->state, modname);
        isnew = true;
    }
    emitter->module = module;
    LitUInt oldprivatescnt = module->private_count;
    if(oldprivatescnt > 0)
    {
        LitDynListPriv* privates = &emitter->privates;
        privates->count = oldprivatescnt - 1;
        lit_privlist_push(privates, (LitPrivate){ true, false });
        for(LitUInt i = 0; i < oldprivatescnt; i++)
        {
            privates->values[i].initialized = true;
        }
    }
    LitCompiler compiler;
    lit_emitter_compilerinit(emitter, &compiler, FUNCTION_SCRIPT);
    emitter->chunk = &compiler.function->chunk;
    lit_emitter_resolvestatements(emitter, statements);
    lit_emitter_scopebegin(emitter);
    bool endedscope = false;
    for(LitUInt i = 0; i < statements->count; i++)
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
    module->main_function = lit_emitter_compilerend(emitter, modname);
    if(isnew)
    {
        LitUInt total = emitter->privates.count;
        module->privates = lit_sysmem_malloc(total * sizeof(LitValue));
        for(LitUInt i = 0; i < total; i++)
        {
            module->privates[i] = lit_value_makenull();
        }
    }
    else
    {
        module->privates = (LitValue*)lit_sysmem_realloc(module->privates, sizeof(LitValue) * (module->private_count));
        for(LitUInt i = oldprivatescnt; i < module->private_count; i++)
        {
            module->privates[i] = lit_value_makenull();
        }
    }
    lit_privlist_destroy(&emitter->privates);
    if(isnew && !state->had_error)
    {
        lit_table_set(state, &state->vm->modules->values, modname, lit_value_fromobject(module));
    }
    module->ran = true;
    return module;
}



void lit_debug_disasmodule(LitModule* module, const char* source)
{
    lit_debug_disaschunk(&module->main_function->chunk, module->main_function->name->chars, source);
}

void lit_debug_printconst(LitValue value)
{
    if(IS_FUNCTION(value))
    {
        LitString* fnname = AS_FUNCTION(value)->name;
        printf("%sfunction %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_CLOSURE(value))
    {
        LitString* fnname = AS_CLOSURE(value)->function->name;
        printf("%sclosure %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_CLOSURE_PROTOTYPE(value))
    {
        LitString* fnname = AS_CLOSURE_PROTOTYPE(value)->function->name;
        printf("%sclosure prototype %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_STRING(value))
    {
        LitString* string = AS_STRING(value);
        printf("%s\"%.*s\"%s", COLOR_CYAN, string->length, string->chars, COLOR_RESET);
    }
    else if(lit_value_isnumber(value))
    {
        printf("%s%g%s", COLOR_CYAN, lit_value_asnumber(value), COLOR_RESET);
    }
    else
    {
        printf("unknown");
    }
}

void lit_debug_disaschunk(LitChunk* chunk, const char* name, const char* source)
{
    LitDynListVal* values = &chunk->constants;
    printf("^^ %s ^^\n", name);
    if(values->count > 0)
    {
        printf("%sconstants:%s\n", COLOR_MAGENTA, COLOR_RESET);
        for(LitUInt i = 0; i < values->count; i++)
        {
            LitValue value = values->values[i];
            printf("% 4d ", i);
            lit_debug_printconst(value);
            printf("\n");
        }
    }
    printf("%stext:%s\n", COLOR_MAGENTA, COLOR_RESET);
    for(LitUInt offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        lit_debug_disasinstr(chunk, offset, source, false);
    }
    printf("%shex:%s\n", COLOR_MAGENTA, COLOR_RESET);
    for(LitUInt offset = 0; offset < chunk->compiledcodecount; offset++)
    {
        printf("%08lX ", chunk->compiledcodechunk[offset]);
    }
    printf("\n");
    printf("vv %s vv\n", name);
}

void lit_debug_printabcinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_B(instruction), LIT_INSTRUCTION_C(instruction));
}

void lit_debug_printabxinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_BX(instruction));
}

void lit_debug_printasbxinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%li\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_SBX(instruction));
}

void lit_debug_printregister(uint16_t reg)
{
    printf(" \t%hu", reg);
}

void lit_debug_printconstarg(LitChunk* chunk, uint16_t arg, bool indent)
{
    arg &= 0xff;
    printf("%sc%hu (", indent ? " \t" : "", arg);
    lit_debug_printconst(chunk->constants.values[arg]);
    printf(")");
}

void lit_debug_printconstorregister(LitChunk* chunk, uint16_t arg)
{
    if(IS_BIT_SET(arg, 8))
    {
        lit_debug_printconstarg(chunk, arg, true);
    }
    else
    {
        lit_debug_printregister(arg);
    }
}

void lit_debug_printunaryinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_B(instruction));
    printf("\n");
}

void lit_debug_printbinaryinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));

    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_B(instruction));
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_C(instruction));

    printf("\n");
}

void lit_debug_printglobalinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "");

    lit_debug_printconstarg(chunk, LIT_INSTRUCTION_BX(instruction), false);
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_A(instruction));

    printf("\n");
}

static LitDebugInstructionFn debuginstrfuncs[] = { lit_debug_printabcinstr, lit_debug_printabxinstr, lit_debug_printasbxinstr };

void lit_debug_disasinstr(LitChunk* chunk, LitUInt offset, const char* source, bool forceline)
{
    LitUInt line = lit_chunk_getline(chunk, offset);
    bool same = !chunk->haslineinfo || (offset > 0 && line == lit_chunk_getline(chunk, offset - 1));
    if(!same && source != NULL)
    {
        LitUInt index = 0;
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
                printf("%s        %.*s%s\n", COLOR_RED, nextline ? (int)(nextline - outputline) : (int)strlen(prevline), outputline, COLOR_RESET);
                break;
            }
        }
    }
    printf("%04d ", offset);
    if(same && !forceline)
    {
        printf("   | ");
    }
    else
    {
        printf("%s%4d%s ", COLOR_BLUE, line, COLOR_RESET);
    }
    uint64_t instruction = chunk->compiledcodechunk[offset];
    uint8_t opcode = LIT_INSTRUCTION_OPCODE(instruction);
    switch(opcode)
    {
        case OP_MOVE:
            lit_debug_printbinaryinstr(chunk, instruction, "MOVE");
            break;
        case OP_ADD:
            lit_debug_printbinaryinstr(chunk, instruction, "ADD");
            break;
        case OP_SUBTRACT:
            lit_debug_printbinaryinstr(chunk, instruction, "SUBTRACT");
            break;
        case OP_MULTIPLY:
            lit_debug_printbinaryinstr(chunk, instruction, "MULTIPLY");
            break;
        case OP_DIVIDE:
            lit_debug_printbinaryinstr(chunk, instruction, "DIVIDE");
            break;
        case OP_NEGATE:
            lit_debug_printunaryinstr(chunk, instruction, "NEGATE");
            break;
        case OP_NOT:
            lit_debug_printunaryinstr(chunk, instruction, "NOT");
            break;
        case OP_EQUAL:
            lit_debug_printbinaryinstr(chunk, instruction, "EQUAL");
            break;
        case OP_LESS:
            lit_debug_printbinaryinstr(chunk, instruction, "LESS");
            break;
        case OP_LESS_EQUAL:
            lit_debug_printbinaryinstr(chunk, instruction, "LESS_EQUAL");
            break;
        case OP_SET_GLOBAL:
            lit_debug_printglobalinstr(chunk, instruction, "SET_GLOBAL");
            break;
        case OP_GET_GLOBAL:
            lit_debug_printglobalinstr(chunk, instruction, "GET_GLOBAL");
            break;
        default:
        {
            switch(opcode)
            {
// A simple way to automatically generate case printers for all the opcodes
#define OPCODE(name, stringname, type)                                   \
    case OP_##name:                                                       \
    {                                                                     \
        debuginstrfuncs[(int)type](instruction, stringname); \
        break;                                                            \
    }
#if 1
OPCODE(MOVE, "MOVE", LIT_INSTRUCTION_ABC) // R(A) := RC(B)
OPCODE(LOAD_NULL, "LOAD_NULL", LIT_INSTRUCTION_ABC) // R(A) := null
OPCODE(LOAD_BOOL, "LOAD_BOOL", LIT_INSTRUCTION_ABC) // R(A) := (bool) B
OPCODE(CLOSURE, "CLOSURE", LIT_INSTRUCTION_ABX) // R(A) := PrC[Bx]
OPCODE(ARRAY, "ARRAY", LIT_INSTRUCTION_ABX) // R(A) := new Array(Bx)
OPCODE(OBJECT, "OBJECT", LIT_INSTRUCTION_ABC) // R(A) = new Object()
OPCODE(RANGE, "RANGE", LIT_INSTRUCTION_ABC) // R(A) = new Range(RC(B), RC(C))
OPCODE(RETURN, "RETURN", LIT_INSTRUCTION_ABC) // return R(A)
OPCODE(ADD, "ADD", LIT_INSTRUCTION_ABC) // R(A) := RC(B) + RC(C)
OPCODE(SUBTRACT, "SUBTRACT", LIT_INSTRUCTION_ABC) // R(A) := RC(B) - RC(C)
OPCODE(MULTIPLY, "MULTIPLY", LIT_INSTRUCTION_ABC) // R(A) := RC(B) * RC(C)
OPCODE(DIVIDE, "DIVIDE", LIT_INSTRUCTION_ABC) // R(A) := RC(B) / RC(C)
OPCODE(FLOOR_DIVIDE, "FLOOR_DIVIDE", LIT_INSTRUCTION_ABC) // R(A) := floor(RC(B) / RC(C))
OPCODE(MOD, "MOD", LIT_INSTRUCTION_ABC) // R(A) := RC(B) % RC(C)
OPCODE(POWER, "POWER", LIT_INSTRUCTION_ABC) // R(A) := pow(RC(B), RC(C))
OPCODE(LSHIFT, "LSHIFT", LIT_INSTRUCTION_ABC) // R(A) := RC(B) << RC(C)
OPCODE(RSHIFT, "RSHIFT", LIT_INSTRUCTION_ABC) // R(A) := RC(B) >> RC(C)
OPCODE(BXOR, "BXOR", LIT_INSTRUCTION_ABC) // R(A) := RC(B) ^ RC(C)
OPCODE(BAND, "BAND", LIT_INSTRUCTION_ABC) // R(A) := RC(B) & RC(C)
OPCODE(BOR, "BOR", LIT_INSTRUCTION_ABC) // R(A) := RC(B) | RC(C)
OPCODE(JUMP, "JUMP", LIT_INSTRUCTION_ASBX) // PC += sBx
OPCODE(TRUE_JUMP, "TRUE_JUMP", LIT_INSTRUCTION_ABX) // if (R(A)) PC += Bx
OPCODE(FALSE_JUMP, "FALSE_JUMP", LIT_INSTRUCTION_ABX) // if (not R(A)) PC += Bx
OPCODE(NON_NULL_JUMP, "NON_NULL_JUMP", LIT_INSTRUCTION_ABX) // if (R(A) != null) PC += Bx
OPCODE(NULL_JUMP, "NULL_JUMP", LIT_INSTRUCTION_ABX) // if (R(A) == null) PC += Bx
OPCODE(EQUAL, "EQUAL", LIT_INSTRUCTION_ABC) // R(A) := RC(B) == RC(C)
OPCODE(LESS, "LESS", LIT_INSTRUCTION_ABC) // R(A) := RC(B) < RC(C)
OPCODE(LESS_EQUAL, "LESS_EQUAL", LIT_INSTRUCTION_ABC) // R(A) := RC(B) <= RC(C)
OPCODE(GREATER, "GREATER", LIT_INSTRUCTION_ABC) // R(A) := RC(B) > RC(C)
OPCODE(GREATER_EQUAL, "GREATER_EQUAL", LIT_INSTRUCTION_ABC) // R(A) := RC(B) >= RC(C)
OPCODE(NEGATE, "NEGATE", LIT_INSTRUCTION_ABC) // R(A) := -RC(B)
OPCODE(NOT, "NOT", LIT_INSTRUCTION_ABC) // R(A) := !RC(B)
OPCODE(BNOT, "BNOT", LIT_INSTRUCTION_ABC) // R(A) := ~RC(B)
OPCODE(SET_GLOBAL, "SET_GLOBAL", LIT_INSTRUCTION_ABX) // G[C(A)] := RC(BX)
OPCODE(GET_GLOBAL, "GET_GLOBAL", LIT_INSTRUCTION_ABX) // R(A) := G[C(Bx)]
OPCODE(SET_UPVALUE, "SET_UPVALUE", LIT_INSTRUCTION_ABX) // U[A] := RC(Bx)
OPCODE(GET_UPVALUE, "GET_UPVALUE", LIT_INSTRUCTION_ABX) // R(A) := U[Bx]
OPCODE(SET_PRIVATE, "SET_PRIVATE", LIT_INSTRUCTION_ABX) // P[A] := RC(Bx)
OPCODE(GET_PRIVATE, "GET_PRIVATE", LIT_INSTRUCTION_ABX) // R(A) := P[C(Bx)]
OPCODE(CALL, "CALL", LIT_INSTRUCTION_ABC) // R(A) := R(A)(R(A + 1), ..., R(A + B - 1))
OPCODE(CLOSE_UPVALUE, "CLOSE_UPVALUE", LIT_INSTRUCTION_ABC) // close_upvalue(R(A))
OPCODE(CLASS, "CLASS", LIT_INSTRUCTION_ABC) // G[C(A)] = R[C] = new_class(C(A), C(B - 1))
OPCODE(STATIC_FIELD, "STATIC_FIELD", LIT_INSTRUCTION_ABC) // R(A)[C(B)] = RC(C)
OPCODE(METHOD, "METHOD", LIT_INSTRUCTION_ABC) // R(A).Methods[C(B)] = RC(C)
OPCODE(GET_FIELD, "GET_FIELD", LIT_INSTRUCTION_ABC) // R(A) = R(B)[C(C)]
OPCODE(GET_SUPER_METHOD, "GET_SUPER_METHOD", LIT_INSTRUCTION_ABC) // R(A) = R(B).super[C(C)]
OPCODE(SET_FIELD, "SET_FIELD", LIT_INSTRUCTION_ABC) // R(A)[C(B)] = R(C)
OPCODE(IS, "IS", LIT_INSTRUCTION_ABC) // R(A) := RC(B) is G[C(C)]
OPCODE(INVOKE, "INVOKE", LIT_INSTRUCTION_ABC) // R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1))
OPCODE(INVOKE_SUPER, "INVOKE_SUPER", LIT_INSTRUCTION_ABC) // R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1))
OPCODE(SUBSCRIPT_GET, "SUBSCRIPT_GET", LIT_INSTRUCTION_ABC) // R(A) := R(A)[RC(B)]
OPCODE(SUBSCRIPT_SET, "SUBSCRIPT_SET", LIT_INSTRUCTION_ABC) // R(A)[RC(B)] := R(C)
OPCODE(PUSH_ARRAY_ELEMENT, "PUSH_ARRAY_ELEMENT", LIT_INSTRUCTION_ABX) // R(A)[R(A).count++] = RC(Bx)
OPCODE(PUSH_OBJECT_ELEMENT, "PUSH_OBJECT_ELEMENT", LIT_INSTRUCTION_ABC) // R(A)[R(B)] = RC(C)
OPCODE(REFERENCE_GLOBAL, "REFERENCE_GLOBAL", LIT_INSTRUCTION_ABX) // R(A) := ref G(C[Bx])
OPCODE(REFERENCE_PRIVATE, "REFERENCE_PRIVATE", LIT_INSTRUCTION_ABX) // R(A) := ref P(Bx)
OPCODE(REFERENCE_LOCAL, "REFERENCE_LOCAL", LIT_INSTRUCTION_ABC) // R(A) := ref R(B)
OPCODE(REFERENCE_UPVALUE, "REFERENCE_UPVALUE", LIT_INSTRUCTION_ABX) // R(A) := ref U(Bx)
OPCODE(REFERENCE_FIELD, "REFERENCE_FIELD", LIT_INSTRUCTION_ABC) // R(A) = ref R(B)[C(C)]
OPCODE(SET_REFERENCE, "SET_REFERENCE", LIT_INSTRUCTION_ABC) // ref R(A) := R(B)
#endif
#undef OPCODE
                default:
                {
                    printf("Unknown opcode %d\n", opcode);
                    break;
                }
            }
        }
    }
}

void lit_debug_traceframe(LitFiber* fiber)
{
(void)fiber;
#ifdef LIT_TRACE_STACK
    if(fiber == NULL)
    {
        return;
    }
    LitCallFrame* frame = &fiber->framevals[fiber->framecount - 1];
    printf("== fiber %p f%i %s (expects %i, max %i, added %i, current %i, exits %i) ==\n", fiber, fiber->framecount - 1, frame->function->name->chars,
           frame->function->argcount, frame->function->max_registers, frame->function->max_registers + (int)(fiber->stack_top - fiber->stack),
           fiber->stack_capacity, frame->return_address == NULL);
#endif
}




LitString* lit_state_errorfmtv(LitState* state, LitUInt line, const char* fmt, va_list args)
{
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, fmt, argscopy) + 1;
    va_end(argscopy);
    char buffer[buffersize];
    vsnprintf(buffer, buffersize, fmt, args);
    buffer[buffersize - 1] = '\0';
    if(line != 0)
    {
        return AS_STRING(lit_string_format(state, "[line #]: $", (double)line, (const char*)buffer));
    }
    return AS_STRING(lit_string_format(state, "$", (const char*)buffer));
}

LitString* lit_state_errorfmt(LitState* state, LitUInt line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(state, line, fmt, args);
    va_end(args);
    return result;
}




void lit_eventsystem_init(LitEventSystem* event_system)
{
    lit_eventsystem_reset(event_system);
}

void lit_eventsystem_reset(LitEventSystem* event_system)
{
    event_system->events = NULL;
    event_system->last_event = NULL;
}


void lit_eventsystem_destroy(LitEventSystem* event_system)
{
    event_system->events = NULL;
    event_system->last_event = NULL;
}

uint64_t lit_eventsystem_millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

void lit_eventsystem_registerevent(LitState* state, LitValue callback, uint64_t time)
{
    LitEventSystem* event_system = state->event_system;
    LitEvent* event = (LitEvent*)lit_sysmem_malloc(sizeof(LitEvent));
    event->expire_time = lit_eventsystem_millis() + time;
    event->callback = callback;
    event->next = NULL;
    event->previous = event_system->last_event;
    if(event_system->last_event == NULL)
    {
        event_system->events = event;
    }
    else
    {
        event_system->last_event->next = event;
    }
    event_system->last_event = event;
}

void lit_eventsystem_loop(LitState* state)
{
    LitEventSystem* event_system = state->event_system;
    while(event_system->events != NULL)
    {
        LitEvent* event = event_system->events;
        while(event != NULL)
        {
            if(lit_eventsystem_millis() >= event->expire_time)
            {
                LitEvent* nextevent = event->next;
                if(event->previous != NULL)
                {
                    event->previous->next = nextevent;
                }
                if(event == event_system->events)
                {
                    event_system->events = nextevent;
                }
                if(event == event_system->last_event)
                {
                    event_system->last_event = NULL;
                }
                lit_state_callvalue(state, event->callback, NULL, 0);
                lit_sysmem_free(event);
                event = nextevent;
            }
            else
            {
                event = event->next;
            }
        }
    }
}





void lit_state_openlibraries(LitState* state)
{
    lit_open_math_library(state);
    lit_open_file_library(state);
    lit_open_gc_library(state);
}

LitValue lit_objfn_invalidconstructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    lit_vm_raisefatalerror(vm, "Can't create an instance of built-in type", lit_value_asinstance(instance)->klass->name);
    return lit_value_makenull();
}

/*
 * Class
 */

LitValue objfnclass_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_format(vm->state, "class @", lit_value_fromobject(AS_CLASS(instance)->name));
}

int lit_coreutil_tableiterator(LitTable* table, int number)
{
    if(table->count == 0)
    {
        return -1;
    }
    if(number >= (int)table->capacity)
    {
        return -1;
    }
    number++;
    for(; number < table->capacity; number++)
    {
        if(table->entries[number].key != NULL)
        {
            return number;
        }
    }
    return -1;
}

LitValue lit_coreutil_tableiterkey(LitTable* table, int index)
{
    if(table->capacity <= index)
    {
        return lit_value_makenull();
    }
    return lit_value_fromobject(table->entries[index].key);
}

LitValue objfnclass_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitClass* klass = AS_CLASS(instance);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int methodsCapacity = (int)klass->methods.capacity;
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

LitValue objfnclass_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitClass* klass = AS_CLASS(instance);
    LitUInt methodsCapacity = klass->methods.capacity;
    bool fields = index >= methodsCapacity;
    return lit_coreutil_tableiterkey(fields ? &klass->static_fields : &klass->methods, fields ? index - methodsCapacity : index);
}

LitValue objfnclass_super(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
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

LitValue objfnclass_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitClass* klass = AS_CLASS(instance);
    if(argc == 2)
    {
        if(!IS_STRING(args[0]))
        {
            lit_vm_raisefatalerror(vm, "Class index must be a string");
        }
        lit_table_set(vm->state, &klass->static_fields, AS_STRING(args[0]), args[1]);
        return args[1];
    }
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Class index must be a string");
    }
    LitValue value;
    if(lit_table_get(&klass->static_fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_get(&klass->methods, AS_STRING(args[0]), &value))
    {
        return value;
    }
    return lit_value_makenull();
}


LitValue objfnclass_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)vm;
    return lit_value_fromobject(AS_CLASS(instance)->name);
}

/*
 * Object
 */

LitValue objfnobject_class(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_fromobject(lit_state_getclassfor(vm->state, instance));
}

LitValue objfnobject_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitState* state = vm->state;
    LitClass* klass = lit_state_getclassfor(vm->state, instance);
    if(klass != state->object_class)
    {
        return lit_string_format(state, "@ instance", lit_value_fromobject(klass->name));
    }
    LitTable* values = &lit_value_asinstance(instance)->fields;
    if(values->count == 0)
    {
        return OBJECT_CONST_STRING(state, "{}");
    }
    LitUInt valueamount = values->count;
    LitString* valuesconverted[valueamount];
    LitString* keys[valueamount];
    LitUInt indentation = LIT_GET_NUMBER(0, 0) + 1;
    LitUInt objfnstring_length = indentation + 2;
    LitUInt i = 0;
    LitUInt index = 0;
    do
    {
        LitTabEntry* entry = &values->entries[index++];
        if(entry->key != NULL)
        {
            LitString* value = lit_tostring_value(state, entry->value, indentation);
            lit_state_pushroot(state, (LitObject*)value);
            if(IS_STRING(entry->value))
            {
                value = AS_STRING(lit_string_format(state, "\"@\"", lit_value_fromobject(value)));
                lit_state_poproot(state);
                lit_state_pushroot(state, (LitObject*)value);
            }
            valuesconverted[i] = value;
            keys[i] = entry->key;
            objfnstring_length += entry->key->length + 2 + value->length + (i == valueamount - 1 ? 1 : 2) + indentation;
            i++;
        }
    } while(i < valueamount);
    char buffer[objfnstring_length + 1];
    memcpy(buffer, "{\n", 2);
    LitUInt bufferindex = 2;
    for(i = 0; i < valueamount; i++)
    {
        LitString* key = keys[i];
        LitString* value = valuesconverted[i];
        for(LitUInt j = 0; j < indentation; j++)
        {
            buffer[bufferindex++] = '\t';
        }
        memcpy(&buffer[bufferindex], key->chars, key->length);
        bufferindex += key->length;
        memcpy(&buffer[bufferindex], ": ", 2);
        bufferindex += 2;
        memcpy(&buffer[bufferindex], value->chars, value->length);
        bufferindex += value->length;
        if(i == valueamount - 1)
        {
            buffer[bufferindex++] = '\n';
            for(LitUInt j = 0; j < indentation - 1; j++)
            {
                buffer[bufferindex++] = '\t';
            }
            buffer[bufferindex++] = '}';
        }
        else
        {
            memcpy(&buffer[bufferindex], ",\n", 2);
        }
        bufferindex += 2;
        lit_state_poproot(state);
    }
    buffer[objfnstring_length] = '\0';
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, objfnstring_length));
}

LitValue objfnobject_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitObjType type;
    (void)type;
    if(!lit_value_isinstance(instance))
    {
        type = OBJECT_TYPE(instance);
        lit_vm_raisefatalerror(vm, "Can't modify built-in types");
    }
    LitInstance* inst = lit_value_asinstance(instance);
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Object index must be a string");
    }
    if(argc == 2)
    {
        lit_table_set(vm->state, &inst->fields, AS_STRING(args[0]), args[1]);
        return args[1];
    }
    LitValue value;
    if(lit_table_get(&inst->fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_get(&inst->klass->static_fields, AS_STRING(args[0]), &value))
    {
        return value;
    }
    if(lit_table_get(&inst->klass->methods, AS_STRING(args[0]), &value))
    {
        return value;
    }
    return lit_value_makenull();
}

LitValue objfnobject_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);
    LitInstance* self = lit_value_asinstance(instance);
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int value = lit_coreutil_tableiterator(&self->fields, index);
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(value);
}

LitValue objfnobject_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitInstance* self = lit_value_asinstance(instance);
    return lit_coreutil_tableiterkey(&self->fields, index);
}

/*
 * Number
 */

LitValue objfnnumber_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_numbertostring(vm->state, lit_value_asnumber(instance));
}

LitValue objfnnumber_chr(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)args;
    (void)argc;
    char c;
    double dn;
    LitString* cs;
    dn = lit_value_asnumber(instance);
    c = dn;
    cs = lit_string_copy(vm->state, &c, 1);
    return lit_value_fromobject(cs);
}


/*
 * Bool
 */

LitValue objfnbool_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return OBJECT_CONST_STRING(vm->state, lit_value_asbool(instance) ? "true" : "false");
}

/*
 * String
 */

LitValue objfnstring_plus(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    LitString* string = AS_STRING(instance);
    LitValue value = args[0];
    LitString* stringvalue = NULL;
    if(IS_STRING(value))
    {
        stringvalue = AS_STRING(value);
    }
    else
    {
        stringvalue = lit_tostring_value(vm->state, value, 0);
    }
    LitUInt length = string->length + stringvalue->length;
    LitString* result = lit_object_makeemptystring(vm->state, length);
    result->chars = lit_sysmem_malloc((length + 1) * sizeof(char));
    result->chars[length] = '\0';
    memcpy(result->chars, string->chars, string->length);
    memcpy(result->chars + string->length, stringvalue->chars, stringvalue->length);
    result->hash = lit_string_hash(result->chars, result->length);
    lit_string_register(vm->state, result);
    return lit_value_fromobject(result);
}

LitValue objfnstring_compare(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* self;
    LitString* other;
    (void)argc;
    self = AS_STRING(instance);
    if(IS_STRING(args[0]))
    {
        other = AS_STRING(args[0]);
        if(self->length == other->length)
        {
            //fprintf(stderr, "string: same length(self=\"%s\" other=\"%s\")... strncmp=%d\n", self->chars, other->chars, strncmp(self->chars, other->chars, self->length));
            if(memcmp(self->chars, other->chars, self->length) == 0)
            {
                return lit_value_makebool(true);
            }
        }
        return lit_value_makebool(false);
    }
    else if(lit_value_isnull(args[0]))
    {
        if((self == NULL) || lit_value_isnull(instance))
        {
            return lit_value_makebool(true);
        }
        return lit_value_makebool(false);
    }
    lit_vm_raisefatalerror(vm, "can only compare string to another string or null");
    return lit_value_makebool(false);
}

LitValue objfnstring_less(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return lit_value_makebool(strcmp(AS_STRING(instance)->chars, LIT_CHECK_STRING(0)) < 0);
}

LitValue objfnstring_greater(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return lit_value_makebool(strcmp(AS_STRING(instance)->chars, LIT_CHECK_STRING(0)) > 0);
}

LitValue objfnstring_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return instance;
}

LitValue objfnstring_tonumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    double result = strtod(AS_STRING(instance)->chars, NULL);
    if(errno == ERANGE)
    {
        errno = 0;
        return lit_value_makenull();
    }
    return lit_value_makenumber(result);
}

LitValue objfnstring_touppercase(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitString* string = AS_STRING(instance);
    char buffer[string->length];
    for(LitUInt i = 0; i < string->length; i++)
    {
        buffer[i] = (char)toupper(string->chars[i]);
    }
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, string->length));
}

LitValue objfnstring_tolowercase(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitString* string = AS_STRING(instance);
    char buffer[string->length];
    for(LitUInt i = 0; i < string->length; i++)
    {
        buffer[i] = (char)tolower(string->chars[i]);
    }
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, string->length));
}

LitValue objfnstring_contains(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);
    if(sub == string)
    {
        return lit_value_makebool(true);
    }
    return lit_value_makebool(strstr(string->chars, sub->chars) != NULL);
}

LitValue objfnstring_startswith(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);
    if(sub == string)
    {
        return lit_value_makebool(true);
    }
    if(sub->length > string->length)
    {
        return lit_value_makebool(false);
    }
    for(LitUInt i = 0; i < sub->length; i++)
    {
        if(sub->chars[i] != string->chars[i])
        {
            return lit_value_makebool(false);
        }
    }
    return lit_value_makebool(true);
}

LitValue objfnstring_endswith(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);
    if(sub == string)
    {
        return lit_value_makebool(true);
    }
    if(sub->length > string->length)
    {
        return lit_value_makebool(false);
    }
    LitUInt start = string->length - sub->length;
    for(LitUInt i = 0; i < sub->length; i++)
    {
        if(sub->chars[i] != string->chars[i + start])
        {
            return lit_value_makebool(false);
        }
    }
    return lit_value_makebool(true);
}

LitValue objfnstring_replace(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(2);
    if(!IS_STRING(args[0]) || !IS_STRING(args[1]))
    {
        lit_vm_raisefatalerror(vm, "Expected 2 string arguments");
    }
    LitString* string = AS_STRING(instance);
    LitString* what = AS_STRING(args[0]);
    LitString* with = AS_STRING(args[1]);
    LitUInt bufferlength = 0;
    for(LitUInt i = 0; i < string->length; i++)
    {
        if(strncmp(string->chars + i, what->chars, what->length) == 0)
        {
            i += what->length - 1;
            bufferlength += with->length;
        }
        else
        {
            bufferlength++;
        }
    }
    LitUInt bufferindex = 0;
    char buffer[bufferlength + 1];
    for(LitUInt i = 0; i < string->length; i++)
    {
        if(strncmp(string->chars + i, what->chars, what->length) == 0)
        {
            memcpy(buffer + bufferindex, with->chars, with->length);
            bufferindex += with->length;
            i += what->length - 1;
        }
        else
        {
            buffer[bufferindex] = string->chars[i];
            bufferindex++;
        }
    }
    buffer[bufferlength] = '\0';
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, bufferlength));
}

LitValue objfnstring_splice(LitVm* vm, LitString* string, int from, int to)
{
    int length = lit_ustring_length(string);
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
        lit_vm_raisefatalerror(vm, "String splice from bound is larger that to bound");
    }
    from = lit_uchar_offset(string->chars, from);
    to = lit_uchar_offset(string->chars, to);
    return lit_value_fromobject(lit_ustring_from_range(vm->state, string, from, to - from + 1));
}

LitValue objfnstring_substring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);
    return objfnstring_splice(vm, AS_STRING(instance), from, to);
}

LitValue objfnstring_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(IS_RANGE(args[0]))
    {
        LitRange* range = AS_RANGE(args[0]);
        return objfnstring_splice(vm, AS_STRING(instance), range->from, range->to);
    }
    LitString* string = AS_STRING(instance);
    int index = lit_value_asnumber(args[0]);
    if(argc != 1)
    {
        lit_vm_raisefatalerror(vm, "Can't modify strings with the subscript op");
    }
    if(index < 0)
    {
        index = lit_ustring_length(string) + index;
        if(index < 0)
        {
            return lit_value_makenull();
        }
    }
    LitString* c = lit_ustring_code_point_at(vm->state, string, lit_uchar_offset(string->chars, index));
    return c == NULL ? lit_value_makenull() : lit_value_fromobject(c);
}


LitValue objfnstring_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_ustring_length(AS_STRING(instance)));
}

LitValue objfnstring_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    if(lit_value_isnull(args[0]))
    {
        if(string->length == 0)
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
        if(index >= (int)string->length)
        {
            return lit_value_makenull();
        }
    } while((string->chars[index] & 0xc0) == 0x80);
    return lit_value_makenumber(index);
}

LitValue objfnstring_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    uint32_t index = LIT_CHECK_NUMBER(0);
    if(index == UINT32_MAX)
    {
        return lit_value_makebool(false);
    }
    return lit_value_fromobject(lit_ustring_code_point_at(vm->state, string, index));
}

/*
 * Function
 */

LitValue objfnfunction_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_function_getname(vm, instance);
}

LitValue objfnfunction_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_function_getname(vm, instance);
}

/*
 * Fiber
 */

LitValue objfnfiber_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    LitValue arg;
    printf("Fiber.constructor:argc=%d\n", argc);
    if((argc == 0) || (!lit_value_iscallablefunction(args[0])))
    {
        printf("args[0]=");
        lit_value_printvalue(args[0]);
        printf("\n");
        lit_vm_raisefatalerror(vm, "Fiber constructor expects a function as its argument");
    }
    arg = args[0];
    LitModule* module = vm->fiber->module;

    LitFiber* fiber;

    if(IS_FUNCTION(arg))
    {
        fiber = lit_object_makefiber(vm->state, module, AS_FUNCTION(arg));
    }
    else
    {
        fiber = lit_object_makefiberclosure(vm->state, module, AS_CLOSURE(arg));
    }

    fiber->parent = vm->fiber;

    return lit_value_fromobject(fiber);
}

bool lit_coreutil_isfiberdone(LitFiber* fiber)
{
    return fiber->framecount == 0 || fiber->abort;
}

LitValue objfnfiber_done(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makebool(lit_coreutil_isfiberdone(AS_FIBER(instance)));
}

LitValue objfnfiber_error(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return AS_FIBER(instance)->error;
}

LitValue objfnfiber_current(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(vm->fiber);
}

void lit_coreutil_runfiber(LitVm* vm, LitFiber* fiber, LitValue* args, LitUInt argc, bool catcher)
{
    if(lit_coreutil_isfiberdone(fiber))
    {
        lit_vm_raisefatalerror(vm, "Fiber already finished executing");
    }
    fiber->parent = vm->fiber;
    fiber->catcher = catcher;
    vm->fiber = fiber;
    LitCallFrame* frame = &fiber->framevals[fiber->framecount - 1];
    if(frame->ip == frame->function->chunk.compiledcodechunk)
    {
        fiber->argcount = argc;
        LitFunction* function = frame->function;
        LitValue* start = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->max_registers :
                                                   fiber->registers;
        lit_fiber_ensureregisters(fiber, start - fiber->registers + function->max_registers);
        frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->max_registers :
                                                fiber->registers;
        for(int i = argc + 1; i < function->max_registers; i++)
        {
            frame->slots[i] = lit_value_makenull();
        }
        frame->slots[0] = lit_value_fromobject(function);
        for(uint8_t i = 0; i < argc; i++)
        {
            frame->slots[i + 1] = args[i];
        }
        bool vararg = frame->function->vararg;
        LitUInt functionargcount = function->argcount;
        fiber->argcount = functionargcount;
        if(vararg)
        {
            if(functionargcount == argc && IS_VARARG_ARRAY(*(frame->slots + functionargcount)))
            {
                // No need to repack the arguments
            }
            else
            {
                LitArray* array = &lit_object_makevararray(vm->state)->array;
                lit_state_pushroot(vm->state, (LitObject*)array);
                *(frame->slots + functionargcount) = lit_value_fromobject(array);
                int varargcount = argc - functionargcount + 1;
                if(varargcount > 0)
                {
                    lit_dynlistval_ensuresize(&array->values, varargcount);
                    for(int i = 0; i < varargcount; i++)
                    {
                        array->values.values[i] = args[i + functionargcount - 1];
                    }
                }
                lit_state_poproot(vm->state);
            }
        }
    }
    if(vm->state->config.traceexecution)
    {
        fprintf(stderr, "fiber start:\n");
    }
}

bool objfnfiber_run(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    vm->fiber->return_address = args - 1;
    lit_coreutil_runfiber(vm, AS_FIBER(instance), args, argc, false);
    return true;
}

bool objfnfiber_try(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    vm->fiber->return_address = args - 1;
    lit_coreutil_runfiber(vm, AS_FIBER(instance), args, argc, true);
    return true;
}

bool objfnfiber_yield(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    if(vm->fiber->parent == NULL)
    {
        lit_vm_handleerror(vm, argc == 0 ? CONST_STRING(vm->state, "Fiber was yielded") : lit_tostring_value(vm->state, args[0], 0));
        return true;
    }
    vm->fiber = vm->fiber->parent;
    *vm->fiber->return_address = argc == 0 ? lit_value_makenull() : lit_value_fromobject(lit_tostring_value(vm->state, args[0], 0));
    return true;
}

bool objfnfiber_yeet(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    if(vm->fiber->parent == NULL)
    {
        lit_vm_handleerror(vm, argc == 0 ? CONST_STRING(vm->state, "Fiber was yeeted") : lit_tostring_value(vm->state, args[0], 0));
        return true;
    }
    vm->fiber = vm->fiber->parent;
    *vm->fiber->return_address = argc == 0 ? lit_value_makenull() : lit_value_fromobject(lit_tostring_value(vm->state, args[0], 0));
    return true;
}

bool objfnfiber_abort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    LitString* value = argc == 0 ? CONST_STRING(vm->state, "Fiber was aborted") : lit_tostring_value(vm->state, args[0], 0);
    lit_vm_handleerror(vm, value);
    if(vm->fiber->return_address != NULL)
    {
        *vm->fiber->return_address = lit_value_fromobject(value);
    }
    return true;
}

/*
 * Module
 */

LitValue lit_coreutil_accessprivate(LitVm* vm, LitMap* map, LitString* name, LitValue* val)
{
    LitValue value;
    LitString* id = CONST_STRING(vm->state, "_module");
    if(!lit_table_get(&map->values, id, &value) || !IS_MODULE(value))
    {
        return lit_value_makenull();
    }
    LitModule* module = AS_MODULE(value);
    if(id == name)
    {
        return lit_value_fromobject(module);
    }
    if(lit_table_get(&module->private_names->values, name, &value))
    {
        int index = (int)lit_value_asnumber(value);
        if(index > -1 && index < (int)module->private_count)
        {
            if(val != NULL)
            {
                module->privates[index] = *val;
                return *val;
            }
            return module->privates[index];
        }
    }
    return lit_value_makenull();
}

LitValue objfnmodule_privates(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitModule* module = IS_MODULE(instance) ? AS_MODULE(instance) : vm->fiber->module;
    LitMap* map = module->private_names;
    if(map->index_fn == NULL)
    {
        map->index_fn = lit_coreutil_accessprivate;
        lit_table_set(vm->state, &map->values, CONST_STRING(vm->state, "_module"), lit_value_fromobject(module));
    }
    return lit_value_fromobject(map);
}

LitValue objfnmodule_current(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(vm->fiber->module);
}

LitValue objfnmodule_toString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_string_format(vm->state, "Module @", lit_value_fromobject(AS_MODULE(instance)->name));
}

LitValue objfnmodule_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)vm;
    return lit_value_fromobject(AS_MODULE(instance)->name);
}

/*
 * Array
 */

LitValue objfnarray_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_fromobject(lit_object_makearray(vm->state));
}

LitValue objfnarray_splice(LitVm* vm, LitArray* array, int from, int to)
{
    LitUInt length = array->values.count;
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
        lit_vm_raisefatalerror(vm, "String splice from bound is larger that to bound");
    }
    from = fmax(from, 0);
    to = fmin(to, (int)length - 1);
    length = fmin(length, to - from + 1);
    LitArray* newarray = lit_object_makearray(vm->state);
    for(LitUInt i = 0; i < length; i++)
    {
        lit_vallist_push(&newarray->values, array->values.values[from + i]);
    }
    return lit_value_fromobject(newarray);
}

LitValue objfnarray_slice(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);
    return objfnarray_splice(vm, AS_ARRAY(instance), from, to);
}

LitValue objfnarray_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(argc == 2)
    {
        if(!lit_value_isnumber(args[0]))
        {
            lit_vm_raisefatalerror(vm, "Array index must be a number");
        }
        LitDynListVal* values = &AS_ARRAY(instance)->values;
        int index = lit_value_asnumber(args[0]);
        if(index < 0)
        {
            index = fmax(0, values->count + index);
        }
        lit_dynlistval_ensuresize(values, index + 1);
        return values->values[index] = args[1];
    }
    if(!lit_value_isnumber(args[0]))
    {
        if(IS_RANGE(args[0]))
        {
            LitRange* range = AS_RANGE(args[0]);
            return objfnarray_splice(vm, AS_ARRAY(instance), (int)range->from, (int)range->to);
        }
        lit_vm_raisefatalerror(vm, "Array index must be a number");
        return lit_value_makenull();
    }
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    int index = lit_value_asnumber(args[0]);
    if(index < 0)
    {
        index = fmax(0, values->count + index);
    }
    if(values->capacity <= (LitUInt)index)
    {
        return lit_value_makenull();
    }
    return values->values[index];
}

LitValue objfnarray_push(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    lit_vallist_push(&AS_ARRAY(instance)->values, args[0]);
    return lit_value_makenull();
}

LitValue objfnarray_insert(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(2)
    LitDynListVal* values = &AS_ARRAY(instance)->values;
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
        for(int i = values->count - 1; i > index; i--)
        {
            values->values[i] = values->values[i - 1];
        }
    }
    values->values[index] = value;
    return lit_value_makenull();
}

LitValue objfnarray_addall(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    if(!IS_ARRAY(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Expected array as the argument");
    }
    LitArray* array = AS_ARRAY(instance);
    LitArray* toAdd = AS_ARRAY(args[0]);
    for(LitUInt i = 0; i < toAdd->values.count; i++)
    {
        lit_vallist_push(&array->values, toAdd->values.values[i]);
    }
    return lit_value_makenull();
}

int lit_coreutil_indexof(LitState* state, LitArray* array, LitValue value)
{
    LitValue* ptr;
    for(LitUInt i = 0; i < array->values.count; i++)
    {
        ptr = &array->values.values[i];
        if(lit_value_compare(state, *ptr, value))
        {
            return (int)i;
        }
    }
    return -1;
}

LitValue objfnarray_indexof(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    int index = lit_coreutil_indexof(vm->state, AS_ARRAY(instance), args[0]);
    return index == -1 ? lit_value_makenull() : lit_value_makenumber(index);
}

LitValue lit_coreutil_removeat(LitArray* array, LitUInt index)
{
    LitDynListVal* values = &array->values;
    LitUInt count = values->count;
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
        for(LitUInt i = index; i < values->count - 1; i++)
        {
            values->values[i] = values->values[i + 1];
        }
        values->values[count - 1] = lit_value_makenull();
    }
    values->count--;
    return value;
}

LitValue objfnarray_remove(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitArray* array = AS_ARRAY(instance);
    int index = lit_coreutil_indexof(vm->state, array, args[0]);
    if(index != -1)
    {
        return lit_coreutil_removeat(array, (LitUInt)index);
    }
    return lit_value_makenull();
}

LitValue objfnarray_removeat(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int index = LIT_CHECK_NUMBER(0);
    if(index < 0)
    {
        return lit_value_makenull();
    }
    return lit_coreutil_removeat(AS_ARRAY(instance), (LitUInt)index);
}

LitValue objfnarray_contains(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    return lit_value_makebool(lit_coreutil_indexof(vm->state, AS_ARRAY(instance), args[0]) != -1);
}

LitValue objfnarray_clear(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    AS_ARRAY(instance)->values.count = 0;
    return lit_value_makenull();
}

LitValue objfnarray_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitArray* array = AS_ARRAY(instance);
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

LitValue objfnarray_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    if(values->count <= index)
    {
        return lit_value_makenull();
    }
    return values->values[index];
}

LitValue objfnarray_foreach(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitValue callback = args[0];
    if(!lit_value_iscallablefunction(callback))
    {
        lit_vm_raisefatalerror(vm, "Expected a function as the callback");
    }
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    for(LitUInt i = 0; i < values->count; i++)
    {
        lit_state_callvalue(vm->state, callback, &values->values[i], 1);
    }
    return lit_value_makenull();
}

LitValue objfnarray_join(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    LitString* strings[values->count];
    LitUInt length = 0;
    for(LitUInt i = 0; i < values->count; i++)
    {
        LitString* string = lit_tostring_value(vm->state, values->values[i], 0);
        strings[i] = string;
        length += string->length;
    }
    LitUInt index = 0;
    char chars[length + 1];
    chars[length] = '\0';
    for(LitUInt i = 0; i < values->count; i++)
    {
        LitString* string = strings[i];
        memcpy(chars + index, string->chars, string->length);
        index += string->length;
    }
    return lit_value_fromobject(lit_string_copy(vm->state, chars, length));
}

bool sort_compare(LitState* state, LitValue a, LitValue b)
{
    if(lit_value_isnumber(a) && lit_value_isnumber(b))
    {
        return lit_value_asnumber(a) < lit_value_asnumber(b);
    }
    return !lit_is_falsey(lit_state_findandcallmethod(state, a, CONST_STRING(state, "<"), (LitValue[1]){ b }, 1).result);
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

void lit_coreutil_customquicksort(LitVm* vm, LitValue* l, int length, LitValue callee)
{
    if(length < 2)
    {
        return;
    }
    LitState* state = vm->state;
    int pivotindex = length / 2;
    int i;
    int j;
    LitValue pivot = l[pivotindex];
#define COMPARE(a, b)                                                             \
    ({                                                                            \
        LitResult r = lit_state_callvalue(state, callee, (LitValue[2]){ a, b }, 2); \
        if(r.type != INTERPRET_OK)                                                \
            return;                                                               \
        !lit_is_falsey(r.result);                                                 \
    })
    for(i = 0, j = length - 1;; i++, j--)
    {
        while(i < pivotindex && COMPARE(l[i], pivot))
        {
            i++;
        }
        while(j > pivotindex && COMPARE(pivot, l[j]))
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
#undef COMPARE
    lit_coreutil_customquicksort(vm, l, i, callee);
    lit_coreutil_customquicksort(vm, l + i, length - i, callee);
}

LitValue objfnarray_sort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    if(argc == 1 && lit_value_iscallablefunction(args[0]))
    {
        lit_coreutil_customquicksort(vm, values->values, values->count, args[0]);
    }
    else
    {
        lit_coreutil_basicquicksort(vm->state, values->values, values->count);
    }
    return instance;
}

LitValue objfnarray_clone(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitState* state = vm->state;
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    LitArray* array = lit_object_makearray(state);
    LitDynListVal* newvalues = &array->values;
    lit_dynlistval_ensuresize(newvalues, values->count);
    // lit_dynlistval_ensuresize sets the count to max of previous count (0 in this case) and new count, so we have to reset it
    newvalues->count = 0;
    for(LitUInt i = 0; i < values->count; i++)
    {
        lit_vallist_push(newvalues, values->values[i]);
    }
    return lit_value_fromobject(array);
}

LitValue objfnarray_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt indentation = LIT_SINGLE_LINE_MAPS_ENABLED ? 0 : LIT_GET_NUMBER(0, 0) + 1;
    LitDynListVal* values = &AS_ARRAY(instance)->values;
    LitState* state = vm->state;
    if(values->count == 0)
    {
        return OBJECT_CONST_STRING(state, "[]");
    }
    bool hasmore = values->count > LIT_CONTAINER_OUTPUT_MAX;
    LitUInt valueamount = hasmore ? LIT_CONTAINER_OUTPUT_MAX : values->count;
    LitString* valuesconverted[valueamount];
    LitUInt slength = 3;// "[ ]"
    if(hasmore)
    {
        slength += 3;
    }
    for(LitUInt i = 0; i < valueamount; i++)
    {
        LitValue field = values->values[(hasmore && i == valueamount - 1) ? values->count - 1 : i];
        LitString* value = lit_tostring_value(state, field, indentation);
        lit_state_pushroot(state, (LitObject*)value);
        if(IS_STRING(field))
        {
            value = AS_STRING(lit_string_format(state, "\"@\"", lit_value_fromobject(value)));
        }
        valuesconverted[i] = value;
        slength += value->length + (i == valueamount - 1 ? 1 : 2);
    }
    char buffer[slength + 1];
    memcpy(buffer, "[ ", 2);
    LitUInt bufferindex = 2;
    for(LitUInt i = 0; i < valueamount; i++)
    {
        LitString* part = valuesconverted[i];
        memcpy(&buffer[bufferindex], part->chars, part->length);
        bufferindex += part->length;
        if(hasmore && i == valueamount - 2)
        {
            memcpy(&buffer[bufferindex], " ... ", 5);
            bufferindex += 5;
        }
        else
        {
            memcpy(&buffer[bufferindex], (i == valueamount - 1) ? " ]" : ", ", 2);
            bufferindex += 2;
        }
        lit_state_poproot(state);
    }
    buffer[slength] = '\0';
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, slength));
}

LitValue objfnarray_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    (void)vm;
    return lit_value_makenumber(AS_ARRAY(instance)->values.count);
}

/*
 * Map
 */

LitValue objfnmap_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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
    return lit_value_fromobject(lit_object_makemap(vm->state, pfields));
}

LitValue objfnmap_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(!IS_STRING(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Map index must be a string");
    }
    LitMap* map = AS_MAP(instance);
    LitString* index = AS_STRING(args[0]);
    if(argc == 2)
    {
        LitValue val = args[1];
        if(map->index_fn != NULL)
        {
            return map->index_fn(vm, map, index, &val);
        }
        lit_map_set(vm->state, map, index, val);
        return val;
    }
    LitValue value;
    if(map->index_fn != NULL)
    {
        return map->index_fn(vm, map, index, NULL);
    }
    if(!lit_table_get(&map->values, index, &value))
    {
        return lit_value_makenull();
    }
    return value;
}

LitValue objfnmap_addall(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    if(!lit_value_ismap(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Expected map as the argument");
    }
    lit_map_add_all(vm->state, AS_MAP(args[0]), AS_MAP(instance));
    return lit_value_makenull();
}

LitValue objfnmap_clear(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    AS_MAP(instance)->values.count = 0;
    return lit_value_makenull();
}

LitValue objfnmap_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    int index = lit_value_isnull(args[0]) ? -1 : lit_value_asnumber(args[0]);
    int value = lit_coreutil_tableiterator(&AS_MAP(instance)->values, index);
    return value == -1 ? lit_value_makenull() : lit_value_makenumber(value);
}

LitValue objfnmap_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    return lit_coreutil_tableiterkey(&AS_MAP(instance)->values, index);
}

LitValue objfnmap_foreach(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitValue callback = args[0];
    if(!lit_value_iscallablefunction(callback))
    {
        lit_vm_raisefatalerror(vm, "Expected a function as the callback");
    }
    LitTable* values = &AS_MAP(instance)->values;
    for(int i = 0; i < values->capacity; i++)
    {
        LitTabEntry* entry = &values->entries[i];
        if(entry->key != NULL)
        {
            lit_state_callvalue(vm->state, callback, (LitValue[2]){ lit_value_fromobject(entry->key), entry->value }, 2);
        }
    }
    return lit_value_makenull();
}

LitValue objfnmap_clone(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitState* state = vm->state;
    LitMap* map = lit_object_makemap(state, NULL);
    lit_table_add_all(state, &AS_MAP(instance)->values, &map->values);
    return lit_value_fromobject(map);
}

LitValue objfnmap_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitState* state = vm->state;
    LitMap* map = AS_MAP(instance);
    LitTable* values = &map->values;
    if(values->count == 0)
    {
        return OBJECT_CONST_STRING(state, "{}");
    }
    bool haswrapper = map->index_fn != NULL;
    bool hasmore = values->count > LIT_CONTAINER_OUTPUT_MAX;
    LitUInt valueamount = hasmore ? LIT_CONTAINER_OUTPUT_MAX : values->count;
    LitString* valuesconverted[valueamount];
    LitString* keys[valueamount];
    LitUInt indentation = LIT_SINGLE_LINE_MAPS_ENABLED ? 0 : LIT_GET_NUMBER(0, 0) + 1;
    LitUInt slength = (LIT_SINGLE_LINE_MAPS_ENABLED ? 3 : 2) + indentation;
    if(hasmore)
    {
        slength += LIT_SINGLE_LINE_MAPS_ENABLED ? 5 : 6;
    }
    LitUInt i = 0;
    LitUInt index = 0;
    do
    {
        LitTabEntry* entry = &values->entries[index++];
        if(entry->key != NULL)
        {
            // Special hidden key
            LitValue field = haswrapper ? map->index_fn(vm, map, entry->key, NULL) : entry->value;
            // This check is required to prevent infinite loops when playing with Module.privates and such
            LitString* value = (lit_value_ismap(field) && AS_MAP(field)->index_fn != NULL) ? CONST_STRING(state, "map") : lit_tostring_value(state, field, indentation);
            lit_state_pushroot(state, (LitObject*)value);
            if(IS_STRING(field))
            {
                value = AS_STRING(lit_string_format(state, "\"@\"", lit_value_fromobject(value)));
            }
            valuesconverted[i] = value;
            keys[i] = entry->key;
            slength += entry->key->length + 2 + value->length + (i == valueamount - 1 ? 1 : 2) + indentation;
            i++;
        }
    } while(i < valueamount);
    char buffer[slength + 1];
#ifdef LIT_SINGLE_LINE_MAPS
    memcpy(buffer, "{ ", 2);
#else
    memcpy(buffer, "{\n", 2);
#endif
    LitUInt bufferindex = 2;
    for(i = 0; i < valueamount; i++)
    {
        LitString* key = keys[i];
        LitString* value = valuesconverted[i];
        for(LitUInt j = 0; j < indentation; j++)
        {
            buffer[bufferindex++] = '\t';
        }
        memcpy(&buffer[bufferindex], key->chars, key->length);
        bufferindex += key->length;
        memcpy(&buffer[bufferindex], ": ", 2);
        bufferindex += 2;
        memcpy(&buffer[bufferindex], value->chars, value->length);
        bufferindex += value->length;
        if(hasmore && i == valueamount - 1)
        {
#ifdef LIT_SINGLE_LINE_MAPS
            memcpy(&buffer[bufferindex], ", ... }", 7);
#else
            memcpy(&buffer[bufferindex], ",\n\t...\n", 7);
            bufferindex += 7;
            for(LitUInt j = 0; j < indentation - 1; j++)
            {
                buffer[bufferindex++] = '\t';
            }
            buffer[bufferindex++] = '}';
#endif
        }
        else
        {
#ifdef LIT_SINGLE_LINE_MAPS
            memcpy(&buffer[bufferindex], (i == valueamount - 1) ? " }" : ", ", 2);
#else
            if(i == valueamount - 1)
            {
                buffer[bufferindex++] = '\n';
                for(LitUInt j = 0; j < indentation - 1; j++)
                {
                    buffer[bufferindex++] = '\t';
                }
                buffer[bufferindex++] = '}';
            }
            else
            {
                memcpy(&buffer[bufferindex], ",\n", 2);
            }
#endif
            bufferindex += 2;
        }
        lit_state_poproot(state);
    }
    buffer[slength] = '\0';
    return lit_value_fromobject(lit_string_copy(vm->state, buffer, slength));
}

LitValue objfnmap_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber(AS_MAP(instance)->values.count);
}

/*
 * Range
 */

LitValue objfnrange_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
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

LitValue objfnrange_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    LIT_ENSURE_ARGS(1)
    return args[0];
}

LitValue objfnrange_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitRange* range = AS_RANGE(instance);
    return lit_string_format(vm->state, "Range(#, #)", range->from, range->to);
}

LitValue objfnrange_from(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber(AS_RANGE(instance)->from);
}

LitValue objfnrange_setfrom(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    AS_RANGE(instance)->from = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue objfnrange_to(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber(AS_RANGE(instance)->to);
}

LitValue objfnrange_setto(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    AS_RANGE(instance)->to = lit_value_asnumber(args[0]);
    return args[0];
}

LitValue objfnrange_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    LitRange* range = AS_RANGE(instance);
    return lit_value_makenumber(range->to - range->from);
}

/*
 * Natives
 */

LitValue lit_corefn_time(LitVm* vm, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber((double)clock() / CLOCKS_PER_SEC);
}

LitValue lit_corefn_systemtime(LitVm* vm, LitUInt argc, LitValue* args)
{
    (void)vm;
    (void)argc;
    (void)args;
    return lit_value_makenumber(time(NULL));
}

LitValue lit_corefn_print(LitVm* vm, LitUInt argc, LitValue* args)
{
    LitString* s;
    if(argc == 0)
    {
        return lit_value_makenull();
    }
    for(LitUInt i = 0; i < argc; i++)
    {
        s = lit_tostring_value(vm->state, args[i], 0);
        lit_printf(vm->state, "%.*s", s->length, s->chars);
    }
    return lit_value_makenull();
}


LitValue lit_corefn_println(LitVm* vm, LitUInt argc, LitValue* args)
{
    LitValue r;
    r = lit_corefn_print(vm, argc, args);
    lit_printf(vm->state, "\n");
    return r;
}

LitValue lit_corefn_openlibrary(LitVm* vm, LitUInt argc, LitValue* args)
{
    const char* name = LIT_CHECK_STRING(0);
    /*
    if(strcmp(name, "network") == 0)
    {
        lit_open_network_library(vm->state);
    }
    else
    */
    {
        lit_vm_raisefatalerror(vm, "Unknown built-in library %s", name);
    }
    return lit_value_makenull();
}

bool interpret(LitVm* vm, LitModule* module)
{
    LitFunction* function = module->main_function;
    LitFiber* fiber = lit_object_makefiber(vm->state, module, function);
    fiber->parent = vm->fiber;
    vm->fiber = fiber;
    return true;
}

bool lit_coreutil_compileandinterpret(LitVm* vm, LitString* modname, char* source)
{
    LitModule* module = lit_state_compilemodulesource(vm->state, modname, source);
    if(module == NULL)
    {
        return false;
    }
    module->ran = true;
    return interpret(vm, module);
}

bool lit_corefn_eval(LitVm* vm, LitUInt argc, LitValue* args)
{
    char* code = (char*)LIT_CHECK_STRING(0);
    return lit_coreutil_compileandinterpret(vm, vm->fiber->module->name, code);
}

void lit_state_opencorelibrary(LitState* state)
{
    LitClass* klass;
    bool wasallowed = state->allow_gc;
    state->allow_gc = false; 
    {
        klass = lit_class_make(state, "Class", NULL);
        lit_class_bindmethod(state, klass, "toString", objfnclass_tostring);
        lit_class_bindmethod(state, klass, "[]", objfnclass_subscript);
        lit_class_bindstaticmethod(state, klass, "toString", objfnclass_tostring);
        lit_class_bindstaticmethod(state, klass, "iterator", objfnclass_iterator);
        lit_class_bindstaticmethod(state, klass, "iteratorValue", objfnclass_itervalue);
        lit_class_bindgetsetter(state, klass, "super", objfnclass_super, NULL);
        lit_class_bindstaticgetter(state, klass, "super", objfnclass_super);
        lit_class_bindstaticgetter(state, klass, "name", objfnclass_name);
        state->class_class = klass;
        lit_state_setglobal(state, klass->name, lit_value_fromobject(klass));
        state->allow_gc = wasallowed;    
    }
    {
        klass = lit_class_make(state, "Object", NULL);
        lit_class_inherit(state, klass, state->class_class);
        lit_class_bindmethod(state, klass, "toString", objfnobject_tostring);
        lit_class_bindmethod(state, klass, "[]", objfnobject_subscript);
        lit_class_bindmethod(state, klass, "iterator", objfnobject_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", objfnobject_itervalue);
        lit_class_bindgetsetter(state, klass, "class", objfnobject_class, NULL);
        state->object_class = klass;
        state->object_class->super = state->class_class;
    }
    {
        klass = lit_class_make(state, "Number", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindgetsetter(state, klass, "chr", objfnnumber_chr, NULL);
        lit_class_bindmethod(state, klass, "toString", objfnnumber_tostring);
        state->number_class = klass;
    }
    {
        klass = lit_class_make(state, "String", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindmethod(state, klass, "+", objfnstring_plus);
        //lit_class_bindmethod(state, klass, "<", objfnstring_less);
        //lit_class_bindmethod(state, klass, ">", objfnstring_greater);
        lit_class_bindmethod(state, klass, "==", objfnstring_compare);
        lit_class_bindmethod(state, klass, "toString", objfnstring_tostring);
        lit_class_bindmethod(state, klass, "toNumber", objfnstring_tonumber);
        lit_class_bindmethod(state, klass, "toUpperCase", objfnstring_touppercase);
        lit_class_bindmethod(state, klass, "toLowerCase", objfnstring_tolowercase);
        lit_class_bindmethod(state, klass, "contains", objfnstring_contains);
        lit_class_bindmethod(state, klass, "startsWith", objfnstring_startswith);
        lit_class_bindmethod(state, klass, "endsWith", objfnstring_endswith);
        lit_class_bindmethod(state, klass, "replace", objfnstring_replace);
        lit_class_bindmethod(state, klass, "substring", objfnstring_substring);
        lit_class_bindmethod(state, klass, "iterator", objfnstring_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", objfnstring_itervalue);
        lit_class_bindmethod(state, klass, "[]", objfnstring_subscript);
        lit_class_bindmethod(state, klass, "charCodeAt", objfnstring_subscript);
        lit_class_bindgetsetter(state, klass, "length", objfnstring_length, NULL);
        state->string_class = klass;
    }
    {
        klass = lit_class_make(state, "Bool", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindmethod(state, klass, "toString", objfnbool_tostring);
        state->bool_class = klass;
    }
    {
        klass = lit_class_make(state, "Function", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindmethod(state, klass, "toString", objfnfunction_tostring);
        lit_class_bindgetsetter(state, klass, "name", objfnfunction_name, NULL);
        state->function_class = klass;
    }
    {
        klass = lit_class_make(state, "Fiber", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, objfnfiber_constructor);
        lit_class_bindprimitive(state, klass, "run", objfnfiber_run);
        lit_class_bindprimitive(state, klass, "try", objfnfiber_try);
        lit_class_bindgetsetter(state, klass, "done", objfnfiber_done, NULL);
        lit_class_bindgetsetter(state, klass, "error", objfnfiber_error, NULL);
        lit_class_bindstaticprimitive(state, klass, "yield", objfnfiber_yield);
        lit_class_bindstaticprimitive(state, klass, "yeet", objfnfiber_yeet);
        lit_class_bindstaticprimitive(state, klass, "abort", objfnfiber_abort);
        lit_class_bindstaticgetter(state, klass, "current", objfnfiber_current);
        state->fiber_class = klass;
    }
    {
        klass = lit_class_make(state, "Module", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_setstaticfield(state, klass, "loaded", lit_value_fromobject(state->vm->modules));
        lit_class_bindstaticgetter(state, klass, "privates", objfnmodule_privates);
        lit_class_bindstaticgetter(state, klass, "current", objfnmodule_current);
        lit_class_bindmethod(state, klass, "toString", objfnmodule_toString);
        lit_class_bindgetsetter(state, klass, "name", objfnmodule_name, NULL);
        lit_class_bindgetsetter(state, klass, "privates", objfnmodule_privates, NULL);
        state->module_class = klass;
    }
    {
        klass = lit_class_make(state, "Array", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, objfnarray_constructor);
        lit_class_bindmethod(state, klass, "[]", objfnarray_subscript);
        lit_class_bindmethod(state, klass, "add", objfnarray_push);
        lit_class_bindmethod(state, klass, "push", objfnarray_push);
        lit_class_bindmethod(state, klass, "insert", objfnarray_insert);
        lit_class_bindmethod(state, klass, "slice", objfnarray_slice);
        lit_class_bindmethod(state, klass, "addAll", objfnarray_addall);
        lit_class_bindmethod(state, klass, "remove", objfnarray_remove);
        lit_class_bindmethod(state, klass, "removeAt", objfnarray_removeat);
        lit_class_bindmethod(state, klass, "indexxOf", objfnarray_indexof);
        lit_class_bindmethod(state, klass, "contains", objfnarray_contains);
        lit_class_bindmethod(state, klass, "clear", objfnarray_clear);
        lit_class_bindmethod(state, klass, "iterator", objfnarray_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", objfnarray_itervalue);
        lit_class_bindmethod(state, klass, "forEach", objfnarray_foreach);
        lit_class_bindmethod(state, klass, "join", objfnarray_join);
        lit_class_bindmethod(state, klass, "sort", objfnarray_sort);
        lit_class_bindmethod(state, klass, "clone", objfnarray_clone);
        lit_class_bindmethod(state, klass, "toString", objfnarray_tostring);
        lit_class_bindgetsetter(state, klass, "length", objfnarray_length, NULL);
        state->array_class = klass;
    }
    {
        klass = lit_class_make(state, "Map", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, objfnmap_constructor);
        lit_class_bindmethod(state, klass, "[]", objfnmap_subscript);
        lit_class_bindmethod(state, klass, "addAll", objfnmap_addall);
        lit_class_bindmethod(state, klass, "clear", objfnmap_clear);
        lit_class_bindmethod(state, klass, "iterator", objfnmap_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", objfnmap_itervalue);
        lit_class_bindmethod(state, klass, "forEach", objfnmap_foreach);
        lit_class_bindmethod(state, klass, "clone", objfnmap_clone);
        lit_class_bindmethod(state, klass, "toString", objfnmap_tostring);
        lit_class_bindgetsetter(state, klass, "length", objfnmap_length, NULL);
        state->map_class = klass;
    }
    {
        klass = lit_class_make(state, "Range", state->object_class);
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindmethod(state, klass, "iterator", objfnrange_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", objfnrange_itervalue);
        lit_class_bindmethod(state, klass, "toString", objfnrange_tostring);
        lit_class_bindgetsetter(state, klass, "from", objfnrange_from, objfnrange_setfrom);
        lit_class_bindgetsetter(state, klass, "to", objfnrange_to, objfnrange_setto);
        lit_class_bindgetsetter(state, klass, "length", objfnrange_length, NULL);
        state->range_class = klass;
    }
    state->allow_gc = wasallowed;
    lit_state_defnative(state, "time", lit_corefn_time);
    lit_state_defnative(state, "systemTime", lit_corefn_systemtime);
    lit_state_defnative(state, "print", lit_corefn_print);
    lit_state_defnative(state, "println", lit_corefn_println);
    lit_state_defnative(state, "openLibrary", lit_corefn_openlibrary);
    lit_state_defnativeprimitive(state, "eval", lit_corefn_eval);
    lit_state_setglobal(state, CONST_STRING(state, "globals"), lit_value_fromobject(state->vm->globals));
}





static uint8_t btmp;
static uint16_t stmp;
static uint32_t itmp;
static double dtmp;


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
    return stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode);
}

bool lit_dir_exists(const char* path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0 && S_ISDIR(buffer.st_mode);
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

void lit_write_uint64_t(FILE* file, uint64_t byte)
{
    fwrite(&byte, sizeof(uint64_t), 1, file);
}

void lit_write_double(FILE* file, double byte)
{
    fwrite(&byte, sizeof(double), 1, file);
}

void lit_write_string(FILE* file, LitString* string)
{
    uint16_t c = string->length;
    fwrite(&c, sizeof(uint16_t), 1, file);
    for(uint16_t i = 0; i < c; i++)
    {
        lit_write_uint8_t(file, (uint8_t)string->chars[i] ^ LIT_STRING_KEY);
    }
}

uint8_t lit_read_uint8_t(FILE* file)
{
    size_t rsz;
    (void)rsz;
    rsz = fread(&btmp, sizeof(uint8_t), 1, file);
    return btmp;
}

uint16_t lit_read_uint16_t(FILE* file)
{
    size_t rsz;
    (void)rsz;
    rsz = fread(&stmp, sizeof(uint16_t), 1, file);
    return stmp;
}

uint32_t lit_read_uint32_t(FILE* file)
{
    size_t rsz;
    (void)rsz;
    rsz = fread(&itmp, sizeof(uint32_t), 1, file);
    return itmp;
}

double lit_read_double(FILE* file)
{
    size_t rsz;
    (void)rsz;
    rsz = fread(&dtmp, sizeof(double), 1, file);
    return dtmp;
}

LitString* lit_read_string(LitState* state, FILE* file)
{
    size_t rsz;
    uint16_t length;
    (void)rsz;
    rsz = fread(&length, sizeof(uint16_t), 1, file);
    if(length < 1)
    {
        return NULL;
    }
    char line[length];
    for(uint16_t i = 0; i < length; i++)
    {
        line[i] = (char)lit_read_uint8_t(file) ^ LIT_STRING_KEY;
    }
    return lit_string_copy(state, line, length);
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
    uint8_t values[8];
    double result;
    for(LitUInt i = 0; i < 8; i++)
    {
        values[i] = lit_read_euint8_t(file);
    }
    memcpy(&result, values, 8);
    return result;
}

LitString* lit_read_estring(LitState* state, LitEmulatedFile* file)
{
    uint16_t length = lit_read_euint16_t(file);
    if(length < 1)
    {
        return NULL;
    }
    char line[length];
    for(uint16_t i = 0; i < length; i++)
    {
        line[i] = (char)lit_read_euint8_t(file) ^ LIT_STRING_KEY;
    }
    return lit_string_copy(state, line, length);
}

void save_function(FILE* file, LitFunction* function)
{
    save_chunk(file, &function->chunk);
    lit_write_string(file, function->name);
    lit_write_uint8_t(file, function->argcount);
    lit_write_uint32_t(file, function->upvaluecount);
    lit_write_uint8_t(file, (uint8_t)function->vararg);
    lit_write_uint8_t(file, (uint16_t)function->max_registers);
}

LitFunction* load_function(LitState* state, LitEmulatedFile* file, LitModule* module)
{
    LitFunction* function = lit_object_makefunction(state, module);
    load_chunk(state, file, module, &function->chunk);
    function->name = lit_read_estring(state, file);
    function->argcount = lit_read_euint8_t(file);
    function->upvaluecount = lit_read_euint16_t(file);
    function->vararg = (bool)lit_read_euint8_t(file);
    function->max_registers = lit_read_euint8_t(file);
    return function;
}

void save_chunk(FILE* file, LitChunk* chunk)
{
    lit_write_uint32_t(file, chunk->compiledcodecount);
    for(LitUInt i = 0; i < chunk->compiledcodecount; i++)
    {
        lit_write_uint64_t(file, chunk->compiledcodechunk[i]);
    }
    if(chunk->haslineinfo)
    {
        LitUInt c = chunk->linecount * 2 + 2;
        lit_write_uint32_t(file, c);
        for(LitUInt i = 0; i < c; i++)
        {
            lit_write_uint16_t(file, chunk->lines[i]);
        }
    }
    else
    {
        lit_write_uint32_t(file, 0);
    }
    lit_write_uint32_t(file, chunk->constants.count);
    for(LitUInt i = 0; i < chunk->constants.count; i++)
    {
        LitValue constant = chunk->constants.values[i];
        if(lit_value_isobject(constant))
        {
            LitObjType type = lit_value_asobject(constant)->type;
            lit_write_uint8_t(file, (uint8_t)(type + 1));
            switch(type)
            {
                case LIT_OBJ_STRING:
                {
                    lit_write_string(file, AS_STRING(constant));
                    break;
                }
                case LIT_OBJ_FUNCTION:
                {
                    save_function(file, AS_FUNCTION(constant));
                    break;
                }
                default:
                {
                    UNREACHABLE
                    break;
                }
            }
        }
        else
        {
            lit_write_uint8_t(file, 0);
            lit_write_double(file, lit_value_asnumber(constant));
        }
    }
}

void load_chunk(LitState* state, LitEmulatedFile* file, LitModule* module, LitChunk* chunk)
{
    lit_chunk_init(chunk);
    LitUInt count = lit_read_euint32_t(file);
    chunk->compiledcodechunk = (uint64_t*)lit_sysmem_malloc(sizeof(uint64_t) * count);
    chunk->compiledcodecount = count;
    chunk->capacity = count;
    for(LitUInt i = 0; i < count; i++)
    {
        chunk->compiledcodechunk[i] = lit_read_euint64_t(file);
    }
    count = lit_read_euint32_t(file);
    if(count > 0)
    {
        chunk->lines = (uint16_t*)lit_sysmem_malloc(sizeof(uint16_t) * count);
        chunk->linecount = count;
        chunk->linecapacity = count;
        for(LitUInt i = 0; i < count; i++)
        {
            chunk->lines[i] = lit_read_euint16_t(file);
        }
    }
    else
    {
        chunk->haslineinfo = false;
    }
    count = lit_read_euint32_t(file);
    chunk->constants.values = (LitValue*)lit_sysmem_malloc(sizeof(LitValue) * count);
    chunk->constants.count = count;
    chunk->constants.capacity = count;
    for(LitUInt i = 0; i < count; i++)
    {
        uint8_t type = lit_read_euint8_t(file);
        if(type == 0)
        {
            chunk->constants.values[i] = lit_value_makenumber(lit_read_edouble(file));
        }
        else
        {
            switch((LitObjType)(type - 1))
            {
                case LIT_OBJ_STRING:
                {
                    chunk->constants.values[i] = lit_value_fromobject(lit_read_estring(state, file));
                    break;
                }
                case LIT_OBJ_FUNCTION:
                {
                    chunk->constants.values[i] = lit_value_fromobject(load_function(state, file, module));
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
    bool disabled;
    disabled = false;
    lit_write_string(file, module->name);
    lit_write_uint16_t(file, module->private_count);
    lit_write_uint8_t(file, (uint8_t)disabled);
    if(!disabled)
    {
        LitTable* privates = &module->private_names->values;
        for(LitUInt i = 0; i < module->private_count; i++)
        {
            if(privates->entries[i].key != NULL)
            {
                lit_write_string(file, privates->entries[i].key);
                lit_write_uint16_t(file, (uint16_t)lit_value_asnumber(privates->entries[i].value));
            }
        }
    }
    save_function(file, module->main_function);
}

LitModule* lit_load_module(LitState* state, const char* input)
{
    LitEmulatedFile file;
    lit_init_emulated_file(&file, input);
    if(lit_read_euint16_t(&file) != LIT_BYTECODE_MAGIC_NUMBER)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown magic number");
        return NULL;
    }
    uint8_t bytecodeversion = lit_read_euint8_t(&file);
    if(bytecodeversion > LIT_BYTECODE_VERSION)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown bytecode version '%i'", (int)bytecodeversion);
        return NULL;
    }
    uint16_t modulecount = lit_read_euint16_t(&file);
    LitModule* first = NULL;
    for(uint16_t j = 0; j < modulecount; j++)
    {
        LitModule* module = lit_object_makemodule(state, lit_read_estring(state, &file));
        LitTable* privates = &module->private_names->values;
        uint16_t privatescount = lit_read_euint16_t(&file);
        bool enabled = !((bool)lit_read_euint8_t(&file));
        module->privates = lit_sysmem_malloc(privatescount * sizeof(LitValue));
        module->private_count = privatescount;
        for(uint16_t i = 0; i < privatescount; i++)
        {
            module->privates[i] = lit_value_makenull();
            if(enabled)
            {
                LitString* name = lit_read_estring(state, &file);
                lit_table_set(state, privates, name, lit_value_makenumber(lit_read_euint16_t(&file)));
            }
        }
        module->main_function = load_function(state, &file, module);
        lit_table_set(state, &state->vm->modules->values, module->name, lit_value_fromobject(module));
        if(j == 0)
        {
            first = module;
        }
    }
    if(lit_read_euint16_t(&file) != LIT_BYTECODE_END_NUMBER)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown end number");
        return NULL;
    }
    return first;
}

LitClass* lit_class_make(LitState* state, const char* name, LitClass* super)
{
    LitClass* klass = lit_object_makeclass(state, lit_string_copy(state, name, strlen(name)));
    lit_state_setglobal(state, klass->name, lit_value_fromobject(klass));
    if(super != NULL)
    {
        lit_class_inherit(state, klass, super);
    }                       
    return klass;
}

void lit_class_bindstaticgetter(LitState* state, LitClass* klass, const char* name, LitNativeMethodFn getter)
{
    LitString* nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &klass->static_fields, nm, lit_value_fromobject(lit_object_makefield(state, (LitObject*)lit_object_makenativemethod(state, getter, nm), NULL)));
}

void lit_class_inherit(LitState* state, LitClass* selfclass, LitClass* other)
{
    selfclass->super = (LitClass*)other;
    if(selfclass->init_method == NULL)
    {
        selfclass->init_method = other->init_method;
    }
    lit_table_add_all_ignoring(state, &other->methods, &selfclass->methods);
    lit_table_add_all_ignoring(state, &other->static_fields, &selfclass->static_fields);
}

void lit_class_bindmethod(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->methods, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}


void lit_class_bindprimitive(LitState* state, LitClass* selfclass, const char* name, LitPrimitiveMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->methods, nm, lit_value_fromobject(lit_object_makeprimitivemethod(state, fn, nm)));
}

void lit_class_bindconstructor(LitState* state, LitClass* selfclass, LitNativeMethodFn fn)
{
    const char* fname;
    fname = "constructor";
    LitString* nm;
    LitNativeMethod* meth;
    nm = lit_string_copy(state, fname, strlen(fname));
    meth = lit_object_makenativemethod(state, fn, nm);
    selfclass->init_method = (LitObject*)meth;
    lit_table_set(state, &selfclass->methods, nm, lit_value_fromobject(meth));
}

void lit_class_bindstaticmethod(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, lit_value_fromobject(lit_object_makenativemethod(state, fn, nm)));
}

void lit_class_bindstaticprimitive(LitState* state, LitClass* selfclass, const char* name, LitPrimitiveMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, lit_value_fromobject(lit_object_makeprimitivemethod(state, fn, nm)));
}

void lit_class_setstaticfield(LitState* state, LitClass* selfclass, const char* name, LitValue val)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, val);
}

void lit_class_bindgetsetter(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fnget, LitNativeMethodFn fnset)
{
    LitString* nm;
    LitObject* mthset;
    LitObject* mthget;
    mthset = NULL;
    mthget = NULL;
    nm = lit_string_copy(state, name, strlen(name));
    if(fnget)
    {
        mthget = (LitObject*)lit_object_makenativemethod(state, fnget, nm);
    }
    if(fnset)
    {
        mthset = (LitObject*)lit_object_makenativemethod(state, fnset, nm);
    }
    lit_table_set(state, &selfclass->methods, nm, lit_value_fromobject(lit_object_makefield(state, mthget, mthset)));
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

LitValue file_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    const char* path = LIT_CHECK_STRING(0);
    const char* mode = LIT_GET_STRING(1, "rw");
    FILE* file = fopen(path, mode);
    if(file == NULL)
    {
        lit_vm_raisefatalerror(vm, "Failed to open file %s with mode %s (C error: %s)", path, mode, strerror(errno));
    }
    LitFileData* data = LIT_INSERT_DATA(LitFileData, cleanup_file);
    data->path = (char*)path;
    data->file = file;
    return instance;
}

LitValue file_close(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    fclose(data->file);
    data->file = NULL;
    return lit_value_makenull();
}

LitValue lit_coreutil_fileexists(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    char* file_name = NULL;
    if(lit_value_isinstance(instance))
    {
        file_name = LIT_EXTRACT_DATA(LitFileData)->path;
    }
    else
    {
        file_name = (char*)LIT_CHECK_STRING(0);
    }
    return lit_value_makebool(lit_file_exists(file_name));
}

LitValue file_create(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    const char* path = LIT_CHECK_STRING(0);
    FILE* file = fopen(path, "w");
    if(file == NULL)
    {
        lit_vm_raisefatalerror(vm, "Failed to create file %s", path);
    }
    fclose(file);
    return lit_value_makenull();
}

/*
 * ==
 * File writing
 */

LitValue file_write(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitString* value = lit_tostring_value(vm->state, args[0], 0);
    fwrite(value->chars, sizeof(char), value->length, LIT_EXTRACT_DATA(LitFileData)->file);
    return lit_value_makenull();
}

LitValue file_writeByte(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    uint8_t byte = (uint8_t)LIT_CHECK_NUMBER(0);
    lit_write_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file, byte);
    return lit_value_makenull();
}

LitValue file_writeShort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    uint16_t shrt = (uint16_t)LIT_CHECK_NUMBER(0);
    lit_write_uint16_t(LIT_EXTRACT_DATA(LitFileData)->file, shrt);
    return lit_value_makenull();
}

LitValue file_writeNumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    float num = (float)LIT_CHECK_NUMBER(0);
    lit_write_uint32_t(LIT_EXTRACT_DATA(LitFileData)->file, num);
    return lit_value_makenull();
}

LitValue file_writeBool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    bool value = LIT_CHECK_BOOL(0);
    lit_write_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file, (uint8_t)value ? '1' : '0');
    return lit_value_makenull();
}

LitValue file_writeString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(LIT_CHECK_STRING(0) == NULL)
    {
        return lit_value_makenull();
    }
    LitString* string = AS_STRING(args[0]);
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    lit_write_string(data->file, string);
    return lit_value_makenull();
}

/*
 * ==
 * File reading
 */

LitValue file_readAll(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    size_t rsz;
    LitFileData* data;
    (void)rsz;
    data = LIT_EXTRACT_DATA(LitFileData);
    fseek(data->file, 0, SEEK_END);
    LitUInt length = ftell(data->file);
    fseek(data->file, 0, SEEK_SET);
    LitString* result = lit_object_makeemptystring(vm->state, length);
    result->chars = lit_sysmem_malloc((length + 1) * sizeof(char));
    result->chars[length] = '\0';
    rsz = fread(result->chars, sizeof(char), length, data->file);
    result->hash = lit_string_hash(result->chars, result->length);
    lit_string_register(vm->state, result);
    return lit_value_fromobject(result);
}

LitValue file_readLine(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt maxlength = (LitUInt)LIT_GET_NUMBER(0, 128);
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    char line[maxlength];
    if(!fgets(line, maxlength, data->file))
    {
        return lit_value_makenull();
    }
    return lit_value_fromobject(lit_string_copy(vm->state, line, strlen(line) - 1));
}

LitValue file_readByte(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_read_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

LitValue file_readShort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_read_uint16_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

LitValue file_readNumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_makenumber(lit_read_uint32_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

LitValue file_readBool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    return lit_value_makebool((char)lit_read_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file) == '1');
}

LitValue file_readString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)args;
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    LitString* string = lit_read_string(vm->state, data->file);
    return string == NULL ? lit_value_makenull() : lit_value_fromobject(string);
}

LitValue file_getLastModified(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    struct stat buffer;
    char* file_name = NULL;
    if(lit_value_isinstance(instance))
    {
        file_name = LIT_EXTRACT_DATA(LitFileData)->path;
    }
    else
    {
        file_name = (char*)LIT_CHECK_STRING(0);
    }
    if(stat(file_name, &buffer) != 0)
    {
        return lit_value_makenumber(0);
    }
#ifdef WIN32
    return lit_value_makenumber(buffer.st_mtime);// Why, Windows, why?
#else
    return lit_value_makenumber(buffer.st_mtim.tv_sec);
#endif
}


/*
 * Directory
 */

LitValue directory_exists(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    const char* directoryname = LIT_CHECK_STRING(0);
    struct stat buffer;
    return lit_value_makebool(stat(directoryname, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

LitValue directory_listFiles(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    struct dirent* ep;
    LitState* state = vm->state;
    const char* path = LIT_CHECK_STRING(0);
    DIR* dir = opendir(path);
    LitArray* array = lit_object_makearray(state);
    if(dir == NULL)
    {
        return lit_value_fromobject(array);
    }
    while((ep = readdir(dir)))
    {
        const char* dirname = ep->d_name;
        if(strcmp(dirname, "..") == 0 || strcmp(dirname, ".") == 0)
        {
            continue;
        }
        size_t basedirnamelength = strlen(path);
        size_t dirnamelength = strlen(dirname);
        size_t totallength = dirnamelength + basedirnamelength + 2;
        char subdirname[totallength];
        memcpy(subdirname, path, basedirnamelength);
        memcpy(subdirname + basedirnamelength + 1, dirname, dirnamelength);
        subdirname[basedirnamelength] = '/';
        subdirname[totallength - 1] = '\0';
        struct stat st;
        stat(subdirname, &st);
        if(S_ISREG(st.st_mode))
        {
            lit_vallist_push(&array->values, OBJECT_CONST_STRING(state, dirname));
        }
    }
    closedir(dir);
    return lit_value_fromobject(array);
}

LitValue directory_listDirectories(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    struct dirent* ep;
    LitState* state = vm->state;
    const char* path = LIT_CHECK_STRING(0);
    DIR* dir = opendir(path);
    LitArray* array = lit_object_makearray(state);
    if(dir == NULL)
    {
        return lit_value_fromobject(array);
    }
    while((ep = readdir(dir)))
    {
        const char* dirname = ep->d_name;
        if(strcmp(dirname, "..") == 0 || strcmp(dirname, ".") == 0)
        {
            continue;
        }
        size_t basedirnamelength = strlen(path);
        size_t dirnamelength = strlen(dirname);
        size_t totallength = dirnamelength + basedirnamelength + 2;
        char subdirname[totallength];
        memcpy(subdirname, path, basedirnamelength);
        memcpy(subdirname + basedirnamelength + 1, dirname, dirnamelength);
        subdirname[basedirnamelength] = '/';
        subdirname[totallength - 1] = '\0';
        struct stat st;
        stat(subdirname, &st);
        if(S_ISDIR(st.st_mode))
        {
            lit_vallist_push(&array->values, OBJECT_CONST_STRING(state, dirname));
        }
    }
    closedir(dir);
    return lit_value_fromobject(array);
}

void lit_open_file_library(LitState* state)
{
    bool wasallowed = state->allow_gc;                                 \
    state->allow_gc = false;      
    {
        LitClass* klass = lit_class_make(state, "File", state->object_class);
        lit_class_bindstaticmethod(state, klass, "exists", lit_coreutil_fileexists);
        lit_class_bindstaticmethod(state, klass, "getLastModified", file_getLastModified);
        lit_class_bindstaticmethod(state, klass, "create", file_create);
        lit_class_bindconstructor(state, klass, file_constructor);
        lit_class_bindmethod(state, klass, "close", file_close);
        lit_class_bindmethod(state, klass, "write", file_write);
        lit_class_bindmethod(state, klass, "writeByte", file_writeByte);
        lit_class_bindmethod(state, klass, "writeShort", file_writeShort);
        lit_class_bindmethod(state, klass, "writeNumber", file_writeNumber);
        lit_class_bindmethod(state, klass, "writeBool", file_writeBool);
        lit_class_bindmethod(state, klass, "writeString", file_writeString);
        lit_class_bindmethod(state, klass, "readAll", file_readAll);
        lit_class_bindmethod(state, klass, "readLine", file_readLine);
        lit_class_bindmethod(state, klass, "readByte", file_readByte);
        lit_class_bindmethod(state, klass, "readShort", file_readShort);
        lit_class_bindmethod(state, klass, "readNumber", file_readNumber);
        lit_class_bindmethod(state, klass, "readBool", file_readBool);
        lit_class_bindmethod(state, klass, "readString", file_readString);
        lit_class_bindmethod(state, klass, "getLastModified", file_getLastModified);
        lit_class_bindgetsetter(state, klass, "exists", lit_coreutil_fileexists, NULL);
    }
    {
        LitClass* klass = lit_class_make(state, "Directory", state->object_class);
        lit_class_bindstaticmethod(state, klass, "exists", directory_exists);
        lit_class_bindstaticmethod(state, klass, "listFiles", directory_listFiles);
        lit_class_bindstaticmethod(state, klass, "listDirectories", directory_listDirectories);
    }
    state->allow_gc = wasallowed;
}



LitValue gc_memory_used(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(vm->state->bytes_allocated);
}

LitValue gc_next_round(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    return lit_value_makenumber(vm->state->next_gc);
}

LitValue gc_trigger(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    (void)argc;
    (void)args;
    vm->state->allow_gc = true;
    int64_t collected = lit_collect_garbage(vm);
    vm->state->allow_gc = false;
    return lit_value_makenumber(collected);
}

void lit_open_gc_library(LitState* state)
{
    bool wasallowed = state->allow_gc;
    state->allow_gc = false; 
    LitClass* klass = lit_class_make(state, "GC", state->object_class);
    lit_class_bindstaticgetter(state, klass, "memoryUsed", gc_memory_used);
    lit_class_bindstaticgetter(state, klass, "nextRound", gc_next_round);
    lit_class_bindstaticmethod(state, klass, "trigger", gc_trigger);
    state->allow_gc = wasallowed;
}


LitValue objfnmath_abs(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fabs(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_cos(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(cos(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_sin(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(sin(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_tan(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(tan(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_acos(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(acos(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_asin(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(asin(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_atan(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(atan(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_atan2(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(atan2(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue objfnmath_floor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(floor(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_ceil(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(ceil(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_round(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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

LitValue objfnmath_min(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fmin(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue objfnmath_max(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(fmax(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

LitValue objfnmath_mid(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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

LitValue objfnmath_toRadians(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(LIT_CHECK_NUMBER(0) * M_PI / 180.0);
}

LitValue objfnmath_toDegrees(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(LIT_CHECK_NUMBER(0) * 180.0 / M_PI);
}

LitValue objfnmath_sqrt(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(sqrt(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_log(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(exp(LIT_CHECK_NUMBER(0)));
}

LitValue objfnmath_exp(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)instance;
    return lit_value_makenumber(exp(LIT_CHECK_NUMBER(0)));
}

/*
 * Random
 */

LitUInt staticrandomdata;

LitUInt* extract_random_data(LitState* state, LitValue instance)
{
    if(IS_CLASS(instance))
    {
        return &staticrandomdata;
    }
    LitValue data;
    if(!lit_table_get(&lit_value_asinstance(instance)->fields, CONST_STRING(state, "_data"), &data))
    {
        return 0;
    }
    return (LitUInt*)AS_USERDATA(data)->data;
}

LitValue objfnrandom_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUserdata* userdata = lit_object_makeuserdata(vm->state, sizeof(LitUInt));
    lit_table_set(vm->state, &lit_value_asinstance(instance)->fields, CONST_STRING(vm->state, "_data"), lit_value_fromobject(userdata));

    LitUInt* data = (LitUInt*)userdata->data;

    if(argc == 1)
    {
        LitUInt number = (LitUInt)LIT_CHECK_NUMBER(0);
        *data = number;
    }
    else
    {
        int r = rand();
        *data = *((LitUInt*)&r);
    }

    return instance;
}

LitValue objfnrandom_setSeed(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);
    if(argc == 1)
    {
        LitUInt number = (LitUInt)LIT_CHECK_NUMBER(0);
        *data = number;
    }
    else
    {
        *data = time(NULL);
    }
    return lit_value_makenull();
}

int custom_random(LitUInt* data)
{
    *data = (*data * 125) % 2796203;
    return *data;
}

LitValue objfnrandom_int(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);
    if(argc == 1)
    {
        int bound = (int)LIT_GET_NUMBER(0, 1);
        return lit_value_makenumber(custom_random(data) % bound);
    }
    else if(argc == 2)
    {
        int min = (int)LIT_GET_NUMBER(0, 0);
        int max = (int)LIT_GET_NUMBER(1, 1);
        if(max - min == 0)
        {
            return lit_value_makenumber(max);
        }
        return lit_value_makenumber(min + custom_random(data) % (max - min));
    }
    return lit_value_makenumber(custom_random(data));
}

LitValue objfnrandom_float(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);
    double value = (double)custom_random(data) / RAND_MAX;
    if(argc == 1)
    {
        int bound = (int)LIT_GET_NUMBER(0, 0);
        return lit_value_makenumber(value * bound);
    }
    else if(argc == 2)
    {
        int min = (int)LIT_GET_NUMBER(0, 0);
        int max = (int)LIT_GET_NUMBER(1, 1);
        if(max - min == 0)
        {
            return lit_value_makenumber(max);
        }
        return lit_value_makenumber(min + value * (max - min));
    }
    return lit_value_makenumber(value);
}

LitValue objfnrandom_bool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    (void)argc;
    (void)vm;
    (void)args;
    return lit_value_makebool(custom_random(extract_random_data(vm->state, instance)) % 2);
}

LitValue objfnrandom_chance(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    float c = LIT_GET_NUMBER(0, 50);
    return lit_value_makebool((((float)custom_random(extract_random_data(vm->state, instance))) / RAND_MAX * 100) <= c);
}

LitValue objfnrandom_pick(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int value = custom_random(extract_random_data(vm->state, instance));
    if(argc == 1)
    {
        if(IS_ARRAY(args[0]))
        {
            LitArray* array = AS_ARRAY(args[0]);
            if(array->values.count == 0)
            {
                return lit_value_makenull();
            }
            return array->values.values[value % array->values.count];
        }
        else if(lit_value_ismap(args[0]))
        {
            LitMap* map = AS_MAP(args[0]);
            LitUInt length = map->values.count;
            LitUInt capacity = map->values.capacity;
            if(length == 0)
            {
                return lit_value_makenull();
            }
            LitUInt target = value % length;
            LitUInt index = 0;
            for(LitUInt i = 0; i < capacity; i++)
            {
                if(map->values.entries[i].key != NULL)
                {
                    if(index == target)
                    {
                        return map->values.entries[i].value;
                    }
                    index++;
                }
            }
        }
        else
        {
            lit_vm_raisefatalerror(vm, "Expected map or array as the argument");
        }
    }
    else
    {
        return args[value % argc];
    }
    return lit_value_makenull();
}

void lit_open_math_library(LitState* state)
{
    LitClass* klass;
    bool wasallowed = state->allow_gc;
    state->allow_gc = false; 
    {
        klass = lit_class_make(state, "Math", state->object_class);
        lit_class_setstaticfield(state, klass, "Pi", lit_value_makenumber(M_PI));
        lit_class_setstaticfield(state, klass, "Tau", lit_value_makenumber(M_PI * 2));
        lit_class_bindstaticmethod(state, klass, "abs", objfnmath_abs);
        lit_class_bindstaticmethod(state, klass, "sin", objfnmath_sin);
        lit_class_bindstaticmethod(state, klass, "cos", objfnmath_cos);
        lit_class_bindstaticmethod(state, klass, "tan", objfnmath_tan);
        lit_class_bindstaticmethod(state, klass, "asin", objfnmath_asin);
        lit_class_bindstaticmethod(state, klass, "acos", objfnmath_acos);
        lit_class_bindstaticmethod(state, klass, "atan", objfnmath_atan);
        lit_class_bindstaticmethod(state, klass, "atan2", objfnmath_atan2);
        lit_class_bindstaticmethod(state, klass, "floor", objfnmath_floor);
        lit_class_bindstaticmethod(state, klass, "ceil", objfnmath_ceil);
        lit_class_bindstaticmethod(state, klass, "round", objfnmath_round);
        lit_class_bindstaticmethod(state, klass, "min", objfnmath_min);
        lit_class_bindstaticmethod(state, klass, "max", objfnmath_max);
        lit_class_bindstaticmethod(state, klass, "mid", objfnmath_mid);
        lit_class_bindstaticmethod(state, klass, "toRadians", objfnmath_toRadians);
        lit_class_bindstaticmethod(state, klass, "toDegrees", objfnmath_toDegrees);
        lit_class_bindstaticmethod(state, klass, "sqrt", objfnmath_sqrt);
        lit_class_bindstaticmethod(state, klass, "log", objfnmath_log);
        lit_class_bindstaticmethod(state, klass, "exp", objfnmath_exp);
    }
    srand(time(NULL));
    int r = rand();
    staticrandomdata = *((LitUInt*)&r);
    {
        klass = lit_class_make(state, "Random", state->object_class);
        lit_class_bindconstructor(state, klass, objfnrandom_constructor);
        lit_class_bindmethod(state, klass, "setSeed", objfnrandom_setSeed);
        lit_class_bindmethod(state, klass, "int", objfnrandom_int);
        lit_class_bindmethod(state, klass, "float", objfnrandom_float);
        lit_class_bindmethod(state, klass, "chance", objfnrandom_chance);
        lit_class_bindmethod(state, klass, "pick", objfnrandom_pick);
        lit_class_bindstaticmethod(state, klass, "setSeed", objfnrandom_setSeed);
        lit_class_bindstaticmethod(state, klass, "int", objfnrandom_int);
        lit_class_bindstaticmethod(state, klass, "float", objfnrandom_float);
        lit_class_bindstaticmethod(state, klass, "bool", objfnrandom_bool);
        lit_class_bindstaticmethod(state, klass, "chance", objfnrandom_chance);
        lit_class_bindstaticmethod(state, klass, "pick", objfnrandom_pick);
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

void lit_uintlist_destroy(LitState* state, LitDynListUInt* array)
{
    LIT_FREE_ARRAY(state, LitUInt, array->values, array->capacity);
    lit_uintlist_reset(array);
}

void lit_uintlist_push(LitDynListUInt* array, LitUInt value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitUInt*)lit_sysmem_realloc(array->values, sizeof(LitUInt) * (array->capacity));
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


void lit_bytelist_destroy(LitState* state, LitDynListByte* array)
{
    LIT_FREE_ARRAY(state, uint8_t, array->values, array->capacity);
    lit_bytelist_reset(array);
}

void lit_bytelist_push(LitDynListByte* array, uint8_t value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
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

void lit_vallist_destroy(LitState* state, LitDynListVal* array)
{
    LIT_FREE_ARRAY(state, LitValue, array->values, array->capacity);
    lit_vallist_reset(array);
}

void lit_vallist_push(LitDynListVal* array, LitValue value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = (LitValue*)lit_sysmem_realloc(array->values, sizeof(LitValue) * (array->capacity));
    }
    array->values[array->count] = value;
    array->count++;
}



bool lit_fiber_ensureframes(LitVm* vm, LitFiber* fiber)
/*
{
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(vm, "No fiber to run on");
        return true;
    }
    if(fiber->framecount == LIT_CALL_FRAMES_MAX)
    {
        lit_vm_raisefatalerror(vm, "Stack overflow");
        return true;
    }
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        LitUInt newcapacity = ((fiber->framecapacity + 1) * 2);
        fiber->framevals = (LitCallFrame*)lit_sysmem_realloc(fiber->framevals, sizeof(LitCallFrame) * newcapacity);
        fiber->framecapacity = newcapacity;
    }
    return false;
}
*/
{
    size_t incsize;
    size_t oldsize;
    size_t inccap;
    (void)oldsize;
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(vm, "No fiber to run on");
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
    LitVm* vm = state->vm;
    LitFiber* fiber = vm->fiber;
    if(callee == NULL)
    {
        lit_vm_raisefatalerror(vm, "Attempt to call a null value");
        return NULL;
    }
    if(lit_fiber_ensureframes(vm, fiber))
    {
        return NULL;
    }
    LitValue* start
    = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->max_registers : fiber->registers;
    lit_fiber_ensureregisters(fiber, start - fiber->registers + callee->max_registers);
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    frame->slots = fiber->framecount > 1 ? fiber->framevals[fiber->framecount - 2].slots + fiber->framevals[fiber->framecount - 2].function->max_registers : fiber->registers;
#ifdef LIT_TRACE_NULL_FILL
    printf("Filling with nulls\n");
#endif
    for(int i = argc + 1; i < callee->max_registers; i++)
    {
        frame->slots[i] = lit_value_makenull();
    }
    frame->slots[0] = lit_value_fromobject(callee);
    for(uint8_t i = 0; i < argc; i++)
    {
        frame->slots[i + 1] = arguments[i];
    }
    LitUInt targetargcount = callee->argcount;
    bool vararg = callee->vararg;
    if(targetargcount > argc)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif
        for(LitUInt i = argc; i < targetargcount; i++)
        {
            *(frame->slots + i + 1) = lit_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = lit_value_fromobject(lit_object_makearray(vm->state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            // No need to repack the arguments
        }
        else
        {
            LitArray* array = &lit_object_makevararray(vm->state)->array;
            lit_state_pushroot(vm->state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->values, argc - targetargcount + 1);
            LitUInt j = 0;
            for(LitUInt i = targetargcount - 1; i < argc; i++)
            {
                array->values.values[j++] = *(frame->slots + i + 1);
            }
            *(frame->slots + targetargcount) = lit_value_fromobject(array);
            lit_state_poproot(vm->state);
        }
    }
    frame->ip = callee->chunk.compiledcodechunk;
    frame->closure = NULL;
    frame->function = callee;
    frame->result_ignored = false;
    frame->return_to_c = true;
    frame->return_address = NULL;
    return frame;
}

LitResult execute_call(LitState* state, LitCallFrame* frame)
{
    if(frame == NULL)
    {
        RETURN_RUNTIME_ERROR()
    }
    LitFiber* fiber = state->vm->fiber;
    LitResult result = lit_interpret_fiber(state, fiber);
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

LitResult lit_state_callclosure(LitState* state, LitClosure* callee, LitValue* arguments, uint8_t argc)
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
    LitVm* vm = state->vm;
    if(lit_value_isobject(callee))
    {
        if(lit_set_native_exit_jump())
        {
            RETURN_RUNTIME_ERROR()
        }
        LitObjType type = OBJECT_TYPE(callee);
        if(type == LIT_OBJ_FUNCTION)
        {
            return lit_state_callfunction(state, AS_FUNCTION(callee), arguments, argc);
        }
        else if(type == LIT_OBJ_CLOSURE)
        {
            return lit_state_callclosure(state, AS_CLOSURE(callee), arguments, argc);
        }
        LitFiber* fiber = vm->fiber;
        if(lit_fiber_ensureframes(vm, fiber))
        {
            RETURN_RUNTIME_ERROR()
        }
        LitValue* start = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->max_registers :
                                                   fiber->registers;
        lit_fiber_ensureregisters(fiber, start - fiber->registers + 3 + argc);
        LitValue* slot = fiber->framecount > 0 ? fiber->framevals[fiber->framecount - 1].slots + fiber->framevals[fiber->framecount - 1].function->max_registers :
                                                  fiber->registers;
#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif
        for(int i = argc; i < argc + 3; i++)
        {
            *(slot + i) = lit_value_makenull();
        }
        *slot = instance;
        if(type != LIT_OBJ_CLASS)
        {
            for(uint8_t i = 0; i < argc; i++)
            {
                *(slot + i + 1) = arguments[i];
            }
        }
        if(state->config.traceexecution)
        {
            fprintf(stderr, "        |\n        | f%i ", fiber->framecount);
            for(int i = 0; i <= argc; i++)
            {
                fprintf(stderr, "[ ");
                lit_value_printvalue(*(slot + i));
                fprintf(stderr, " ]");
            }
            printf("\n");
        }
        switch(type)
        {
            case LIT_OBJ_NATIVEFUNCTION:
            {
                // For some reason, single line expression doesn't work
                LitValue value = AS_NATIVE_FUNCTION(callee)->function(vm, argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_NATIVEPRIMITIVE:
            {
                AS_NATIVE_PRIMITIVE(callee)->function(vm, argc, slot + 1);
                RETURN_OK(lit_value_makenull())
            }
            case LIT_OBJ_NATIVEMETHOD:
            {
                LitNativeMethod* method = AS_NATIVE_METHOD(callee);
                // For some reason, single line expression doesn't work
                LitValue value = method->method(vm, *slot, argc, slot + 1);
                RETURN_OK(value)
            }
            case LIT_OBJ_PRIMITIVEMETHOD:
            {
                AS_PRIMITIVE_METHOD(callee)->method(vm, *slot, argc, slot + 1);
                RETURN_OK(lit_value_makenull())
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* inst = lit_object_makeinstance(vm->state, klass);
                if(klass->init_method != NULL)
                {
                    lit_state_callmethod(state, *slot, lit_value_fromobject(klass->init_method), arguments, argc);
                }
                RETURN_OK(lit_value_fromobject(inst))
            }
            case LIT_OBJ_BOUNDMETHOD:
            {
                LitBoundMethod* boundmethod = AS_BOUND_METHOD(callee);
                LitValue method = boundmethod->method;
                if(IS_NATIVE_METHOD(method))
                {
                    // For some reason, single line expression doesn't work
                    LitValue value = AS_NATIVE_METHOD(method)->method(vm, boundmethod->receiver, argc, slot + 1);
                    RETURN_OK(value)
                }
                else if(IS_PRIMITIVE_METHOD(method))
                {
                    AS_PRIMITIVE_METHOD(method)->method(vm, boundmethod->receiver, argc, slot + 1);
                    RETURN_OK(lit_value_makenull())
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
        lit_vm_raisefatalerror(vm, "Attempt to call a null value");
    }
    else
    {
        lit_vm_raisefatalerror(vm, "Can only call functions and classes");
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
    LitVm* vm;
    LitFiber* fiber;
    LitClass* klass;
    LitInstance* inst;
    vm = state->vm;
    fiber = vm->fiber;
    ok = false;
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(vm, "No fiber to run on");
        RETURN_RUNTIME_ERROR()
    }
    klass = lit_state_getclassfor(state, callee);
    if(lit_value_isinstance(callee))
    {
        inst = lit_value_asinstance(callee);
        if(lit_table_get(&inst->fields, mthname, &method))
        {
            ok = true;
        }
    }
    else if(klass != NULL)
    {
        if(lit_table_get(&klass->methods, mthname, &method))
        {
            ok = true;
        }
    }
    if(ok)
    {
        return lit_state_callmethod(state, callee, method, arguments, argc);
    }
    return (LitResult){ INTERPRET_INVALID, lit_value_makenull() };
}

LitString* lit_tostring_value(LitState* state, LitValue object, LitUInt indentation)
{
    LitUInt needed;
    LitValue* tmptr;
    if(IS_STRING(object))
    {
        return AS_STRING(object);
    }
    else if(!lit_value_isobject(object))
    {
        if(lit_value_isnull(object))
        {
            return CONST_STRING(state, "null");
        }
        else if(lit_value_isnumber(object))
        {
            return AS_STRING(lit_string_numbertostring(state, lit_value_asnumber(object)));
        }
        else if(lit_value_isbool(object))
        {
            return CONST_STRING(state, lit_value_asbool(object) ? "true" : "false");
        }
    }
    else if(IS_REFERENCE(object))
    {
        LitValue* slot = AS_REFERENCE(object)->slot;
        if(slot == NULL)
        {
            return CONST_STRING(state, "null");
        }
        return lit_tostring_value(state, *slot, 0);
    }
    LitVm* vm = state->vm;
    LitFiber* fiber = vm->fiber;
    if(lit_fiber_ensureframes(vm, fiber))
    {
        return CONST_STRING(state, "null");
    }
    LitFunction* function = state->api_function;
    if(function == NULL)
    {
        function = state->api_function = lit_object_makefunction(state, fiber->module);
        function->chunk.haslineinfo = false;
        function->name = state->api_name;
        LitChunk* chunk = &function->chunk;
        chunk->compiledcodecount = 0;
        chunk->constants.count = 0;
        function->max_registers = 3;
        int constant = lit_chunk_addconstant(state, chunk, OBJECT_CONST_STRING(state, "toString"));
        lit_chunk_push(chunk, LIT_FORM_ABC_INSTRUCTION(OP_INVOKE, 1, 2, constant), 1);
        lit_chunk_push(chunk, LIT_FORM_ABC_INSTRUCTION(OP_RETURN, 1, 0, 0), 1);
    }
    tmptr = fiber->registers;
    if(fiber->framecount > 0)
    {
        tmptr = (fiber->framevals[fiber->framecount - 1].slots + (int)fiber->framevals[fiber->framecount - 1].function->max_registers);
    }
    needed = (tmptr - fiber->registers + function->max_registers);
    lit_fiber_ensureregisters(fiber, needed);
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    frame->ip = function->chunk.compiledcodechunk;
    frame->closure = NULL;
    frame->function = function;
    // "Duplicated" code due to lit_fiber_ensureregisters messing with register pointers
    frame->slots
    = (fiber->framecount > 1 ? (fiber->framevals[fiber->framecount - 2].slots + (int)fiber->framevals[fiber->framecount - 2].function->max_registers) : fiber->registers);
    frame->result_ignored = false;
    frame->return_to_c = true;
    frame->return_address = NULL;
    frame->slots[0] = lit_value_fromobject(function);
    frame->slots[1] = object;
    frame->slots[2] = lit_value_makenumber(indentation);
    LitResult result = lit_interpret_fiber(state, fiber);
    if(result.type != INTERPRET_OK)
    {
        return CONST_STRING(state, "null");
    }
    if(!IS_STRING(result.result))
    {
        return CONST_STRING(state, "invalid toString()");
    }
    return AS_STRING(result.result);
}

LitValue lit_state_callnew(LitVm* vm, const char* name, LitValue* args, LitUInt argc)
{
    LitValue value;
    if(!lit_table_get(&vm->globals->values, CONST_STRING(vm->state, name), &value))
    {
        lit_vm_raisefatalerror(vm, "Failed to create instance of class %s: class not found", name);
        return lit_value_makenull();
    }
    LitClass* klass = AS_CLASS(value);
    if(klass->init_method == NULL)
    {
        return lit_value_fromobject(lit_object_makeinstance(vm->state, klass));
    }
    return lit_state_callmethod(vm->state, value, value, args, argc).result;
}




bool lit_value_iscallablefunction(LitValue value)
{
    if(lit_value_isobject(value))
    {
        LitObjType type = OBJECT_TYPE(value);
        return (
            (type == LIT_OBJ_CLOSURE) ||
            (type == LIT_OBJ_FUNCTION) ||
            (type == LIT_OBJ_NATIVEFUNCTION) ||
            (type == LIT_OBJ_NATIVEPRIMITIVE) ||
            (type == LIT_OBJ_NATIVEMETHOD) ||
            (type == LIT_OBJ_PRIMITIVEMETHOD) ||
            (type == LIT_OBJ_BOUNDMETHOD)
        );
    }
    return false;
}

LitString* lit_object_makeemptystring(LitState* state, LitUInt length)
{
    LitString* string = ALLOCATE_OBJECT(state, LitString, LIT_OBJ_STRING);
    string->length = length;
    return string;
}

void lit_string_register(LitState* state, LitString* string)
{
    lit_state_pushroot(state, (LitObject*)string);
    lit_table_set(state, &state->vm->strings, string, lit_value_makenull());
    lit_state_poproot(state);
}

LitString* lit_object_allocstring(LitState* state, char* chars, LitUInt length, uint32_t hash)
{
    LitString* string = lit_object_makeemptystring(state, length);
    string->chars = chars;
    string->hash = hash;
    lit_string_register(state, string);
    return string;
}

uint32_t lit_string_hash(const char* key, LitUInt length)
{
    uint32_t hash = 2166136261u;
    for(LitUInt i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

LitString* lit_string_take(LitState* state, const char* chars, LitUInt length)
{
    uint32_t hash = lit_string_hash(chars, length);
    LitString* interned = lit_table_find_string(&state->vm->strings, chars, length, hash);
    if(interned != NULL)
    {
        return interned;
    }
    return lit_object_allocstring(state, (char*)chars, length, hash);
}

LitString* lit_string_copy(LitState* state, const char* chars, LitUInt length)
{
    uint32_t hash = lit_string_hash(chars, length);
    LitString* interned = lit_table_find_string(&state->vm->strings, chars, length, hash);
    if(interned != NULL)
    {
        return interned;
    }
    char* heapchars = lit_sysmem_malloc((length + 1) * sizeof(char));
    memcpy(heapchars, chars, length);
    heapchars[length] = '\0';
#ifdef LIT_LOG_ALLOCATION
    printf("Allocated new string '%s'\n", chars);
#endif
    return lit_object_allocstring(state, heapchars, length, hash);
}

LitValue lit_string_numbertostring(LitState* state, double value)
{
    if(isnan(value))
    {
        return OBJECT_CONST_STRING(state, "nan");
    }
    if(isinf(value))
    {
        if(value > 0.0)
        {
            return OBJECT_CONST_STRING(state, "infinity");
        }
        else
        {
            return OBJECT_CONST_STRING(state, "-infinity");
        }
    }
    char buffer[24];
    int length = sprintf(buffer, "%.14g", value);
    return lit_value_fromobject(lit_string_copy(state, buffer, length));
}

LitValue lit_string_format(LitState* state, const char* format, ...)
{
    bool wasallowed = state->allow_gc;
    state->allow_gc = false;
    va_list arglist;
    va_start(arglist, format);
    size_t totallength = 0;
    for(const char* c = format; *c != '\0'; c++)
    {
        switch(*c)
        {
            case '$':
            {
                const char* cc = va_arg(arglist, const char*);
                if(cc != NULL)
                {
                    totallength += strlen(cc);
                    break;
                }
                goto defaultending;
            }
            case '@':
            {
                LitValue v = va_arg(arglist, LitValue);
                LitString* ss = AS_STRING(v);
                if(ss != NULL)
                {
                    totallength += ss->length;
                    break;
                }
                goto defaultending;
            }
            case '#':
            {
                totallength += AS_STRING(lit_string_numbertostring(state, va_arg(arglist, double)))->length;
                break;
            }
            default:
            {
            defaultending:
                totallength++;
                break;
            }
        }
    }
    va_end(arglist);
    LitString* result = lit_object_makeemptystring(state, totallength);
    result->chars = lit_sysmem_malloc((totallength + 1) * sizeof(char));
    result->chars[totallength] = '\0';
    char* start = result->chars;
    va_start(arglist, format);
    for(const char* c = format; *c != '\0'; c++)
    {
        switch(*c)
        {
            case '$':
            {
                const char* string = va_arg(arglist, const char*);
                if(string != NULL)
                {
                    size_t length = strlen(string);
                    memcpy(start, string, length);
                    start += length;
                    break;
                }
                goto defaultendingcopying;
            }
            case '@':
            {
                LitString* string = AS_STRING(va_arg(arglist, LitValue));
                if(string != NULL)
                {
                    memcpy(start, string->chars, string->length);
                    start += string->length;
                    break;
                }
                goto defaultendingcopying;
            }
            case '#':
            {
                LitString* string = AS_STRING(lit_string_numbertostring(state, va_arg(arglist, double)));
                memcpy(start, string->chars, string->length);
                start += string->length;
                break;
            }
            default:
            {
            defaultendingcopying:
                *start++ = *c;
                break;
            }
        }
    }
    va_end(arglist);
    result->hash = lit_string_hash(result->chars, result->length);
    lit_string_register(state, result);
    state->allow_gc = wasallowed;
    return lit_value_fromobject(result);
}

LitObject* lit_object_allocobject(LitState* state, size_t size, LitObjType type)
{
    LitObject* object = (LitObject*)lit_reallocate(state, NULL, 0, size);
    object->type = type;
    object->marked = false;
    object->next = state->vm->objects;
    state->vm->objects = object;
#ifdef LIT_LOG_ALLOCATION
    printf("%p allocate %ld for %s\n", (void*)object, size, lit_tostring_typename(type));
#endif
    return object;
}

LitFunction* lit_object_makefunction(LitState* state, LitModule* module)
{
    LitFunction* function = ALLOCATE_OBJECT(state, LitFunction, LIT_OBJ_FUNCTION);
    lit_chunk_init(&function->chunk);
    function->name = NULL;
    function->argcount = 0;
    function->upvaluecount = 0;
    function->max_registers = 0;
    function->module = module;
    function->vararg = false;
    return function;
}

LitValue lit_function_getname(LitVm* vm, LitValue instance)
{
    LitString* name = NULL;
    switch(OBJECT_TYPE(instance))
    {
        case LIT_OBJ_FUNCTION:
        {
            name = AS_FUNCTION(instance)->name;
            break;
        }
        case LIT_OBJ_CLOSURE:
        {
            name = AS_CLOSURE(instance)->function->name;
            break;
        }
        case LIT_OBJ_CLOSUREPROTOTYPE:
        {
            name = AS_CLOSURE_PROTOTYPE(instance)->function->name;
            break;
        }
        case LIT_OBJ_FIELD:
        {
            LitField* field = AS_FIELD(instance);
            if(field->getter != NULL)
            {
                return lit_function_getname(vm, lit_value_fromobject(field->getter));
            }
            return lit_function_getname(vm, lit_value_fromobject(field->setter));
        }
        case LIT_OBJ_NATIVEPRIMITIVE:
        {
            name = AS_NATIVE_PRIMITIVE(instance)->name;
            break;
        }
        case LIT_OBJ_NATIVEFUNCTION:
        {
            name = AS_NATIVE_FUNCTION(instance)->name;
            break;
        }
        case LIT_OBJ_NATIVEMETHOD:
        {
            name = AS_NATIVE_METHOD(instance)->name;
            break;
        }
        case LIT_OBJ_PRIMITIVEMETHOD:
        {
            name = AS_PRIMITIVE_METHOD(instance)->name;
            break;
        }
        case LIT_OBJ_BOUNDMETHOD:
        {
            return lit_function_getname(vm, AS_BOUND_METHOD(instance)->method);
        }
        default:
        {
            break;
        }
    }
    if(name == NULL)
    {
        return lit_string_format(vm->state, "function #", *((double*)lit_value_asobject(instance)));
    }
    return lit_string_format(vm->state, "function @", lit_value_fromobject(name));
}

LitUpvalue* lit_object_makeupvalue(LitState* state, LitValue* slot)
{
    LitUpvalue* upvalue = ALLOCATE_OBJECT(state, LitUpvalue, LIT_OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->closed = lit_value_makenull();
    upvalue->next = NULL;
    return upvalue;
}

LitClosure* lit_object_makeclosure(LitState* state, LitFunction* function)
{
    LitClosure* closure = ALLOCATE_OBJECT(state, LitClosure, LIT_OBJ_CLOSURE);
    closure->function = function;
    closure->upvaluecount = 0;// To prevent GC crashes
    lit_state_pushroot(state, (LitObject*)closure);
    LitUpvalue** upvalues = lit_sysmem_malloc(function->upvaluecount * sizeof(LitUpvalue*));
    lit_state_poproot(state);
    for(LitUInt i = 0; i < function->upvaluecount; i++)
    {
        upvalues[i] = NULL;
    }
    closure->upvalues = upvalues;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

LitClosurePrototype* lit_object_makeclosureproto(LitState* state, LitFunction* function)
{
    LitClosurePrototype* closure = ALLOCATE_OBJECT(state, LitClosurePrototype, LIT_OBJ_CLOSUREPROTOTYPE);
    lit_state_pushroot(state, (LitObject*)closure);
    closure->indexes = lit_sysmem_malloc(function->upvaluecount * sizeof(uint8_t));
    closure->local = lit_sysmem_malloc(function->upvaluecount * sizeof(bool));
    lit_state_poproot(state);
    closure->function = function;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

LitNativeFunction* lit_object_makenativefunc(LitState* state, LitNativeFunctionFn function, LitString* name)
{
    LitNativeFunction* native = ALLOCATE_OBJECT(state, LitNativeFunction, LIT_OBJ_NATIVEFUNCTION);
    native->function = function;
    native->name = name;
    return native;
}

LitNativePrimitive* lit_object_makenativeprimitive(LitState* state, LitNativePrimitiveFn function, LitString* name)
{
    LitNativePrimitive* native = ALLOCATE_OBJECT(state, LitNativePrimitive, LIT_OBJ_NATIVEPRIMITIVE);
    native->function = function;
    native->name = name;
    return native;
}

LitNativeMethod* lit_object_makenativemethod(LitState* state, LitNativeMethodFn method, LitString* name)
{
    LitNativeMethod* native = ALLOCATE_OBJECT(state, LitNativeMethod, LIT_OBJ_NATIVEMETHOD);
    native->method = method;
    native->name = name;
    return native;
}

LitPrimitiveMethod* lit_object_makeprimitivemethod(LitState* state, LitPrimitiveMethodFn method, LitString* name)
{
    LitPrimitiveMethod* native = ALLOCATE_OBJECT(state, LitPrimitiveMethod, LIT_OBJ_PRIMITIVEMETHOD);
    native->method = method;
    native->name = name;
    return native;
}

LitFiber* lit_object_makefiber(LitState* state, LitModule* module, LitFunction* function)
{
    // Allocate in advance, just in case GC is triggered
    uint8_t registers_allocated = function == NULL ? 1 : (uint8_t)lit_closest_power_of_two(function->max_registers);
    LitValue* registers = lit_sysmem_malloc(registers_allocated * sizeof(LitValue));
    LitCallFrame* framevals = lit_sysmem_malloc(LIT_INITIAL_CALL_FRAMES * sizeof(LitCallFrame));
    LitFiber* fiber = ALLOCATE_OBJECT(state, LitFiber, LIT_OBJ_FIBER);
    if(module->main_fiber == NULL)
    {
        module->main_fiber = fiber;
    }
    fiber->registers = registers;
    for(uint8_t i = 0; i < registers_allocated; i++)
    {
        fiber->registers[i] = lit_value_makenull();
    }
    fiber->registers_allocated = registers_allocated;
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
    fiber->return_address = NULL;
    if(function != NULL)
    {
        LitCallFrame* frame = &fiber->framevals[0];
        frame->closure = NULL;
        frame->function = function;
        frame->slots = fiber->registers;
        frame->result_ignored = false;
        frame->return_to_c = false;
        frame->return_address = NULL;
        lit_fiber_ensureregisters(fiber, function->max_registers);
        frame->ip = function->chunk.compiledcodechunk;
    }
    return fiber;
}

LitFiber* lit_object_makefiberclosure(LitState* state, LitModule* module, LitClosure* closure)
{
    LitFiber* fiber = lit_object_makefiber(state, module, closure->function);
    fiber->framevals[0].closure = closure;
    return fiber;
}

void lit_fiber_ensureregisters(LitFiber* fiber, LitUInt needed)
{
    if(fiber->registers_allocated >= needed)
    {
        return;
    }
    LitUInt capacity = (LitUInt)lit_closest_power_of_two((int)needed);
    LitValue* oldregisters = fiber->registers;
    fiber->registers = (LitValue*)lit_sysmem_realloc(fiber->registers, sizeof(LitValue) * capacity);
    for(LitUInt i = fiber->registers_allocated; i < capacity; i++)
    {
        fiber->registers[i] = lit_value_makenull();
    }
    fiber->registers_allocated = capacity;
    if(fiber->registers != oldregisters)
    {
        for(LitUInt i = 0; i < fiber->framecount; i++)
        {
            LitCallFrame* frame = &fiber->framevals[i];
            int difference = (frame->slots - oldregisters);
            LitValue* oldslots = frame->slots;
            frame->slots = fiber->registers + difference;
            if(frame->return_address != NULL)
            {
                frame->return_address = fiber->registers + (oldslots - oldregisters);
            }
        }
        for(LitUpvalue* upvalue = fiber->open_upvalues; upvalue != NULL; upvalue = upvalue->next)
        {
            upvalue->location = fiber->registers + (upvalue->location - oldregisters);
        }
    }
}

LitModule* lit_object_makemodule(LitState* state, LitString* name)
{
    LitModule* module = ALLOCATE_OBJECT(state, LitModule, LIT_OBJ_MODULE);
    module->name = name;
    module->return_value = lit_value_makenull();
    module->main_function = NULL;
    module->privates = NULL;
    module->ran = false;
    module->main_fiber = NULL;
    module->private_count = 0;
    module->private_names = lit_object_makemap(state, NULL);
    return module;
}

LitClass* lit_object_makeclass(LitState* state, LitString* name)
{
    LitClass* klass = ALLOCATE_OBJECT(state, LitClass, LIT_OBJ_CLASS);
    klass->name = name;
    klass->init_method = NULL;
    klass->super = NULL;
    lit_init_table(&klass->methods);
    lit_init_table(&klass->static_fields);
    return klass;
}

LitInstance* lit_object_makeinstance(LitState* state, LitClass* klass)
{
    LitInstance* instance = ALLOCATE_OBJECT(state, LitInstance, LIT_OBJ_INSTANCE);
    instance->klass = klass;
    lit_init_table(&instance->fields);
    return instance;
}

LitBoundMethod* lit_object_makeboundmethod(LitState* state, LitValue receiver, LitValue method)
{
    LitBoundMethod* boundmethod = ALLOCATE_OBJECT(state, LitBoundMethod, LIT_OBJ_BOUNDMETHOD);
    boundmethod->receiver = receiver;
    boundmethod->method = method;
    return boundmethod;
}

LitArray* lit_object_makearray(LitState* state)
{
    LitArray* array = ALLOCATE_OBJECT(state, LitArray, LIT_OBJ_ARRAY);
    lit_vallist_init(&array->values);
    return array;
}

LitVarargArray* lit_object_makevararray(LitState* state)
{
    LitVarargArray* array = ALLOCATE_OBJECT(state, LitVarargArray, LIT_OBJ_VARARGARRAY);
    lit_vallist_init(&array->array.values);
    return array;
}

LitMap* lit_object_makemap(LitState* state, LitTable* fields)
{
    LitMap* map = ALLOCATE_OBJECT(state, LitMap, LIT_OBJ_MAP);
    lit_init_table(&map->values);
    map->index_fn = NULL;
    if(fields != NULL)
    {
        //void lit_table_add_all(LitState *state, LitTable *from, LitTable *to);
        lit_table_add_all(state, fields, &map->values);
    }
    return map;
}

bool lit_map_set(LitState* state, LitMap* map, LitString* key, LitValue value)
{
    if(lit_value_isnull(value))
    {
        lit_map_delete(map, key);
        return false;
    }
    return lit_table_set(state, &map->values, key, value);
}

bool lit_map_get(LitMap* map, LitString* key, LitValue* value)
{
    return lit_table_get(&map->values, key, value);
}

bool lit_map_delete(LitMap* map, LitString* key)
{
    return lit_table_delete(&map->values, key);
}

void lit_map_add_all(LitState* state, LitMap* from, LitMap* to)
{
    for(int i = 0; i <= from->values.capacity; i++)
    {
        LitTabEntry* entry = &from->values.entries[i];
        if(entry->key != NULL)
        {
            lit_table_set(state, &to->values, entry->key, entry->value);
        }
    }
}

LitUserdata* lit_object_makeuserdata(LitState* state, size_t size)
{
    LitUserdata* userdata = ALLOCATE_OBJECT(state, LitUserdata, LIT_OBJ_USERDATA);
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

LitRange* lit_object_makerange(LitState* state, double from, double to)
{
    LitRange* range = ALLOCATE_OBJECT(state, LitRange, LIT_OBJ_RANGE);
    range->from = from;
    range->to = to;
    return range;
}

LitField* lit_object_makefield(LitState* state, LitObject* getter, LitObject* setter)
{
    LitField* field = ALLOCATE_OBJECT(state, LitField, LIT_OBJ_FIELD);
    field->getter = getter;
    field->setter = setter;
    return field;
}

LitReference* lit_object_makereference(LitState* state, LitValue* slot)
{
    LitReference* reference = ALLOCATE_OBJECT(state, LitReference, LIT_OBJ_REFERENCE);
    reference->slot = slot;
    return reference;
}


void lit_init_table(LitTable* table)
{
    table->capacity = -1;
    table->count = 0;
    table->entries = NULL;
}

void lit_free_table(LitState* state, LitTable* table)
{
    if(table->capacity > 0)
    {
        LIT_FREE_ARRAY(state, LitTabEntry, table->entries, table->capacity + 1);
    }
    lit_init_table(table);
}

LitTabEntry* find_entry(LitTabEntry* entries, int capacity, LitString* key)
{
    uint32_t index = key->hash % capacity;
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

void adjust_capacity(LitState* state, LitTable* table, int capacity)
{
    LitTabEntry* entries = lit_sysmem_malloc((capacity + 1) * sizeof(LitTabEntry));
    for(int i = 0; i <= capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = lit_value_makenull();
    }
    table->count = 0;
    for(int i = 0; i <= table->capacity; i++)
    {
        LitTabEntry* entry = &table->entries[i];
        if(entry->key == NULL)
        {
            continue;
        }
        LitTabEntry* destination = find_entry(entries, capacity, entry->key);
        destination->key = entry->key;
        destination->value = entry->value;
        table->count++;
    }
    LIT_FREE_ARRAY(state, LitTabEntry, table->entries, table->capacity + 1);
    table->capacity = capacity;
    table->entries = entries;
}

bool lit_table_set(LitState* state, LitTable* table, LitString* key, LitValue value)
{
    if(table->count + 1 > (table->capacity + 1) * TABLE_MAX_LOAD)
    {
        int capacity = LIT_GROW_CAPACITY(table->capacity + 1) - 1;
        adjust_capacity(state, table, capacity);
    }
    LitTabEntry* entry = find_entry(table->entries, table->capacity, key);
    bool isnew = entry->key == NULL;
    if(isnew && lit_value_isnull(entry->value))
    {
        table->count++;
    }
    entry->key = key;
    entry->value = value;
    return isnew;
}

bool lit_table_get(LitTable* table, LitString* key, LitValue* value)
{
    if(table->count == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->entries, table->capacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    *value = entry->value;
    return true;
}

bool lit_table_get_slot(LitTable* table, LitString* key, LitValue** value)
{
    if(table->count == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->entries, table->capacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    *value = &entry->value;
    return true;
}

bool lit_table_delete(LitTable* table, LitString* key)
{
    if(table->count == 0)
    {
        return false;
    }
    LitTabEntry* entry = find_entry(table->entries, table->capacity, key);
    if(entry->key == NULL)
    {
        return false;
    }
    entry->key = NULL;
    entry->value = lit_value_makebool(true);
    return true;
}

LitString* lit_table_find_string(LitTable* table, const char* chars, LitUInt length, uint32_t hash)
{
    if(table->count == 0)
    {
        return NULL;
    }
    uint32_t index = hash % table->capacity;
    while(true)
    {
        LitTabEntry* entry = &table->entries[index];
        if(entry->key == NULL)
        {
            if(lit_value_isnull(entry->value))
            {
                return NULL;
            }
        }
        else if(entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}

void lit_table_add_all(LitState* state, LitTable* from, LitTable* to)
{
    for(int i = 0; i <= from->capacity; i++)
    {
        LitTabEntry* entry = &from->entries[i];
        if(entry->key != NULL)
        {
            lit_table_set(state, to, entry->key, entry->value);
        }
    }
}

void lit_table_add_all_ignoring(LitState* state, LitTable* from, LitTable* to)
{
    LitValue fake;
    for(int i = 0; i <= from->capacity; i++)
    {
        LitTabEntry* entry = &from->entries[i];
        if(entry->key != NULL && !lit_table_get(to, entry->key, &fake))
        {
            lit_table_set(state, to, entry->key, entry->value);
        }
    }
}

void lit_table_remove_white(LitTable* table)
{
    LitObject* obj;
    for(int i = 0; i <= table->capacity; i++)
    {
        LitTabEntry* entry = &table->entries[i];
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

void lit_mark_table(LitVm* vm, LitTable* table)
{
    for(int i = 0; i <= table->capacity; i++)
    {
        LitTabEntry* entry = &table->entries[i];
        lit_mark_object(vm, (LitObject*)entry->key);
        lit_mark_value(vm, entry->value);
    }
}

static bool measurecompilationtime;
static double lastsourcetime = 0;

void lit_state_enablecompilationtimemeasurement()
{
    measurecompilationtime = true;
}

void lit_state_defaulthandleerror(LitState* state, const char* message)
{
    (void)state;
    fflush(stdout);
    fprintf(stderr, "%s%s%s\n", COLOR_RED, message, COLOR_RESET);
    fflush(stderr);
}

void lit_state_defaulthandleprintf(LitState* state, const char* message)
{
    (void)state;
    printf("%s", message);
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
    state->print_fn = lit_state_defaulthandleprintf;
    state->had_error = false;
    state->roots = NULL;
    state->root_count = 0;
    state->root_capacity = 0;
    state->last_module = NULL;
    state->config.traceexecution = false;
    state->config.tracechunk = false;
    state->scanner = (LitScanner*)lit_sysmem_malloc(sizeof(LitScanner));
    state->parser = (LitParser*)lit_sysmem_malloc(sizeof(LitParser));
    lit_parser_init(state, (LitParser*)state->parser);
    state->emitter = (LitEmitter*)lit_sysmem_malloc(sizeof(LitEmitter));
    lit_emitter_init(state, state->emitter);
    state->event_system = (LitEventSystem*)lit_sysmem_malloc(sizeof(LitEventSystem));
    lit_eventsystem_init(state->event_system);
    state->vm = (LitVm*)lit_sysmem_malloc(sizeof(LitVm));
    lit_init_vm(state, state->vm);
    lit_api_init(state);
    lit_state_opencorelibrary(state);
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
    lit_eventsystem_destroy(state->event_system);
    lit_sysmem_free(state->event_system);
    lit_sysmem_free(state->scanner);
    lit_parser_destroy(state->parser);
    lit_sysmem_free(state->parser);
    lit_emitter_destroy(state->emitter);
    lit_sysmem_free(state->emitter);
    lit_free_vm(state->vm);
    lit_sysmem_free(state->vm);
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
            case LIT_OBJ_FUNCTION:
            case LIT_OBJ_CLOSURE:
            case LIT_OBJ_CLOSUREPROTOTYPE:
            case LIT_OBJ_NATIVEFUNCTION:
            case LIT_OBJ_NATIVEPRIMITIVE:
            case LIT_OBJ_BOUNDMETHOD:
            case LIT_OBJ_PRIMITIVEMETHOD:
            case LIT_OBJ_NATIVEMETHOD:
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
    for(LitUInt i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }
    lit_exprlist_destroy(statements);
}

LitResult lit_state_interpretsource(LitState* state, const char* modname, char* code)
{
    return lit_state_interninterpretsource(state, lit_string_copy(state, modname, strlen(modname)), code);
}

LitModule* lit_state_compilemodulesource(LitState* state, LitString* modname, char* code)
{
    bool allowedgc = state->allow_gc;
    state->allow_gc = false;
    state->had_error = false;
    LitModule* module = NULL;
    // This is a lbc format
    if((code[1] << 8 | code[0]) == LIT_BYTECODE_MAGIC_NUMBER)
    {
        module = lit_load_module(state, code);
    }
    else
    {
        clock_t t = 0;
        clock_t total_t = 0;
        if(measurecompilationtime)
        {
            total_t = t = clock();
        }
        LitDynListExpr statements;
        lit_exprlist_init(&statements);
        if(lit_parser_parsesource(state->parser, modname->chars, code, &statements))
        {
            relstmts(state, &statements);
            return NULL;
        }
        if(measurecompilationtime)
        {
            printf("Parsing:        %gms\n", (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
            t = clock();
        }
        module = lit_emitter_emitmod(state->emitter, &statements, modname);
        relstmts(state, &statements);
        if(measurecompilationtime)
        {
            printf("Emitting:       %gms\n", (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
            printf("\nTotal:          %gms\n-----------------------\n", (double)(clock() - total_t) / CLOCKS_PER_SEC * 1000 + lastsourcetime);
        }
    }
    state->allow_gc = allowedgc;
    return state->had_error ? NULL : module;
}

LitModule* lit_state_getmodulebyname(LitState* state, const char* name)
{
    LitValue value;
    if(lit_table_get(&state->vm->modules->values, CONST_STRING(state, name), &value))
    {
        return AS_MODULE(value);
    }
    return NULL;
}

LitResult lit_state_interninterpretsource(LitState* state, LitString* modname, char* code)
{
    LitModule* module = lit_state_compilemodulesource(state, modname, code);
    if(module == NULL)
    {
        return (LitResult){ INTERPRET_COMPILE_ERROR, lit_value_makenull() };
    }
    LitResult result = lit_interpret_module(state, module);
    state->last_module = module;
    return result;
}

char* lit_util_patchfilename(char* file_name)
{
    int namelength = strlen(file_name);
    // Check, if our file_name ends with .lit or lbc, and remove it
    if(namelength > 4 && (memcmp(file_name + namelength - 4, ".lit", 4) == 0 || memcmp(file_name + namelength - 4, ".lbc", 4) == 0))
    {
        file_name[namelength - 4] = '\0';
        namelength -= 4;
    }
    // Check, if our file_name starts with ./ and remove it (useless, and makes the module name be ..main)
    if(namelength > 2 && memcmp(file_name, "./", 2) == 0)
    {
        file_name += 2;
        namelength -= 2;
    }
    for(int i = 0; i < namelength; i++)
    {
        char c = file_name[i];
        if(c == '/' || c == '\\')
        {
            file_name[i] = '.';
        }
    }
    return file_name;
}

char* lit_util_dupstring(const char* string)
{
    size_t length = strlen(string) + 1;
    char* newstring = (char*)lit_sysmem_malloc(length);
    memcpy(newstring, string, length);
    return newstring;
}

bool lit_state_compileandsavefiles(LitState* state, char* files[], LitUInt numfiles, const char* outputfile)
{
    LitModule* compiledmodules[numfiles];
    for(LitUInt i = 0; i < numfiles; i++)
    {
        char* file_name = lit_util_dupstring(files[i]);
        char* source = lit_read_file(file_name);
        if(source == NULL)
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Failed to open file '%s'", file_name);
            return false;
        }
        file_name = lit_util_patchfilename(file_name);
        LitString* modname = lit_string_copy(state, file_name, strlen(file_name));
        LitModule* module = lit_state_compilemodulesource(state, modname, source);
        compiledmodules[i] = module;
        lit_sysmem_free((void*)source);
        lit_sysmem_free((void*)file_name);
        if(module == NULL)
        {
            return false;
        }
    }
    FILE* file = fopen(outputfile, "w+b");
    if(file == NULL)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to open for writing file '%s'", outputfile);
        return false;
    }
    lit_write_uint16_t(file, LIT_BYTECODE_MAGIC_NUMBER);
    lit_write_uint8_t(file, LIT_BYTECODE_VERSION);
    lit_write_uint16_t(file, numfiles);
    for(LitUInt i = 0; i < numfiles; i++)
    {
        lit_save_module(compiledmodules[i], file);
    }
    lit_write_uint16_t(file, LIT_BYTECODE_END_NUMBER);
    fclose(file);
    return true;
}

char* lit_util_readsource(LitState* state, const char* file_name)
{
    clock_t t = 0;
    if(measurecompilationtime)
    {
        t = clock();
    }
    char* source = lit_read_file(file_name);
    if(source == NULL)
    {
        lit_state_raiseerror(state, RUNTIME_ERROR, "Failed to open file '%s'", file_name);
    }
    if(measurecompilationtime)
    {
        printf("Reading source: %gms\n", lastsourcetime = (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
    }
    return source;
}

LitResult lit_state_interpretfile(LitState* state, const char* file)
{
    char* source = lit_util_readsource(state, file);
    if(source == NULL)
    {
        return INTERPRET_RUNTIME_FAIL;
    }
    LitResult result = lit_state_interpretsource(state, file, source);
    lit_sysmem_free((void*)source);
    return result;
}

LitResult lit_state_dumpfile(LitState* state, const char* file)
{
    char* source = lit_util_readsource(state, file);
    if(source == NULL)
    {
        return INTERPRET_RUNTIME_FAIL;
    }
    LitResult result;
    LitString* modname = lit_string_copy(state, file, strlen(file));
    LitModule* module = lit_state_compilemodulesource(state, modname, source);
    if(module == NULL)
    {
        result = INTERPRET_RUNTIME_FAIL;
    }
    else
    {
        lit_debug_disasmodule(module, source);
        result = (LitResult){ INTERPRET_OK, lit_value_makenull() };
    }
    lit_sysmem_free((void*)source);
    return result;
}

void lit_state_raiseerror(LitState* state, LitErrorType type, const char* message, ...)
{
    va_list args;
    (void)type;
    va_start(args, message);
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, message, argscopy) + 1;
    va_end(argscopy);
    char buffer[buffersize];
    vsnprintf(buffer, buffersize, message, args);
    va_end(args);
    state->error_fn(state, buffer);
    state->had_error = true;
}

void lit_printf(LitState* state, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, message, argscopy) + 1;
    va_end(argscopy);
    char buffer[buffersize];
    vsnprintf(buffer, buffersize, message, args);
    va_end(args);
    state->print_fn(state, buffer);
}


const char* lit_tostring_typename(LitObjType t)
{
    
    static const char* lit_object_type_names[] =
    {
        "string",        "function",          "nativefunction", "nativeprimitive",
        "nativemethod", "primitivemethod",  "fiber",           "module",
        "closure",       "clsproto", "upvalue",         "class",
        "instance",      "boundmethod",      "array",           "array",
        "map",           "userdata",          "range",           "field",
        "reference"
    };
    return lit_object_type_names[t];
}

#define lit_vmexec_pushgc(state, allow) \
    bool wasallowed = state->allow_gc; \
    state->allow_gc = allow;

#define lit_vmexec_popgc(state) \
    state->allow_gc = wasallowed;



void lit_vmexec_traceframe(LitVm* vm, LitFiber* fiber)
{
    if(vm->state->config.traceexecution)
    {
        lit_debug_traceframe(fiber);
    }
}

void lit_vmexec_resetvm(LitState* state, LitVm* vm)
{
    vm->state = state;
    vm->objects = NULL;
    vm->fiber = NULL;
    vm->gray_stack = NULL;
    vm->gray_count = 0;
    vm->gray_capacity = 0;
    lit_init_table(&vm->strings);
    vm->globals = NULL;
    vm->modules = NULL;
}

void lit_init_vm(LitState* state, LitVm* vm)
{
    lit_vmexec_resetvm(state, vm);
    vm->globals = lit_object_makemap(state, NULL);
    vm->modules = lit_object_makemap(state, NULL);
}

void lit_free_vm(LitVm* vm)
{
    lit_free_table(vm->state, &vm->strings);
    lit_free_objects(vm->state, vm->objects);
    lit_vmexec_resetvm(vm->state, vm);
}

bool lit_vm_handleerror(LitVm* vm, LitString* errorstring)
{
    LitValue error = lit_value_fromobject(errorstring);
    LitFiber* fiber = vm->fiber;
    while(fiber != NULL)
    {
        fiber->error = error;
        if(fiber->catcher)
        {
            fiber->caught = true;
            vm->fiber = fiber->parent;
            if(vm->fiber->return_address != NULL)
            {
                *vm->fiber->return_address = error;
            }
            return true;
        }
        LitFiber* caller = fiber->parent;
        fiber->parent = NULL;
        fiber = caller;
    }
    fiber = vm->fiber;
    fiber->abort = true;
    fiber->error = error;
    if(fiber->parent != NULL)
    {
        fiber->parent->abort = true;
    }
    // Maan, formatting c strings is hard...
    int count = (int)fiber->framecount - 1;
    size_t length = snprintf(NULL, 0, "%s%s\n", COLOR_RED, errorstring->chars);
    for(int i = count; i >= 0; i--)
    {
        LitCallFrame* frame = &fiber->framevals[i];
        LitFunction* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->chars;
        if(chunk->haslineinfo)
        {
            length += snprintf(NULL, 0, "[line %d] in %s()\n", lit_chunk_getline(chunk, frame->ip - chunk->compiledcodechunk - 1), name);
        }
        else
        {
            length += snprintf(NULL, 0, "\tin %s()\n", name);
        }
    }
    length += snprintf(NULL, 0, "%s", COLOR_RESET);
    char buffer[length + 1];
    buffer[length] = '\0';
    char* start = buffer + sprintf(buffer, "%s%s\n", COLOR_RED, errorstring->chars);
    for(int i = count; i >= 0; i--)
    {
        LitCallFrame* frame = &fiber->framevals[i];
        LitFunction* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->chars;
        if(chunk->haslineinfo)
        {
            start += sprintf(start, "[line %d] in %s()\n", lit_chunk_getline(chunk, frame->ip - chunk->compiledcodechunk - 1), name);
        }
        else
        {
            start += sprintf(start, "\tin %s()\n", name);
        }
    }
    start += sprintf(start, "%s", COLOR_RESET);
    lit_state_raiseerror(vm->state, RUNTIME_ERROR, buffer);
    return false;
}

bool lit_vm_raiseerrorva(LitVm* vm, const char* format, va_list args)
{
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, format, argscopy) + 1;
    va_end(argscopy);
    char buffer[buffersize];
    vsnprintf(buffer, buffersize, format, args);
    return lit_vm_handleerror(vm, lit_string_copy(vm->state, buffer, buffersize));
}

bool lit_vm_raiseerror(LitVm* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = lit_vm_raiseerrorva(vm, format, args);
    va_end(args);
    return result;
}

bool lit_vm_raisefatalerror(LitVm* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = lit_vm_raiseerrorva(vm, format, args);
    va_end(args);
    lit_native_exit_jump();
    return result;
}

bool lit_vmexec_callcallable(LitVm* vm, LitFunction* function, LitClosure* closure, uint8_t argc, LitUInt calleeregister)
{
    LitFiber* fiber = vm->fiber;
    assert(fiber->framecount > 0);
    #if 0
    if(fiber->framecount == LIT_CALL_FRAMES_MAX)
    {
        lit_vm_raiseerror(vm, "Stack overflow");
        return false;
    }
    #endif
    if(fiber->framecount + 1 > fiber->framecapacity)
    {
        LitUInt newcapacity = ((fiber->framecapacity + 1) * 2);
        fiber->framevals = (LitCallFrame*)lit_sysmem_realloc(fiber->framevals, sizeof(LitCallFrame) * newcapacity);
        fiber->framecapacity = newcapacity;
    }
    LitCallFrame* frame = &fiber->framevals[fiber->framecount++];
    LitCallFrame* previousframe = &fiber->framevals[fiber->framecount - 2];
    frame->function = function;
    frame->closure = closure;
    frame->ip = function->chunk.compiledcodechunk;
    frame->slots = previousframe->slots + calleeregister;
    frame->result_ignored = false;
    frame->return_to_c = false;
    frame->return_address = previousframe->slots + (int)calleeregister;
    lit_fiber_ensureregisters(fiber, frame->slots - fiber->registers + function->max_registers);
    LitUInt targetargcount = function->argcount;
    bool vararg = function->vararg;
    if(targetargcount > argc)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif
        for(LitUInt i = argc; i < targetargcount; i++)
        {
            *(frame->slots + i + 1) = lit_value_makenull();
        }
        if(vararg)
        {
            *(frame->slots + targetargcount) = lit_value_fromobject(lit_object_makearray(vm->state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == argc && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            // No need to repack the arguments
        }
        else
        {
            LitArray* array = &lit_object_makevararray(vm->state)->array;
            lit_state_pushroot(vm->state, (LitObject*)array);
            lit_dynlistval_ensuresize(&array->values, argc - targetargcount + 1);
            LitUInt j = 0;
            for(LitUInt i = targetargcount - 1; i < argc; i++)
            {
                array->values.values[j++] = *(frame->slots + i + 1);
            }
            *(frame->slots + targetargcount) = lit_value_fromobject(array);
            lit_state_poproot(vm->state);
        }
    }
    return true;
}

bool lit_vmexec_actualcallvalue(LitVm* vm, LitUInt calleeregister, uint8_t argc, const LitValue alternatecallee)
{
    LitCallFrame* frame = &vm->fiber->framevals[vm->fiber->framecount - 1];
    LitValue callee = lit_value_isnull(alternatecallee) ? frame->slots[calleeregister] : alternatecallee;
    if(lit_value_isobject(callee))
    {
        if(lit_set_native_exit_jump())
        {
            bool caught = vm->fiber->caught;
            vm->fiber->caught = false;
            return !caught;
        }
        switch(OBJECT_TYPE(callee))
        {
            case LIT_OBJ_FUNCTION:
            {
                return lit_vmexec_callcallable(vm, AS_FUNCTION(callee), NULL, argc, calleeregister);
            }
            case LIT_OBJ_CLOSURE:
            {
                LitClosure* closure = AS_CLOSURE(callee);
                return lit_vmexec_callcallable(vm, closure->function, closure, argc, calleeregister);
            }
            case LIT_OBJ_NATIVEFUNCTION:
            {
                // For some reason, single line expression doesn't work
                LitValue value = AS_NATIVE_FUNCTION(callee)->function(vm, argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                return !vm->fiber->abort;
            }
            case LIT_OBJ_NATIVEPRIMITIVE:
            {
                lit_vmexec_pushgc(vm->state, false);
                bool result = AS_NATIVE_PRIMITIVE(callee)->function(vm, argc, frame->slots + calleeregister + 1);
                lit_vmexec_popgc(vm->state);
                return !result;
            }
            case LIT_OBJ_NATIVEMETHOD:
            {
                lit_vmexec_pushgc(vm->state, false);
                LitNativeMethod* method = AS_NATIVE_METHOD(callee);
                LitFiber* fiber = vm->fiber;
                // For some reason, single line expression doesn't work
                LitValue value = method->method(vm, *(frame->slots + calleeregister), argc, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;
                lit_vmexec_popgc(vm->state);
                return !fiber->abort;
            }
            case LIT_OBJ_PRIMITIVEMETHOD:
            {
                lit_vmexec_pushgc(vm->state, false);
                bool result = AS_PRIMITIVE_METHOD(callee)->method(vm, *(frame->slots + calleeregister), argc, frame->slots + calleeregister + 1);
                lit_vmexec_popgc(vm->state);
                return !result;
            }
            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* instance = lit_object_makeinstance(vm->state, klass);
                frame->slots[calleeregister] = lit_value_fromobject(instance);
                if(klass->init_method != NULL)
                {
                    return lit_vmexec_actualcallvalue(vm, calleeregister, argc, lit_value_fromobject(klass->init_method));
                }
                return true;
            }
            case LIT_OBJ_BOUNDMETHOD:
            {
                LitBoundMethod* boundmethod = AS_BOUND_METHOD(callee);
                LitValue method = boundmethod->method;
                if(IS_NATIVE_METHOD(method))
                {
                    lit_vmexec_pushgc(vm->state, false);
                    // For some reason, single line expression doesn't work
                    LitValue value = AS_NATIVE_METHOD(method)->method(vm, boundmethod->receiver, argc, frame->slots + calleeregister + 1);
                    frame->slots[calleeregister] = value;
                    lit_vmexec_popgc(vm->state);
                    return !vm->fiber->abort;
                }
                else if(IS_PRIMITIVE_METHOD(method))
                {
                    lit_vmexec_pushgc(vm->state, false);
                    if(AS_PRIMITIVE_METHOD(method)->method(vm, boundmethod->receiver, argc, frame->slots + calleeregister + 1))
                    {
                        lit_vmexec_popgc(vm->state);
                        return false;
                    }
                    lit_vmexec_popgc(vm->state);
                    return true;
                }
                else
                {
                    frame->slots[calleeregister] = boundmethod->receiver;
                    return lit_vmexec_callcallable(vm, AS_FUNCTION(method), NULL, argc, calleeregister);
                }
                return !vm->fiber->abort;
            }
            default:
            {
                break;
            }
        }
    }
    if(lit_value_isnull(callee))
    {
        return lit_vm_raiseerror(vm, "Attempt to call a null value");
    }
    else
    {
        return lit_vm_raiseerror(vm, "Can only call functions and classes, got %s", lit_get_value_type(callee));
    }
    return true;
}

LitUpvalue* lit_vmexec_captureupvalue(LitState* state, LitValue* local)
{
    LitUpvalue* previousupvalue = NULL;
    LitUpvalue* upvalue = state->vm->fiber->open_upvalues;
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
        state->vm->fiber->open_upvalues = createdupvalue;
    }
    else
    {
        previousupvalue->next = createdupvalue;
    }
    return createdupvalue;
}

void lit_vmexec_closeupvalues(LitVm* vm, const LitValue* last)
{
    LitFiber* fiber = vm->fiber;
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
    LitVm* vm = state->vm;
    LitFiber* fiber = lit_object_makefiber(state, module, module->main_function);
    vm->fiber = fiber;
    LitResult result = lit_interpret_fiber(state, fiber);
    return result;
}

#define LIT_CONF_USECOMPUTEDGOTO 0

#if defined(__CPPCHECK__) || (!defined(__GNUC__))
    #define LIT_CONF_USECOMPUTEDGOTO 0
#endif

#define lit_vmmac_dispatchnext() \
    goto dispatch;

#if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    #define LABELNAME(nm) label_##nm
    #define CASE_CODE(name) \
        label_##name:
#else
    #define CASE_CODE(name) \
        case name:
#endif

#define lit_vmmac_readframe() \
    fiber = vm->fiber; \
    vm->frame = &fiber->framevals[fiber->framecount - 1]; \
    vm->current_chunk = &vm->frame->function->chunk; \
    vm->constants = vm->current_chunk->constants.values; \
    vm->ip = vm->frame->ip; \
    fiber->module = vm->frame->function->module; \
    vm->registers = vm->frame->slots; \
    vm->privates = fiber->module->privates; \
    vm->upvalues = vm->frame->closure == NULL ? NULL : vm->frame->closure->upvalues;

#define lit_vmmac_writeframe() \
    vm->frame->ip = vm->ip;

#define lit_vmmac_returnerror() \
    lit_vmexec_popgc(state); \
    return (LitResult){ INTERPRET_RUNTIME_ERROR, lit_value_makenull() };

#define lit_vmmac_recoverstate() \
    lit_vmmac_writeframe(); \
    fiber = vm->fiber; \
    if(fiber == NULL) \
    { \
        return (LitResult){ INTERPRET_OK, lit_value_makenull() }; \
    } \
    if(fiber->abort) \
    { \
        lit_vmmac_returnerror(); \
    } \
    lit_vmmac_readframe(); \
    lit_vmexec_traceframe(vm, fiber);

#define lit_vmmac_callvalue(callee, reg, argc) \
    if(!lit_vmexec_actualcallvalue(vm, reg, argc, callee)) \
    { \
        lit_vmmac_recoverstate(); \
    }

#define lit_vmmac_fail(...) \
    if(lit_vm_raiseerror(vm, __VA_ARGS__)) \
    { \
        lit_vmmac_recoverstate(); \
        lit_vmmac_dispatchnext(); \
    } \
    else \
    { \
        lit_vmmac_returnerror(); \
    }

#define lit_vmmac_getrc(r) \
    (IS_BIT_SET(r, 8) ? vm->constants[r & 0xff] : vm->registers[r])


#define lit_vmmac_invokemethoddefault(reg, bv, m, argc) \
    lit_vmmac_writeframe() \
    LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vmmac_fail("Only instances and classes have methods") \
    } \
    LitString* mthname = CONST_STRING(vm->state, m); \
    LitValue method; \
    if((lit_value_isinstance(bv) && (lit_table_get(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_get(&klass->methods, mthname, &method)) \
    { \
        lit_vmmac_callvalue(method, reg, argc) \
    } \
    else \
    { \
        lit_vmmac_fail("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars) \
    } \
    lit_vmmac_readframe()

#define lit_vmmac_invokemethodandcontinue(reg, bv, m, argc) \
    lit_vmmac_writeframe() \
    LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vmmac_fail("Only instances and classes have methods"); \
    } \
    LitString* mthname = CONST_STRING(vm->state, m); \
    LitValue method; \
    if((lit_value_isinstance(bv) && (lit_table_get(&lit_value_asinstance(bv)->fields, mthname, &method))) || lit_table_get(&klass->methods, mthname, &method)) \
    { \
        lit_vmmac_callvalue(method, reg, argc); \
        lit_vmmac_readframe(); \
        lit_vmmac_dispatchnext(); \
    }


// Instruction helpers
#define BINARY_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(vm->instruction); \
    uint16_t b = LIT_INSTRUCTION_B(vm->instruction); \
    uint16_t c = LIT_INSTRUCTION_C(vm->instruction); \
    LitValue bv = lit_vmmac_getrc(b); \
    LitValue cv = lit_vmmac_getrc(c); \
    if(lit_value_isnumber(bv)) \
    { \
        if(!lit_value_isnumber(cv)) \
        { \
            lit_vmmac_fail("Attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        vm->registers[a] = type(lit_value_asnumber(bv) op lit_value_asnumber(cv)); \
    } \
    else if(lit_value_isnull(bv)) \
    { \
        lit_vmmac_fail("Attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        vm->registers[a] = lit_vmmac_getrc(b);; \
        LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);; \
        lit_vmmac_invokemethoddefault(a, vm->registers[a], opstring, 1); \
        vm->registers[a + 1] = tmpb;; \
    }

#define COMPARISON_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(vm->instruction); \
    uint16_t b = LIT_INSTRUCTION_B(vm->instruction); \
    uint16_t c = LIT_INSTRUCTION_C(vm->instruction); \
    LitValue bv = lit_vmmac_getrc(b); \
    LitValue cv = lit_vmmac_getrc(c); \
    if(lit_value_isnumber(bv)) \
    { \
        if(!lit_value_isnumber(cv)) \
        { \
            lit_vmmac_fail("Attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        vm->registers[a] = type(lit_value_asnumber(bv) op lit_value_asnumber(cv)); \
    } \
    else if(lit_value_isnull(bv)) \
    { \
        lit_vmmac_fail("Attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        vm->registers[a] = lit_vmmac_getrc(b);; \
        LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);; \
        lit_vmmac_invokemethoddefault(a, vm->registers[a], opstring, 1); \
        vm->registers[a + 1] = tmpb;; \
    }



LitResult lit_interpret_fiber(LitState* state, LitFiber* fiber)
{
    assert(fiber->framecount > 0);
    state->vm->fiber = fiber;
    // Has to be inside of the function in order for goto to work
    
    #if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    static void* dispatchtable[] = {
        &&LABELNAME(OP_MOVE),// R(A) := RC(B)
        &&LABELNAME(OP_LOAD_NULL),// R(A) := null
        &&LABELNAME(OP_LOAD_BOOL),// R(A) := (bool) B
        &&LABELNAME(OP_CLOSURE),// R(A) := PrC[Bx]
        &&LABELNAME(OP_ARRAY),// R(A) := new Array(Bx)
        &&LABELNAME(OP_OBJECT),// R(A) = new Object()
        &&LABELNAME(OP_RANGE),// R(A) = new Range(RC(B), RC(C))
        &&LABELNAME(OP_RETURN),// return R(A)
        &&LABELNAME(OP_ADD),// R(A) := RC(B) + RC(C)
        &&LABELNAME(OP_SUBTRACT),// R(A) := RC(B) - RC(C)
        &&LABELNAME(OP_MULTIPLY),// R(A) := RC(B) * RC(C)
        &&LABELNAME(OP_DIVIDE),// R(A) := RC(B) / RC(C)
        &&LABELNAME(OP_FLOOR_DIVIDE),// R(A) := floor(RC(B) / RC(C))
        &&LABELNAME(OP_MOD),// R(A) := RC(B) % RC(C)
        &&LABELNAME(OP_POWER),// R(A) := pow(RC(B), RC(C))
        &&LABELNAME(OP_LSHIFT),// R(A) := RC(B) << RC(C)
        &&LABELNAME(OP_RSHIFT),// R(A) := RC(B) >> RC(C)
        &&LABELNAME(OP_BXOR),// R(A) := RC(B) ^ RC(C)
        &&LABELNAME(OP_BAND),// R(A) := RC(B) & RC(C)
        &&LABELNAME(OP_BOR),// R(A) := RC(B) | RC(C)
        &&LABELNAME(OP_JUMP),// PC += sBx
        &&LABELNAME(OP_TRUE_JUMP),// if (R(A)) PC += Bx
        &&LABELNAME(OP_FALSE_JUMP),// if (not R(A)) PC += Bx
        &&LABELNAME(OP_NON_NULL_JUMP),// if (R(A) != null) PC += Bx
        &&LABELNAME(OP_NULL_JUMP),// if (R(A) == null) PC += Bx
        &&LABELNAME(OP_EQUAL),// R(A) := RC(B) == RC(C)
        &&LABELNAME(OP_LESS),// R(A) := RC(B) < RC(C)
        &&LABELNAME(OP_LESS_EQUAL),// R(A) := RC(B) <= RC(C)
        &&LABELNAME(OP_GREATER),// R(A) := RC(B) > RC(C)
        &&LABELNAME(OP_GREATER_EQUAL),// R(A) := RC(B) >= RC(C)
        &&LABELNAME(OP_NEGATE),// R(A) := -RC(B)
        &&LABELNAME(OP_NOT),// R(A) := !RC(B)
        &&LABELNAME(OP_BNOT),// R(A) := ~RC(B)
        &&LABELNAME(OP_SET_GLOBAL),// G[C(A)] := RC(BX)
        &&LABELNAME(OP_GET_GLOBAL),// R(A) := G[C(Bx)]
        &&LABELNAME(OP_SET_UPVALUE),// U[A] := RC(Bx)
        &&LABELNAME(OP_GET_UPVALUE),// R(A) := U[Bx]
        &&LABELNAME(OP_SET_PRIVATE),// P[A] := RC(Bx)
        &&LABELNAME(OP_GET_PRIVATE),// R(A) := P[C(Bx)]
        &&LABELNAME(OP_CALL),// R(A) := R(A)(R(A + 1), ..., R(A + B - 1))
        &&LABELNAME(OP_CLOSE_UPVALUE),// close_upvalue(R(A))
        &&LABELNAME(OP_CLASS),// G[C(A)] = R[C] = new_class(C(A), C(B - 1))
        &&LABELNAME(OP_STATIC_FIELD),// R(A)[C(B)] = RC(C)
        &&LABELNAME(OP_METHOD),// R(A).Methods[C(B)] = RC(C)
        &&LABELNAME(OP_GET_FIELD),// R(A) = R(B)[C(C)]
        &&LABELNAME(OP_GET_SUPER_METHOD),// R(A) = R(B).super[C(C)]
        &&LABELNAME(OP_SET_FIELD),// R(A)[C(B)] = R(C)
        &&LABELNAME(OP_IS),// R(A) := RC(B) is G[C(C)]
        &&LABELNAME(OP_INVOKE),// R(A) := R(A)[C(C)](R(A + 1), ..., R(A + B - 1))
        &&LABELNAME(OP_INVOKE_SUPER),// R(A) := R(A).super[C(C)](R(A + 1), ..., R(A + B - 1))
        &&LABELNAME(OP_SUBSCRIPT_GET),// R(A) := R(A)[RC(B)]
        &&LABELNAME(OP_SUBSCRIPT_SET),// R(A)[RC(B)] := R(C)
        &&LABELNAME(OP_PUSH_ARRAY_ELEMENT),// R(A)[R(A).count++] = RC(Bx)
        &&LABELNAME(OP_PUSH_OBJECT_ELEMENT),// R(A)[R(B)] = RC(C)
        &&LABELNAME(OP_REFERENCE_GLOBAL),// R(A) := ref G(C[Bx])
        &&LABELNAME(OP_REFERENCE_PRIVATE),// R(A) := ref P(Bx)
        &&LABELNAME(OP_REFERENCE_LOCAL),// R(A) := ref R(B)
        &&LABELNAME(OP_REFERENCE_UPVALUE),// R(A) := ref U(Bx)
        &&LABELNAME(OP_REFERENCE_FIELD),// R(A) = ref R(B)[C(C)]
        &&LABELNAME(OP_SET_REFERENCE),// ref R(A) := R(B)
    };
    #endif
    LitCallFrame* previousframe;
    LitVm* vm = state->vm;
    LitTable* globals;
    globals = &vm->globals->values;
    lit_vmexec_pushgc(state, true) fiber->abort = false;
    /*
    LitCallFrame* frame;
    LitChunk* current_chunk;
    LitValue* registers;
    LitValue* constants;
    LitValue* privates;
    LitUpvalue** upvalues;
    uint64_t* ip;
    uint64_t instruction;
    */
    lit_vmmac_readframe() vm->fiber = fiber;
    vm->registers[0] = lit_value_fromobject(vm->frame->function);
    lit_vmexec_traceframe(vm, fiber);
    if(state->config.traceexecution)
    {
        printf("fiber start:\n");
    }

dispatch:
    vm->instruction = *vm->ip++;
    if(state->config.traceexecution)
    {
        previousframe = vm->frame;
        if(vm->frame->function->max_registers > 0)
        {
            printf("        |\n        | f%i ", fiber->framecount);
            for(int i = 0; i < vm->frame->function->max_registers; i++)
            {
                printf("[ ");
                lit_value_printvalue(*(vm->registers + i));
                printf(" ]");
            }
            printf("\n");
        }
        lit_debug_disasinstr(vm->current_chunk, (LitUInt)(vm->ip - vm->current_chunk->compiledcodechunk - 1), NULL, vm->frame != previousframe);
        previousframe = vm->frame;
    }
    #if defined(LIT_CONF_USECOMPUTEDGOTO) && (LIT_CONF_USECOMPUTEDGOTO == 1)
    goto* dispatchtable[LIT_INSTRUCTION_OPCODE(vm->instruction)];
    #else
    switch(LIT_INSTRUCTION_OPCODE(vm->instruction))
    #endif
    {
        CASE_CODE(OP_MOVE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction));
            lit_vmmac_dispatchnext()
        }
        CASE_CODE(OP_LOAD_NULL)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_makenull();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LOAD_BOOL)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_makebool(LIT_INSTRUCTION_B(vm->instruction) != 0);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLOSURE)
        {
            LitClosurePrototype* clsproto = AS_CLOSURE_PROTOTYPE(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]);
            LitClosure* closure = lit_object_makeclosure(state, clsproto->function);
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(closure);
            for(LitUInt i = 0; i < closure->function->upvaluecount; i++)
            {
                uint8_t index = clsproto->indexes[i];
                if(clsproto->local[i])
                {
                    closure->upvalues[i] = lit_vmexec_captureupvalue(state, vm->registers + index);
                }
                else
                {
                    closure->upvalues[i] = vm->upvalues[index];
                }
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_ARRAY)
        {
            LitArray* array = lit_object_makearray(state);
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(array);
            lit_dynlistval_ensureactualsize(&array->values, LIT_INSTRUCTION_B(vm->instruction));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_OBJECT)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makeinstance(state, state->object_class));
            lit_vmmac_dispatchnext()
        }
        CASE_CODE(OP_RANGE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makerange(state, lit_value_asnumber(lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction))), lit_value_asnumber(lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)))));
            lit_vmmac_dispatchnext()
        }
        CASE_CODE(OP_RETURN)
        {
            LitValue value = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
            lit_vmexec_closeupvalues(vm, vm->registers);
            fiber->framecount--;
            if(vm->frame->return_to_c)
            {
                vm->frame->return_to_c = false;
                fiber->module->return_value = value;
                return (LitResult){ INTERPRET_OK, value };
            }
            if(fiber->framecount == 0 || vm->frame->return_address == NULL)
            {
                if(fiber->framecount == 0)
                {
                    fiber->module->return_value = value;
                }
                if(fiber->parent != NULL)
                {
                    vm->fiber = fiber->parent;
                    if(vm->fiber->return_address != NULL)
                    {
                        *vm->fiber->return_address = value;
                    }
                    if(state->config.traceexecution)
                    {
                        printf("fiber continue:\n");
                    }
                    lit_vmmac_readframe() lit_vmmac_dispatchnext()
                }
                return (LitResult){ INTERPRET_OK, value };
            }
            *vm->frame->return_address = value;
            lit_vmmac_readframe();
            lit_vmexec_traceframe(vm, fiber);
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
            uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
            uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
            uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
            LitValue bv = lit_vmmac_getrc(b);
            LitValue cv = lit_vmmac_getrc(c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                vm->registers[a] = lit_value_makenumber(floor(lit_value_asnumber(bv) / lit_value_asnumber(cv)));
            }
            else
            {
                vm->registers[a] = lit_vmmac_getrc(b);;
                LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);;
                lit_vmmac_invokemethoddefault(a, vm->registers[a], "#", 1);
                vm->registers[a + 1] = tmpb;;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_MOD)
        {
            uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
            uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
            uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
            LitValue bv = lit_vmmac_getrc(b);
            LitValue cv = lit_vmmac_getrc(c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                vm->registers[a] = lit_value_makenumber(fmod(lit_value_asnumber(bv), lit_value_asnumber(cv)));
            }
            else
            {
                vm->registers[a] = lit_vmmac_getrc(b);;
                LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);;
                lit_vmmac_invokemethoddefault(a, vm->registers[a], "%", 1);
                vm->registers[a + 1] = tmpb;;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_POWER)
        {
            uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
            uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
            uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
            LitValue bv = lit_vmmac_getrc(b);
            LitValue cv = lit_vmmac_getrc(c);
            if(lit_value_isnumber(bv) && lit_value_isnumber(cv))
            {
                vm->registers[a] = lit_value_makenumber(pow(lit_value_asnumber(bv), lit_value_asnumber(cv)));
            }
            else
            {
                vm->registers[a] = lit_vmmac_getrc(b);;
                LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);;
                lit_vmmac_invokemethoddefault(a, vm->registers[a], "**", 1);
                vm->registers[a + 1] = tmpb;;
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_LSHIFT)
        {
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction)); LitValue cv = lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)); if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv)) { lit_vmmac_fail("Operands of bitwise op %s must be two numbers, got %s and %s", "<<", lit_get_value_type(bv), lit_get_value_type(cv)); } vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (lit_value_makenumber((int)lit_value_asnumber(bv) <<(int) lit_value_asnumber(cv)));;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_RSHIFT)
        {
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction)); LitValue cv = lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)); if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv)) { lit_vmmac_fail("Operands of bitwise op %s must be two numbers, got %s and %s", ">>", lit_get_value_type(bv), lit_get_value_type(cv)); } vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (lit_value_makenumber((int)lit_value_asnumber(bv) >>(int) lit_value_asnumber(cv)));;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BXOR)
        {
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction)); LitValue cv = lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)); if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv)) { lit_vmmac_fail("Operands of bitwise op %s must be two numbers, got %s and %s", "^", lit_get_value_type(bv), lit_get_value_type(cv)); } vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (lit_value_makenumber((int)lit_value_asnumber(bv) ^(int) lit_value_asnumber(cv)));;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BAND)
        {
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction)); LitValue cv = lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)); if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv)) { lit_vmmac_fail("Operands of bitwise op %s must be two numbers, got %s and %s", "&", lit_get_value_type(bv), lit_get_value_type(cv)); } vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (lit_value_makenumber((int)lit_value_asnumber(bv) &(int) lit_value_asnumber(cv)));;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BOR)
        {
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction)); LitValue cv = lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)); if(!lit_value_isnumber(bv) && !lit_value_isnumber(cv)) { lit_vmmac_fail("Operands of bitwise op %s must be two numbers, got %s and %s", "|", lit_get_value_type(bv), lit_get_value_type(cv)); } vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (lit_value_makenumber((int)lit_value_asnumber(bv) |(int) lit_value_asnumber(cv)));;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_JUMP)
        {
            vm->ip += LIT_INSTRUCTION_SBX(vm->instruction);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_TRUE_JUMP)
        {
            if(!lit_is_falsey(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
            {
                vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_FALSE_JUMP)
        {
            if(lit_is_falsey(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
            {
                vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NON_NULL_JUMP)
        {
            if(!lit_value_isnull(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
            {
                vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NULL_JUMP)
        {
            if(lit_value_isnull(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
            {
                vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_EQUAL)
        {
            LitValue ptmp;
            uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
            uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
            uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
            LitValue bv = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction));
            if(lit_value_isinstance(bv))
            {
                vm->registers[a] = lit_vmmac_getrc(b);;
                LitValue tmpb = vm->registers[a + 1]; vm->registers[a + 1] = lit_vmmac_getrc(c);;
                lit_vmmac_invokemethodandcontinue(a, vm->registers[a], "==", 1);
                vm->registers[a + 1] = tmpb;
            }
            ptmp = lit_vmmac_getrc(c);
            vm->registers[a] = lit_value_makebool(lit_value_compare(vm->state, bv, ptmp));
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
            LitValue value = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction));
            if(!lit_value_isnumber(value))
            {
                // Don't even ask me why
                // This doesn't kill our performance, since it's a error anyway
                if(IS_STRING(value) && strcmp(AS_CSTRING(value), "muffin") == 0)
                {
                    lit_vmmac_fail("Idk, can you negate a muffin?");
                }
                else
                {
                    lit_vmmac_fail("Operand must be a number");
                }
            }
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_makenumber(-lit_value_asnumber(value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_NOT)
        {
            uint8_t b = LIT_INSTRUCTION_B(vm->instruction);
            LitValue value = lit_vmmac_getrc(b);
            if(lit_value_isinstance(value))
            {
                lit_vmmac_invokemethodandcontinue(b, value, "!", 0);
            }
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_makebool(lit_is_falsey(value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_BNOT)
        {
            LitValue value = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction));
            if(!lit_value_isnumber(value))
            {
                lit_vmmac_fail("Operand must be a number");
            }
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_makenumber(~((int)lit_value_asnumber(value)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_GLOBAL)
        {
            lit_table_set(state, globals, AS_STRING(vm->constants[LIT_INSTRUCTION_A(vm->instruction)]), lit_vmmac_getrc(LIT_INSTRUCTION_BX(vm->instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_GLOBAL)
        {
            LitValue* reg = &vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
            if(!lit_table_get(globals, AS_STRING(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]), reg))
            {
                *reg = lit_value_makenull();
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_UPVALUE)
        {
            *vm->frame->closure->upvalues[LIT_INSTRUCTION_A(vm->instruction)]->location = lit_vmmac_getrc(LIT_INSTRUCTION_BX(vm->instruction));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_UPVALUE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = *vm->frame->closure->upvalues[LIT_INSTRUCTION_BX(vm->instruction)]->location;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_PRIVATE)
        {
            uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
            uint32_t b = LIT_INSTRUCTION_BX(vm->instruction);
            vm->privates[(uint16_t)b] = IS_BIT_SET(b, 16) ? vm->constants[a] : vm->registers[a];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_PRIVATE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = vm->privates[LIT_INSTRUCTION_BX(vm->instruction)];
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CALL)
        {
            lit_vmmac_writeframe();
            if(!lit_vmexec_actualcallvalue(vm, LIT_INSTRUCTION_A(vm->instruction), LIT_INSTRUCTION_B(vm->instruction) - 1, lit_value_makenull()))
            {
                lit_vmmac_returnerror();
            }
            lit_vmmac_readframe();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLOSE_UPVALUE)
        {
            lit_vmexec_closeupvalues(vm, &vm->registers[LIT_INSTRUCTION_A(vm->instruction)] - 1);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_CLASS)
        {
            LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_A(vm->instruction)]);
            LitClass* klass = lit_object_makeclass(state, name);
            vm->registers[LIT_INSTRUCTION_C(vm->instruction)] = lit_value_fromobject(klass);
            lit_table_set(state, &vm->globals->values, name, lit_value_fromobject(klass));
            uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
            if(b == 0)
            {
                klass->super = state->object_class;
                lit_table_add_all(state, &klass->super->methods, &klass->methods);
                lit_table_add_all(state, &klass->super->static_fields, &klass->static_fields);
            }
            else
            {
                LitValue super = vm->registers[--b];
                if(!IS_CLASS(super))
                {
                    lit_vmmac_fail("Superclass must be a class");
                }
                LitClass* superklass = AS_CLASS(super);
                klass->super = superklass;
                klass->init_method = superklass->init_method;
                lit_table_add_all(state, &superklass->methods, &klass->methods);
                lit_table_add_all(state, &klass->super->static_fields, &klass->static_fields);
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_STATIC_FIELD)
        {
            lit_table_set(state, &AS_CLASS(vm->registers[LIT_INSTRUCTION_A(vm->instruction)])->static_fields, AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]), lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_METHOD)
        {
            LitClass* klass = AS_CLASS(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]);
            LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]);
            if((klass->init_method == NULL || (klass->super != NULL && klass->init_method == ((LitClass*)klass->super)->init_method)) && name->length == 11 && memcmp(name->chars, "constructor", 11) == 0)
            {
                klass->init_method = lit_value_asobject(lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)));
            }
            lit_table_set(state, &klass->methods, name, lit_vmmac_getrc(LIT_INSTRUCTION_C(vm->instruction)));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_FIELD)
        {
            LitValue object = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail("Attempt to index a null value");
            }
            LitValue value;
            LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            if(lit_value_isinstance(object))
            {
                LitInstance* instance = lit_value_asinstance(object);
                if(!lit_table_get(&instance->fields, name, &value))
                {
                    if(lit_table_get(&instance->klass->methods, name, &value))
                    {
                        if(IS_FIELD(value))
                        {
                            LitField* field = AS_FIELD(value);
                            if(field->getter == NULL)
                            {
                                lit_vmmac_fail("Class %s does not have a getter for the field %s", instance->klass->name->chars, name->chars);
                            }
                            lit_vmmac_writeframe();
                            lit_vmmac_callvalue(lit_value_fromobject(AS_FIELD(value)->getter), resultreg, 0);
                            lit_vmmac_readframe();
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
                if(lit_table_get(&klass->static_fields, name, &value))
                {
                    if(IS_NATIVE_METHOD(value) || IS_PRIMITIVE_METHOD(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                    else if(IS_FIELD(value))
                    {
                        LitField* field = AS_FIELD(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail("Class %s does not have a getter for the field %s", klass->name->chars, name->chars);
                        }
                        lit_vmmac_writeframe();
                        lit_vmmac_callvalue(lit_value_fromobject(field->getter), resultreg, 0);
                        lit_vmmac_readframe();
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
                    lit_vmmac_fail("Only instances and classes have fields");
                }
                if(lit_table_get(&klass->methods, name, &value))
                {
                    if(IS_FIELD(value))
                    {
                        LitField* field = AS_FIELD(value);
                        if(field->getter == NULL)
                        {
                            lit_vmmac_fail("Class %s does not have a getter for the field %s", klass->name->chars, name->chars);
                        }
                        lit_vmmac_writeframe();
                        lit_vmmac_callvalue(lit_value_fromobject(AS_FIELD(value)->getter), resultreg, 0);
                        lit_vmmac_readframe();
                        lit_vmmac_dispatchnext();
                    }
                    else if(IS_NATIVE_METHOD(value) || IS_PRIMITIVE_METHOD(value))
                    {
                        value = lit_value_fromobject(lit_object_makeboundmethod(state, object, value));
                    }
                }
                else
                {
                    value = lit_value_makenull();
                }
            }
            vm->registers[resultreg] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_GET_SUPER_METHOD)
        {
            LitValue instance = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
            LitClass* klass = AS_CLASS(instance);
            LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
            LitValue value;
            if(lit_table_get(&klass->methods, mthname, &value) || lit_table_get(&klass->static_fields, mthname, &value))
            {
                value = lit_value_fromobject(lit_object_makeboundmethod(state, vm->registers[LIT_INSTRUCTION_A(vm->instruction)], value));
            }
            else
            {
                value = lit_value_makenull();
            }
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_FIELD)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = vm->registers[resultreg];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("Attempt to index a null value");
            }
            LitValue value = vm->registers[LIT_INSTRUCTION_C(vm->instruction)];
            LitString* fieldname = AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]);
            if(IS_CLASS(instance))
            {
                LitClass* klass = AS_CLASS(instance);
                LitValue setter;
                if(lit_table_get(&klass->static_fields, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("Class %s does not have a setter for the field %s", klass->name->chars, fieldname->chars);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
                    lit_vmmac_readframe();
                    lit_vmmac_dispatchnext();
                }
                if(lit_value_isnull(value))
                {
                    lit_table_delete(&klass->static_fields, fieldname);
                }
                else
                {
                    lit_table_set(state, &klass->static_fields, fieldname, value);
                }
            }
            else if(lit_value_isinstance(instance))
            {
                LitInstance* inst = lit_value_asinstance(instance);
                LitValue setter;
                if(lit_table_get(&inst->klass->methods, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("Class %s does not have a setter for the field %s", inst->klass->name->chars, fieldname->chars);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
                    lit_vmmac_readframe();
                    lit_vmmac_dispatchnext();
                }
                if(lit_value_isnull(value))
                {
                    lit_table_delete(&inst->fields, fieldname);
                }
                else
                {
                    lit_table_set(state, &inst->fields, fieldname, value);
                }
            }
            else
            {
                LitClass* klass = lit_state_getclassfor(state, instance);
                if(klass == NULL)
                {
                    lit_vmmac_fail("Only instances and classes have fields");
                }
                LitValue setter;
                if(lit_table_get(&klass->methods, fieldname, &setter) && IS_FIELD(setter))
                {
                    LitField* field = AS_FIELD(setter);
                    if(field->setter == NULL)
                    {
                        lit_vmmac_fail("Class %s does not have a setter for the field %s", klass->name->chars, fieldname->chars);
                    }
                    lit_vmmac_writeframe();
                    lit_vmmac_callvalue(lit_value_fromobject(field->setter), resultreg, 1);
                    lit_vmmac_readframe();
                    lit_vmmac_dispatchnext();
                }
                else
                {
                    lit_vmmac_fail("Class %s does not contain field %s", klass->name->chars, fieldname->chars);
                }
            }
            vm->registers[resultreg] = value;
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_IS)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = lit_vmmac_getrc(LIT_INSTRUCTION_B(vm->instruction));
            if(lit_value_isnull(instance))
            {
                vm->registers[resultreg] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            LitClass* instanceklass = lit_state_getclassfor(state, instance);
            LitValue klass;
            if(!lit_table_get(globals, AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]), &klass))
            {
                vm->registers[resultreg] = lit_value_makebool(false);
                lit_vmmac_dispatchnext();
            }
            if(instanceklass == NULL || !IS_CLASS(klass))
            {
                lit_vmmac_fail("Operands must be an instance and a class");
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
            vm->registers[resultreg] = lit_value_makebool(found);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_INVOKE)
        {
            lit_vmmac_writeframe();
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = vm->registers[resultreg];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("Attempt to index a null value");
            }
            LitClass* klass = IS_CLASS(instance) ? AS_CLASS(instance) : lit_state_getclassfor(state, instance);
            if(klass == NULL)
            {
                lit_vmmac_fail("Only instances and classes have methods");
            }
            LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
            int argc = LIT_INSTRUCTION_B(vm->instruction) - 1;
            LitValue method;
            if(lit_value_isinstance(instance) && (lit_table_get(&lit_value_asinstance(instance)->fields, mthname, &method)))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else if(IS_CLASS(instance) && lit_table_get(&klass->static_fields, mthname, &method))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else if(lit_table_get(&klass->methods, mthname, &method))
            {
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else
            {
                lit_vmmac_fail("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars);
            }
            lit_vmmac_readframe();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_INVOKE_SUPER)
        {
            lit_vmmac_writeframe();
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = vm->registers[resultreg + 1];
            if(lit_value_isnull(instance))
            {
                lit_vmmac_fail("Attempt to index a null value");
            }
            LitClass* klass = AS_CLASS(instance);
            if(klass == NULL)
            {
                lit_vmmac_fail("Only instances and classes have methods");
            }
            LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
            int argc = LIT_INSTRUCTION_B(vm->instruction) - 1;
            LitValue method;
            if(lit_table_get(&klass->methods, mthname, &method) || lit_table_get(&klass->static_fields, mthname, &method))
            {
                for(LitUInt i = resultreg + 1; i <= resultreg + (LitUInt)argc; i++)
                {
                    vm->registers[i] = vm->registers[i + 1];
                }
                lit_vmmac_callvalue(method, resultreg, argc);
            }
            else
            {
                lit_vmmac_fail("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars);
            }
            lit_vmmac_readframe();
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SUBSCRIPT_GET)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = lit_vmmac_getrc(resultreg);
            lit_vmmac_invokemethoddefault(resultreg, instance, "[]", 1);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SUBSCRIPT_SET)
        {
            uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
            LitValue instance = lit_vmmac_getrc(resultreg);
            lit_vmmac_invokemethoddefault(resultreg, instance, "[]", 2);
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_PUSH_ARRAY_ELEMENT)
        {
            LitDynListVal* array = &AS_ARRAY(vm->registers[LIT_INSTRUCTION_A(vm->instruction)])->values;
            array->values[array->count++] = lit_vmmac_getrc(LIT_INSTRUCTION_BX(vm->instruction));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_PUSH_OBJECT_ELEMENT)
        {
            LitValue operand = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
            LitString* key = AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]);
            LitValue value = vm->registers[LIT_INSTRUCTION_C(vm->instruction)];
            if(lit_value_ismap(operand))
            {
                lit_table_set(state, &AS_MAP(operand)->values, key, value);
            }
            else if(lit_value_isinstance(operand))
            {
                lit_table_set(state, &lit_value_asinstance(operand)->fields, key, value);
            }
            else
            {
                lit_vmmac_fail("slotted an object or a map as the operand, got %s", lit_get_value_type(operand));
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_GLOBAL)
        {
            LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]);
            LitValue* value;
            if(lit_table_get_slot(&vm->globals->values, name, &value))
            {
                vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            }
            else
            {
                lit_vmmac_fail("Attempt to reference a null value");
            }
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_PRIVATE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makereference(state, &vm->privates[LIT_INSTRUCTION_BX(vm->instruction)]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_LOCAL)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makereference(state, &vm->registers[LIT_INSTRUCTION_B(vm->instruction)]));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_UPVALUE)
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makereference(state, vm->upvalues[LIT_INSTRUCTION_BX(vm->instruction)]->location));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_REFERENCE_FIELD)
        {
            LitValue object = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
            if(lit_value_isnull(object))
            {
                lit_vmmac_fail("Attempt to index a null value");
            }
            LitValue* value;
            LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
            if(lit_value_isinstance(object))
            {
                if(!lit_table_get_slot(&lit_value_asinstance(object)->fields, name, &value))
                {
                    lit_vmmac_fail("Attempt to reference a null value");
                }
            }
            else
            {
                lit_vmmac_fail("You can only reference fields of real instances");
            }
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_value_fromobject(lit_object_makereference(state, value));
            lit_vmmac_dispatchnext();
        }
        CASE_CODE(OP_SET_REFERENCE)
        {
            LitValue reference = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
            if(!IS_REFERENCE(reference))
            {
                lit_vmmac_fail("Provided value is not a reference");
            }
            *AS_REFERENCE(reference)->slot = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
            lit_vmmac_dispatchnext();
        }
        #if !defined(LIT_CONF_USECOMPUTEDGOTO) || (LIT_CONF_USECOMPUTEDGOTO == 0)
        default:
        #endif
            {
            lit_vmmac_fail("Unknown op %i", vm->instruction);
            lit_vmmac_returnerror();
            }
    }
}


void lit_native_exit_jump()
{
    longjmp(g_vmglobaljumpbuf, 1);
}

// Used for clean up on Ctrl+C / Ctrl+Z
static LitState* replstate;

void interupt_handler(int signalid)
{
    lit_state_destroy(replstate);
    printf("\nExiting (signalid=%d).\n", signalid);
    exit(0);
}

void run_repl(LitState* state)
{
    replstate = state;
    signal(SIGINT, interupt_handler);
#ifndef _WIN32
    signal(SIGTSTP, interupt_handler);
#endif
    printf("lit v%s, developed by @egordorichev\n", LIT_VERSION_STRING);
#ifdef LIT_USE_LIBREADLINE
    char* line;
#else
    char line[LIT_REPL_INPUT_MAX];
#endif
    while(true)
    {
        printf("%s>%s ", COLOR_BLUE, COLOR_RESET);
#ifdef LIT_USE_LIBREADLINE
        line = readline("");
        add_history(line);
#else
        if(!fgets(line, LIT_REPL_INPUT_MAX, stdin))
        {
            printf("\n");
            break;
        }
#endif
        if(strcmp(line, "exit"))
        {
            break;
        }
        LitResult result = lit_state_interpretsource(state, "repl", line);
        if(result.type == INTERPRET_OK && !lit_value_isnull(result.result))
        {
            printf("%s%s%s\n", COLOR_GREEN, lit_tostring_value(state, result.result, 0)->chars, COLOR_RESET);
        }
        lit_eventsystem_loop(state);
    }
}

void show_help()
{
    printf("lit [options] [files]\n");
    printf("\t-o --output [file]\tInstead of running the file the compiled bytecode will be saved.\n");
    printf("\t-e --eval [string]\tRuns the given code string.\n");
    printf("\t-p --pass [args]\tPasses the rest of the arguments to the script.\n");
    printf("\t-i --interactive\tStarts an interactive shell.\n");
    printf("\t-d --dump\t\tDumps all the bytecode chunks from the given file.\n");
    printf("\t-t --time\t\tMeasures and prints the compilation timings.\n");
    printf("\t-c --test\t\tRuns all tests (useful for code coverage testing).\n");
    printf("\t-h --help\t\tI wonder, what this option does.\n");
    printf("\tIf no code to run is provided, lit will try to run either main.lbc or main.lit and, if fails, default to an interactive shell will start.\n");
}



bool match_arg(const char* arg, const char* a, const char* b)
{
    return strcmp(arg, a) == 0 || strcmp(arg, b) == 0;
}

int main(int argc, char* argv[])
{
    bool dump;
    int64_t amount;
    LitState* state = lit_state_make();
    lit_state_openlibraries(state);
    char* filestorun[argc - 1];
    LitUInt numfilestorun = 0;
    LitStatusCode result = INTERPRET_OK;
    dump = false;
    amount = 0;
    for(int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        if(arg[0] == '-')
        {
            if(match_arg(arg, "-e", "--eval") || match_arg(arg, "-o", "--output"))
            {
                // It takes an extra argument, count it or we will use it as the file name to run :P
                i++;
            }
            else if(match_arg(arg, "-p", "--pass"))
            {
                // The rest of the args go to the script, go home pls
                break;
            }
            else if(match_arg(arg, "-d", "--dump"))
            {
                dump = true;
            }
            continue;
        }
        filestorun[numfilestorun++] = (char*)arg;
    }
    LitArray* argarray = NULL;
    bool showrepl = false;
    bool evaled = false;
    bool showedhelp = false;
    char* bytecodefile = NULL;
    for(int i = 1; i < argc; i++)
    {
        int argsleft = argc - i - 1;
        const char* arg = argv[i];
        if(match_arg(arg, "-e", "--eval"))
        {
            evaled = true;
            if(argsleft == 0)
            {
                fprintf(stderr, "Expected code to run for the eval argument.\n");
                return 1;
            }
            const char* string = argv[++i];
            size_t length = strlen(string) + 1;
            char source[length];
            memcpy(source, string, length);
            const char* modname = numfilestorun == 0 ? "repl" : filestorun[0];
            if(dump)
            {
                LitModule* module = lit_state_compilemodulesource(state, CONST_STRING(state, modname), source);
                if(module == NULL)
                {
                    goto endmain;
                }
                lit_debug_disasmodule(module, source);
            }
            else
            {
                result = lit_state_interpretsource(state, modname, source).type;
                if(result != INTERPRET_OK)
                {
                    goto endmain;
                }
            }
        }
        else if(match_arg(arg, "-h", "--help"))
        {
            show_help();
            showedhelp = true;
        }
        else if(match_arg(arg, "-t", "--trace"))
        {
            state->config.traceexecution = true;
        }
        else if(match_arg(arg, "--time", "--time"))
        {
            lit_state_enablecompilationtimemeasurement();
        }
        else if(match_arg(arg, "-i", "--interactive"))
        {
            showrepl = true;
        }
        else if(match_arg(arg, "-d", "--dump"))
        {
            dump = true;
        }
        else if(match_arg(arg, "-o", "--output"))
        {
            if(argsleft == 0)
            {
                fprintf(stderr, "Expected file name where to save the bytecode.\n");
                return 1;
            }
            bytecodefile = (char*)argv[++i];
        }
        else if(match_arg(arg, "-p", "--pass"))
        {
            argarray = lit_object_makearray(state);
            for(int j = 0; j < argsleft; j++)
            {
                const char* argstring = argv[i + j + 1];
                lit_vallist_push(&argarray->values, OBJECT_CONST_STRING(state, argstring));
            }
            lit_state_setglobal(state, CONST_STRING(state, "args"), lit_value_fromobject(argarray));
            break;
        }
        else if(arg[0] == '-')
        {
            fprintf(stderr, "Unknown argument '%s', run 'lit --help' for help.\n", arg);
            return 1;
        }
    }
    if(numfilestorun > 0)
    {
        if(bytecodefile != NULL)
        {
            if(!lit_state_compileandsavefiles(state, filestorun, numfilestorun, bytecodefile))
            {
                result = INTERPRET_COMPILE_ERROR;
            }
        }
        else
        {
            if(argarray == NULL)
            {
                argarray = lit_object_makearray(state);
            }
            lit_state_setglobal(state, CONST_STRING(state, "args"), lit_value_fromobject(argarray));
            for(LitUInt i = 0; i < numfilestorun; i++)
            {
                char* file = filestorun[i];
                if(dump)
                {
                    result = lit_state_dumpfile(state, file).type;
                }
                else
                {
                    result = lit_state_interpretfile(state, file).type;
                }
                if(result != INTERPRET_OK)
                {
                    goto endmain;
                }
            }
        }
    }
    if(showrepl)
    {
        run_repl(state);
    }
    else if(!showedhelp && !evaled && numfilestorun == 0)
    {
        if(lit_file_exists("main.lbc"))
        {
            result = lit_state_interpretfile(state, "main.lbc").type;
        }
        else if(lit_file_exists("main.lit"))
        {
            result = lit_state_interpretfile(state, "main.lit").type;
        }
        else
        {
            run_repl(state);
        }
    }
    lit_eventsystem_loop(state);
    amount = lit_state_destroy(state);
    endmain:
    if(amount != 0)
    {
        fprintf(stderr, "Error: memory leak of %i bytes!\n", (int)amount);
    }
    if(result != INTERPRET_OK)
    {
        return 1;
    }
    return 0;
}

