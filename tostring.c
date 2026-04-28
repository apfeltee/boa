
#include "lit.h"

const char* lit_tostring_typename(LitObjectType t)
{
    
    static const char* lit_object_type_names[] =
    {
        "string",        "function",          "nativefunction", "nativeprimitive",
        "nativemethod", "primitivemethod",  "fiber",           "module",
        "closure",       "clsproto", "upvalue",         "class",
        "instance",      "boundmethod",      "array",           "array",
        "map",           "userdata",          "range",           "field",
        "reference"
    };
    return lit_object_type_names[t];
}



