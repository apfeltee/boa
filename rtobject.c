
#include <memory.h>
#include <math.h>
#include "lit.h"

bool lit_value_iscallablefunction(LitValue value)
{
    if(IS_OBJECT(value))
    {
        LitObjectType type = OBJECT_TYPE(value);
        return type == LIT_OBJ_CLOSURE || type == LIT_OBJ_FUNCTION || type == LIT_OBJ_NATIVEFUNCTION || type == LIT_OBJ_NATIVEPRIMITIVE
               || type == LIT_OBJ_NATIVEMETHOD || type == LIT_OBJ_PRIMITIVEMETHOD || type == LIT_OBJ_BOUNDMETHOD;
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
    lit_table_set(state, &state->vm->strings, string, NULL_VALUE);
    lit_state_poproot(state);
}

static LitString* lit_object_allocstring(LitState* state, char* chars, LitUInt length, uint32_t hash)
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

    char* heapchars = LIT_ALLOCATE(state, char, length + 1);

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

    return OBJECT_VALUE(lit_string_copy(state, buffer, length));
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
    result->chars = LIT_ALLOCATE(state, char, totallength + 1);
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

    return OBJECT_VALUE(result);
}

LitObject* lit_object_allocobject(LitState* state, size_t size, LitObjectType type)
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
    function->arg_count = 0;
    function->upvalue_count = 0;
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
                return lit_function_getname(vm, OBJECT_VALUE(field->getter));
            }

            return lit_function_getname(vm, OBJECT_VALUE(field->setter));
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
        return OBJECT_VALUE(lit_string_format(vm->state, "function #", *((double*)AS_OBJECT(instance))));
    }

    return OBJECT_VALUE(lit_string_format(vm->state, "function @", OBJECT_VALUE(name)));
}

LitUpvalue* lit_object_makeupvalue(LitState* state, LitValue* slot)
{
    LitUpvalue* upvalue = ALLOCATE_OBJECT(state, LitUpvalue, LIT_OBJ_UPVALUE);

    upvalue->location = slot;
    upvalue->closed = NULL_VALUE;
    upvalue->next = NULL;

    return upvalue;
}

LitClosure* lit_object_makeclosure(LitState* state, LitFunction* function)
{
    LitClosure* closure = ALLOCATE_OBJECT(state, LitClosure, LIT_OBJ_CLOSURE);

    closure->function = function;
    closure->upvalue_count = 0;// To prevent GC crashes

    lit_state_pushroot(state, (LitObject*)closure);
    LitUpvalue** upvalues = LIT_ALLOCATE(state, LitUpvalue*, function->upvalue_count);
    lit_state_poproot(state);

    for(LitUInt i = 0; i < function->upvalue_count; i++)
    {
        upvalues[i] = NULL;
    }

    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;

    return closure;
}

LitClosurePrototype* lit_object_makeclosureproto(LitState* state, LitFunction* function)
{
    LitClosurePrototype* closure = ALLOCATE_OBJECT(state, LitClosurePrototype, LIT_OBJ_CLOSUREPROTOTYPE);

    lit_state_pushroot(state, (LitObject*)closure);

    closure->indexes = LIT_ALLOCATE(state, uint8_t, function->upvalue_count);
    closure->local = LIT_ALLOCATE(state, bool, function->upvalue_count);

    lit_state_poproot(state);

    closure->function = function;
    closure->upvalue_count = function->upvalue_count;

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
    LitValue* registers = LIT_ALLOCATE(state, LitValue, registers_allocated);

    LitCallFrame* frames = LIT_ALLOCATE(state, LitCallFrame, LIT_INITIAL_CALL_FRAMES);
    LitFiber* fiber = ALLOCATE_OBJECT(state, LitFiber, LIT_OBJ_FIBER);

    if(module->main_fiber == NULL)
    {
        module->main_fiber = fiber;
    }

    fiber->registers = registers;

    for(uint8_t i = 0; i < registers_allocated; i++)
    {
        fiber->registers[i] = NULL_VALUE;
    }

    fiber->registers_allocated = registers_allocated;
    fiber->frames = frames;
    fiber->frame_capacity = LIT_INITIAL_CALL_FRAMES;
    fiber->parent = NULL;
    fiber->frame_count = function == NULL ? 0 : 1;
    fiber->arg_count = 0;
    fiber->module = module;
    fiber->catcher = false;
    fiber->caught = false;
    fiber->error = NULL_VALUE;
    fiber->open_upvalues = NULL;
    fiber->abort = false;
    fiber->return_address = NULL;

    if(function != NULL)
    {
        LitCallFrame* frame = &fiber->frames[0];

        frame->closure = NULL;
        frame->function = function;
        frame->slots = fiber->registers;
        frame->result_ignored = false;
        frame->return_to_c = false;
        frame->return_address = NULL;

        lit_fiber_ensureregisters(state, fiber, function->max_registers);
        frame->ip = function->chunk.code;
    }

    return fiber;
}

LitFiber* lit_object_makefiberclosure(LitState* state, LitModule* module, LitClosure* closure)
{
    LitFiber* fiber = lit_object_makefiber(state, module, closure->function);
    fiber->frames[0].closure = closure;

    return fiber;
}

void lit_fiber_ensureregisters(LitState* state, LitFiber* fiber, LitUInt needed)
{
    if(fiber->registers_allocated >= needed)
    {
        return;
    }

    LitUInt capacity = (LitUInt)lit_closest_power_of_two((int)needed);
    LitValue* oldregisters = fiber->registers;

    fiber->registers = (LitValue*)lit_reallocate(state, fiber->registers, sizeof(LitValue) * fiber->registers_allocated, sizeof(LitValue) * capacity);

    for(LitUInt i = fiber->registers_allocated; i < capacity; i++)
    {
        fiber->registers[i] = NULL_VALUE;
    }

    fiber->registers_allocated = capacity;

    if(fiber->registers != oldregisters)
    {
        for(LitUInt i = 0; i < fiber->frame_count; i++)
        {
            LitCallFrame* frame = &fiber->frames[i];
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
    module->return_value = NULL_VALUE;
    module->main_function = NULL;
    module->privates = NULL;
    module->ran = false;
    module->main_fiber = NULL;
    module->private_count = 0;
    module->private_names = lit_object_makemap(state);

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

LitMap* lit_object_makemap(LitState* state)
{
    LitMap* map = ALLOCATE_OBJECT(state, LitMap, LIT_OBJ_MAP);

    lit_init_table(&map->values);
    map->index_fn = NULL;

    return map;
}

bool lit_map_set(LitState* state, LitMap* map, LitString* key, LitValue value)
{
    if(value == NULL_VALUE)
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
        LitTableEntry* entry = &from->values.entries[i];

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
        userdata->data = lit_reallocate(state, NULL, 0, size);
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