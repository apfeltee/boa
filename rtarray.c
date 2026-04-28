
#include "lit.h"

void lit_uintlist_init(LitUIntList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_uintlist_destroy(LitState* state, LitUIntList* array)
{
    LIT_FREE_ARRAY(state, LitUInt, array->values, array->capacity);
    lit_uintlist_init(array);
}
void lit_uintlist_push(LitState* state, LitUIntList* array, LitUInt value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitUInt, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}


void lit_bytelist_init(LitByteList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_bytelist_destroy(LitState* state, LitByteList* array)
{
    LIT_FREE_ARRAY(state, uint8_t, array->values, array->capacity);
    lit_bytelist_init(array);
}
void lit_bytelist_push(LitState* state, LitByteList* array, uint8_t value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, uint8_t, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void lit_vallist_init(LitValList* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}
void lit_vallist_destroy(LitState* state, LitValList* array)
{
    LIT_FREE_ARRAY(state, LitValue, array->values, array->capacity);
    lit_vallist_init(array);
}
void lit_vallist_push(LitState* state, LitValList* array, LitValue value)
{
    if(array->capacity < array->count + 1)
    {
        LitUInt oldcapacity = array->capacity;
        array->capacity = LIT_GROW_CAPACITY(oldcapacity);
        array->values = LIT_GROW_ARRAY(state, array->values, LitValue, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}
