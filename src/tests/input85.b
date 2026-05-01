values[6];
links[6];
head;

set_node(id, value, next) {
    values[id] = value;
    links[id] = next;
    return(0);
}

push_front(id) {
    links[id] = head;
    head = id;
    return(0);
}

insert_after(pos, id) {
    links[id] = links[pos];
    links[pos] = id;
    return(0);
}

print_list(start) {
    auto node;
    extrn printf;

    node = start;
    while (node != -1) {
        printf("%d\n", values[node]);
        node = links[node];
    }

    return(0);
}

reverse_list(start) {
    auto previous, node, next;

    previous = -1;
    node = start;

    while (node != -1) {
        next = links[node];
        links[node] = previous;
        previous = node;
        node = next;
    }

    return(previous);
}

main() {
    set_node(0, 10, -1);
    set_node(1, 20, -1);
    set_node(2, 30, -1);
    set_node(3, 40, -1);
    set_node(4, 25, -1);

    head = -1;
    push_front(0);
    push_front(1);
    push_front(2);
    push_front(3);

    print_list(head);

    insert_after(1, 4);
    print_list(head);

    head = reverse_list(head);
    print_list(head);

    return(0);
}
