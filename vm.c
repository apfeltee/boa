

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <setjmp.h>
#include "priv.h"



#define lit_vmexec_pushgc(state, allow) \
    bool wasallowed = state->allow_gc; \
    state->allow_gc = allow;

#define lit_vmexec_popgc(state) \
    state->allow_gc = wasallowed;

jmp_buf lit_vmglobal_jumpbuf;


static inline void lit_vmexec_traceframe(LitVm* vm, LitFiber* fiber)
{
    if(vm->state->config.traceexecution)
    {
        lit_debug_traceframe(fiber);
    }
}

static void lit_vmexec_resetvm(LitState* state, LitVm* vm)
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

    vm->globals = lit_object_makemap(state);
    vm->modules = lit_object_makemap(state);
}

void lit_free_vm(LitVm* vm)
{
    lit_free_table(vm->state, &vm->strings);
    lit_free_objects(vm->state, vm->objects);

    lit_vmexec_resetvm(vm->state, vm);
}

bool lit_vm_handleerror(LitVm* vm, LitString* errorstring)
{
    LitValue error = OBJECT_VALUE(errorstring);
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
    int count = (int)fiber->frame_count - 1;
    size_t length = snprintf(NULL, 0, "%s%s\n", COLOR_RED, errorstring->chars);
    for(int i = count; i >= 0; i--)
    {
        LitCallFrame* frame = &fiber->frames[i];
        LitFunction* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->chars;
        if(chunk->has_line_info)
        {
            length += snprintf(NULL, 0, "[line %d] in %s()\n", lit_chunk_getline(chunk, frame->ip - chunk->code - 1), name);
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
        LitCallFrame* frame = &fiber->frames[i];
        LitFunction* function = frame->function;
        LitChunk* chunk = &function->chunk;
        const char* name = function->name == NULL ? "unknown" : function->name->chars;

        if(chunk->has_line_info)
        {
            start += sprintf(start, "[line %d] in %s()\n", lit_chunk_getline(chunk, frame->ip - chunk->code - 1), name);
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

static bool lit_vmexec_callcallable(LitVm* vm, LitFunction* function, LitClosure* closure, uint8_t arg_count, LitUInt calleeregister)
{
    LitFiber* fiber = vm->fiber;
    assert(fiber->frame_count > 0);

    if(fiber->frame_count == LIT_CALL_FRAMES_MAX)
    {
        lit_vm_raiseerror(vm, "Stack overflow");
        return false;
    }

    if(fiber->frame_count + 1 > fiber->frame_capacity)
    {
        LitUInt newcapacity = fmin(LIT_CALL_FRAMES_MAX, fiber->frame_capacity * 2);
        fiber->frames = (LitCallFrame*)lit_reallocate(vm->state, fiber->frames, sizeof(LitCallFrame) * fiber->frame_capacity, sizeof(LitCallFrame) * newcapacity);
        fiber->frame_capacity = newcapacity;
    }

    LitCallFrame* frame = &fiber->frames[fiber->frame_count++];
    LitCallFrame* previousframe = &fiber->frames[fiber->frame_count - 2];

    frame->function = function;
    frame->closure = closure;
    frame->ip = function->chunk.code;
    frame->slots = previousframe->slots + calleeregister;
    frame->result_ignored = false;
    frame->return_to_c = false;
    frame->return_address = previousframe->slots + (int)calleeregister;

    lit_fiber_ensureregisters(vm->state, fiber, frame->slots - fiber->registers + function->max_registers);
    LitUInt targetargcount = function->arg_count;
    bool vararg = function->vararg;

    if(targetargcount > arg_count)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif

        for(LitUInt i = arg_count; i < targetargcount; i++)
        {
            *(frame->slots + i + 1) = NULL_VALUE;
        }

        if(vararg)
        {
            *(frame->slots + targetargcount) = OBJECT_VALUE(lit_object_makearray(vm->state));
        }
    }
    else if(vararg)
    {
        if(targetargcount == arg_count && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            // No need to repack the arguments
        }
        else
        {
            LitArray* array = &lit_object_makevararray(vm->state)->array;
            lit_state_pushroot(vm->state, (LitObject*)array);
            lit_values_ensure_size(vm->state, &array->values, arg_count - targetargcount + 1);

            LitUInt j = 0;

            for(LitUInt i = targetargcount - 1; i < arg_count; i++)
            {
                array->values.values[j++] = *(frame->slots + i + 1);
            }

            *(frame->slots + targetargcount) = OBJECT_VALUE(array);
            lit_state_poproot(vm->state);
        }
    }

    return true;
}

static bool lit_vmexec_actualcallvalue(LitVm* vm, LitUInt calleeregister, uint8_t arg_count, LitValue alternatecallee)
{
    LitCallFrame* frame = &vm->fiber->frames[vm->fiber->frame_count - 1];
    LitValue callee = IS_NULL(alternatecallee) ? frame->slots[calleeregister] : alternatecallee;

    if(IS_OBJECT(callee))
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
                return lit_vmexec_callcallable(vm, AS_FUNCTION(callee), NULL, arg_count, calleeregister);
            }

            case LIT_OBJ_CLOSURE:
            {
                LitClosure* closure = AS_CLOSURE(callee);
                return lit_vmexec_callcallable(vm, closure->function, closure, arg_count, calleeregister);
            }

            case LIT_OBJ_NATIVEFUNCTION:
            {
                // For some reason, single line expression doesn't work
                LitValue value = AS_NATIVE_FUNCTION(callee)->function(vm, arg_count, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;

                return !vm->fiber->abort;
            }

            case LIT_OBJ_NATIVEPRIMITIVE:
            {
                lit_vmexec_pushgc(vm->state, false);

                LitFiber* fiber = vm->fiber;
                bool result = AS_NATIVE_PRIMITIVE(callee)->function(vm, arg_count, frame->slots + calleeregister + 1);

                lit_vmexec_popgc(vm->state);
                return !result;
            }

            case LIT_OBJ_NATIVEMETHOD:
            {
                lit_vmexec_pushgc(vm->state, false);

                LitNativeMethod* method = AS_NATIVE_METHOD(callee);
                LitFiber* fiber = vm->fiber;

                // For some reason, single line expression doesn't work
                LitValue value = method->method(vm, *(frame->slots + calleeregister), arg_count, frame->slots + calleeregister + 1);
                frame->slots[calleeregister] = value;

                lit_vmexec_popgc(vm->state);

                return !fiber->abort;
            }

            case LIT_OBJ_PRIMITIVEMETHOD:
            {
                lit_vmexec_pushgc(vm->state, false);

                LitFiber* fiber = vm->fiber;
                bool result = AS_PRIMITIVE_METHOD(callee)->method(vm, *(frame->slots + calleeregister), arg_count, frame->slots + calleeregister + 1);

                lit_vmexec_popgc(vm->state);
                return !result;
            }

            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* instance = lit_object_makeinstance(vm->state, klass);

                frame->slots[calleeregister] = OBJECT_VALUE(instance);

                if(klass->init_method != NULL)
                {
                    return lit_vmexec_actualcallvalue(vm, calleeregister, arg_count, OBJECT_VALUE(klass->init_method));
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
                    LitValue value = AS_NATIVE_METHOD(method)->method(vm, boundmethod->receiver, arg_count, frame->slots + calleeregister + 1);
                    frame->slots[calleeregister] = value;
                    lit_vmexec_popgc(vm->state);

                    return !vm->fiber->abort;
                }
                else if(IS_PRIMITIVE_METHOD(method))
                {
                    LitFiber* fiber = vm->fiber;
                    lit_vmexec_pushgc(vm->state, false);

                    if(AS_PRIMITIVE_METHOD(method)->method(vm, boundmethod->receiver, arg_count, frame->slots + calleeregister + 1))
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
                    return lit_vmexec_callcallable(vm, AS_FUNCTION(method), NULL, arg_count, calleeregister);
                }

                return !vm->fiber->abort;
            }

            default:
            {
                break;
            }
        }
    }

    if(IS_NULL(callee))
    {
        return lit_vm_raiseerror(vm, "Attempt to call a null value");
    }
    else
    {
        return lit_vm_raiseerror(vm, "Can only call functions and classes, got %s", lit_get_value_type(callee));
    }

    return true;
}

static LitUpvalue* lit_vmexec_captureupvalue(LitState* state, LitValue* local)
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

static void lit_vmexec_closeupvalues(LitVm* vm, const LitValue* last)
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


#define lit_vmexec_dispatchnext() \
    goto dispatch;

#define CASE_CODE(name) \
    OP_##name:

#define lit_vmexec_readframe() \
    fiber = vm->fiber; \
    vm->frame = &fiber->frames[fiber->frame_count - 1]; \
    vm->current_chunk = &vm->frame->function->chunk; \
    vm->constants = vm->current_chunk->constants.values; \
    vm->ip = vm->frame->ip; \
    fiber->module = vm->frame->function->module; \
    vm->registers = vm->frame->slots; \
    vm->privates = fiber->module->privates; \
    vm->upvalues = vm->frame->closure == NULL ? NULL : vm->frame->closure->upvalues;

#define lit_vmexec_writeframe() \
    vm->frame->ip = vm->ip;

#define lit_vmexec_returnerror() \
    lit_vmexec_popgc(state); \
    return (LitResult){ INTERPRET_RUNTIME_ERROR, NULL_VALUE };

#define lit_vmexec_recoverstate() \
    lit_vmexec_writeframe(); \
    fiber = vm->fiber; \
    if(fiber == NULL) \
    { \
        return (LitResult){ INTERPRET_OK, NULL_VALUE }; \
    } \
    if(fiber->abort) \
    { \
        lit_vmexec_returnerror(); \
    } \
    lit_vmexec_readframe(); \
    lit_vmexec_traceframe(vm, fiber);

#define lit_vmexec_callvalue(callee, reg, arg_count) \
    if(!lit_vmexec_actualcallvalue(vm, reg, arg_count, callee)) \
    { \
        lit_vmexec_recoverstate(); \
    }

#define lit_vm_failva(format, ...) \
    if(lit_vm_raiseerror(vm, format, __VA_ARGS__)) \
    { \
        lit_vmexec_recoverstate(); \
        lit_vmexec_dispatchnext(); \
    } \
    else \
    { \
        lit_vmexec_returnerror(); \
    }

#define lit_vm_fail(format) \
    lit_vm_failva(format, NULL)

#define lit_vmexec_getrc(r) \
    (IS_BIT_SET(r, 8) ? vm->constants[r & 0xff] : vm->registers[r])

// TODO: push_root()?
#define WRAP_CONSTANT(r, to, tmp) \
    LitValue tmp = vm->registers[to]; \
    vm->registers[to] = lit_vmexec_getrc(r);

#define UNWRAP_CONSTANT(r, to, tmp) vm->registers[to] = tmp;

#define INVOKE_METHOD(reg, bv, m, arg_count) \
    lit_vmexec_writeframe() \
    LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vm_fail("Only instances and classes have methods") \
    } \
    LitString* mthname = CONST_STRING(vm->state, m); \
    LitValue method; \
    if((IS_INSTANCE(bv) && (lit_table_get(&AS_INSTANCE(bv)->fields, mthname, &method))) || lit_table_get(&klass->methods, mthname, &method)) \
    { \
        lit_vmexec_callvalue(method, reg, arg_count) \
    } \
    else \
    { \
        lit_vm_failva("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars) \
    } \
    lit_vmexec_readframe()

#define INVOKE_METHOD_AND_CONTINUE(reg, bv, m, arg_count) \
    lit_vmexec_writeframe() \
    LitClass* klass = lit_state_getclassfor(state, bv); \
    if(klass == NULL) \
    { \
        lit_vm_fail("Only instances and classes have methods"); \
    } \
    LitString* mthname = CONST_STRING(vm->state, m); \
    LitValue method; \
    if((IS_INSTANCE(bv) && (lit_table_get(&AS_INSTANCE(bv)->fields, mthname, &method))) || lit_table_get(&klass->methods, mthname, &method)) \
    { \
        lit_vmexec_callvalue(method, reg, arg_count); \
        lit_vmexec_readframe(); \
        lit_vmexec_dispatchnext(); \
    }

// Instruction helpers
#define BINARY_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(vm->instruction); \
    uint16_t b = LIT_INSTRUCTION_B(vm->instruction); \
    uint16_t c = LIT_INSTRUCTION_C(vm->instruction); \
    LitValue bv = lit_vmexec_getrc(b); \
    LitValue cv = lit_vmexec_getrc(c); \
    if(IS_NUMBER(bv)) \
    { \
        if(!IS_NUMBER(cv)) \
        { \
            lit_vm_failva("Attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        vm->registers[a] = type(AS_NUMBER(bv) op AS_NUMBER(cv)); \
    } \
    else if(IS_NULL(bv)) \
    { \
        lit_vm_failva("Attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        WRAP_CONSTANT(b, a, tmpa); \
        WRAP_CONSTANT(c, a + 1, tmpb); \
        INVOKE_METHOD(a, vm->registers[a], opstring, 1); \
        UNWRAP_CONSTANT(c, a + 1, tmpb); \
    }

#define COMPARISON_INSTRUCTION(type, op, opstring) \
    uint8_t a = LIT_INSTRUCTION_A(vm->instruction); \
    uint16_t b = LIT_INSTRUCTION_B(vm->instruction); \
    uint16_t c = LIT_INSTRUCTION_C(vm->instruction); \
    LitValue bv = lit_vmexec_getrc(b); \
    LitValue cv = lit_vmexec_getrc(c); \
    if(IS_NUMBER(bv)) \
    { \
        if(!IS_NUMBER(cv)) \
        { \
            lit_vm_failva("Attempt to use the operator %s with a number and a %s", opstring, lit_get_value_type(cv)); \
        } \
        vm->registers[a] = type(AS_NUMBER(bv) op AS_NUMBER(cv)); \
    } \
    else if(IS_NULL(bv)) \
    { \
        lit_vm_failva("Attempt to use the operator %s on a null value", opstring); \
    } \
    else \
    { \
        WRAP_CONSTANT(b, a, tmpa); \
        WRAP_CONSTANT(c, a + 1, tmpb); \
        INVOKE_METHOD(a, vm->registers[a], opstring, 1); \
        UNWRAP_CONSTANT(c, a + 1, tmpb); \
    }

#define BITWISE_INSTRUCTION(op, opstring) \
    LitValue bv = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction)); \
    LitValue cv = lit_vmexec_getrc(LIT_INSTRUCTION_C(vm->instruction)); \
    if(!IS_NUMBER(bv) && !IS_NUMBER(cv)) \
    { \
        lit_vm_failva("Operands of bitwise op %s must be two numbers, got %s and %s", opstring, lit_get_value_type(bv), lit_get_value_type(cv)); \
    } \
    vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = (NUMBER_VALUE((int)AS_NUMBER(bv) op(int) AS_NUMBER(cv)));


LitResult lit_interpret_fiber(LitState* state, LitFiber* fiber)
{
    assert(fiber->frame_count > 0);
    state->vm->fiber = fiber;

    // Has to be inside of the function in order for goto to work
    static void* dispatchtable[] = {
#define OPCODE(name, a, b) &&OP_##name,
#include "opcodes.inc"
#undef OPCODE
    };
    LitCallFrame* previousframe;

    LitVm* vm = state->vm;
    LitTable* globals;
    globals = &vm->globals->values;

    lit_vmexec_pushgc(state, true)

    fiber->abort = false;

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
    lit_vmexec_readframe()
    vm->fiber = fiber;
    vm->registers[0] = OBJECT_VALUE(vm->frame->function);
    lit_vmexec_traceframe(vm, fiber);
    if(state->config.traceexecution)
    {
        printf("fiber start:\n");
        LitCallFrame* previousframe = vm->frame;
    }

dispatch:
    vm->instruction = *vm->ip++;

    if(state->config.traceexecution)
    {
        previousframe = vm->frame;
        if(vm->frame->function->max_registers > 0)
        {
            printf("        |\n        | f%i ", fiber->frame_count);

            for(int i = 0; i < vm->frame->function->max_registers; i++)
            {
                printf("[ ");
                lit_print_value(*(vm->registers + i));
                printf(" ]");
            }

            printf("\n");
        }
        lit_debug_disasinstr(vm->current_chunk, (LitUInt)(vm->ip - vm->current_chunk->code - 1), NULL, vm->frame != previousframe);
        previousframe = vm->frame;
    }
    goto* dispatchtable[LIT_INSTRUCTION_OPCODE(vm->instruction)];

    CASE_CODE(MOVE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction));
        lit_vmexec_dispatchnext()
    }

    CASE_CODE(LOAD_NULL)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = NULL_VALUE;
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(LOAD_BOOL)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = BOOL_VALUE(LIT_INSTRUCTION_B(vm->instruction) != 0);
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(CLOSURE)
    {
        LitClosurePrototype* clsproto = AS_CLOSURE_PROTOTYPE(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]);
        LitClosure* closure = lit_object_makeclosure(state, clsproto->function);
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(closure);
        for(LitUInt i = 0; i < closure->function->upvalue_count; i++)
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
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(ARRAY)
    {
        LitArray* array = lit_object_makearray(state);
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(array);
        lit_values_ensure_size_empty(state, &array->values, LIT_INSTRUCTION_B(vm->instruction));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(OBJECT)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makeinstance(state, state->object_class));
        lit_vmexec_dispatchnext()
    }

    CASE_CODE(RANGE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)]
        = OBJECT_VALUE(lit_object_makerange(state, AS_NUMBER(lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction))), AS_NUMBER(lit_vmexec_getrc(LIT_INSTRUCTION_C(vm->instruction)))));
        lit_vmexec_dispatchnext()
    }

    CASE_CODE(RETURN)
    {
        LitValue value = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
        lit_vmexec_closeupvalues(vm, vm->registers);

        fiber->frame_count--;

        if(vm->frame->return_to_c)
        {
            vm->frame->return_to_c = false;
            fiber->module->return_value = value;

            return (LitResult){ INTERPRET_OK, value };
        }

        if(fiber->frame_count == 0 || vm->frame->return_address == NULL)
        {
            if(fiber->frame_count == 0)
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


                lit_vmexec_readframe()
                lit_vmexec_dispatchnext()
            }

            return (LitResult){ INTERPRET_OK, value };
        }

        *vm->frame->return_address = value;

        lit_vmexec_readframe();
        lit_vmexec_traceframe(vm, fiber);
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(ADD)
    {
        BINARY_INSTRUCTION(NUMBER_VALUE, +, "+");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(SUBTRACT)
    {
        BINARY_INSTRUCTION(NUMBER_VALUE, -, "-");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(MULTIPLY)
    {
        BINARY_INSTRUCTION(NUMBER_VALUE, *, "*");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(DIVIDE)
    {
        BINARY_INSTRUCTION(NUMBER_VALUE, /, "/");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(FLOOR_DIVIDE)
    {
        uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
        uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
        uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
        LitValue bv = lit_vmexec_getrc(b);
        LitValue cv = lit_vmexec_getrc(c);
        if(IS_NUMBER(bv) && IS_NUMBER(cv))
        {
            vm->registers[a] = NUMBER_VALUE(floor(AS_NUMBER(bv) / AS_NUMBER(cv)));
        }
        else
        {
            WRAP_CONSTANT(b, a, tmpa);
            WRAP_CONSTANT(c, a + 1, tmpb);
            INVOKE_METHOD(a, vm->registers[a], "#", 1);
            UNWRAP_CONSTANT(c, a + 1, tmpb);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(MOD)
    {
        uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
        uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
        uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
        LitValue bv = lit_vmexec_getrc(b);
        LitValue cv = lit_vmexec_getrc(c);
        if(IS_NUMBER(bv) && IS_NUMBER(cv))
        {
            vm->registers[a] = NUMBER_VALUE(fmod(AS_NUMBER(bv), AS_NUMBER(cv)));
        }
        else
        {
            WRAP_CONSTANT(b, a, tmpa);
            WRAP_CONSTANT(c, a + 1, tmpb);
            INVOKE_METHOD(a, vm->registers[a], "%", 1);
            UNWRAP_CONSTANT(c, a + 1, tmpb);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(POWER)
    {
        uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
        uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
        uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
        LitValue bv = lit_vmexec_getrc(b);
        LitValue cv = lit_vmexec_getrc(c);
        if(IS_NUMBER(bv) && IS_NUMBER(cv))
        {
            vm->registers[a] = NUMBER_VALUE(pow(AS_NUMBER(bv), AS_NUMBER(cv)));
        }
        else
        {
            WRAP_CONSTANT(b, a, tmpa);
            WRAP_CONSTANT(c, a + 1, tmpb);
            INVOKE_METHOD(a, vm->registers[a], "**", 1);
            UNWRAP_CONSTANT(c, a + 1, tmpb);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(LSHIFT)
    {
        BITWISE_INSTRUCTION(<<, "<<");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(RSHIFT)
    {
        BITWISE_INSTRUCTION(>>, ">>");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(BXOR)
    {
        BITWISE_INSTRUCTION(^, "^");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(BAND)
    {
        BITWISE_INSTRUCTION(&, "&");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(BOR)
    {
        BITWISE_INSTRUCTION(|, "|");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(JUMP)
    {
        vm->ip += LIT_INSTRUCTION_SBX(vm->instruction);
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(TRUE_JUMP)
    {
        if(!lit_is_falsey(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
        {
            vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(FALSE_JUMP)
    {
        if(lit_is_falsey(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]))
        {
            vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(NON_NULL_JUMP)
    {
        if(vm->registers[LIT_INSTRUCTION_A(vm->instruction)] != NULL_VALUE)
        {
            vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(NULL_JUMP)
    {
        if(vm->registers[LIT_INSTRUCTION_A(vm->instruction)] == NULL_VALUE)
        {
            vm->ip += LIT_INSTRUCTION_BX(vm->instruction);
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(EQUAL)
    {
        uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
        uint16_t b = LIT_INSTRUCTION_B(vm->instruction);
        uint16_t c = LIT_INSTRUCTION_C(vm->instruction);
        LitValue bv = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction));
        if(IS_INSTANCE(bv))
        {
            WRAP_CONSTANT(b, a, tmpa);
            WRAP_CONSTANT(c, a + 1, tmpb);
            INVOKE_METHOD_AND_CONTINUE(a, vm->registers[a], "==", 1);
            UNWRAP_CONSTANT(c, a + 1, tmpb);
        }
        vm->registers[a] = BOOL_VALUE(bv == lit_vmexec_getrc(c));
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(LESS)
    {
        COMPARISON_INSTRUCTION(BOOL_VALUE, <, "<");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(LESS_EQUAL)
    {
        COMPARISON_INSTRUCTION(BOOL_VALUE, <=, "<=");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(GREATER)
    {
        COMPARISON_INSTRUCTION(BOOL_VALUE, >, ">");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(GREATER_EQUAL)
    {
        COMPARISON_INSTRUCTION(BOOL_VALUE, >=, ">=");
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(NEGATE)
    {
        LitValue value = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction));
        if(!IS_NUMBER(value))
        {
            // Don't even ask me why
            // This doesn't kill our performance, since it's a error anyway
            if(IS_STRING(value) && strcmp(AS_CSTRING(value), "muffin") == 0)
            {
                lit_vm_fail("Idk, can you negate a muffin?");
            }
            else
            {
                lit_vm_fail("Operand must be a number");
            }
        }
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = NUMBER_VALUE(-AS_NUMBER(value));
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(NOT)
    {
        uint8_t b = LIT_INSTRUCTION_B(vm->instruction);
        LitValue value = lit_vmexec_getrc(b);
        if(IS_INSTANCE(value))
        {
            INVOKE_METHOD_AND_CONTINUE(b, value, "!", 0);
        }
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = BOOL_VALUE(lit_is_falsey(value));
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(BNOT)
    {
        LitValue value = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction));
        if(!IS_NUMBER(value))
        {
            lit_vm_fail("Operand must be a number");
        }
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = NUMBER_VALUE(~((int)AS_NUMBER(value)));
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(SET_GLOBAL)
    {
        lit_table_set(state, globals, AS_STRING(vm->constants[LIT_INSTRUCTION_A(vm->instruction)]), lit_vmexec_getrc(LIT_INSTRUCTION_BX(vm->instruction)));
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(GET_GLOBAL)
    {
        LitValue* reg = &vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
        if(!lit_table_get(globals, AS_STRING(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]), reg))
        {
            *reg = NULL_VALUE;
        }
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(SET_UPVALUE)
    {
        *vm->frame->closure->upvalues[LIT_INSTRUCTION_A(vm->instruction)]->location = lit_vmexec_getrc(LIT_INSTRUCTION_BX(vm->instruction));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(GET_UPVALUE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = *vm->frame->closure->upvalues[LIT_INSTRUCTION_BX(vm->instruction)]->location;
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(SET_PRIVATE)
    {
        uint8_t a = LIT_INSTRUCTION_A(vm->instruction);
        uint32_t b = LIT_INSTRUCTION_BX(vm->instruction);

        vm->privates[(uint16_t)b] = IS_BIT_SET(b, 16) ? vm->constants[a] : vm->registers[a];
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(GET_PRIVATE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = vm->privates[LIT_INSTRUCTION_BX(vm->instruction)];
        lit_vmexec_dispatchnext();
    }
    CASE_CODE(CALL)
    {
        lit_vmexec_writeframe();

        if(!lit_vmexec_actualcallvalue(vm, LIT_INSTRUCTION_A(vm->instruction), LIT_INSTRUCTION_B(vm->instruction) - 1, NULL_VALUE))
        {
            lit_vmexec_returnerror();
        }

        lit_vmexec_readframe();
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(CLOSE_UPVALUE)
    {
        lit_vmexec_closeupvalues(vm, &vm->registers[LIT_INSTRUCTION_A(vm->instruction)] - 1);
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(CLASS)
    {
        LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_A(vm->instruction)]);
        LitClass* klass = lit_object_makeclass(state, name);

        vm->registers[LIT_INSTRUCTION_C(vm->instruction)] = OBJECT_VALUE(klass);
        lit_table_set(state, &vm->globals->values, name, OBJECT_VALUE(klass));

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
                lit_vm_fail("Superclass must be a class");
            }

            LitClass* superklass = AS_CLASS(super);

            klass->super = superklass;
            klass->init_method = superklass->init_method;

            lit_table_add_all(state, &superklass->methods, &klass->methods);
            lit_table_add_all(state, &klass->super->static_fields, &klass->static_fields);
        }

        lit_vmexec_dispatchnext();
    }

    CASE_CODE(STATIC_FIELD)
    {
        lit_table_set(state, &AS_CLASS(vm->registers[LIT_INSTRUCTION_A(vm->instruction)])->static_fields, AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]),
                      lit_vmexec_getrc(LIT_INSTRUCTION_C(vm->instruction)));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(METHOD)
    {
        LitClass* klass = AS_CLASS(vm->registers[LIT_INSTRUCTION_A(vm->instruction)]);
        LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]);

        if((klass->init_method == NULL || (klass->super != NULL && klass->init_method == ((LitClass*)klass->super)->init_method)) && name->length == 11
           && memcmp(name->chars, "constructor", 11) == 0)
        {
            klass->init_method = AS_OBJECT(lit_vmexec_getrc(LIT_INSTRUCTION_C(vm->instruction)));
        }

        lit_table_set(state, &klass->methods, name, lit_vmexec_getrc(LIT_INSTRUCTION_C(vm->instruction)));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(GET_FIELD)
    {
        LitValue object = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];

        if(IS_NULL(object))
        {
            lit_vm_fail("Attempt to index a null value");
        }

        LitValue value;
        LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);

        if(IS_INSTANCE(object))
        {
            LitInstance* instance = AS_INSTANCE(object);

            if(!lit_table_get(&instance->fields, name, &value))
            {
                if(lit_table_get(&instance->klass->methods, name, &value))
                {
                    if(IS_FIELD(value))
                    {
                        LitField* field = AS_FIELD(value);

                        if(field->getter == NULL)
                        {
                            lit_vm_failva("Class %s does not have a getter for the field %s", instance->klass->name->chars, name->chars);
                        }

                        lit_vmexec_writeframe();
                        lit_vmexec_callvalue(OBJECT_VALUE(AS_FIELD(value)->getter), resultreg, 0);
                        lit_vmexec_readframe();
                        lit_vmexec_dispatchnext();
                    }
                    else
                    {
                        value = OBJECT_VALUE(lit_object_makeboundmethod(state, object, value));
                    }
                }
                else
                {
                    value = NULL_VALUE;
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
                    value = OBJECT_VALUE(lit_object_makeboundmethod(state, object, value));
                }
                else if(IS_FIELD(value))
                {
                    LitField* field = AS_FIELD(value);

                    if(field->getter == NULL)
                    {
                        lit_vm_failva("Class %s does not have a getter for the field %s", klass->name->chars, name->chars);
                    }

                    lit_vmexec_writeframe();
                    lit_vmexec_callvalue(OBJECT_VALUE(field->getter), resultreg, 0);
                    lit_vmexec_readframe();
                    lit_vmexec_dispatchnext();
                }
            }
            else
            {
                value = NULL_VALUE;
            }
        }
        else
        {
            LitClass* klass = lit_state_getclassfor(state, object);

            if(klass == NULL)
            {
                lit_vm_fail("Only instances and classes have fields");
            }

            if(lit_table_get(&klass->methods, name, &value))
            {
                if(IS_FIELD(value))
                {
                    LitField* field = AS_FIELD(value);

                    if(field->getter == NULL)
                    {
                        lit_vm_failva("Class %s does not have a getter for the field %s", klass->name->chars, name->chars);
                    }

                    lit_vmexec_writeframe();
                    lit_vmexec_callvalue(OBJECT_VALUE(AS_FIELD(value)->getter), resultreg, 0);
                    lit_vmexec_readframe();
                    lit_vmexec_dispatchnext();
                }
                else if(IS_NATIVE_METHOD(value) || IS_PRIMITIVE_METHOD(value))
                {
                    value = OBJECT_VALUE(lit_object_makeboundmethod(state, object, value));
                }
            }
            else
            {
                value = NULL_VALUE;
            }
        }

        vm->registers[resultreg] = value;
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(GET_SUPER_METHOD)
    {
        LitValue instance = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
        LitClass* klass = AS_CLASS(instance);
        LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);

        LitValue value;

        if(lit_table_get(&klass->methods, mthname, &value) || lit_table_get(&klass->static_fields, mthname, &value))
        {
            value = OBJECT_VALUE(lit_object_makeboundmethod(state, vm->registers[LIT_INSTRUCTION_A(vm->instruction)], value));
        }
        else
        {
            value = NULL_VALUE;
        }

        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = value;
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(SET_FIELD)
    {
        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = vm->registers[resultreg];

        if(IS_NULL(instance))
        {
            lit_vm_fail("Attempt to index a null value");
        }

        LitValue value = vm->registers[LIT_INSTRUCTION_C(vm->instruction)];
        int b = LIT_INSTRUCTION_B(vm->instruction);
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
                    lit_vm_failva("Class %s does not have a setter for the field %s", klass->name->chars, fieldname->chars);
                }

                lit_vmexec_writeframe();
                lit_vmexec_callvalue(OBJECT_VALUE(field->setter), resultreg, 1);
                lit_vmexec_readframe();
                lit_vmexec_dispatchnext();
            }

            if(IS_NULL(value))
            {
                lit_table_delete(&klass->static_fields, fieldname);
            }
            else
            {
                lit_table_set(state, &klass->static_fields, fieldname, value);
            }
        }
        else if(IS_INSTANCE(instance))
        {
            LitInstance* inst = AS_INSTANCE(instance);
            LitValue setter;

            if(lit_table_get(&inst->klass->methods, fieldname, &setter) && IS_FIELD(setter))
            {
                LitField* field = AS_FIELD(setter);

                if(field->setter == NULL)
                {
                    lit_vm_failva("Class %s does not have a setter for the field %s", inst->klass->name->chars, fieldname->chars);
                }

                lit_vmexec_writeframe();
                lit_vmexec_callvalue(OBJECT_VALUE(field->setter), resultreg, 1);
                lit_vmexec_readframe();
                lit_vmexec_dispatchnext();
            }

            if(IS_NULL(value))
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
                lit_vm_fail("Only instances and classes have fields");
            }

            LitValue setter;

            if(lit_table_get(&klass->methods, fieldname, &setter) && IS_FIELD(setter))
            {
                LitField* field = AS_FIELD(setter);

                if(field->setter == NULL)
                {
                    lit_vm_failva("Class %s does not have a setter for the field %s", klass->name->chars, fieldname->chars);
                }

                lit_vmexec_writeframe();
                lit_vmexec_callvalue(OBJECT_VALUE(field->setter), resultreg, 1);
                lit_vmexec_readframe();
                lit_vmexec_dispatchnext();
            }
            else
            {
                lit_vm_failva("Class %s does not contain field %s", klass->name->chars, fieldname->chars);
            }
        }

        vm->registers[resultreg] = value;
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(IS)
    {
        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = lit_vmexec_getrc(LIT_INSTRUCTION_B(vm->instruction));

        if(IS_NULL(instance))
        {
            vm->registers[resultreg] = FALSE_VALUE;
            lit_vmexec_dispatchnext();
        }

        LitClass* instanceklass = lit_state_getclassfor(state, instance);
        LitValue klass;

        if(!lit_table_get(globals, AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]), &klass))
        {
            vm->registers[resultreg] = FALSE_VALUE;
            lit_vmexec_dispatchnext();
        }

        if(instanceklass == NULL || !IS_CLASS(klass))
        {
            lit_vm_fail("Operands must be an instance and a class");
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

        vm->registers[resultreg] = BOOL_VALUE(found);
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(INVOKE)
    {
        lit_vmexec_writeframe();

        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = vm->registers[resultreg];

        if(IS_NULL(instance))
        {
            lit_vm_fail("Attempt to index a null value");
        }

        LitClass* klass = IS_CLASS(instance) ? AS_CLASS(instance) : lit_state_getclassfor(state, instance);

        if(klass == NULL)
        {
            lit_vm_fail("Only instances and classes have methods");
        }

        LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
        int arg_count = LIT_INSTRUCTION_B(vm->instruction) - 1;
        LitValue method;

        if(IS_INSTANCE(instance) && (lit_table_get(&AS_INSTANCE(instance)->fields, mthname, &method)))
        {
            lit_vmexec_callvalue(method, resultreg, arg_count);
        }
        else if(IS_CLASS(instance) && lit_table_get(&klass->static_fields, mthname, &method))
        {
            lit_vmexec_callvalue(method, resultreg, arg_count);
        }
        else if(lit_table_get(&klass->methods, mthname, &method))
        {
            lit_vmexec_callvalue(method, resultreg, arg_count);
        }
        else
        {
            lit_vm_failva("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars);
        }

        lit_vmexec_readframe();
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(INVOKE_SUPER)
    {
        lit_vmexec_writeframe();

        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = vm->registers[resultreg + 1];

        if(IS_NULL(instance))
        {
            lit_vm_fail("Attempt to index a null value");
        }

        LitClass* klass = AS_CLASS(instance);

        if(klass == NULL)
        {
            lit_vm_fail("Only instances and classes have methods");
        }

        LitString* mthname = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);
        int arg_count = LIT_INSTRUCTION_B(vm->instruction) - 1;
        LitValue method;

        if(lit_table_get(&klass->methods, mthname, &method) || lit_table_get(&klass->static_fields, mthname, &method))
        {
            for(LitUInt i = resultreg + 1; i <= resultreg + (LitUInt)arg_count; i++)
            {
                vm->registers[i] = vm->registers[i + 1];
            }

            lit_vmexec_callvalue(method, resultreg, arg_count);
        }
        else
        {
            lit_vm_failva("Attempt to call method '%s', that is not defined in class %s", mthname->chars, klass->name->chars);
        }

        lit_vmexec_readframe();
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(SUBSCRIPT_GET)
    {
        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = lit_vmexec_getrc(resultreg);

        INVOKE_METHOD(resultreg, instance, "[]", 1);
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(SUBSCRIPT_SET)
    {
        uint8_t resultreg = LIT_INSTRUCTION_A(vm->instruction);
        LitValue instance = lit_vmexec_getrc(resultreg);

        INVOKE_METHOD(resultreg, instance, "[]", 2);
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(PUSH_ARRAY_ELEMENT)
    {
        LitValList* array = &AS_ARRAY(vm->registers[LIT_INSTRUCTION_A(vm->instruction)])->values;
        array->values[array->count++] = lit_vmexec_getrc(LIT_INSTRUCTION_BX(vm->instruction));

        lit_vmexec_dispatchnext();
    }

    CASE_CODE(PUSH_OBJECT_ELEMENT)
    {
        LitValue operand = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];
        LitString* key = AS_STRING(vm->constants[LIT_INSTRUCTION_B(vm->instruction)]);
        LitValue value = vm->registers[LIT_INSTRUCTION_C(vm->instruction)];

        if(IS_MAP(operand))
        {
            lit_table_set(state, &AS_MAP(operand)->values, key, value);
        }
        else if(IS_INSTANCE(operand))
        {
            lit_table_set(state, &AS_INSTANCE(operand)->fields, key, value);
        }
        else
        {
            lit_vm_failva("slotted an object or a map as the operand, got %s", lit_get_value_type(operand));
        }

        lit_vmexec_dispatchnext();
    }

    CASE_CODE(REFERENCE_GLOBAL)
    {
        LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_BX(vm->instruction)]);
        LitValue* value;

        if(lit_table_get_slot(&vm->globals->values, name, &value))
        {
            vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makereference(state, value));
        }
        else
        {
            lit_vm_fail("Attempt to reference a null value");
        }

        lit_vmexec_dispatchnext();
    }

    CASE_CODE(REFERENCE_PRIVATE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makereference(state, &vm->privates[LIT_INSTRUCTION_BX(vm->instruction)]));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(REFERENCE_LOCAL)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makereference(state, &vm->registers[LIT_INSTRUCTION_B(vm->instruction)]));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(REFERENCE_UPVALUE)
    {
        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makereference(state, vm->upvalues[LIT_INSTRUCTION_BX(vm->instruction)]->location));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(REFERENCE_FIELD)
    {
        LitValue object = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];

        if(IS_NULL(object))
        {
            lit_vm_fail("Attempt to index a null value");
        }

        LitValue* value;
        LitString* name = AS_STRING(vm->constants[LIT_INSTRUCTION_C(vm->instruction)]);

        if(IS_INSTANCE(object))
        {
            if(!lit_table_get_slot(&AS_INSTANCE(object)->fields, name, &value))
            {
                lit_vm_fail("Attempt to reference a null value");
            }
        }
        else
        {
            lit_vm_fail("You can only reference fields of real instances");
        }

        vm->registers[LIT_INSTRUCTION_A(vm->instruction)] = OBJECT_VALUE(lit_object_makereference(state, value));
        lit_vmexec_dispatchnext();
    }

    CASE_CODE(SET_REFERENCE)
    {
        LitValue reference = vm->registers[LIT_INSTRUCTION_A(vm->instruction)];

        if(!IS_REFERENCE(reference))
        {
            lit_vm_fail("Provided value is not a reference");
        }

        *AS_REFERENCE(reference)->slot = vm->registers[LIT_INSTRUCTION_B(vm->instruction)];
        lit_vmexec_dispatchnext();
    }

    lit_vm_failva("Unknown op %i", vm->instruction);
    lit_vmexec_returnerror();
}

void lit_native_exit_jump()
{
    longjmp(lit_vmglobal_jumpbuf, 1);
}

