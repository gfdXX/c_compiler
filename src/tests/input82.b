bubble[6];
insert[6];

bubble_sort(n) {
    auto i, j, limit, temp;

    i = 0;
    while (i < n - 1) {
        j = 0;
        limit = n - i - 1;

        while (j < limit) {
            if (bubble[j] > bubble[j + 1]) {
                temp = bubble[j];
                bubble[j] = bubble[j + 1];
                bubble[j + 1] = temp;
            }

            j = j + 1;
        }

        i = i + 1;
    }

    return(0);
}

insertion_sort(n) {
    auto i, j, key, more;

    i = 1;
    while (i < n) {
        key = insert[i];
        j = i - 1;
        more = 1;

        while ((j >= 0) * more) {
            if (insert[j] > key) {
                insert[j + 1] = insert[j];
                j = j - 1;
            } else {
                more = 0;
            }
        }

        insert[j + 1] = key;
        i = i + 1;
    }

    return(0);
}

print_bubble(n) {
    auto i;
    extrn printf;

    i = 0;
    while (i < n) {
        printf("%d\n", bubble[i]);
        i = i + 1;
    }

    return(0);
}

print_insert(n) {
    auto i;
    extrn printf;

    i = 0;
    while (i < n) {
        printf("%d\n", insert[i]);
        i = i + 1;
    }

    return(0);
}

main() {
    bubble[0] = 7;
    bubble[1] = 2;
    bubble[2] = 9;
    bubble[3] = 1;
    bubble[4] = 5;
    bubble[5] = 3;

    insert[0] = 4;
    insert[1] = 8;
    insert[2] = 6;
    insert[3] = 0;
    insert[4] = 7;
    insert[5] = 2;

    bubble_sort(6);
    print_bubble(6);

    insertion_sort(6);
    print_insert(6);

    return(0);
}
