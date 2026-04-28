
#include "lit.h"

void lit_class_inherit(LitState* state, LitClass* selfclass, LitClass* other)
{
    selfclass->super = (LitClass*)other;
    if(selfclass->init_method == NULL)
    {
        selfclass->init_method = other->init_method;
    }
    lit_table_add_all_ignoring(state, &other->methods, &selfclass->methods);
    lit_table_add_all_ignoring(state, &other->static_fields, &selfclass->static_fields);
}

void lit_class_bindmethod(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->methods, nm, OBJECT_VALUE(lit_object_makenativemethod(state, fn, nm))); \
}


void lit_class_bindprimitive(LitState* state, LitClass* selfclass, const char* name, LitPrimitiveMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->methods, nm, OBJECT_VALUE(lit_object_makeprimitivemethod(state, fn, nm))); \
}

void lit_class_bindconstructor(LitState* state, LitClass* selfclass, LitNativeMethodFn fn)
{
    const char* fname;
    fname = "constructor";
    LitString* nm;
    LitNativeMethod* meth;
    nm = lit_string_copy(state, fname, strlen(fname));
    meth = lit_object_makenativemethod(state, fn, nm);
    selfclass->init_method = (LitObject*)meth;
    lit_table_set(state, &selfclass->methods, nm, OBJECT_VALUE(meth));
}

void lit_class_bindstaticmethod(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, OBJECT_VALUE(lit_object_makenativemethod(state, fn, nm)));
}

void lit_class_bindstaticprimitive(LitState* state, LitClass* selfclass, const char* name, LitPrimitiveMethodFn fn)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, OBJECT_VALUE(lit_object_makeprimitivemethod(state, fn, nm)));
}

void lit_class_setstaticfield(LitState* state, LitClass* selfclass, const char* name, LitValue val)
{
    LitString* nm;
    nm = lit_string_copy(state, name, strlen(name));
    lit_table_set(state, &selfclass->static_fields, nm, val);
}

void lit_class_bindgetsetter(LitState* state, LitClass* selfclass, const char* name, LitNativeMethodFn fnget, LitNativeMethodFn fnset)
{
    LitString* nm;
    LitObject* mthset;
    LitObject* mthget;
    mthset = NULL;
    mthget = NULL;
    nm = lit_string_copy(state, name, strlen(name));
    if(fnget)
    {
        mthget = (LitObject*)lit_object_makenativemethod(state, fnget, nm);
    }
    if(fnset)
    {
        mthset = (LitObject*)lit_object_makenativemethod(state, fnset, nm);
    }
    lit_table_set(state, &selfclass->methods, nm, OBJECT_VALUE(lit_object_makefield(state, mthget, mthset)));
}
