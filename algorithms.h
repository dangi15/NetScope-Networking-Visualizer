#pragma once
#include <vector>
#include <map>
using namespace std;

vector<int> Dijkstra(int src, int dest, map<int, vector<pair<int, int>>>& graph);

vector<int> BFS(int src, int dest, map<int, vector<pair<int, int>>>& graph);