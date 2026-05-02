#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *expression_list(int endtoken)
{
    struct ASTnode *tree = NULL;
    struct ASTnode *child = NULL;
    int exprcount = 0;

    // Loop until the final right parentheses
    while (Token.token != endtoken)
    {

        // Parse the next expression and increment the expression count
        child = binexpr(0);
        exprcount++;

        // Build an A_GLUE AST node with the previous tree as the left child
        // and the new expression as the right child. Store the expression count.
        tree = mkastnode(A_GLUE, P_NONE, tree, NULL, child, exprcount);

        // Stop when we reach the end token
        if (Token.token == endtoken)
        {
            break;
        }

        // Must have a ',' at this point
        match(T_COMMA, ",");
    }

    // Return the tree of expressions
    return (tree);
}

static struct ASTnode *function_call(struct ASTnode *callee)
{
    struct ASTnode *args, *tree;
    int id = -1;

    lparen();
    args = expression_list(T_RPAREN);
    rparen();

    if (callee->op == A_ADDR && Symtable[callee->id].stype == S_FUNCTION)
    {
        id = callee->id;
        tree = mkastunary(A_FUNCCALL, Symtable[id].type, args, id);
    }
    else
    {
        callee->rvalue = 1;
        tree = mkastnode(A_FUNCCALL, P_LONG, args, callee, NULL, -1);
    }

    return (tree);
}

static struct ASTnode *index_access(struct ASTnode *base)
{
    struct ASTnode *index;
    struct ASTnode *tmp;

    scan(&Token);
    index = binexpr(0);
    match(T_RBRACKET, "]");

    if (inttype(base->type) && ptrtype(index->type))
    {
        tmp = base;
        base = index;
        index = tmp;
    }

    if (!inttype(index->type))
    {
        fatal("Array index is not of integer type");
    }

    base->rvalue = 1;
    index->rvalue = 1;

    if (!ptrtype(base->type))
    {
        base->type = pointer_to(P_LONG);
    }

    index = modify_type(index, base->type, A_ADD);
    if (index == NULL)
    {
        fatal("Array index cannot be scaled");
    }

    base = mkastnode(A_ADD, base->type, base, NULL, index, 0);
    return (mkastunary(A_DEREF, value_at(base->type), base, 0));
}

static struct ASTnode *identifier_node(void)
{
    int id;

    id = findsymbol(Text);
    if (id == -1)
    {
        fatals("Unknown symbol", Text);
    }

    switch (Symtable[id].stype)
    {
        case S_VARIABLE:
            return (mkastleaf(A_IDENT, Symtable[id].type, id));
        case S_ARRAY:
        case S_FUNCTION:
            return (mkastleaf(A_ADDR, Symtable[id].type, id));
        case S_LABEL:
            return (mkastleaf(A_LABELADDR, pointer_to(P_LONG), id));
        default:
            fatals("Bad symbol kind", Text);
    }

    return (NULL);
}

static struct ASTnode *postfix(struct ASTnode *n)
{
    while (1)
    {
        switch (Token.token)
        {
            case T_LPAREN:
                n = function_call(n);
                break;
            case T_LBRACKET:
                n = index_access(n);
                break;
            case T_INC:
                if (n->op != A_IDENT)
                {
                    fatal("++ can only be applied to a variable");
                }
                n = mkastleaf(A_POSTINC, n->type, n->id);
                scan(&Token);
                break;
            case T_DEC:
                if (n->op != A_IDENT)
                {
                    fatal("-- can only be applied to a variable");
                }
                n = mkastleaf(A_POSTDEC, n->type, n->id);
                scan(&Token);
                break;
            default:
                return (n);
        }
    }
}

// Parse a primary factor and return an
// AST node representing it.
static struct ASTnode *primary(void)
{
    struct ASTnode *n;
    int id;

    switch (Token.token)
    {
        case T_INTLIT:
            // For an INTLIT token, make a leaf AST node for it.
            // Make it a P_CHAR if it's within the P_CHAR range
            if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
            {
                n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
            }
            else
            {
                n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
            }
            scan(&Token);
            return (postfix(n));
        case T_STRLIT:
            // For a STRLIT token, generate the assembly for it.
            // Then make a leaf AST node for it. id is the string's label.
            id = genglobstr(Text);
            n = mkastleaf(A_STRLIT, pointer_to(P_CHAR), id);
            scan(&Token);
            return (postfix(n));
        case T_IDENT:
            n = identifier_node();
            scan(&Token);
            return (postfix(n));
        case T_LPAREN:
            // Beginning of a parenthesised expression, skip the '('.
            // Scan in the expression and the right parenthesis
            scan(&Token);
            n = binexpr(0);
            rparen();
            return (postfix(n));
        default:
            fatals("Expecting a primary expression, got token", Token.tokstr);
    }

    return (NULL);
}

// Convert a binary operator token into a binary AST operation.
// We rely on a 1:1 mapping from token to AST operation
static int binastop(int tokentype)
{
    if (tokentype > T_EOF && tokentype <= T_MOD)
    {
        return (tokentype);
    }
    
    fatald("Syntax error, token", tokentype);
    return (0);			// Keep -Wall happy
}

// Return true if a token is right-associative,
// false otherwise.
static int rightassoc(int tokentype)
{
    if (tokentype == T_ASSIGN)
    {
        return (1);
    }

    return (0);
}

static int expression_end(int tokentype)
{
    return (tokentype == T_SEMI || tokentype == T_RPAREN ||
            tokentype == T_RBRACKET || tokentype == T_COMMA ||
            tokentype == T_COLON || tokentype == T_LBRACE ||
            tokentype == T_RBRACE || tokentype == T_CASE ||
            tokentype == T_DEFAULT || tokentype == T_EOF);
}

static int compound_assign_op(int tokentype)
{
    switch (tokentype)
    {
        case T_OR:
        case T_XOR:
        case T_AMPER:
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
        case T_LSHIFT:
        case T_RSHIFT:
        case T_PLUS:
        case T_MINUS:
        case T_STAR:
        case T_SLASH:
        case T_MOD:
            return (binastop(tokentype));
        default:
            return (0);
    }
}

static struct ASTnode *dupast(struct ASTnode *n)
{
    struct ASTnode *copy;

    if (n == NULL)
    {
        return (NULL);
    }

    copy = mkastnode(n->op, n->type,
                     dupast(n->left), dupast(n->mid), dupast(n->right),
                     n->intvalue);
    copy->rvalue = n->rvalue;
    return (copy);
}

// Operator precedence for each token. Must
// match up with the order of tokens in defs.h
static int OpPrec[] = {
    0, 10, 20, 30,		// T_EOF, T_ASSIGN, T_LOGOR, T_LOGAND
    40, 50, 60,			// T_OR, T_XOR, T_AMPER 
    70, 70,			    // T_EQ, T_NE
    80, 80, 80, 80,		// T_LT, T_GT, T_LE, T_GE
    90, 90,			    // T_LSHIFT, T_RSHIFT
    100, 100,			// T_PLUS, T_MINUS
    110, 110, 110		// T_STAR, T_SLASH, T_MOD
};

// Check that we have a binary operator and
// return its precedence.
static int op_precedence(int tokentype)
{
    int prec;
    if (tokentype > T_MOD)
    {
        fatald("Token with no precedence in op_precedence:", tokentype);
    }

    prec = OpPrec[tokentype];

    if (prec == 0)
    {
        fatald("Syntax error, token", tokentype);
    }

    return (prec);
}

// Parse a prefix expression and return 
// a sub-tree representing it.
struct ASTnode *prefix(void)
{
    struct ASTnode *tree;
    struct ASTnode *child;
    switch (Token.token)
    {
        case T_AMPER:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            if (tree->op == A_IDENT)
            {
                tree->op = A_ADDR;
                tree->type = pointer_to(tree->type);
                break;
            }
            if (tree->op == A_DEREF)
            {
                child = tree->left;
                child->rvalue = 1;
                tree = child;
                break;
            }
            fatal("& operator must be followed by an lvalue");
            break;
        case T_STAR:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            if (!ptrtype(tree->type))
            {
                tree->type = pointer_to(P_LONG);
            }

            // Prepend an A_DEREF operation to the tree
            tree->rvalue = 1;
            tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0);
            break;
        case T_MINUS:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            // Prepend a A_NEGATE operation to the tree and
            // make the child an rvalue. Because chars are unsigned,
            // also widen this to int so that it's signed
            tree->rvalue = 1;
            tree = modify_type(tree, P_INT, 0);
            tree = mkastunary(A_NEGATE, tree->type, tree, 0);
            break;
        case T_INVERT:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            // Prepend a A_INVERT operation to the tree and
            // make the child an rvalue.
            tree->rvalue = 1;
            tree = mkastunary(A_INVERT, tree->type, tree, 0);
            break;
        case T_LOGNOT:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            // Prepend a A_LOGNOT operation to the tree and
            // make the child an rvalue.
            tree->rvalue = 1;
            tree = mkastunary(A_LOGNOT, tree->type, tree, 0);
            break;
        case T_INC:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            // For now, ensure it's an identifier
            if (tree->op != A_IDENT)
            {
                fatal("++ operator must be followed by an identifier");
            }

            // Prepend an A_PREINC operation to the tree
            tree = mkastunary(A_PREINC, tree->type, tree, 0);
            break;
        case T_DEC:
            // Get the next token and parse it
            // recursively as a prefix expression
            scan(&Token);
            tree = prefix();

            // For now, ensure it's an identifier
            if (tree->op != A_IDENT)
            {
                fatal("-- operator must be followed by an identifier");
            }

            // Prepend an A_PREDEC operation to the tree
            tree = mkastunary(A_PREDEC, tree->type, tree, 0);
            break;
        default:
            tree = primary();
    }
    return (tree);
}

// Return an AST tree whose root is a binary operator.
// Parameter ptp is the previous token's precedence.
struct ASTnode *binexpr(int ptp)
{
    struct ASTnode *left, *right, *mid;
    struct ASTnode *ltemp, *rtemp, *target;
    int ASTop, compoundop;
    int tokentype, assignop;
    int opprec;
    const int ternary_prec = 15;

    // Get the tree on the left.
    // Fetch the next token at the same time.
    left = prefix();

    // If we hit one of several terminating tokens, return just the left node
    tokentype = Token.token;
    if (expression_end(tokentype))
    {
        left->rvalue = 1;
        return (left);
    }

    while (1)
    {
        if (tokentype == T_QUESTION)
        {
            if (ternary_prec <= ptp)
            {
                break;
            }

            scan(&Token);
            mid = binexpr(0);
            match(T_COLON, ":");
            right = binexpr(ternary_prec);

            left->rvalue = 1;
            mid->rvalue = 1;
            right->rvalue = 1;

            if (left->op < A_EQ || left->op > A_GE)
            {
                left = mkastunary(A_TOBOOL, left->type, left, 0);
            }

            rtemp = modify_type(right, mid->type, 0);
            if (rtemp != NULL)
            {
                right = rtemp;
            }

            left = mkastnode(A_TERNARY, mid->type, left, mid, right, 0);
            tokentype = Token.token;
            if (expression_end(tokentype))
            {
                left->rvalue = 1;
                return (left);
            }
            continue;
        }

        if (tokentype > T_MOD)
        {
            break;
        }

        opprec = op_precedence(tokentype);
        if (!((opprec > ptp) || (rightassoc(tokentype) && opprec == ptp)))
        {
            break;
        }

        assignop = (tokentype == T_ASSIGN) ? Token.intvalue : 0;

        // Fetch in the next token
        scan(&Token);
        compoundop = compound_assign_op(assignop);

        // Recursively call binexpr() with the
        // precedence of our token to build a sub-tree
        right = binexpr(OpPrec[tokentype]);

        // Determine the operation to be performed on the sub-trees
        ASTop = binastop(tokentype);

        if (ASTop == A_ASSIGN)
        {
            // Assignment
            // Make the right tree into an rvalue
            right->rvalue = 1;

            if (compoundop)
            {
                target = left;
                left = dupast(target);
                left->rvalue = 1;

                ltemp = modify_type(left, right->type, compoundop);
                rtemp = modify_type(right, left->type, compoundop);

                if (ltemp == NULL && rtemp == NULL)
                {
                    fatal("Incompatible types in compound assignment");
                }
                if (ltemp != NULL)
                {
                    left = ltemp;
                }
                if (rtemp != NULL)
                {
                    right = rtemp;
                }

                left = mkastnode(compoundop, left->type, left, NULL, right, 0);
                right = target;
            }
            else
            {
                // Ensure the right's type matches the left
                right = modify_type(right, left->type, 0);
                if (right == NULL)
                {
                    fatal("Incompatible expression in assignment");
                }

                // Make an assignment AST tree. However, switch
                // left and right around, so that the right expression's 
                // code will be generated before the left expression
                ltemp = left;
                left = right;
                right = ltemp;
            }
        }
        else
        {

            // We are not doing an assignment, so both trees should be rvalues
            // Convert both trees into rvalue if they are lvalue trees
            left->rvalue = 1;
            right->rvalue = 1;

            // Ensure the two types are compatible by trying
            // to modify each tree to match the other's type.
            ltemp = modify_type(left, right->type, ASTop);
            rtemp = modify_type(right, left->type, ASTop);

            if (ltemp == NULL && rtemp == NULL)
            {
                fatal("Incompatible types in binary expression");
            }
            if (ltemp != NULL)
            {
                left = ltemp;
            }
            if (rtemp != NULL)
            {
                right = rtemp;
            }
        }

        // Join that sub-tree with ours. Convert the token
        // into an AST operation at the same time.
        left = mkastnode(binastop(tokentype), left->type, left, NULL, right, 0);

        // Update the details of the current token.
        // If we hit a terminating token, return just the left node
        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN ||
            tokentype == T_RBRACKET || tokentype == T_COMMA ||
            tokentype == T_COLON || tokentype == T_LBRACE ||
            tokentype == T_RBRACE || tokentype == T_CASE ||
            tokentype == T_DEFAULT || tokentype == T_EOF)
        {
            left->rvalue = 1;
            return (left);
        }
    }

    // Return the tree we have when the precedence
    // is the same or lower
    left->rvalue = 1;
    return (left);
}
