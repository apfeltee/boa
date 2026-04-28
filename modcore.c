
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "lit.h"

void lit_state_openlibraries(LitState* state)
{
    lit_open_math_library(state);
    lit_open_file_library(state);
    lit_open_gc_library(state);
}

static LitValue lit_objfn_invalidconstructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    lit_vm_raisefatalerror(vm, "Can't create an instance of built-in type", AS_INSTANCE(instance)->klass->name);
    return NULL_VALUE;
}

/*
 * Class
 */

static LitValue lit_objfn_class_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_string_format(vm->state, "class @", OBJECT_VALUE(AS_CLASS(instance)->name)));
}

static int lit_coreutil_tableiterator(LitTable* table, int number)
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

static LitValue lit_coreutil_tableiterkey(LitTable* table, int index)
{
    if(table->capacity <= index)
    {
        return NULL_VALUE;
    }

    return OBJECT_VALUE(table->entries[index].key);
}

static LitValue lit_objfn_class_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);

    LitClass* klass = AS_CLASS(instance);
    int index = args[0] == NULL_VALUE ? -1 : AS_NUMBER(args[0]);
    int methodsCapacity = (int)klass->methods.capacity;
    bool fields = index >= methodsCapacity;

    int value = lit_coreutil_tableiterator(fields ? &klass->static_fields : &klass->methods, fields ? index - methodsCapacity : index);

    if(value == -1)
    {
        if(fields)
        {
            return NULL_VALUE;
        }

        index++;
        fields = true;
        value = lit_coreutil_tableiterator(&klass->static_fields, index - methodsCapacity);
    }

    return value == -1 ? NULL_VALUE : NUMBER_VALUE(fields ? value + methodsCapacity : value);
}

static LitValue lit_objfn_class_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitClass* klass = AS_CLASS(instance);
    LitUInt methodsCapacity = klass->methods.capacity;
    bool fields = index >= methodsCapacity;

    return lit_coreutil_tableiterkey(fields ? &klass->static_fields : &klass->methods, fields ? index - methodsCapacity : index);
}

static LitValue lit_objfn_class_super(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitClass* super = NULL;

    if(IS_INSTANCE(instance))
    {
        super = AS_INSTANCE(instance)->klass->super;
    }
    else
    {
        super = AS_CLASS(instance)->super;
    }

    if(super == NULL)
    {
        return NULL_VALUE;
    }

    return OBJECT_VALUE(super);
}

static LitValue lit_objfn_class_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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

    return NULL_VALUE;
}


static LitValue lit_objfn_class_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(AS_CLASS(instance)->name);
}

/*
 * Object
 */

static LitValue lit_objfn_object_class(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_state_getclassfor(vm->state, instance));
}

static LitValue lit_objfn_object_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitState* state = vm->state;
    LitClass* klass = lit_state_getclassfor(vm->state, instance);

    if(klass != state->object_class)
    {
        return lit_string_format(state, "@ instance", OBJECT_VALUE(klass->name));
    }

    LitTable* values = &AS_INSTANCE(instance)->fields;

    if(values->count == 0)
    {
        return OBJECT_CONST_STRING(state, "{}");
    }

    LitUInt valueamount = values->count;

    LitString* valuesconverted[valueamount];
    LitString* keys[valueamount];

    LitUInt indentation = LIT_GET_NUMBER(0, 0) + 1;
    LitUInt lit_objfn_string_length = indentation + 2;

    LitUInt i = 0;
    LitUInt index = 0;

    do
    {
        LitTableEntry* entry = &values->entries[index++];

        if(entry->key != NULL)
        {
            LitString* value = lit_tostring_value(state, entry->value, indentation);

            lit_state_pushroot(state, (LitObject*)value);

            if(IS_STRING(entry->value))
            {
                value = AS_STRING(lit_string_format(state, "\"@\"", OBJECT_VALUE(value)));
                lit_state_poproot(state);
                lit_state_pushroot(state, (LitObject*)value);
            }

            valuesconverted[i] = value;
            keys[i] = entry->key;
            lit_objfn_string_length += entry->key->length + 2 + value->length + (i == valueamount - 1 ? 1 : 2) + indentation;

            i++;
        }
    } while(i < valueamount);

    char buffer[lit_objfn_string_length + 1];
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

    buffer[lit_objfn_string_length] = '\0';
    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, lit_objfn_string_length));
}

static LitValue lit_objfn_object_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(!IS_INSTANCE(instance))
    {
        LitObjectType type = OBJECT_TYPE(instance);
        lit_vm_raisefatalerror(vm, "Can't modify built-in types");
    }

    LitInstance* inst = AS_INSTANCE(instance);

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

    return NULL_VALUE;
}

static LitValue lit_objfn_object_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1);

    LitInstance* self = AS_INSTANCE(instance);

    int index = args[0] == NULL_VALUE ? -1 : AS_NUMBER(args[0]);
    int value = lit_coreutil_tableiterator(&self->fields, index);

    return value == -1 ? NULL_VALUE : NUMBER_VALUE(value);
}

static LitValue lit_objfn_object_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitInstance* self = AS_INSTANCE(instance);

    return lit_coreutil_tableiterkey(&self->fields, index);
}

/*
 * Number
 */

static LitValue lit_objfn_number_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_string_numbertostring(vm->state, AS_NUMBER(instance)));
}


static LitValue lit_objfn_number_chr(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    char c;
    double dn;
    LitString* cs;
    dn = AS_NUMBER(instance);
    c = dn;
    cs = lit_string_copy(vm->state, &c, 1);
    return OBJECT_VALUE(cs);
}


/*
 * Bool
 */

static LitValue lit_objfn_bool_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_CONST_STRING(vm->state, AS_BOOL(instance) ? "true" : "false");
}

/*
 * String
 */

static LitValue lit_objfn_string_plus(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
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

    result->chars = LIT_ALLOCATE(vm->state, char, length + 1);
    result->chars[length] = '\0';

    memcpy(result->chars, string->chars, string->length);
    memcpy(result->chars + string->length, stringvalue->chars, stringvalue->length);

    result->hash = lit_string_hash(result->chars, result->length);
    lit_string_register(vm->state, result);

    return OBJECT_VALUE(result);
}

static LitValue lit_objfn_string_compare(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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
                return TRUE_VALUE;
            }
        }
        return FALSE_VALUE;
    }
    else if(IS_NULL(args[0]))
    {
        if((self == NULL) || IS_NULL(instance))
        {
            return TRUE_VALUE;
        }
        return FALSE_VALUE;
    }
    lit_vm_raisefatalerror(vm, "can only compare string to another string or null");
    return FALSE_VALUE;
}

static LitValue lit_objfn_string_less(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return BOOL_VALUE(strcmp(AS_STRING(instance)->chars, LIT_CHECK_STRING(0)) < 0);
}

static LitValue lit_objfn_string_greater(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return BOOL_VALUE(strcmp(AS_STRING(instance)->chars, LIT_CHECK_STRING(0)) > 0);
}

static LitValue lit_objfn_string_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return instance;
}

static LitValue lit_objfn_string_tonumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    double result = strtod(AS_STRING(instance)->chars, NULL);

    if(errno == ERANGE)
    {
        errno = 0;
        return NULL_VALUE;
    }

    return NUMBER_VALUE(result);
}

static LitValue lit_objfn_string_touppercase(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    char buffer[string->length];

    for(LitUInt i = 0; i < string->length; i++)
    {
        buffer[i] = (char)toupper(string->chars[i]);
    }

    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, string->length));
}

static LitValue lit_objfn_string_tolowercase(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    char buffer[string->length];

    for(LitUInt i = 0; i < string->length; i++)
    {
        buffer[i] = (char)tolower(string->chars[i]);
    }

    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, string->length));
}

static LitValue lit_objfn_string_contains(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);

    if(sub == string)
    {
        return TRUE_VALUE;
    }

    return BOOL_VALUE(strstr(string->chars, sub->chars) != NULL);
}

static LitValue lit_objfn_string_startswith(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);

    if(sub == string)
    {
        return TRUE_VALUE;
    }

    if(sub->length > string->length)
    {
        return FALSE_VALUE;
    }

    for(LitUInt i = 0; i < sub->length; i++)
    {
        if(sub->chars[i] != string->chars[i])
        {
            return FALSE_VALUE;
        }
    }

    return TRUE_VALUE;
}

static LitValue lit_objfn_string_endswith(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    LitString* sub = LIT_CHECK_OBJECT_STRING(0);

    if(sub == string)
    {
        return TRUE_VALUE;
    }

    if(sub->length > string->length)
    {
        return FALSE_VALUE;
    }

    LitUInt start = string->length - sub->length;

    for(LitUInt i = 0; i < sub->length; i++)
    {
        if(sub->chars[i] != string->chars[i + start])
        {
            return FALSE_VALUE;
        }
    }

    return TRUE_VALUE;
}

static LitValue lit_objfn_string_replace(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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

    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, bufferlength));
}

static LitValue lit_objfn_string_splice(LitVm* vm, LitString* string, int from, int to)
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

    return OBJECT_VALUE(lit_ustring_from_range(vm->state, string, from, to - from + 1));
}

static LitValue lit_objfn_string_substring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);

    return lit_objfn_string_splice(vm, AS_STRING(instance), from, to);
}

static LitValue lit_objfn_string_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(IS_RANGE(args[0]))
    {
        LitRange* range = AS_RANGE(args[0]);
        return lit_objfn_string_splice(vm, AS_STRING(instance), range->from, range->to);
    }

    LitString* string = AS_STRING(instance);
    int index = LIT_CHECK_NUMBER(0);

    if(argc != 1)
    {
        lit_vm_raisefatalerror(vm, "Can't modify strings with the subscript op");
    }

    if(index < 0)
    {
        index = lit_ustring_length(string) + index;

        if(index < 0)
        {
            return NULL_VALUE;
        }
    }

    LitString* c = lit_ustring_code_point_at(vm->state, string, lit_uchar_offset(string->chars, index));

    return c == NULL ? NULL_VALUE : OBJECT_VALUE(c);
}


static LitValue lit_objfn_string_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(lit_ustring_length(AS_STRING(instance)));
}

static LitValue lit_objfn_string_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);

    if(IS_NULL(args[0]))
    {
        if(string->length == 0)
        {
            return NULL_VALUE;
        }

        return NUMBER_VALUE(0);
    }

    int index = LIT_CHECK_NUMBER(0);

    if(index < 0)
    {
        return NULL_VALUE;
    }

    do
    {
        index++;

        if(index >= (int)string->length)
        {
            return NULL_VALUE;
        }
    } while((string->chars[index] & 0xc0) == 0x80);

    return NUMBER_VALUE(index);
}

static LitValue lit_objfn_string_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* string = AS_STRING(instance);
    uint32_t index = LIT_CHECK_NUMBER(0);

    if(index == UINT32_MAX)
    {
        return false;
    }

    return OBJECT_VALUE(lit_ustring_code_point_at(vm->state, string, index));
}

/*
 * Function
 */

static LitValue lit_objfn_function_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return lit_function_getname(vm, instance);
}

static LitValue lit_objfn_function_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return lit_function_getname(vm, instance);
}

/*
 * Fiber
 */

static LitValue lit_objfn_fiber_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitValue arg;

    if((argc < 1) || (!IS_CALLABLE_FUNCTION(args[0])))
    {
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

    return OBJECT_VALUE(fiber);
}

static bool lit_coreutil_isfiberdone(LitFiber* fiber)
{
    return fiber->frame_count == 0 || fiber->abort;
}

static LitValue lit_objfn_fiber_done(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return BOOL_VALUE(lit_coreutil_isfiberdone(AS_FIBER(instance)));
}

static LitValue lit_objfn_fiber_error(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return AS_FIBER(instance)->error;
}

static LitValue lit_objfn_fiber_current(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(vm->fiber);
}

static void lit_coreutil_runfiber(LitVm* vm, LitFiber* fiber, LitValue* args, LitUInt argc, bool catcher)
{
    if(lit_coreutil_isfiberdone(fiber))
    {
        lit_vm_raisefatalerror(vm, "Fiber already finished executing");
    }

    fiber->parent = vm->fiber;
    fiber->catcher = catcher;

    vm->fiber = fiber;

    LitCallFrame* frame = &fiber->frames[fiber->frame_count - 1];

    if(frame->ip == frame->function->chunk.code)
    {
        fiber->arg_count = argc;
        LitFunction* function = frame->function;

        LitValue* start = fiber->frame_count > 1 ? fiber->frames[fiber->frame_count - 2].slots + fiber->frames[fiber->frame_count - 2].function->max_registers :
                                                   fiber->registers;
        lit_fiber_ensureregisters(vm->state, fiber, start - fiber->registers + function->max_registers);

        frame->slots = fiber->frame_count > 1 ? fiber->frames[fiber->frame_count - 2].slots + fiber->frames[fiber->frame_count - 2].function->max_registers :
                                                fiber->registers;

        for(int i = argc + 1; i < function->max_registers; i++)
        {
            frame->slots[i] = NULL_VALUE;
        }

        frame->slots[0] = OBJECT_VALUE(function);

        for(uint8_t i = 0; i < argc; i++)
        {
            frame->slots[i + 1] = args[i];
        }

        bool vararg = frame->function->vararg;
        LitUInt functionargcount = function->arg_count;

        fiber->arg_count = functionargcount;

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
                *(frame->slots + functionargcount) = OBJECT_VALUE(array);

                int varargcount = argc - functionargcount + 1;

                if(varargcount > 0)
                {
                    lit_values_ensure_size(vm->state, &array->values, varargcount);

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

static bool lit_objfn_fiber_run(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    vm->fiber->return_address = args - 1;
    lit_coreutil_runfiber(vm, AS_FIBER(instance), args, argc, false);
    return true;
}

static bool lit_objfn_fiber_try(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    vm->fiber->return_address = args - 1;
    lit_coreutil_runfiber(vm, AS_FIBER(instance), args, argc, true);
    return true;
}

static bool lit_objfn_fiber_yield(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(vm->fiber->parent == NULL)
    {
        lit_vm_handleerror(vm, argc == 0 ? CONST_STRING(vm->state, "Fiber was yielded") : lit_tostring_value(vm->state, args[0], 0));
        return true;
    }

    vm->fiber = vm->fiber->parent;
    *vm->fiber->return_address = argc == 0 ? NULL_VALUE : OBJECT_VALUE(lit_tostring_value(vm->state, args[0], 0));

    return true;
}

static bool lit_objfn_fiber_yeet(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(vm->fiber->parent == NULL)
    {
        lit_vm_handleerror(vm, argc == 0 ? CONST_STRING(vm->state, "Fiber was yeeted") : lit_tostring_value(vm->state, args[0], 0));
        return true;
    }

    vm->fiber = vm->fiber->parent;
    *vm->fiber->return_address = argc == 0 ? NULL_VALUE : OBJECT_VALUE(lit_tostring_value(vm->state, args[0], 0));

    return true;
}

static bool lit_objfn_fiber_abort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitString* value = argc == 0 ? CONST_STRING(vm->state, "Fiber was aborted") : lit_tostring_value(vm->state, args[0], 0);

    lit_vm_handleerror(vm, value);
    if(vm->fiber->return_address != NULL)
    {
        *vm->fiber->return_address = OBJECT_VALUE(value);
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
        return NULL_VALUE;
    }

    LitModule* module = AS_MODULE(value);

    if(id == name)
    {
        return OBJECT_VALUE(module);
    }

    if(lit_table_get(&module->private_names->values, name, &value))
    {
        int index = (int)AS_NUMBER(value);

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

    return NULL_VALUE;
}

static LitValue lit_objfn_module_privates(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitModule* module = IS_MODULE(instance) ? AS_MODULE(instance) : vm->fiber->module;
    LitMap* map = module->private_names;

    if(map->index_fn == NULL)
    {
        map->index_fn = lit_coreutil_accessprivate;
        lit_table_set(vm->state, &map->values, CONST_STRING(vm->state, "_module"), OBJECT_VALUE(module));
    }

    return OBJECT_VALUE(map);
}

static LitValue lit_objfn_module_current(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(vm->fiber->module);
}

static LitValue lit_objfn_module_toString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_string_format(vm->state, "Module @", OBJECT_VALUE(AS_MODULE(instance)->name)));
}

static LitValue lit_objfn_module_name(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(AS_MODULE(instance)->name);
}

/*
 * Array
 */

static LitValue lit_objfn_array_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_object_makearray(vm->state));
}

static LitValue lit_objfn_array_splice(LitVm* vm, LitArray* array, int from, int to)
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
        lit_vallist_push(vm->state, &newarray->values, array->values.values[from + i]);
    }

    return OBJECT_VALUE(newarray);
}

static LitValue lit_objfn_array_slice(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int from = LIT_CHECK_NUMBER(0);
    int to = LIT_CHECK_NUMBER(1);

    return lit_objfn_array_splice(vm, AS_ARRAY(instance), from, to);
}

static LitValue lit_objfn_array_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(argc == 2)
    {
        if(!IS_NUMBER(args[0]))
        {
            lit_vm_raisefatalerror(vm, "Array index must be a number");
        }

        LitValList* values = &AS_ARRAY(instance)->values;
        int index = AS_NUMBER(args[0]);

        if(index < 0)
        {
            index = fmax(0, values->count + index);
        }

        lit_values_ensure_size(vm->state, values, index + 1);
        return values->values[index] = args[1];
    }

    if(!IS_NUMBER(args[0]))
    {
        if(IS_RANGE(args[0]))
        {
            LitRange* range = AS_RANGE(args[0]);
            return lit_objfn_array_splice(vm, AS_ARRAY(instance), (int)range->from, (int)range->to);
        }

        lit_vm_raisefatalerror(vm, "Array index must be a number");
        return NULL_VALUE;
    }

    LitValList* values = &AS_ARRAY(instance)->values;
    int index = AS_NUMBER(args[0]);

    if(index < 0)
    {
        index = fmax(0, values->count + index);
    }

    if(values->capacity <= (LitUInt)index)
    {
        return NULL_VALUE;
    }

    return values->values[index];
}

static LitValue lit_objfn_array_add(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    lit_vallist_push(vm->state, &AS_ARRAY(instance)->values, args[0]);

    return NULL_VALUE;
}

static LitValue lit_objfn_array_insert(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(2)

    LitValList* values = &AS_ARRAY(instance)->values;
    int index = LIT_CHECK_NUMBER(0);

    if(index < 0)
    {
        index = fmax(0, values->count + index);
    }

    LitValue value = args[1];

    if((int)values->count <= index)
    {
        lit_values_ensure_size(vm->state, values, index + 1);
    }
    else
    {
        lit_values_ensure_size(vm->state, values, values->count + 1);

        for(int i = values->count - 1; i > index; i--)
        {
            values->values[i] = values->values[i - 1];
        }
    }

    values->values[index] = value;
    return NULL_VALUE;
}

static LitValue lit_objfn_array_addall(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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
        lit_vallist_push(vm->state, &array->values, toAdd->values.values[i]);
    }

    return NULL_VALUE;
}

static int lit_coreutil_indexof(LitArray* array, LitValue value)
{
    for(LitUInt i = 0; i < array->values.count; i++)
    {
        if(array->values.values[i] == value)
        {
            return (int)i;
        }
    }

    return -1;
}

static LitValue lit_objfn_array_indexof(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    int index = lit_coreutil_indexof(AS_ARRAY(instance), args[0]);
    return index == -1 ? NULL_VALUE : NUMBER_VALUE(index);
}

static LitValue lit_coreutil_removeat(LitArray* array, LitUInt index)
{
    LitValList* values = &array->values;
    LitUInt count = values->count;

    if(index >= count)
    {
        return NULL_VALUE;
    }

    LitValue value = values->values[index];

    if(index == count - 1)
    {
        values->values[index] = NULL_VALUE;
    }
    else
    {
        for(LitUInt i = index; i < values->count - 1; i++)
        {
            values->values[i] = values->values[i + 1];
        }

        values->values[count - 1] = NULL_VALUE;
    }

    values->count--;
    return value;
}

static LitValue lit_objfn_array_remove(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    LitArray* array = AS_ARRAY(instance);
    int index = lit_coreutil_indexof(array, args[0]);

    if(index != -1)
    {
        return lit_coreutil_removeat(array, (LitUInt)index);
    }

    return NULL_VALUE;
}

static LitValue lit_objfn_array_removeat(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int index = LIT_CHECK_NUMBER(0);

    if(index < 0)
    {
        return NULL_VALUE;
    }

    return lit_coreutil_removeat(AS_ARRAY(instance), (LitUInt)index);
}

static LitValue lit_objfn_array_contains(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    return BOOL_VALUE(lit_coreutil_indexof(AS_ARRAY(instance), args[0]) != -1);
}

static LitValue lit_objfn_array_clear(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    AS_ARRAY(instance)->values.count = 0;
    return NULL_VALUE;
}

static LitValue lit_objfn_array_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    LitArray* array = AS_ARRAY(instance);
    int number = 0;

    if(IS_NUMBER(args[0]))
    {
        number = AS_NUMBER(args[0]);

        if(number >= (int)array->values.count - 1)
        {
            return NULL_VALUE;
        }

        number++;
    }

    return array->values.count == 0 ? NULL_VALUE : NUMBER_VALUE(number);
}

static LitValue lit_objfn_array_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    LitValList* values = &AS_ARRAY(instance)->values;

    if(values->count <= index)
    {
        return NULL_VALUE;
    }

    return values->values[index];
}

static LitValue lit_objfn_array_foreach(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitValue callback = args[0];

    if(!IS_CALLABLE_FUNCTION(callback))
    {
        lit_vm_raisefatalerror(vm, "Expected a function as the callback");
    }

    LitValList* values = &AS_ARRAY(instance)->values;

    for(LitUInt i = 0; i < values->count; i++)
    {
        lit_state_callvalue(vm->state, callback, &values->values[i], 1);
    }

    return NULL_VALUE;
}

static LitValue lit_objfn_array_join(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitValList* values = &AS_ARRAY(instance)->values;
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

    return OBJECT_VALUE(lit_string_copy(vm->state, chars, length));
}

static inline bool compare(LitState* state, LitValue a, LitValue b)
{
    if(IS_NUMBER(a) && IS_NUMBER(b))
    {
        return AS_NUMBER(a) < AS_NUMBER(b);
    }

    return !lit_is_falsey(lit_state_findandcallmethod(state, a, CONST_STRING(state, "<"), (LitValue[1]){ b }, 1).result);
}

static void lit_coreutil_basicquicksort(LitState* state, LitValue* l, int length)
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
        while(i < pivotindex && compare(state, l[i], pivot))
        {
            i++;
        }

        while(j > pivotindex && compare(state, pivot, l[j]))
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

static void lit_coreutil_customquicksort(LitVm* vm, LitValue* l, int length, LitValue callee)
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

static LitValue lit_objfn_array_sort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitValList* values = &AS_ARRAY(instance)->values;

    if(argc == 1 && IS_CALLABLE_FUNCTION(args[0]))
    {
        lit_coreutil_customquicksort(vm, values->values, values->count, args[0]);
    }
    else
    {
        lit_coreutil_basicquicksort(vm->state, values->values, values->count);
    }

    return instance;
}

static LitValue lit_objfn_array_clone(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitState* state = vm->state;
    LitValList* values = &AS_ARRAY(instance)->values;
    LitArray* array = lit_object_makearray(state);
    LitValList* newvalues = &array->values;

    lit_values_ensure_size(state, newvalues, values->count);

    // lit_values_ensure_size sets the count to max of previous count (0 in this case) and new count, so we have to reset it
    newvalues->count = 0;

    for(LitUInt i = 0; i < values->count; i++)
    {
        lit_vallist_push(state, newvalues, values->values[i]);
    }

    return OBJECT_VALUE(array);
}

static LitValue lit_objfn_array_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt indentation = LIT_SINGLE_LINE_MAPS_ENABLED ? 0 : LIT_GET_NUMBER(0, 0) + 1;

    LitValList* values = &AS_ARRAY(instance)->values;
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
            value = AS_STRING(lit_string_format(state, "\"@\"", OBJECT_VALUE(value)));
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
    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, slength));
}

static LitValue lit_objfn_array_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(AS_ARRAY(instance)->values.count);
}

/*
 * Map
 */

static LitValue lit_objfn_map_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return OBJECT_VALUE(lit_object_makemap(vm->state));
}

static LitValue lit_objfn_map_subscript(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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
        return NULL_VALUE;
    }

    return value;
}

static LitValue lit_objfn_map_addall(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    if(!IS_MAP(args[0]))
    {
        lit_vm_raisefatalerror(vm, "Expected map as the argument");
    }

    lit_map_add_all(vm->state, AS_MAP(args[0]), AS_MAP(instance));
    return NULL_VALUE;
}

static LitValue lit_objfn_map_clear(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    AS_MAP(instance)->values.count = 0;
    return NULL_VALUE;
}

static LitValue lit_objfn_map_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    int index = args[0] == NULL_VALUE ? -1 : AS_NUMBER(args[0]);

    int value = lit_coreutil_tableiterator(&AS_MAP(instance)->values, index);
    return value == -1 ? NULL_VALUE : NUMBER_VALUE(value);
}

static LitValue lit_objfn_map_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt index = LIT_CHECK_NUMBER(0);
    return lit_coreutil_tableiterkey(&AS_MAP(instance)->values, index);
}

static LitValue lit_objfn_map_foreach(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    LitValue callback = args[0];

    if(!IS_CALLABLE_FUNCTION(callback))
    {
        lit_vm_raisefatalerror(vm, "Expected a function as the callback");
    }

    LitTable* values = &AS_MAP(instance)->values;

    for(int i = 0; i < values->capacity; i++)
    {
        LitTableEntry* entry = &values->entries[i];

        if(entry->key != NULL)
        {
            lit_state_callvalue(vm->state, callback, (LitValue[2]){ OBJECT_VALUE(entry->key), entry->value }, 2);
        }
    }

    return NULL_VALUE;
}

static LitValue lit_objfn_map_clone(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitState* state = vm->state;
    LitMap* map = lit_object_makemap(state);

    lit_table_add_all(state, &AS_MAP(instance)->values, &map->values);

    return OBJECT_VALUE(map);
}

static LitValue lit_objfn_map_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
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
        LitTableEntry* entry = &values->entries[index++];

        if(entry->key != NULL)
        {
            // Special hidden key
            LitValue field = haswrapper ? map->index_fn(vm, map, entry->key, NULL) : entry->value;
            // This check is required to prevent infinite loops when playing with Module.privates and such
            LitString* value = (IS_MAP(field) && AS_MAP(field)->index_fn != NULL) ? CONST_STRING(state, "map") : lit_tostring_value(state, field, indentation);

            lit_state_pushroot(state, (LitObject*)value);

            if(IS_STRING(field))
            {
                value = AS_STRING(lit_string_format(state, "\"@\"", OBJECT_VALUE(value)));
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
    return OBJECT_VALUE(lit_string_copy(vm->state, buffer, slength));
}

static LitValue lit_objfn_map_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(AS_MAP(instance)->values.count);
}

/*
 * Range
 */

static LitValue lit_objfn_range_iterator(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    LitRange* range = AS_RANGE(instance);
    int number = range->from;

    if(IS_NUMBER(args[0]))
    {
        number = AS_NUMBER(args[0]);

        if(range->to > range->from ? number >= range->to : number <= range->to)
        {
            return NULL_VALUE;
        }

        number += (range->from - range->to) > 0 ? -1 : 1;
    }

    return NUMBER_VALUE(number);
}

static LitValue lit_objfn_range_itervalue(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)
    return args[0];
}

static LitValue lit_objfn_range_tostring(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitRange* range = AS_RANGE(instance);
    return OBJECT_VALUE(lit_string_format(vm->state, "Range(#, #)", range->from, range->to));
}

static LitValue lit_objfn_range_from(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(AS_RANGE(instance)->from);
}

static LitValue lit_objfn_range_setfrom(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    AS_RANGE(instance)->from = AS_NUMBER(args[0]);
    return args[0];
}

static LitValue lit_objfn_range_to(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(AS_RANGE(instance)->to);
}

static LitValue lit_objfn_range_setto(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    AS_RANGE(instance)->to = AS_NUMBER(args[0]);
    return args[0];
}

static LitValue lit_objfn_range_length(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitRange* range = AS_RANGE(instance);
    return NUMBER_VALUE(range->to - range->from);
}

/*
 * Natives
 */

static LitValue lit_corefn_time(LitVm* vm, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
}

static LitValue lit_corefn_systemtime(LitVm* vm, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(time(NULL));
}

static LitValue lit_corefn_print(LitVm* vm, LitUInt argc, LitValue* args)
{
    LitString* s;
    if(argc == 0)
    {
        return NULL_VALUE;
    }
    for(LitUInt i = 0; i < argc; i++)
    {
        s = lit_tostring_value(vm->state, args[i], 0);
        lit_printf(vm->state, "%.*s", s->length, s->chars);
    }
    return NULL_VALUE;
}


static LitValue lit_corefn_println(LitVm* vm, LitUInt argc, LitValue* args)
{
    LitValue r;
    r = lit_corefn_print(vm, argc, args);
    lit_printf(vm->state, "\n");
    return r;
}

static LitValue lit_corefn_openlibrary(LitVm* vm, LitUInt argc, LitValue* args)
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

    return NULL_VALUE;
}

static bool interpret(LitVm* vm, LitModule* module)
{
    LitFunction* function = module->main_function;
    LitFiber* fiber = lit_object_makefiber(vm->state, module, function);

    fiber->parent = vm->fiber;
    vm->fiber = fiber;

    return true;
}

static bool lit_coreutil_compileandinterpret(LitVm* vm, LitString* modname, char* source)
{
    LitModule* module = lit_state_compilemodulesource(vm->state, modname, source);

    if(module == NULL)
    {
        return false;
    }

    module->ran = true;
    return interpret(vm, module);
}

static bool lit_corefn_eval(LitVm* vm, LitUInt argc, LitValue* args)
{
    char* code = (char*)LIT_CHECK_STRING(0);
    return lit_coreutil_compileandinterpret(vm, vm->fiber->module->name, code);
}

static bool lit_coreutil_fileexists(const char* filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}


void lit_state_opencorelibrary(LitState* state)
{
    {
        LIT_BEGIN_CLASS("Class");
        lit_class_bindmethod(state, klass, "toString", lit_objfn_class_tostring);
        lit_class_bindmethod(state, klass, "[]", lit_objfn_class_subscript);

        lit_class_bindstaticmethod(state, klass, "toString", lit_objfn_class_tostring);
        lit_class_bindstaticmethod(state, klass, "iterator", lit_objfn_class_iterator);
        lit_class_bindstaticmethod(state, klass, "iteratorValue", lit_objfn_class_itervalue);

        lit_class_bindgetsetter(state, klass, "super", lit_objfn_class_super, NULL);
        LIT_BIND_STATIC_GETTER("super", lit_objfn_class_super);
        LIT_BIND_STATIC_GETTER("name", lit_objfn_class_name);

        state->class_class = klass;
        LIT_END_CLASS_IGNORING();
    }
    {
        LIT_BEGIN_CLASS("Object");
        lit_class_inherit(state, klass, state->class_class);

        lit_class_bindmethod(state, klass, "toString", lit_objfn_object_tostring);
        lit_class_bindmethod(state, klass, "[]", lit_objfn_object_subscript);
        lit_class_bindmethod(state, klass, "iterator", lit_objfn_object_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", lit_objfn_object_itervalue);
        lit_class_bindgetsetter(state, klass, "class", lit_objfn_object_class, NULL);

        state->object_class = klass;
        state->object_class->super = state->class_class;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Number");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);
        lit_class_bindgetsetter(state, klass, "chr", lit_objfn_number_chr, NULL);
        lit_class_bindmethod(state, klass, "toString", lit_objfn_number_tostring);
        state->number_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("String");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);

        lit_class_bindmethod(state, klass, "+", lit_objfn_string_plus);
        //lit_class_bindmethod(state, klass, "<", lit_objfn_string_less);
        //lit_class_bindmethod(state, klass, ">", lit_objfn_string_greater);
        //lit_class_bindmethod(state, klass, "==", lit_objfn_string_compare);
        lit_class_bindmethod(state, klass, "toString", lit_objfn_string_tostring);
        lit_class_bindmethod(state, klass, "toNumber", lit_objfn_string_tonumber);
        lit_class_bindmethod(state, klass, "toUpperCase", lit_objfn_string_touppercase);
        lit_class_bindmethod(state, klass, "toLowerCase", lit_objfn_string_tolowercase);
        lit_class_bindmethod(state, klass, "contains", lit_objfn_string_contains);
        lit_class_bindmethod(state, klass, "startsWith", lit_objfn_string_startswith);
        lit_class_bindmethod(state, klass, "endsWith", lit_objfn_string_endswith);
        lit_class_bindmethod(state, klass, "replace", lit_objfn_string_replace);
        lit_class_bindmethod(state, klass, "substring", lit_objfn_string_substring);
        lit_class_bindmethod(state, klass, "iterator", lit_objfn_string_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", lit_objfn_string_itervalue);
        lit_class_bindmethod(state, klass, "[]", lit_objfn_string_subscript);
        lit_class_bindmethod(state, klass, "charCodeAt", lit_objfn_string_subscript);

        lit_class_bindgetsetter(state, klass, "length", lit_objfn_string_length, NULL);

        state->string_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Bool");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);

        lit_class_bindmethod(state, klass, "toString", lit_objfn_bool_tostring);
        state->bool_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Function");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);

        lit_class_bindmethod(state, klass, "toString", lit_objfn_function_tostring);
        lit_class_bindgetsetter(state, klass, "name", lit_objfn_function_name, NULL);

        state->function_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Fiber");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_fiber_constructor);

        lit_class_bindprimitive(state, klass, "run", lit_objfn_fiber_run);
        lit_class_bindprimitive(state, klass, "try", lit_objfn_fiber_try);
        lit_class_bindgetsetter(state, klass, "done", lit_objfn_fiber_done, NULL);
        lit_class_bindgetsetter(state, klass, "error", lit_objfn_fiber_error, NULL);

        lit_class_bindstaticprimitive(state, klass, "yield", lit_objfn_fiber_yield);
        lit_class_bindstaticprimitive(state, klass, "yeet", lit_objfn_fiber_yeet);
        lit_class_bindstaticprimitive(state, klass, "abort", lit_objfn_fiber_abort);
        LIT_BIND_STATIC_GETTER("current", lit_objfn_fiber_current);

        state->fiber_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Module");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);

        lit_class_setstaticfield(state, klass, "loaded", OBJECT_VALUE(state->vm->modules));
        LIT_BIND_STATIC_GETTER("privates", lit_objfn_module_privates);
        LIT_BIND_STATIC_GETTER("current", lit_objfn_module_current);

        lit_class_bindmethod(state, klass, "toString", lit_objfn_module_toString);
        lit_class_bindgetsetter(state, klass, "name", lit_objfn_module_name, NULL);
        lit_class_bindgetsetter(state, klass, "privates", lit_objfn_module_privates, NULL);

        state->module_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Array");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_array_constructor);
        lit_class_bindmethod(state, klass, "[]", lit_objfn_array_subscript);
        lit_class_bindmethod(state, klass, "add", lit_objfn_array_add);
        lit_class_bindmethod(state, klass, "insert", lit_objfn_array_insert);
        lit_class_bindmethod(state, klass, "slice", lit_objfn_array_slice);
        lit_class_bindmethod(state, klass, "addAll", lit_objfn_array_addall);
        lit_class_bindmethod(state, klass, "remove", lit_objfn_array_remove);
        lit_class_bindmethod(state, klass, "lit_coreutil_removeat", lit_objfn_array_removeat);
        lit_class_bindmethod(state, klass, "lit_coreutil_indexof", lit_objfn_array_indexof);
        lit_class_bindmethod(state, klass, "contains", lit_objfn_array_contains);
        lit_class_bindmethod(state, klass, "clear", lit_objfn_array_clear);
        lit_class_bindmethod(state, klass, "iterator", lit_objfn_array_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", lit_objfn_array_itervalue);
        lit_class_bindmethod(state, klass, "forEach", lit_objfn_array_foreach);
        lit_class_bindmethod(state, klass, "join", lit_objfn_array_join);
        lit_class_bindmethod(state, klass, "sort", lit_objfn_array_sort);
        lit_class_bindmethod(state, klass, "clone", lit_objfn_array_clone);
        lit_class_bindmethod(state, klass, "toString", lit_objfn_array_tostring);
        lit_class_bindgetsetter(state, klass, "length", lit_objfn_array_length, NULL);
        state->array_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Map");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_map_constructor);

        lit_class_bindmethod(state, klass, "[]", lit_objfn_map_subscript);
        lit_class_bindmethod(state, klass, "addAll", lit_objfn_map_addall);
        lit_class_bindmethod(state, klass, "clear", lit_objfn_map_clear);
        lit_class_bindmethod(state, klass, "iterator", lit_objfn_map_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", lit_objfn_map_itervalue);
        lit_class_bindmethod(state, klass, "forEach", lit_objfn_map_foreach);
        lit_class_bindmethod(state, klass, "clone", lit_objfn_map_clone);
        lit_class_bindmethod(state, klass, "toString", lit_objfn_map_tostring);

        lit_class_bindgetsetter(state, klass, "length", lit_objfn_map_length, NULL);

        state->map_class = klass;
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Range");
        lit_class_inherit(state, klass, state->object_class);
        lit_class_bindconstructor(state, klass, lit_objfn_invalidconstructor);

        lit_class_bindmethod(state, klass, "iterator", lit_objfn_range_iterator);
        lit_class_bindmethod(state, klass, "iteratorValue", lit_objfn_range_itervalue);
        lit_class_bindmethod(state, klass, "toString", lit_objfn_range_tostring);

        lit_class_bindgetsetter(state, klass, "from", lit_objfn_range_from, lit_objfn_range_setfrom);
        lit_class_bindgetsetter(state, klass, "to", lit_objfn_range_to, lit_objfn_range_setto);
        lit_class_bindgetsetter(state, klass, "length", lit_objfn_range_length, NULL);

        state->range_class = klass;
        LIT_END_CLASS();
    }
    lit_state_defnative(state, "time", lit_corefn_time);
    lit_state_defnative(state, "systemTime", lit_corefn_systemtime);
    lit_state_defnative(state, "print", lit_corefn_print);
    lit_state_defnative(state, "println", lit_corefn_println);
    lit_state_defnative(state, "openLibrary", lit_corefn_openlibrary);

    lit_state_defnativeprimitive(state, "eval", lit_corefn_eval);

    lit_state_setglobal(state, CONST_STRING(state, "globals"), OBJECT_VALUE(state->vm->globals));
}