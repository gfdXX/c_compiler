main() {
    auto x;
    extrn printf;

    x = 2;

    switch x {
    case 1:
        printf("%d\n", 1);
    default:
        printf("%d\n", 9);
    case 2:
        printf("%d\n", 2);
    }

    return 0;
}
