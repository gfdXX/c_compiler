#include "defs.h"
#include "data.h"
#include "decl.h"

static int freereg[4];
static char *reglist[4] = { "%r8", "%r9", "%r10", "%r11" };
static char *breglist[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };
static char *dreglist[4] = { "%r8d", "%r9d", "%r10d", "%r11d" };

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
    for (int i = 0; i < 4; i++)
    {
        if (freereg[i])
        {
            freereg[i] = 0;
            return (i);
        }
    }

    fatal("Out of registers");
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

/**
 * @brief Prints the assembly preamble/prologue
 * @ingroup CodeGeneration
 * @details Generates the initial assembly code required before the main program:
 *          - Resets all register allocations
 *          - Defines the printint function for integer output
 *          - Sets up the main function prologue
 *          - Includes string constant for printf format
 * @note This function must be called before generating any executable code
 */
void cgpreamble()
{
    freeall_registers();
    fputs("\t.text\n", Outfile);
}

/**
 * @brief Prints the assembly postamble/epilogue
 * @ingroup CodeGeneration
 * @details Generates the final assembly code required after the main program:
 *          - Sets the return value to 0
 *          - Restores the base pointer
 *          - Returns from main function
 * @note This function must be called after generating all executable code
 */
void cgfuncpreamble(int id)
{
    char *name = Gsym[id].name;
    fprintf(Outfile,
        "\t.text\n"
        "\t.globl\t%s\n"
        "\t.type\t%s, @function\n"
        "%s:\n" "\tpushq\t%%rbp\n"
        "\tmovq\t%%rsp, %%rbp\n", name, name, name);
}

void cgfuncpostamble(int id)
{
    cglabel(Gsym[id].endlabel);
    fputs("\tpopq	%rbp\n" "\tret\n", Outfile);
}

/**
 * @brief Loads an integer literal value into a register
 * @ingroup CodeGeneration
 * @details Allocates a new register and generates a movq instruction to load
 *          an integer literal into that register
 * @param[in] value The integer value to load
 * @return The number of the register containing the loaded value
 */
int cgloadint(int value, int type)
{

    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
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
int cgloadglob(int id) {
    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    switch (Gsym[id].type)
    {
        case P_CHAR:
            fprintf(Outfile, "\tmovzbq\t%s(\%%rip), %s\n", Gsym[id].name,
                reglist[r]);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovzbl\t%s(\%%rip), %s\n", Gsym[id].name,
                reglist[r]);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", Gsym[id].name,
                reglist[r]);
            break;
        default:
            fatald("Bad type in cgloadglob:", Gsym[id].type);
    }

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
 * @brief Prints an integer value from a register
 * @ingroup CodeGeneration
 * @details Generates code to call the printint function:
 *          - Moves the value from register to rdi (first argument)
 *          - Calls the printint function
 *          - Frees the used register
 * @param[in] r Number of the register containing the integer value to print
 */
void cgprintint(int r)
{
    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    free_register(r);
}

// Call a function with one argument from the given register
// Return the register with the result
int cgcall(int r, int id)
{
    // Get a new register
    int outr = alloc_register();
    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\t%s\n", Gsym[id].name);
    fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
    free_register(r);
    return (outr);
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
    switch (Gsym[id].type)
    {
        case P_CHAR:
            fprintf(Outfile, "\tmovb\t%s, %s(\%%rip)\n", breglist[r],
                Gsym[id].name);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovl\t%s, %s(\%%rip)\n", dreglist[r],
                Gsym[id].name);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r],
                Gsym[id].name);
            break;
        default:
            fatald("Bad type in cgloadglob:", Gsym[id].type);
    }
  return (r);
}

// Array of type sizes in P_XXX order.
// 0 means no size.
static int psize[] = { 0, 0, 1, 4, 8 };

// Given a P_XXX type value, return the
// size of a primitive type in bytes.
int cgprimsize(int type)
{
    // Check the type is valid
    if (type < P_NONE || type > P_LONG)
    {
        fatal("Bad type in cgprimsize()");
    }

    return (psize[type]);
}

/**
 * @brief Generate a global symbol
 * @ingroup CodeGeneration
 * @param[in] sym Name of the global symbol to generate
 */
void cgglobsym(int id)
{
    int typesize;
    // Get the size of the type
    typesize = cgprimsize(Gsym[id].type);

    fprintf(Outfile, "\t.comm\t%s,%d,%d\n", Gsym[id].name, typesize, typesize);
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
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

// Generate a label
void cglabel(int l)
{
    fprintf(Outfile, "L%d:\n", l);
}

// Generate a jump to a label
void cgjump(int l) 
{
    fprintf(Outfile, "\tjmp\tL%d\n", l);
}

// List of inverted jump instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = { "jne", "je", "jge", "jle", "jg", "jl" };

// Compare two registers and jump if false.
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
    // Check the range of the AST operation
    if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
    freeall_registers();
    return (NOREG);
}

// Widen the value in the register from the old
// to the new type, and return a register with
// this new value
int cgwiden(int r, int oldtype, int newtype)
{
    // Nothing to do
    return (r);
}

// Generate code to return a value from a function
void cgreturn(int reg, int id)
{
    // Generate code depending on the function's type
    switch (Gsym[id].type)
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
            fatald("Bad function type in cgreturn:", Gsym[id].type);
    }
    
    cgjump(Gsym[id].endlabel);
}