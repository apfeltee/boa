
#define fmax(a, b) ((a) > (b) ? (a) : (b))

uint32_t lit_emitter_reserveregister(LitEmitter* emitter)
{
    LitCompiler* compiler = emitter->compiler;
    if(compiler->registers_used == LIT_REGISTERS_MAX)
    {
        lit_emitter_raiseerror(emitter, emitter->last_line, "Too many registers required");
        return 0;
    }
    compiler->function->maxregisters = fmax(compiler->function->maxregisters, compiler->registers_used+1);
    ++compiler->registers_used;
    return compiler->registers_used - 1;
}
