

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lit.h"

static bool measurecompilationtime;
static double lastsourcetime = 0;

void lit_state_enablecompilationtimemeasurement()
{
    measurecompilationtime = true;
}

static void lit_state_defaulthandleerror(LitState* state, const char* message)
{
    fflush(stdout);
    fprintf(stderr, "%s%s%s\n", COLOR_RED, message, COLOR_RESET);
    fflush(stderr);
}

static void lit_state_defaulthandleprintf(LitState* state, const char* message)
{
    printf("%s", message);
}

LitState* lit_state_make()
{
    LitState* state = (LitState*)malloc(sizeof(LitState));

    state->class_class = NULL;
    state->object_class = NULL;
    state->number_class = NULL;
    state->string_class = NULL;
    state->bool_class = NULL;
    state->function_class = NULL;
    state->fiber_class = NULL;
    state->module_class = NULL;
    state->array_class = NULL;
    state->map_class = NULL;
    state->range_class = NULL;

    state->bytes_allocated = 0;

    state->next_gc = 256 * 1024;
    state->allow_gc = false;

    state->error_fn = lit_state_defaulthandleerror;
    state->print_fn = lit_state_defaulthandleprintf;
    state->had_error = false;
    state->roots = NULL;
    state->root_count = 0;
    state->root_capacity = 0;
    state->last_module = NULL;
    state->config.traceexecution = false;
    state->config.tracechunk = false;

    state->scanner = (LitScanner*)malloc(sizeof(LitScanner));

    state->parser = (LitParser*)malloc(sizeof(LitParser));
    lit_parser_init(state, (LitParser*)state->parser);

    state->emitter = (LitEmitter*)malloc(sizeof(LitEmitter));
    lit_emitter_init(state, state->emitter);


    state->event_system = (LitEventSystem*)malloc(sizeof(LitEventSystem));
    lit_eventsystem_init(state, state->event_system);

    state->vm = (LitVm*)malloc(sizeof(LitVm));

    lit_init_vm(state, state->vm);
    lit_api_init(state);
    lit_state_opencorelibrary(state);

    return state;
}

int64_t lit_state_destroy(LitState* state)
{
    if(state->roots != NULL)
    {
        free(state->roots);
        state->roots = NULL;
    }

    lit_api_destroy(state);

    lit_eventsystem_destroy(state->event_system);
    free(state->event_system);


    free(state->scanner);

    lit_parser_destroy(state->parser);
    free(state->parser);

    lit_emitter_destroy(state->emitter);
    free(state->emitter);


    lit_free_vm(state->vm);
    free(state->vm);

    int64_t amount = state->bytes_allocated;
    free(state);

    return amount;
}

void lit_state_pushroot(LitState* state, LitObject* object)
{
    lit_state_pushvalueroot(state, OBJECT_VALUE(object));
}

void lit_state_pushvalueroot(LitState* state, LitValue value)
{
    if(state->root_count + 1 >= state->root_capacity)
    {
        state->root_capacity = LIT_GROW_CAPACITY(state->root_capacity);
        state->roots = realloc(state->roots, state->root_capacity * sizeof(LitValue));
    }

    state->roots[state->root_count++] = value;
}

LitValue lit_state_peekroot(LitState* state, uint8_t distance)
{
    assert(state->root_count - distance + 1 > 0);
    return state->roots[state->root_count - distance - 1];
}

void lit_state_poproot(LitState* state)
{
    state->root_count--;
}

void lit_state_poproots(LitState* state, uint8_t amount)
{
    state->root_count -= amount;
}


LitClass* lit_state_getclassfor(LitState* state, LitValue value)
{
    if(IS_OBJECT(value))
    {
        switch(OBJECT_TYPE(value))
        {
            case LIT_OBJ_STRING:
                return state->string_class;
            case LIT_OBJ_USERDATA:
                return state->object_class;

            case LIT_OBJ_FIELD:
            case LIT_OBJ_FUNCTION:
            case LIT_OBJ_CLOSURE:
            case LIT_OBJ_CLOSUREPROTOTYPE:
            case LIT_OBJ_NATIVEFUNCTION:
            case LIT_OBJ_NATIVEPRIMITIVE:
            case LIT_OBJ_BOUNDMETHOD:
            case LIT_OBJ_PRIMITIVEMETHOD:
            case LIT_OBJ_NATIVEMETHOD:
            {
                return state->function_class;
            }

            case LIT_OBJ_FIBER:
                return state->fiber_class;
            case LIT_OBJ_MODULE:
                return state->module_class;
            case LIT_OBJ_UPVALUE:
            {
                LitUpvalue* upvalue = AS_UPVALUE(value);

                if(upvalue->location == NULL)
                {
                    return lit_state_getclassfor(state, upvalue->closed);
                }

                return lit_state_getclassfor(state, *upvalue->location);
            }

            case LIT_OBJ_INSTANCE:
                return AS_INSTANCE(value)->klass;
            case LIT_OBJ_CLASS:
                return state->class_class;
            case LIT_OBJ_ARRAY:
            case LIT_OBJ_VARARGARRAY:
                return state->array_class;
            case LIT_OBJ_MAP:
                return state->map_class;
            case LIT_OBJ_RANGE:
                return state->range_class;

            case LIT_OBJ_REFERENCE:
            {
                LitValue* slot = AS_REFERENCE(value)->slot;

                if(slot != NULL)
                {
                    return lit_state_getclassfor(state, *slot);
                }

                return state->object_class;
            }
        }
    }
    else if(IS_NUMBER(value))
    {
        return state->number_class;
    }
    else if(IS_BOOL(value))
    {
        return state->bool_class;
    }

    return NULL;
}

static void relstmts(LitState* state, LitExprList* statements)
{
    for(LitUInt i = 0; i < statements->count; i++)
    {
        lit_ast_destroystmt(state, statements->values[i]);
    }

    lit_free_stmtlist_destroy(state, statements);
}

LitResult lit_state_interpretsource(LitState* state, const char* modname, char* code)
{
    return lit_state_interninterpretsource(state, lit_string_copy(state, modname, strlen(modname)), code);
}

LitModule* lit_state_compilemodulesource(LitState* state, LitString* modname, char* code)
{
    bool allowedgc = state->allow_gc;

    state->allow_gc = false;
    state->had_error = false;

    LitModule* module = NULL;

    // This is a lbc format
    if((code[1] << 8 | code[0]) == LIT_BYTECODE_MAGIC_NUMBER)
    {
        module = lit_load_module(state, code);
    }
    else
    {
        clock_t t = 0;
        clock_t total_t = 0;

        if(measurecompilationtime)
        {
            total_t = t = clock();
        }


        LitExprList statements;
        lit_stmtlist_init(&statements);

        if(lit_parser_parsesource(state->parser, modname->chars, code, &statements))
        {
            relstmts(state, &statements);
            return NULL;
        }

        if(measurecompilationtime)
        {
            printf("Parsing:        %gms\n", (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
            t = clock();
        }


        module = lit_emitter_emitmod(state->emitter, &statements, modname);
        relstmts(state, &statements);

        if(measurecompilationtime)
        {
            printf("Emitting:       %gms\n", (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
            printf("\nTotal:          %gms\n-----------------------\n", (double)(clock() - total_t) / CLOCKS_PER_SEC * 1000 + lastsourcetime);
        }
    }

    state->allow_gc = allowedgc;
    return state->had_error ? NULL : module;
}

LitModule* lit_state_getmodulebyname(LitState* state, const char* name)
{
    LitValue value;

    if(lit_table_get(&state->vm->modules->values, CONST_STRING(state, name), &value))
    {
        return AS_MODULE(value);
    }

    return NULL;
}

LitResult lit_state_interninterpretsource(LitState* state, LitString* modname, char* code)
{
    LitModule* module = lit_state_compilemodulesource(state, modname, code);

    if(module == NULL)
    {
        return (LitResult){ INTERPRET_COMPILE_ERROR, NULL_VALUE };
    }

    LitResult result = lit_interpret_module(state, module);
    state->last_module = module;

    return result;
}

char* lit_util_patchfilename(char* file_name)
{
    int namelength = strlen(file_name);

    // Check, if our file_name ends with .lit or lbc, and remove it
    if(namelength > 4 && (memcmp(file_name + namelength - 4, ".lit", 4) == 0 || memcmp(file_name + namelength - 4, ".lbc", 4) == 0))
    {
        file_name[namelength - 4] = '\0';
        namelength -= 4;
    }

    // Check, if our file_name starts with ./ and remove it (useless, and makes the module name be ..main)
    if(namelength > 2 && memcmp(file_name, "./", 2) == 0)
    {
        file_name += 2;
        namelength -= 2;
    }

    for(int i = 0; i < namelength; i++)
    {
        char c = file_name[i];

        if(c == '/' || c == '\\')
        {
            file_name[i] = '.';
        }
    }

    return file_name;
}

char* lit_util_dupstring(const char* string)
{
    size_t length = strlen(string) + 1;
    char* newstring = malloc(length);
    memcpy(newstring, string, length);

    return newstring;
}

bool lit_state_compileandsavefiles(LitState* state, char* files[], LitUInt numfiles, const char* outputfile)
{
    LitModule* compiledmodules[numfiles];

    for(LitUInt i = 0; i < numfiles; i++)
    {
        char* file_name = lit_util_dupstring(files[i]);
        char* source = lit_read_file(file_name);

        if(source == NULL)
        {
            lit_state_raiseerror(state, COMPILE_ERROR, "Failed to open file '%s'", file_name);
            return false;
        }

        file_name = lit_util_patchfilename(file_name);

        LitString* modname = lit_string_copy(state, file_name, strlen(file_name));
        LitModule* module = lit_state_compilemodulesource(state, modname, source);

        compiledmodules[i] = module;

        free((void*)source);
        free((void*)file_name);

        if(module == NULL)
        {
            return false;
        }
    }

    FILE* file = fopen(outputfile, "w+b");

    if(file == NULL)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to open for writing file '%s'", outputfile);
        return false;
    }

    lit_write_uint16_t(file, LIT_BYTECODE_MAGIC_NUMBER);
    lit_write_uint8_t(file, LIT_BYTECODE_VERSION);
    lit_write_uint16_t(file, numfiles);

    for(LitUInt i = 0; i < numfiles; i++)
    {
        lit_save_module(compiledmodules[i], file);
    }

    lit_write_uint16_t(file, LIT_BYTECODE_END_NUMBER);
    fclose(file);

    return true;
}

static char* lit_util_readsource(LitState* state, const char* file, char** patchedfilename)
{
    clock_t t = 0;

    if(measurecompilationtime)
    {
        t = clock();
    }

    char* file_name = lit_util_dupstring(file);
    char* source = lit_read_file(file_name);

    if(source == NULL)
    {
        lit_state_raiseerror(state, RUNTIME_ERROR, "Failed to open file '%s'", file_name);
    }

    file_name = lit_util_patchfilename(file_name);

    if(measurecompilationtime)
    {
        printf("Reading source: %gms\n", lastsourcetime = (double)(clock() - t) / CLOCKS_PER_SEC * 1000);
    }

    *patchedfilename = file_name;
    return source;
}

LitResult lit_state_interpretfile(LitState* state, const char* file)
{
    char* patchedfilename;
    char* source = lit_util_readsource(state, file, &patchedfilename);

    if(source == NULL)
    {
        return INTERPRET_RUNTIME_FAIL;
    }

    LitResult result = lit_state_interpretsource(state, patchedfilename, source);
    free(patchedfilename);
    free((void*)source);
    return result;
}

LitResult lit_state_dumpfile(LitState* state, const char* file)
{
    char* patchedfilename;
    char* source = lit_util_readsource(state, file, &patchedfilename);

    if(source == NULL)
    {
        return INTERPRET_RUNTIME_FAIL;
    }

    LitResult result;
    LitString* modname = lit_string_copy(state, patchedfilename, strlen(patchedfilename));
    LitModule* module = lit_state_compilemodulesource(state, modname, source);

    if(module == NULL)
    {
        result = INTERPRET_RUNTIME_FAIL;
    }
    else
    {
        lit_debug_disasmodule(module, source);
        result = (LitResult){ INTERPRET_OK, NULL_VALUE };
    }

    free((void*)source);
    free((void*)patchedfilename);

    return result;
}

void lit_state_raiseerror(LitState* state, LitErrorType type, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, message, argscopy) + 1;
    va_end(argscopy);

    char buffer[buffersize];
    vsnprintf(buffer, buffersize, message, args);
    va_end(args);

    state->error_fn(state, buffer);
    state->had_error = true;
}

void lit_printf(LitState* state, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    va_list argscopy;
    va_copy(argscopy, args);
    size_t buffersize = vsnprintf(NULL, 0, message, argscopy) + 1;
    va_end(argscopy);

    char buffer[buffersize];
    vsnprintf(buffer, buffersize, message, args);
    va_end(args);

    state->print_fn(state, buffer);
}