#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN     512     // Length of symbols in input
#define NSYMBOLS    1024    // Number of symbol table entries

struct token 
{
    int token;
    int intvalue;
};

enum
{
    T_EOF,
    
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,

    T_EQ,
    T_NE,
    T_LT,
    T_GT,
    T_LE,
    T_GE,

    T_INTLIT,
    T_SEMI,
    T_ASSIGN,
    T_IDENT,

    // Keywords
    T_PRINT,
    T_INT
};

enum
{
    A_ADD = 1,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,

    A_EQ,
    A_NE,
    A_LT,
    A_GT,
    A_LE,
    A_GE,

    A_INTLIT,
    A_IDENT,
    A_LVIDENT,
    A_ASSIGN
};

struct ASTnode
{
    int     op;
    struct  ASTnode *left;
    struct  ASTnode *right;
    int     intvalue;

    union
    {
        int intvalue;
        int id;
    } v;
    
};

struct symtable
{
    char *name;
};

