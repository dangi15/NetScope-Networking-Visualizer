#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <queue>
#include <climits>
#include <optional>
#include <algorithm>
#include <set>
#include <map>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
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
	//for (int i : path) cout << i << ' ';
	//cout << endl;
	return path;
}


vector<int> Dijkstra(map<int, vector<pair<int, int>>>& graph, int src, int dest) {
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
	//for (int i : path) cout << i << ' ';
	//cout << endl;
	return path;
}

struct Node {
	int id;
	sf::CircleShape shape;
};

int main() {
	sf::RenderWindow window(sf::VideoMode({ 1000, 700 }), "Network Visualizer");
	window.setFramerateLimit(60);

	map<int, vector<pair<int, int>>> graph;
	map<int, Node> nodes;
	vector<int> finalPath;
	vector<int> bfsSelect, dijkstraSelect;
	bool bfsMode = false, dijkstraMode = false;
	string weightInput = "";
	bool weightMode = false;
	int u = -1, v = -1;

	sf::Font font;
	font.openFromFile("Sakire.ttf");

	int nodeId = 0;
	vector<int> selected;

	while (window.isOpen()) {
		while (const optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}
			else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
				if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
					window.close();
				}
				if (keyPressed->scancode == sf::Keyboard::Scancode::C) {
					graph.clear();
					nodes.clear();
					finalPath.clear();
					nodeId = 0;
				}
				if (keyPressed->scancode == sf::Keyboard::Scancode::B) {
					bfsMode = true;
					dijkstraMode = false;
					bfsSelect.clear();
				}
				if (keyPressed->scancode == sf::Keyboard::Scancode::D) {
					dijkstraMode = true;
					bfsMode = false;
					dijkstraSelect.clear();
				}
				if (weightMode) {
					if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
						int w = INT_MAX;
						if(!weightInput.empty())
							w = stoi(weightInput);
						bool exists = false;
						for (auto& p : graph[u]) {
							if (p.first == v) {
								exists = true;
								break;
							}
						}
						if (!exists) {
							graph[u].push_back({ v, w });
							graph[v].push_back({ u, w });
						}
						weightMode = false;
					}
					else if (keyPressed->scancode == sf::Keyboard::Scancode::Backspace) {
						if (!weightInput.empty())
							weightInput.pop_back();
					}
				}
			}
			else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
				if (mouse->button == sf::Mouse::Button::Left) {
					sf::Vector2f pos = window.mapPixelToCoords({ mouse->position.x, mouse->position.y });

					sf::CircleShape circle(20.f);
					circle.setFillColor(sf::Color::Red);
					circle.setOrigin({20.f, 20.f});
					circle.setPosition(pos);

					nodes[nodeId] = { nodeId, circle };
					graph[nodeId] = {};
					nodeId++;
				}

				if (mouse->button == sf::Mouse::Button::Right) {
					sf::Vector2f clickPos = window.mapPixelToCoords({ mouse->position.x, mouse->position.y });
					finalPath.clear();
					if (bfsMode) {
						for (auto& [id, node] : nodes) {
							if (node.shape.getGlobalBounds().contains(clickPos)) {
								bfsSelect.push_back(id);
								node.shape.setFillColor(sf::Color::Green);
								break;
							}
						}

						if (bfsSelect.size() == 2) {
							finalPath = BFS(bfsSelect[0], bfsSelect[1], graph);
							bfsSelect.clear();
							bfsMode = false;
						}
					}
					else if (dijkstraMode) {
						for (auto& [id, node] : nodes) {
							if (node.shape.getGlobalBounds().contains(clickPos)) {
								dijkstraSelect.push_back(id);
								break;
							}
						}

						if (dijkstraSelect.size() == 2) {
							finalPath = Dijkstra(graph, dijkstraSelect[0], dijkstraSelect[1]);
							dijkstraSelect.clear();
							dijkstraMode = false;
						}
					}
					else {
						for (auto& [id, node] : nodes) {
							if (node.shape.getGlobalBounds().contains(clickPos)) {
								selected.push_back(id);
								break;
							}
						}
						if (selected.size() == 2) {
							u = selected[0];
							v = selected[1];
							if (u != v) {
								weightMode = true;
								weightInput = "";
							}
							selected.clear();
						}
					}
				}
			}
			else if (const auto& text = event->getIf<sf::Event::TextEntered>()) {
				if (weightMode) {
					if (isdigit(text->unicode))
						weightInput += static_cast<char>(text->unicode);
				}
			}
		}

		window.clear();

		set<pair<int, int>> drawn;
		for (auto& [u, neighbors] : graph) {
			for (auto& [v, w] : neighbors) {
				if (drawn.count({ v, u })) continue;

				sf::Vertex line[] = {
					sf::Vertex(nodes[u].shape.getPosition()),
					sf::Vertex(nodes[v].shape.getPosition())
				};
				window.draw(line, 2, sf::PrimitiveType::Lines);
				drawn.insert({ u, v });
			}
		}
		
		for (int i = 0; i + 1 < finalPath.size(); i++) {
			int u = finalPath[i];
			int v = finalPath[i + 1];

			sf::Vertex line[] = {
				sf::Vertex(nodes[u].shape.getPosition(), sf::Color::Blue),
				sf::Vertex(nodes[v].shape.getPosition(), sf::Color::Blue)
			};
			window.draw(line, 2, sf::PrimitiveType::Lines);
		}

		for (auto& [id, node] : nodes)
			window.draw(node.shape);

		if (weightMode) {


			sf::Text txt(font);
			txt.setString("Weight: " + weightInput);
			txt.setCharacterSize(20);
			txt.setFillColor(sf::Color::White);
			txt.setPosition({ 20.f, 20.f });

			window.draw(txt);
		}

		window.display();
	}
		
	return 0;
}