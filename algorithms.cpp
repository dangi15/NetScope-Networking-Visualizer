#include "Algorithms.h"
#include <queue>
#include <algorithm>
#include <climits>
using namespace std;

static constexpr int INF = 1'000'000'000;

static bool graphHasNegativeWeights(AdjList& graph) {
    for (auto& [u, neighbors] : graph)
        for (auto& [v, w] : neighbors)
            if (w < 0) return true;
    return false;
}

// ── BFS (Flood / broadcast routing) ─────────────────────────────────────────
RouteResult BFS(int src, int dest, AdjList& graph) {
    RouteResult result;
    map<int, int> parent;
    map<int, bool> visited;
    queue<int> q;

    q.push(src);
    visited[src] = true;
    parent[src] = -1;

    while (!q.empty()) {
        int u = q.front(); q.pop();
        if (u == dest) break;
        for (auto& [v, w] : graph[u]) {
            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                q.push(v);
            }
        }
    }

    if (!visited[dest]) { result.noPath = true; return result; }

    for (int v = dest; v != -1; v = parent[v]) result.path.push_back(v);
    reverse(result.path.begin(), result.path.end());

    // Cost = sum of latencies along path
    for (int i = 0; i + 1 < (int)result.path.size(); i++) {
        int u = result.path[i], nxt = result.path[i + 1];
        for (auto& [v, w] : graph[u]) if (v == nxt) { result.totalCost += w; break; }
    }
    return result;
}

// ── Dijkstra (OSPF — link-state routing) ────────────────────────────────────
RouteResult Dijkstra(int src, int dest, AdjList& graph) {
    RouteResult result;
    if (graphHasNegativeWeights(graph)) { result.negativeWeights = true; return result; }

    map<int, int> dist, parent;
    map<int, bool> visited;
    for (auto& [node, _] : graph) { dist[node] = INF; parent[node] = -1; }
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
    pq.push({ 0, src });

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        if (u == dest) break;
        for (auto& [v, w] : graph[u]) {
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({ dist[v], v });
            }
        }
    }

    if (dist[dest] == INF) { result.noPath = true; return result; }
    result.totalCost = dist[dest];
    for (int v = dest; v != -1; v = parent[v]) result.path.push_back(v);
    reverse(result.path.begin(), result.path.end());
    return result;
}

// ── Bellman-Ford (RIP — distance-vector routing) ─────────────────────────────
RouteResult BellmanFord(int src, int dest, int numNodes, AdjList& graph) {
    RouteResult result;
    map<int, int> dist, parent;
    for (auto& [id, _] : graph) { dist[id] = INF; parent[id] = -1; }
    dist[src] = 0;

    for (int i = 0; i < numNodes - 1; i++) {
        for (auto& [u, neighbors] : graph) {
            if (dist[u] == INF) continue;
            for (auto& [v, w] : neighbors) {
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    parent[v] = u;
                }
            }
        }
    }
    // Negative cycle detection
    for (auto& [u, neighbors] : graph) {
        if (dist[u] == INF) continue;
        for (auto& [v, w] : neighbors) {
            if (dist[u] + w < dist[v]) { result.negativeCycle = true; return result; }
        }
    }

    if (dist[dest] == INF) { result.noPath = true; return result; }
    result.totalCost = dist[dest];
    for (int v = dest; v != -1; v = parent[v]) result.path.push_back(v);
    reverse(result.path.begin(), result.path.end());
    return result;
}

// -- Kahn's (network dependency / packet scheduling order) --------------------
vector<int> KahnsAlgorithm(AdjList& graph) {
    map<int, int> inDegree;
    for (auto& [u, _] : graph) inDegree[u] = 0;
    for (auto& [u, neighbors] : graph)
        for (auto& [v, w] : neighbors) inDegree[v]++;

    queue<int> q;
    for (auto& [id, deg] : inDegree) if (deg == 0) q.push(id);

    vector<int> topo;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        topo.push_back(u);
        for (auto& [v, w] : graph[u])
            if (--inDegree[v] == 0) q.push(v);
    }
    return (topo.size() == graph.size()) ? topo : vector<int>();
}

// ── Full routing table from src (Dijkstra) ───────────────────────────────────
map<int, pair<int, int>> buildRoutingTable(int src, AdjList& graph) {
    map<int, int> dist, parent;
    map<int, bool> visited;
    for (auto& [node, _] : graph) { dist[node] = INF; parent[node] = -1; }
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
    pq.push({ 0, src });

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        for (auto& [v, w] : graph[u]) {
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({ dist[v], v });
            }
        }
    }

    // For each dest, find the first hop from src
    map<int, pair<int, int>> table; // dest -> {cost, next_hop}
    for (auto& [node, _] : graph) {
        if (node == src || dist[node] == INF) continue;
        // Trace back to find next hop after src
        int cur = node;
        while (parent[cur] != src && parent[cur] != -1) cur = parent[cur];
        table[node] = { dist[node], cur };
    }
    return table;
}

// -- MaxBandwidth (QoS / widest-path routing) ---------------------------------
// Finds the path from src to dest that maximises the minimum link bandwidth
// (the bottleneck). Uses a max-heap modified Dijkstra:
//   - bottleneck[u] = best achievable minimum-bw to reach u
//   - relaxation: bottleneck[v] = max(bottleneck[v], min(bottleneck[u], bw(u,v)))
// Time complexity: O((V + E) log V) -- same as standard Dijkstra.
RouteResult MaxBandwidth(int src, int dest, BwAdjList& bwGraph) {
    RouteResult result;
    if (bwGraph.empty()) { result.noPath = true; return result; }

    // bottleneck[u] = highest guaranteed minimum bandwidth to reach u from src.
    // Initialised to 0 (unreachable); src starts at INT_MAX (unlimited).
    map<int, int> bottleneck, parent;
    for (auto& [node, _] : bwGraph) { bottleneck[node] = 0; parent[node] = -1; }
    bottleneck[src] = INT_MAX;

    // Max-heap: {bottleneck_so_far, node}
    priority_queue<pair<int, int>> pq;
    pq.push({ INT_MAX, src });

    map<int, bool> visited;

    while (!pq.empty()) {
        auto [bw, u] = pq.top(); pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        if (u == dest) break;

        for (auto& [v, linkBw] : bwGraph[u]) {
            if (visited[v]) continue;
            // New bottleneck to v through u = min of current path bw and this link
            int newBw = min(bw, linkBw);
            if (newBw > bottleneck[v]) {
                bottleneck[v] = newBw;
                parent[v] = u;
                pq.push({ newBw, v });
            }
        }
    }

    if (bottleneck[dest] == 0) { result.noPath = true; return result; }

    // bottleneckBw is the minimum bandwidth link along the chosen path
    result.bottleneckBw = bottleneck[dest];
    for (int v = dest; v != -1; v = parent[v]) result.path.push_back(v);
    reverse(result.path.begin(), result.path.end());
    return result;
}