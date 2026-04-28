
#include <stdio.h>
#include "priv.h"

void lit_debug_disasmodule(LitModule* module, const char* source)
{
    lit_debug_disaschunk(&module->main_function->chunk, module->main_function->name->chars, source);
}

static void lit_debug_printconst(LitValue value)
{
    if(IS_FUNCTION(value))
    {
        LitString* fnname = AS_FUNCTION(value)->name;
        printf("%sfunction %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_CLOSURE(value))
    {
        LitString* fnname = AS_CLOSURE(value)->function->name;
        printf("%sclosure %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_CLOSURE_PROTOTYPE(value))
    {
        LitString* fnname = AS_CLOSURE_PROTOTYPE(value)->function->name;
        printf("%sclosure prototype %.*s%s", COLOR_CYAN, fnname->length, fnname->chars, COLOR_RESET);
    }
    else if(IS_STRING(value))
    {
        LitString* string = AS_STRING(value);
        printf("%s\"%.*s\"%s", COLOR_CYAN, string->length, string->chars, COLOR_RESET);
    }
    else if(IS_NUMBER(value))
    {
        printf("%s%g%s", COLOR_CYAN, AS_NUMBER(value), COLOR_RESET);
    }
    else
    {
        printf("unknown");
    }
}

void lit_debug_disaschunk(LitChunk* chunk, const char* name, const char* source)
{
    LitValList* values = &chunk->constants;
    printf("^^ %s ^^\n", name);

    if(values->count > 0)
    {
        printf("%sconstants:%s\n", COLOR_MAGENTA, COLOR_RESET);

        for(LitUInt i = 0; i < values->count; i++)
        {
            LitValue value = values->values[i];

            printf("% 4d ", i);
            lit_debug_printconst(value);
            printf("\n");
        }
    }

    printf("%stext:%s\n", COLOR_MAGENTA, COLOR_RESET);

    for(LitUInt offset = 0; offset < chunk->count; offset++)
    {
        lit_debug_disasinstr(chunk, offset, source, false);
    }


    printf("%shex:%s\n", COLOR_MAGENTA, COLOR_RESET);

    for(LitUInt offset = 0; offset < chunk->count; offset++)
    {
        printf("%08lX ", chunk->code[offset]);
    }

    printf("\n");

    printf("vv %s vv\n", name);
}

typedef void (*LitDebugInstructionFn)(uint64_t instruction, const char* name);

static void lit_debug_printabcinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_B(instruction), LIT_INSTRUCTION_C(instruction));
}

static void lit_debug_printabxinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%lu\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_BX(instruction));
}

static void lit_debug_printasbxinstr(uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu \t%li\n", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction),
           LIT_INSTRUCTION_SBX(instruction));
}

static void lit_debug_printregister(uint16_t reg)
{
    printf(" \t%hu", reg);
}

static void lit_debug_printconstarg(LitChunk* chunk, uint16_t arg, bool indent)
{
    arg &= 0xff;

    printf("%sc%hu (", indent ? " \t" : "", arg);
    lit_debug_printconst(chunk->constants.values[arg]);
    printf(")");
}

static void lit_debug_printconstorregister(LitChunk* chunk, uint16_t arg)
{
    if(IS_BIT_SET(arg, 8))
    {
        lit_debug_printconstarg(chunk, arg, true);
    }
    else
    {
        lit_debug_printregister(arg);
    }
}

static void lit_debug_printunaryinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_B(instruction));
    printf("\n");
}

static void lit_debug_printbinaryinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s %lu", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "", LIT_INSTRUCTION_A(instruction));

    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_B(instruction));
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_C(instruction));

    printf("\n");
}

static void lit_debug_printglobalinstr(LitChunk* chunk, uint64_t instruction, const char* name)
{
    printf("%s%s%s%*s", COLOR_YELLOW, name, COLOR_RESET, LIT_LONGEST_OP_NAME - (int)strlen(name), "");

    lit_debug_printconstarg(chunk, LIT_INSTRUCTION_BX(instruction), false);
    lit_debug_printconstorregister(chunk, LIT_INSTRUCTION_A(instruction));

    printf("\n");
}

static LitDebugInstructionFn debuginstrfuncs[] = { lit_debug_printabcinstr, lit_debug_printabxinstr, lit_debug_printasbxinstr };

void lit_debug_disasinstr(LitChunk* chunk, LitUInt offset, const char* source, bool forceline)
{
    LitUInt line = lit_chunk_getline(chunk, offset);
    bool same = !chunk->has_line_info || (offset > 0 && line == lit_chunk_getline(chunk, offset - 1));

    if(!same && source != NULL)
    {
        LitUInt index = 0;
        char* currentline = (char*)source;

        while(currentline)
        {
            char* nextline = strchr(currentline, '\n');
            char* prevline = currentline;

            index++;
            currentline = nextline ? (nextline + 1) : NULL;

            if(index == line)
            {
                char* outputline = prevline ? prevline : nextline;
                char c;

                while((c = *outputline) && (c == '\t' || c == ' '))
                {
                    outputline++;
                }

                printf("%s        %.*s%s\n", COLOR_RED, nextline ? (int)(nextline - outputline) : (int)strlen(prevline), outputline, COLOR_RESET);
                break;
            }
        }
    }

    printf("%04d ", offset);

    if(same && !forceline)
    {
        printf("   | ");
    }
    else
    {
        printf("%s%4d%s ", COLOR_BLUE, line, COLOR_RESET);
    }

    uint64_t instruction = chunk->code[offset];
    uint8_t opcode = LIT_INSTRUCTION_OPCODE(instruction);

    switch(opcode)
    {
        case OP_MOVE:
            lit_debug_printbinaryinstr(chunk, instruction, "MOVE");
            break;
        case OP_ADD:
            lit_debug_printbinaryinstr(chunk, instruction, "ADD");
            break;
        case OP_SUBTRACT:
            lit_debug_printbinaryinstr(chunk, instruction, "SUBTRACT");
            break;
        case OP_MULTIPLY:
            lit_debug_printbinaryinstr(chunk, instruction, "MULTIPLY");
            break;
        case OP_DIVIDE:
            lit_debug_printbinaryinstr(chunk, instruction, "DIVIDE");
            break;
        case OP_NEGATE:
            lit_debug_printunaryinstr(chunk, instruction, "NEGATE");
            break;
        case OP_NOT:
            lit_debug_printunaryinstr(chunk, instruction, "NOT");
            break;

        case OP_EQUAL:
            lit_debug_printbinaryinstr(chunk, instruction, "EQUAL");
            break;
        case OP_LESS:
            lit_debug_printbinaryinstr(chunk, instruction, "LESS");
            break;
        case OP_LESS_EQUAL:
            lit_debug_printbinaryinstr(chunk, instruction, "LESS_EQUAL");
            break;

        case OP_SET_GLOBAL:
            lit_debug_printglobalinstr(chunk, instruction, "SET_GLOBAL");
            break;
        case OP_GET_GLOBAL:
            lit_debug_printglobalinstr(chunk, instruction, "GET_GLOBAL");
            break;

        default:
        {
            switch(opcode)
            {
// A simple way to automatically generate case printers for all the opcodes
#define OPCODE(name, stringname, type)                                   \
    case OP_##name:                                                       \
    {                                                                     \
        debuginstrfuncs[(int)type](instruction, stringname); \
        break;                                                            \
    }

#include "opcodes.inc"
#undef OPCODE

                default:
                {
                    printf("Unknown opcode %d\n", opcode);
                    break;
                }
            }
        }
    }
}

void lit_debug_traceframe(LitFiber* fiber)
{
#ifdef LIT_TRACE_STACK
    if(fiber == NULL)
    {
        return;
    }

    LitCallFrame* frame = &fiber->frames[fiber->frame_count - 1];
    printf("== fiber %p f%i %s (expects %i, max %i, added %i, current %i, exits %i) ==\n", fiber, fiber->frame_count - 1, frame->function->name->chars,
           frame->function->arg_count, frame->function->max_registers, frame->function->max_registers + (int)(fiber->stack_top - fiber->stack),
           fiber->stack_capacity, frame->return_address == NULL);
#endif
}