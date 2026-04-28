
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include "lit.h"

#ifdef LIT_OS_WINDOWS
    #define stat _stat
#endif

typedef struct
{
    char* path;
    FILE* file;
} LitFileData;

static uint8_t btmp;
static uint16_t stmp;
static uint32_t itmp;
static double dtmp;

static void save_chunk(FILE* file, LitChunk* chunk);
static void load_chunk(LitState* state, LitEmulatedFile* file, LitModule* module, LitChunk* chunk);

char* lit_read_file(const char* path)
{
    FILE* file = fopen(path, "rb");

    if(file == NULL)
    {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesread = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesread] = '\0';

    fclose(file);
    return buffer;
}

bool lit_file_exists(const char* path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode);
}

bool lit_dir_exists(const char* path)
{
    struct stat buffer;
    return stat(path, &buffer) == 0 && S_ISDIR(buffer.st_mode);
}

void lit_write_uint8_t(FILE* file, uint8_t byte)
{
    fwrite(&byte, sizeof(uint8_t), 1, file);
}

void lit_write_uint16_t(FILE* file, uint16_t byte)
{
    fwrite(&byte, sizeof(uint16_t), 1, file);
}

void lit_write_uint32_t(FILE* file, uint32_t byte)
{
    fwrite(&byte, sizeof(uint32_t), 1, file);
}

void lit_write_uint64_t(FILE* file, uint64_t byte)
{
    fwrite(&byte, sizeof(uint64_t), 1, file);
}

void lit_write_double(FILE* file, double byte)
{
    fwrite(&byte, sizeof(double), 1, file);
}

void lit_write_string(FILE* file, LitString* string)
{
    uint16_t c = string->length;
    fwrite(&c, 2, 1, file);

    for(uint16_t i = 0; i < c; i++)
    {
        lit_write_uint8_t(file, (uint8_t)string->chars[i] ^ LIT_STRING_KEY);
    }
}

uint8_t lit_read_uint8_t(FILE* file)
{
    fread(&btmp, sizeof(uint8_t), 1, file);
    return btmp;
}

uint16_t lit_read_uint16_t(FILE* file)
{
    fread(&stmp, sizeof(uint16_t), 1, file);
    return stmp;
}

uint32_t lit_read_uint32_t(FILE* file)
{
    fread(&itmp, sizeof(uint32_t), 1, file);
    return itmp;
}

double lit_read_double(FILE* file)
{
    fread(&dtmp, sizeof(double), 1, file);
    return dtmp;
}

LitString* lit_read_string(LitState* state, FILE* file)
{
    uint16_t length;
    fread(&length, 2, 1, file);

    if(length < 1)
    {
        return NULL;
    }

    char line[length];

    for(uint16_t i = 0; i < length; i++)
    {
        line[i] = (char)lit_read_uint8_t(file) ^ LIT_STRING_KEY;
    }

    return lit_string_copy(state, line, length);
}

void lit_init_emulated_file(LitEmulatedFile* file, const char* source)
{
    file->source = source;
    file->position = 0;
}

uint8_t lit_read_euint8_t(LitEmulatedFile* file)
{
    return (uint8_t)file->source[file->position++];
}

uint16_t lit_read_euint16_t(LitEmulatedFile* file)
{
    return (uint16_t)(lit_read_euint8_t(file) | (lit_read_euint8_t(file) << 8u));
}

uint32_t lit_read_euint32_t(LitEmulatedFile* file)
{
    return (uint32_t)(lit_read_euint8_t(file) | (lit_read_euint8_t(file) << 8u) | (lit_read_euint8_t(file) << 16u) | (lit_read_euint8_t(file) << 24u));
}

uint64_t lit_read_euint64_t(LitEmulatedFile* file)
{
    return (uint64_t)(lit_read_euint32_t(file) | ((uint64_t)lit_read_euint32_t(file) << 32u));
}

double lit_read_edouble(LitEmulatedFile* file)
{
    uint8_t values[8];
    double result;

    for(LitUInt i = 0; i < 8; i++)
    {
        values[i] = lit_read_euint8_t(file);
    }

    memcpy(&result, values, 8);
    return result;
}

LitString* lit_read_estring(LitState* state, LitEmulatedFile* file)
{
    uint16_t length = lit_read_euint16_t(file);

    if(length < 1)
    {
        return NULL;
    }

    char line[length];

    for(uint16_t i = 0; i < length; i++)
    {
        line[i] = (char)lit_read_euint8_t(file) ^ LIT_STRING_KEY;
    }

    return lit_string_copy(state, line, length);
}

static void save_function(FILE* file, LitFunction* function)
{
    save_chunk(file, &function->chunk);
    lit_write_string(file, function->name);

    lit_write_uint8_t(file, function->arg_count);
    lit_write_uint16_t(file, function->upvalue_count);
    lit_write_uint8_t(file, (uint8_t)function->vararg);
    lit_write_uint8_t(file, (uint16_t)function->max_registers);
}

static LitFunction* load_function(LitState* state, LitEmulatedFile* file, LitModule* module)
{
    LitFunction* function = lit_object_makefunction(state, module);

    load_chunk(state, file, module, &function->chunk);
    function->name = lit_read_estring(state, file);

    function->arg_count = lit_read_euint8_t(file);
    function->upvalue_count = lit_read_euint16_t(file);
    function->vararg = (bool)lit_read_euint8_t(file);
    function->max_registers = lit_read_euint8_t(file);

    return function;
}

static void save_chunk(FILE* file, LitChunk* chunk)
{
    lit_write_uint32_t(file, chunk->count);

    for(LitUInt i = 0; i < chunk->count; i++)
    {
        lit_write_uint64_t(file, chunk->code[i]);
    }

    if(chunk->has_line_info)
    {
        LitUInt c = chunk->line_count * 2 + 2;
        lit_write_uint32_t(file, c);

        for(LitUInt i = 0; i < c; i++)
        {
            lit_write_uint16_t(file, chunk->lines[i]);
        }
    }
    else
    {
        lit_write_uint32_t(file, 0);
    }

    lit_write_uint32_t(file, chunk->constants.count);

    for(LitUInt i = 0; i < chunk->constants.count; i++)
    {
        LitValue constant = chunk->constants.values[i];

        if(IS_OBJECT(constant))
        {
            LitObjectType type = AS_OBJECT(constant)->type;
            lit_write_uint8_t(file, (uint8_t)(type + 1));

            switch(type)
            {
                case LIT_OBJ_STRING:
                {
                    lit_write_string(file, AS_STRING(constant));
                    break;
                }

                case LIT_OBJ_FUNCTION:
                {
                    save_function(file, AS_FUNCTION(constant));
                    break;
                }

                default:
                {
                    UNREACHABLE
                    break;
                }
            }
        }
        else
        {
            lit_write_uint8_t(file, 0);
            lit_write_double(file, AS_NUMBER(constant));
        }
    }
}

static void load_chunk(LitState* state, LitEmulatedFile* file, LitModule* module, LitChunk* chunk)
{
    lit_chunk_init(chunk);
    LitUInt count = lit_read_euint32_t(file);

    chunk->code = (uint64_t*)lit_reallocate(state, NULL, 0, sizeof(uint64_t) * count);
    chunk->count = count;
    chunk->capacity = count;

    for(LitUInt i = 0; i < count; i++)
    {
        chunk->code[i] = lit_read_euint64_t(file);
    }

    count = lit_read_euint32_t(file);

    if(count > 0)
    {
        chunk->lines = (uint16_t*)lit_reallocate(state, NULL, 0, sizeof(uint16_t) * count);
        chunk->line_count = count;
        chunk->line_capacity = count;

        for(LitUInt i = 0; i < count; i++)
        {
            chunk->lines[i] = lit_read_euint16_t(file);
        }
    }
    else
    {
        chunk->has_line_info = false;
    }

    count = lit_read_euint32_t(file);
    chunk->constants.values = (LitValue*)lit_reallocate(state, NULL, 0, sizeof(LitValue) * count);
    chunk->constants.count = count;
    chunk->constants.capacity = count;

    for(LitUInt i = 0; i < count; i++)
    {
        uint8_t type = lit_read_euint8_t(file);

        if(type == 0)
        {
            chunk->constants.values[i] = NUMBER_VALUE(lit_read_edouble(file));
        }
        else
        {
            switch((LitObjectType)(type - 1))
            {
                case LIT_OBJ_STRING:
                {
                    chunk->constants.values[i] = OBJECT_VALUE(lit_read_estring(state, file));
                    break;
                }

                case LIT_OBJ_FUNCTION:
                {
                    chunk->constants.values[i] = OBJECT_VALUE(load_function(state, file, module));
                    break;
                }

                default:
                {
                    UNREACHABLE
                    break;
                }
            }
        }
    }
}

void lit_save_module(LitModule* module, FILE* file)
{
    bool disabled;
    disabled = false;

    lit_write_string(file, module->name);
    lit_write_uint16_t(file, module->private_count);
    lit_write_uint8_t(file, (uint8_t)disabled);

    if(!disabled)
    {
        LitTable* privates = &module->private_names->values;

        for(LitUInt i = 0; i < module->private_count; i++)
        {
            if(privates->entries[i].key != NULL)
            {
                lit_write_string(file, privates->entries[i].key);
                lit_write_uint16_t(file, (uint16_t)AS_NUMBER(privates->entries[i].value));
            }
        }
    }

    save_function(file, module->main_function);
}

LitModule* lit_load_module(LitState* state, const char* input)
{
    LitEmulatedFile file;
    lit_init_emulated_file(&file, input);

    if(lit_read_euint16_t(&file) != LIT_BYTECODE_MAGIC_NUMBER)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown magic number");
        return NULL;
    }

    uint8_t bytecodeversion = lit_read_euint8_t(&file);

    if(bytecodeversion > LIT_BYTECODE_VERSION)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown bytecode version '%i'", (int)bytecodeversion);
        return NULL;
    }

    uint16_t modulecount = lit_read_euint16_t(&file);
    LitModule* first = NULL;

    for(uint16_t j = 0; j < modulecount; j++)
    {
        LitModule* module = lit_object_makemodule(state, lit_read_estring(state, &file));
        LitTable* privates = &module->private_names->values;

        uint16_t privatescount = lit_read_euint16_t(&file);
        bool enabled = !((bool)lit_read_euint8_t(&file));

        module->privates = LIT_ALLOCATE(state, LitValue, privatescount);
        module->private_count = privatescount;

        for(uint16_t i = 0; i < privatescount; i++)
        {
            module->privates[i] = NULL_VALUE;

            if(enabled)
            {
                LitString* name = lit_read_estring(state, &file);
                lit_table_set(state, privates, name, NUMBER_VALUE(lit_read_euint16_t(&file)));
            }
        }

        module->main_function = load_function(state, &file, module);
        lit_table_set(state, &state->vm->modules->values, module->name, OBJECT_VALUE(module));

        if(j == 0)
        {
            first = module;
        }
    }

    if(lit_read_euint16_t(&file) != LIT_BYTECODE_END_NUMBER)
    {
        lit_state_raiseerror(state, COMPILE_ERROR, "Failed to read compiled code, unknown end number");
        return NULL;
    }

    return first;
}

void cleanup_file(LitState* state, LitUserdata* data, bool mark)
{
    if(mark)
    {
        return;
    }

    LitFileData* filedata = ((LitFileData*)data->data);

    if(filedata->file != NULL)
    {
        fclose(filedata->file);
        filedata->file = NULL;
    }
}

static LitValue file_constructor(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    const char* path = LIT_CHECK_STRING(0);
    const char* mode = LIT_GET_STRING(1, "rw");

    FILE* file = fopen(path, mode);

    if(file == NULL)
    {
        lit_vm_raisefatalerror(vm, "Failed to open file %s with mode %s (C error: %s)", path, mode, strerror(errno));
    }

    LitFileData* data = LIT_INSERT_DATA(LitFileData, cleanup_file);

    data->path = (char*)path;
    data->file = file;

    return instance;
}

static LitValue file_close(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    fclose(data->file);

    data->file = NULL;
    return NULL_VALUE;
}

static LitValue lit_coreutil_fileexists(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    char* file_name = NULL;

    if(IS_INSTANCE(instance))
    {
        file_name = LIT_EXTRACT_DATA(LitFileData)->path;
    }
    else
    {
        file_name = (char*)LIT_CHECK_STRING(0);
    }

    return BOOL_VALUE(lit_file_exists(file_name));
}

static LitValue file_create(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    const char* path = LIT_CHECK_STRING(0);
    FILE* file = fopen(path, "w");

    if(file == NULL)
    {
        lit_vm_raisefatalerror(vm, "Failed to create file %s", path);
    }

    fclose(file);
    return NULL_VALUE;
}

/*
 * ==
 * File writing
 */

static LitValue file_write(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LIT_ENSURE_ARGS(1)

    LitString* value = lit_tostring_value(vm->state, args[0], 0);
    fwrite(value->chars, value->length, 1, LIT_EXTRACT_DATA(LitFileData)->file);

    return NULL_VALUE;
}

static LitValue file_writeByte(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    uint8_t byte = (uint8_t)LIT_CHECK_NUMBER(0);
    lit_write_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file, byte);

    return NULL_VALUE;
}

static LitValue file_writeShort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    uint16_t shrt = (uint16_t)LIT_CHECK_NUMBER(0);
    lit_write_uint16_t(LIT_EXTRACT_DATA(LitFileData)->file, shrt);

    return NULL_VALUE;
}

static LitValue file_writeNumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    float num = (float)LIT_CHECK_NUMBER(0);
    lit_write_uint32_t(LIT_EXTRACT_DATA(LitFileData)->file, num);

    return NULL_VALUE;
}

static LitValue file_writeBool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    bool value = LIT_CHECK_BOOL(0);

    lit_write_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file, (uint8_t)value ? '1' : '0');
    return NULL_VALUE;
}

static LitValue file_writeString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    if(LIT_CHECK_STRING(0) == NULL)
    {
        return NULL_VALUE;
    }

    LitString* string = AS_STRING(args[0]);
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);

    lit_write_string(data->file, string);
    return NULL_VALUE;
}

/*
 * ==
 * File reading
 */

static LitValue file_readAll(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);

    fseek(data->file, 0, SEEK_END);
    LitUInt length = ftell(data->file);
    fseek(data->file, 0, SEEK_SET);

    LitString* result = lit_object_makeemptystring(vm->state, length);

    result->chars = LIT_ALLOCATE(vm->state, char, length + 1);
    result->chars[length] = '\0';

    fread(result->chars, 1, length, data->file);

    result->hash = lit_string_hash(result->chars, result->length);
    lit_string_register(vm->state, result);

    return OBJECT_VALUE(result);
}

static LitValue file_readLine(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitUInt maxlength = (LitUInt)LIT_GET_NUMBER(0, 128);
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);

    char line[maxlength];

    if(!fgets(line, maxlength, data->file))
    {
        return NULL_VALUE;
    }

    return OBJECT_VALUE(lit_string_copy(vm->state, line, strlen(line) - 1));
}

static LitValue file_readByte(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(lit_read_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

static LitValue file_readShort(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(lit_read_uint16_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

static LitValue file_readNumber(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return NUMBER_VALUE(lit_read_uint32_t(LIT_EXTRACT_DATA(LitFileData)->file));
}

static LitValue file_readBool(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    return BOOL_VALUE((char)lit_read_uint8_t(LIT_EXTRACT_DATA(LitFileData)->file) == '1');
}

static LitValue file_readString(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    LitFileData* data = LIT_EXTRACT_DATA(LitFileData);
    LitString* string = lit_read_string(vm->state, data->file);

    return string == NULL ? NULL_VALUE : OBJECT_VALUE(string);
}

static LitValue file_getLastModified(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    struct stat buffer;
    char* file_name = NULL;

    if(IS_INSTANCE(instance))
    {
        file_name = LIT_EXTRACT_DATA(LitFileData)->path;
    }
    else
    {
        file_name = (char*)LIT_CHECK_STRING(0);
    }

    if(stat(file_name, &buffer) != 0)
    {
        return NUMBER_VALUE(0);
    }

#ifdef WIN32
    return NUMBER_VALUE(buffer.st_mtime);// Why, Windows, why?
#else
    return NUMBER_VALUE(buffer.st_mtim.tv_sec);
#endif
}


/*
 * Directory
 */

static LitValue directory_exists(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    const char* directoryname = LIT_CHECK_STRING(0);
    struct stat buffer;

    return BOOL_VALUE(stat(directoryname, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

static LitValue directory_listFiles(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    struct dirent* ep;

    LitState* state = vm->state;
    const char* path = LIT_CHECK_STRING(0);
    DIR* dir = opendir(path);
    LitArray* array = lit_object_makearray(state);

    if(dir == NULL)
    {
        return OBJECT_VALUE(array);
    }

    while((ep = readdir(dir)))
    {
        const char* dirname = ep->d_name;

        if(strcmp(dirname, "..") == 0 || strcmp(dirname, ".") == 0)
        {
            continue;
        }

        size_t basedirnamelength = strlen(path);

        size_t dirnamelength = strlen(dirname);
        size_t totallength = dirnamelength + basedirnamelength + 2;

        char subdirname[totallength];

        memcpy(subdirname, path, basedirnamelength);
        memcpy(subdirname + basedirnamelength + 1, dirname, dirnamelength);

        subdirname[basedirnamelength] = '/';
        subdirname[totallength - 1] = '\0';

        struct stat st;
        stat(subdirname, &st);

        if(S_ISREG(st.st_mode))
        {
            lit_vallist_push(state, &array->values, OBJECT_CONST_STRING(state, dirname));
        }
    }


    closedir(dir);
    return OBJECT_VALUE(array);
}

static LitValue directory_listDirectories(LitVm* vm, LitValue instance, LitUInt argc, LitValue* args)
{
    struct dirent* ep;

    LitState* state = vm->state;
    const char* path = LIT_CHECK_STRING(0);
    DIR* dir = opendir(path);
    LitArray* array = lit_object_makearray(state);

    if(dir == NULL)
    {
        return OBJECT_VALUE(array);
    }

    while((ep = readdir(dir)))
    {
        const char* dirname = ep->d_name;

        if(strcmp(dirname, "..") == 0 || strcmp(dirname, ".") == 0)
        {
            continue;
        }

        size_t basedirnamelength = strlen(path);

        size_t dirnamelength = strlen(dirname);
        size_t totallength = dirnamelength + basedirnamelength + 2;

        char subdirname[totallength];

        memcpy(subdirname, path, basedirnamelength);
        memcpy(subdirname + basedirnamelength + 1, dirname, dirnamelength);

        subdirname[basedirnamelength] = '/';
        subdirname[totallength - 1] = '\0';

        struct stat st;
        stat(subdirname, &st);

        if(S_ISDIR(st.st_mode))
        {
            lit_vallist_push(state, &array->values, OBJECT_CONST_STRING(state, dirname));
        }
    }

    closedir(dir);
    return OBJECT_VALUE(array);
}

void lit_open_file_library(LitState* state)
{
    {
        LIT_BEGIN_CLASS("File");
        lit_class_bindstaticmethod(state, klass, "exists", lit_coreutil_fileexists);
        lit_class_bindstaticmethod(state, klass, "getLastModified", file_getLastModified);
        lit_class_bindstaticmethod(state, klass, "create", file_create);

        lit_class_bindconstructor(state, klass, file_constructor);
        lit_class_bindmethod(state, klass, "close", file_close);
        lit_class_bindmethod(state, klass, "write", file_write);

        lit_class_bindmethod(state, klass, "writeByte", file_writeByte);
        lit_class_bindmethod(state, klass, "writeShort", file_writeShort);
        lit_class_bindmethod(state, klass, "writeNumber", file_writeNumber);
        lit_class_bindmethod(state, klass, "writeBool", file_writeBool);
        lit_class_bindmethod(state, klass, "writeString", file_writeString);

        lit_class_bindmethod(state, klass, "readAll", file_readAll);
        lit_class_bindmethod(state, klass, "readLine", file_readLine);

        lit_class_bindmethod(state, klass, "readByte", file_readByte);
        lit_class_bindmethod(state, klass, "readShort", file_readShort);
        lit_class_bindmethod(state, klass, "readNumber", file_readNumber);
        lit_class_bindmethod(state, klass, "readBool", file_readBool);
        lit_class_bindmethod(state, klass, "readString", file_readString);

        lit_class_bindmethod(state, klass, "getLastModified", file_getLastModified);

        lit_class_bindgetsetter(state, klass, "exists", lit_coreutil_fileexists, NULL);
        LIT_END_CLASS();
    }
    {
        LIT_BEGIN_CLASS("Directory");
        lit_class_bindstaticmethod(state, klass, "exists", directory_exists);
        lit_class_bindstaticmethod(state, klass, "listFiles", directory_listFiles);
        lit_class_bindstaticmethod(state, klass, "listDirectories", directory_listDirectories);
        LIT_END_CLASS();
    }
}
