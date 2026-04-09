#define _USE_MATH_DEFINES
#include <iostream>
#include <optional>
#include <map>
#include <vector>
#include <SFML/Graphics.hpp>

#include "Graph.h"
#include "Algorithms.h"
#include "Node.h"
#include "Renderer.h"

using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1000, 700 }), "Network Visualizer");
    window.setFramerateLimit(60);

    Graph graph;
    map<int, Node> nodes;

    vector<int> finalPath;
    vector<int> bfsSelect, dijkstraSelect, selected;

    bool bfsMode = false, dijkstraMode = false, weightMode = false;

    string weightInput = "";
    int u = -1, v = -1;

    int nodeId = 0;

    sf::Font font;
    if (!font.openFromFile("D:\\coding\\c++\\daaPBL\\daaPBL\\Sakire.ttf")) {
        cout << "Font load failed\n";
        return -1;
    }

    while (window.isOpen()) {
        while (const optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();

                if (key->scancode == sf::Keyboard::Scancode::C) {
                    graph.adj.clear();
                    nodes.clear();
                    finalPath.clear();
                    nodeId = 0;
                }

                if (key->scancode == sf::Keyboard::Scancode::B) {
                    bfsMode = true;
                    dijkstraMode = false;
                    bfsSelect.clear();
                }

                if (key->scancode == sf::Keyboard::Scancode::D) {
                    dijkstraMode = true;
                    bfsMode = false;
                    dijkstraSelect.clear();
                }

                if (weightMode) {
                    if (key->scancode == sf::Keyboard::Scancode::Enter) {
                        int w = weightInput.empty() ? 1 : stoi(weightInput);
                        if (!graph.edgeExists(u, v)) {
                            graph.addEdge(u, v, w);
                        }
                        weightMode = false;
                    }
                    else if (key->scancode == sf::Keyboard::Scancode::Backspace) {
                        if (!weightInput.empty())
                            weightInput.pop_back();
                    }
                }
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button == sf::Mouse::Button::Left) {
                    sf::Vector2f pos = window.mapPixelToCoords(mouse->position);

                    sf::CircleShape circle(20.f);
                    circle.setFillColor(sf::Color::Red);
                    circle.setOrigin({ 20.f, 20.f });
                    circle.setPosition(pos);

                    nodes[nodeId] = { nodeId, circle };
                    graph.addNode(nodeId);

                    nodeId++;
                }

                if (mouse->button == sf::Mouse::Button::Right) {
                    sf::Vector2f clickPos = window.mapPixelToCoords(mouse->position);
                    finalPath.clear();

                    if (bfsMode) {
                        for (auto& [id, node] : nodes) {
                            if (node.shape.getGlobalBounds().contains(clickPos)) {
                                bfsSelect.push_back(id);
                                break;
                            }
                        }

                        if (bfsSelect.size() == 2) {
                            finalPath = BFS(bfsSelect[0], bfsSelect[1], graph.adj);
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
                            finalPath = Dijkstra(dijkstraSelect[0], dijkstraSelect[1], graph.adj);
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

            else if (const auto* text = event->getIf<sf::Event::TextEntered>()) {
                if (weightMode) {
                    if (isdigit(text->unicode))
                        weightInput += static_cast<char>(text->unicode);
                }
            }
        }

        window.clear(sf::Color::White);

        drawEdges(window, nodes, graph.adj, font);
        drawPath(window, nodes, finalPath);
        drawNodes(window, nodes, graph.adj, font);

        if (weightMode) {
            sf::Text txt(font);
            txt.setString("Weight: " + weightInput);
            txt.setCharacterSize(20);
            txt.setFillColor(sf::Color::Black);
            txt.setPosition({ 20.f, 20.f });
            window.draw(txt);
        }

        window.display();
    }

    return 0;
}