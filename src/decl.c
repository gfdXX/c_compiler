#include "defs.h"
#include "data.h"
#include "decl.h"

void var_declaration(void)
{
    match(T_INT, "int");
    ident();
    addglob(Text);
    genglobsym(Text);
    semi();
}

// For now we have a very simplistic function definition grammar
//
// function_declaration: 'void' identifier '(' ')' compound_statement   ;
//
// Parse the declaration of a simplistic function
struct ASTnode *function_declaration(void)
{
    struct ASTnode *tree;
    int nameslot;

    // Find the 'void', the identifier, and the '(' ')'.
    // For now, do nothing with them
    match(T_VOID, "void");
    ident();
    nameslot= addglob(Text);
    lparen();
    rparen();

    // Get the AST tree for the compound statement
    tree = compound_statement();

    // Return an A_FUNCTION node which has the function's nameslot
    // and the compound statement sub-tree
    return(mkastunary(A_FUNCTION, tree, nameslot));
}