
#include <stdlib.h>
#include <setjmp.h>
#include "priv.h"


static jmp_buf jumpbuffer;
static LitParseRule rules[LTOKEN_EOF + 1];
static bool didsetuprules;


/* ccparser.c */
static void lit_parser_compilerinit(LitParser *parser, LitCompiler *compiler);
static void lit_parser_compilerend(LitParser *parser, LitCompiler *compiler);
static void lit_parser_scopebegin(LitParser *parser);
static void lit_parser_scopeend(LitParser *parser);
static LitParseRule *lit_parser_getrule(LitTokenType type);
void lit_parser_init(LitState *state, LitParser *parser);
void lit_parser_destroy(LitParser *parser);
static void lit_parser_failactual(LitParser *parser, LitToken *token, const char *message);
static void lit_parser_failatv(LitParser *parser, LitToken *token, const char *fmt, va_list args);
static void lit_parser_failherefmt(LitParser *parser, const char *fmt, ...);
static void lit_parser_failfmt(LitParser *parser, const char *fmt, ...);
static void lit_parser_advance(LitParser *parser);
static bool lit_parser_check(LitParser *parser, LitTokenType type);
static bool lit_parser_match(LitParser *parser, LitTokenType type);
static bool lit_parser_matchident(LitParser *parser, const char *type);
static void lit_parser_consume(LitParser *parser, LitTokenType type, const char *error);
static bool lit_parser_matchlinefeed(LitParser *parser);
static void lit_parser_ignorelinefeeds(LitParser *parser);
static LitExpression *lit_parser_parseblock(LitParser *parser);
static LitExpression *lit_parser_parseprec(LitParser *parser, LitPrecedence precedence, bool err);
static LitExpression *lit_parser_rulenumber(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_parselambda(LitParser *parser, LitFunctionStatement *lambda);
static void lit_parser_parseparams(LitParser *parser, LitParamList *parameters);
static LitExpression *lit_parser_rulegroupingorlambda(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_parsecall(LitParser *parser, LitExpression *prev, bool can_assign);
static LitExpression *lit_parser_ruleunary(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_rulebinary(LitParser *parser, LitExpression *prev, bool can_assign);
static LitExpression *lit_parser_rulelogicaland(LitParser *parser, LitExpression *prev, bool can_assign);
static LitExpression *lit_parser_rulelogicalor(LitParser *parser, LitExpression *prev, bool can_assign);
static LitExpression *lit_parser_rulenullfilter(LitParser *parser, LitExpression *prev, bool can_assign);
static LitTokenType lit_parser_convertcompoundop(LitTokenType op);
static LitExpression *lit_parser_rulecompound(LitParser *parser, LitExpression *prev, bool can_assign);
static LitExpression *lit_parser_ruleliteral(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_rulestring(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_ruleinterpolation(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_ruleobject(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_parsevarexprbase(LitParser *parser, bool can_assign, bool new);
static LitExpression *lit_parser_rulevarexpr(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_rulenewexpr(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_ruledot(LitParser *parser, LitExpression *previous, bool can_assign);
static LitExpression *lit_parser_rulerange(LitParser *parser, LitExpression *previous, bool can_assign);
static LitExpression *lit_parser_ruleternaryorquestion(LitParser *parser, LitExpression *previous, bool can_assign);
static LitExpression *lit_parser_rulearray(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_parsesubscript(LitParser *parser, LitExpression *previous, bool can_assign);
static LitExpression *lit_parser_rulethis(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_rulesuper(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_rulenothing(LitParser *parser, bool canassign);
static LitExpression *lit_parser_rulefunction(LitParser *parser, bool canassign);
static LitExpression *lit_parser_rulereference(LitParser *parser, bool can_assign);
static LitExpression *lit_parser_parseexpr(LitParser *parser);
static LitExpression *lit_parser_parsevardecl(LitParser *parser);
static LitExpression *lit_parser_parseif(LitParser *parser);
static LitExpression *lit_parser_parsefor(LitParser *parser);
static LitExpression *lit_parser_parsewhile(LitParser *parser);
static LitExpression *lit_parser_parsefunction(LitParser *parser);
static LitExpression *lit_parser_parsereturn(LitParser *parser);
static LitExpression *lit_parser_parsefield(LitParser *parser, LitString *name, bool is_static);
static LitExpression *lit_parser_parsemethod(LitParser *parser, bool is_static);
static LitExpression *lit_parser_parseclass(LitParser *parser);
static void lit_parser_sync(LitParser *parser);
static LitExpression *lit_parser_parsestmt(LitParser *parser);
static LitExpression *lit_parser_parsedecl(LitParser *parser);
bool lit_parser_parsesource(LitParser *parser, const char *file_name, const char *source, LitExprList *statements);
static void lit_parser_setuprules(void);

static void lit_parser_compilerinit(LitParser* parser, LitCompiler* compiler)
{
    compiler->scope_depth = 0;
    compiler->function = NULL;
    compiler->enclosing = (struct LitCompiler*)parser->compiler;

    parser->compiler = compiler;
}

static void lit_parser_compilerend(LitParser* parser, LitCompiler* compiler)
{
    parser->compiler = (LitCompiler*)compiler->enclosing;
}

static void lit_parser_scopebegin(LitParser* parser)
{
    parser->compiler->scope_depth++;
}

static void lit_parser_scopeend(LitParser* parser)
{
    parser->compiler->scope_depth--;
}

static LitParseRule* lit_parser_getrule(LitTokenType type)
{
    return &rules[type];
}

static inline bool lit_scanner_isatend(LitParser* parser)
{
    return parser->current.type == LTOKEN_EOF;
}

void lit_parser_init(LitState* state, LitParser* parser)
{
    if(!didsetuprules)
    {
        didsetuprules = true;
        lit_parser_setuprules();
    }

    parser->state = state;
    parser->had_error = false;
    parser->panic_mode = false;
}

void lit_parser_destroy(LitParser* parser)
{
}

static void lit_parser_failactual(LitParser* parser, LitToken* token, const char* message)
{
    if(parser->panic_mode)
    {
        return;
    }

    lit_state_raiseerror(parser->state, COMPILE_ERROR, message);
    parser->had_error = true;
    lit_parser_sync(parser);
}

static void lit_parser_failatv(LitParser* parser, LitToken* token, const char* fmt, va_list args)
{
    lit_parser_failactual(parser, token, lit_state_errorfmtv(parser->state, token->line, fmt, args)->chars);
}

static void lit_parser_failherefmt(LitParser* parser, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_parser_failatv(parser, &parser->current, fmt, args);
    va_end(args);
}

static void lit_parser_failfmt(LitParser* parser, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    lit_parser_failatv(parser, &parser->previous, fmt, args);
    va_end(args);
}

static void lit_parser_advance(LitParser* parser)
{
    parser->previous = parser->current;

    while(true)
    {
        parser->current = lit_scanner_scantoken(parser->state->scanner);

        if(parser->current.type != LTOKEN_ERROR)
        {
            break;
        }

        lit_parser_failactual(parser, &parser->current, parser->current.start);
    }
}

static bool lit_parser_check(LitParser* parser, LitTokenType type)
{
    return parser->current.type == type;
}

static bool lit_parser_match(LitParser* parser, LitTokenType type)
{
    if(parser->current.type == type)
    {
        lit_parser_advance(parser);
        return true;
    }

    return false;
}

static bool lit_parser_matchident(LitParser* parser, const char* type)
{
    if(parser->current.type == LTOKEN_IDENTIFIER && memcmp(parser->previous.start, type, fmax(strlen(type), parser->previous.length)))
    {
        lit_parser_advance(parser);
        return true;
    }

    return false;
}

static void lit_parser_consume(LitParser* parser, LitTokenType type, const char* error)
{
    if(parser->current.type == type)
    {
        lit_parser_advance(parser);
        return;
    }

    bool line = parser->previous.type == LTOKEN_NEW_LINE;
    lit_parser_failactual(parser, &parser->current,
                 lit_state_errorfmt(parser->state, parser->current.line, "Expected %s, got '%.*s'", error, line ? 8 : parser->previous.length,
                                  line ? "new line" : parser->previous.start)
                 ->chars);
}

static bool lit_parser_matchlinefeed(LitParser* parser)
{
    if(!lit_parser_match(parser, LTOKEN_NEW_LINE))
    {
        return false;
    }

    while(lit_parser_match(parser, LTOKEN_NEW_LINE))
    {
    }

    return true;
}

static void lit_parser_ignorelinefeeds(LitParser* parser)
{
    lit_parser_matchlinefeed(parser);
}

static LitExpression* lit_parser_parseblock(LitParser* parser)
{
    lit_parser_scopebegin(parser);
    LitBlockStatement* statement = lit_ast_makeblockstmt(parser->state, parser->previous.line);
    lit_parser_ignorelinefeeds(parser);
    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE) && !lit_parser_check(parser, LTOKEN_EOF))
    {
        lit_stmtlist_push(parser->state, &statement->statements, lit_parser_parsestmt(parser));
        lit_parser_ignorelinefeeds(parser);
    }
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}'");
    lit_parser_scopeend(parser);
    return (LitExpression*)statement;
}

static LitExpression* lit_parser_parseprec(LitParser* parser, LitPrecedence precedence, bool err)
{
    LitToken previous = parser->previous;

    lit_parser_advance(parser);
    LitPrefixParseFn prefix_rule = lit_parser_getrule(parser->previous.type)->prefix;

    if(prefix_rule == NULL)
    {
        // todo: file start
        bool prevnewline = previous.start != NULL && *previous.start == '\n';
        bool parserprevnewline = parser->previous.start != NULL && *parser->previous.start == '\n';

        lit_parser_failfmt(parser, "Expected expression after '%.*s', got '%.*s'", prevnewline ? 8 : previous.length, prevnewline ? "new line" : previous.start,
              parserprevnewline ? 8 : parser->previous.length, parserprevnewline ? "new line" : parser->previous.start);
        return NULL;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    LitExpression* expr = prefix_rule(parser, can_assign);

    lit_parser_ignorelinefeeds(parser);

    while(precedence <= lit_parser_getrule(parser->current.type)->precedence)
    {
        lit_parser_advance(parser);
        LitInfixParseFn infix_rule = lit_parser_getrule(parser->previous.type)->infix;
        expr = infix_rule(parser, expr, can_assign);
    }

    if(err && can_assign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        lit_parser_failfmt(parser, "Invalid assigment target");
    }

    return expr;
}

static LitExpression* lit_parser_rulenumber(LitParser* parser, bool can_assign)
{
    return (LitExpression*)lit_ast_makeliteralexpr(parser->state, parser->previous.line, parser->previous.value);
}

static LitExpression* lit_parser_parselambda(LitParser* parser, LitFunctionStatement* lambda)
{
    lambda->body = lit_parser_parsestmt(parser);
    return (LitExpression*)lambda;
}

static void lit_parser_parseparams(LitParser* parser, LitParamList* parameters)
{
    bool haddefault = false;

    while(!lit_parser_check(parser, LTOKEN_RIGHT_PAREN))
    {
        // Vararg ...
        if(lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
        {
            lit_paramlist_push(parser->state, parameters, (LitParameter){ "...", 3, 0, NULL });

            return;
        }

        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "argument name");
        const char* argname = parser->previous.start;
        LitUInt arglength = parser->previous.length;
        LitExpression* default_value = NULL;

        if(lit_parser_match(parser, LTOKEN_EQUAL))
        {
            haddefault = true;
            default_value = lit_parser_parseexpr(parser);
        }
        else if(haddefault)
        {
            lit_parser_failfmt(parser, "Default arguments must always be in the end of the argument list.");
        }

        lit_paramlist_push(parser->state, parameters, (LitParameter){ argname, arglength, 0, default_value });

        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }
}

static LitExpression* lit_parser_rulegroupingorlambda(LitParser* parser, bool can_assign)
{
    if(lit_parser_match(parser, LTOKEN_RIGHT_PAREN))
    {
        lit_parser_consume(parser, LTOKEN_ARROW, "=> after lambda arguments");
        return lit_parser_parselambda(parser, lit_ast_makelambdaexpr(parser->state, parser->previous.line));
    }

    const char* start = parser->previous.start;
    LitUInt line = parser->previous.line;

    if(lit_parser_match(parser, LTOKEN_IDENTIFIER) || lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
    {
        LitState* state = parser->state;

        const char* firstargstart = parser->previous.start;
        LitUInt firstarglength = parser->previous.length;

        if(lit_parser_match(parser, LTOKEN_COMMA) || (lit_parser_match(parser, LTOKEN_RIGHT_PAREN) && lit_parser_match(parser, LTOKEN_ARROW)))
        {
            bool hadarrow = parser->previous.type == LTOKEN_ARROW;
            bool hadvararg = parser->previous.type == LTOKEN_DOT_DOT_DOT;

            // This is a lambda
            LitFunctionStatement* lambda = lit_ast_makelambdaexpr(state, line);
            LitExpression* defvalue = NULL;

            bool haddefault = lit_parser_match(parser, LTOKEN_EQUAL);

            if(haddefault)
            {
                defvalue = lit_parser_parseexpr(parser);
            }

            lit_paramlist_push(state, &lambda->parameters, (LitParameter){ firstargstart, firstarglength, 0, defvalue });

            if(!hadvararg && parser->previous.type == LTOKEN_COMMA)
            {
                do
                {
                    bool stop = false;

                    if(lit_parser_match(parser, LTOKEN_DOT_DOT_DOT))
                    {
                        stop = true;
                    }
                    else
                    {
                        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "argument name");
                    }

                    const char* argname = parser->previous.start;
                    LitUInt arglength = parser->previous.length;
                    LitExpression* default_value = NULL;

                    if(lit_parser_match(parser, LTOKEN_EQUAL))
                    {
                        default_value = lit_parser_parseexpr(parser);
                        haddefault = true;
                    }
                    else if(haddefault)
                    {
                        lit_parser_failfmt(parser, "Default arguments must always be in the end of the argument list.");
                    }

                    lit_paramlist_push(state, &lambda->parameters, (LitParameter){ argname, arglength, 0, default_value });

                    if(stop)
                    {
                        break;
                    }
                } while(lit_parser_match(parser, LTOKEN_COMMA));
            }

            if(!hadarrow)
            {
                lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after lambda parameters");
                lit_parser_consume(parser, LTOKEN_ARROW, "=> after lambda parameters");
            }

            return lit_parser_parselambda(parser, lambda);
        }
        else
        {
            // Ouch, this was a grouping with a single identifier

            LitScanner* scanner = state->scanner;

            scanner->current = start;
            scanner->line = line;

            parser->current = lit_scanner_scantoken(scanner);
            lit_parser_advance(parser);
        }
    }

    LitExpression* expression = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after grouping expression");

    return expression;
}

static LitExpression* lit_parser_parsecall(LitParser* parser, LitExpression* prev, bool can_assign)
{
    LitCallExpression* expression = lit_ast_makecallexpr(parser->state, parser->previous.line, prev);

    while(!lit_parser_check(parser, LTOKEN_RIGHT_PAREN))
    {
        LitExpression* e = lit_parser_parseexpr(parser);
        lit_exprlist_push(parser->state, &expression->args, e);

        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }

        if(e->type == LIT_EXPR_VAR)
        {
            LitVarExpression* ee = (LitVarExpression*)e;

            // Vararg ...
            if(ee->length == 3 && memcmp(ee->name, "...", 3) == 0)
            {
                break;
            }
        }
    }

    if(expression->args.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)expression->args.count);
    }

    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after arguments");
    return (LitExpression*)expression;
}

static LitExpression* lit_parser_ruleunary(LitParser* parser, bool can_assign)
{
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;
    LitExpression* expression = lit_parser_parseprec(parser, PREC_UNARY, true);

    return (LitExpression*)lit_ast_makeunaryexpr(parser->state, line, expression, op);
}

static LitExpression* lit_parser_rulebinary(LitParser* parser, LitExpression* prev, bool can_assign)
{
    bool invert = parser->previous.type == LTOKEN_BANG;

    if(invert)
    {
        lit_parser_consume(parser, LTOKEN_IS, "'is' after '!'");
    }

    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;

    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);

    expression = (LitExpression*)lit_ast_makebinaryexpr(parser->state, line, prev, expression, op);

    if(invert)
    {
        expression = (LitExpression*)lit_ast_makeunaryexpr(parser->state, line, expression, LTOKEN_BANG);
    }

    return expression;
}

static LitExpression* lit_parser_rulelogicaland(LitParser* parser, LitExpression* prev, bool can_assign)
{
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;

    return (LitExpression*)lit_ast_makebinaryexpr(parser->state, line, prev, lit_parser_parseprec(parser, PREC_AND, true), op);
}

static LitExpression* lit_parser_rulelogicalor(LitParser* parser, LitExpression* prev, bool can_assign)
{
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;

    return (LitExpression*)lit_ast_makebinaryexpr(parser->state, line, prev, lit_parser_parseprec(parser, PREC_OR, true), op);
}

static LitExpression* lit_parser_rulenullfilter(LitParser* parser, LitExpression* prev, bool can_assign)
{
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;

    return (LitExpression*)lit_ast_makebinaryexpr(parser->state, line, prev, lit_parser_parseprec(parser, PREC_NULL, true), op);
}

static LitTokenType lit_parser_convertcompoundop(LitTokenType op)
{
    switch(op)
    {
        case LTOKEN_PLUS_EQUAL:
            return LTOKEN_PLUS;
        case LTOKEN_MINUS_EQUAL:
            return LTOKEN_MINUS;
        case LTOKEN_STAR_EQUAL:
            return LTOKEN_STAR;
        case LTOKEN_SLASH_EQUAL:
            return LTOKEN_SLASH;
        case LTOKEN_SHARP_EQUAL:
            return LTOKEN_SHARP;
        case LTOKEN_PERCENT_EQUAL:
            return LTOKEN_PERCENT;
        case LTOKEN_CARET_EQUAL:
            return LTOKEN_CARET;
        case LTOKEN_BAR_EQUAL:
            return LTOKEN_BAR;
        case LTOKEN_AMPERSAND_EQUAL:
            return LTOKEN_AMPERSAND;

        case LTOKEN_PLUS_PLUS:
            return LTOKEN_PLUS;
        case LTOKEN_MINUS_MINUS:
            return LTOKEN_MINUS;

        default:
        {
            UNREACHABLE
        }
    }

    return LTOKEN_EOF;
}

static LitExpression* lit_parser_rulecompound(LitParser* parser, LitExpression* prev, bool can_assign)
{
    LitTokenType op = parser->previous.type;
    LitUInt line = parser->previous.line;

    LitParseRule* rule = lit_parser_getrule(op);
    LitExpression* expression;

    if(op == LTOKEN_PLUS_PLUS || op == LTOKEN_MINUS_MINUS)
    {
        expression = (LitExpression*)lit_ast_makeliteralexpr(parser->state, line, NUMBER_VALUE(1));
    }
    else
    {
        expression = lit_parser_parseprec(parser, (LitPrecedence)(rule->precedence + 1), true);
    }

    LitBinaryExpression* binary = lit_ast_makebinaryexpr(parser->state, line, prev, expression, lit_parser_convertcompoundop(op));
    binary->ignore_left = true;// To make sure we don't free it twice

    return (LitExpression*)lit_ast_makeassignexpr(parser->state, line, prev, (LitExpression*)binary);
}

static LitExpression* lit_parser_ruleliteral(LitParser* parser, bool can_assign)
{
    LitUInt line = parser->previous.line;

    switch(parser->previous.type)
    {
        case LTOKEN_TRUE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(parser->state, line, TRUE_VALUE);
        }

        case LTOKEN_FALSE:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(parser->state, line, FALSE_VALUE);
        }

        case LTOKEN_NULL:
        {
            return (LitExpression*)lit_ast_makeliteralexpr(parser->state, line, NULL_VALUE);
        }

        default:
            UNREACHABLE
    }

    return NULL;
}

static LitExpression* lit_parser_rulestring(LitParser* parser, bool can_assign)
{
    LitExpression* expression = (LitExpression*)lit_ast_makeliteralexpr(parser->state, parser->previous.line, parser->previous.value);

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, can_assign);
    }

    return expression;
}

static LitExpression* lit_parser_ruleinterpolation(LitParser* parser, bool can_assign)
{
    LitInterpolationExpression* expression = lit_ast_makeinterpolationexpr(parser->state, parser->previous.line);

    do
    {
        if(AS_STRING(parser->previous.value)->length > 0)
        {
            lit_exprlist_push(parser->state, &expression->expressions,
                                  (LitExpression*)lit_ast_makeliteralexpr(parser->state, parser->previous.line, parser->previous.value));
        }

        lit_exprlist_push(parser->state, &expression->expressions, lit_parser_parseexpr(parser));
    } while(lit_parser_match(parser, LTOKEN_INTERPOLATION));

    lit_parser_consume(parser, LTOKEN_STRING, "end of interpolation");

    if(AS_STRING(parser->previous.value)->length > 0)
    {
        lit_exprlist_push(parser->state, &expression->expressions,
                              (LitExpression*)lit_ast_makeliteralexpr(parser->state, parser->previous.line, parser->previous.value));
    }

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, (LitExpression*)expression, can_assign);
    }

    return (LitExpression*)expression;
}

static LitExpression* lit_parser_ruleobject(LitParser* parser, bool can_assign)
{
    LitObjectExpression* object = lit_ast_makeobjectexpr(parser->state, parser->previous.line);
    lit_parser_ignorelinefeeds(parser);

    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "key string after '{'");
        lit_vallist_push(parser->state, &object->keys, OBJECT_VALUE(lit_string_copy(parser->state, parser->previous.start, parser->previous.length)));

        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_COLON, "':' after key string");

        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(parser->state, &object->values, lit_parser_parseexpr(parser));

        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }

    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after object");

    return (LitExpression*)object;
}

static LitExpression* lit_parser_parsevarexprbase(LitParser* parser, bool can_assign, bool new)
{
    LitExpression* expression = (LitExpression*)lit_ast_makevarexpr(parser->state, parser->previous.line, parser->previous.start, parser->previous.length);

    if(new)
    {
        bool hadargs = lit_parser_check(parser, LTOKEN_LEFT_PAREN);
        LitCallExpression* call = NULL;

        if(hadargs)
        {
            lit_parser_advance(parser);
            call = (LitCallExpression*)lit_parser_parsecall(parser, expression, false);
        }

        if(lit_parser_match(parser, LTOKEN_LEFT_BRACE))
        {
            if(call == NULL)
            {
                call = lit_ast_makecallexpr(parser->state, expression->line, expression);
            }

            call->init = lit_parser_ruleobject(parser, false);
        }
        else if(!hadargs)
        {
            lit_parser_failherefmt(parser, "Expected %s, got '%.*s'", "argument list for instance creation", parser->previous.length, parser->previous.start);
        }

        return (LitExpression*)call;
    }

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, can_assign);
    }

    if(can_assign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->state, parser->previous.line, expression, lit_parser_parseexpr(parser));
    }

    return expression;
}

static LitExpression* lit_parser_rulevarexpr(LitParser* parser, bool can_assign)
{
    return lit_parser_parsevarexprbase(parser, can_assign, false);
}

static LitExpression* lit_parser_rulenewexpr(LitParser* parser, bool can_assign)
{
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "class name after 'new'");
    return lit_parser_parsevarexprbase(parser, false, true);
}

static LitExpression* lit_parser_ruledot(LitParser* parser, LitExpression* previous, bool can_assign)
{
    LitUInt line = parser->previous.line;
    bool ignored = parser->previous.type == LTOKEN_SMALL_ARROW;

    if(!(lit_parser_match(parser, LTOKEN_CLASS) || lit_parser_match(parser, LTOKEN_SUPER)))
    {// class and super are allowed field names
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignored ? "propety name after '->'" : "property name after '.'");
    }

    const char* name = parser->previous.start;
    LitUInt length = parser->previous.length;

    if(!ignored && can_assign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makesetexpr(parser->state, line, previous, name, length, lit_parser_parseexpr(parser));
    }
    else
    {
        LitExpression* expression = (LitExpression*)lit_ast_makegetexpr(parser->state, line, previous, name, length, false, ignored);

        if(!ignored && lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
        {
            return lit_parser_parsesubscript(parser, expression, can_assign);
        }

        return expression;
    }
}

static LitExpression* lit_parser_rulerange(LitParser* parser, LitExpression* previous, bool can_assign)
{
    LitUInt line = parser->previous.line;
    return (LitExpression*)lit_ast_makerangeexpr(parser->state, line, previous, lit_parser_parseexpr(parser));
}

static LitExpression* lit_parser_ruleternaryorquestion(LitParser* parser, LitExpression* previous, bool can_assign)
{
    LitUInt line = parser->previous.line;

    if(lit_parser_match(parser, LTOKEN_DOT) /* || lit_parser_match(parser, LTOKEN_SMALL_ARROW)*/)
    {
        bool ignored = parser->previous.type == LTOKEN_SMALL_ARROW;
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignored ? "property name after '->'" : "property name after '.'");
        return (LitExpression*)lit_ast_makegetexpr(parser->state, line, previous, parser->previous.start, parser->previous.length, true, ignored);
    }

    LitExpression* if_branch = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_COLON, "':' after expression");
    LitExpression* else_branch = lit_parser_parseexpr(parser);

    return (LitExpression*)lit_ast_maketernaryexpr(parser->state, line, previous, if_branch, else_branch);
}

static LitExpression* lit_parser_rulearray(LitParser* parser, bool can_assign)
{
    LitArrayExpression* array = lit_ast_makearrayexpr(parser->state, parser->previous.line);
    lit_parser_ignorelinefeeds(parser);

    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACKET))
    {
        lit_parser_ignorelinefeeds(parser);
        lit_exprlist_push(parser->state, &array->values, lit_parser_parseexpr(parser));

        if(!lit_parser_match(parser, LTOKEN_COMMA))
        {
            break;
        }
    }

    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after array");

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, (LitExpression*)array, can_assign);
    }

    return (LitExpression*)array;
}

static LitExpression* lit_parser_parsesubscript(LitParser* parser, LitExpression* previous, bool can_assign)
{
    LitUInt line = parser->previous.line;

    LitExpression* index = lit_parser_parseexpr(parser);
    lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after subscript");

    LitExpression* expression = (LitExpression*)lit_ast_makesubscriptexpr(parser->state, line, previous, index);

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, can_assign);
    }
    else if(can_assign && lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->state, parser->previous.line, expression, lit_parser_parseexpr(parser));
    }

    return expression;
}

static LitExpression* lit_parser_rulethis(LitParser* parser, bool can_assign)
{
    LitExpression* expression = (LitExpression*)lit_ast_makethisexpr(parser->state, parser->previous.line);

    if(lit_parser_match(parser, LTOKEN_LEFT_BRACKET))
    {
        return lit_parser_parsesubscript(parser, expression, can_assign);
    }

    return expression;
}

static LitExpression* lit_parser_rulesuper(LitParser* parser, bool can_assign)
{
    LitUInt line = parser->previous.line;

    if(!(lit_parser_match(parser, LTOKEN_DOT) || lit_parser_match(parser, LTOKEN_SMALL_ARROW)))
    {
        LitExpression* expression = (LitExpression*)lit_ast_makesuperexpr(parser->state, line, lit_string_copy(parser->state, "constructor", 11), false);
        lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after 'super'");

        return lit_parser_parsecall(parser, expression, false);
    }

    bool ignoring = parser->previous.type == LTOKEN_SMALL_ARROW;
    lit_parser_consume(parser, LTOKEN_IDENTIFIER, ignoring ? "super method name after '->'" : "super method name after '.'");

    LitExpression* expression
    = (LitExpression*)lit_ast_makesuperexpr(parser->state, line, lit_string_copy(parser->state, parser->previous.start, parser->previous.length), ignoring);

    if(lit_parser_match(parser, LTOKEN_LEFT_PAREN))
    {
        return lit_parser_parsecall(parser, expression, false);
    }

    return expression;
}


static LitExpression *lit_parser_rulenothing(LitParser *parser, bool canassign)
{
    (void)canassign;
    return NULL;
}


static LitExpression *lit_parser_rulefunction(LitParser *parser, bool canassign)
{
    (void)canassign;
    return lit_parser_parsefunction(parser);
}


static LitExpression* lit_parser_rulereference(LitParser* parser, bool can_assign)
{
    LitUInt line = parser->previous.line;
    lit_parser_ignorelinefeeds(parser);

    LitReferenceExpression* expression = lit_ast_makerefexpr(parser->state, line, lit_parser_parseprec(parser, PREC_CALL, false));

    if(lit_parser_match(parser, LTOKEN_EQUAL))
    {
        return (LitExpression*)lit_ast_makeassignexpr(parser->state, line, (LitExpression*)expression, lit_parser_parseexpr(parser));
    }

    return (LitExpression*)expression;
}

static LitExpression* lit_parser_parseexpr(LitParser* parser)
{
    lit_parser_ignorelinefeeds(parser);
    return lit_parser_parseprec(parser, PREC_ASSIGNMENT, true);
}

static LitExpression* lit_parser_parsevardecl(LitParser* parser)
{
    bool constant = parser->previous.type == LTOKEN_CONST;
    LitUInt line = parser->previous.line;

    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "variable name");

    const char* name = parser->previous.start;
    LitUInt length = parser->previous.length;

    LitExpression* init = NULL;

    if(lit_parser_match(parser, LTOKEN_EQUAL))
    {
        init = lit_parser_parseexpr(parser);
    }

    return (LitExpression*)lit_ast_makevardefstmt(parser->state, line, name, length, init, constant);
}

static LitExpression* lit_parser_parseif(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    bool invert = lit_parser_match(parser, LTOKEN_BANG);

    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);

    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }

    if(invert)
    {
        condition = (LitExpression*)lit_ast_makeunaryexpr(parser->state, condition->line, condition, LTOKEN_BANG);
    }
    lit_parser_ignorelinefeeds(parser);

    LitExpression* if_branch = lit_parser_parsestmt(parser);

    LitExprList* elseif_conditions = NULL;
    LitExprList* elseif_branches = NULL;
    LitExpression* else_branch = NULL;
    lit_parser_ignorelinefeeds(parser);
    while(lit_parser_match(parser, LTOKEN_ELSE))
    {
        // else if
        if(lit_parser_match(parser, LTOKEN_IF))
        {
            if(elseif_conditions == NULL)
            {
                elseif_conditions = lit_ast_allocexprlist(parser->state);
                elseif_branches = lit_ast_allocstmtlist(parser->state);
            }

            invert = lit_parser_match(parser, LTOKEN_BANG);
            hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
            LitExpression* e = lit_parser_parseexpr(parser);

            if(hadparen)
            {
                lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
            }

            if(invert)
            {
                e = (LitExpression*)lit_ast_makeunaryexpr(parser->state, condition->line, e, LTOKEN_BANG);
            }

            lit_exprlist_push(parser->state, elseif_conditions, e);
            lit_parser_ignorelinefeeds(parser);
            lit_stmtlist_push(parser->state, elseif_branches, lit_parser_parsestmt(parser));
            lit_parser_ignorelinefeeds(parser);
            continue;
        }

        // else
        if(else_branch != NULL)
        {
            lit_parser_failfmt(parser, "If-statement can have only one else-branch");
        }

        else_branch = lit_parser_parsestmt(parser);
    }

    return (LitExpression*)lit_ast_makeifstatement(parser->state, line, condition, if_branch, else_branch, elseif_conditions, elseif_branches);
}

static LitExpression* lit_parser_parsefor(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);

    LitExpression* var = NULL;
    LitExpression* init = NULL;

    if(!lit_parser_check(parser, LTOKEN_SEMICOLON))
    {
        if(lit_parser_match(parser, LTOKEN_VAR))
        {
            var = lit_parser_parsevardecl(parser);
        }
        else
        {
            init = lit_parser_parseexpr(parser);
        }
    }

    bool c_style = !lit_parser_match(parser, LTOKEN_IN);
    LitExpression* condition = NULL;
    LitExpression* increment = NULL;

    if(c_style)
    {
        lit_parser_consume(parser, LTOKEN_SEMICOLON, "';'");
        condition = lit_parser_check(parser, LTOKEN_SEMICOLON) ? NULL : lit_parser_parseexpr(parser);

        lit_parser_consume(parser, LTOKEN_SEMICOLON, "';'");
        increment = lit_parser_check(parser, LTOKEN_RIGHT_PAREN) ? NULL : lit_parser_parseexpr(parser);
    }
    else
    {
        condition = lit_parser_parseexpr(parser);

        if(var == NULL)
        {
            lit_parser_failfmt(parser, "For-loops using in-iteration must declare a new variable");
        }
    }

    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }

    return (LitExpression*)lit_ast_makeforstmt(parser->state, line, init, var, condition, increment, lit_parser_parsestmt(parser), c_style);
}

static LitExpression* lit_parser_parsewhile(LitParser* parser)
{
    LitUInt line = parser->previous.line;

    bool hadparen = lit_parser_match(parser, LTOKEN_LEFT_PAREN);
    LitExpression* condition = lit_parser_parseexpr(parser);

    if(hadparen)
    {
        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')'");
    }

    LitExpression* body = lit_parser_parsestmt(parser);

    return (LitExpression*)lit_ast_makewhilestmt(parser->state, line, condition, body);
}

static LitExpression* lit_parser_parsefunction(LitParser* parser)
{
    LitUInt line;
    LitUInt namelen;
    bool export;
    bool noname;
    const char* fnname;
    noname = false;
    fnname = "anonymous";
    namelen = strlen(fnname);
    export = parser->previous.type == LTOKEN_EXPORT;

    if(export)
    {
        lit_parser_consume(parser, LTOKEN_FUNCTION, "'function' after 'export'");
    }

    line = parser->previous.line;
    if(lit_parser_check(parser, LTOKEN_IDENTIFIER))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "function name");
        fnname = parser->previous.start;
        namelen = parser->previous.length;
    }
    else
    {
        noname = true;
    }
    if(lit_parser_match(parser, LTOKEN_DOT))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "function name");

        LitFunctionStatement* lambda = lit_ast_makelambdaexpr(parser->state, line);
        LitSetExpression* to = lit_ast_makesetexpr(parser->state, line, (LitExpression*)lit_ast_makevarexpr(parser->state, line, fnname, namelen),
                                                         parser->previous.start, parser->previous.length, (LitExpression*)lambda);

        lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after function name");

        LitCompiler compiler;
        lit_parser_compilerinit(parser, &compiler);
        lit_parser_scopebegin(parser);

        lit_parser_parseparams(parser, &lambda->parameters);

        if(lambda->parameters.count > 255)
        {
            lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)lambda->parameters.count);
        }

        lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after function arguments");
        lambda->body = lit_parser_parsestmt(parser);

        lit_parser_scopeend(parser);
        lit_parser_compilerend(parser, &compiler);

        return (LitExpression*)lit_ast_makeexprstmt(parser->state, line, (LitExpression*)to);
    }

    LitFunctionStatement* function;
    
    if(noname)
    {
        function = lit_ast_makelambdaexpr(parser->state, line);
    }
    else
    {
        function = lit_ast_makefuncdefstmt(parser->state, line, fnname, namelen);
    }
    function->exported = export;

    lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after function name");

    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);

    lit_parser_parseparams(parser, &function->parameters);

    if(function->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)function->parameters.count);
    }

    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after function arguments");

    function->body = lit_parser_parsestmt(parser);

    lit_parser_scopeend(parser);
    lit_parser_compilerend(parser, &compiler);

    return (LitExpression*)function;
}

static LitExpression* lit_parser_parsereturn(LitParser* parser)
{
    LitUInt line = parser->previous.line;
    LitExpression* expression = NULL;

    if(!lit_parser_check(parser, LTOKEN_NEW_LINE) && !lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        expression = lit_parser_parseexpr(parser);
    }

    return (LitExpression*)lit_ast_makereturnstmt(parser->state, line, expression);
}


static LitExpression* lit_parser_parsefield(LitParser* parser, LitString* name, bool is_static)
{
    LitUInt line = parser->previous.line;

    LitExpression* getter = NULL;
    LitExpression* setter = NULL;

    if(lit_parser_match(parser, LTOKEN_ARROW))
    {
        getter = lit_parser_parsestmt(parser);
    }
    else
    {
        lit_parser_match(parser, LTOKEN_LEFT_BRACE);// Will be LTOKEN_LEFT_BRACE, otherwise this method won't be called
        lit_parser_ignorelinefeeds(parser);

        if(lit_parser_matchident(parser, "get"))
        {
            lit_parser_match(parser, LTOKEN_ARROW);// Ignore it if it's present
            getter = lit_parser_parsestmt(parser);
        }

        lit_parser_ignorelinefeeds(parser);

        if(lit_parser_matchident(parser, "set"))
        {
            lit_parser_match(parser, LTOKEN_ARROW);// Ignore it if it's present
            setter = lit_parser_parsestmt(parser);
        }

        if(getter == NULL && setter == NULL)
        {
            lit_parser_failfmt(parser, "Expected declaration of either getter or setter, got none");
        }

        lit_parser_ignorelinefeeds(parser);
        lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after field declaration");
    }

    return (LitExpression*)lit_ast_makefieldstmt(parser->state, line, name, getter, setter, is_static);
}

static LitTokenType operators[] = { LTOKEN_PLUS,         LTOKEN_MINUS, LTOKEN_STAR,       LTOKEN_PERCENT, LTOKEN_SLASH,         LTOKEN_SHARP,

                                    LTOKEN_BANG,         LTOKEN_LESS,  LTOKEN_LESS_EQUAL, LTOKEN_GREATER, LTOKEN_GREATER_EQUAL, LTOKEN_EQUAL_EQUAL,

                                    LTOKEN_LEFT_BRACKET,

                                    LTOKEN_EOF };

static LitExpression* lit_parser_parsemethod(LitParser* parser, bool is_static)
{
    if(lit_parser_match(parser, LTOKEN_STATIC))
    {
        is_static = true;
    }

    LitString* name = NULL;

    if(lit_parser_match(parser, LTOKEN_OPERATOR))
    {
        if(is_static)
        {
            lit_parser_failfmt(parser, "Operator methods can't be static or defined in static classes");
        }

        LitUInt i = 0;

        while(operators[i] != LTOKEN_EOF)
        {
            if(lit_parser_match(parser, operators[i]))
            {
                break;
            }

            i++;
        }

        if(parser->previous.type == LTOKEN_LEFT_BRACKET)
        {
            lit_parser_consume(parser, LTOKEN_RIGHT_BRACKET, "']' after '[' in op method declaration");
            name = lit_string_copy(parser->state, "[]", 2);
        }
        else
        {
            name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
        }
    }
    else
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "method name");
        name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);

        if(lit_parser_check(parser, LTOKEN_LEFT_BRACE) || lit_parser_check(parser, LTOKEN_ARROW))
        {
            return lit_parser_parsefield(parser, name, is_static);
        }
    }

    LitMethodStatement* method = lit_ast_makemethoddefstmt(parser->state, parser->previous.line, name, is_static);

    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);
    lit_parser_scopebegin(parser);

    lit_parser_consume(parser, LTOKEN_LEFT_PAREN, "'(' after method name");
    lit_parser_parseparams(parser, &method->parameters);

    if(method->parameters.count > 255)
    {
        lit_parser_failfmt(parser, "Function can't have more than 255 arguments, got %i", (int)method->parameters.count);
    }

    lit_parser_consume(parser, LTOKEN_RIGHT_PAREN, "')' after method arguments");

    method->body = lit_parser_parsestmt(parser);

    lit_parser_scopeend(parser);
    lit_parser_compilerend(parser, &compiler);

    return (LitExpression*)method;
}

static LitExpression* lit_parser_parseclass(LitParser* parser)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }

    LitUInt line = parser->previous.line;

    bool is_static = parser->previous.type == LTOKEN_STATIC;

    if(is_static)
    {
        lit_parser_consume(parser, LTOKEN_CLASS, "'class' after 'static'");
    }

    lit_parser_consume(parser, LTOKEN_IDENTIFIER, "class name after 'class'");
    LitString* name = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);
    LitString* super = NULL;

    if(lit_parser_match(parser, LTOKEN_COLON))
    {
        lit_parser_consume(parser, LTOKEN_IDENTIFIER, "super class name after ':'");
        super = lit_string_copy(parser->state, parser->previous.start, parser->previous.length);

        if(super == name)
        {
            lit_parser_failfmt(parser, "Class can't inherit itself");
        }
    }

    LitClassStatement* klass = lit_ast_makeclassdefstmt(parser->state, line, name, super);

    lit_parser_ignorelinefeeds(parser);
    lit_parser_consume(parser, LTOKEN_LEFT_BRACE, "'{' before class body");
    lit_parser_ignorelinefeeds(parser);

    bool finishedparsingfields = false;

    while(!lit_parser_check(parser, LTOKEN_RIGHT_BRACE))
    {
        bool fieldisstatic = false;

        if(lit_parser_match(parser, LTOKEN_STATIC))
        {
            fieldisstatic = true;

            if(lit_parser_match(parser, LTOKEN_VAR))
            {
                if(finishedparsingfields)
                {
                    lit_parser_failfmt(parser, "All static fields must be defined before the methods");
                }

                LitExpression* var = lit_parser_parsevardecl(parser);

                if(var != NULL)
                {
                    lit_stmtlist_push(parser->state, &klass->fields, var);
                }

                lit_parser_ignorelinefeeds(parser);
                continue;
            }
            else
            {
                finishedparsingfields = true;
            }
        }

        LitExpression* method = lit_parser_parsemethod(parser, is_static || fieldisstatic);

        if(method != NULL)
        {
            lit_stmtlist_push(parser->state, &klass->fields, method);
        }

        lit_parser_ignorelinefeeds(parser);
    }

    lit_parser_consume(parser, LTOKEN_RIGHT_BRACE, "'}' after class body");
    return (LitExpression*)klass;
}

static void lit_parser_sync(LitParser* parser)
{
    parser->panic_mode = false;

    while(parser->current.type != LTOKEN_EOF)
    {
        if(parser->previous.type == LTOKEN_NEW_LINE)
        {
            longjmp(jumpbuffer, 1);
            return;
        }

        switch(parser->current.type)
        {
            case LTOKEN_CLASS:
            case LTOKEN_FUNCTION:
            case LTOKEN_EXPORT:
            case LTOKEN_VAR:
            case LTOKEN_CONST:
            case LTOKEN_FOR:
            case LTOKEN_STATIC:
            case LTOKEN_IF:
            case LTOKEN_WHILE:
            case LTOKEN_RETURN:
            {
                longjmp(jumpbuffer, 1);
                return;
            }

            default:
            {
                lit_parser_advance(parser);
            }
        }
    }
}

static LitExpression* lit_parser_parsestmt(LitParser* parser)
{
    if(setjmp(jumpbuffer))
    {
        return NULL;
    }
    lit_parser_ignorelinefeeds(parser);
    if(lit_parser_match(parser, LTOKEN_VAR) || lit_parser_match(parser, LTOKEN_CONST))
    {
        return lit_parser_parsevardecl(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_IF))
    {
        return lit_parser_parseif(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_FOR))
    {
        return lit_parser_parsefor(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_WHILE))
    {
        return lit_parser_parsewhile(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_CONTINUE))
    {
        return (LitExpression*)lit_ast_makecontinuestmt(parser->state, parser->previous.line);
    }
    else if(lit_parser_match(parser, LTOKEN_BREAK))
    {
        return (LitExpression*)lit_ast_makebreakstmt(parser->state, parser->previous.line);
    }
    else if(lit_parser_match(parser, LTOKEN_FUNCTION) || lit_parser_match(parser, LTOKEN_EXPORT))
    {
        return lit_parser_parsefunction(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_RETURN))
    {
        return lit_parser_parsereturn(parser);
    }
    else if(lit_parser_match(parser, LTOKEN_LEFT_BRACE))
    {
        lit_parser_ignorelinefeeds(parser);
        return lit_parser_parseblock(parser);
    }

    LitExpression* expression = lit_parser_parseexpr(parser);
    return expression == NULL ? NULL : (LitExpression*)lit_ast_makeexprstmt(parser->state, parser->previous.line, expression);
}

static LitExpression* lit_parser_parsedecl(LitParser* parser)
{
    LitExpression* statement = NULL;

    if(lit_parser_match(parser, LTOKEN_CLASS) || lit_parser_match(parser, LTOKEN_STATIC))
    {
        statement = lit_parser_parseclass(parser);
    }
    else
    {
        statement = lit_parser_parsestmt(parser);
    }

    return statement;
}

bool lit_parser_parsesource(LitParser* parser, const char* file_name, const char* source, LitExprList* statements)
{
    parser->had_error = false;
    parser->panic_mode = false;

    lit_scanner_init(parser->state, parser->state->scanner, file_name, source);

    LitCompiler compiler;
    lit_parser_compilerinit(parser, &compiler);

    lit_parser_advance(parser);
    lit_parser_ignorelinefeeds(parser);

    if(!lit_scanner_isatend(parser))
    {
        do
        {
            LitExpression* statement = lit_parser_parsedecl(parser);

            if(statement != NULL)
            {
                lit_stmtlist_push(parser->state, statements, statement);
            }

            if(!lit_parser_matchlinefeed(parser))
            {
                if(lit_parser_match(parser, LTOKEN_EOF))
                {
                    break;
                }
            }
        } while(!lit_scanner_isatend(parser));
    }

    return parser->had_error || parser->state->scanner->had_error;
}

static void lit_parser_setuprules()
{
    rules[LTOKEN_LEFT_PAREN] = (LitParseRule){ lit_parser_rulegroupingorlambda, lit_parser_parsecall, PREC_CALL };
    rules[LTOKEN_PLUS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_MINUS] = (LitParseRule){ lit_parser_ruleunary, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_BANG] = (LitParseRule){ lit_parser_ruleunary, lit_parser_rulebinary, PREC_TERM };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_SLASH] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_SHARP] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_STAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_BAR] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BOR };
    rules[LTOKEN_AMPERSAND] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BAND };
    rules[LTOKEN_TILDE] = (LitParseRule){ lit_parser_ruleunary, NULL, PREC_UNARY };
    rules[LTOKEN_CARET] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_BOR };
    rules[LTOKEN_LESS_LESS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_SHIFT };
    rules[LTOKEN_GREATER_GREATER] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_SHIFT };

    rules[LTOKEN_PERCENT] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_FACTOR };
    rules[LTOKEN_IS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_IS };
    rules[LTOKEN_NUMBER] = (LitParseRule){ lit_parser_rulenumber, NULL, PREC_NONE };
    rules[LTOKEN_TRUE] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_FALSE] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_NULL] = (LitParseRule){ lit_parser_ruleliteral, NULL, PREC_NONE };
    rules[LTOKEN_BANG_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_EQUALITY };
    rules[LTOKEN_EQUAL_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_EQUALITY };
    rules[LTOKEN_GREATER] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_GREATER_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_LESS] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_LESS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulebinary, PREC_COMPARISON };
    rules[LTOKEN_STRING] = (LitParseRule){ lit_parser_rulestring, NULL, PREC_NONE };
    rules[LTOKEN_INTERPOLATION] = (LitParseRule){ lit_parser_ruleinterpolation, NULL, PREC_NONE };
    rules[LTOKEN_IDENTIFIER] = (LitParseRule){ lit_parser_rulevarexpr, NULL, PREC_NONE };
    rules[LTOKEN_NEW] = (LitParseRule){ lit_parser_rulenewexpr, NULL, PREC_NONE };
    rules[LTOKEN_PLUS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_MINUS_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_STAR_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_SLASH_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_SHARP_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_PERCENT_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_CARET_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_BAR_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_AMPERSAND_EQUAL] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_PLUS_PLUS] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_MINUS_MINUS] = (LitParseRule){ NULL, lit_parser_rulecompound, PREC_COMPOUND };
    rules[LTOKEN_AMPERSAND_AMPERSAND] = (LitParseRule){ NULL, lit_parser_rulelogicaland, PREC_AND };
    rules[LTOKEN_BAR_BAR] = (LitParseRule){ NULL, lit_parser_rulelogicalor, PREC_AND };
    rules[LTOKEN_QUESTION_QUESTION] = (LitParseRule){ NULL, lit_parser_rulenullfilter, PREC_NULL };
    rules[LTOKEN_DOT] = (LitParseRule){ NULL, lit_parser_ruledot, PREC_CALL };
    // rules[LTOKEN_SMALL_ARROW] = (LitParseRule) { NULL, lit_parser_ruledot, PREC_CALL };
    rules[LTOKEN_DOT_DOT] = (LitParseRule){ NULL, lit_parser_rulerange, PREC_RANGE };
    rules[LTOKEN_DOT_DOT_DOT] = (LitParseRule){ lit_parser_rulevarexpr, NULL, PREC_ASSIGNMENT };
    rules[LTOKEN_LEFT_BRACKET] = (LitParseRule){ lit_parser_rulearray, lit_parser_parsesubscript, PREC_NONE };
    rules[LTOKEN_LEFT_BRACE] = (LitParseRule){ lit_parser_ruleobject, NULL, PREC_NONE };
    rules[LTOKEN_THIS] = (LitParseRule){ lit_parser_rulethis, NULL, PREC_NONE };
    rules[LTOKEN_SUPER] = (LitParseRule){ lit_parser_rulesuper, NULL, PREC_NONE };
    rules[LTOKEN_QUESTION] = (LitParseRule){ NULL, lit_parser_ruleternaryorquestion, PREC_EQUALITY };
    rules[LTOKEN_REF] = (LitParseRule){ lit_parser_rulereference, NULL, PREC_NONE };
    rules[LTOKEN_SEMICOLON] = (LitParseRule){lit_parser_rulenothing, NULL, PREC_NONE};
    rules[LTOKEN_FUNCTION] = (LitParseRule){lit_parser_rulefunction, NULL, PREC_NONE};
 
}