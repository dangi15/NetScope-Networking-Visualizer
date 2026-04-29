#pragma once
#include <vector>
#include <SFML/System/Vector2.hpp>

// Represents a single packet travelling along a computed route.
// The animation is driven by a normalised parameter t in [0,1] that
// moves from the start of the current edge to its end each step.
struct PacketAnim {
    std::vector<int> path;   // router IDs in order
    int   edge = 0;       // index of the edge currently being traversed (path[edge] -> path[edge+1])
    float t = 0.f;     // interpolation along the current edge [0,1]
    float speed = 1.5f;    // edges per second
    bool  active = false;
    bool  paused = false;
    bool  loop = true;    // restart from src when packet reaches dst

    // Circular trail of recent positions for the motion-blur tail
    static constexpr int TRAIL_LEN = 12;
    sf::Vector2f trail[TRAIL_LEN] = {};
    int trailHead = 0;

    void start(const std::vector<int>& p, float spd = 1.5f) {
        path = p;
        edge = 0;
        t = 0.f;
        speed = spd;
        active = true;
        paused = false;
        trailHead = 0;
        for (auto& v : trail) v = {};
    }

    void stop() { active = false; path.clear(); }

    // Push a new world position into the trail ring-buffer
    void pushTrail(sf::Vector2f pos) {
        trail[trailHead] = pos;
        trailHead = (trailHead + 1) % TRAIL_LEN;
    }
};