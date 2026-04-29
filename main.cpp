#define _USE_MATH_DEFINES
#include <iostream>
#include <optional>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <SFML/Graphics.hpp>

#include "Graph.h"
#include "Algorithms.h"
#include "Node.h"
#include "Renderer.h"
#include "packet_anim.h"

using namespace std;

// -- Helpers ------------------------------------------------------------------

static int hitTest(const map<int, Node>& nodes, sf::Vector2f p) {
    for (auto& [id, node] : nodes)
        if (node.shape.getGlobalBounds().contains(p)) return id;
    return -1;
}

// Returns the {u,v} pair of the edge whose midpoint is within `tol` pixels of click position `p`, or {-1,-1} if none.
static pair<int, int> linkHitTest(const Graph& graph, const map<int, Node>& nodes, sf::Vector2f p, float tol = 14.f) {
    for (auto& [u, neighbors] : graph.adj) {
        for (auto& [v, w] : neighbors) {
            if (!nodes.count(u) || !nodes.count(v)) continue;
            sf::Vector2f p1 = nodes.at(u).shape.getPosition();
            sf::Vector2f p2 = nodes.at(v).shape.getPosition();
            // Project p onto the segment, measure perpendicular distance
            sf::Vector2f seg = p2 - p1;
            sf::Vector2f toP = p - p1;
            float segLen2 = seg.x * seg.x + seg.y * seg.y;
            if (segLen2 < 0.001f) continue;
            float t = std::max(0.f, std::min(1.f, (toP.x * seg.x + toP.y * seg.y) / segLen2));
            sf::Vector2f closest = p1 + seg * t;
            sf::Vector2f diff = p - closest;
            if (diff.x * diff.x + diff.y * diff.y < tol * tol)
                return { u, v };
        }
    }
    return { -1, -1 };
}

// Returns a copy of the adjacency list with all down routers and down links removed.
// All algorithm calls use this so that offline routers/links are never part of any route.
static AdjList filterGraph(const Graph& graph, const map<int, Node>& nodes, bool directed) {
    AdjList filtered;
    for (auto& [u, neighbors] : graph.adj) {
        if (nodes.count(u) && nodes.at(u).down) continue;
        filtered[u] = {};
        for (auto& [v, w] : neighbors) {
            if (nodes.count(v) && nodes.at(v).down) continue;
            if (graph.isLinkDown(u, v, directed))   continue;
            filtered[u].push_back({ v, w });
        }
    }
    return filtered;
}

// Returns a bandwidth adjacency list {neighbour, bandwidth_mbps} for QoS routing,
// with down routers and down links excluded — same filtering rules as filterGraph.
static BwAdjList filterBwGraph(const Graph& graph, const map<int, Node>& nodes, bool directed) {
    BwAdjList bw;
    for (auto& [u, neighbors] : graph.adj) {
        if (nodes.count(u) && nodes.at(u).down) continue;
        bw[u] = {};
        for (auto& [v, latency] : neighbors) {
            if (nodes.count(v) && nodes.at(v).down)       continue;
            if (graph.isLinkDown(u, v, directed))          continue;
            auto it = graph.linkProps.find({ u, v });
            int bwVal = (it != graph.linkProps.end()) ? it->second.bandwidth : 100;
            bw[u].push_back({ v, bwVal });
        }
    }
    return bw;
}

static void pushLog(vector<string>& log, const string& msg) {
    log.push_back(msg);
    if (log.size() > 50) log.erase(log.begin());
}

// -- Main ---------------------------------------------------------------------

int main(int argc, char* argv[]) {
    sf::RenderWindow window(sf::VideoMode({ 1250,700 }), "NetScope: Network Visualiser");
    window.setFramerateLimit(60);

    Graph graph;
    map<int, Node> nodes;

    // Routing state
    vector<int> finalPath;
    vector<int> topoOrder;
    vector<int> algoSelect, selectedNodes;
    int algoMode = 0;  // 0=none 1=BFS/Flood 2=Dijkstra/OSPF 3=BellmanFord/RIP 4=QoS/MaxBandwidth
    int lastAlgoMode = 0; // algoMode that produced the current finalPath (for colouring)
    const char* modeLabels[] = {
        "",
        "Flood (BFS): pick source router...",
        "OSPF (Dijkstra): pick source router...",
        "RIP (Bellman-Ford): pick source router...",
        "QoS (Max Bandwidth): pick source router..."
    };

    // Routing table panel
    map<int, pair<int, int>> routingTable;
    int routingTableSrc = -1;
    bool showRoutingTable = false;

    // Event log
    vector<string> eventLog;

    // Packet animation
    PacketAnim packet;
    sf::Clock frameClock;   // used to compute dt each frame

    // Graph properties
    bool isDirected = false;
    // Link input state
    bool weightMode = false;
    bool enteringBw = false;   // false=entering latency, true=entering bandwidth
    string latencyInput, bwInput;
    int edgeU = -1, edgeV = -1;

    int nodeId = 0;
    int lastHoveredNode = -1;  // tracks whichever node the mouse is currently over
    int dragNode = -1;         // node currently being dragged (-1 = none)
    sf::Vector2f dragOffset;   // offset from node centre to click point, keeps drag smooth

    // Font loading
    sf::Font font;
    font.openFromFile("segoeui.ttf");

    auto clearAll = [&]() {
        graph.adj.clear(); graph.linkProps.clear(); graph.downLinks.clear();
        nodes.clear(); finalPath.clear(); topoOrder.clear();
        algoSelect.clear(); selectedNodes.clear();
        routingTable.clear(); routingTableSrc = -1; showRoutingTable = false;
        algoMode = 0; lastAlgoMode = 0; weightMode = false; enteringBw = false; nodeId = 0;
        dragNode = -1;
        eventLog.clear();
        packet.stop();
        };

    // -- Event loop ------------------------------------------------------------

    while (window.isOpen()) {
        while (const optional ev = window.pollEvent()) {

            if (ev->is<sf::Event::Closed>()) window.close();

            else if (const auto* key = ev->getIf<sf::Event::KeyPressed>()) {
                auto sc = key->scancode;

                if (sc == sf::Keyboard::Scancode::Escape) window.close();
                if (sc == sf::Keyboard::Scancode::C) clearAll();

                if (sc == sf::Keyboard::Scancode::T && !weightMode) {
                    isDirected = !isDirected;
                    finalPath.clear(); topoOrder.clear();
                    pushLog(eventLog, string("Network mode: ") + (isDirected ? "Directed" : "Undirected"));
                }

                // Algorithm mode selection
                if (!weightMode) {
                    if (sc == sf::Keyboard::Scancode::B) {
                        algoMode = 1; algoSelect.clear(); selectedNodes.clear();
                        finalPath.clear(); topoOrder.clear(); showRoutingTable = false;
                        pushLog(eventLog, "Flood (BFS) mode: right-click source, then destination");
                    }
                    if (sc == sf::Keyboard::Scancode::D) {
                        algoMode = 2; algoSelect.clear(); selectedNodes.clear();
                        finalPath.clear(); topoOrder.clear(); showRoutingTable = false;
                        pushLog(eventLog, "OSPF/Dijkstra mode: right-click source, then destination");
                    }
                    if (sc == sf::Keyboard::Scancode::F) {
                        algoMode = 3; algoSelect.clear(); selectedNodes.clear();
                        finalPath.clear(); topoOrder.clear(); showRoutingTable = false;
                        pushLog(eventLog, "RIP/Bellman-Ford mode: right-click source, then destination");
                    }
                    if (sc == sf::Keyboard::Scancode::Q) {
                        algoMode = 4; algoSelect.clear(); selectedNodes.clear();
                        finalPath.clear(); topoOrder.clear(); showRoutingTable = false;
                        pushLog(eventLog, "QoS mode: finds path with highest bottleneck bandwidth");
                    }

                    if (sc == sf::Keyboard::Scancode::K) {
                        if (!isDirected)
                            pushLog(eventLog, "Packet scheduling (Kahn's): requires a directed network  -  press T to switch");
                        else if (graph.adj.empty())
                            pushLog(eventLog, "Packet scheduling (Kahn's): no routers in the network");
                        else {
                            finalPath.clear();
                            topoOrder = KahnsAlgorithm(graph.adj);
                            if (topoOrder.empty())
                                pushLog(eventLog, "Packet scheduling (Kahn's): cycle detected  -  no valid order");
                            else {
                                string msg = "Packet schedule: ";
                                for (int i = 0; i < (int)topoOrder.size(); i++) {
                                    msg += (nodes.count(topoOrder[i]) ? nodes[topoOrder[i]].name : "R?");
                                    if (i + 1 < (int)topoOrder.size()) msg += " -> ";
                                }
                                pushLog(eventLog, msg);
                            }
                        }
                    }

                    // Tab: build and show routing table for first selected node (or node 0)
                    if (sc == sf::Keyboard::Scancode::Tab) {
                        showRoutingTable = !showRoutingTable;
                        if (showRoutingTable) {
                            routingTableSrc = algoSelect.empty() ?
                                (nodes.empty() ? -1 : nodes.begin()->first) : algoSelect[0];
                            if (routingTableSrc != -1) {
                                { AdjList live = filterGraph(graph, nodes, isDirected); routingTable = buildRoutingTable(routingTableSrc, live); }
                                string nm = nodes.count(routingTableSrc) ? nodes[routingTableSrc].name : "?";
                                pushLog(eventLog, "Routing table shown for " + nm);
                            }
                        }
                    }

                    // P: pause / resume packet animation
                    if (sc == sf::Keyboard::Scancode::P && packet.active) {
                        packet.paused = !packet.paused;
                        pushLog(eventLog, packet.paused ? "Packet paused" : "Packet resumed");
                    }

                    // Space: retrigger packet on the current path (restart from source)
                    if (sc == sf::Keyboard::Scancode::Space && !finalPath.empty()) {
                        packet.start(finalPath, packet.speed);
                        pushLog(eventLog, "Packet restarted");
                    }

                    // + / - : adjust packet speed
                    if (sc == sf::Keyboard::Scancode::Equal) {   // = / + key
                        packet.speed = std::min(packet.speed + 0.5f, 8.f);
                        pushLog(eventLog, "Packet speed: " + to_string((int)(packet.speed * 10) / 10) + "x");
                    }
                    if (sc == sf::Keyboard::Scancode::Hyphen) {  // - key
                        packet.speed = std::max(packet.speed - 0.5f, 0.5f);
                        pushLog(eventLog, "Packet speed: " + to_string((int)(packet.speed * 10) / 10) + "x");
                    }

                    // X key: toggle the hovered router up/down (simulate failure/recovery)
                    if (sc == sf::Keyboard::Scancode::X && lastHoveredNode != -1) {
                        auto& n = nodes[lastHoveredNode];
                        n.down = !n.down;
                        finalPath.clear(); topoOrder.clear();
                        pushLog(eventLog, n.name + (n.down ? " went DOWN (simulating failure)" : " came UP (recovered)"));
                    }
                }

                // Link weight input
                if (weightMode) {
                    string& cur = enteringBw ? bwInput : latencyInput;

                    if (sc == sf::Keyboard::Scancode::Enter) {
                        if (!enteringBw) {
                            // Done with latency, move to bandwidth
                            enteringBw = true;
                        }
                        else {
                            // Commit edge
                            int lat = 1, bw = 100;
                            try { if (!latencyInput.empty()) lat = max(1, stoi(latencyInput)); }
                            catch (...) {}
                            try { if (!bwInput.empty())     bw = max(1, stoi(bwInput)); }
                            catch (...) {}
                            graph.addEdge(edgeU, edgeV, { lat, bw }, isDirected);
                            string nm_u = nodes.count(edgeU) ? nodes[edgeU].name : "R?";
                            string nm_v = nodes.count(edgeV) ? nodes[edgeV].name : "R?";
                            pushLog(eventLog, "Link added: " + nm_u + " <-> " + nm_v +
                                "  " + to_string(lat) + "ms / " + to_string(bw) + "Mbps");
                            weightMode = false; enteringBw = false;
                            latencyInput.clear(); bwInput.clear();
                        }
                    }
                    else if (sc == sf::Keyboard::Scancode::Backspace) {
                        if (!cur.empty()) cur.pop_back();
                    }
                    else if (sc == sf::Keyboard::Scancode::Escape) {
                        weightMode = false; 
                        enteringBw = false;
                        latencyInput.clear(); 
                        bwInput.clear();
                    }
                }
            }

            else if (const auto* text = ev->getIf<sf::Event::TextEntered>()) {
                if (weightMode) {
                    string& cur = enteringBw ? bwInput : latencyInput;
                    char c = static_cast<char>(text->unicode);
                    if (isdigit(c)) cur += c;
                }
            }

            else if (const auto* mouse = ev->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f pos = window.mapPixelToCoords(mouse->position);

                if (mouse->button == sf::Mouse::Button::Left) {
                    int hit = hitTest(nodes, pos);
                    if (hit != -1) {
                        // Clicked an existing node — start dragging it
                        dragNode = hit;
                        dragOffset = nodes[hit].shape.getPosition() - pos;
                    }
                    else {
                        // Clicked empty space — place a new router
                        sf::CircleShape circle(NODE_RADIUS);
                        circle.setFillColor(sf::Color(40, 120, 200));
                        circle.setOrigin({ NODE_RADIUS, NODE_RADIUS });
                        circle.setPosition(pos);
                        string name = "R" + to_string(nodeId);
                        nodes[nodeId] = { nodeId, name, circle, false };
                        graph.addNode(nodeId);
                        pushLog(eventLog, "Router " + name + " added");
                        nodeId++;
                    }
                }

                else if (mouse->button == sf::Mouse::Button::Right) {
                    if (weightMode) continue;

                    // Shift+right-click: toggle link failure on the clicked edge
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
                        auto [lu, lv] = linkHitTest(graph, nodes, pos);
                        if (lu != -1) {
                            graph.toggleLink(lu, lv, isDirected);
                            bool nowDown = graph.isLinkDown(lu, lv, isDirected);
                            string nu = nodes.count(lu) ? nodes[lu].name : "?";
                            string nv = nodes.count(lv) ? nodes[lv].name : "?";
                            finalPath.clear(); packet.stop();
                            pushLog(eventLog, "Link " + nu + " <-> " + nv +
                                (nowDown ? " FAILED (cable cut)" : " RESTORED"));
                        }
                        continue;
                    }

                    int clicked = hitTest(nodes, pos);
                    if (clicked == -1) continue;

                    finalPath.clear(); topoOrder.clear();

                    if (algoMode != 0) {
                        algoSelect.push_back(clicked);
                        selectedNodes.push_back(clicked);

                        if (algoSelect.size() == 2) {
                            int src = algoSelect[0], dst = algoSelect[1];
                            string srcN = nodes.count(src) ? nodes[src].name : "?";
                            string dstN = nodes.count(dst) ? nodes[dst].name : "?";

                            RouteResult res;
                            string algo;
                            AdjList live = filterGraph(graph, nodes, isDirected);
                            if (algoMode == 1) { res = BFS(src, dst, live);                          algo = "Flood (BFS)"; }
                            if (algoMode == 2) { res = Dijkstra(src, dst, live);                      algo = "OSPF (Dijkstra)"; }
                            if (algoMode == 3) { res = BellmanFord(src, dst, (int)live.size(), live);  algo = "RIP (Bellman-Ford)"; }
                            if (algoMode == 4) {
                                BwAdjList bwLive = filterBwGraph(graph, nodes, isDirected);
                                res = MaxBandwidth(src, dst, bwLive);
                                algo = "QoS (Max BW)";
                            }

                            if (res.negativeWeights)
                                pushLog(eventLog, "OSPF/Dijkstra: negative-latency link detected  -  use RIP (F) instead");
                            else if (res.negativeCycle)
                                pushLog(eventLog, "RIP: negative cycle detected  -  routing loop!");
                            else if (res.noPath)
                                pushLog(eventLog, algo + ": no route from " + srcN + " to " + dstN);
                            else {
                                finalPath = res.path;
                                // Launch packet animation along the new path
                                lastAlgoMode = algoMode;
                                packet.start(finalPath);
                                // Build path string with router names
                                string pathStr = algo + ": ";
                                for (int i = 0; i < (int)finalPath.size(); i++) {
                                    pathStr += nodes.count(finalPath[i]) ? nodes[finalPath[i]].name : "?";
                                    if (i + 1 < (int)finalPath.size()) pathStr += " -> ";
                                }
                                if (res.bottleneckBw > 0)
                                    pathStr += "  [bottleneck: " + to_string(res.bottleneckBw) + " Mbps]";
                                else
                                    pathStr += "  [" + to_string(res.totalCost) + "ms total]";
                                pushLog(eventLog, pathStr);
                            }

                            algoSelect.clear(); selectedNodes.clear(); algoMode = 0;
                        }
                    }
                    else {
                        // Edge-drawing: select two routers
                        algoSelect.push_back(clicked);
                        selectedNodes.push_back(clicked);
                        if (algoSelect.size() == 2) {
                            edgeU = algoSelect[0]; edgeV = algoSelect[1];
                            algoSelect.clear(); selectedNodes.clear();
                            if (edgeU != edgeV) { weightMode = true; enteringBw = false; latencyInput.clear(); bwInput.clear(); }
                        }
                    }
                }

                else if (mouse->button == sf::Mouse::Button::Middle) {
                    int clicked = hitTest(nodes, pos);
                    if (clicked != -1) {
                        string nm = nodes[clicked].name;
                        graph.removeNode(clicked);
                        nodes.erase(clicked);
                        finalPath.clear(); topoOrder.clear();
                        algoSelect.clear(); selectedNodes.clear();
                        pushLog(eventLog, "Router " + nm + " removed");
                    }
                }
            }
            else if (const auto* moved = ev->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2f pos = window.mapPixelToCoords(moved->position);
                lastHoveredNode = hitTest(nodes, pos);

                // Move the dragged node with the mouse
                if (dragNode != -1 && nodes.count(dragNode))
                    nodes[dragNode].shape.setPosition(pos + dragOffset);
            }

            else if (const auto* released = ev->getIf<sf::Event::MouseButtonReleased>()) {
                if (released->button == sf::Mouse::Button::Left)
                    dragNode = -1;
            }
        }

        float dt = frameClock.restart().asSeconds();

        window.clear(sf::Color(18, 18, 28));

        drawEdges(window, nodes, graph, font, isDirected);
        drawPath(window, nodes, finalPath, lastAlgoMode == 4);
        drawNodes(window, nodes, graph, font, selectedNodes, topoOrder);
        updateAndDrawPacket(window, packet, nodes, dt);

        if (showRoutingTable && routingTableSrc != -1)
            drawRoutingTable(window, font, routingTable, nodes, routingTableSrc);

        // Build mode status string
        string activeModeStr;
        if (algoMode != 0) {
            activeModeStr = modeLabels[algoMode];
            if (algoSelect.size() == 1) {
                string srcN = nodes.count(algoSelect[0]) ? nodes[algoSelect[0]].name : "?";
                activeModeStr = string(modeLabels[algoMode]);
                // Replace "source router..." with "dest router... (src=Rx)"
                size_t pos2 = activeModeStr.find("source");
                if (pos2 != string::npos)
                    activeModeStr = activeModeStr.substr(0, pos2) + "destination router...  (src=" + srcN + ")";
            }
        }

        drawHUD(window, font, isDirected, activeModeStr,
            weightMode, edgeU, edgeV, latencyInput, bwInput, enteringBw,
            topoOrder);

        drawEventLog(window, font, eventLog);

        window.display();
    }
    return 0;
}