main() {
    auto x, y;
    extrn printf;

    x = 3;
    y = x > 2 ? 11 : 22;
    printf("%d\n", y);

    y = x < 2 ? 33 : 44;
    printf("%d\n", y);

    goto skip;
    printf("%d\n", 999);

skip:
    printf("%d\n", 55);

    switch x {
    case 1:
        printf("%d\n", 1);
    case 3:
        printf("%d\n", 3);
    default:
        printf("%d\n", 9);
    }

    return 0;
}
