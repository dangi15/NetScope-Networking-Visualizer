#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include "Node.h"
using namespace std;

void drawEdges(sf::RenderWindow& window, map<int, Node>& nodes, map<int, std::vector<std::pair<int, int>>>& graph, sf::Font& font);

void drawNodes(sf::RenderWindow& window, map<int, Node>& nodes, map<int, std::vector<std::pair<int, int>>>& graph, sf::Font& font);

void drawPath(sf::RenderWindow& window, map<int, Node>& nodes, vector<int>& path);