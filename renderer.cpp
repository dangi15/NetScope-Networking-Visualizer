#include "Renderer.h"
#include "packet_anim.h"
#include <set>
#include <cmath>
#include <string>
using namespace std;

// -- Geometry helpers ---------------------------------------------------------

static void thickLine(sf::RenderWindow& window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color) {

    sf::Vector2f dir = p2 - p1;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f) return;
    sf::Vector2f u = dir / len, n(-u.y, u.x);
    sf::Vector2f off = n * (thickness / 2.f);

    sf::VertexArray q(sf::PrimitiveType::TriangleStrip, 4);
    q[0].position = p1 + off; q[1].position = p1 - off;
    q[2].position = p2 + off; q[3].position = p2 - off;
    for (int i = 0; i < 4; i++) q[i].color = color;
    window.draw(q);
}

static void drawArrowHead(sf::RenderWindow& window, sf::Vector2f tip, sf::Vector2f from, sf::Color color) {

    sf::Vector2f dir = tip - from;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f) return;
    sf::Vector2f u = dir / len, n(-u.y, u.x);
    sf::Vector2f base = tip - u * 14.f;

    sf::VertexArray tri(sf::PrimitiveType::Triangles, 3);
    tri[0].position = tip;
    tri[1].position = base + n * 7.f;
    tri[2].position = base - n * 7.f;
    tri[0].color = tri[1].color = tri[2].color = color;
    window.draw(tri);
}

// -- drawEdges ----------------------------------------------------------------

// Draw a dashed line between two points (alternating drawn/gap segments)
static void dashedLine(sf::RenderWindow& window,
    sf::Vector2f p1, sf::Vector2f p2, float thickness,
    sf::Color color, float dashLen = 10.f, float gapLen = 7.f) {

    sf::Vector2f dir = p2 - p1;
    float total = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (total < 0.001f) return;
    sf::Vector2f u = dir / total;
    float travelled = 0.f;
    bool drawing = true;
    while (travelled < total) {
        float segLen = drawing ? dashLen : gapLen;
        float end = std::min(travelled + segLen, total);
        if (drawing) {
            sf::Vector2f a = p1 + u * travelled;
            sf::Vector2f b = p1 + u * end;
            thickLine(window, a, b, thickness, color);
        }
        travelled = end;
        drawing = !drawing;
    }
}

void drawEdges(sf::RenderWindow& window, map<int, Node>& nodes,
    Graph& graph, sf::Font& font, bool directed) {

    set<pair<int, int>> drawn;

    for (auto& [u, neighbors] : graph.adj) {
        for (auto& [v, w] : neighbors) {
            if (!directed && drawn.count({ v,u })) continue;
            if (nodes.find(u) == nodes.end() || nodes.find(v) == nodes.end()) continue;

            auto pos1 = nodes[u].shape.getPosition();
            auto pos2 = nodes[v].shape.getPosition();
            sf::Vector2f dir = pos2 - pos1;
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (dist < 0.001f) continue;
            sf::Vector2f unitDir = dir / dist;

            sf::Vector2f lineStart = pos1 + unitDir * NODE_RADIUS;
            sf::Vector2f lineEnd = pos2 - unitDir * NODE_RADIUS;

            bool linkDown = graph.isLinkDown(u, v, directed);
            bool routerDown = nodes[u].down || nodes[v].down;

            if (linkDown) {
                // Failed link: dashed orange-red line
                dashedLine(window, lineStart, lineEnd, 3.f, sf::Color(220, 60, 60));

                // Draw a bold X at the midpoint
                sf::Vector2f mid((pos1.x + pos2.x) / 2.f, (pos1.y + pos2.y) / 2.f);
                float s = 8.f;
                sf::Vector2f perp(-unitDir.y, unitDir.x);
                // Two diagonal strokes of the X
                thickLine(window, mid - unitDir * s - perp * s,
                    mid + unitDir * s + perp * s, 3.f, sf::Color(255, 60, 60));
                thickLine(window, mid + unitDir * s - perp * s,
                    mid - unitDir * s + perp * s, 3.f, sf::Color(255, 60, 60));

            }
            else if (routerDown) {
                // One endpoint is down  -  draw greyed out solid line
                thickLine(window, lineStart, lineEnd, 3.f, sf::Color(100, 100, 100));
                if (directed) drawArrowHead(window, lineEnd, lineStart, sf::Color(100, 100, 100));

            }
            else {
                // Normal operational link  -  colour by latency
                sf::Color edgeColor;
                if (w <= 10)  edgeColor = sf::Color(40, 160, 40);
                else if (w <= 50)  edgeColor = sf::Color(200, 160, 0);
                else               edgeColor = sf::Color(200, 50, 50);

                thickLine(window, lineStart, lineEnd, 3.f, edgeColor);
                if (directed) drawArrowHead(window, lineEnd, lineStart, edgeColor);

                // Label: "10ms / 100Mbps"
                sf::Vector2f mid((pos1.x + pos2.x) / 2.f, (pos1.y + pos2.y) / 2.f);
                sf::Vector2f normal(-unitDir.y, unitDir.x);

                auto it = graph.linkProps.find({ u,v });
                string label = to_string(w) + "ms";
                if (it != graph.linkProps.end())
                    label += " / " + to_string(it->second.bandwidth) + "M";

                sf::Text wt(font);
                wt.setString(label);
                wt.setCharacterSize(13);
                wt.setFillColor(sf::Color::White);
                sf::FloatRect b = wt.getLocalBounds();
                wt.setOrigin({ b.size.x / 2.f, b.size.y / 2.f });
                wt.setPosition(mid + normal * 18.f);

                float angleDeg = std::atan2(unitDir.y, unitDir.x) * 180.f / 3.14159265f;
                if (angleDeg > 90.f || angleDeg < -90.f) angleDeg += 180.f;
                wt.setRotation(sf::degrees(angleDeg));
                window.draw(wt);
            }

            drawn.insert({ u,v });
        }
    }
}

// -- drawPath -----------------------------------------------------------------

void drawPath(sf::RenderWindow& window, map<int, Node>& nodes,
    vector<int>& path, bool isQoS) {

    // Latency path = blue, QoS/bandwidth path = purple
    sf::Color col = isQoS ? sf::Color(180, 60, 220) : sf::Color(0, 100, 255);
    for (int i = 0; i + 1 < (int)path.size(); i++) {
        int u = path[i], v = path[i + 1];
        if (!nodes.count(u) || !nodes.count(v)) continue;
        auto pos1 = nodes[u].shape.getPosition(), pos2 = nodes[v].shape.getPosition();
        sf::Vector2f dir = pos2 - pos1;
        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (dist < 0.001f) continue;
        sf::Vector2f ud = dir / dist;
        thickLine(window, pos1 + ud * NODE_RADIUS, pos2 - ud * NODE_RADIUS, 6.f, col);
        drawArrowHead(window, pos2 - ud * NODE_RADIUS, pos1 + ud * NODE_RADIUS, col);
    }
}

// -- drawNodes ----------------------------------------------------------------

void drawNodes(sf::RenderWindow& window, map<int, Node>& nodes, Graph& graph,
    sf::Font& font, const vector<int>& selectedNodes, const vector<int>& topoOrder) {

    for (auto& [id, node] : nodes) {
        if (node.down) {
            node.shape.setFillColor(sf::Color(130, 130, 130));
        }
        else {
            bool hasEdges = !graph.adj[id].empty();
            if (!hasEdges) {
                for (auto& [other, nbs] : graph.adj)
                    for (auto& [v, w] : nbs)
                        if (v == id) { hasEdges = true; break; }
            }
            node.shape.setFillColor(hasEdges ? sf::Color(40, 120, 200) : sf::Color(220, 80, 80));
        }
    }
    for (int id : selectedNodes)
        if (nodes.count(id)) nodes[id].shape.setFillColor(sf::Color(255, 200, 0));

    // Draw router icon outline
    for (auto& [id, node] : nodes) {
        sf::CircleShape outline = node.shape;
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineColor(node.down ? sf::Color(90, 90, 90) : sf::Color(255, 255, 255, 180));
        outline.setOutlineThickness(2.f);
        window.draw(node.shape);
        window.draw(outline);
    }

    for (auto& [id, node] : nodes) {
        string label = node.name;
        if (!topoOrder.empty()) {
            for (int i = 0; i < (int)topoOrder.size(); i++)
                if (topoOrder[i] == id) { label += "[" + to_string(i) + "]"; break; }
        }
        sf::Text txt(font);
        txt.setString(label);
        txt.setCharacterSize(13);
        txt.setFillColor(sf::Color::White);
        txt.setStyle(sf::Text::Bold);
        sf::FloatRect b = txt.getLocalBounds();
        txt.setOrigin({ b.size.x / 2.f, b.size.y / 2.f });
        txt.setPosition({ node.shape.getPosition().x, node.shape.getPosition().y });
        if (node.down) txt.setFillColor(sf::Color(200, 200, 200));
        window.draw(txt);
    }
}

// -- drawHUD ------------------------------------------------------------------

void drawHUD(sf::RenderWindow& window, sf::Font& font, bool isDirected,
    const string& activeMode, bool weightMode,
    int u, int v, const string& latencyInput, const string& bwInput, bool enteringBw,
    const vector<int>& topoOrder) {

    sf::RectangleShape bar({ 1350.f, 72.f });
    bar.setFillColor(sf::Color(30, 30, 50));
    bar.setPosition({ 0.f, 0.f });
    window.draw(bar);

    sf::RectangleShape div({ 1350.f, 1.f });
    div.setFillColor(sf::Color(70, 70, 100));
    div.setPosition({ 0.f, 28.f });
    window.draw(div);

    sf::RectangleShape div2({ 1350.f, 1.f });
    div2.setFillColor(sf::Color(70, 70, 100));
    div2.setPosition({ 0.f, 50.f });
    window.draw(div2);

    // Row 1: topology type + active mode
    sf::Text modeText(font);
    modeText.setCharacterSize(16);
    modeText.setFillColor(sf::Color(130, 200, 255));
    modeText.setPosition({ 12.f,5.f });
    string modeStr = string(isDirected ? "Directed" : "Undirected") + " Network";
    if (!activeMode.empty()) modeStr += "  |  " + activeMode;
    modeText.setString(modeStr);
    window.draw(modeText);

    // Row 2: interaction hints
    sf::Text hint(font);
    hint.setCharacterSize(12);
    hint.setFillColor(sf::Color(160, 160, 180));
    hint.setString("LClick=place router | RClick=select | Shift+RClick=link failure | Ctrl+RClick=delete link | MClick=delete router | X=router up/down | T=directed | C=clear | Esc=quit");
    hint.setPosition({ 12.f, 33.f });
    window.draw(hint);

    // Row 3: algorithm keys
    sf::Text hint2(font);
    hint2.setCharacterSize(12);
    hint2.setFillColor(sf::Color(130, 160, 200));
    hint2.setString("B=Flood(BFS) | D=OSPF(Dijkstra) | F=RIP(Bellman-Ford) | Q=QoS(MaxBW) | K=schedule | Tab=routing table | P=pause packet | +=faster | -=slower");
    hint2.setPosition({ 12.f, 55.f });
    window.draw(hint2);

    // Link property prompt
    if (weightMode) {
        sf::RectangleShape bg({ 520.f, 60.f });
        bg.setFillColor(sf::Color(255, 255, 220));
        bg.setOutlineColor(sf::Color(180, 160, 0));
        bg.setOutlineThickness(1.f);
        bg.setPosition({ 12.f, 78.f });
        window.draw(bg);

        string prompt;
        if (!enteringBw)
            prompt = "Latency (ms) for R" + to_string(u) + " -> R" + to_string(v) + ": " + latencyInput + "_   (Enter to confirm)";
        else
            prompt = "Bandwidth (Mbps) for R" + to_string(u) + " -> R" + to_string(v) + ": " + bwInput + "_   (Enter to confirm)";

        sf::Text wt(font);
        wt.setString(prompt);
        wt.setCharacterSize(15);
        wt.setFillColor(sf::Color(60, 40, 0));
        wt.setPosition({ 18.f, 90.f });
        window.draw(wt);
    }

    // Kahn's topological order strip at bottom
    if (!topoOrder.empty()) {
        sf::RectangleShape bar2({ 1350.f,26.f });
        bar2.setFillColor(sf::Color(20, 40, 80));
        bar2.setPosition({ 0.f,674.f });
        window.draw(bar2);

        string orderStr = "Packet schedule order: ";
        for (int i = 0; i < (int)topoOrder.size(); i++) {
            orderStr += "R" + to_string(topoOrder[i]);
            if (i + 1 < (int)topoOrder.size()) orderStr += " -> ";
        }
        sf::Text ot(font);
        ot.setString(orderStr);
        ot.setCharacterSize(14);
        ot.setFillColor(sf::Color(180, 210, 255));
        ot.setPosition({ 12.f,678.f });
        window.draw(ot);
    }
}

// -- drawRoutingTable ---------------------------------------------------------

void drawRoutingTable(sf::RenderWindow& window, sf::Font& font,
    const map<int, pair<int, int>>& table,
    const map<int, Node>& nodes, int srcId) {

    float panelW = 220.f, panelX = 1250.f - panelW;
    float rowH = 22.f;
    float panelH = rowH * (table.size() + 2) + 10.f;
    float panelY = 60.f;

    sf::RectangleShape bg({ panelW, panelH });
    bg.setFillColor(sf::Color(20, 20, 40, 230));
    bg.setOutlineColor(sf::Color(80, 120, 200));
    bg.setOutlineThickness(1.f);
    bg.setPosition({ panelX, panelY });
    window.draw(bg);

    // Header
    string srcName = nodes.count(srcId) ? nodes.at(srcId).name : "R?";
    sf::Text hdr(font);
    hdr.setString("Routing table: " + srcName);
    hdr.setCharacterSize(13);
    hdr.setFillColor(sf::Color(130, 200, 255));
    hdr.setStyle(sf::Text::Bold);
    hdr.setPosition({ panelX + 8.f, panelY + 5.f });
    window.draw(hdr);

    sf::Text colHdr(font);
    colHdr.setString("Dest       Cost    NextHop");
    colHdr.setCharacterSize(11);
    colHdr.setFillColor(sf::Color(180, 180, 200));
    colHdr.setPosition({ panelX + 8.f, panelY + rowH + 4.f });
    window.draw(colHdr);

    int row = 0;
    for (auto& [dest, cn] : table) {
        auto [cost, nextHop] = cn;
        string destName = nodes.count(dest) ? nodes.at(dest).name : to_string(dest);
        string nhName = nodes.count(nextHop) ? nodes.at(nextHop).name : to_string(nextHop);

        string line = destName;
        while ((int)line.size() < 10) line += ' ';
        line += to_string(cost) + "ms";
        while ((int)line.size() < 19) line += ' ';
        line += nhName;

        sf::Text entry(font);
        entry.setString(line);
        entry.setCharacterSize(12);
        entry.setFillColor(sf::Color(220, 230, 255));
        entry.setPosition({ panelX + 8.f, panelY + rowH * (row + 2) + 4.f });
        window.draw(entry);
        row++;
    }
}

// -- drawEventLog -------------------------------------------------------------

void drawEventLog(sf::RenderWindow& window, sf::Font& font, const vector<string>& log) {
    if (log.empty()) return;
    int show = min((int)log.size(), 4);
    float lineH = 18.f;
    float barH = show * lineH + 8.f;
    float barY = 670.f - barH;

    sf::RectangleShape bg({ 1350.f, barH });
    bg.setFillColor(sf::Color(10, 10, 20, 210));
    bg.setPosition({ 0.f, barY });
    window.draw(bg);

    for (int i = 0; i < show; i++) {
        sf::Text t(font);
        t.setString(log[log.size() - show + i]);
        t.setCharacterSize(13);
        // Fade older lines
        int alpha = 120 + (i * 135 / show);
        t.setFillColor(sf::Color(160, 220, 160, alpha));
        t.setPosition({ 10.f, barY + 4.f + i * lineH });
        window.draw(t);
    }
}

// -- updateAndDrawPacket -------------------------------------------------------

void updateAndDrawPacket(sf::RenderWindow& window,
    PacketAnim& pkt,
    const map<int, Node>& nodes,
    float dt) {
    if (!pkt.active || pkt.path.size() < 2) return;

    // -- Advance t ------------------------------------------------------------
    if (!pkt.paused) {
        // Get the latency of the current edge so faster links feel faster.
        // We normalise so a 10ms link is crossed in 1/speed seconds baseline.
        pkt.t += pkt.speed * dt;

        if (pkt.t >= 1.f) {
            pkt.t = 0.f;
            pkt.edge++;
            if (pkt.edge >= (int)pkt.path.size() - 1) {
                if (pkt.loop)
                    pkt.edge = 0;   // restart
                else {
                    pkt.active = false;
                    return;
                }
            }
        }
    }

    // -- Compute world position ------------------------------------------------
    int u = pkt.path[pkt.edge];
    int v = pkt.path[pkt.edge + 1];
    if (!nodes.count(u) || !nodes.count(v)) return;

    sf::Vector2f p1 = nodes.at(u).shape.getPosition();
    sf::Vector2f p2 = nodes.at(v).shape.getPosition();

    // Ease in-out so the packet visibly accelerates/decelerates at each hop
    float ease = pkt.t * pkt.t * (3.f - 2.f * pkt.t);
    sf::Vector2f pos = p1 + (p2 - p1) * ease;

    pkt.pushTrail(pos);

    // -- Draw motion-blur trail ------------------------------------------------
    // Iterate oldest -> newest through the ring buffer
    for (int i = 0; i < PacketAnim::TRAIL_LEN; i++) {
        int idx = (pkt.trailHead + i) % PacketAnim::TRAIL_LEN;
        float frac = (float)(i + 1) / PacketAnim::TRAIL_LEN;  // 0=oldest 1=newest

        float radius = 4.f * frac;
        uint8_t alpha = (uint8_t)(180.f * frac * frac);

        // Colour shifts from cyan (old) to white (new)
        uint8_t r = (uint8_t)(100 + 155 * frac);
        uint8_t g = (uint8_t)(200);
        uint8_t b = 255;

        sf::CircleShape ghost(radius);
        ghost.setOrigin({ radius, radius });
        ghost.setPosition(pkt.trail[idx]);
        ghost.setFillColor(sf::Color(r, g, b, alpha));
        window.draw(ghost);
    }

    // -- Draw packet head -----------------------------------------------------
    // Outer glow ring
    sf::CircleShape glow(14.f);
    glow.setOrigin({ 14.f, 14.f });
    glow.setPosition(pos);
    glow.setFillColor(sf::Color(100, 220, 255, 50));
    window.draw(glow);

    // Inner bright core
    sf::CircleShape head(7.f);
    head.setOrigin({ 7.f, 7.f });
    head.setPosition(pos);
    head.setFillColor(sf::Color(200, 240, 255));
    head.setOutlineColor(sf::Color::White);
    head.setOutlineThickness(1.5f);
    window.draw(head);

    // -- Draw hop counter label beneath the packet -----------------------------
    // (shows which hop out of total, e.g. "2/4")
    // Drawn without font dependency  -  just a small overlay near the packet.
    // Full text label is handled by caller passing font if desired.
}