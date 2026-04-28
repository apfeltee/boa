
#include "priv.h"

void lit_exprlist_init(LitExprList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_exprlist_destroy(LitState* state, LitExprList* array)
{
    LIT_FREE_ARRAY(state, LitExpression*, array->values, array->capacity);
    lit_exprlist_init(array);
}
void lit_exprlist_push(LitState* state, LitExprList* array, LitExpression* value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitExpression*, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_stmtlist_init(LitExprList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_free_stmtlist_destroy(LitState* state, LitExprList* array)
{
    LIT_FREE_ARRAY(state, LitExpression*, array->values, array->capacity);
    lit_stmtlist_init(array);
}
void lit_stmtlist_push(LitState* state, LitExprList* array, LitExpression* value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitExpression*, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_paramlist_init(LitParamList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_paramlist_destroy(LitState* state, LitParamList* array)
{
    LIT_FREE_ARRAY(state, LitParameter, array->values, array->capacity);
    lit_paramlist_init(array);
}
void lit_paramlist_push(LitState* state, LitParamList* array, LitParameter value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitParameter, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
};


#define FREE_EXPRESSION(type) lit_reallocate(state, expression, sizeof(type), 0);

static void lit_ast_destroyparamlist(LitState* state, LitParamList* parameters)
{
    for(LitUInt i = 0; i < parameters->count; i++)
    {
        lit_ast_destroyexpression(state, parameters->values[i].default_value);
    }

    lit_paramlist_destroy(state, parameters);
}

void lit_ast_destroyexprlist(LitState* state, LitExprList* expressions)
{
    if(expressions == NULL)
    {
        return;
    }

    for(LitUInt i = 0; i < expressions->count; i++)
    {
        lit_ast_destroyexpression(state, expressions->values[i]);
    }

    lit_exprlist_destroy(state, expressions);
}

void lit_ast_destroystmtlist(LitState* state, LitExprList* statements)
{
    if(statements == NULL)
    {
        return;
    }

    for(LitUInt i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }

    lit_free_stmtlist_destroy(state, statements);
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
            FREE_EXPRESSION(LitLiteralExpression)
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

            FREE_EXPRESSION(LitBinaryExpression)
            break;
        }

        case LIT_EXPR_UNARY:
        {
            lit_ast_destroyexpression(state, ((LitUnaryExpression*)expression)->right);
            FREE_EXPRESSION(LitUnaryExpression)

            break;
        }

        case LIT_EXPR_VAR:
        {
            FREE_EXPRESSION(LitVarExpression)
            break;
        }

        case LIT_EXPR_ASSIGN:
        {
            LitAssignExpression* expr = (LitAssignExpression*)expression;

            lit_ast_destroyexpression(state, expr->to);
            lit_ast_destroyexpression(state, expr->value);

            FREE_EXPRESSION(LitAssignExpression)
            break;
        }

        case LIT_EXPR_CALL:
        {
            LitCallExpression* expr = (LitCallExpression*)expression;

            lit_ast_destroyexpression(state, expr->callee);
            lit_ast_destroyexpression(state, expr->init);

            lit_ast_destroyexprlist(state, &expr->args);

            FREE_EXPRESSION(LitCallExpression)
            break;
        }

        case LIT_EXPR_GET:
        {
            lit_ast_destroyexpression(state, ((LitGetExpression*)expression)->where);
            FREE_EXPRESSION(LitGetExpression)
            break;
        }

        case LIT_EXPR_SET:
        {
            LitSetExpression* expr = (LitSetExpression*)expression;

            lit_ast_destroyexpression(state, expr->where);
            lit_ast_destroyexpression(state, expr->value);

            FREE_EXPRESSION(LitSetExpression)
            break;
        }

        case LIT_EXPR_LAMBDA:
        {
            LitFunctionStatement* expr = (LitFunctionStatement*)expression;

            lit_ast_destroyparamlist(state, &expr->parameters);
            lit_ast_destroystmt(state, expr->body);

            FREE_EXPRESSION(LitFunctionStatement)
            break;
        }

        case LIT_EXPR_ARRAY:
        {
            lit_ast_destroyexprlist(state, &((LitArrayExpression*)expression)->values);
            FREE_EXPRESSION(LitArrayExpression)

            break;
        }

        case LIT_EXPR_OBJECT:
        {
            LitObjectExpression* map = (LitObjectExpression*)expression;

            lit_vallist_destroy(state, &map->keys);
            lit_ast_destroyexprlist(state, &map->values);

            FREE_EXPRESSION(LitObjectExpression)
            break;
        }

        case LIT_EXPR_SUBSCRIPT:
        {
            LitSubscriptExpression* expr = (LitSubscriptExpression*)expression;

            lit_ast_destroyexpression(state, expr->array);
            lit_ast_destroyexpression(state, expr->index);

            FREE_EXPRESSION(LitSubscriptExpression)
            break;
        }

        case LIT_EXPR_THIS:
        {
            FREE_EXPRESSION(LitThisExpression)
            break;
        }

        case LIT_EXPR_SUPER:
        {
            FREE_EXPRESSION(LitSuperExpression)
            break;
        }

        case LIT_EXPR_RANGE:
        {
            LitRangeExpression* expr = (LitRangeExpression*)expression;

            lit_ast_destroyexpression(state, expr->from);
            lit_ast_destroyexpression(state, expr->to);

            FREE_EXPRESSION(LitRangeExpression)
            break;
        }

        case LIT_EXPR_TERNARY:
        {
            LitTernaryExpression* expr = (LitTernaryExpression*)expression;

            lit_ast_destroyexpression(state, expr->condition);
            lit_ast_destroyexpression(state, expr->if_branch);
            lit_ast_destroyexpression(state, expr->else_branch);

            FREE_EXPRESSION(LitTernaryExpression)
            break;
        }

        case LIT_EXPR_INTERPOLATION:
        {
            lit_ast_destroyexprlist(state, &((LitInterpolationExpression*)expression)->expressions);
            FREE_EXPRESSION(LitInterpolationExpression)

            break;
        }

        case LIT_EXPR_REFERENCE:
        {
            lit_ast_destroyexpression(state, ((LitReferenceExpression*)expression)->to);
            FREE_EXPRESSION(LitReferenceExpression)

            break;
        }

        default:
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Unknown expression type %d", (int)expression->type);
            break;
        }
    }
}

#define ALLOCATE_EXPRESSION(state, type, objecttype) (type*)lit_ast_allocexpr(state, line, sizeof(type), objecttype)

static LitExpression* lit_ast_allocexpr(LitState* state, uint64_t line, size_t size, LitExpressionType type)
{
    LitExpression* object = (LitExpression*)lit_reallocate(state, NULL, 0, size);

    object->type = type;
    object->line = line;

    return object;
}

LitLiteralExpression* lit_ast_makeliteralexpr(LitState* state, LitUInt line, LitValue value)
{
    LitLiteralExpression* expression = ALLOCATE_EXPRESSION(state, LitLiteralExpression, LIT_EXPR_LITERAL);
    expression->value = value;
    return expression;
}

LitBinaryExpression* lit_ast_makebinaryexpr(LitState* state, LitUInt line, LitExpression* left, LitExpression* right, LitTokenType op)
{
    LitBinaryExpression* expression = ALLOCATE_EXPRESSION(state, LitBinaryExpression, LIT_EXPR_BINARY);

    expression->left = left;
    expression->right = right;
    expression->op = op;
    expression->ignore_left = false;

    return expression;
}

LitUnaryExpression* lit_ast_makeunaryexpr(LitState* state, LitUInt line, LitExpression* right, LitTokenType op)
{
    LitUnaryExpression* expression = ALLOCATE_EXPRESSION(state, LitUnaryExpression, LIT_EXPR_UNARY);

    expression->right = right;
    expression->op = op;

    return expression;
}

LitVarExpression* lit_ast_makevarexpr(LitState* state, LitUInt line, const char* name, LitUInt length)
{
    LitVarExpression* expression = ALLOCATE_EXPRESSION(state, LitVarExpression, LIT_EXPR_VAR);

    expression->name = name;
    expression->length = length;

    return expression;
}

LitAssignExpression* lit_ast_makeassignexpr(LitState* state, LitUInt line, LitExpression* to, LitExpression* value)
{
    LitAssignExpression* expression = ALLOCATE_EXPRESSION(state, LitAssignExpression, LIT_EXPR_ASSIGN);

    expression->to = to;
    expression->value = value;

    return expression;
}

LitCallExpression* lit_ast_makecallexpr(LitState* state, LitUInt line, LitExpression* callee)
{
    LitCallExpression* expression = ALLOCATE_EXPRESSION(state, LitCallExpression, LIT_EXPR_CALL);

    expression->callee = callee;
    expression->init = NULL;

    lit_exprlist_init(&expression->args);

    return expression;
}

LitGetExpression* lit_ast_makegetexpr(LitState* state, LitUInt line, LitExpression* where, const char* name, LitUInt length, bool questionable, bool ignore_result)
{
    LitGetExpression* expression = ALLOCATE_EXPRESSION(state, LitGetExpression, LIT_EXPR_GET);

    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->ignore_emit = false;
    expression->jump = questionable ? 0 : -1;
    expression->ignore_result = ignore_result;

    return expression;
}

LitSetExpression* lit_ast_makesetexpr(LitState* state, LitUInt line, LitExpression* where, const char* name, LitUInt length, LitExpression* value)
{
    LitSetExpression* expression = ALLOCATE_EXPRESSION(state, LitSetExpression, LIT_EXPR_SET);

    expression->where = where;
    expression->name = name;
    expression->length = length;
    expression->value = value;

    return expression;
}

LitFunctionStatement* lit_ast_makelambdaexpr(LitState* state, LitUInt line)
{
    LitFunctionStatement* expression = ALLOCATE_EXPRESSION(state, LitFunctionStatement, LIT_EXPR_LAMBDA);

    expression->body = NULL;
    lit_paramlist_init(&expression->parameters);

    return expression;
}

LitArrayExpression* lit_ast_makearrayexpr(LitState* state, LitUInt line)
{
    LitArrayExpression* expression = ALLOCATE_EXPRESSION(state, LitArrayExpression, LIT_EXPR_ARRAY);
    lit_exprlist_init(&expression->values);
    return expression;
}

LitObjectExpression* lit_ast_makeobjectexpr(LitState* state, LitUInt line)
{
    LitObjectExpression* expression = ALLOCATE_EXPRESSION(state, LitObjectExpression, LIT_EXPR_OBJECT);

    lit_vallist_init(&expression->keys);
    lit_exprlist_init(&expression->values);

    return expression;
}

LitSubscriptExpression* lit_ast_makesubscriptexpr(LitState* state, LitUInt line, LitExpression* array, LitExpression* index)
{
    LitSubscriptExpression* expression = ALLOCATE_EXPRESSION(state, LitSubscriptExpression, LIT_EXPR_SUBSCRIPT);

    expression->array = array;
    expression->index = index;

    return expression;
}

LitThisExpression* lit_ast_makethisexpr(LitState* state, LitUInt line)
{
    return ALLOCATE_EXPRESSION(state, LitThisExpression, LIT_EXPR_THIS);
}

LitSuperExpression* lit_ast_makesuperexpr(LitState* state, LitUInt line, LitString* method, bool ignore_result)
{
    LitSuperExpression* expression = ALLOCATE_EXPRESSION(state, LitSuperExpression, LIT_EXPR_SUPER);

    expression->method = method;
    expression->ignore_emit = false;
    expression->ignore_result = ignore_result;

    return expression;
}

LitRangeExpression* lit_ast_makerangeexpr(LitState* state, LitUInt line, LitExpression* from, LitExpression* to)
{
    LitRangeExpression* expression = ALLOCATE_EXPRESSION(state, LitRangeExpression, LIT_EXPR_RANGE);

    expression->from = from;
    expression->to = to;

    return expression;
}

LitTernaryExpression* lit_ast_maketernaryexpr(LitState* state, LitUInt line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch)
{
    LitTernaryExpression* expression = ALLOCATE_EXPRESSION(state, LitTernaryExpression, LIT_EXPR_TERNARY);

    expression->condition = condition;
    expression->if_branch = if_branch;
    expression->else_branch = else_branch;

    return expression;
}

LitInterpolationExpression* lit_ast_makeinterpolationexpr(LitState* state, LitUInt line)
{
    LitInterpolationExpression* expression = ALLOCATE_EXPRESSION(state, LitInterpolationExpression, LIT_EXPR_INTERPOLATION);
    lit_exprlist_init(&expression->expressions);
    return expression;
}

LitReferenceExpression* lit_ast_makerefexpr(LitState* state, LitUInt line, LitExpression* to)
{
    LitReferenceExpression* expression = ALLOCATE_EXPRESSION(state, LitReferenceExpression, LIT_EXPR_REFERENCE);
    expression->to = to;
    return expression;
}

#define FREE_STATEMENT(type) lit_reallocate(state, statement, sizeof(type), 0);

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
            FREE_STATEMENT(LitExpressionStatement)
            break;
        }

        case LIT_EXPR_BLOCK:
        {
            lit_ast_destroystmtlist(state, &((LitBlockStatement*)statement)->statements);
            FREE_STATEMENT(LitBlockStatement)
            break;
        }

        case LIT_EXPR_VARDECL:
        {
            lit_ast_destroyexpression(state, ((LitVarStatement*)statement)->init);
            FREE_STATEMENT(LitVarStatement)
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

            FREE_STATEMENT(LitIfStatement)
            break;
        }

        case LIT_EXPR_WHILE:
        {
            LitWhileStatement* stmt = (LitWhileStatement*)statement;

            lit_ast_destroyexpression(state, stmt->condition);
            lit_ast_destroystmt(state, stmt->body);

            FREE_STATEMENT(LitWhileStatement)
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

            FREE_STATEMENT(LitForStatement)
            break;
        }

        case LIT_EXPR_CONTINUE:
        {
            FREE_STATEMENT(LitContinueStatement)
            break;
        }

        case LIT_EXPR_BREAK:
        {
            FREE_STATEMENT(LitBreakStatement)
            break;
        }

        case LIT_EXPR_FUNCTION:
        {
            LitFunctionStatement* stmt = (LitFunctionStatement*)statement;

            lit_ast_destroystmt(state, stmt->body);
            lit_ast_destroyparamlist(state, &stmt->parameters);

            FREE_STATEMENT(LitFunctionStatement)
            break;
        }

        case LIT_EXPR_RETURN:
        {
            lit_ast_destroyexpression(state, ((LitReturnStatement*)statement)->expression);
            FREE_STATEMENT(LitReturnStatement)

            break;
        }

        case LIT_EXPR_METHOD:
        {
            LitMethodStatement* stmt = (LitMethodStatement*)statement;

            lit_ast_destroyparamlist(state, &stmt->parameters);
            lit_ast_destroystmt(state, stmt->body);

            FREE_STATEMENT(LitMethodStatement)

            break;
        }

        case LIT_EXPR_CLASS:
        {
            lit_ast_destroystmtlist(state, &((LitClassStatement*)statement)->fields);
            FREE_STATEMENT(LitClassStatement)
            break;
        }

        case LIT_EXPR_FIELD:
        {
            LitFieldStatement* stmt = (LitFieldStatement*)statement;

            lit_ast_destroystmt(state, stmt->getter);
            lit_ast_destroystmt(state, stmt->setter);

            FREE_STATEMENT(LitFieldStatement)
            break;
        }

        default:
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Unknown statement type %d", (int)statement->type);
            break;
        }
    }
}

#define ALLOCATE_STATEMENT(state, type, objecttype) (type*)lit_ast_allocstmt(state, line, sizeof(type), objecttype)

static LitExpression* lit_ast_allocstmt(LitState* state, uint64_t line, size_t size, LitExpressionType type)
{
    LitExpression* object = (LitExpression*)lit_reallocate(state, NULL, 0, size);

    object->type = type;
    object->line = line;

    return object;
}

LitExpressionStatement* lit_ast_makeexprstmt(LitState* state, LitUInt line, LitExpression* expression)
{
    LitExpressionStatement* statement = ALLOCATE_STATEMENT(state, LitExpressionStatement, LIT_EXPR_EXPRESSION);
    statement->expression = expression;

    return statement;
}

LitBlockStatement* lit_ast_makeblockstmt(LitState* state, LitUInt line)
{
    LitBlockStatement* statement = ALLOCATE_STATEMENT(state, LitBlockStatement, LIT_EXPR_BLOCK);
    lit_stmtlist_init(&statement->statements);
    return statement;
}

LitVarStatement* lit_ast_makevardefstmt(LitState* state, LitUInt line, const char* name, LitUInt length, LitExpression* init, bool constant)
{
    LitVarStatement* statement = ALLOCATE_STATEMENT(state, LitVarStatement, LIT_EXPR_VARDECL);

    statement->name = name;
    statement->length = length;
    statement->init = init;
    statement->constant = constant;

    return statement;
}

LitIfStatement*
lit_ast_makeifstatement(LitState* state, LitUInt line, LitExpression* condition, LitExpression* if_branch, LitExpression* else_branch, LitExprList* elseif_conditions, LitExprList* elseif_branches)
{
    LitIfStatement* statement = ALLOCATE_STATEMENT(state, LitIfStatement, LIT_EXPR_IF);

    statement->condition = condition;
    statement->if_branch = if_branch;
    statement->else_branch = else_branch;
    statement->elseif_conditions = elseif_conditions;
    statement->elseif_branches = elseif_branches;

    return statement;
}

LitWhileStatement* lit_ast_makewhilestmt(LitState* state, LitUInt line, LitExpression* condition, LitExpression* body)
{
    LitWhileStatement* statement = ALLOCATE_STATEMENT(state, LitWhileStatement, LIT_EXPR_WHILE);

    statement->condition = condition;
    statement->body = body;

    return statement;
}

LitForStatement*
lit_ast_makeforstmt(LitState* state, LitUInt line, LitExpression* init, LitExpression* var, LitExpression* condition, LitExpression* increment, LitExpression* body, bool c_style)
{
    LitForStatement* statement = ALLOCATE_STATEMENT(state, LitForStatement, LIT_EXPR_FOR);

    statement->init = init;
    statement->var = var;
    statement->condition = condition;
    statement->increment = increment;
    statement->body = body;
    statement->c_style = c_style;

    return statement;
}

LitContinueStatement* lit_ast_makecontinuestmt(LitState* state, LitUInt line)
{
    return ALLOCATE_STATEMENT(state, LitContinueStatement, LIT_EXPR_CONTINUE);
}

LitBreakStatement* lit_ast_makebreakstmt(LitState* state, LitUInt line)
{
    return ALLOCATE_STATEMENT(state, LitBreakStatement, LIT_EXPR_BREAK);
}

LitFunctionStatement* lit_ast_makefuncdefstmt(LitState* state, LitUInt line, const char* name, LitUInt length)
{
    LitFunctionStatement* function = ALLOCATE_STATEMENT(state, LitFunctionStatement, LIT_EXPR_FUNCTION);

    function->name = name;
    function->length = length;
    function->body = NULL;

    lit_paramlist_init(&function->parameters);

    return function;
}

LitReturnStatement* lit_ast_makereturnstmt(LitState* state, LitUInt line, LitExpression* expression)
{
    LitReturnStatement* statement = ALLOCATE_STATEMENT(state, LitReturnStatement, LIT_EXPR_RETURN);
    statement->expression = expression;

    return statement;
}

LitMethodStatement* lit_ast_makemethoddefstmt(LitState* state, LitUInt line, LitString* name, bool is_static)
{
    LitMethodStatement* statement = ALLOCATE_STATEMENT(state, LitMethodStatement, LIT_EXPR_METHOD);

    statement->name = name;
    statement->body = NULL;
    statement->is_static = is_static;

    lit_paramlist_init(&statement->parameters);

    return statement;
}

LitClassStatement* lit_ast_makeclassdefstmt(LitState* state, LitUInt line, LitString* name, LitString* parent)
{
    LitClassStatement* statement = ALLOCATE_STATEMENT(state, LitClassStatement, LIT_EXPR_CLASS);

    statement->name = name;
    statement->parent = parent;

    lit_stmtlist_init(&statement->fields);

    return statement;
}

LitFieldStatement* lit_ast_makefieldstmt(LitState* state, LitUInt line, LitString* name, LitExpression* getter, LitExpression* setter, bool is_static)
{
    LitFieldStatement* statement = ALLOCATE_STATEMENT(state, LitFieldStatement, LIT_EXPR_FIELD);

    statement->name = name;
    statement->getter = getter;
    statement->setter = setter;
    statement->is_static = is_static;

    return statement;
}

LitExprList* lit_ast_allocexprlist(LitState* state)
{
    LitExprList* expressions = (LitExprList*)lit_reallocate(state, NULL, 0, sizeof(LitExprList));
    lit_exprlist_init(expressions);
    return expressions;
}

void lit_ast_destroyallocatedexprlist(LitState* state, LitExprList* expressions)
{
    if(expressions == NULL)
    {
        return;
    }

    for(LitUInt i = 0; i < expressions->count; i++)
    {
        lit_ast_destroyexpression(state, expressions->values[i]);
    }

    lit_exprlist_destroy(state, expressions);
    lit_reallocate(state, expressions, sizeof(LitExprList), 0);
}

LitExprList* lit_ast_allocstmtlist(LitState* state)
{
    LitExprList* statements = (LitExprList*)lit_reallocate(state, NULL, 0, sizeof(LitExprList));
    lit_stmtlist_init(statements);
    return statements;
}

void lit_ast_destroyallocatedstmtlist(LitState* state, LitExprList* statements)
{
    if(statements == NULL)
    {
        return;
    }

    for(LitUInt i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }

    lit_free_stmtlist_destroy(state, statements);
    lit_reallocate(state, statements, sizeof(LitExprList), 0);
}