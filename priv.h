
#pragma once

#include "lit.h"

#define PUSH(value) (*fiber->stack_top++ = value)

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
    #define OPCODE(name, a, b) OP_##name,
    #include "opcodes.inc"
    #undef OPCODE
};

typedef enum LitExpressionType LitExpressionType;
typedef enum LitInstructionType LitInstructionType;
typedef enum LitOpCode LitOpCode;


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
    LitExprList args;

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

struct LitParamList
{
    LitUInt capacity;
    LitUInt count;
    LitParameter* values;
};




struct LitArrayExpression
{
    LitExpression expression;
    LitExprList values;
};


struct LitObjectExpression
{
    LitExpression expression;

    LitValList keys;
    LitExprList values;
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
    LitExprList expressions;
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
    LitExprList statements;
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

    LitExprList* elseif_conditions;
    LitExprList* elseif_branches;
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

    LitParamList parameters;
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
    LitParamList parameters;
    LitExpression* body;

    bool is_static;
};


struct LitClassStatement
{
    LitExpression statement;

    LitString* name;
    LitString* parent;

    LitExprList fields;
};

struct LitFieldStatement
{
    LitExpression statement;

    LitString* name;
    LitExpression* getter;
    LitExpression* setter;

    bool is_static;
};
