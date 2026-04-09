#pragma once
#include <map>
#include <vector>
using namespace std;

class Graph {
public:
    map<int, vector<pair<int, int>>> adj;

    void addNode(int id);
    void addEdge(int u, int v, int w);
    bool edgeExists(int u, int v);
};