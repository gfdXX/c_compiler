#include "defs.h"
#include "data.h"
#include "decl.h"

static int chrpos(char *s, int c)
{
    char *p;

    p = strchr(s, c);
    return (p ? p - s : -1);
}

static int next(void)
{
    int c;

    if (Putback)
    {
        c = Putback;
        Putback = 0;
        return (c);
    }
    
    c = fgetc(Infile);

    if ('\n' == c)
    {
        Line++;
    }

    return (c);
}

static void putback(int c)
{
    Putback = c;
}

static int skip(void)
{
    int c;

    c = next();

    while (1)
    {
        while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
        {
            c = next();
        }

        if (c == '/')
        {
            int d = next();

            if (d == '*')
            {
                int prev = 0;

                while ((c = next()) != EOF)
                {
                    if (prev == '*' && c == '/')
                    {
                        break;
                    }
                    prev = c;
                }

                if (c == EOF)
                {
                    fatal("Unterminated comment");
                }

                c = next();
                continue;
            }

            putback(d);
        }

        return (c);
    }
}

// Return the next character from a character
// or string literal
static int scanch(void)
{
    int c;

    // Get the next input character and interpret
    // metacharacters that start with a backslash
    c = next();

    if (c == '\\')
    {
        switch (c = next())
        {
            case 'a':
                return '\a';
            case 'b':
                return '\b';
            case 'f':
                return '\f';
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 't':
                return '\t';
            case 'v':
                return '\v';
            case '\\':
                return '\\';
            case '"':
                return '"';
            case '\'':
                return '\'';
            default:
                fatalc("unknown escape sequence", c);
        }
    }
    return (c);			// Just an ordinary old character!
}

static int scanint(int c)
{
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0)
    {
        val = val * 10 + k;
        c = next();
    }

    // We hit a non-integer character, put it back.
    putback(c);
    return (val);
}

// Scan in a string literal from the input file,
// and store it in buf[]. Return the length of
// the string. 
static int scanstr(char *buf)
{
    int i, c;

    // Loop while we have enough buffer space
    for (i = 0; i < TEXTLEN - 1; i++)
    {
        // Get the next char and append to buf
        // Return when we hit the ending double quote
        if ((c = scanch()) == '"')
        {
            buf[i] = 0;
            return (i);
        }
        buf[i] = c;
    }
    // Ran out of buf[] space
    fatal("String literal too long");
    return (0);
}

static int scanident(int c, char *buf, int lim) {
    int i = 0;

    while (isalpha(c) || isdigit(c) || '_' == c)
    {
        if (lim - 1 == i) 
        {
            fatal("Identifier too long");
        }
        else if (i < lim - 1)
        {
            buf[i++] = c;
        }

        c = next();
    }

    putback(c);
    buf[i] = '\0';

    return (i);
}

static int keyword(char *s)
{
    switch (*s)
    {
        case 'a':
            if (!strcmp(s, "auto"))
            {
                return (T_AUTO);
            }
            break;
        case 'c':
            if (!strcmp(s, "case"))
            {
                return (T_CASE);
            }
            if (!strcmp(s, "char"))
            {
                return (T_CHAR);
            }
            break;
        case 'd':
            if (!strcmp(s, "default"))
            {
                return (T_DEFAULT);
            }
            break;
        case 'e':
            if (!strcmp(s, "else"))
            {
                return (T_ELSE);
            }
            if (!strcmp(s, "extrn"))
            {
                return (T_EXTRN);
            }
            break;
        // case 'f':
        //     if (!strcmp(s, "for"))
        //     {
        //         return (T_FOR);
        //     }
        //     break;
        case 'i':
            if (!strcmp(s, "if"))
            {
                return (T_IF);
            }
            if (!strcmp(s, "int"))
            {
                return (T_INT);
            }
            break;
        case 'l':
            if (!strcmp(s, "long"))
            {
                return (T_LONG);
            }
            break;
        case 'r':
            if (!strcmp(s, "return"))
            {
                return (T_RETURN);
            }
            break;
        case 'w':
            if (!strcmp(s, "while"))
            {
                return (T_WHILE);
            }
            break;
        case 'v':
            if (!strcmp(s, "void"))
            {
                return (T_VOID);
            }
            break;
        case 's':
            if (!strcmp(s, "switch"))
            {
                return (T_SWITCH);
            }
            break;
    }
    return (0);
}

// A pointer to a rejected token
static struct token *Rejtoken = NULL;

// Reject the token that we just scanned
void reject_token(struct token *t)
{
    if (Rejtoken != NULL)
    {
        fatal("Can't reject token twice");
    }

    Rejtoken = t;
}

// List of token strings, for debugging purposes
char *Tstring[] = {
    "EOF", "=", "||", "&&", "|", "^", "&",
    "==", "!=", "<", ">", "<=", ">=", "<<", ">>",
    "+", "-", "*", "/", "++", "--", "~", "!",
    "void", "char", "int", "long",
    "auto", "extrn",
    "if", "else", "while", "for", "return",
    "switch", "case", "default",
    "intlit", "strlit", ";", "identifier",
    "{", "}", "(", ")", "[", "]", ",",
    ":"
};

// Scan and return the next token found in the input.
// Return 1 if token valid, 0 if no tokens left.
int scan(struct token *t) {
    int c, tokentype;

    // If we have any rejected token, return it
    if (Rejtoken != NULL)
    {
        t = Rejtoken;
        Rejtoken = NULL;
        return (1);
    }
    // Skip whitespace
    c = skip();

    // Determine the token based on
    // the input character
    switch (c)
    {
        case EOF:
            t->token = T_EOF;
            return (0);
        case '+':
            if ((c = next()) == '+')
            {
                t->token = T_INC;
            }
            else
            {
                putback(c);
                t->token = T_PLUS;
            }
            break;
        case '-':
            if ((c = next()) == '-')
            {
                t->token = T_DEC;
            }
            else
            {
                putback(c);
                t->token = T_MINUS;
            }
            break;
        case '*':
            t->token = T_STAR;
            break;
        case '/':
            t->token = T_SLASH;
            break;
        case ';':
            t->token = T_SEMI;
            break;
        case '{':
            t->token = T_LBRACE;
            break;
        case '}':
            t->token = T_RBRACE;
            break;
        case '(':
            t->token = T_LPAREN;
            break;
        case ')':
            t->token = T_RPAREN;
            break;
        case '[':
            t->token = T_LBRACKET;
            break;
        case ']':
            t->token = T_RBRACKET;
            break;
        case '~':
            t->token = T_INVERT;
            break;
        case '^':
            t->token = T_XOR;
            break;
        case ',':
            t->token = T_COMMA;
            break;
        case ':':
            t->token = T_COLON;
            break;
        case '=':
            if ((c = next()) == '=')
            {
                t->token = T_EQ;
            }
            else
            {
                putback(c);
                t->token = T_ASSIGN;
            }
            break;
        case '!':
            if ((c = next()) == '=')
            {
                t->token = T_NE;
            }
            else
            {
                putback(c);
                t->token = T_LOGNOT;
            }
            break;
        case '<':
            if ((c = next()) == '=')
            {
                t->token = T_LE;
            } else if (c == '<') {
                t->token = T_LSHIFT;
            }
            else
            {
                putback(c);
                t->token = T_LT;
            }
            break;
        case '>':
            if ((c = next()) == '=')
            {
                t->token = T_GE;
            }
            else if (c == '>')
            {
                t->token = T_RSHIFT;
            } 
            else
            {
                putback(c);
                t->token = T_GT;
            }
            break;
        case '&':
            if ((c = next()) == '&') 
            {
                t->token = T_LOGAND;
            } 
            else 
            {
                putback(c);
                t->token = T_AMPER;
            }
            break;
        case '|':
            if ((c = next()) == '|') 
            {
                t->token = T_LOGOR;
            }
            else 
            {
                putback(c);
                t->token = T_OR;
            }
            break;
        case '\'':
            // If it's a quote, scan in the
            // literal character value and
            // the trailing quote
            t->intvalue = scanch();
            t->token = T_INTLIT;

            if (next() != '\'')
            {
                fatal("Expected '\\'' at end of char literal");
            }

            break;
        case '"':
            // Scan in a literal string
            scanstr(Text);
            t->token = T_STRLIT;

            break;
        default:
            // If it's a digit, scan the
            // literal integer value in
            if (isdigit(c))
            {
                t->intvalue = scanint(c);
                t->token = T_INTLIT;
                break;
            } 
            else if (isalpha(c) || '_' == c) 
            {
                // Read in a keyword or identifier
                scanident(c, Text, TEXTLEN);

                // If it's a recognised keyword, return that token
                if ((tokentype = keyword(Text)) != 0)
                {
                    t->token = tokentype;
                    break;
                }
                // Not a recognised keyword, so it must be an identifier
                t->token = T_IDENT;
                break;
            }
            // The character isn't part of any recognised token, error
            fatalc("Unrecognised character", c);
    }

    // We found a token
    t->tokstr = Tstring[t->token];
    return (1);
}
