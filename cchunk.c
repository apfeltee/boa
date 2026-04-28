
#include "lit.h"

void lit_chunk_init(LitChunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    chunk->has_line_info = true;
    chunk->line_count = 0;
    chunk->line_capacity = 0;
    chunk->lines = NULL;

    lit_vallist_init(&chunk->constants);
}

void lit_chunk_destroy(LitState* state, LitChunk* chunk)
{
    LIT_FREE_ARRAY(state, uint64_t, chunk->code, chunk->capacity);
    LIT_FREE_ARRAY(state, uint16_t, chunk->lines, chunk->line_capacity);

    lit_vallist_destroy(state, &chunk->constants);
    lit_chunk_init(chunk);
}

void lit_chunk_push(LitState* state, LitChunk* chunk, uint64_t word, uint16_t line)
{
    if(chunk->capacity < chunk->count + 1)
    {
        LitUInt oldcapacity = chunk->capacity;

        chunk->capacity = LIT_GROW_CAPACITY(oldcapacity);
        chunk->code = LIT_GROW_ARRAY(state, chunk->code, uint64_t, oldcapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = word;
    chunk->count++;

    if(!chunk->has_line_info)
    {
        return;
    }

    if(chunk->line_capacity < chunk->line_count + 4)
    {
        LitUInt oldcapacity = chunk->line_capacity;

        chunk->line_capacity = LIT_GROW_CAPACITY(chunk->line_capacity);
        chunk->lines = LIT_GROW_ARRAY(state, chunk->lines, uint16_t, oldcapacity, chunk->line_capacity);

        if(oldcapacity == 0)
        {
            chunk->lines[0] = 0;
            chunk->lines[1] = 0;
        }
    }

    LitUInt lineindex = chunk->line_count;
    LitUInt value = chunk->lines[lineindex];

    if(value != 0 && value != line)
    {
        chunk->line_count += 2;
        lineindex = chunk->line_count;
        chunk->lines[lineindex + 1] = 0;
    }

    chunk->lines[lineindex] = line;
    chunk->lines[lineindex + 1]++;
}

LitUInt lit_chunk_addconstant(LitState* state, LitChunk* chunk, LitValue constant)
{
    for(LitUInt i = 0; i < chunk->constants.count; i++)
    {
        if(chunk->constants.values[i] == constant)
        {
            return i;
        }
    }

    lit_state_pushvalueroot(state, constant);
    lit_vallist_push(state, &chunk->constants, constant);
    lit_state_poproot(state);

    return chunk->constants.count - 1;
}

LitUInt lit_chunk_getline(LitChunk* chunk, LitUInt offset)
{
    if(!chunk->has_line_info)
    {
        return 0;
    }

    LitUInt rle = 0;
    LitUInt line = 0;
    LitUInt index = 0;

    for(LitUInt i = 0; i <= offset; i++)
    {
        if(rle > 0)
        {
            rle--;
            continue;
        }

        line = chunk->lines[index];
        rle = chunk->lines[index + 1];

        if(rle > 0)
        {
            rle--;
        }

        index += 2;
    }

    return line;
}

void lit_chunk_shrink(LitState* state, LitChunk* chunk)
{
    if(chunk->capacity > chunk->count)
    {
        LitUInt oldcapacity = chunk->capacity;

        chunk->capacity = chunk->count;
        chunk->code = LIT_GROW_ARRAY(state, chunk->code, uint64_t, oldcapacity, chunk->capacity);
    }

    if(chunk->line_capacity > chunk->line_count)
    {
        LitUInt oldcapacity = chunk->line_capacity;

        chunk->line_capacity = chunk->line_count + 2;
        chunk->lines = LIT_GROW_ARRAY(state, chunk->lines, uint16_t, oldcapacity, chunk->line_capacity);
    }
}