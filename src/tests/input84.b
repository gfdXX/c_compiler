graph[25];
visited[5];

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

dfs_visit(node, n) {
    auto i;
    extrn printf;

    visited[node] = 1;
    printf("%d\n", node);

    i = 0;
    while (i < n) {
        if (graph[node * n + i] != 0) {
            if (visited[i] == 0) {
                dfs_visit(i, n);
            }
        }

        i = i + 1;
    }

    return(0);
}

dfs(start, n) {
    clear_visited(n);
    dfs_visit(start, n);
    return(0);
}

main() {
    setup_graph(5);
    dfs(0, 5);
    return(0);
}
