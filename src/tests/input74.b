int main() {
    int x;
    int y;
    y= 0;
    x= 1;

    switch(x) {
        case 1:  { y= 5; }
        case 2:  { y= 7; }
        case 3:  { y= 9; }
        default: { y= 100; }
    }

    printf("%d\n", y);

  return(0);
}
