#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include <string>
#include "Node.h"
#include "Graph.h"
#include "packet_anim.h"
using namespace std;

// Draw all links; directed=true draws arrowheads; shows latency + bandwidth
void drawEdges(sf::RenderWindow& window, map<int, Node>& nodes, Graph& graph, sf::Font& font, bool directed);

// Highlight the active route; isQoS=true draws in purple instead of blue
void drawPath(sf::RenderWindow& window, map<int, Node>& nodes, vector<int>& path, bool isQoS = false);

// Draw routers with status colours; highlights selectedNodes yellow; annotates topo rank
void drawNodes(sf::RenderWindow& window, map<int, Node>& nodes, Graph& graph, sf::Font& font, const vector<int>& selectedNodes, const vector<int>& topoOrder);

// Top HUD: mode, hints, weight prompt
void drawHUD(sf::RenderWindow& window, sf::Font& font, bool isDirected, const string& activeMode, bool weightMode, int u, int v, const string& latencyInput, const string& bwInput, bool enteringBw,
    const vector<int>& topoOrder);

// Right-side routing table panel
void drawRoutingTable(sf::RenderWindow& window, sf::Font& font, const map<int, pair<int, int>>& table, const map<int, Node>& nodes, int srcId);

// Bottom event log (last N messages)
void drawEventLog(sf::RenderWindow& window, sf::Font& font, const vector<string>& log);

// Advance packet animation state by dt seconds, then draw the packet + trail
void updateAndDrawPacket(sf::RenderWindow& window, PacketAnim& pkt, const map<int, Node>& nodes, float dt);