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

    if(Putback)
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

    return c;
}

static void putback(int c)
{
    Putback = c;
}

static int skip(void)
{
    int c;

    c = next();

    while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
    {
        c = next();
    }
    
    return (c);
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
        case 'i':
            if (!strcmp(s, "int"))
            {
                return (T_INT);
            }
            break;
        case 'p':
            if (!strcmp(s, "print"))
            {
                return (T_PRINT);
            }
            break;
    }
    return (0);
}

int scan(struct token *t)
{
    int c, tokentype;

    c = skip();

    switch (c)
    {
        case EOF:
            t->token = T_EOF;
            return (0);
        case '+':
            t->token = T_PLUS;
            break;
        case '-':
            t->token = T_MINUS;
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
        case '=':
            t->token = T_EQUALS;
            break;
        default:
            if (isdigit(c))
            {
                t->intvalue = scanint(c);
                t->token = T_INTLIT;
                break;
            }
            else if (isalpha(c) || '_' == c)
            {
                scanident(c, Text, TEXTLEN);

                if (tokentype = keyword(Text))
                {
                    t->token = tokentype;
                    break;
                }

                t->token = T_IDENT;
                break;
            }

            fatalc("Unrecognised character", c);
    }

    // We found a token
    return (1);
}