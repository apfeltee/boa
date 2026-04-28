
#include <stdio.h>
#include "lit.h"

double lit_value_to_number(LitValue value)
{
    return *((double*)&value);
}

LitValue lit_number_to_value(double num)
{
    return *((LitValue*)&num);
}

bool lit_is_falsey(LitValue value)
{
    return (
        (IS_BOOL(value) && (value == FALSE_VALUE)) ||
        IS_NULL(value) ||
        (IS_NUMBER(value) && AS_NUMBER(value) == 0)
    );
}


static void print_object(LitValue value)
{
    switch(OBJECT_TYPE(value))
    {
        case LIT_OBJ_STRING:
        {
            printf("%s", AS_CSTRING(value));
            break;
        }

        case LIT_OBJ_FUNCTION:
        {
            printf("function %s", AS_FUNCTION(value)->name->chars);
            break;
        }

        case LIT_OBJ_CLOSURE:
        {
#ifdef LIT_TRACE_STACK
            printf("closure %s", AS_CLOSURE(value)->function->name->chars);
#else
            printf("function %s", AS_CLOSURE(value)->function->name->chars);
#endif
            break;
        }

        case LIT_OBJ_CLOSUREPROTOTYPE:
        {
#ifdef LIT_TRACE_STACK
            printf("closure %s", AS_CLOSURE_PROTOTYPE(value)->function->name->chars);
#else
            printf("function %s", AS_CLOSURE_PROTOTYPE(value)->function->name->chars);
#endif
            break;
        }

        case LIT_OBJ_NATIVEPRIMITIVE:
        {
            printf("function %s", AS_NATIVE_PRIMITIVE(value)->name->chars);
            break;
        }

        case LIT_OBJ_NATIVEFUNCTION:
        {
            printf("function %s", AS_NATIVE_FUNCTION(value)->name->chars);
            break;
        }

        case LIT_OBJ_PRIMITIVEMETHOD:
        {
            printf("function %s", AS_PRIMITIVE_METHOD(value)->name->chars);
            break;
        }

        case LIT_OBJ_NATIVEMETHOD:
        {
            printf("function %s", AS_NATIVE_METHOD(value)->name->chars);
            break;
        }

        case LIT_OBJ_FIBER:
        {
            printf("fiber");
            break;
        }

        case LIT_OBJ_MODULE:
        {
            printf("module %s", AS_MODULE(value)->name->chars);
            break;
        }

        case LIT_OBJ_UPVALUE:
        {
            LitUpvalue* upvalue = AS_UPVALUE(value);

            if(upvalue->location == NULL)
            {
                lit_print_value(upvalue->closed);
            }
            else
            {
                print_object(*upvalue->location);
            }

            break;
        }

        case LIT_OBJ_CLASS:
        {
            printf("class %s", AS_CLASS(value)->name->chars);
            break;
        }

        case LIT_OBJ_INSTANCE:
        {
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        }

        case LIT_OBJ_BOUNDMETHOD:
        {
            lit_print_value(AS_BOUND_METHOD(value)->method);
            return;
        }

        case LIT_OBJ_VARARGARRAY:
        case LIT_OBJ_ARRAY:
        {
#ifdef LIT_MINIMIZE_CONTAINERS
            printf("array");
#else
            LitArray* array = AS_ARRAY(value);
            LitUInt size = array->values.count;

            printf("[");

            if(size > 32)
            {
                printf(" (too big to be displayed) ");
            }
            else if(size > 0)
            {
                printf(" ");

                for(LitUInt i = 0; i < size; i++)
                {
                    lit_print_value(array->values.values[i]);

                    if(i + 1 < size)
                    {
                        printf(", ");
                    }
                    else
                    {
                        printf(" ");
                    }
                }
            }

            printf("]");
#endif
            break;
        }

        case LIT_OBJ_MAP:
        {
#ifdef LIT_MINIMIZE_CONTAINERS
            printf("map");
#else
            LitMap* map = AS_MAP(value);
            LitUInt size = map->values.count;
            printf("{");
            bool hadbefore = false;

            if(size > 16)
            {
                printf(" (too big to be displayed) ");
            }
            else if(size > 0)
            {
                for(int i = 0; i < map->values.capacity; i++)
                {
                    LitTableEntry* entry = &map->values.entries[i];

                    if(entry->key != NULL)
                    {
                        if(hadbefore)
                        {
                            printf(", ");
                        }
                        else
                        {
                            printf(" ");
                        }

                        printf("%s: ", entry->key->chars);
                        lit_print_value(entry->value);
                        hadbefore = true;
                    }
                }
            }

            if(hadbefore)
            {
                printf(" }");
            }
            else
            {
                printf("}");
            }
#endif
            break;
        }

        case LIT_OBJ_USERDATA:
        {
            printf("userdata");
            break;
        }

        case LIT_OBJ_RANGE:
        {
            LitRange* range = AS_RANGE(value);
            printf("%g .. %g", range->from, range->to);

            break;
        }

        case LIT_OBJ_FIELD:
        {
            printf("field");
            break;
        }

        case LIT_OBJ_REFERENCE:
        {
            printf("reference => ");
            LitValue* slot = AS_REFERENCE(value)->slot;

            if(slot == NULL)
            {
                printf("null");
            }
            else
            {
                lit_print_value(*slot);
            }

            break;
        }

        default:
        {
            printf("[unknown object %lu %i]", value, OBJECT_TYPE(value));
            break;
        }
    }
}

void lit_print_value(LitValue value)
{
    if(IS_BOOL(value))
    {
        printf(AS_BOOL(value) ? "true" : "false");
    }
    else if(IS_NULL(value))
    {
        printf("null");
    }
    else if(IS_NUMBER(value))
    {
        printf("%g", AS_NUMBER(value));
    }
    else if(IS_OBJECT(value))
    {
        print_object(value);
    }
    else
    {
        printf("[unknown value %lu]", value);
    }
}

void lit_values_ensure_size(LitState* state, LitValList* values, LitUInt size)
{
    lit_values_ensure_size_empty(state, values, size);

    if(values->count < size)
    {
        values->count = size;
    }
}

void lit_values_ensure_size_empty(LitState* state, LitValList* values, LitUInt size)
{
    if(values->capacity < size)
    {
        LitUInt oldcapacity = values->capacity;
        values->capacity = size;
        values->values = LIT_GROW_ARRAY(state, values->values, LitValue, oldcapacity, size);

        for(LitUInt i = oldcapacity; i < size; i++)
        {
            values->values[i] = NULL_VALUE;
        }
    }
}

const char* lit_get_value_type(LitValue value)
{
    if(IS_BOOL(value))
    {
        return "bool";
    }
    else if(IS_NULL(value))
    {
        return "null";
    }
    else if(IS_NUMBER(value))
    {
        return "number";
    }
    else if(IS_OBJECT(value))
    {
        return lit_tostring_typename(OBJECT_TYPE(value));
    }

    return "unknown";
}