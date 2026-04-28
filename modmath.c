
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "lit.h"

static LitValue math_abs(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(fabs(LIT_CHECK_NUMBER(0)));
}

static LitValue math_cos(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(cos(LIT_CHECK_NUMBER(0)));
}

static LitValue math_sin(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(sin(LIT_CHECK_NUMBER(0)));
}

static LitValue math_tan(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(tan(LIT_CHECK_NUMBER(0)));
}

static LitValue math_acos(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(acos(LIT_CHECK_NUMBER(0)));
}

static LitValue math_asin(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(asin(LIT_CHECK_NUMBER(0)));
}

static LitValue math_atan(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(atan(LIT_CHECK_NUMBER(0)));
}

static LitValue math_atan2(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(atan2(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

static LitValue math_floor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(floor(LIT_CHECK_NUMBER(0)));
}

static LitValue math_ceil(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(ceil(LIT_CHECK_NUMBER(0)));
}

static LitValue math_round(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    double value = LIT_CHECK_NUMBER(0);

    if(argc > 1)
    {
        int places = (int)pow(10, LIT_CHECK_NUMBER(1));
        return NUMBER_VALUE(round(value * places) / places);
    }

    return NUMBER_VALUE(round(value));
}

static LitValue math_min(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(fmin(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

static LitValue math_max(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(fmax(LIT_CHECK_NUMBER(0), LIT_CHECK_NUMBER(1)));
}

static LitValue math_mid(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    double x = LIT_CHECK_NUMBER(0);
    double y = LIT_CHECK_NUMBER(1);
    double z = LIT_CHECK_NUMBER(2);

    if(x > y)
    {
        return NUMBER_VALUE(fmax(x, fmin(y, z)));
    }
    else
    {
        return NUMBER_VALUE(fmax(y, fmin(x, z)));
    }
}

static LitValue math_toRadians(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(LIT_CHECK_NUMBER(0) * M_PI / 180.0);
}

static LitValue math_toDegrees(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(LIT_CHECK_NUMBER(0) * 180.0 / M_PI);
}

static LitValue math_sqrt(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(sqrt(LIT_CHECK_NUMBER(0)));
}

static LitValue math_log(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(exp(LIT_CHECK_NUMBER(0)));
}

static LitValue math_exp(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(exp(LIT_CHECK_NUMBER(0)));
}

/*
 * Random
 */

static LitUInt staticrandomdata;

static LitUInt* extract_random_data(LitState* state, LitValue instance)
{
    if(IS_CLASS(instance))
    {
        return &staticrandomdata;
    }

    LitValue data;

    if(!lit_table_get(&AS_INSTANCE(instance)->fields, CONST_STRING(state, "_data"), &data))
    {
        return 0;
    }

    return (LitUInt*)AS_USERDATA(data)->data;
}

static LitValue random_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUserdata* userdata = lit_object_makeuserdata(vm->state, sizeof(LitUInt));
    lit_table_set(vm->state, &AS_INSTANCE(instance)->fields, CONST_STRING(vm->state, "_data"), OBJECT_VALUE(userdata));

    LitUInt* data = (LitUInt*)userdata->data;

    if(argc == 1)
    {
        LitUInt number = (LitUInt)LIT_CHECK_NUMBER(0);
        *data = number;
    }
    else
    {
        int r = rand();
        *data = *((LitUInt*)&r);
    }

    return OBJECT_VALUE(instance);
}

static LitValue random_setSeed(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);

    if(argc == 1)
    {
        LitUInt number = (LitUInt)LIT_CHECK_NUMBER(0);
        *data = number;
    }
    else
    {
        *data = time(NULL);
    }

    return NULL_VALUE;
}

int custom_random(LitUInt* data)
{
    *data = (*data * 125) % 2796203;
    return *data;
}

static LitValue random_int(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);

    if(argc == 1)
    {
        int bound = (int)LIT_GET_NUMBER(0, 1);
        return NUMBER_VALUE(custom_random(data) % bound);
    }
    else if(argc == 2)
    {
        int min = (int)LIT_GET_NUMBER(0, 0);
        int max = (int)LIT_GET_NUMBER(1, 1);

        if(max - min == 0)
        {
            return NUMBER_VALUE(max);
        }

        return NUMBER_VALUE(min + custom_random(data) % (max - min));
    }

    return NUMBER_VALUE(custom_random(data));
}

static LitValue random_float(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt* data = extract_random_data(vm->state, instance);
    double value = (double)custom_random(data) / RAND_MAX;

    if(argc == 1)
    {
        int bound = (int)LIT_GET_NUMBER(0, 0);
        return NUMBER_VALUE(value * bound);
    }
    else if(argc == 2)
    {
        int min = (int)LIT_GET_NUMBER(0, 0);
        int max = (int)LIT_GET_NUMBER(1, 1);

        if(max - min == 0)
        {
            return NUMBER_VALUE(max);
        }

        return NUMBER_VALUE(min + value * (max - min));
    }

    return NUMBER_VALUE(value);
}

static LitValue random_bool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return BOOL_VALUE(custom_random(extract_random_data(vm->state, instance)) % 2);
}

static LitValue random_chance(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    float c = LIT_GET_NUMBER(0, 50);
    return BOOL_VALUE((((float)custom_random(extract_random_data(vm->state, instance))) / RAND_MAX * 100) <= c);
}

static LitValue random_pick(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    int value = custom_random(extract_random_data(vm->state, instance));

    if(argc == 1)
    {
        if(IS_ARRAY(args[0]))
        {
            LitArray* array = AS_ARRAY(args[0]);

            if(array->values.count == 0)
            {
                return NULL_VALUE;
            }

            return array->values.values[value % array->values.count];
        }
        else if(IS_MAP(args[0]))
        {
            LitMap* map = AS_MAP(args[0]);
            LitUInt length = map->values.count;
            LitUInt capacity = map->values.capacity;

            if(length == 0)
            {
                return NULL_VALUE;
            }

            LitUInt target = value % length;
            LitUInt index = 0;

            for(LitUInt i = 0; i < capacity; i++)
            {
                if(map->values.entries[i].key != NULL)
                {
                    if(index == target)
                    {
                        return map->values.entries[i].value;
                    }

                    index++;
                }
            }
        }
        else
        {
            lit_vm_raisefatalerror(vm, "Expected map or array as the argument");
        }
    }
    else
    {
        return args[value % argc];
    }

    return NULL_VALUE;
}

void lit_open_math_library(LitState* state)
{
    {
        LIT_BEGIN_CLASS("Math");
        lit_class_setstaticfield(state, klass, "Pi", NUMBER_VALUE(M_PI));
        lit_class_setstaticfield(state, klass, "Tau", NUMBER_VALUE(M_PI * 2));

        lit_class_bindstaticmethod(state, klass, "abs", math_abs);
        lit_class_bindstaticmethod(state, klass, "sin", math_sin);
        lit_class_bindstaticmethod(state, klass, "cos", math_cos);
        lit_class_bindstaticmethod(state, klass, "tan", math_tan);
        lit_class_bindstaticmethod(state, klass, "asin", math_asin);
        lit_class_bindstaticmethod(state, klass, "acos", math_acos);
        lit_class_bindstaticmethod(state, klass, "atan", math_atan);
        lit_class_bindstaticmethod(state, klass, "atan2", math_atan2);
        lit_class_bindstaticmethod(state, klass, "floor", math_floor);
        lit_class_bindstaticmethod(state, klass, "ceil", math_ceil);
        lit_class_bindstaticmethod(state, klass, "round", math_round);
        lit_class_bindstaticmethod(state, klass, "min", math_min);
        lit_class_bindstaticmethod(state, klass, "max", math_max);
        lit_class_bindstaticmethod(state, klass, "mid", math_mid);
        lit_class_bindstaticmethod(state, klass, "toRadians", math_toRadians);
        lit_class_bindstaticmethod(state, klass, "toDegrees", math_toDegrees);
        lit_class_bindstaticmethod(state, klass, "sqrt", math_sqrt);
        lit_class_bindstaticmethod(state, klass, "log", math_log);
        lit_class_bindstaticmethod(state, klass, "exp", math_exp);
        LIT_END_CLASS();
    }
    srand(time(NULL));

    int r = rand();
    staticrandomdata = *((LitUInt*)&r);
    {
        LIT_BEGIN_CLASS("Random");
        lit_class_bindconstructor(state, klass, random_constructor);

        lit_class_bindmethod(state, klass, "setSeed", random_setSeed);
        lit_class_bindmethod(state, klass, "int", random_int);
        lit_class_bindmethod(state, klass, "float", random_float);
        lit_class_bindmethod(state, klass, "chance", random_chance);
        lit_class_bindmethod(state, klass, "pick", random_pick);

        lit_class_bindstaticmethod(state, klass, "setSeed", random_setSeed);
        lit_class_bindstaticmethod(state, klass, "int", random_int);
        lit_class_bindstaticmethod(state, klass, "float", random_float);
        lit_class_bindstaticmethod(state, klass, "bool", random_bool);
        lit_class_bindstaticmethod(state, klass, "chance", random_chance);
        lit_class_bindstaticmethod(state, klass, "pick", random_pick);
        LIT_END_CLASS();
    }
}