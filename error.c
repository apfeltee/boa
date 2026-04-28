
#include <stdio.h>
#include "lit.h"



LitString* lit_state_errorfmtv(LitState* state, LitUInt line, const char* fmt, va_list args)
{
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, fmt, argscopy) + 1;
    va_end(argscopy);
    char buffer[buffersize];
    vsnprintf(buffer, buffersize, fmt, args);
    buffer[buffersize - 1] = '\0';

    if(line != 0)
    {
        return AS_STRING(lit_string_format(state, "[line #]: $", (double)line, (const char*)buffer));
    }

    return AS_STRING(lit_string_format(state, "$", (const char*)buffer));
}

LitString* lit_state_errorfmt(LitState* state, LitUInt line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(state, line, fmt, args);
    va_end(args);
    return result;
}