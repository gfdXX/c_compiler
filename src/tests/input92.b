add(a, b) {
    return a + b;
}

mul(a, b) {
    return a * b;
}

main() {
    auto f;
    extrn printf;

    f = add;
    printf("%d\n", f(2, 3));

    f = 1 ? mul : add;
    printf("%d\n", f(2, 3));

    return 0;
}
