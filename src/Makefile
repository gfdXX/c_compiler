HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

run: $(SRCS) $(HSRCS)
	cc -o run -g -Wall $(SRCS)

clean:
	rm -f run cwjarm compn *.o *.s out

test: run tests/runtests
	(cd tests; chmod +x runtests; ./runtests)