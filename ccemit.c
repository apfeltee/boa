
#include <math.h>
#include <string.h>
#include "priv.h"

void lit_privlist_init(LitPrivList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_privlist_destroy(LitState* state, LitPrivList* array)
{
    LIT_FREE_ARRAY(state, LitPrivate, array->values, array->capacity);
    lit_privlist_init(array);
}
void lit_privlist_push(LitState* state, LitPrivList* array, LitPrivate value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitPrivate, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}


void lit_loclist_init(LitLocList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_loclist_destroy(LitState* state, LitLocList* array)
{
    LIT_FREE_ARRAY(state, LitLocal, array->values, array->capacity);
    lit_loclist_init(array);
}
void lit_loclist_push(LitState* state, LitLocList* array, LitLocal value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitLocal, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

static void lit_emitter_emitexpr(LitEmitter* emitter, LitExpression* expression, uint8_t reg);
static bool lit_emitter_emitstmt(LitEmitter* emitter, LitExpression* statement);
static void resolve_statement(LitEmitter* emitter, LitExpression* statement);

static void lit_emitter_resolvestatements(LitEmitter* emitter, LitExprList* statements)
{
    for(LitUInt i = 0; i < statements->count; i++)
    {
        resolve_statement(emitter, statements->values[i]);
    }
}

void lit_emitter_init(LitState* state, LitEmitter* emitter)
{
    emitter->state = state;
    emitter->loop_start = 0;
    emitter->emit_reference = 0;
    emitter->class_name = NULL;
    emitter->compiler = NULL;
    emitter->chunk = NULL;
    emitter->module = NULL;
    emitter->class_has_super = false;

    lit_privlist_init(&emitter->privates);
    lit_uintlist_init(&emitter->breaks);
    lit_uintlist_init(&emitter->continues);
}

void lit_emitter_destroy(LitEmitter* emitter)
{
    lit_uintlist_destroy(emitter->state, &emitter->breaks);
    lit_uintlist_destroy(emitter->state, &emitter->continues);
}

static void lit_emitter_raiseerror(LitEmitter* emitter, LitUInt line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_state_raiseerror(emitter->state, COMPILE_ERROR, lit_state_errorfmtv(emitter->state, line, fmt, args)->chars);
    va_end(args);
}

static LitUInt lit_emitter_emittmp(LitEmitter* emitter)
{
    lit_chunk_push(emitter->state, emitter->chunk, 0, emitter->last_line);
    return emitter->chunk->count - 1;
}

static void lit_emitter_patchinstr(LitEmitter* emitter, uint64_t position, uint64_t instruction)
{
    emitter->chunk->code[position] = instruction;
}

static void lit_emitter_emitabc(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, uint16_t b, uint16_t c)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->state, emitter->chunk, LIT_FORM_ABC_INSTRUCTION(opcode, a, b, c), emitter->last_line);
}

static void lit_emitter_emitabx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, uint32_t bx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->state, emitter->chunk, LIT_FORM_ABX_INSTRUCTION(opcode, a, bx), emitter->last_line);
}

static void lit_emitter_emitasbx(LitEmitter* emitter, uint16_t line, uint8_t opcode, uint8_t a, int32_t sbx)
{
    emitter->last_line = fmax(line, emitter->last_line);
    lit_chunk_push(emitter->state, emitter->chunk, LIT_FORM_ASBX_INSTRUCTION(opcode, a, sbx), emitter->last_line);
}

static void lit_emitter_dumpusedregisters(LitEmitter* emitter)
{
    printf("[ ");
    for(LitUInt i = 0; i < emitter->compiler->registers_used; i++)
    {
        printf("%i ", i);
    }

    printf("]\n");
}

// Be very careful with the use of this function, always reserve a register just before using it, do not wait around!
static uint8_t lit_emitter_reserveregister(LitEmitter* emitter)
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

static void lit_emitter_freeregister(LitEmitter* emitter, uint16_t reg)
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

static void lit_emitter_compilerinit(LitEmitter* emitter, LitCompiler* compiler, LitFunctionType type)
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
        lit_loclist_push(emitter->state, &compiler->locals, (LitLocal){ "this", 4, -1, false, false, lit_emitter_reserveregister(emitter) });
    }
    else
    {
        lit_loclist_push(emitter->state, &compiler->locals, (LitLocal){ "", 0, -1, false, false, lit_emitter_reserveregister(emitter) });
    }
}

static LitFunction* lit_emitter_compilerend(LitEmitter* emitter, LitString* name)
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

static void lit_emitter_scopebegin(LitEmitter* emitter)
{
    emitter->compiler->scope_depth++;
}

static void lit_emitter_scopeend(LitEmitter* emitter)
{
    if(emitter->compiler->scope_depth == -1)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "Invalid scope ending");
    }

    emitter->compiler->scope_depth--;

    LitCompiler* compiler = emitter->compiler;
    LitLocList* locals = &compiler->locals;

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

static uint16_t lit_emitter_addconst(LitEmitter* emitter, LitUInt line, LitValue value)
{
    LitUInt constant = lit_chunk_addconstant(emitter->state, emitter->chunk, value);

    if(constant >= UINT16_MAX)
    {
        lit_emitter_raiseerror(emitter, line, "Too many constants for one chunk");
    }

    return constant;
}

static int lit_emitter_addprivate(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line, bool constant)
{
    LitPrivList* privates = &emitter->privates;

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

        return AS_NUMBER(index);
    }

    LitState* state = emitter->state;
    int index = (int)privates->count;

    lit_privlist_push(state, privates, (LitPrivate){ false, constant });

    lit_table_set(state, private_names, lit_string_copy(state, name, length), NUMBER_VALUE(index));
    emitter->module->private_count++;

    return index;
}

static int lit_emitter_resolveprivate(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line)
{
    LitTable* private_names = &emitter->module->private_names->values;
    LitString* key = lit_table_find_string(private_names, name, length, lit_string_hash(name, length));

    if(key != NULL)
    {
        LitValue index;
        lit_table_get(private_names, key, &index);

        int numberindex = AS_NUMBER(index);

        if(!emitter->privates.values[numberindex].initialized)
        {
            lit_emitter_raiseerror(emitter, line, "Variable '%.*s' can't use itself in its initializer", length, name);
        }

        return numberindex;
    }

    return -1;
}

static int lit_emitter_addlocal(LitEmitter* emitter, const char* name, LitUInt length, LitUInt line, bool constant, uint8_t reg)
{
    LitCompiler* compiler = emitter->compiler;
    LitLocList* locals = &compiler->locals;

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

    lit_loclist_push(emitter->state, locals, (LitLocal){ name, length, UINT16_MAX, false, constant, reg });

    return (int)locals->count - 1;
}

static int lit_emitter_resolvelocal(LitEmitter* emitter, LitCompiler* compiler, const char* name, LitUInt length, LitUInt line)
{
    LitLocList* locals = &compiler->locals;

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

static int lit_emitter_addupvalue(LitEmitter* emitter, LitCompiler* compiler, uint8_t index, LitUInt line, bool islocal)
{
    LitUInt upvalue_count = compiler->function->upvalue_count;

    for(LitUInt i = 0; i < upvalue_count; i++)
    {
        LitCompilerUpvalue* upvalue = &compiler->upvalues[i];

        if(upvalue->index == index && upvalue->isLocal == islocal)
        {
            return i;
        }
    }

    if(upvalue_count == UINT16_COUNT)
    {
        lit_emitter_raiseerror(emitter, line, "Too many upvalues for one function");
        return 0;
    }

    compiler->upvalues[upvalue_count].isLocal = islocal;
    compiler->upvalues[upvalue_count].index = index;

    return compiler->function->upvalue_count++;
}

static int lit_emitter_resolveupvalue(LitEmitter* emitter, LitCompiler* compiler, const char* name, LitUInt length, LitUInt line)
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

static void lit_emitter_marklocalinit(LitEmitter* emitter, LitUInt index)
{
    emitter->compiler->locals.values[index].depth = emitter->compiler->scope_depth;
}

static void lit_emitter_markprivateinit(LitEmitter* emitter, LitUInt index)
{
    emitter->privates.values[index].initialized = true;
}

static void resolve_statement(LitEmitter* emitter, LitExpression* statement)
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

static LitOpCode lit_emitter_translateunaryop(LitTokenType token)
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

static LitOpCode lit_emitter_translatebinaryop(LitTokenType token)
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

static uint16_t lit_emitter_parsearg(LitEmitter* emitter, LitExpression* expression, uint8_t reg)
{
    if(expression->type == LIT_EXPR_LITERAL)
    {
        LitValue value = ((LitLiteralExpression*)expression)->value;

        if(IS_NUMBER(value) || IS_STRING(value))
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

static void lit_emitter_emitbinaryexpr(LitEmitter* emitter, LitBinaryExpression* expr, uint8_t reg, bool swap)
{
    LitTokenType op = expr->op;

    if(op == LTOKEN_AMPERSAND_AMPERSAND || op == LTOKEN_BAR_BAR || op == LTOKEN_QUESTION_QUESTION)
    {
        lit_emitter_emitexpr(emitter, expr->left, reg);
        LitUInt jump = lit_emitter_emittmp(emitter);
        lit_emitter_emitexpr(emitter, expr->right, reg);

        lit_emitter_patchinstr(emitter, jump,
                          LIT_FORM_ABX_INSTRUCTION(op == LTOKEN_BAR_BAR ? OP_TRUE_JUMP : (op == LTOKEN_QUESTION_QUESTION ? OP_NON_NULL_JUMP : OP_FALSE_JUMP),
                                                   reg, emitter->chunk->count - jump - 1));
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

            int constant = lit_emitter_addconst(emitter, expr->expression.line, OBJECT_VALUE(lit_string_copy(emitter->state, e->name, e->length)));
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

static bool lit_emitter_emitparams(LitEmitter* emitter, LitParamList* parameters, LitUInt line)
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
            lit_emitter_patchinstr(emitter, jump, LIT_FORM_ABX_INSTRUCTION(OP_NON_NULL_JUMP, reg, (int64_t)emitter->chunk->count - jump - 1));
        }
    }

    return false;
}

static void lit_emitter_emitexprfull(LitEmitter* emitter, LitExpression* expression, uint8_t reg, bool ignored);

static void lit_emitter_emitexprignoringregister(LitEmitter* emitter, LitExpression* expression)
{
    uint8_t reg = lit_emitter_reserveregister(emitter);

    lit_emitter_emitexprfull(emitter, expression, reg, true);
    lit_emitter_freeregister(emitter, reg);
}

static void lit_emitter_emitexpr(LitEmitter* emitter, LitExpression* expression, uint8_t reg)
{
    lit_emitter_emitexprfull(emitter, expression, reg, false);
}

static void lit_emitter_emitexprfull(LitEmitter* emitter, LitExpression* expression, uint8_t reg, bool ignored)
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

            if(IS_NULL(value))
            {
                lit_emitter_emitabc(emitter, expression->line, OP_LOAD_NULL, reg, 0, 0);
            }
            else if(IS_BOOL(value))
            {
                lit_emitter_emitabc(emitter, expression->line, OP_LOAD_BOOL, reg, (uint8_t)AS_BOOL(value), 0);
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
                        uint16_t constant = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(lit_string_copy(emitter->state, expr->name, expr->length)));

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
                            uint16_t constant = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(lit_string_copy(emitter->state, e->name, e->length)));
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

                int constant = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(lit_string_copy(emitter->state, e->name, e->length)));

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
            LitUInt arg_count = expr->args.count;
            uint16_t argregs[arg_count];

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

            for(LitUInt i = 0; i < arg_count; i++)
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

                int constant = lit_emitter_addconst(emitter, emitter->last_line, OBJECT_VALUE(lit_string_copy(emitter->state, e->name, e->length)));
                lit_emitter_emitabc(emitter, expression->line, OP_INVOKE, reg, arg_count + 1, constant);
            }
            else if(super)
            {
                assert(tmpreg == reg + 1);

                LitSuperExpression* e = (LitSuperExpression*)expr->callee;
                uint8_t index = lit_emitter_resolveupvalue(emitter, emitter->compiler, "super", 5, emitter->last_line);

                lit_emitter_emitabx(emitter, expression->line, OP_GET_UPVALUE, tmpreg, index);
                lit_emitter_emitabc(emitter, expression->line, OP_MOVE, reg, 0, 0);
                lit_emitter_emitabc(emitter, emitter->last_line, OP_INVOKE_SUPER, reg, arg_count + 1, lit_emitter_addconst(emitter, emitter->last_line, OBJECT_VALUE(e->method)));

                lit_emitter_freeregister(emitter, tmpreg);
            }
            else
            {
                lit_emitter_emitabc(emitter, expression->line, OP_CALL, reg, arg_count + 1, 1);
            }

            for(LitUInt i = 0; i < arg_count; i++)
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
                            lit_emitter_patchinstr(emitter, getter->jump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, reg, (int64_t)emitter->chunk->count - getter->jump - 1));
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
                    int constant = lit_emitter_addconst(emitter, emitter->last_line, OBJECT_VALUE(lit_string_copy(emitter->state, expr->name, expr->length)));

                    if(ref)
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_REFERENCE_FIELD, reg, reg, constant);
                    }
                    else
                    {
                        lit_emitter_emitabc(emitter, expression->line, OP_GET_FIELD, reg, reg, constant);
                    }
                }

                lit_emitter_patchinstr(emitter, expr->jump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, reg, (int64_t)emitter->chunk->count - expr->jump - 1));
            }
            else if(emit)
            {
                int constant = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(lit_string_copy(emitter->state, expr->name, expr->length)));

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

            int constant = lit_emitter_addconst(emitter, emitter->last_line, OBJECT_VALUE(lit_string_copy(emitter->state, expr->name, expr->length)));

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
            = AS_STRING(lit_string_format(emitter->state, "lambda @:@", OBJECT_VALUE(emitter->module->name), lit_string_numbertostring(emitter->state, expression->line)));

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

            function->arg_count = expr->parameters.count;
            function->max_registers += function->arg_count;
            function->vararg = vararg;

            uint16_t functionreg;
            bool closure = function->upvalue_count > 0;

            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);

                for(LitUInt i = 0; i < function->upvalue_count; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];

                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }

                uint16_t constidx = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(clsproto));
                lit_emitter_emitabx(emitter, expression->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(function));
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

            int64_t start = emitter->chunk->count;
            lit_emitter_emitexpr(emitter, expr->if_branch, reg);

            LitUInt elseskip = lit_emitter_emittmp(emitter);

            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->count - start));

            int64_t elsestart = emitter->chunk->count;
            lit_emitter_emitexpr(emitter, expr->else_branch, reg);
            lit_emitter_patchinstr(emitter, elseskip, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->count - elsestart));

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
                lit_emitter_emitabc(emitter, expression->line, OP_GET_SUPER_METHOD, reg, tmpreg, lit_emitter_addconst(emitter, expression->line, OBJECT_VALUE(expr->method)));

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

static void lit_emitter_patchloopjumps(LitEmitter* emitter, LitUIntList* breaks)
{
    for(LitUInt i = 0; i < breaks->count; i++)
    {
        lit_emitter_patchinstr(emitter, breaks->values[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->count - breaks->values[i] - 1));
    }

    lit_uintlist_destroy(emitter->state, breaks);
}

static void lit_emitter_emitstmtscoped(LitEmitter* emitter, LitExpression* statement)
{
    lit_emitter_scopebegin(emitter);

    if(!lit_emitter_emitstmt(emitter, statement))
    {
        lit_emitter_scopeend(emitter);
    }
}

static bool lit_emitter_emitstmt(LitEmitter* emitter, LitExpression* statement)
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
            LitExprList* statements = &((LitBlockStatement*)statement)->statements;
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

            bool private = emitter->compiler->enclosing == NULL && emitter->compiler->scope_depth == 0;

            if(stmt->init == NULL)
            {
                lit_emitter_emitabc(emitter, statement->line, OP_LOAD_NULL, reg, 0, 0);
            }
            else
            {
                lit_emitter_emitexpr(emitter, stmt->init, reg);
            }

            int index = private ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) :
                                  lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, stmt->constant, reg);

            if(private)
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

            int64_t start = emitter->chunk->count;
            lit_emitter_emitstmtscoped(emitter, stmt->if_branch);

            if(stmt->else_branch)
            {
                elseskip = lit_emitter_emittmp(emitter);
            }

            lit_emitter_patchinstr(emitter, condbranchskip, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->count - start));

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
                    lit_emitter_patchinstr(emitter, nextjump, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, elseifcondreg, (int64_t)emitter->chunk->count - nextjump - 1));
                }
            }

            if(stmt->else_branch)
            {
                int64_t elsestart = emitter->chunk->count;
                lit_emitter_emitstmtscoped(emitter, stmt->else_branch);
                lit_emitter_patchinstr(emitter, elseskip, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->count - elseskip - 1));
            }

            for(LitUInt i = 0; i < endjumpcount; i++)
            {
                if(stmt->elseif_conditions->values[i] == NULL)
                {
                    continue;
                }

                lit_emitter_patchinstr(emitter, endjumps[i], LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->count - endjumps[i] - 1));
            }

            break;
        }

        case LIT_EXPR_FUNCTION:
        {
            LitFunctionStatement* stmt = (LitFunctionStatement*)statement;

            bool export = stmt->exported;
            bool private = !export && emitter->compiler->enclosing == NULL && emitter->compiler->scope_depth == 0;
            bool local = !(export || private);

            int index;
            uint8_t reg = 0;

            if(!export)
            {
                index = private ? lit_emitter_resolveprivate(emitter, stmt->name, stmt->length, statement->line) :
                                  lit_emitter_addlocal(emitter, stmt->name, stmt->length, statement->line, false, reg = lit_emitter_reserveregister(emitter));
            }

            LitString* name = lit_string_copy(emitter->state, stmt->name, stmt->length);

            if(local)
            {
                lit_emitter_marklocalinit(emitter, index);
            }
            else if(private)
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

            function->arg_count = stmt->parameters.count;
            function->max_registers += function->arg_count;
            function->vararg = vararg;

            uint16_t functionreg;
            bool closure = function->upvalue_count > 0;

            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);

                for(LitUInt i = 0; i < function->upvalue_count; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];

                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }

                uint16_t constidx = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(clsproto));
                lit_emitter_emitabx(emitter, statement->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(function));
                SET_BIT(functionreg, 8);
            }

            if(export)
            {
                uint16_t nameconst = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(function->name));
                lit_emitter_emitabx(emitter, statement->line, OP_SET_GLOBAL, nameconst, functionreg);
            }
            else if(private)
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

            LitUIntList old_breaks = emitter->breaks;
            LitUIntList old_continues = emitter->continues;

            lit_uintlist_init(&emitter->breaks);
            lit_uintlist_init(&emitter->continues);

            lit_emitter_emitexpr(emitter, stmt->condition, reg);

            LitUInt tmpinstr = lit_emitter_emittmp(emitter);
            lit_emitter_emitstmtscoped(emitter, stmt->body);

            lit_emitter_patchloopjumps(emitter, &emitter->continues);
            lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)beforecond - emitter->chunk->count - 1);
            lit_emitter_patchinstr(emitter, tmpinstr, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, reg, emitter->chunk->count - tmpinstr - 1));
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

            LitUIntList old_breaks = emitter->breaks;
            LitUIntList old_continues = emitter->continues;

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

                LitUInt start = emitter->chunk->count;
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
                    LitUInt incrstart = emitter->chunk->count;

                    lit_emitter_emitexprignoringregister(emitter, stmt->increment);
                    lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->count - 1);

                    start = incrstart;
                    lit_emitter_patchinstr(emitter, bodyjump, LIT_FORM_ASBX_INSTRUCTION(OP_JUMP, 0, (int64_t)emitter->chunk->count - bodyjump - 1));
                }

                emitter->loop_start = start;
                bool endedscope = false;

                emitter->loop_start = start;
                lit_emitter_scopebegin(emitter);

                if(stmt->body != NULL)
                {
                    if(stmt->body->type == LIT_EXPR_BLOCK)
                    {
                        LitExprList* statements = &((LitBlockStatement*)stmt->body)->statements;

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

                lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->count - 1);

                if(stmt->condition != NULL)
                {
                    lit_emitter_patchinstr(emitter, exitjump, LIT_FORM_ABX_INSTRUCTION(OP_FALSE_JUMP, condreg, (int64_t)emitter->chunk->count - exitjump - 1));
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

                LitUInt start = emitter->chunk->count;
                emitter->loop_start = emitter->chunk->count;

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
                        LitExprList* statements = &((LitBlockStatement*)stmt->body)->statements;

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

                lit_emitter_emitasbx(emitter, statement->line, OP_JUMP, 0, (int)start - emitter->chunk->count - 1);
                lit_emitter_patchinstr(emitter, exitjump, LIT_FORM_ABX_INSTRUCTION(OP_NULL_JUMP, iterator, (int64_t)emitter->chunk->count - exitjump - 1));

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

            lit_uintlist_push(emitter->state, &emitter->breaks, lit_emitter_emittmp(emitter));
            break;
        }

        case LIT_EXPR_CONTINUE:
        {
            if(emitter->compiler->loop_depth == 0)
            {
                lit_emitter_raiseerror(emitter, statement->line, "Can't use '%s' outside of loops", "continue");
            }

            lit_uintlist_push(emitter->state, &emitter->continues, lit_emitter_emittmp(emitter));
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
                uint16_t constant = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(stmt->parent));

                b = lit_emitter_reserveregister(emitter);
                lit_emitter_emitabx(emitter, statement->line, OP_GET_GLOBAL, b, constant);
            }

            int nameconst = lit_emitter_addconst(emitter, emitter->last_line, OBJECT_VALUE(stmt->name));
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
                    int fieldnameconst = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(lit_string_copy(emitter->state, var->name, var->length)));

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

            LitFunction* function = lit_emitter_compilerend(emitter, AS_STRING(lit_string_format(emitter->state, "@:@", OBJECT_VALUE(emitter->class_name), stmt->name)));

            function->arg_count = stmt->parameters.count;
            function->max_registers += function->arg_count;
            function->vararg = vararg;

            uint16_t functionreg;
            bool closure = function->upvalue_count > 0;

            if(closure)
            {
                functionreg = lit_emitter_reserveregister(emitter);
                LitClosurePrototype* clsproto = lit_object_makeclosureproto(emitter->state, function);

                for(LitUInt i = 0; i < function->upvalue_count; i++)
                {
                    LitCompilerUpvalue* upvalue = &compiler.upvalues[i];

                    clsproto->local[i] = upvalue->isLocal;
                    clsproto->indexes[i] = upvalue->index;
                }

                uint16_t constidx = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(clsproto));
                lit_emitter_emitabx(emitter, statement->line, OP_CLOSURE, functionreg, constidx);
            }
            else
            {
                functionreg = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(function));
                SET_BIT(functionreg, 8);
            }

            int fieldnameconst = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(stmt->name));
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

                getter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_format(emitter->state, "@:get @", OBJECT_VALUE(emitter->class_name), stmt->name)));
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

                setter = lit_emitter_compilerend(emitter, AS_STRING(lit_string_format(emitter->state, "@:set @", OBJECT_VALUE(emitter->class_name), stmt->name)));
                setter->arg_count = 1;
                setter->max_registers++;
            }

            LitField* field = lit_object_makefield(emitter->state, (LitObject*)getter, (LitObject*)setter);
            int constant = lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(field));
            SET_BIT(constant, 8);

            lit_emitter_emitabc(emitter, statement->line, stmt->is_static ? OP_STATIC_FIELD : OP_METHOD, emitter->class_register,
                                 lit_emitter_addconst(emitter, statement->line, OBJECT_VALUE(stmt->name)), constant);

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

LitModule* lit_emitter_emitmod(LitEmitter* emitter, LitExprList* statements, LitString* modname)
{
    emitter->last_line = 1;
    emitter->emit_reference = 0;

    LitState* state = emitter->state;

    LitValue modulevalue;
    LitModule* module;

    bool new = false;

    if(lit_table_get(&emitter->state->vm->modules->values, modname, &modulevalue))
    {
        module = AS_MODULE(modulevalue);
    }
    else
    {
        module = lit_object_makemodule(emitter->state, modname);
        new = true;
    }

    emitter->module = module;
    LitUInt oldprivatescnt = module->private_count;

    if(oldprivatescnt > 0)
    {
        LitPrivList* privates = &emitter->privates;
        privates->count = oldprivatescnt - 1;

        lit_privlist_push(state, privates, (LitPrivate){ true, false });

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

    if(new)
    {
        LitUInt total = emitter->privates.count;
        module->privates = LIT_ALLOCATE(emitter->state, LitValue, total);

        for(LitUInt i = 0; i < total; i++)
        {
            module->privates[i] = NULL_VALUE;
        }
    }
    else
    {
        module->privates = LIT_GROW_ARRAY(emitter->state, module->privates, LitValue, oldprivatescnt, module->private_count);

        for(LitUInt i = oldprivatescnt; i < module->private_count; i++)
        {
            module->privates[i] = NULL_VALUE;
        }
    }

    lit_privlist_destroy(emitter->state, &emitter->privates);

    if(new && !state->had_error)
    {
        lit_table_set(state, &state->vm->modules->values, modname, OBJECT_VALUE(module));
    }

    module->ran = true;
    return module;
}