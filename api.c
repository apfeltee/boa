
#include <string.h>
#include <math.h>
#include "lit.h"


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
        return NULL_VALUE;
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

double lit_args_checknumber(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_NUMBER(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a number as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_NUMBER(args[id]);
}

double lit_args_getnumber(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id, double def)
{
    if(arg_count <= id || !IS_NUMBER(args[id]))
    {
        return def;
    }

    return AS_NUMBER(args[id]);
}

bool lit_args_checkbool(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_BOOL(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a boolean as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_BOOL(args[id]);
}

bool lit_args_getbool(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id, bool def)
{
    if(arg_count <= id || !IS_BOOL(args[id]))
    {
        return def;
    }

    return AS_BOOL(args[id]);
}

const char* lit_args_checkstring(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a string as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_STRING(args[id])->chars;
}

const char* lit_args_getstring(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id, const char* def)
{
    if(arg_count <= id || !IS_STRING(args[id]))
    {
        return def;
    }

    return AS_STRING(args[id])->chars;
}

LitString* lit_args_checkobjstring(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_STRING(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a string as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_STRING(args[id]);
}

LitInstance* lit_args_checkinstance(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_INSTANCE(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected an instance as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_INSTANCE(args[id]);
}

LitValue* lit_check_reference(LitVm* vm, LitValue* args, uint8_t arg_count, uint8_t id)
{
    if(arg_count <= id || !IS_REFERENCE(args[id]))
    {
        lit_vm_raisefatalerror(vm, "Expected a reference as argument #%i, got a %s", (int)id, id >= arg_count ? "null" : lit_get_value_type(args[id]));
    }

    return AS_REFERENCE(args[id])->slot;
}

void lit_args_ensurebool(LitVm* vm, LitValue value, const char* error)
{
    if(!IS_BOOL(value))
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
    if(!IS_NUMBER(value))
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

void lit_args_ensureobjtype(LitVm* vm, LitValue value, LitObjectType type, const char* error)
{
    if(!IS_OBJECT(value) || OBJECT_TYPE(value) != type)
    {
        lit_vm_raisefatalerror(vm, error);
    }
}

LitValue lit_table_getfield(LitState* state, LitTable* table, const char* name)
{
    LitValue value;

    if(!lit_table_get(table, CONST_STRING(state, name), &value))
    {
        value = NULL_VALUE;
    }

    return value;
}

LitValue lit_map_getfield(LitState* state, LitMap* map, const char* name)
{
    LitValue value;

    if(!lit_table_get(&map->values, CONST_STRING(state, name), &value))
    {
        value = NULL_VALUE;
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