HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall $(SRCS)

clean:
	rm -f cwj cwjarm compn *.o *.s out

test: cwj tests/runtests
	(cd tests; chmod +x runtests; ./runtests)