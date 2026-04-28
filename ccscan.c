
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "lit.h"


void lit_scanner_init(LitState* state, LitScanner* scanner, const char* file_name, const char* source)
{
    scanner->line = 1;
    scanner->start = source;
    scanner->current = source;
    scanner->file_name = file_name;
    scanner->state = state;
    scanner->num_braces = 0;
    scanner->had_error = false;
}

static LitToken lit_scanner_maketoken(LitScanner* scanner, LitTokenType type)
{
    LitToken token;

    token.type = type;
    token.start = scanner->start;
    token.length = (LitUInt)(scanner->current - scanner->start);
    token.line = scanner->line;

    return token;
}

static LitToken lit_scanner_makeerrortoken(LitScanner* scanner, const char* fmt, ...)
{
    scanner->had_error = true;

    va_list args;
    va_start(args, fmt);
    LitString* result = lit_state_errorfmtv(scanner->state, scanner->line, fmt, args);
    va_end(args);

    LitToken token;

    token.type = LTOKEN_ERROR;
    token.start = result->chars;
    token.length = result->length;
    token.line = scanner->line;

    return token;
}

static bool lit_scanner_isatend(LitScanner* scanner)
{
    return *scanner->current == '\0';
}

static char lit_scanner_advance(LitScanner* scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

static bool lit_scanner_match(LitScanner* scanner, char expected)
{
    if(lit_scanner_isatend(scanner))
    {
        return false;
    }

    if(*scanner->current != expected)
    {
        return false;
    }

    scanner->current++;
    return true;
}

static LitToken lit_scanner_matchtoken(LitScanner* scanner, char c, LitTokenType a, LitTokenType b)
{
    return lit_scanner_maketoken(scanner, lit_scanner_match(scanner, c) ? a : b);
}

static LitToken lit_scanner_matchtokens(LitScanner* scanner, char cr, char cb, LitTokenType a, LitTokenType b, LitTokenType c)
{
    return lit_scanner_maketoken(scanner, lit_scanner_match(scanner, cr) ? a : (lit_scanner_match(scanner, cb) ? b : c));
}

static char lit_scanner_peek(LitScanner* scanner)
{
    return *scanner->current;
}

static char lit_scanner_peeknext(LitScanner* scanner)
{
    if(lit_scanner_isatend(scanner))
    {
        return '\0';
    }

    return scanner->current[1];
}

static bool lit_scanner_skipwhitespace(LitScanner* scanner)
{
    while(true)
    {
        char c = lit_scanner_peek(scanner);

        switch(c)
        {
            case 1:
            case 2:
            case 3:
            case ' ':
            case '\r':
            case '\t':
            {
                lit_scanner_advance(scanner);
                break;
            }

            case '\n':
            {
                scanner->start = scanner->current;
                lit_scanner_advance(scanner);

                return true;
            }

            case '/':
            {
                if(lit_scanner_peeknext(scanner) == '/')
                {
                    while(lit_scanner_peek(scanner) != '\n' && !lit_scanner_isatend(scanner))
                    {
                        lit_scanner_advance(scanner);
                    }

                    return lit_scanner_skipwhitespace(scanner);
                }
                else if(lit_scanner_peeknext(scanner) == '*')
                {
                    lit_scanner_advance(scanner);
                    lit_scanner_advance(scanner);

                    while((lit_scanner_peek(scanner) != '*' || lit_scanner_peeknext(scanner) != '/') && !lit_scanner_isatend(scanner))
                    {
                        if(lit_scanner_peek(scanner) == '\n')
                        {
                            scanner->line++;
                        }

                        lit_scanner_advance(scanner);
                    }

                    lit_scanner_advance(scanner);
                    lit_scanner_advance(scanner);

                    return lit_scanner_skipwhitespace(scanner);
                }

                return false;
            }

            default:
                return false;
        }
    }
}

static LitToken lit_scanner_scanstring(LitScanner* scanner, bool interpolation)
{
    LitState* state = scanner->state;
    LitTokenType stringtype = LTOKEN_STRING;

    LitByteList bytes;
    lit_bytelist_init(&bytes);

    while(true)
    {
        char c = lit_scanner_advance(scanner);

        if(c == '\"')
        {
            break;
        }
        else if(interpolation && c == '{')
        {
            if(scanner->num_braces >= LIT_INTERPOLATION_NESTING_MAX)
            {
                return lit_scanner_makeerrortoken(scanner, "Interpolation nesting is too deep, maximum is %i", LIT_INTERPOLATION_NESTING_MAX);
            }

            stringtype = LTOKEN_INTERPOLATION;
            scanner->braces[scanner->num_braces++] = 1;

            break;
        }

        switch(c)
        {
            case '\0':
                return lit_scanner_makeerrortoken(scanner, "Unterminated string");

            case '\n':
            {
                scanner->line++;
                lit_bytelist_push(state, &bytes, c);

                break;
            }

            case '\\':
            {
                switch(lit_scanner_advance(scanner))
                {
                    case '\"':
                        lit_bytelist_push(state, &bytes, '\"');
                        break;
                    case '\\':
                        lit_bytelist_push(state, &bytes, '\\');
                        break;
                    case '0':
                        lit_bytelist_push(state, &bytes, '\0');
                        break;
                    case '{':
                        lit_bytelist_push(state, &bytes, '{');
                        break;
                    case 'a':
                        lit_bytelist_push(state, &bytes, '\a');
                        break;
                    case 'b':
                        lit_bytelist_push(state, &bytes, '\b');
                        break;
                    case 'f':
                        lit_bytelist_push(state, &bytes, '\f');
                        break;
                    case 'n':
                        lit_bytelist_push(state, &bytes, '\n');
                        break;
                    case 'r':
                        lit_bytelist_push(state, &bytes, '\r');
                        break;
                    case 't':
                        lit_bytelist_push(state, &bytes, '\t');
                        break;
                    case 'v':
                        lit_bytelist_push(state, &bytes, '\v');
                        break;
                    case 'e':
                        lit_bytelist_push(state, &bytes, 27);
                        break;
                    default:
                    {
                        return lit_scanner_makeerrortoken(scanner, "Invalid escape character '%c'", scanner->current[-1]);
                    }
                }

                break;
            }

            default:
            {
                lit_bytelist_push(state, &bytes, c);
                break;
            }
        }
    }

    LitToken token = lit_scanner_maketoken(scanner, stringtype);
    token.value = OBJECT_VALUE(lit_string_copy(state, (const char*)bytes.values, bytes.count));
    lit_bytelist_destroy(state, &bytes);

    return token;
}

static int lit_scanner_scanhexdigit(LitScanner* scanner)
{
    char c = lit_scanner_advance(scanner);

    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }

    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    scanner->current--;
    return -1;
}

static int lit_scanner_scanbinarydigit(LitScanner* scanner)
{
    char c = lit_scanner_advance(scanner);

    if(c >= '0' && c <= '1')
    {
        return c - '0';
    }

    scanner->current--;
    return -1;
}

static LitToken lit_scanner_makenumbertoken(LitScanner* scanner, bool ishex, bool isbinary)
{
    errno = 0;
    LitValue value;

    if(ishex)
    {
        value = NUMBER_VALUE((double)strtoll(scanner->start, NULL, 16));
    }
    else if(isbinary)
    {
        value = NUMBER_VALUE((int)strtoll(scanner->start + 2, NULL, 2));
    }
    else
    {
        value = NUMBER_VALUE(strtod(scanner->start, NULL));
    }

    if(errno == ERANGE)
    {
        errno = 0;
        return lit_scanner_makeerrortoken(scanner, "Number is too big to be represented by a single literal");
    }

    LitToken token = lit_scanner_maketoken(scanner, LTOKEN_NUMBER);
    token.value = value;
    return token;
}

static LitToken lit_scanner_scannumber(LitScanner* scanner)
{
    if(lit_scanner_match(scanner, 'x'))
    {
        while(lit_scanner_scanhexdigit(scanner) != -1)
        {
            continue;
        }

        return lit_scanner_makenumbertoken(scanner, true, false);
    }

    if(lit_scanner_match(scanner, 'b'))
    {
        while(lit_scanner_scanbinarydigit(scanner) != -1)
        {
            continue;
        }

        return lit_scanner_makenumbertoken(scanner, false, true);
    }

    while(lit_is_digit(lit_scanner_peek(scanner)))
    {
        lit_scanner_advance(scanner);
    }

    // Look for a fractional part.
    if(lit_scanner_peek(scanner) == '.' && lit_is_digit(lit_scanner_peeknext(scanner)))
    {
        // Consume the '.'
        lit_scanner_advance(scanner);

        while(lit_is_digit(lit_scanner_peek(scanner)))
        {
            lit_scanner_advance(scanner);
        }
    }

    return lit_scanner_makenumbertoken(scanner, false, false);
}

static LitTokenType lit_scanner_checkkeyword(LitScanner* scanner, int start, int length, const char* rest, LitTokenType type)
{
    if(scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0)
    {
        return type;
    }

    return LTOKEN_IDENTIFIER;
}

static LitTokenType lit_scanner_scanidenttype(LitScanner* scanner)
{
    switch(scanner->start[0])
    {
        case 'b':
            return lit_scanner_checkkeyword(scanner, 1, 4, "reak", LTOKEN_BREAK);

        case 'c':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'l':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "ass", LTOKEN_CLASS);

                    case 'o':
                    {
                        if(scanner->current - scanner->start > 3)
                        {
                            switch(scanner->start[3])
                            {
                                case 's':
                                    return lit_scanner_checkkeyword(scanner, 2, 3, "nst", LTOKEN_CONST);
                                case 't':
                                    return lit_scanner_checkkeyword(scanner, 2, 6, "ntinue", LTOKEN_CONTINUE);
                            }
                        }
                    }
                }
            }

            break;
        }

        case 'e':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'l':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "se", LTOKEN_ELSE);
                    case 'x':
                        return lit_scanner_checkkeyword(scanner, 2, 4, "port", LTOKEN_EXPORT);
                }
            }

            break;
        }


        case 'f':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'a':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "lse", LTOKEN_FALSE);
                    case 'o':
                        return lit_scanner_checkkeyword(scanner, 2, 1, "r", LTOKEN_FOR);
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 6, "nction", LTOKEN_FUNCTION);
                }
            }

            break;
        }

        case 'i':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 's':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IS);
                    case 'f':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IF);
                    case 'n':
                        return lit_scanner_checkkeyword(scanner, 2, 0, "", LTOKEN_IN);
                }
            }

            break;
        }

        case 'n':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "ll", LTOKEN_NULL);
                    case 'e':
                        return lit_scanner_checkkeyword(scanner, 2, 1, "w", LTOKEN_NEW);
                }
            }

            break;
        }

        case 'r':
        {
            if(scanner->current - scanner->start > 2)
            {
                switch(scanner->start[2])
                {
                    case 'f':
                        return lit_scanner_checkkeyword(scanner, 3, 0, "", LTOKEN_REF);
                    case 't':
                        return lit_scanner_checkkeyword(scanner, 3, 3, "urn", LTOKEN_RETURN);
                }
            }

            break;
        }

        case 'o':
            return lit_scanner_checkkeyword(scanner, 1, 7, "perator", LTOKEN_OPERATOR);

        case 's':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'u':
                        return lit_scanner_checkkeyword(scanner, 2, 3, "per", LTOKEN_SUPER);
                    case 't':
                        return lit_scanner_checkkeyword(scanner, 2, 4, "atic", LTOKEN_STATIC);
                }
            }

            break;
        }

        case 't':
        {
            if(scanner->current - scanner->start > 1)
            {
                switch(scanner->start[1])
                {
                    case 'h':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "is", LTOKEN_THIS);
                    case 'r':
                        return lit_scanner_checkkeyword(scanner, 2, 2, "ue", LTOKEN_TRUE);
                }
            }

            break;
        }

        case 'v':
            return lit_scanner_checkkeyword(scanner, 1, 2, "ar", LTOKEN_VAR);
        case 'w':
            return lit_scanner_checkkeyword(scanner, 1, 4, "hile", LTOKEN_WHILE);
    }

    return LTOKEN_IDENTIFIER;
}

static LitToken lit_scanner_scanident(LitScanner* scanner)
{
    while(lit_is_alpha(lit_scanner_peek(scanner)) || lit_is_digit(lit_scanner_peek(scanner)))
    {
        lit_scanner_advance(scanner);
    }

    return lit_scanner_maketoken(scanner, lit_scanner_scanidenttype(scanner));
}

LitToken lit_scanner_scantoken(LitScanner* scanner)
{
    if(lit_scanner_skipwhitespace(scanner))
    {
        LitToken token = lit_scanner_maketoken(scanner, LTOKEN_NEW_LINE);
        scanner->line++;

        return token;
    }

    scanner->start = scanner->current;

    if(lit_scanner_isatend(scanner))
    {
        return lit_scanner_maketoken(scanner, LTOKEN_EOF);
    }

    char c = lit_scanner_advance(scanner);

    if(lit_is_digit(c))
    {
        return lit_scanner_scannumber(scanner);
    }

    if(lit_is_alpha(c))
    {
        return lit_scanner_scanident(scanner);
    }

    switch(c)
    {
        case '(':
            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_PAREN);
        case ')':
            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_PAREN);

        case '{':
        {
            if(scanner->num_braces > 0)
            {
                scanner->braces[scanner->num_braces - 1]++;
            }

            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_BRACE);
        }

        case '}':
        {
            if(scanner->num_braces > 0 && --scanner->braces[scanner->num_braces - 1] == 0)
            {
                scanner->num_braces--;
                return lit_scanner_scanstring(scanner, true);
            }

            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_BRACE);
        }

        case '[':
            return lit_scanner_maketoken(scanner, LTOKEN_LEFT_BRACKET);
        case ']':
            return lit_scanner_maketoken(scanner, LTOKEN_RIGHT_BRACKET);
        case ';':
            return lit_scanner_maketoken(scanner, LTOKEN_SEMICOLON);
        case ',':
            return lit_scanner_maketoken(scanner, LTOKEN_COMMA);
        case ':':
            return lit_scanner_maketoken(scanner, LTOKEN_COLON);
        case '~':
            return lit_scanner_maketoken(scanner, LTOKEN_TILDE);

        case '+':
            return lit_scanner_matchtokens(scanner, '=', '+', LTOKEN_PLUS_EQUAL, LTOKEN_PLUS_PLUS, LTOKEN_PLUS);
        case '-':
            return lit_scanner_match(scanner, '>') ? lit_scanner_maketoken(scanner, LTOKEN_SMALL_ARROW) : lit_scanner_matchtokens(scanner, '=', '-', LTOKEN_MINUS_EQUAL, LTOKEN_MINUS_MINUS, LTOKEN_MINUS);
        case '/':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_SLASH_EQUAL, LTOKEN_SLASH);
        case '#':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_SHARP_EQUAL, LTOKEN_SHARP);
        case '!':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_BANG_EQUAL, LTOKEN_BANG);
        case '?':
            return lit_scanner_matchtoken(scanner, '?', LTOKEN_QUESTION_QUESTION, LTOKEN_QUESTION);
        case '%':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_PERCENT_EQUAL, LTOKEN_PERCENT);
        case '^':
            return lit_scanner_matchtoken(scanner, '=', LTOKEN_CARET_EQUAL, LTOKEN_CARET);

        case '>':
            return lit_scanner_matchtokens(scanner, '=', '>', LTOKEN_GREATER_EQUAL, LTOKEN_GREATER_GREATER, LTOKEN_GREATER);
        case '<':
            return lit_scanner_matchtokens(scanner, '=', '<', LTOKEN_LESS_EQUAL, LTOKEN_LESS_LESS, LTOKEN_LESS);
        case '*':
            return lit_scanner_matchtokens(scanner, '=', '*', LTOKEN_STAR_EQUAL, LTOKEN_STAR_STAR, LTOKEN_STAR);
        case '=':
            return lit_scanner_matchtokens(scanner, '=', '>', LTOKEN_EQUAL_EQUAL, LTOKEN_ARROW, LTOKEN_EQUAL);
        case '|':
            return lit_scanner_matchtokens(scanner, '=', '|', LTOKEN_BAR_EQUAL, LTOKEN_BAR_BAR, LTOKEN_BAR);
        case '&':
            return lit_scanner_matchtokens(scanner, '=', '&', LTOKEN_AMPERSAND_EQUAL, LTOKEN_AMPERSAND_AMPERSAND, LTOKEN_AMPERSAND);

        case '.':
        {
            if(!lit_scanner_match(scanner, '.'))
            {
                return lit_scanner_maketoken(scanner, LTOKEN_DOT);
            }

            return lit_scanner_matchtoken(scanner, '.', LTOKEN_DOT_DOT_DOT, LTOKEN_DOT_DOT);
        }

        case '$':
        {
            if(!lit_scanner_match(scanner, '\"'))
            {
                return lit_scanner_makeerrortoken(scanner, "Expected '%c' after '%c', got '%c'", '\"', '$', lit_scanner_peek(scanner));
            }

            return lit_scanner_scanstring(scanner, true);
        }

        case '"':
            return lit_scanner_scanstring(scanner, false);
    }

    printf("%s\n", scanner->current);
    return lit_scanner_makeerrortoken(scanner, "Unexpected character '%c'", c);
}