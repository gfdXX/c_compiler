graph[25];
visited[5];
queue[5];

clear_graph(n) {
    auto i;

    i = 0;
    while (i < n * n) {
        graph[i] = 0;
        i = i + 1;
    }

    return(0);
}

clear_visited(n) {
    auto i;

    i = 0;
    while (i < n) {
        visited[i] = 0;
        i = i + 1;
    }

    return(0);
}

add_edge(a, b, n) {
    graph[a * n + b] = 1;
    graph[b * n + a] = 1;
    return(0);
}

setup_graph(n) {
    clear_graph(n);

    add_edge(0, 1, n);
    add_edge(0, 2, n);
    add_edge(1, 3, n);
    add_edge(2, 4, n);

    return(0);
}

bfs(start, n) {
    auto head, tail, node, i;
    extrn printf;

    clear_visited(n);

    head = 0;
    tail = 0;
    visited[start] = 1;
    queue[tail] = start;
    tail = tail + 1;

    while (head < tail) {
        node = queue[head];
        head = head + 1;
        printf("%d\n", node);

        i = 0;
        while (i < n) {
            if (graph[node * n + i] != 0) {
                if (visited[i] == 0) {
                    visited[i] = 1;
                    queue[tail] = i;
                    tail = tail + 1;
                }
            }

            i = i + 1;
        }
    }

    return(0);
}

main() {
    setup_graph(5);
    bfs(0, 5);
    return(0);
}
