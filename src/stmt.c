#include "defs.h"
#include "data.h"
#include "decl.h"

// Prototypes
static struct ASTnode *single_statement(void);

static struct ASTnode *if_statement(void)
{
    struct ASTnode *condAST, *trueAST, *falseAST = NULL;
    
    match(T_IF, "if");
    lparen();

    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE)
    {
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
    }
    
    rparen();

    trueAST = compound_statement();

    if (Token.token == T_ELSE)
    {
        scan(&Token);
        falseAST = compound_statement();
    }

    return (mkastnode(A_IF, P_NONE, condAST, trueAST, falseAST, 0));
}

static struct ASTnode *while_statement(void)
{
    struct ASTnode *condAST, *bodyAST;
    
    match(T_WHILE, "while");
    lparen();

    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE)
    {
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
    }
    
    rparen();
    Looplevel++;
    bodyAST = compound_statement();
    Looplevel--;

    return (mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, 0));
}

// static struct ASTnode *for_statement(void)
// {
//     struct ASTnode *condAST, *bodyAST;
//     struct ASTnode *preopAST, *postopAST;
//     struct ASTnode *tree;
    
//     match(T_FOR, "for");
//     lparen();

//     preopAST = single_statement();
//     semi();

//     condAST = binexpr(0);
//     if (condAST->op < A_EQ || condAST->op > A_GE)
//     {
//         condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
//     }
    
//     semi();

//     postopAST = single_statement();
//     rparen();

//     bodyAST = compound_statement();

//     tree = mkastnode(A_GLUE, P_NONE, bodyAST, NULL, postopAST, 0);

//     // Make a WHILE loop with the condition and this new body
//     tree = mkastnode(A_WHILE, P_NONE, condAST, NULL, tree, 0);

//     return (mkastnode(A_GLUE, P_NONE, preopAST, NULL, tree, 0));
// }

// Parse a return statement and return its AST
static struct ASTnode *return_statement(void) {
    struct ASTnode *tree;

    // Can't return a value if function returns P_VOID
    if (Symtable[Functionid].type == P_VOID)
    {
        fatal("Can't return from a void function");
    }

    // Ensure we have 'return' '('
    match(T_RETURN, "return");
    lparen();

    // Parse the following expression
    tree = binexpr(0);

    // Ensure this is compatible with the function's type
    tree = modify_type(tree, Symtable[Functionid].type, 0);

    if (tree == NULL)
    {
        fatal("Incompatible type to return");
    }

    tree = mkastunary(A_RETURN, P_NONE, tree, 0);

    // Get the ')'
    rparen();
    return (tree);
}

// Parse a switch statement and return its AST
static struct ASTnode *switch_statement(void)
{
    struct ASTnode *left, *n, *c, *casetree= NULL, *casetail;
    int inloop = 1, casecount = 0;
    int seendefault = 0;
    int ASTop, casevalue;

    // Skip the 'switch' and '('
    scan(&Token);
    lparen();

    // Get the switch expression, the ')' and the '{'
    left= binexpr(0);
    rparen();
    lbrace();

    // Ensure that this is of int type
    if (!inttype(left->type))
    {
        fatal("Switch expression is not of integer type");
    }

    // Build an A_SWITCH subtree with the expression as
    // the child
    n= mkastunary(A_SWITCH, 0, left, 0);

    // Now parse the cases
    Switchlevel++;
    while (inloop)
    {
        switch(Token.token)
        {
            // Leave the loop when we hit a '}'
            case T_RBRACE:
                if (casecount==0)
                {
                    fatal("No cases in switch");
                }
                inloop=0;
                break;
            case T_CASE:
            case T_DEFAULT:
                // Ensure this isn't after a previous 'default'
                if (seendefault)
                {
                    fatal("case or default after existing default");

                }
                // Set the AST operation. Scan the case value if required
                if (Token.token==T_DEFAULT)
                {
                    ASTop= A_DEFAULT;
                    seendefault= 1;
                    scan(&Token);
                }
                else 
                {
                ASTop= A_CASE;
                scan(&Token);
                left= binexpr(0);
                // Ensure the case value is an integer literal
                if (left->op != A_INTLIT)
                {
                    fatal("Expecting integer literal for case value");
                }
                casevalue= left->intvalue;

                // Walk the list of existing case values to ensure
                // that there isn't a duplicate case value
                for (c= casetree; c != NULL; c= c -> right)
                {
                    if (casevalue == c->intvalue)
                    {
                        fatal("Duplicate case value");
                    }
                }
                }

                // Scan the ':' and get the compound expression
                match(T_COLON, ":");
                left= compound_statement(); casecount++;

                // Build a sub-tree with the compound statement as the left child
                // and link it in to the growing A_CASE tree
                if (casetree==NULL)
                {
                    casetree= casetail= mkastunary(ASTop, 0, left, casevalue);
                }
                else
                {
                    casetail->right= mkastunary(ASTop, 0, left, casevalue);
                    casetail= casetail->right;
                }
                break;
            default:
                fatald("Unexpected token in switch", Token.token);
        }
    }
    Switchlevel--;

    // We have a sub-tree with the cases and any default. Put the
    // case count into the A_SWITCH node and attach the case tree.
    n->intvalue= casecount;
    n->right= casetree;
    rbrace();

    return(n);
}

static struct ASTnode *single_statement(void)
{
    int type;

    switch (Token.token)
    {
        case T_CHAR:
        case T_INT:
        case T_LONG:
            type = parse_type();
            ident();
            var_declaration(type, C_LOCAL);
            semi();
            return (NULL);		// No AST generated here
        case T_IF:
            return (if_statement());
        case T_WHILE:
            return (while_statement());
        // case T_FOR:
        //     return (for_statement());
        case T_RETURN:
            return (return_statement());
        case T_SWITCH:
            return (switch_statement());
        default:
            return (binexpr(0));
    }
    return (NULL);		// Keep -Wall happy
}

struct ASTnode *compound_statement(void)
{
    struct ASTnode *left = NULL;
    struct ASTnode *tree;

    lbrace();

    while (1)
    {
        tree = single_statement();

        if (tree != NULL && (tree->op == A_ASSIGN ||
			tree->op == A_RETURN || tree->op == A_FUNCCALL))
        {
            semi();
        }

        if (tree != NULL)
        {
            if (left == NULL)
            {
                left = tree;
            }
            else
            {
                left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, 0);
            }
        }

        if (Token.token == T_RBRACE)
        {
            rbrace();
            return (left);
        }
        
    }
}