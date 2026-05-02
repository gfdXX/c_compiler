glob 5, 6, 7;
vec[4] 10, 20, 030, 'A';

main() {
    auto p;
    extrn printf;

    p = &glob;

    printf("%d\n", glob);
    printf("%d\n", p[1]);
    printf("%d\n", vec[2]);
    printf("%d\n", vec[3]);
    printf("%d\n", '*n');
    printf("%d\n", 'AZ');

    return 0;
}
