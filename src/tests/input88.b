main() {
    auto x;
    extrn printf;

    x = 4;
    x =+ 3;
    printf("%d\n", x);

    x =- 2;
    printf("%d\n", x);

    x =* 4;
    printf("%d\n", x);

    x =/ 5;
    printf("%d\n", x);

    x =% 3;
    printf("%d\n", x);

    x =| 6;
    printf("%d\n", x);

    x =& 3;
    printf("%d\n", x);

    x =<< 2;
    printf("%d\n", x);

    x =>> 1;
    printf("%d\n", x);

    x =< 7;
    printf("%d\n", x);

    x => 0;
    printf("%d\n", x);

    x =<= 1;
    printf("%d\n", x);

    x =>= 2;
    printf("%d\n", x);

    x = 10;
    x === 10;
    printf("%d\n", x);

    x = 1;
    x =!= 0;
    printf("%d\n", x);

    return(0);
}
