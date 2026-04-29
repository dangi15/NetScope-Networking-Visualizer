#include "Graph.h"

void Graph::addNode(int id) {
    if (adj.find(id) == adj.end()) adj[id] = {};
}

bool Graph::edgeExists(int u, int v) {
    for (auto& p : adj[u])
        if (p.first == v) return true;
    return false;
}

void Graph::addEdge(int u, int v, LinkProps props, bool directed) {
    if (!edgeExists(u, v)) {
        adj[u].push_back({ v, props.latency });
        linkProps[{u, v}] = props;
    }
    if (!directed && !edgeExists(v, u)) {
        adj[v].push_back({ u, props.latency });
        linkProps[{v, u}] = props;
    }
}

void Graph::removeNode(int id) {
    // Erase only linkProps and downLinks entries that involve the deleted node.
    // The old code iterated ALL edges in the graph and erased everything,
    // wiping bandwidth/latency data for unrelated links.
    for (auto& [nb, w] : adj[id]) {
        linkProps.erase({ id, nb });
        linkProps.erase({ nb, id });
        downLinks.erase({ std::min(id, nb), std::max(id, nb) });
        downLinks.erase({ id, nb });
        downLinks.erase({ nb, id });
    }
    // Also erase entries where id appears as the destination
    for (auto& [node, neighbors] : adj) {
        for (auto& [nb, w] : neighbors) {
            if (nb == id) {
                linkProps.erase({ node, id });
                linkProps.erase({ id, node });
                downLinks.erase({ std::min(node, id), std::max(node, id) });
            }
        }
    }

    adj.erase(id);
    for (auto& [node, neighbors] : adj) {
        neighbors.erase(
            std::remove_if(neighbors.begin(), neighbors.end(),
                [id](const std::pair<int, int>& e) { return e.first == id; }),
            neighbors.end()
        );
    }
}

void Graph::removeEdge(int u, int v, bool directed) {
    auto& nu = adj[u];
    nu.erase(std::remove_if(nu.begin(), nu.end(),
        [v](const std::pair<int, int>& e) { return e.first == v; }), nu.end());
    linkProps.erase({ u, v });
    downLinks.erase({ std::min(u,v), std::max(u,v) });
    downLinks.erase({ u,v });
    if (!directed) {
        auto& nv = adj[v];
        nv.erase(std::remove_if(nv.begin(), nv.end(),
            [u](const std::pair<int, int>& e) { return e.first == u; }), nv.end());
        linkProps.erase({ v, u });
        downLinks.erase({ v,u });
    }
}