#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include "Link.h"

// Each adjacency entry: {neighbour_id, latency_ms}
// bandwidth is stored separately in linkProps
using AdjList = std::map<int, std::vector<std::pair<int, int>>>;

class Graph {
public:
    AdjList adj;

    // Full link properties keyed by {u,v} (and {v,u} for undirected)
    std::map<std::pair<int, int>, LinkProps> linkProps;

    // Failed links: stored as {min(u,v), max(u,v)} so undirected pairs
    // have a single canonical key. Directed graphs store {u,v} as-is.
    std::set<std::pair<int, int>> downLinks;

    void addNode(int id);
    void addEdge(int u, int v, LinkProps props, bool directed = false);
    bool edgeExists(int u, int v);
    void removeNode(int id);
    void removeEdge(int u, int v, bool directed = false);
    int getNodeCount() { return adj.size(); }

    // Toggle a link between failed and operational.
    // directed=false uses the canonical {min,max} key.
    void toggleLink(int u, int v, bool directed) {
        auto key = directed ? std::make_pair(u, v)
            : std::make_pair(std::min(u, v), std::max(u, v));
        if (downLinks.count(key)) downLinks.erase(key);
        else                      downLinks.insert(key);
    }

    bool isLinkDown(int u, int v, bool directed) const {
        auto key = directed ? std::make_pair(u, v)
            : std::make_pair(std::min(u, v), std::max(u, v));
        return downLinks.count(key) > 0;
    }
};