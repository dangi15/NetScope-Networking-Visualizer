#pragma once
#include <SFML/Graphics.hpp>
#include <string>

static constexpr float NODE_RADIUS = 22.f;

struct Node {
    int id;
    std::string name;   // e.g. "R0", "R1" — shown on the router circle
    sf::CircleShape shape;
    bool down = false;  // true = router is offline (greyed out, excluded from routing)
};