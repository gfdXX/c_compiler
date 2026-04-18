#include "defs.h"
#include "data.h"
#include "decl.h"

// Parse a function call with a single expression
// argument and return its AST
struct ASTnode *funccall(void) {
    struct ASTnode *tree;
    int id;

    // Check that the identifier has been defined,
    // then make a leaf node for it. XXX Add structural type test
    if ((id = findglob(Text)) == -1) {
        fatals("Undeclared function", Text);
    }
    // Get the '('
    lparen();

    // Parse the following expression
    tree = binexpr(0);

    // Build the function call AST node. Store the
    // function's return type as this node's type.
    // Also record the function's symbol-id
    tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

    // Get the ')'
    rparen();
    return (tree);
}

static struct ASTnode *primary(void)
{
    struct ASTnode *n;
    int id;

    switch (Token.token)
    {
        case T_INTLIT:
            if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
            {
                n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
            }
            else
            {
                n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
            }
            break;
        case T_IDENT:
            scan(&Token);

            if (Token.token == T_LPAREN)
            {
                return (funccall());
            }

            reject_token(&Token);

            id = findglob(Text);

            if (id == -1)
            {
                fatals("Unknown variable", Text);
            }

            n = mkastleaf(A_IDENT, Gsym[id].type, id);
            break;
        default:
            fatald("Syntax error, token", Token.token);
    }

    scan(&Token);
    return (n);
}

static int arithop(int tokentype)
{
    if (tokentype > T_EOF && tokentype < T_INTLIT)
    {
        return (tokentype);
    }
    
    fatald("Syntax error, token", tokentype);
}

static int OpPrec[] = {
    0, 10, 10,                    // T_EOF, T_PLUS, T_MINUS
    20, 20,                       // T_STAR, T_SLASH
    30, 30,                       // T_EQ, T_NE
    40, 40, 40, 40                // T_LT, T_GT, T_LE, T_GE
};

static int op_precedence(int tokentype) {
    int prec = OpPrec[tokentype];
    
    if (prec == 0)
    {
        fatald("Syntax error, token", tokentype);
    }

    return (prec);
}

struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left, *right;
    int lefttype, righttype;
    int tokentype;

    left = primary();

    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN)
    {
        return (left);
    }

    while (op_precedence(tokentype) > ptp) 
    {
        scan(&Token);

        right = binexpr(OpPrec[tokentype]);

        // Ensure the two types are compatible.
        lefttype = left->type;
        righttype = right->type;

        if (!type_compatible(&lefttype, &righttype, 0))
        {
            fatal("Incompatible types");
        }
        // Widen either side if required. type vars are A_WIDEN now
        if (lefttype)
        {
            left = mkastunary(lefttype, right->type, left, 0);
        }
        if (righttype)
        {
            right = mkastunary(righttype, left->type, right, 0);
        }

        left = mkastnode(arithop(tokentype), left->type, left, NULL, right, 0);

        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN)
        {
            return (left);
        }

    }
    return (left);
}
