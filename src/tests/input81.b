fib(n) {
    if (n <= 1) return(n);
    return(fib(n - 1) + fib(n - 2));
}

fibdyn(n) {
    auto a, b, i, next;

    if (n <= 1) return(n);

    a = 0;
    b = 1;
    i = 2;

    while (i <= n) {
        next = a + b;
        a = b;
        b = next;
        i = i + 1;
    }

    return(b);
}

main() {
    extrn printf;

    printf("%d\n", fib(10));
    printf("%d\n", fibdyn(10));
    return(0);
}
