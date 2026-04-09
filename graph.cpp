#include "Graph.h"

void Graph::addNode(int id) {
    adj[id] = {};
}

bool Graph::edgeExists(int u, int v) {
    for (auto& p : adj[u]) {
        if (p.first == v) return true;
    }
    return false;
}

void Graph::addEdge(int u, int v, int w) {
    if (!edgeExists(u, v)) {
        adj[u].push_back({ v, w });
        adj[v].push_back({ u, w });
    }
}