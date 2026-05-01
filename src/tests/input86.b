done() {
    return;
}

main() {
    auto x, y;
    extrn printf;

    ;

    x = 17;
    y = x % 5;
    printf("%d\n", y);

    x = 10;
    x =+ 7;
    printf("%d\n", x);

    x =* 3;
    printf("%d\n", x);

    x =% 8;
    printf("%d\n", x);

    x =| 2;
    printf("%d\n", x);

    x =& 6;
    printf("%d\n", x);

    done();
    return(0);
}
