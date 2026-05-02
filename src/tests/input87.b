numbers[3];

main() {
    auto x, y;
    extrn printf;

    x = -1;
    printf("%d\n", x);

    x = 10;
    x =- 3;
    printf("%d\n", x);

    x = -3;
    printf("%d\n", x);

    y = 0;
    y =+ 5;
    printf("%d\n", y);

    numbers[0] = -1;
    numbers[1] = 2;
    numbers[2] = numbers[0] + numbers[1];
    printf("%d\n", numbers[0]);
    printf("%d\n", numbers[2]);

    return(0);
}
