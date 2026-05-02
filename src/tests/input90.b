main() {
    auto v 4, p;
    extrn printf;

    v[0] = 3;
    v[1] = 4;
    v[2] = 5;
    v[3] = 6;

    p = &v[1];
    printf("%d\n", *p);

    *p = 20;
    printf("%d\n", v[1]);
    printf("%d\n", 2[v]);
    printf("%d\n", *(v + 3));

    return 0;
}
