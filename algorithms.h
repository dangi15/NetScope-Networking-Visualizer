#pragma once
#include <vector>
#include <map>
#include <string>
#include <climits>
using namespace std;

// Returned by Dijkstra() when the graph contains negative-weight edges.
inline constexpr int DIJKSTRA_NEG_WEIGHT_SENTINEL = INT_MIN;

// Result of a routing query
struct RouteResult {
    vector<int> path;           // node IDs in order src -> ... -> dst
    int totalCost = 0;       // sum of latencies (ms), or 0 for QoS mode
    int bottleneckBw = 0;       // minimum bandwidth along the path (Mbps); 0 = not a QoS result
    bool negativeWeights = false;
    bool noPath = false;
    bool negativeCycle = false;
};

// Standard latency-based adjacency list: {neighbour, latency_ms}
using AdjList = map<int, vector<pair<int, int>>>;

// Bandwidth adjacency list used exclusively by MaxBandwidth: {neighbour, bandwidth_mbps}
using BwAdjList = map<int, vector<pair<int, int>>>;

RouteResult BFS(int src, int dest, AdjList& graph);
RouteResult Dijkstra(int src, int dest, AdjList& graph);
RouteResult BellmanFord(int src, int dest, int numNodes, AdjList& graph);
vector<int> KahnsAlgorithm(AdjList& graph);

// QoS / widest-path: maximises the minimum (bottleneck) bandwidth along the path.
// Uses a max-heap modified Dijkstra. bwGraph carries {neighbour, bandwidth_mbps}.
RouteResult MaxBandwidth(int src, int dest, BwAdjList& bwGraph);

// Build a full distance table from src to every reachable node (for routing table panel)
// Returns map<node_id, {cost, next_hop}>
map<int, pair<int, int>> buildRoutingTable(int src, AdjList& graph);