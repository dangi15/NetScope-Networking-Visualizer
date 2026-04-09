#include <vector>
#include <map>
#include <queue>
using namespace std;

vector<int> BFS(int src, int dest, map<int, vector<pair<int, int>>>& graph) {
	map<int, int> parent;
	map<int, bool> visited;

	queue<int> q;
	q.push(src);
	visited[src] = true;
	parent[src] = -1;

	while (!q.empty()) {
		int u = q.front();
		q.pop();

		if (u == dest) break;

		for (auto& [v, w] : graph[u]) {
			if (!visited[v]) {
				visited[v] = true;
				parent[v] = u;
				q.push(v);
			}
		}
	}

	if (!visited[dest]) return {};

	vector<int> path;
	for (int v = dest; v != -1; v = parent[v])
		path.push_back(v);

	reverse(path.begin(), path.end());
	return path;
}


vector<int> Dijkstra(int src, int dest, map<int, vector<pair<int, int>>>& graph) {
	map<int, int> dist, parent;
	for (auto& [node, _] : graph) {
		dist[node] = INT_MAX;
		parent[node] = -1;
	}

	priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;

	dist[src] = 0;
	pq.push({ 0, src });

	while (!pq.empty()) {
		auto [d, u] = pq.top();
		pq.pop();

		if (d > dist[u]) continue;

		for (auto& [v, w] : graph[u]) {
			if (dist[u] + w < dist[v]) {
				dist[v] = dist[u] + w;
				parent[v] = u;
				pq.push({ dist[v], v });
			}
		}
	}

	if (dist[dest] == INT_MAX) return {};

	vector<int> path;
	for (int v = dest; v != -1; v = parent[v])
		path.push_back(v);
	reverse(path.begin(), path.end());
	return path;
}