#pragma once

// Properties of a network link between two routers.
// Latency (ms) is used as the routing cost (lower = preferred).
// Bandwidth (Mbps) is displayed on the link but does not affect routing cost
// (in a real OSPF implementation it would; this keeps the project approachable).
struct LinkProps {
    int latency;    // ms  — used as edge weight in all algorithms
    int bandwidth;  // Mbps — cosmetic / informational
};