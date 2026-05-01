#include "defs.h"
#include "data.h"
#include "decl.h"

// Flag to say which section were are outputting in to
enum { no_seg, text_seg, data_seg } currSeg = no_seg;

void cgtextseg()
{
    if (currSeg != text_seg)
    {
        fputs("\t.text\n", Outfile);
        currSeg = text_seg;
    }
}

void cgdataseg()
{
    if (currSeg != data_seg)
    {
        fputs("\t.data\n", Outfile);
        currSeg = data_seg;
    }
}

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
static int stackOffset;

// Create the position of a new local variable.
static int newlocaloffset(int type)
{
    // Decrement the offset by a minimum of 4 bytes
    // and allocate on the stack
    localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;
    return (-localOffset);
}

// List of available registers and their names.
// We need a list of byte and doubleword registers, too
// The list also includes the registers used to
// hold function parameters
#define NUMFREEREGS 4
#define FIRSTPARAMREG 9		// Position of first parameter register
static int freereg[NUMFREEREGS];
static char *reglist[] =
    { "%r10", "%r11", "%r12", "%r13", "%r9", "%r8", "%rcx", "%rdx", "%rsi",
    "%rdi"
};

static char *breglist[] =
    { "%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%cl", "%dl", "%sil",
    "%dil"
};

static char *dreglist[] =
    { "%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%ecx", "%edx",
    "%esi", "%edi"
};

/**
 * @brief Sets all registers as available
 * @ingroup RegisterManagement
 * @details Resets the register allocation status by marking all four general-purpose
 *          registers (0-3) as free/available for allocation
 */
void freeall_registers(void)
{
    freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

/**
 * @brief Allocates a free register
 * @ingroup RegisterManagement
 * @details Searches for the first available register in the freereg array.
 *          If found, marks it as used and returns its number.
 *          If no registers are available, prints an error and terminates the program
 * @return The number of the allocated register (0-3)
 * @note This function is static and only used internally by the code generation module
 */
static int alloc_register(void)
{
    for (int i = 0; i < NUMFREEREGS; i++) 
    {
        if (freereg[i])
        {
            freereg[i] = 0;
            return (i);
        }
    }

    fatal("Out of registers");
    return (NOREG);		// Keep -Wall happy
}

/**
 * @brief Returns a register to the list of available registers
 * @ingroup RegisterManagement
 * @details Marks the specified register as free/available for future allocation.
 *          Performs a sanity check to ensure the register wasn't already free
 * @param[in] reg The number of the register to free (0-3)
 * @note This function is static and only used internally by the code generation module
 * @warning If the register is already free, prints an error and terminates the program
 */
static void free_register(int reg)
{
    if (freereg[reg] != 0)
    {
        fatald("Error trying to free register", reg);
    }
    freereg[reg] = 1;
}

void cgfreereg(int reg)
{
    free_register(reg);
}

// Print out the assembly preamble
void cgpreamble()
{
    freeall_registers();
    cgtextseg();
    fprintf(Outfile,
        "# internal switch(expr) routine\n"
        "# %%rsi = switch table, %%rax = expr\n"
        "# from SubC: http://www.t3x.org/subc/\n"
        "\n"
        "switch:\n"
        "        pushq   %%rsi\n"
        "        movq    %%rdx, %%rsi\n"
        "        movq    %%rax, %%rbx\n"
        "        cld\n"
        "        lodsq\n"
        "        movq    %%rax, %%rcx\n"
        "next:\n"
        "        lodsq\n"
        "        movq    %%rax, %%rdx\n"
        "        lodsq\n"
        "        cmpq    %%rdx, %%rbx\n"
        "        jnz     no\n"
        "        popq    %%rsi\n"
        "        jmp     *%%rax\n"
        "no:\n"
        "        loop    next\n"
        "        lodsq\n"
        "        popq    %%rsi\n" "        jmp     *%%rax\n" "\n");
}

// Nothing to do
void cgpostamble() {}

/**
 * @brief Print out a function preamble
 * @ingroup CodeGeneration
 * @param[in] id The identifier number of the function for which to generate the preamble
 */
void cgfuncpreamble(int id)
{
    char *name = Symtable[id].name;
    int i;
    int paramOffset = 16;		// Any pushed params start at this stack offset
    int paramReg = FIRSTPARAMREG;	// Index to the first param register in above reg lists

    // Output in the text segment, reset local offset
    cgtextseg();
    localOffset = 0;

    // Output the function start, save the %rsp and %rsp
    fprintf(Outfile,
        "\t.globl\t%s\n"
        "\t.type\t%s, @function\n"
        "%s:\n" "\tpushq\t%%rbp\n"
        "\tmovq\t%%rsp, %%rbp\n", name, name, name);

    // Copy any in-register parameters to the stack
    // Stop after no more than six parameter registers
    for (i = NSYMBOLS - 1; i > Locls; i--)
    {
        if (Symtable[i].class != C_PARAM)
        {
            break;
        }
        if (i < NSYMBOLS - 6)
        {
            break;
        }

        Symtable[i].posn = newlocaloffset(Symtable[i].type);
        cgstorlocal(paramReg--, i);
    }

    // For the remainder, if they are a parameter then they are
    // already on the stack. If only a local, make a stack position.
    for (; i > Locls; i--)
    {
        if (Symtable[i].class == C_PARAM)
        {
            Symtable[i].posn = paramOffset;
            paramOffset += 8;
        } 
        else 
        {
            Symtable[i].posn = newlocaloffset(Symtable[i].type);
        }
    }

    // Align the stack pointer to be a multiple of 16
    // less than its previous value
    stackOffset = (localOffset + 15) & ~15;
    fprintf(Outfile, "\taddq\t$%d,%%rsp\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(int id)
{
    cglabel(Symtable[id].endlabel);
    fprintf(Outfile, "\taddq\t$%d,%%rsp\n", stackOffset);
    fputs("\tpopq	%rbp\n" "\tret\n", Outfile);
}

/**
 * @brief Load an integer literal value into a register.
 * @ingroup CodeGeneration
 * @param[in] value The integer value to load into the register
 * @param[in] type The type of the integer literal (e.g. P_INT, P_CHAR)
 * @return The number of the register containing the loaded value
 */
int cgloadint(int value, int type) 
{
    // Get a new register
    int r = alloc_register();

    fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
    return (r);
}

/**
 * @brief Load a value from a variable into a register.
 * @ingroup CodeGeneration
 * @details Allocates a new register and generates a movq instruction to load
 *          an integer literal into that register
 * @param[in] identifier The variable identifier
 * @return Return the number of the register
 */
int cgloadglob(int id, int op)
{
    // Get a new register
    int r = alloc_register();

    if (cgprimsize(Symtable[id].type) == 8)
    {
        if (op == A_PREINC)
        {
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
        }
        if (op == A_PREDEC)
        {
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
        }
        fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", Symtable[id].name,
            reglist[r]);
        if (op == A_POSTINC)
        {
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
        }
        if (op == A_POSTDEC)
        {
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
        }
    }
    else
    {
        // Print out the code to initialise it
        switch (Symtable[id].type)
        {
            case P_CHAR:
                if (op == A_PREINC)
                {
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
                }
                if (op == A_PREDEC)
                {
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
                }
                fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", Symtable[id].name,
                    reglist[r]);
                if (op == A_POSTINC)
                {
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
                }
                if (op == A_POSTDEC)
                {
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
                }
                break;
            case P_INT:
                if (op == A_PREINC)
                {
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
                }
                if (op == A_PREDEC)
                {
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
                }
                fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", Symtable[id].name,
                    reglist[r]);
                if (op == A_POSTINC)
                {
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
                }
                if (op == A_POSTDEC)
                {
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
                }
                break;
            default:
                fatald("Bad type in cgloadglob:", Symtable[id].type);
        }
    }

    return (r);
}

/**
 * @brief Load a value from a local variable into a register.
 * @ingroup CodeGeneration
 * @details Allocates a new register and generates a movq instruction to load
 *          an integer literal into that register
 * @param[in] id The local variable identifier
 * @param[in] op The operation to perform (pre- or post-increment/decrement)
 * @return Return the number of the register
 */
int cgloadlocal(int id, int op)
{
    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    if (cgprimsize(Symtable[id].type) == 8)
    {
        if (op == A_PREINC)
        {
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
        }
        if (op == A_PREDEC)
        {
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);
        }
        fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", Symtable[id].posn,
            reglist[r]);
        if (op == A_POSTINC)
        {
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
        }
        if (op == A_POSTDEC)
        {
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);
        }
    } 
    else
    {
        switch (Symtable[id].type)
        {
            case P_CHAR:
                if (op == A_PREINC)
                {
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
                }
                if (op == A_PREDEC)
                {
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);
                }
                fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", Symtable[id].posn,
                    reglist[r]);
                if (op == A_POSTINC)
                {
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
                }
                if (op == A_POSTDEC)
                {
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);
                }
                break;
            case P_INT:
                if (op == A_PREINC)
                {
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
                }
                if (op == A_PREDEC)
                {
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);
                }
                fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", Symtable[id].posn,
                    reglist[r]);
                
                if (op == A_POSTINC)
                {
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
                }
                if (op == A_POSTDEC)
                {
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);
                }
                break;
                default:
                    fatald("Bad type in cgloadlocal:", Symtable[id].type);
        }
    }

    return (r);
}

/**
 * @brief Given the label number of a global string,
 * @ingroup CodeGeneration
 * @details Loads the address of a global string into a new register
 * @param[in] id The label number of the global string
 * @return The number of the register containing the string address
 */
int cgloadglobstr(int id)
{
    // Get a new register
    int r = alloc_register();
    fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n", id, reglist[r]);
    return (r);
}

/**
 * @brief Performs addition of two registers
 * @ingroup CodeGeneration
 * @details Generates an addq instruction to add the values in two registers.
 *          The result is stored in the second register, the first register is freed
 * @param[in] r1 Number of the first register (addend, will be freed)
 * @param[in] r2 Number of the second register (addend, result)
 * @return The number of the register containing the addition result (r2)
 */
int cgadd(int r1, int r2)
{
    fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);

    return (r2);
}

/**
 * @brief Performs subtraction of registers (r1 - r2)
 * @ingroup CodeGeneration
 * @details Generates a subq instruction to subtract the second register from the first.
 *          The result is stored in the first register, the second register is freed
 * @param[in] r1 Number of the first register (minuend, result)
 * @param[in] r2 Number of the second register (subtrahend, will be freed)
 * @return The number of the register containing the subtraction result (r1)
 */
int cgsub(int r1, int r2)
{
    fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
    free_register(r2);

    return (r1);
}

/**
 * @brief Performs multiplication of two registers
 * @ingroup CodeGeneration
 * @details Generates an imulq instruction to multiply the values in two registers.
 *          The result is stored in the second register, the first register is freed
 * @param[in] r1 Number of the first register (multiplier, will be freed)
 * @param[in] r2 Number of the second register (multiplier, result)
 * @return The number of the register containing the multiplication result (r2)
 */
int cgmul(int r1, int r2)
{
    fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);

    return (r2);
}

/**
 * @brief Performs division of registers (r1 / r2)
 * @ingroup CodeGeneration
 * @details Generates a sequence of instructions for integer division:
 *          - Move dividend to rax
 *          - Sign extend (cqo)
 *          - Divide by divisor (idivq)
 *          - Move result back to the original register
 * @param[in] r1 Number of the first register (dividend, result)
 * @param[in] r2 Number of the second register (divisor, will be freed)
 * @return The number of the register containing the division result (r1)
 */
int cgdiv(int r1, int r2)
{
    fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
    fprintf(Outfile, "\tcqo\n");
    fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
    fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
    free_register(r2);

    return (r1);
}

/**
 * @brief Performs bitwise AND of two registers
 * @ingroup CodeGeneration
 * @details Generates an andq instruction to perform bitwise AND on the values in two registers.
 *          The result is stored in the second register, the first register is freed
 * @param[in] r1 Number of the first register (operand, will be freed)
 * @param[in] r2 Number of the second register (operand, result)
 * @return The number of the register containing the AND result (r2)
 */
int cgand(int r1, int r2)
{
    fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

/**
 * @brief Performs bitwise OR of two registers
 * @ingroup CodeGeneration
 * @details Generates an orq instruction to perform bitwise OR on the values in two registers.
 *          The result is stored in the second register, the first register is freed
 * @param[in] r1 Number of the first register (operand, will be freed)
 * @param[in] r2 Number of the second register (operand, result)
 * @return The number of the register containing the OR result (r2)
 */
int cgor(int r1, int r2)
{
    fprintf(Outfile, "\torq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

/**
 * @brief Performs bitwise XOR of two registers
 * @ingroup CodeGeneration
 * @details Generates an xorq instruction to perform bitwise XOR on the values in two registers.
 *          The result is stored in the second register, the first register is freed
 * @param[in] r1 Number of the first register (operand, will be freed)
 * @param[in] r2 Number of the second register (operand, result)
 * @return The number of the register containing the XOR result (r2)
 */

int cgxor(int r1, int r2)
{
    fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

/**
 * @brief Performs left shift of a register by a constant value
 * @ingroup CodeGeneration
 * @details Generates a salq instruction to shift the value in the register left by a specified number of bits
 * @param[in] r Number of the register to shift
 * @param[in] val The constant number of bits to shift left
 * @return The number of the register containing the shifted value (r)
 */
int cgshl(int r1, int r2)
{
    fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
    fprintf(Outfile, "\tshlq\t%%cl, %s\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

/**
 * @brief Performs right shift of a register by a constant value
 * @ingroup CodeGeneration
 * @details Generates a shrq instruction to shift the value in the register right by a specified number of bits
 * @param[in] r Number of the register to shift
 * @param[in] val The constant number of bits to shift right
 * @return The number of the register containing the shifted value (r)
 */
int cgshr(int r1, int r2)
{
    fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
    fprintf(Outfile, "\tshrq\t%%cl, %s\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

/**
 * @brief Negates the value in a register
 * @ingroup CodeGeneration
 * @details Generates a negq instruction to negate the value in the register
 * @param[in] r Number of the register to negate
 * @return The number of the register containing the negated value (r)
 */
int cgnegate(int r)
{
    fprintf(Outfile, "\tnegq\t%s\n", reglist[r]);
    return (r);
}

/**
 * @brief Inverts the value in a register
 * @ingroup CodeGeneration
 * @details Generates a notq instruction to invert the value in the register
 * @param[in] r Number of the register to invert
 * @return The number of the register containing the inverted value (r)
 */
int cginvert(int r)
{
    fprintf(Outfile, "\tnotq\t%s\n", reglist[r]);
    return (r);
}

/**
 * @brief Logically negates the value in a register
 * @ingroup CodeGeneration
 * @details Generates code to logically negate the value in the register
 * @param[in] r Number of the register to negate
 * @return The number of the register containing the negated value (r)
 */
int cglognot(int r)
{
    fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
    fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
    return (r);
}

/**
 * @brief Converts an integer value to a boolean value
 * @ingroup CodeGeneration
 * @details Generates code to convert an integer value to a boolean value. Jumps if
 *          it's an IF or WHILE operation
 * @param[in] r Number of the register containing the integer value
 * @param[in] op Operation type (A_IF or A_WHILE)
 * @param[in] label Label for the jump instruction
 * @return The number of the register containing the boolean value (r)
 */
int cgboolean(int r, int op, int label)
{
    fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
    if (op == A_IF || op == A_WHILE)
        fprintf(Outfile, "\tje\tL%d\n", label);
    else {
        fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
        fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
    }
    return (r);
}

/**
 * @brief Calls a function with one argument from the given register
 * @ingroup CodeGeneration
 * @details Generates code to call a function with one argument
 * @param[in] r Number of the register containing the argument
 * @param[in] id Identifier of the function to call
 * @return The number of the register containing the result
 */
int cgcall(int id, int numargs)
{
    int savedregs[NUMFREEREGS];
    int savedcount = 0;
    int stackpad = 0;

    for (int i = 0; i < NUMFREEREGS; i++)
    {
        if (!freereg[i])
        {
            fprintf(Outfile, "\tpushq\t%s\n", reglist[i]);
            savedregs[savedcount++] = i;
        }
    }

    if ((savedcount + ((numargs > 6) ? (numargs - 6) : 0)) & 1)
    {
        fprintf(Outfile, "\tsubq\t$8, %%rsp\n");
        stackpad = 1;
    }

    // Call the function
    fprintf(Outfile, "\tcall\t%s@PLT\n", Symtable[id].name);

    if (stackpad)
    {
        fprintf(Outfile, "\taddq\t$8, %%rsp\n");
    }

    for (int i = savedcount - 1; i >= 0; i--)
    {
        fprintf(Outfile, "\tpopq\t%s\n", reglist[savedregs[i]]);
    }

    // Get a new register
    int outr = alloc_register();

    // Remove any arguments pushed on the stack
    if (numargs > 6)
    {
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8 * (numargs - 6));
    }

    // and copy the return value into our register
    fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
    return (outr);
}

/**
 * @brief Copies an argument from a register to a parameter position
 * @ingroup CodeGeneration
 * @details Given a register with an argument value, copies this argument into the argposn'th
 *          parameter in preparation for a future function call. Note that argposn is 1, 2, 3, 4, ..., never zero.
 * @param[in] r Number of the register containing the argument
 * @param[in] argposn Position of the parameter to copy to
 */
void cgcopyarg(int r, int argposn) {

    // If this is above the sixth argument, simply push the
    // register on the stack. We rely on being called with
    // successive arguments in the correct order for x86-64
    if (argposn > 6)
    {
        fprintf(Outfile, "\tpushq\t%s\n", reglist[r]);
    }
    else
    {
        // Otherwise, copy the value into one of the six registers
        // used to hold parameter values
        fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[r], reglist[FIRSTPARAMREG - argposn + 1]);
    }
}

/**
 * @brief Shifts a register left by a constant
 * @ingroup CodeGeneration
 * @param[in] r Number of the register to shift
 * @param[in] val Number of positions to shift
 * @return The number of the register containing the shifted value (r)
 */
int cgshlconst(int r, int val)
{
    fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
    return (r);
}

/**
 * @brief Store a register's value into a variable
 * @ingroup CodeGeneration
 * @param[in] r Number of the register containing the integer value to print
 * @param[in] identifier Number of the register identifier
 * @return The number of the register containing the value that was stored (r)
 */
int cgstorglob(int r, int id)
{
    if (cgprimsize(Symtable[id].type) == 8)
    {
        fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r],
            Symtable[id].name);
    }
    else
    {
        switch (Symtable[id].type)
        {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r],
                    Symtable[id].name);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", dreglist[r],
                    Symtable[id].name);
                break;
            default:
                fatald("Bad type in cgstorglob:", Symtable[id].type);
        }
    }

    return (r);
}

/**
 * @brief Store a register's value into a local variable
 * @ingroup CodeGeneration
 * @param[in] r Number of the register containing the value to store
 * @param[in] id Number of the local variable identifier
 * @return The number of the register containing the value that was stored (r)
 */
int cgstorlocal(int r, int id)
{

    if (cgprimsize(Symtable[id].type) == 8) {
        fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r],
            Symtable[id].posn);
    }
    else
    {
        switch (Symtable[id].type)
        {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r],
                    Symtable[id].posn);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r],
                    Symtable[id].posn);
                break;
            default:
                fatald("Bad type in cgstorlocal:", Symtable[id].type);
        }
    }
    
    return (r);
}

/**
 * @brief Given a P_XXX type value, return the
 * @ingroup CodeGeneration
 * @param[in] type The type for which to get the size
 * @return The size of the primitive type in bytes
 */
int cgprimsize(int type)
{
    if (ptrtype(type))
    {
        return (8);
    }

    switch (type)
    {
        case P_CHAR:
            return (1);
        case P_INT:
            return (4);
        case P_LONG:
            return (8);
        default:
            fatald("Bad type in cgprimsize:", type);
    }

    return (0);			// Keep -Wall happy
}

/**
 * @brief Generate a global symbol but not functions
 * @ingroup CodeGeneration
 * @param[in] sym Name of the global symbol to generate
 */
void cgglobsym(int id)
{
    int typesize;

    if (Symtable[id].stype == S_FUNCTION)
    {
        return;
    }

    // Get the size of the type
    typesize = cgprimsize(Symtable[id].type);

    // Generate the global identity and the label
    cgdataseg();
    fprintf(Outfile, "\t.globl\t%s\n", Symtable[id].name);
    fprintf(Outfile, "%s:", Symtable[id].name);

    // Generate the space
    for (int i = 0; i < Symtable[id].size; i++)
    {
        switch (typesize)
        {
            case 1:
                fprintf(Outfile, "\t.byte\t0\n");
            break;
            case 4:
                fprintf(Outfile, "\t.long\t0\n");
                break;
            case 8:
                fprintf(Outfile, "\t.quad\t0\n");
                break;
            default:
                fatald("Unknown typesize in cgglobsym: ", typesize);
        }
    }
}

/**
 * @brief Generate a global string and its start label
 * @ingroup CodeGeneration
 * @param[in] l The label number for the global string
 * @param[in] strvalue The string value to generate
 */
void cgglobstr(int l, char *strvalue)
{
    char *cptr;
    cglabel(l);
    for (cptr = strvalue; *cptr; cptr++)
    {
        fprintf(Outfile, "\t.byte\t%d\n", *cptr);
    }
    fprintf(Outfile, "\t.byte\t0\n");
}

/**
 * @brief Generate code to print an integer value from a register
 * @ingroup CodeGeneration
 * @param[in] r Number of the register containing the integer value to print
 * @return The number of the register containing the value that was printed (r)
 */
static char *cmplist[] = { "sete", "setne", "setl", "setg", "setle", "setge" };

/**
 * @brief Compare two registers and set the result in a register
 * @ingroup CodeGeneration
 * @param[in] r1 Number of the register containing the integer value to print
 * @param[in] r2 Number of the register containing the integer value to print
 * @param[in] how String indicating the comparison operation (e.g. "je" for equal, "jl" for less than)
 * @return The number of the register containing the value that was stored (r)
 */
// Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
    {
        fatal("Bad ASTop in cgcompare_and_set()");
    }

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
    free_register(r1);
    return (r2);
}

/**
 * @brief Generate a label
 * @ingroup CodeGeneration
 * @param[in] l The label number to generate
 */
void cglabel(int l)
{
    fprintf(Outfile, "L%d:\n", l);
}

/**
 * @brief Generate a jump to a label
 * @ingroup CodeGeneration
 * @param[in] l The label number to jump to
 */
void cgjump(int l) 
{
    fprintf(Outfile, "\tjmp\tL%d\n", l);
}
    
/**
 * @brief Generate a list of inverted jump instructions
 * @ingroup CodeGeneration
 * @param[in] l The label number to jump to
 */
static char *invcmplist[] = { "jne", "je", "jge", "jle", "jg", "jl" };

// Compare two registers and jump if false.
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
    {
        fatal("Bad ASTop in cgcompare_and_jump()");
    }

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
    freeall_registers();
    return (NOREG);
}

/**
 * @brief Widen the value in the register from the old to the new type
 * @ingroup CodeGeneration
 * @param[in] r The register number containing the value to widen
 * @param[in] oldtype The original type of the value
 * @param[in] newtype The new type of the value
 * @return The register number containing the widened value
 */
int cgwiden(int r, int oldtype, int newtype)
{
    // Nothing to do
    return (r);
}

/**
 * @brief Generate code to return a value from a function
 * @ingroup CodeGeneration
 * @param[in] reg The register number containing the value to return
 * @param[in] id The symbol table index of the function
 */
void cgreturn(int reg, int id) {
    // Generate code depending on the function's type
    switch (Symtable[id].type)
    {
        case P_CHAR:
            fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[reg]);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[reg]);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
            break;
        default:
            fatald("Bad function type in cgreturn:", Symtable[id].type);
    }

    cgjump(Symtable[id].endlabel);
}

/**
 * @brief Generate code to load the address of an
 * @ingroup CodeGeneration
 * @param[in] id The symbol table index of the identifier
 * @return The register number containing the address
 */
int cgaddress(int id) {
    int r = alloc_register();

    if (Symtable[id].class == C_GLOBAL)
    {
        fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", Symtable[id].name,
            reglist[r]);
    }
    else
    {
        fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", Symtable[id].posn,
            reglist[r]);
    }

    return (r);
}

/**
 * @brief Dereference a pointer to get the value it points to
 * @ingroup CodeGeneration
 * @param[in] r The register number containing the pointer
 * @param[in] type The type of the value pointed to
 * @return The register number containing the dereferenced value
 */
int cgderef(int r, int type) {
    // Get the type that we are pointing to
    int newtype = value_at(type);
    // Now get the size of this type
    int size = cgprimsize(newtype);

    switch (size)
    {
        case 1:
            fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
            break;
        case 2:
            fprintf(Outfile, "\tmovslq\t(%s), %s\n", reglist[r], reglist[r]);
            break;
        case 4:
        case 8:
            fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
            break;
        default:
            fatald("Can't cgderef on type:", type);
    }
    return (r);
}

/**
 * @brief Store a value through a dereferenced pointer
 * @ingroup CodeGeneration
 * @param[in] r1 The register number containing the value to store
 * @param[in] r2 The register number containing the pointer
 * @param[in] type The type of the value to store
 * @return The register number containing the stored value
 */
int cgstorderef(int r1, int r2, int type)
{
    // Get the size of the type
    int size = cgprimsize(type);

    switch (size)
    {
        case 1:
            fprintf(Outfile, "\tmovb\t%s, (%s)\n", breglist[r1], reglist[r2]);
            break;
        case 2:
        case 4:
        case 8:
            fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
            break;
        default:
            fatald("Can't cgstoderef on type:", type);
    }
    return (r1);
}

// Generate a switch jump table and the code to
// load the registers and call the switch() code
void cgswitch(int reg, int casecount, int toplabel,
	      int *caselabel, int *caseval, int defaultlabel)
{
    int i, label;

    // Get a label for the switch table
    label = genlabel();
    cglabel(label);

    // Heuristic. If we have no cases, create one case
    // which points to the default case
    if (casecount == 0)
    {
        caseval[0] = 0;
        caselabel[0] = defaultlabel;
        casecount = 1;
    }
    // Generate the switch jump table.
    fprintf(Outfile, "\t.quad\t%d\n", casecount);
    for (i = 0; i < casecount; i++)
    {
        fprintf(Outfile, "\t.quad\t%d, L%d\n", caseval[i], caselabel[i]);
    }

    fprintf(Outfile, "\t.quad\tL%d\n", defaultlabel);

    // Load the specific registers
    cglabel(toplabel);
    fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
    fprintf(Outfile, "\tleaq\tL%d(%%rip), %%rdx\n", label);
    fprintf(Outfile, "\tjmp\tswitch\n");
}
