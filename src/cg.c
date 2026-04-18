#include "defs.h"
#include "data.h"
#include "decl.h"

static int freereg[4];
static char *reglist[4] = { "%r8", "%r9", "%r10", "%r11" };
static char *breglist[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };

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
    fputs
    (
        "\t.text\n"
        ".LC0:\n"
        "\t.string\t\"%d\\n\"\n"
        "printint:\n"
        "\tpushq\t%rbp\n"
        "\tmovq\t%rsp, %rbp\n"
        "\tsubq\t$16, %rsp\n"
        "\tmovl\t%edi, -4(%rbp)\n"
        "\tmovl\t-4(%rbp), %eax\n"
        "\tmovl\t%eax, %esi\n"
        "\tleaq	.LC0(%rip), %rdi\n"
        "\tmovl	$0, %eax\n"
        "\tcall	printf@PLT\n" "\tnop\n" "\tleave\n" "\tret\n" "\n"
        , Outfile
    );
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
void cgfuncpreamble(char *name)
{
    fprintf(Outfile,
        "\t.text\n"
        "\t.globl\t%s\n"
        "\t.type\t%s, @function\n"
        "%s:\n" "\tpushq\t%%rbp\n"
        "\tmovq\t%%rsp, %%rbp\n", name, name, name);
}

void cgfuncpostamble()
{
    fprintf(Outfile,
        "\tmovl	$0, %eax\n"
        "\tpopq	%rbp\n"
        "\tret\n"
        , Outfile);
}

/**
 * @brief Loads an integer literal value into a register
 * @ingroup CodeGeneration
 * @details Allocates a new register and generates a movq instruction to load
 *          an integer literal into that register
 * @param[in] value The integer value to load
 * @return The number of the register containing the loaded value
 */
int cgloadint(int value)
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
int cgloadglob(char *identifier)
{
    // Get a new register
    int r = alloc_register();

    // Print out the code to initialise it
    fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", identifier, reglist[r]);
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

    return(r2);
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

/**
 * @brief Store a register's value into a variable
 * @ingroup CodeGeneration
 * @param[in] r Number of the register containing the integer value to print
 * @param[in] identifier Number of the register identifier
 * @return The number of the register containing the value that was stored (r)
 */
int cgstorglob(int r, char *identifier)
{
    fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], identifier);
    return (r);
}

/**
 * @brief Generate a global symbol
 * @ingroup CodeGeneration
 * @param[in] sym Name of the global symbol to generate
 */
void cgglobsym(char *sym)
{
    fprintf(Outfile, "\t.comm\t%s,8,8\n", sym);
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