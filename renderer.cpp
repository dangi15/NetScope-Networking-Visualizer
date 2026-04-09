#include "Renderer.h"
#include <set>
using namespace std;

void drawEdges(sf::RenderWindow& window, map<int, Node>& nodes, map<int, vector<pair<int, int>>>& graph, sf::Font& font) {
    set<pair<int, int>> drawn;

    for (auto& [u, neighbors] : graph) {
        for (auto& [v, w] : neighbors) {
            if (drawn.count({ v, u })) continue;

            auto pos1 = nodes[u].shape.getPosition();
            auto pos2 = nodes[v].shape.getPosition();

            sf::Vertex line[] = {
                sf::Vertex(pos1),
                sf::Vertex(pos2)
            };
            line[0].color = sf::Color::Black;
            line[1].color = sf::Color::Black;

            window.draw(line, 2, sf::PrimitiveType::Lines);

            sf::Vector2f mid(
                (pos1.x + pos2.x) / 2.f,
                (pos1.y + pos2.y) / 2.f
            );

            sf::Text weightText(font);
            weightText.setString(to_string(w));
            weightText.setCharacterSize(20);
            weightText.setFillColor(sf::Color::Black);
            weightText.setStyle(sf::Text::Bold);

            sf::FloatRect bounds = weightText.getLocalBounds();
            weightText.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            weightText.setPosition(mid);

            window.draw(weightText);

            drawn.insert({ u, v });
        }
    }
}

void drawPath(sf::RenderWindow& window, map<int, Node>& nodes, vector<int>& path) {
    for (int i = 0; i + 1 < path.size(); i++) {
        int u = path[i];
        int v = path[i + 1];

        sf::Vertex line[] = {
            sf::Vertex(nodes[u].shape.getPosition(), sf::Color::Blue),
            sf::Vertex(nodes[v].shape.getPosition(), sf::Color::Blue)
        };

        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void drawNodes(sf::RenderWindow& window, map<int, Node>& nodes, map<int, vector<pair<int, int>>>& graph, sf::Font& font) {
    for (auto& [id, node] : nodes) {
        if (!graph[id].empty())
            node.shape.setFillColor(sf::Color::Green);
        else
            node.shape.setFillColor(sf::Color::Red);
    }

    for (auto& [id, node] : nodes)
        window.draw(node.shape);

    for (auto& [id, node] : nodes) {
        sf::Text txt(font);
        txt.setString(to_string(id));
        txt.setCharacterSize(18);
        txt.setFillColor(sf::Color::Black);

        sf::FloatRect bounds = txt.getLocalBounds();
        txt.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });

        txt.setPosition({
            node.shape.getPosition().x,
            node.shape.getPosition().y - 5.f
            });

        window.draw(txt);
    }
}