
#include <math.h>
#include "priv.h"

static bool lit_state_ensurefiber(LitVm* vm, LitFiber* fiber)
{
    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(vm, "No fiber to run on");
        return true;
    }

    if(fiber->frame_count == LIT_CALL_FRAMES_MAX)
    {
        lit_vm_raisefatalerror(vm, "Stack overflow");
        return true;
    }

    if(fiber->frame_count + 1 > fiber->frame_capacity)
    {
        LitUInt newcapacity = fmin(LIT_CALL_FRAMES_MAX, fiber->frame_capacity * 2);

        fiber->frames = (LitCallFrame*)lit_reallocate(vm->state, fiber->frames, sizeof(LitCallFrame) * fiber->frame_capacity, sizeof(LitCallFrame) * newcapacity);
        fiber->frame_capacity = newcapacity;
    }

    return false;
}

static inline LitCallFrame* setup_call(LitState* state, LitFunction* callee, LitValue* arguments, uint8_t argc)
{
    LitVm* vm = state->vm;
    LitFiber* fiber = vm->fiber;

    if(callee == NULL)
    {
        lit_vm_raisefatalerror(vm, "Attempt to call a null value");
        return NULL;
    }

    if(lit_state_ensurefiber(vm, fiber))
    {
        return NULL;
    }

    LitValue* start
    = fiber->frame_count > 0 ? fiber->frames[fiber->frame_count - 1].slots + fiber->frames[fiber->frame_count - 1].function->max_registers : fiber->registers;
    lit_fiber_ensureregisters(vm->state, fiber, start - fiber->registers + callee->max_registers);

    LitCallFrame* frame = &fiber->frames[fiber->frame_count++];
    frame->slots = fiber->frame_count > 1 ? fiber->frames[fiber->frame_count - 2].slots + fiber->frames[fiber->frame_count - 2].function->max_registers : fiber->registers;

#ifdef LIT_TRACE_NULL_FILL
    printf("Filling with nulls\n");
#endif

    for(int i = argc + 1; i < callee->max_registers; i++)
    {
        frame->slots[i] = NULL_VALUE;
    }

    frame->slots[0] = OBJECT_VALUE(callee);

    for(uint8_t i = 0; i < argc; i++)
    {
        frame->slots[i + 1] = arguments[i];
    }

    LitUInt targetargcount = callee->arg_count;
    bool vararg = callee->vararg;

    if(targetargcount > argc)
    {
#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif

        for(LitUInt i = argc; i < targetargcount; i++)
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
        if(targetargcount == argc && IS_VARARG_ARRAY(*(frame->slots + targetargcount)))
        {
            // No need to repack the arguments
        }
        else
        {
            LitArray* array = &lit_object_makevararray(vm->state)->array;
            lit_state_pushroot(vm->state, (LitObject*)array);
            lit_values_ensure_size(vm->state, &array->values, argc - targetargcount + 1);

            LitUInt j = 0;

            for(LitUInt i = targetargcount - 1; i < argc; i++)
            {
                array->values.values[j++] = *(frame->slots + i + 1);
            }

            *(frame->slots + targetargcount) = OBJECT_VALUE(array);
            lit_state_poproot(vm->state);
        }
    }

    frame->ip = callee->chunk.code;
    frame->closure = NULL;
    frame->function = callee;
    frame->result_ignored = false;
    frame->return_to_c = true;
    frame->return_address = NULL;

    return frame;
}

static inline LitResult execute_call(LitState* state, LitCallFrame* frame)
{
    if(frame == NULL)
    {
        RETURN_RUNTIME_ERROR()
    }

    LitFiber* fiber = state->vm->fiber;
    LitResult result = lit_interpret_fiber(state, fiber);

    if(fiber->error != NULL_VALUE)
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

    if(IS_OBJECT(callee))
    {
        if(lit_set_native_exit_jump())
        {
            RETURN_RUNTIME_ERROR()
        }

        LitObjectType type = OBJECT_TYPE(callee);

        if(type == LIT_OBJ_FUNCTION)
        {
            return lit_state_callfunction(state, AS_FUNCTION(callee), arguments, argc);
        }
        else if(type == LIT_OBJ_CLOSURE)
        {
            return lit_state_callclosure(state, AS_CLOSURE(callee), arguments, argc);
        }

        LitFiber* fiber = vm->fiber;

        if(lit_state_ensurefiber(vm, fiber))
        {
            RETURN_RUNTIME_ERROR()
        }

        LitValue* start = fiber->frame_count > 0 ? fiber->frames[fiber->frame_count - 1].slots + fiber->frames[fiber->frame_count - 1].function->max_registers :
                                                   fiber->registers;
        lit_fiber_ensureregisters(vm->state, fiber, start - fiber->registers + 3 + argc);
        LitValue* slot = fiber->frame_count > 0 ? fiber->frames[fiber->frame_count - 1].slots + fiber->frames[fiber->frame_count - 1].function->max_registers :
                                                  fiber->registers;

#ifdef LIT_TRACE_NULL_FILL
        printf("Filling with nulls\n");
#endif

        for(int i = argc; i < argc + 3; i++)
        {
            *(slot + i) = NULL_VALUE;
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
            fprintf(stderr, "        |\n        | f%i ", fiber->frame_count);
            for(int i = 0; i <= argc; i++)
            {
                fprintf(stderr, "[ ");
                lit_print_value(*(slot + i));
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
                RETURN_OK(NULL_VALUE)
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
                RETURN_OK(NULL_VALUE)
            }

            case LIT_OBJ_CLASS:
            {
                LitClass* klass = AS_CLASS(callee);
                LitInstance* inst = lit_object_makeinstance(vm->state, klass);

                if(klass->init_method != NULL)
                {
                    lit_state_callmethod(state, *slot, OBJECT_VALUE(klass->init_method), arguments, argc);
                }

                RETURN_OK(OBJECT_VALUE(inst))
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
                    RETURN_OK(NULL_VALUE)
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

    if(IS_NULL(callee))
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
    LitVm* vm = state->vm;
    LitFiber* fiber = vm->fiber;

    if(fiber == NULL)
    {
        lit_vm_raisefatalerror(vm, "No fiber to run on");
        RETURN_RUNTIME_ERROR()
    }

    LitClass* klass = lit_state_getclassfor(state, callee);
    LitValue method;

    if((IS_INSTANCE(callee) && lit_table_get(&AS_INSTANCE(callee)->fields, mthname, &method)) || lit_table_get(&klass->methods, mthname, &method))
    {
        return lit_state_callmethod(state, callee, method, arguments, argc);
    }

    return (LitResult){ INTERPRET_INVALID, NULL_VALUE };
}

LitString* lit_tostring_value(LitState* state, LitValue object, LitUInt indentation)
{
    if(IS_STRING(object))
    {
        return AS_STRING(object);
    }
    else if(!IS_OBJECT(object))
    {
        if(IS_NULL(object))
        {
            return CONST_STRING(state, "null");
        }
        else if(IS_NUMBER(object))
        {
            return AS_STRING(lit_string_numbertostring(state, AS_NUMBER(object)));
        }
        else if(IS_BOOL(object))
        {
            return CONST_STRING(state, AS_BOOL(object) ? "true" : "false");
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

    if(lit_state_ensurefiber(vm, fiber))
    {
        return CONST_STRING(state, "null");
    }

    LitFunction* function = state->api_function;

    if(function == NULL)
    {
        function = state->api_function = lit_object_makefunction(state, fiber->module);
        function->chunk.has_line_info = false;
        function->name = state->api_name;

        LitChunk* chunk = &function->chunk;
        chunk->count = 0;
        chunk->constants.count = 0;
        function->max_registers = 3;

        int constant = lit_chunk_addconstant(state, chunk, OBJECT_CONST_STRING(state, "toString"));

        lit_chunk_push(state, chunk, LIT_FORM_ABC_INSTRUCTION(OP_INVOKE, 1, 2, constant), 1);
        lit_chunk_push(state, chunk, LIT_FORM_ABC_INSTRUCTION(OP_RETURN, 1, 0, 0), 1);
    }

    lit_fiber_ensureregisters(
    vm->state, fiber,
    (fiber->frame_count > 0 ? (fiber->frames[fiber->frame_count - 1].slots + (int)fiber->frames[fiber->frame_count - 1].function->max_registers) : fiber->registers)
    - fiber->registers + function->max_registers);
    LitCallFrame* frame = &fiber->frames[fiber->frame_count++];

    frame->ip = function->chunk.code;
    frame->closure = NULL;
    frame->function = function;

    // "Duplicated" code due to lit_fiber_ensureregisters messing with register pointers
    frame->slots
    = (fiber->frame_count > 1 ? (fiber->frames[fiber->frame_count - 2].slots + (int)fiber->frames[fiber->frame_count - 2].function->max_registers) : fiber->registers);
    frame->result_ignored = false;
    frame->return_to_c = true;
    frame->return_address = NULL;

    frame->slots[0] = OBJECT_VALUE(function);
    frame->slots[1] = object;
    frame->slots[2] = NUMBER_VALUE(indentation);

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
        return NULL_VALUE;
    }

    LitClass* klass = AS_CLASS(value);

    if(klass->init_method == NULL)
    {
        return OBJECT_VALUE(lit_object_makeinstance(vm->state, klass));
    }

    return lit_state_callmethod(vm->state, value, value, args, argc).result;
}

#undef PUSH