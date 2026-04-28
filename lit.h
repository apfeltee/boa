
#pragma once
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

    #define LIT_REPOSITORY "https://github.com/egordorichev/lit"

    #define LIT_VERSION_MAJOR 0
    #define LIT_VERSION_MINOR 4
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
    #define LIT_CALL_FRAMES_MAX 1024
    #define LIT_INITIAL_CALL_FRAMES 4
    #define LIT_CONTAINER_OUTPUT_MAX 10

    #if defined(__ANDROID__) || defined(_ANDROID_)
        #define LIT_OS_ANDROID
    #elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        #define LIT_OS_WINDOWS
    #elif __APPLE__
        #define LIT_OS_MAC
        #define LIT_OS_UNIX_LIKE
    #elif __linux__
        #define LIT_OS_LINUX
        #define LIT_OS_UNIX_LIKE
    #else
        #define LIT_OS_UNKNOWN
    #endif

    #ifdef LIT_OS_UNIX_LIKE
        #define LIT_USE_LIBREADLINE
    #endif

    #ifdef LIT_USE_LIBREADLINE
    #else
        #define LIT_REPL_INPUT_MAX 1024
    #endif

    #define LIT_EXIT_CODE_ARGUMENT_ERROR 1
    #define LIT_EXIT_CODE_MEM_LEAK 2
    #define LIT_EXIT_CODE_RUNTIME_ERROR 70
    #define LIT_EXIT_CODE_COMPILE_ERROR 65

    #define LIT_TESTS_DIRECTORY "tests"




    #if !defined(LIT_DISABLE_COLOR) && !defined(LIT_ENABLE_COLOR) && !(defined(LIT_OS_WINDOWS) || defined(EMSCRIPTEN))
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
        #define COLOR_WHITE "\x1B[37m"
    #else
        #define COLOR_RESET ""
        #define COLOR_RED ""
        #define COLOR_GREEN ""
        #define COLOR_YELLOW ""
        #define COLOR_BLUE ""
        #define COLOR_MAGENTA ""
        #define COLOR_CYAN ""
        #define COLOR_WHITE ""
    #endif


    #define UNREACHABLE                                                                 \
        fprintf(stderr, "Unreachable code was reached at %s:%i\n", __FILE__, __LINE__); \
        assert(false);
    #define NOT_IMPLEMENTED                                                               \
        fprintf(stderr, "Unimplemented code was reached at %s:%i\n", __FILE__, __LINE__); \
        assert(false);
    #define UINT8_COUNT UINT8_MAX + 1
    #define UINT16_COUNT UINT16_MAX + 1

    #define RETURN_RUNTIME_ERROR() return (LitResult){ INTERPRET_RUNTIME_ERROR, NULL_VALUE };
    #define INTERPRET_RUNTIME_FAIL ((LitResult){ INTERPRET_INVALID, NULL_VALUE })

    #define RETURN_OK(r) return (LitResult){ INTERPRET_OK, r };

    #define LIT_CHECK_NUMBER(id) lit_args_checknumber(vm, args, argc, id)
    #define LIT_GET_NUMBER(id, def) lit_args_getnumber(vm, args, argc, id, def)

    #define LIT_CHECK_BOOL(id) lit_args_checkbool(vm, args, argc, id)
    #define LIT_GET_BOOL(id, def) lit_args_getbool(vm, args, argc, id, def)

    #define LIT_CHECK_STRING(id) lit_args_checkstring(vm, args, argc, id)
    #define LIT_GET_STRING(id, def) lit_args_getstring(vm, args, argc, id, def)

    #define LIT_CHECK_OBJECT_STRING(id) lit_args_checkobjstring(vm, args, argc, id)
    #define LIT_CHECK_INSTANCE(id) lit_args_checkinstance(vm, args, argc, id)
    #define LIT_CHECK_REFERENCE(id) lit_check_reference(vm, args, argc, id)


    #define LIT_GET_FIELD(id) lit_table_getfield(vm->state, &AS_INSTANCE(instance)->fields, id)
    #define LIT_GET_MAP_FIELD(id) lit_map_getfield(vm->state, &AS_INSTANCE(instance)->fields, id)
    #define LIT_SET_FIELD(id, value) lit_table_setfield(vm->state, &AS_INSTANCE(instance)->fields, id, value)
    #define LIT_SET_MAP_FIELD(id, value) lit_map_setfield(vm->state, &AS_INSTANCE(instance)->fields, id, value)

    #define LIT_ENSURE_ARGS(count)                                                           \
        if(argc != count)                                                               \
        {                                                                                    \
            lit_vm_raisefatalerror(vm, "Expected %i argument, got %i", count, argc); \
            return NULL_VALUE;                                                               \
        }

    #define LIT_ENSURE_MIN_ARGS(count)                                                               \
        if(argc < count)                                                                        \
        {                                                                                            \
            lit_vm_raisefatalerror(vm, "Expected minimum %i argument, got %i", count, argc); \
            return NULL_VALUE;                                                                       \
        }

    #define LIT_ENSURE_MAX_ARGS(count)                                                               \
        if(argc > count)                                                                        \
        {                                                                                            \
            lit_vm_raisefatalerror(vm, "Expected maximum %i argument, got %i", count, argc); \
            return NULL_VALUE;                                                                       \
        }

    #define LIT_INSERT_DATA(type, cleanup)                                                                                      \
        ({                                                                                                                      \
            LitUserdata* userdata = lit_object_makeuserdata(vm->state, sizeof(type));                                               \
            userdata->cleanup_fn = cleanup;                                                                                     \
            lit_table_set(vm->state, &AS_INSTANCE(instance)->fields, CONST_STRING(vm->state, "_data"), OBJECT_VALUE(userdata)); \
            (type*)userdata->data;                                                                                              \
        })

    #define LIT_EXTRACT_DATA(type)                                                                    \
        ({                                                                                            \
            LitValue _d;                                                                              \
            if(!lit_table_get(&AS_INSTANCE(instance)->fields, CONST_STRING(vm->state, "_data"), &_d)) \
            {                                                                                         \
                lit_vm_raisefatalerror(vm, "Failed to extract userdata");                          \
            }                                                                                         \
            (type*)AS_USERDATA(_d)->data;                                                             \
        })

    #define LIT_EXTRACT_DATA_FROM(from, type)                                                     \
        ({                                                                                        \
            LitValue _d;                                                                          \
            if(!lit_table_get(&AS_INSTANCE(from)->fields, CONST_STRING(vm->state, "_data"), &_d)) \
            {                                                                                     \
                lit_vm_raisefatalerror(vm, "Failed to extract userdata");                      \
            }                                                                                     \
            (type*)AS_USERDATA(_d)->data;                                                         \
        })


    // Do not change these, or old bytecode files will break!
    #define LIT_BYTECODE_MAGIC_NUMBER 6932
    #define LIT_BYTECODE_END_NUMBER 2942
    #define LIT_STRING_KEY 48

    #define LIT_BEGIN_CLASS(name)                                               \
        {                                                                       \
            bool wasallowed = state->allow_gc;                                 \
            state->allow_gc = false;                                            \
            LitClass* klass = lit_object_makeclass(state, lit_string_copy(state, name, strlen(name)));



    #define LIT_END_CLASS_IGNORING()                            \
        lit_state_setglobal(state, klass->name, OBJECT_VALUE(klass)); \
        state->allow_gc = wasallowed;                          \
        }

    #define LIT_END_CLASS()                                     \
        lit_state_setglobal(state, klass->name, OBJECT_VALUE(klass)); \
        if(klass->super == NULL)                                \
        {                                                       \
            lit_class_inherit(state, klass, state->object_class);             \
        }                                                       \
        state->allow_gc = wasallowed;                          \
        }

    #define LIT_BIND_STATIC_GETTER(name, getter)                                                                                                                   \
        {                                                                                                                                                          \
            LitString* nm = lit_string_copy(state, name, strlen(name));                                                                                            \
            lit_table_set(state, &klass->static_fields, nm, OBJECT_VALUE(lit_object_makefield(state, (LitObject*)lit_object_makenativemethod(state, getter, nm), NULL))); \
        }


    #define lit_set_native_exit_jump() setjmp(lit_vmglobal_jumpbuf)

    #define OBJECT_TYPE(value) (AS_OBJECT(value)->type)

    #define IS_OBJECTS_TYPE(value, t) (IS_OBJECT(value) && AS_OBJECT(value)->type == t)
    #define IS_STRING(value) IS_OBJECTS_TYPE(value, LIT_OBJ_STRING)
    #define IS_FUNCTION(value) IS_OBJECTS_TYPE(value, LIT_OBJ_FUNCTION)
    #define IS_NATIVE_FUNCTION(value) IS_OBJECTS_TYPE(value, LIT_OBJ_NATIVEFUNCTION)
    #define IS_NATIVE_PRIMITIVE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_NATIVEPRIMITIVE)
    #define IS_NATIVE_METHOD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_NATIVEMETHOD)
    #define IS_PRIMITIVE_METHOD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_PRIMITIVEMETHOD)
    #define IS_MODULE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_MODULE)
    #define IS_CLOSURE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLOSURE)
    #define IS_CLOSURE_PROTOTYPE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLOSUREPROTOTYPE)
    #define IS_UPVALUE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_UPVALUE)
    #define IS_CLASS(value) IS_OBJECTS_TYPE(value, LIT_OBJ_CLASS)
    #define IS_INSTANCE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_INSTANCE)
    #define IS_ARRAY(value) (IS_OBJECTS_TYPE(value, LIT_OBJ_ARRAY) || IS_OBJECTS_TYPE(value, LIT_OBJ_VARARGARRAY))
    #define IS_VARARG_ARRAY(value) IS_OBJECTS_TYPE(value, LIT_OBJ_VARARGARRAY)
    #define IS_MAP(value) IS_OBJECTS_TYPE(value, LIT_OBJ_MAP)
    #define IS_BOUND_METHOD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_BOUNDMETHOD)
    #define IS_USERDATA(value) IS_OBJECTS_TYPE(value, LIT_OBJ_USERDATA)
    #define IS_RANGE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_RANGE)
    #define IS_FIELD(value) IS_OBJECTS_TYPE(value, LIT_OBJ_FIELD)
    #define IS_REFERENCE(value) IS_OBJECTS_TYPE(value, LIT_OBJ_REFERENCE)

    #define IS_CALLABLE_FUNCTION(value) lit_value_iscallablefunction(value)

    #define AS_STRING(value) ((LitString*)AS_OBJECT(value))
    #define AS_CSTRING(value) (((LitString*)AS_OBJECT(value))->chars)
    #define AS_FUNCTION(value) ((LitFunction*)AS_OBJECT(value))
    #define AS_NATIVE_FUNCTION(value) ((LitNativeFunction*)AS_OBJECT(value))
    #define AS_NATIVE_PRIMITIVE(value) ((LitNativePrimitive*)AS_OBJECT(value))
    #define AS_NATIVE_METHOD(value) ((LitNativeMethod*)AS_OBJECT(value))
    #define AS_PRIMITIVE_METHOD(value) ((LitPrimitiveMethod*)AS_OBJECT(value))
    #define AS_MODULE(value) ((LitModule*)AS_OBJECT(value))
    #define AS_CLOSURE(value) ((LitClosure*)AS_OBJECT(value))
    #define AS_CLOSURE_PROTOTYPE(value) ((LitClosurePrototype*)AS_OBJECT(value))
    #define AS_UPVALUE(value) ((LitUpvalue*)AS_OBJECT(value))
    #define AS_CLASS(value) ((LitClass*)AS_OBJECT(value))
    #define AS_INSTANCE(value) ((LitInstance*)AS_OBJECT(value))
    #define AS_ARRAY(value) ((LitArray*)AS_OBJECT(value))
    #define AS_MAP(value) ((LitMap*)AS_OBJECT(value))
    #define AS_BOUND_METHOD(value) ((LitBoundMethod*)AS_OBJECT(value))
    #define AS_USERDATA(value) ((LitUserdata*)AS_OBJECT(value))
    #define AS_RANGE(value) ((LitRange*)AS_OBJECT(value))
    #define AS_FIELD(value) ((LitField*)AS_OBJECT(value))
    #define AS_FIBER(value) ((LitFiber*)AS_OBJECT(value))
    #define AS_REFERENCE(value) ((LitReference*)AS_OBJECT(value))

    #define ALLOCATE_OBJECT(state, type, objectType) (type*)lit_object_allocobject(state, sizeof(type), objectType)
    #define OBJECT_CONST_STRING(state, text) OBJECT_VALUE(lit_string_copy((state), (text), strlen(text)))
    #define CONST_STRING(state, text) lit_string_copy((state), (text), strlen(text))

    #define TABLE_MAX_LOAD 0.75

    #define LIT_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)
    #define LIT_GROW_ARRAY(state, previous, type, oldcount, count) (type*)lit_reallocate(state, previous, sizeof(type) * (oldcount), sizeof(type) * (count))

    #define LIT_FREE_ARRAY(state, type, pointer, oldcount) lit_reallocate(state, pointer, sizeof(type) * (oldcount), 0)

    #define LIT_ALLOCATE(state, type, count) (type*)lit_reallocate(state, NULL, 0, sizeof(type) * (count))
    #define LIT_FREE(state, type, pointer) lit_reallocate(state, pointer, sizeof(type), 0)


    #define SIGN_BIT ((uint64_t)1 << 63u)
    #define QNAN ((uint64_t)0x7ffc000000000000u)

    #define TAG_NULL 1u
    #define TAG_FALSE 2u
    #define TAG_TRUE 3u

    #define IS_BOOL(v) (((v)&FALSE_VALUE) == FALSE_VALUE)
    #define IS_NULL(v) ((v) == NULL_VALUE)
    #define IS_NUMBER(v) (((v)&QNAN) != QNAN)
    #define IS_OBJECT(v) (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

    #define AS_BOOL(v) ((v) == TRUE_VALUE)
    #define AS_NUMBER(v) lit_value_to_number(v)
    #define AS_OBJECT(v) ((LitObject*)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))

    #define BOOL_VALUE(boolean) ((boolean) ? TRUE_VALUE : FALSE_VALUE)
    #define FALSE_VALUE ((LitValue)(uint64_t)(QNAN | TAG_FALSE))
    #define TRUE_VALUE ((LitValue)(uint64_t)(QNAN | TAG_TRUE))
    #define NULL_VALUE ((LitValue)(uint64_t)(QNAN | TAG_NULL))
    #define NUMBER_VALUE(num) lit_number_to_value(num)

    #define OBJECT_VALUE(obj) (LitValue)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))


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

    #define LIT_READ_ABC_INSTRUCTION(instruction)    \
        uint8_t a = LIT_INSTRUCTION_A(instruction);  \
        uint16_t b = LIT_INSTRUCTION_B(instruction); \
        uint16_t c = LIT_INSTRUCTION_C(instruction);

    #define LIT_READ_BX_INSTRUCTION(instruction)    \
        uint8_t a = LIT_INSTRUCTION_A(instruction); \
        uint32_t bx = LIT_INSTRUCTION_BX(instruction);

    #define LIT_READ_SBX_INSTRUCTION(instruction)   \
        uint8_t a = LIT_INSTRUCTION_A(instruction); \
        int32_t sbx = LIT_INSTRUCTION_SBX(instruction);

    #define LIT_FORM_ABC_INSTRUCTION(opcode, a, b, c) \
        (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | (((b)&LIT_B_ARG_SIZE) << LIT_B_ARG_POSITION) | (((c)&LIT_C_ARG_SIZE) << LIT_C_ARG_POSITION))

    #define LIT_FORM_ABX_INSTRUCTION(opcode, a, bx) \
        (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | (((bx)&LIT_BX_ARG_SIZE) << LIT_BX_ARG_POSITION))

    #define LIT_FORM_ASBX_INSTRUCTION(opcode, a, sbx)                                                                                                \
        (((opcode)&LIT_OPCODE_SIZE) | (((a)&LIT_A_ARG_SIZE) << LIT_A_ARG_POSITION) | ((abs((int)(sbx)) & LIT_SBX_ARG_SIZE) << LIT_SBX_ARG_POSITION)) \
        | ((((sbx) < 0 ? 1 : 0) << LIT_SBX_FLAG_POSITION))


enum LitObjectType
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

typedef enum /**/LitObjectType LitObjectType;
typedef enum /**/LitFunctionType LitFunctionType;
typedef enum /**/LitErrorType LitErrorType;
typedef enum /**/LitStatusCode LitStatusCode;
typedef enum /**/LitTokenType LitTokenType;
typedef enum /**/LitPrecedence LitPrecedence;

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
typedef struct /**/LitParamList LitParamList;
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
typedef struct /**/LitPrivList LitPrivList;
typedef struct /**/LitLocal LitLocal;
typedef struct /**/LitLocList LitLocList;
typedef struct /**/LitCompilerUpvalue LitCompilerUpvalue;
typedef struct /**/LitCompiler LitCompiler;
typedef struct /**/LitEmitter LitEmitter;
typedef struct /**/LitParseRule LitParseRule;
typedef struct /**/LitParser LitParser;
typedef struct /**/LitEmulatedFile LitEmulatedFile;
typedef struct /**/LitScanner LitScanner;

typedef uint32_t LitUInt;

typedef uint64_t LitValue;


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



typedef struct LitUIntList
{
    LitUInt capacity;
    LitUInt count;
    LitUInt* values;
} LitUIntList;

typedef struct LitByteList
{
    LitUInt capacity;
    LitUInt count;
    uint8_t* values;
} LitByteList;

typedef struct LitValList
{
    LitUInt capacity;
    LitUInt count;
    LitValue* values;
} LitValList;

typedef struct
{
    LitUInt count;
    LitUInt capacity;
    uint64_t* code;

    bool has_line_info;

    LitUInt line_count;
    LitUInt line_capacity;
    uint16_t* lines;

    LitValList constants;
} LitChunk;

typedef struct
{
    LitString* key;
    LitValue value;
} LitTableEntry;

typedef struct
{
    int count;
    int capacity;

    LitTableEntry* entries;
} LitTable;




typedef struct LitObject
{
    LitObjectType type;
    LitObject* next;

    bool marked;
} LitObject;


typedef struct LitString
{
    LitObject object;

    LitUInt length;
    uint32_t hash;
    char* chars;
} LitString;


typedef struct
{
    LitObject object;

    LitChunk chunk;
    LitString* name;

    uint8_t arg_count;
    uint16_t upvalue_count;
    uint8_t max_registers;

    bool vararg;

    LitModule* module;
} LitFunction;



typedef struct LitUpvalue
{
    LitObject object;

    LitValue* location;
    LitValue closed;

    LitUpvalue* next;
} LitUpvalue;


typedef struct
{
    LitObject object;
    LitFunction* function;

    LitUpvalue** upvalues;
    LitUInt upvalue_count;
} LitClosure;


typedef struct
{
    LitObject object;
    LitFunction* function;

    bool* local;
    uint8_t* indexes;

    LitUInt upvalue_count;
} LitClosurePrototype;


typedef struct
{
    LitObject object;

    LitNativeFunctionFn function;
    LitString* name;
} LitNativeFunction;



typedef struct
{
    LitObject object;

    LitNativePrimitiveFn function;
    LitString* name;
} LitNativePrimitive;


typedef struct
{
    LitObject object;

    LitNativeMethodFn method;
    LitString* name;
} LitNativeMethod;


typedef struct
{
    LitObject object;

    LitPrimitiveMethodFn method;
    LitString* name;
} LitPrimitiveMethod;


typedef struct
{
    LitFunction* function;
    LitClosure* closure;

    uint64_t* ip;
    LitValue* slots;
    LitValue* return_address;

    bool result_ignored;
    bool return_to_c;
} LitCallFrame;



typedef struct LitMap
{
    LitObject object;
    LitTable values;

    LitMapIndexFn index_fn;
} LitMap;



typedef struct LitModule
{
    LitObject object;

    LitValue return_value;
    LitString* name;

    LitValue* privates;
    LitMap* private_names;
    LitUInt private_count;

    LitFunction* main_function;
    LitFiber* main_fiber;

    bool ran;
} LitModule;


typedef struct LitFiber
{
    LitObject object;

    LitFiber* parent;

    LitValue* registers;
    LitUInt registers_allocated;

    LitCallFrame* frames;
    LitUInt frame_capacity;
    LitUInt frame_count;
    LitUInt arg_count;

    LitValue* return_address;
    LitUpvalue* open_upvalues;
    LitModule* module;
    LitValue error;

    bool abort;
    bool catcher;
    bool caught;
} LitFiber;



typedef struct LitClass
{
    LitObject object;

    LitString* name;
    LitObject* init_method;

    LitTable methods;
    LitTable static_fields;

    LitClass* super;
} LitClass;


typedef struct
{
    LitObject object;

    LitClass* klass;
    LitTable fields;
} LitInstance;


typedef struct
{
    LitObject object;

    LitValue receiver;
    LitValue method;
} LitBoundMethod;


typedef struct
{
    LitObject object;
    LitValList values;
} LitArray;


typedef struct
{
    LitArray array;
} LitVarargArray;

typedef struct LitUserdata
{
    LitObject object;

    void* data;
    size_t size;

    LitCleanupFn cleanup_fn;
} LitUserdata;


typedef struct
{
    LitObject object;

    double from;
    double to;
} LitRange;


typedef struct
{
    LitObject object;

    LitObject* getter;
    LitObject* setter;
} LitField;


typedef struct
{
    LitObject object;
    LitValue* slot;
} LitReference;



typedef struct LitEvent
{
    uint64_t expire_time;
    LitValue callback;

    struct LitEvent* next;
    struct LitEvent* previous;
} LitEvent;

typedef struct LitEventSystem
{
    LitEvent* events;
    LitEvent* last_event;
} LitEventSystem;


typedef struct LitState
{
    struct {
        bool traceexecution;
        bool tracechunk;
        /*
        lit_trace_null_fill
        lit_minimize_containers
        lit_log_gc
        lit_log_allocation
        lit_log_marking
        lit_log_blacking
        lit_stress_test_gc
        */
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
    // When adding another class here, DO NOT forget to mark it in lit_mem.c or it will be GC-ed
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
} LitState;





typedef struct LitVm
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

} LitVm;

typedef struct LitResult LitResult;
struct LitResult
{
    LitStatusCode type;
    LitValue result;
};


typedef struct LitToken LitToken;
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

typedef struct LitExprList LitExprList;
struct LitExprList
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

struct LitPrivList
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

struct LitLocList 
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
    LitLocList locals;
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
    LitPrivList privates;
    LitUIntList breaks;
    LitUIntList continues;
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


extern jmp_buf lit_vmglobal_jumpbuf;


#include "proto.inc"

void lit_uintlist_init(LitUIntList* array);
void lit_uintlist_destroy(LitState* state, LitUIntList* array);
void lit_uintlist_push(LitState* state, LitUIntList* array, LitUInt value);


void lit_bytelist_init(LitByteList* array);
void lit_bytelist_destroy(LitState* state, LitByteList* array);
void lit_bytelist_push(LitState* state, LitByteList* array, uint8_t value);


void lit_ast_destroyexpression(LitState* state, LitExpression* expression);


/* api.c */
void lit_api_init(LitState *state);
void lit_api_destroy(LitState *state);
LitValue lit_state_getglobal(LitState *state, LitString *name);
LitFunction *lit_state_getglobalfunction(LitState *state, LitString *name);
void lit_state_setglobal(LitState *state, LitString *name, LitValue value);
bool lit_state_globalexists(LitState *state, LitString *name);
void lit_state_defnative(LitState *state, const char *name, LitNativeFunctionFn native);
void lit_state_defnativeprimitive(LitState *state, const char *name, LitNativePrimitiveFn native);
double lit_args_checknumber(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
double lit_args_getnumber(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id, double def);
bool lit_args_checkbool(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
bool lit_args_getbool(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id, bool def);
const char *lit_args_checkstring(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
const char *lit_args_getstring(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id, const char *def);
LitString *lit_args_checkobjstring(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
LitInstance *lit_args_checkinstance(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
LitValue *lit_check_reference(LitVm *vm, LitValue *args, uint8_t argc, uint8_t id);
void lit_args_ensurebool(LitVm *vm, LitValue value, const char *error);
void lit_args_ensurestring(LitVm *vm, LitValue value, const char *error);
void lit_args_ensurenumber(LitVm *vm, LitValue value, const char *error);
void lit_args_ensureobjtype(LitVm *vm, LitValue value, LitObjectType type, const char *error);
LitValue lit_table_getfield(LitState *state, LitTable *table, const char *name);
LitValue lit_map_getfield(LitState *state, LitMap *map, const char *name);
void lit_table_setfield(LitState *state, LitTable *table, const char *name, LitValue value);
void lit_map_setfield(LitState *state, LitMap *map, const char *name, LitValue value);
/* ccast.c */
void lit_exprlist_init(LitExprList *array);
void lit_exprlist_destroy(LitState *state, LitExprList *array);
void lit_exprlist_push(LitState *state, LitExprList *array, LitExpression *value);
void lit_stmtlist_init(LitExprList *array);
void lit_free_stmtlist_destroy(LitState *state, LitExprList *array);
void lit_stmtlist_push(LitState *state, LitExprList *array, LitExpression *value);
void lit_paramlist_init(LitParamList *array);
void lit_paramlist_destroy(LitState *state, LitParamList *array);
void lit_paramlist_push(LitState *state, LitParamList *array, LitParameter value);
void lit_ast_destroyexprlist(LitState *state, LitExprList *expressions);
void lit_ast_destroystmtlist(LitState *state, LitExprList *statements);
void lit_ast_destroyexpression(LitState *state, LitExpression *expression);
LitLiteralExpression *lit_ast_makeliteralexpr(LitState *state, LitUInt line, LitValue value);
LitBinaryExpression *lit_ast_makebinaryexpr(LitState *state, LitUInt line, LitExpression *left, LitExpression *right, LitTokenType op);
LitUnaryExpression *lit_ast_makeunaryexpr(LitState *state, LitUInt line, LitExpression *right, LitTokenType op);
LitVarExpression *lit_ast_makevarexpr(LitState *state, LitUInt line, const char *name, LitUInt length);
LitAssignExpression *lit_ast_makeassignexpr(LitState *state, LitUInt line, LitExpression *to, LitExpression *value);
LitCallExpression *lit_ast_makecallexpr(LitState *state, LitUInt line, LitExpression *callee);
LitGetExpression *lit_ast_makegetexpr(LitState *state, LitUInt line, LitExpression *where, const char *name, LitUInt length, bool questionable, bool ignore_result);
LitSetExpression *lit_ast_makesetexpr(LitState *state, LitUInt line, LitExpression *where, const char *name, LitUInt length, LitExpression *value);
LitFunctionStatement *lit_ast_makelambdaexpr(LitState *state, LitUInt line);
LitArrayExpression *lit_ast_makearrayexpr(LitState *state, LitUInt line);
LitObjectExpression *lit_ast_makeobjectexpr(LitState *state, LitUInt line);
LitSubscriptExpression *lit_ast_makesubscriptexpr(LitState *state, LitUInt line, LitExpression *array, LitExpression *index);
LitThisExpression *lit_ast_makethisexpr(LitState *state, LitUInt line);
LitSuperExpression *lit_ast_makesuperexpr(LitState *state, LitUInt line, LitString *method, bool ignore_result);
LitRangeExpression *lit_ast_makerangeexpr(LitState *state, LitUInt line, LitExpression *from, LitExpression *to);
LitTernaryExpression *lit_ast_maketernaryexpr(LitState *state, LitUInt line, LitExpression *condition, LitExpression *if_branch, LitExpression *else_branch);
LitInterpolationExpression *lit_ast_makeinterpolationexpr(LitState *state, LitUInt line);
LitReferenceExpression *lit_ast_makerefexpr(LitState *state, LitUInt line, LitExpression *to);
void lit_ast_destroystmt(LitState *state, LitExpression *statement);
LitExpressionStatement *lit_ast_makeexprstmt(LitState *state, LitUInt line, LitExpression *expression);
LitBlockStatement *lit_ast_makeblockstmt(LitState *state, LitUInt line);
LitVarStatement *lit_ast_makevardefstmt(LitState *state, LitUInt line, const char *name, LitUInt length, LitExpression *init, bool constant);
LitIfStatement *lit_ast_makeifstatement(LitState *state, LitUInt line, LitExpression *condition, LitExpression *if_branch, LitExpression *else_branch, LitExprList *elseif_conditions, LitExprList *elseif_branches);
LitWhileStatement *lit_ast_makewhilestmt(LitState *state, LitUInt line, LitExpression *condition, LitExpression *body);
LitForStatement *lit_ast_makeforstmt(LitState *state, LitUInt line, LitExpression *init, LitExpression *var, LitExpression *condition, LitExpression *increment, LitExpression *body, bool c_style);
LitContinueStatement *lit_ast_makecontinuestmt(LitState *state, LitUInt line);
LitBreakStatement *lit_ast_makebreakstmt(LitState *state, LitUInt line);
LitFunctionStatement *lit_ast_makefuncdefstmt(LitState *state, LitUInt line, const char *name, LitUInt length);
LitReturnStatement *lit_ast_makereturnstmt(LitState *state, LitUInt line, LitExpression *expression);
LitMethodStatement *lit_ast_makemethoddefstmt(LitState *state, LitUInt line, LitString *name, bool is_static);
LitClassStatement *lit_ast_makeclassdefstmt(LitState *state, LitUInt line, LitString *name, LitString *parent);
LitFieldStatement *lit_ast_makefieldstmt(LitState *state, LitUInt line, LitString *name, LitExpression *getter, LitExpression *setter, bool is_static);
LitExprList *lit_ast_allocexprlist(LitState *state);
void lit_ast_destroyallocatedexprlist(LitState *state, LitExprList *expressions);
LitExprList *lit_ast_allocstmtlist(LitState *state);
void lit_ast_destroyallocatedstmtlist(LitState *state, LitExprList *statements);
/* ccemit.c */
void lit_privlist_init(LitPrivList *array);
void lit_privlist_destroy(LitState *state, LitPrivList *array);
void lit_privlist_push(LitState *state, LitPrivList *array, LitPrivate value);
void lit_loclist_init(LitLocList *array);
void lit_loclist_destroy(LitState *state, LitLocList *array);
void lit_loclist_push(LitState *state, LitLocList *array, LitLocal value);
void lit_emitter_init(LitState *state, LitEmitter *emitter);
void lit_emitter_destroy(LitEmitter *emitter);
LitModule *lit_emitter_emitmod(LitEmitter *emitter, LitExprList *statements, LitString *modname);
/* cchunk.c */
void lit_chunk_init(LitChunk *chunk);
void lit_chunk_destroy(LitState *state, LitChunk *chunk);
void lit_chunk_push(LitState *state, LitChunk *chunk, uint64_t word, uint16_t line);
LitUInt lit_chunk_addconstant(LitState *state, LitChunk *chunk, LitValue constant);
LitUInt lit_chunk_getline(LitChunk *chunk, LitUInt offset);
void lit_chunk_shrink(LitState *state, LitChunk *chunk);
/* ccparser.c */
void lit_parser_init(LitState *state, LitParser *parser);
void lit_parser_destroy(LitParser *parser);
bool lit_parser_parsesource(LitParser *parser, const char *file_name, const char *source, LitExprList *statements);
/* ccscan.c */
void lit_scanner_init(LitState *state, LitScanner *scanner, const char *file_name, const char *source);
LitToken lit_scanner_scantoken(LitScanner *scanner);
/* debug.c */
void lit_debug_disasmodule(LitModule *module, const char *source);
void lit_debug_disaschunk(LitChunk *chunk, const char *name, const char *source);
void lit_debug_disasinstr(LitChunk *chunk, LitUInt offset, const char *source, bool forceline);
void lit_debug_traceframe(LitFiber *fiber);
/* error.c */
LitString *lit_state_errorfmtv(LitState *state, LitUInt line, const char* fmt, va_list args);
LitString *lit_state_errorfmt(LitState *state, LitUInt line, const char* fmt, ...);
/* event.c */
void lit_eventsystem_init(LitState *state, LitEventSystem *event_system);
void lit_eventsystem_destroy(LitEventSystem *event_system);
uint64_t lit_eventsystem_millis(void);
void lit_eventsystem_registerevent(LitState *state, LitValue callback, uint64_t time);
void lit_eventsystem_loop(LitState *state);
/* libarray.c */
void lit_uintlist_init(LitUIntList *array);
void lit_uintlist_destroy(LitState *state, LitUIntList *array);
void lit_uintlist_push(LitState *state, LitUIntList *array, LitUInt value);
void lit_bytelist_init(LitByteList *array);
void lit_bytelist_destroy(LitState *state, LitByteList *array);
void lit_bytelist_push(LitState *state, LitByteList *array, uint8_t value);
/* libcalls.c */
LitResult lit_state_callfunction(LitState *state, LitFunction *callee, LitValue *arguments, uint8_t argc);
LitResult lit_state_callclosure(LitState *state, LitClosure *callee, LitValue *arguments, uint8_t argc);
LitResult lit_state_callmethod(LitState *state, LitValue instance, LitValue callee, LitValue *arguments, uint8_t argc);
LitResult lit_state_callvalue(LitState *state, LitValue callee, LitValue *arguments, uint8_t argc);
LitResult lit_state_findandcallmethod(LitState *state, LitValue callee, LitString *mthname, LitValue *arguments, uint8_t argc);
LitString *lit_tostring_value(LitState *state, LitValue object, LitUInt indentation);
LitValue lit_state_callnew(LitVm *vm, const char *name, LitValue *args, LitUInt argc);
/* libcore.c */
void lit_state_openlibraries(LitState *state);
LitValue lit_coreutil_accessprivate(LitVm *vm, LitMap *map, LitString *name, LitValue *val);
void lit_state_opencorelibrary(LitState *state);
/* libfile.c */
void cleanup_file(LitState *state, LitUserdata *data, bool mark);
void lit_open_file_library(LitState *state);
/* libfs.c */
char *lit_read_file(const char *path);
bool lit_file_exists(const char *path);
bool lit_dir_exists(const char *path);
void lit_write_uint8_t(FILE *file, uint8_t byte);
void lit_write_uint16_t(FILE *file, uint16_t byte);
void lit_write_uint32_t(FILE *file, uint32_t byte);
void lit_write_uint64_t(FILE *file, uint64_t byte);
void lit_write_double(FILE *file, double byte);
void lit_write_string(FILE *file, LitString *string);
uint8_t lit_read_uint8_t(FILE *file);
uint16_t lit_read_uint16_t(FILE *file);
uint32_t lit_read_uint32_t(FILE *file);
double lit_read_double(FILE *file);
LitString *lit_read_string(LitState *state, FILE *file);
void lit_init_emulated_file(LitEmulatedFile *file, const char *source);
uint8_t lit_read_euint8_t(LitEmulatedFile *file);
uint16_t lit_read_euint16_t(LitEmulatedFile *file);
uint32_t lit_read_euint32_t(LitEmulatedFile *file);
uint64_t lit_read_euint64_t(LitEmulatedFile *file);
double lit_read_edouble(LitEmulatedFile *file);
LitString *lit_read_estring(LitState *state, LitEmulatedFile *file);
void lit_save_module(LitModule *module, FILE *file);
LitModule *lit_load_module(LitState *state, const char *input);
bool lit_generate_source_file(const char *file, const char *output);
/* libgc.c */
void lit_open_gc_library(LitState *state);
/* libmath.c */
int custom_random(LitUInt *data);
void lit_open_math_library(LitState *state);
/* libobject.c */
bool lit_value_iscallablefunction(LitValue value);
LitString *lit_object_makeemptystring(LitState *state, LitUInt length);
void lit_string_register(LitState *state, LitString *string);
uint32_t lit_string_hash(const char *key, LitUInt length);
LitString *lit_string_take(LitState *state, const char *chars, LitUInt length);
LitString *lit_string_copy(LitState *state, const char *chars, LitUInt length);
LitValue lit_string_numbertostring(LitState *state, double value);
LitValue lit_string_format(LitState *state, const char *format, ...);
LitObject *lit_object_allocobject(LitState *state, size_t size, LitObjectType type);
LitFunction *lit_object_makefunction(LitState *state, LitModule *module);
LitValue lit_function_getname(LitVm *vm, LitValue instance);
LitUpvalue *lit_object_makeupvalue(LitState *state, LitValue *slot);
LitClosure *lit_object_makeclosure(LitState *state, LitFunction *function);
LitClosurePrototype *lit_object_makeclosureproto(LitState *state, LitFunction *function);
LitNativeFunction *lit_object_makenativefunc(LitState *state, LitNativeFunctionFn function, LitString *name);
LitNativePrimitive *lit_object_makenativeprimitive(LitState *state, LitNativePrimitiveFn function, LitString *name);
LitNativeMethod *lit_object_makenativemethod(LitState *state, LitNativeMethodFn method, LitString *name);
LitPrimitiveMethod *lit_object_makeprimitivemethod(LitState *state, LitPrimitiveMethodFn method, LitString *name);
LitFiber *lit_object_makefiber(LitState *state, LitModule *module, LitFunction *function);
LitFiber *lit_object_makefiberclosure(LitState *state, LitModule *module, LitClosure *closure);
void lit_fiber_ensureregisters(LitState *state, LitFiber *fiber, LitUInt needed);
LitModule *lit_object_makemodule(LitState *state, LitString *name);
LitClass *lit_object_makeclass(LitState *state, LitString *name);
LitInstance *lit_object_makeinstance(LitState *state, LitClass *klass);
LitBoundMethod *lit_object_makeboundmethod(LitState *state, LitValue receiver, LitValue method);
LitArray *lit_object_makearray(LitState *state);
LitVarargArray *lit_object_makevararray(LitState *state);
LitMap *lit_object_makemap(LitState *state);
bool lit_map_set(LitState *state, LitMap *map, LitString *key, LitValue value);
bool lit_map_get(LitMap *map, LitString *key, LitValue *value);
bool lit_map_delete(LitMap *map, LitString *key);
void lit_map_add_all(LitState *state, LitMap *from, LitMap *to);
LitUserdata *lit_object_makeuserdata(LitState *state, size_t size);
LitRange *lit_object_makerange(LitState *state, double from, double to);
LitField *lit_object_makefield(LitState *state, LitObject *getter, LitObject *setter);
LitReference *lit_object_makereference(LitState *state, LitValue *slot);
/* libtable.c */
void lit_init_table(LitTable *table);
void lit_free_table(LitState *state, LitTable *table);
bool lit_table_set(LitState *state, LitTable *table, LitString *key, LitValue value);
bool lit_table_get(LitTable *table, LitString *key, LitValue *value);
bool lit_table_get_slot(LitTable *table, LitString *key, LitValue **value);
bool lit_table_delete(LitTable *table, LitString *key);
LitString *lit_table_find_string(LitTable *table, const char *chars, LitUInt length, uint32_t hash);
void lit_table_add_all(LitState *state, LitTable *from, LitTable *to);
void lit_table_add_all_ignoring(LitState *state, LitTable *from, LitTable *to);
void lit_table_remove_white(LitTable *table);
void lit_mark_table(LitVm *vm, LitTable *table);
/* mem.c */
void *lit_reallocate(LitState *state, void *pointer, size_t oldsize, size_t newsize);
void lit_free_object(LitState *state, LitObject *object);
void lit_free_objects(LitState *state, LitObject *objects);
void lit_mark_object(LitVm *vm, LitObject *object);
void lit_mark_value(LitVm *vm, LitValue value);
uint64_t lit_collect_garbage(LitVm *vm);
int lit_closest_power_of_two(int n);
/* state.c */
void lit_state_enablecompilationtimemeasurement(void);
LitState *lit_state_make(void);
int64_t lit_state_destroy(LitState *state);
void lit_state_pushroot(LitState *state, LitObject *object);
void lit_state_pushvalueroot(LitState *state, LitValue value);
LitValue lit_state_peekroot(LitState *state, uint8_t distance);
void lit_state_poproot(LitState *state);
void lit_state_poproots(LitState *state, uint8_t amount);
LitClass *lit_state_getclassfor(LitState *state, LitValue value);
LitResult lit_state_interpretsource(LitState *state, const char *modname, char *code);
LitModule *lit_state_compilemodulesource(LitState *state, LitString *modname, char *code);
LitModule *lit_state_getmodulebyname(LitState *state, const char *name);
LitResult lit_state_interninterpretsource(LitState *state, LitString *modname, char *code);
char *lit_util_patchfilename(char *file_name);
char *copy_string(const char *string);
bool lit_state_compileandsavefiles(LitState *state, char *files[], LitUInt numfiles, const char *outputfile);
LitResult lit_state_interpretfile(LitState *state, const char *file);
LitResult lit_state_dumpfile(LitState *state, const char *file);
void lit_state_raiseerror(LitState *state, LitErrorType type, const char *message, ...);
void lit_printf(LitState *state, const char *message, ...);
/* tostring.c */
const char *lit_tostring_typename(LitObjectType t);
/* utf.c */
int lit_decode_num_bytes(uint8_t byte);
int lit_ustring_length(LitString *string);
LitString *lit_ustring_code_point_at(LitState *state, LitString *string, uint32_t index);
LitString *lit_ustring_from_code_point(LitState *state, int value);
LitString *lit_ustring_from_range(LitState *state, LitString *source, int start, uint32_t count);
int lit_encode_num_bytes(int value);
int lit_ustring_encode(int value, uint8_t *bytes);
int lit_ustring_decode(const uint8_t *bytes, uint32_t length);
int lit_uchar_offset(char *str, int index);
/* value.c */
void lit_vallist_init(LitValList *array);
void lit_vallist_destroy(LitState *state, LitValList *array);
void lit_vallist_push(LitState *state, LitValList *array, LitValue value);
void lit_print_value(LitValue value);
void lit_values_ensure_size(LitState *state, LitValList *values, LitUInt size);
void lit_values_ensure_size_empty(LitState *state, LitValList *values, LitUInt size);
const char *lit_get_value_type(LitValue value);
/* vm.c */
void lit_init_vm(LitState *state, LitVm *vm);
void lit_free_vm(LitVm *vm);
bool lit_vm_handleerror(LitVm *vm, LitString *errorstring);
bool lit_vm_raiseerrorva(LitVm *vm, const char *format, va_list args);
bool lit_vm_raiseerror(LitVm *vm, const char *format, ...);
bool lit_vm_raisefatalerror(LitVm *vm, const char *format, ...);
LitResult lit_interpret_module(LitState *state, LitModule *module);
LitResult lit_interpret_fiber(LitState *state, register LitFiber *fiber);
void lit_native_exit_jump(void);

double lit_value_to_number(LitValue value);
LitValue lit_number_to_value(double num);
bool lit_is_falsey(LitValue value);

static inline bool lit_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool lit_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}


