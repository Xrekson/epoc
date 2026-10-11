#pragma once
#include "raylib.h"
#include "raymath.h"
#include "weapons.hpp"
#include "math_springs.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <limits>

// ============================================================================
// PHANTOM FORCES 3 ICONIC MAPS: DESERT STORM, METRO, WAREHOUSE
// Each block has a MaterialType that scales wallbang penetration thickness!
// ============================================================================

enum MapId {
    MAP_DESERT_STORM = 0,
    MAP_METRO,
    MAP_WAREHOUSE,
    MAP_FOUNDRY,
    MAP_DOCKSIDE_CUTOFF,
    MAP_HIGHRISE_ATLAS,
    MAP_SALT_FLATS,
    MAP_MONASTERY_RIDGE,
    MAP_COUNT
};

enum GameModeId {
    MODE_TDM = 0,
    MODE_FLARE_DOMINATION,
    MODE_KING_OF_THE_HILL,
    MODE_COUNT
};

enum MaterialType {
    MAT_CONCRETE = 0,    // High resistance (1.5x thickness cost)
    MAT_SANDSTONE,       // Medium resistance (1.0x thickness cost)
    MAT_WOOD_CRATE,      // Low resistance (0.45x thickness cost - easy wallbang!)
    MAT_SHEET_METAL,     // Thin metal (0.55x thickness cost - Warehouse walls!)
    MAT_GLASS            // Minimal resistance (0.15x thickness cost)
};

inline float GetMaterialPenetrationCost(MaterialType mat) {
    switch (mat) {
        case MAT_CONCRETE:    return 1.45f;
        case MAT_SANDSTONE:   return 0.95f;
        case MAT_WOOD_CRATE:  return 0.42f;
        case MAT_SHEET_METAL: return 0.50f;
        case MAT_GLASS:       return 0.12f;
        default:              return 1.0f;
    }
}

struct MapBlock {
    Vector3 pos;
    Vector3 size;
    Color color;
    MaterialType material;
    bool collidable = true;

    BoundingBox GetBounds() const {
        Vector3 half = Vector3Scale(size, 0.5f);
        return {
            Vector3Subtract(pos, half),
            Vector3Add(pos, half)
        };
    }
};

struct ObjectivePoint {
    const char* label; // "A", "B", "C", or "HILL"
    Vector3 pos;
    float radius = 4.5f;
    int ownerTeam = -1;     // -1 = Neutral, 0 = Phantoms (Blue), 1 = Ghosts (Orange)
    float captureProgress = 0.0f; // -1.0 (Phantoms) to +1.0 (Ghosts)
};

struct GameMap {
    MapId id;
    const char* name;
    const char* subtitle;
    Color skyColor;
    Color groundColor;
    std::vector<MapBlock> blocks;
    std::vector<Vector3> phantomSpawns;
    std::vector<Vector3> ghostSpawns;
    std::vector<Vector3> waypoints;
    std::vector<std::vector<int>> navNeighbors;
    std::vector<ObjectivePoint> flarePoints;
    ObjectivePoint hillPoint;

    void AddBox(Vector3 pos, Vector3 size, Color col, MaterialType mat = MAT_CONCRETE, bool solid = true) {
        blocks.push_back({ pos, size, col, mat, solid });
    }

    // Helper: Build a hollow building with doorway, windows (vaultable & shootable!), and optional 2nd floor + roof stairs
    void AddBuilding(Vector3 center, float w, float d, float h, Color wallCol, MaterialType wallMat, bool twoStory) {
        float t = 0.45f; // Wall thickness (0.45m -> very wallbangable in PF!)
        float doorW = 2.6f;
        float doorH = 2.6f;

        // Floor slab
        AddBox({ center.x, center.y + 0.1f, center.z }, { w, 0.2f, d }, { 95, 92, 86, 255 }, MAT_CONCRETE);

        // Front wall (-Z) with central doorway & 2 side windows
        float sideSegW = (w - doorW) * 0.5f;
        // Left & right of door
        AddBox({ center.x - (doorW * 0.5f + sideSegW * 0.5f), center.y + 0.5f, center.z - d * 0.5f },
               { sideSegW, 1.0f, t }, wallCol, wallMat); // Waist-high window sill (vaultable!)
        AddBox({ center.x + (doorW * 0.5f + sideSegW * 0.5f), center.y + 0.5f, center.z - d * 0.5f },
               { sideSegW, 1.0f, t }, wallCol, wallMat);
        // Header above door & windows
        AddBox({ center.x, center.y + doorH + (h - doorH) * 0.5f, center.z - d * 0.5f },
               { w, h - doorH, t }, wallCol, wallMat);
        // Corner pillars
        AddBox({ center.x - w * 0.5f + 0.6f, center.y + h * 0.5f, center.z - d * 0.5f }, { 1.2f, h, t }, wallCol, wallMat);
        AddBox({ center.x + w * 0.5f - 0.6f, center.y + h * 0.5f, center.z - d * 0.5f }, { 1.2f, h, t }, wallCol, wallMat);

        // Back wall (+Z) with doorway & sniper windows
        AddBox({ center.x - (doorW * 0.5f + sideSegW * 0.5f), center.y + 0.5f, center.z + d * 0.5f },
               { sideSegW, 1.0f, t }, wallCol, wallMat);
        AddBox({ center.x + (doorW * 0.5f + sideSegW * 0.5f), center.y + 0.5f, center.z + d * 0.5f },
               { sideSegW, 1.0f, t }, wallCol, wallMat);
        AddBox({ center.x, center.y + doorH + (h - doorH) * 0.5f, center.z + d * 0.5f },
               { w, h - doorH, t }, wallCol, wallMat);
        AddBox({ center.x - w * 0.5f + 0.6f, center.y + h * 0.5f, center.z + d * 0.5f }, { 1.2f, h, t }, wallCol, wallMat);
        AddBox({ center.x + w * 0.5f - 0.6f, center.y + h * 0.5f, center.z + d * 0.5f }, { 1.2f, h, t }, wallCol, wallMat);

        // Left (-X) & Right (+X) walls with wide windows
        AddBox({ center.x - w * 0.5f, center.y + 0.55f, center.z }, { t, 1.1f, d }, wallCol, wallMat);
        AddBox({ center.x - w * 0.5f, center.y + h - 0.5f, center.z }, { t, 1.0f, d }, wallCol, wallMat);
        AddBox({ center.x - w * 0.5f, center.y + h * 0.5f, center.z - d * 0.38f }, { t, h, d * 0.24f }, wallCol, wallMat);
        AddBox({ center.x - w * 0.5f, center.y + h * 0.5f, center.z + d * 0.38f }, { t, h, d * 0.24f }, wallCol, wallMat);

        AddBox({ center.x + w * 0.5f, center.y + 0.55f, center.z }, { t, 1.1f, d }, wallCol, wallMat);
        AddBox({ center.x + w * 0.5f, center.y + h - 0.5f, center.z }, { t, 1.0f, d }, wallCol, wallMat);
        AddBox({ center.x + w * 0.5f, center.y + h * 0.5f, center.z - d * 0.38f }, { t, h, d * 0.24f }, wallCol, wallMat);
        AddBox({ center.x + w * 0.5f, center.y + h * 0.5f, center.z + d * 0.38f }, { t, h, d * 0.24f }, wallCol, wallMat);

        // Roof slab (with stair opening if twoStory)
        AddBox({ center.x - 1.2f, center.y + h, center.z }, { w - 2.4f, 0.3f, d }, { 115, 110, 100, 255 }, MAT_CONCRETE);

        // Register doorway & interior navigation waypoints for AI pathfinding
        waypoints.push_back({ center.x, center.y + 0.25f, center.z - d * 0.5f - 1.8f }); // Front door approach
        waypoints.push_back({ center.x, center.y + 0.25f, center.z + d * 0.5f + 1.8f }); // Back door approach
        waypoints.push_back({ center.x - 1.2f, center.y + 0.25f, center.z });            // Interior room center

        if (twoStory) {
            // Stepped ramp stairs leading to the roof!
            int steps = 10;
            for (int i = 0; i < steps; ++i) {
                float frac = (float)i / (float)steps;
                float sy = center.y + (i + 1) * (h / steps);
                float sz = center.z - d * 0.35f + frac * (d * 0.70f);
                AddBox({ center.x + w * 0.5f - 1.3f, sy * 0.5f, sz },
                       { 2.0f, sy, (d * 0.75f) / steps + 0.1f }, { 130, 118, 98, 255 }, MAT_SANDSTONE);
            }
            // Rooftop parapet cover walls (1.05m high -> crouch cover & vaultable!)
            Color roofCol = wallCol;
            AddBox({ center.x, center.y + h + 0.55f, center.z - d * 0.5f }, { w, 1.0f, 0.35f }, roofCol, wallMat);
            AddBox({ center.x, center.y + h + 0.55f, center.z + d * 0.5f }, { w, 1.0f, 0.35f }, roofCol, wallMat);
            AddBox({ center.x - w * 0.5f, center.y + h + 0.55f, center.z }, { 0.35f, 1.0f, d }, roofCol, wallMat);

            // Stair bottom, stair top & rooftop sniper vantage waypoints
            waypoints.push_back({ center.x + w * 0.5f - 1.3f, center.y + 0.25f, center.z - d * 0.38f });
            waypoints.push_back({ center.x + w * 0.5f - 1.5f, center.y + h + 0.25f, center.z + d * 0.32f });
            waypoints.push_back({ center.x - 1.8f, center.y + h + 0.25f, center.z });
        }
    }

    // Helper: Build a stepped ramp/staircase between startFoot and endFoot and register AI waypoints
    void AddRampStairs(Vector3 startFoot, Vector3 endFoot, float width, int steps, Color col, MaterialType mat = MAT_CONCRETE) {
        bool alongX = std::fabs(endFoot.x - startFoot.x) >= std::fabs(endFoot.z - startFoot.z);
        float span = alongX ? std::fabs(endFoot.x - startFoot.x) : std::fabs(endFoot.z - startFoot.z);
        float stepSpan = span / (float)std::max(1, steps) + 0.14f;
        for (int i = 0; i < steps; ++i) {
            float tMid = (i + 0.5f) / (float)steps;
            float tTop = (float)(i + 1) / (float)steps;
            float topY = LerpFloat(startFoot.y, endFoot.y, tTop);
            float botY = startFoot.y;
            float h = std::max(0.2f, topY - botY);
            Vector3 p = {
                LerpFloat(startFoot.x, endFoot.x, tMid),
                botY + h * 0.5f,
                LerpFloat(startFoot.z, endFoot.z, tMid)
            };
            Vector3 sz = alongX ? Vector3{ stepSpan, h, width } : Vector3{ width, h, stepSpan };
            AddBox(p, sz, col, mat);
        }
        waypoints.push_back({ startFoot.x, startFoot.y + 0.25f, startFoot.z });
        waypoints.push_back({ endFoot.x,   endFoot.y + 0.25f,   endFoot.z });
    }

    // Helper: Build an elevated skybridge / catwalk with safety railings and waypoints
    void AddCatwalk(Vector3 start, Vector3 end, float width, Color floorCol, Color railCol, MaterialType floorMat = MAT_CONCRETE) {
        Vector3 diff = Vector3Subtract(end, start);
        float len = Vector3Length(diff);
        Vector3 mid = Vector3Lerp(start, end, 0.5f);
        bool alongX = std::fabs(diff.x) >= std::fabs(diff.z);
        Vector3 size = alongX ? Vector3{ len, 0.28f, width } : Vector3{ width, 0.28f, len };
        AddBox(mid, size, floorCol, floorMat);

        float railH = 1.05f;
        if (alongX) {
            AddBox({ mid.x, mid.y + railH * 0.5f + 0.14f, mid.z - width * 0.5f }, { len, railH, 0.20f }, railCol, MAT_SHEET_METAL);
            AddBox({ mid.x, mid.y + railH * 0.5f + 0.14f, mid.z + width * 0.5f }, { len, railH, 0.20f }, railCol, MAT_SHEET_METAL);
        } else {
            AddBox({ mid.x - width * 0.5f, mid.y + railH * 0.5f + 0.14f, mid.z }, { 0.20f, railH, len }, railCol, MAT_SHEET_METAL);
            AddBox({ mid.x + width * 0.5f, mid.y + railH * 0.5f + 0.14f, mid.z }, { 0.20f, railH, len }, railCol, MAT_SHEET_METAL);
        }
        waypoints.push_back({ start.x, start.y + 0.25f, start.z });
        waypoints.push_back({ mid.x,   mid.y   + 0.25f, mid.z   });
        waypoints.push_back({ end.x,   end.y   + 0.25f, end.z   });
    }

    // Helper: Build a hollow underground/indoor tunnel corridor with floor, side walls, and ceiling slab
    void AddTunnelPassage(Vector3 start, Vector3 end, float width, float height, Color wallCol, MaterialType mat = MAT_CONCRETE) {
        Vector3 diff = Vector3Subtract(end, start);
        float len = Vector3Length(diff);
        Vector3 mid = Vector3Lerp(start, end, 0.5f);
        bool alongX = std::fabs(diff.x) >= std::fabs(diff.z);
        float t = 0.40f;

        Vector3 floorSz = alongX ? Vector3{ len, 0.30f, width + t * 2.0f } : Vector3{ width + t * 2.0f, 0.30f, len };
        AddBox({ mid.x, mid.y - 0.15f, mid.z }, floorSz, wallCol, mat);
        Vector3 ceilSz = floorSz;
        AddBox({ mid.x, mid.y + height + 0.15f, mid.z }, ceilSz, wallCol, mat);

        if (alongX) {
            AddBox({ mid.x, mid.y + height * 0.5f, mid.z - (width * 0.5f + t * 0.5f) }, { len, height, t }, wallCol, mat);
            AddBox({ mid.x, mid.y + height * 0.5f, mid.z + (width * 0.5f + t * 0.5f) }, { len, height, t }, wallCol, mat);
        } else {
            AddBox({ mid.x - (width * 0.5f + t * 0.5f), mid.y + height * 0.5f, mid.z }, { t, height, len }, wallCol, mat);
            AddBox({ mid.x + (width * 0.5f + t * 0.5f), mid.y + height * 0.5f, mid.z }, { t, height, len }, wallCol, mat);
        }

        waypoints.push_back({ start.x, start.y + 0.25f, start.z });
        waypoints.push_back({ mid.x,   mid.y   + 0.25f, mid.z   });
        waypoints.push_back({ end.x,   end.y   + 0.25f, end.z   });
    }

    // Helper: Add tactical cover cluster (crates, barriers, sandbags)
    void AddCoverCluster(Vector3 center, Color colA, Color colB, MaterialType matA = MAT_SANDSTONE, MaterialType matB = MAT_WOOD_CRATE) {
        AddBox({ center.x - 1.2f, center.y + 0.75f, center.z }, { 1.6f, 1.5f, 2.8f }, colA, matA);
        AddBox({ center.x + 1.2f, center.y + 0.60f, center.z - 0.4f }, { 1.5f, 1.2f, 1.5f }, colB, matB);
        AddBox({ center.x + 1.2f, center.y + 0.50f, center.z + 1.2f }, { 1.4f, 1.0f, 1.4f }, colB, matB);
    }

    // Check if an elevated point has a floor slab underneath it (prevents walking off roofs)
    bool HasFloorSupportBelow(Vector3 pt) const {
        if (pt.y <= 0.6f) return true;
        Vector3 orig = { pt.x, pt.y + 0.4f, pt.z };
        Vector3 down = { 0.0f, -1.0f, 0.0f };
        for (const auto& blk : blocks) {
            if (!blk.collidable) continue;
            RayBoxHit hit = IntersectRayAABB(orig, down, blk.GetBounds());
            if (hit.hit && hit.tNear >= 0.0f && hit.tNear <= 1.8f) {
                return true;
            }
        }
        return false;
    }

    // Check if an AI bot can walk/vault/climb directly between two 3D points without hitting a solid wall
    bool CanTraverseSegment(Vector3 a, Vector3 b) const {
        Vector3 diff = Vector3Subtract(b, a);
        float dist = Vector3Length(diff);
        if (dist < 0.1f) return true;

        // Upper-chest ray (above waist-high vaultable cover, below door headers)
        Vector3 origHigh = { a.x, a.y + 1.35f, a.z };
        Vector3 destHigh = { b.x, b.y + 1.35f, b.z };
        Vector3 highDiff = Vector3Subtract(destHigh, origHigh);
        float highLen = Vector3Length(highDiff);
        Vector3 highDir = Vector3Scale(highDiff, 1.0f / std::max(0.01f, highLen));

        float minFootY = std::min(a.y, b.y);
        float maxFootY = std::max(a.y, b.y);

        for (const auto& blk : blocks) {
            if (!blk.collidable || blk.material == MAT_GLASS) continue;
            BoundingBox box = blk.GetBounds();
            // Ignore floor slabs and low vaultable/step blocks below the path's upper foot + 1.15m
            if (box.max.y <= maxFootY + 1.12f && (maxFootY - minFootY) > 0.8f) {
                continue; // Ramp/stair step block
            }
            if (box.max.y <= minFootY + 1.15f) {
                continue; // Low cover / window sill (bots can step or vault over it)
            }
            RayBoxHit hit = IntersectRayAABB(origHigh, highDir, box);
            if (hit.hit && hit.tNear >= 0.0f && hit.tNear < highLen) {
                return false;
            }
        }

        // If moving across an elevated level (y > 2.0m), verify the midpoint has floor support
        if (a.y > 2.0f && b.y > 2.0f) {
            Vector3 mid = Vector3Lerp(a, b, 0.5f);
            if (!HasFloorSupportBelow(mid)) return false;
        }
        return true;
    }

    // Build connected waypoint navigation graph for A* pathfinding
    void BuildNavGraph() {
        int n = (int)waypoints.size();
        navNeighbors.assign(n, {});
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                Vector3 a = waypoints[i];
                Vector3 b = waypoints[j];
                float horizDist = Vector2Distance({ a.x, a.z }, { b.x, b.z });
                float dy = std::fabs(a.y - b.y);

                bool validEdge = false;
                if (dy <= 1.1f && horizDist <= 30.0f) {
                    validEdge = CanTraverseSegment(a, b);
                } else if (dy > 1.1f && dy <= 4.2f && horizDist <= 18.5f) {
                    // Staircase or Metro escalator link
                    validEdge = CanTraverseSegment(a, b);
                }

                if (validEdge) {
                    navNeighbors[i].push_back(j);
                    navNeighbors[j].push_back(i);
                }
            }
        }

        // Ensure every node has at least 2 connections so no bot ever gets stranded
        for (int i = 0; i < n; ++i) {
            while ((int)navNeighbors[i].size() < 2 && n > 2) {
                int bestJ = -1;
                float bestScore = 1e9f;
                for (int j = 0; j < n; ++j) {
                    if (i == j) continue;
                    bool already = false;
                    for (int nb : navNeighbors[i]) if (nb == j) already = true;
                    if (already) continue;
                    float d = Vector3Distance(waypoints[i], waypoints[j]) + std::fabs(waypoints[i].y - waypoints[j].y) * 2.5f;
                    if (d < bestScore) {
                        bestScore = d;
                        bestJ = j;
                    }
                }
                if (bestJ >= 0) {
                    navNeighbors[i].push_back(bestJ);
                    navNeighbors[bestJ].push_back(i);
                } else {
                    break;
                }
            }
        }
    }

    // Find the best navigation waypoint for any 3D world position
    int FindClosestWaypoint(Vector3 pos) const {
        if (waypoints.empty()) return 0;
        int bestIdx = 0;
        float bestScore = 1e9f;
        for (int i = 0; i < (int)waypoints.size(); ++i) {
            const Vector3& wp = waypoints[i];
            float horiz = Vector2Distance({ pos.x, pos.z }, { wp.x, wp.z });
            float dy = std::fabs(pos.y - wp.y);
            float score = horiz + dy * 4.5f;
            if (horiz < 24.0f && dy < 1.5f && !CanTraverseSegment(pos, wp)) {
                score += 18.0f; // Penalize waypoints on the other side of a solid wall
            }
            if (score < bestScore) {
                bestScore = score;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    // A* / Dijkstra shortest path query: returns the next immediate waypoint index from startNode toward goalNode
    int FindNextWaypointOnPath(int startNode, int goalNode) const {
        int n = (int)waypoints.size();
        if (n == 0) return 0;
        startNode = std::clamp(startNode, 0, n - 1);
        goalNode  = std::clamp(goalNode,  0, n - 1);
        if (startNode == goalNode) return goalNode;

        std::vector<float> gScore(n, std::numeric_limits<float>::infinity());
        std::vector<float> fScore(n, std::numeric_limits<float>::infinity());
        std::vector<int> cameFrom(n, -1);
        std::vector<bool> closed(n, false);

        gScore[startNode] = 0.0f;
        fScore[startNode] = Vector3Distance(waypoints[startNode], waypoints[goalNode]);

        for (int iter = 0; iter < n; ++iter) {
            int current = -1;
            float bestF = std::numeric_limits<float>::infinity();
            for (int i = 0; i < n; ++i) {
                if (!closed[i] && fScore[i] < bestF) {
                    bestF = fScore[i];
                    current = i;
                }
            }
            if (current < 0 || current == goalNode) break;
            closed[current] = true;

            for (int neighbor : navNeighbors[current]) {
                if (closed[neighbor]) continue;
                float stepCost = Vector3Distance(waypoints[current], waypoints[neighbor]);
                float tentativeG = gScore[current] + stepCost;
                if (tentativeG < gScore[neighbor]) {
                    cameFrom[neighbor] = current;
                    gScore[neighbor] = tentativeG;
                    fScore[neighbor] = tentativeG + Vector3Distance(waypoints[neighbor], waypoints[goalNode]);
                }
            }
        }

        if (cameFrom[goalNode] < 0) {
            // Fallback to neighbor of startNode closest to goalNode
            int bestNb = goalNode;
            float bestD = 1e9f;
            for (int nb : navNeighbors[startNode]) {
                float d = Vector3Distance(waypoints[nb], waypoints[goalNode]);
                if (d < bestD) {
                    bestD = d;
                    bestNb = nb;
                }
            }
            return bestNb;
        }

        int step = goalNode;
        while (cameFrom[step] >= 0 && cameFrom[step] != startNode) {
            step = cameFrom[step];
        }
        return step;
    }
};

inline GameMap BuildMap(MapId id) {
    GameMap m;
    m.id = id;

    if (id == MAP_DESERT_STORM) {
        m.name = "DESERT STORM";
        m.subtitle = "Expanded 160m Sandstone City - Subterranean Tunnels, Bazaar, Fuel Depot & Rooftop Planks";
        m.skyColor = { 188, 216, 242, 255 };
        m.groundColor = { 216, 192, 148, 255 };

        Color sandWall   = { 210, 182, 135, 255 };
        Color sandBrick  = { 194, 162, 116, 255 };
        Color woodCol    = { 156, 114, 68, 255 };
        Color darkSteel  = { 58, 62, 68, 255 };
        Color fuelTankCol = { 178, 182, 190, 255 };

        // 1. Massive 160m x 160m Outer Boundary Walls & Corner Bastion Towers
        float arenaW = 160.0f;
        float arenaD = 160.0f;
        m.AddBox({ 0, 4.5f, -arenaD * 0.5f }, { arenaW, 9.0f, 1.8f }, sandBrick, MAT_CONCRETE);
        m.AddBox({ 0, 4.5f,  arenaD * 0.5f }, { arenaW, 9.0f, 1.8f }, sandBrick, MAT_CONCRETE);
        m.AddBox({ -arenaW * 0.5f, 4.5f, 0 }, { 1.8f, 9.0f, arenaD }, sandBrick, MAT_CONCRETE);
        m.AddBox({  arenaW * 0.5f, 4.5f, 0 }, { 1.8f, 9.0f, arenaD }, sandBrick, MAT_CONCRETE);

        // 4 Corner Perimeter Fort Bastions
        for (int cx = -1; cx <= 1; cx += 2) {
            for (int cz = -1; cz <= 1; cz += 2) {
                m.AddBox({ cx * 74.0f, 4.0f, cz * 74.0f }, { 8.0f, 8.0f, 8.0f }, sandBrick, MAT_CONCRETE);
                m.AddBox({ cx * 74.0f, 8.5f, cz * 74.0f }, { 8.6f, 1.1f, 8.6f }, sandWall, MAT_SANDSTONE);
            }
        }

        // 2. Central Plaza & Upgraded 3-Tier Oil Derrick (King of the Hill)
        m.AddBox({ 0.0f, 0.45f, 0.0f }, { 12.0f, 0.9f, 12.0f }, { 120, 118, 112, 255 }, MAT_CONCRETE);
        // Steel cross-strut derrick legs
        m.AddBox({ -4.8f, 4.5f, -4.8f }, { 0.8f, 9.0f, 0.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({  4.8f, 4.5f, -4.8f }, { 0.8f, 9.0f, 0.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({ -4.8f, 4.5f,  4.8f }, { 0.8f, 9.0f, 0.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({  4.8f, 4.5f,  4.8f }, { 0.8f, 9.0f, 0.8f }, darkSteel, MAT_CONCRETE);
        // Mid-deck (y = 4.2m) + Upper Crow's Nest (y = 8.6m)
        m.AddBox({ 0.0f, 4.2f, 0.0f }, { 10.5f, 0.45f, 10.5f }, darkSteel, MAT_SHEET_METAL);
        m.AddBox({ 0.0f, 4.8f, -5.1f }, { 10.5f, 0.95f, 0.25f }, darkSteel, MAT_SHEET_METAL);
        m.AddBox({ 0.0f, 4.8f,  5.1f }, { 10.5f, 0.95f, 0.25f }, darkSteel, MAT_SHEET_METAL);
        m.AddBox({ 0.0f, 8.6f, 0.0f }, { 6.8f, 0.45f, 6.8f }, darkSteel, MAT_SHEET_METAL);
        // Dual access stairs to Mid-deck
        m.AddRampStairs({ -10.0f, 0.0f, 0.0f }, { -5.2f, 4.2f, 0.0f }, 2.4f, 10, darkSteel, MAT_SHEET_METAL);
        m.AddRampStairs({  10.0f, 0.0f, 0.0f }, {  5.2f, 4.2f, 0.0f }, 2.4f, 10, darkSteel, MAT_SHEET_METAL);
        // Plaza defense sandbag berms & cover crates
        m.AddCoverCluster({ -7.2f, 0.0f, -7.2f }, sandBrick, woodCol);
        m.AddCoverCluster({  7.2f, 0.0f, -7.2f }, sandBrick, woodCol);
        m.AddCoverCluster({ -7.2f, 0.0f,  7.2f }, sandBrick, woodCol);
        m.AddCoverCluster({  7.2f, 0.0f,  7.2f }, sandBrick, woodCol);

        // 3. Covered Bazaar Arcade Tunnels & Rooftop Promenade (Ground y = 0.0m, Roof Promenade y = 3.3m)
        // North-South arcade passage + East-West cross passage
        m.AddTunnelPassage({ 0.0f, 0.0f, -36.0f }, { 0.0f, 0.0f, 36.0f }, 3.8f, 3.0f, sandBrick, MAT_SANDSTONE);
        m.AddTunnelPassage({ -36.0f, 0.0f, 0.0f }, { 36.0f, 0.0f, 0.0f }, 3.8f, 3.0f, sandBrick, MAT_SANDSTONE);
        // 4 Stairways ascending from city streets up to the arcade rooftop promenade
        m.AddRampStairs({ 0.0f, 0.0f, -46.0f }, { 0.0f, 3.3f, -36.0f }, 3.2f, 9, sandBrick, MAT_SANDSTONE);
        m.AddRampStairs({ 0.0f, 0.0f,  46.0f }, { 0.0f, 3.3f,  36.0f }, 3.2f, 9, sandBrick, MAT_SANDSTONE);
        m.AddRampStairs({ -46.0f, 0.0f, 0.0f }, { -36.0f, 3.3f, 0.0f }, 3.2f, 9, sandBrick, MAT_SANDSTONE);
        m.AddRampStairs({  46.0f, 0.0f, 0.0f }, {  36.0f, 3.3f, 0.0f }, 3.2f, 9, sandBrick, MAT_SANDSTONE);
        // Center stair linking Promenade directly to Derrick Platform
        m.AddRampStairs({ 0.0f, 3.3f, 0.0f }, { 0.0f, 4.45f, 0.0f }, 1.8f, 4, darkSteel, MAT_SHEET_METAL);

        // 4. Western Souk / Bazaar District (Flag A at x = -52m)
        m.AddBuilding({ -52.0f, 0.0f, 0.0f }, 18.0f, 16.0f, 4.0f, sandWall, MAT_SANDSTONE, true);
        // Merchant stalls, canopy awnings, and spice crate mazes
        for (int bz = -22; bz <= 22; bz += 11) {
            m.AddBox({ -40.0f, 1.1f, (float)bz }, { 3.2f, 2.2f, 1.4f }, woodCol, MAT_WOOD_CRATE);
            m.AddBox({ -64.0f, 1.1f, (float)bz }, { 3.2f, 2.2f, 1.4f }, woodCol, MAT_WOOD_CRATE);
            m.AddBox({ -52.0f, 2.4f, (float)bz }, { 5.2f, 0.2f, 4.8f }, sandWall, MAT_SHEET_METAL); // Fabric canopy
        }
        m.AddCoverCluster({ -52.0f, 0.0f, -14.0f }, sandBrick, woodCol);
        m.AddCoverCluster({ -52.0f, 0.0f,  14.0f }, sandBrick, woodCol);

        // 5. Eastern Fuel Depot & Industrial Tank Farm (Flag C at x = +52m)
        m.AddBuilding({ 52.0f, 0.0f, 0.0f }, 18.0f, 16.0f, 4.0f, darkSteel, MAT_CONCRETE, true);
        // 4 Large Cylindrical Storage Tanks with Elevated Inspection Catwalks
        m.AddBox({ 42.0f, 2.8f, -16.0f }, { 6.8f, 5.6f, 6.8f }, fuelTankCol, MAT_CONCRETE);
        m.AddBox({ 62.0f, 2.8f, -16.0f }, { 6.8f, 5.6f, 6.8f }, fuelTankCol, MAT_CONCRETE);
        m.AddBox({ 42.0f, 2.8f,  16.0f }, { 6.8f, 5.6f, 6.8f }, fuelTankCol, MAT_CONCRETE);
        m.AddBox({ 62.0f, 2.8f,  16.0f }, { 6.8f, 5.6f, 6.8f }, fuelTankCol, MAT_CONCRETE);
        // Catwalk linking North & South tanks at y = 4.4m
        m.AddCatwalk({ 52.0f, 4.4f, -16.0f }, { 52.0f, 4.4f, 16.0f }, 2.2f, darkSteel, darkSteel, MAT_SHEET_METAL);
        m.AddRampStairs({ 52.0f, 0.0f, -24.0f }, { 52.0f, 4.4f, -16.0f }, 2.0f, 10, darkSteel, MAT_SHEET_METAL);

        // 6. 4 Two-Story Corner Sandstone Compounds + Inter-Roof Bridges
        m.AddBuilding({ -46.0f, 0.0f, -46.0f }, 18.0f, 18.0f, 4.2f, sandWall, MAT_SANDSTONE, true);
        m.AddBuilding({  46.0f, 0.0f, -46.0f }, 18.0f, 18.0f, 4.2f, sandWall, MAT_SANDSTONE, true);
        m.AddBuilding({ -46.0f, 0.0f,  46.0f }, 18.0f, 18.0f, 4.2f, sandWall, MAT_SANDSTONE, true);
        m.AddBuilding({  46.0f, 0.0f,  46.0f }, 18.0f, 18.0f, 4.2f, sandWall, MAT_SANDSTONE, true);

        // 4 Intermediate Residential Townhouses (Mid-block CQB flanking)
        m.AddBuilding({ -22.0f, 0.0f, -24.0f }, 14.0f, 14.0f, 3.8f, sandBrick, MAT_SANDSTONE, false);
        m.AddBuilding({  22.0f, 0.0f, -24.0f }, 14.0f, 14.0f, 3.8f, sandBrick, MAT_SANDSTONE, false);
        m.AddBuilding({ -22.0f, 0.0f,  24.0f }, 14.0f, 14.0f, 3.8f, sandBrick, MAT_SANDSTONE, false);
        m.AddBuilding({  22.0f, 0.0f,  24.0f }, 14.0f, 14.0f, 3.8f, sandBrick, MAT_SANDSTONE, false);

        // Rooftop Catwalk Planks across North & South Alleyways
        m.AddCatwalk({ -37.0f, 4.2f, -46.0f }, { -29.0f, 3.8f, -24.0f }, 1.8f, woodCol, woodCol, MAT_WOOD_CRATE);
        m.AddCatwalk({  37.0f, 4.2f, -46.0f }, {  29.0f, 3.8f, -24.0f }, 1.8f, woodCol, woodCol, MAT_WOOD_CRATE);
        m.AddCatwalk({ -37.0f, 4.2f,  46.0f }, { -29.0f, 3.8f,  24.0f }, 1.8f, woodCol, woodCol, MAT_WOOD_CRATE);
        m.AddCatwalk({  37.0f, 4.2f,  46.0f }, {  29.0f, 3.8f,  24.0f }, 1.8f, woodCol, woodCol, MAT_WOOD_CRATE);

        // Street barricades & sandbags
        for (int side = -1; side <= 1; side += 2) {
            m.AddCoverCluster({ -22.0f, 0.0f, side * 44.0f }, sandBrick, woodCol);
            m.AddCoverCluster({  22.0f, 0.0f, side * 44.0f }, sandBrick, woodCol);
            m.AddCoverCluster({ side * 44.0f, 0.0f, -22.0f }, sandBrick, woodCol);
            m.AddCoverCluster({ side * 44.0f, 0.0f,  22.0f }, sandBrick, woodCol);
        }

        m.phantomSpawns = { { -58, 1, -68 }, { -24, 1, -70 }, { 24, 1, -70 }, { 58, 1, -68 } };
        m.ghostSpawns   = { { -58, 1,  68 }, { -24, 1,  70 }, { 24, 1,  70 }, { 58, 1,  68 } };
        m.waypoints.insert(m.waypoints.end(), {
            {   0.0f, 0.85f,   0.0f }, // Central Oil Derrick Base
            {   0.0f, 4.45f,   0.0f }, // Derrick Mid-Deck
            {   0.0f, 0.25f,   0.0f }, // Covered Arcade Crossroad Ground
            {   0.0f, 0.25f, -24.0f }, {   0.0f, 0.25f,  24.0f },
            { -24.0f, 0.25f,   0.0f }, {  24.0f, 0.25f,   0.0f },
            {   0.0f, 3.55f, -24.0f }, {   0.0f, 3.55f,  24.0f }, // Arcade Rooftop Promenade
            { -24.0f, 3.55f,   0.0f }, {  24.0f, 3.55f,   0.0f },
            // Plaza & Streets
            { -16.0f, 0.25f, -16.0f }, {  16.0f, 0.25f, -16.0f },
            { -16.0f, 0.25f,  16.0f }, {  16.0f, 0.25f,  16.0f },
            { -34.0f, 0.25f, -34.0f }, {  34.0f, 0.25f, -34.0f },
            { -34.0f, 0.25f,  34.0f }, {  34.0f, 0.25f,  34.0f },
            // Bazaar (West)
            { -52.0f, 0.25f,   0.0f }, { -52.0f, 0.25f, -20.0f }, { -52.0f, 0.25f, 20.0f },
            { -64.0f, 0.25f,   0.0f },
            // Fuel Depot (East)
            {  52.0f, 0.25f,   0.0f }, {  52.0f, 0.25f, -20.0f }, {  52.0f, 0.25f, 20.0f },
            {  52.0f, 4.65f,   0.0f }, {  64.0f, 0.25f,   0.0f },
            // Spawns & Outer Perimeters
            { -58.0f, 0.25f, -60.0f }, {  58.0f, 0.25f, -60.0f },
            { -58.0f, 0.25f,  60.0f }, {  58.0f, 0.25f,  60.0f },
            {   0.0f, 0.25f, -62.0f }, {   0.0f, 0.25f,  62.0f }
        });
        m.flarePoints = {
            { "A", { -52.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }, // Western Bazaar
            { "B", {   0.0f, 0.8f, 0.0f }, 6.0f, -1, 0.0f }, // Central Oil Derrick Hill
            { "C", {  52.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }  // Eastern Fuel Depot
        };
        m.hillPoint = { "HILL", { 0.0f, 0.8f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_METRO) {
        m.name = "METRO";
        m.subtitle = "Expanded 170m 3-Level Underground Terminal - Sub-Track Tunnels, 6 Trains & Atrium";
        m.skyColor = { 24, 28, 36, 255 };
        m.groundColor = { 56, 60, 68, 255 }; // Lower subway track level (y = 0)

        Color tileWhite = { 205, 212, 220, 255 };
        Color concGrey  = { 112, 118, 128, 255 };
        Color pillarCol = { 76, 84, 98, 255 };
        Color trainRed  = { 175, 48, 48, 255 };
        Color trainBlue = { 48, 95, 175, 255 };
        Color woodCol   = { 152, 112, 68, 255 };
        Color railMetal = { 38, 42, 48, 255 };

        // 1. Massive 110m x 170m Outer Station Vault Walls
        float W = 110.0f;
        float D = 170.0f;
        m.AddBox({ 0, 6.0f, -D * 0.5f }, { W, 12.0f, 1.8f }, concGrey, MAT_CONCRETE);
        m.AddBox({ 0, 6.0f,  D * 0.5f }, { W, 12.0f, 1.8f }, concGrey, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, concGrey, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, concGrey, MAT_CONCRETE);

        // 2. LEVEL 0 & MEZZANINE: Outer Service Utility Tunnels & Central Track Crossover
        // West Service Corridor (x = -48m) & East Service Corridor (x = +48m) spanning z = -56m to +56m
        m.AddTunnelPassage({ -48.0f, 0.0f, -56.0f }, { -48.0f, 0.0f, 56.0f }, 3.4f, 2.8f, concGrey, MAT_CONCRETE);
        m.AddTunnelPassage({  48.0f, 0.0f, -56.0f }, {  48.0f, 0.0f, 56.0f }, 3.4f, 2.8f, concGrey, MAT_CONCRETE);
        // Central Elevated Track Crossover Bridge spanning over tracks at z = 0 (y = 3.6m)
        m.AddCatwalk({ -44.0f, 3.6f, 0.0f }, { 44.0f, 3.6f, 0.0f }, 3.6f, concGrey, pillarCol, MAT_CONCRETE);
        // 4 Stairs connecting Central & Outer Platforms to the Crossover Bridge
        m.AddRampStairs({ -44.0f, 0.15f, -12.0f }, { -44.0f, 3.6f, 0.0f }, 2.4f, 8, concGrey, MAT_CONCRETE);
        m.AddRampStairs({ -44.0f, 0.15f,  12.0f }, { -44.0f, 3.6f, 0.0f }, 2.4f, 8, concGrey, MAT_CONCRETE);
        m.AddRampStairs({  44.0f, 0.15f, -12.0f }, {  44.0f, 3.6f, 0.0f }, 2.4f, 8, concGrey, MAT_CONCRETE);
        m.AddRampStairs({  44.0f, 0.15f,  12.0f }, {  44.0f, 3.6f, 0.0f }, 2.4f, 8, concGrey, MAT_CONCRETE);

        // 3. LEVEL 0: 4-Track Railway Platform Terminus (y = 0.0m)
        // Central Island Platform (x = -11m to +11m) & Outer Platforms (x = +/-44m)
        m.AddBox({ 0.0f, 0.15f, 0.0f }, { 22.0f, 0.30f, 140.0f }, tileWhite, MAT_CONCRETE);
        m.AddBox({ -44.0f, 0.15f, 0.0f }, { 18.0f, 0.30f, 140.0f }, tileWhite, MAT_CONCRETE);
        m.AddBox({  44.0f, 0.15f, 0.0f }, { 18.0f, 0.30f, 140.0f }, tileWhite, MAT_CONCRETE);
        // Steel Rails in track depressions (x = -26m and +26m)
        m.AddBox({ -25.0f, 0.05f, 0.0f }, { 0.2f, 0.1f, 140.0f }, railMetal, MAT_SHEET_METAL);
        m.AddBox({ -27.0f, 0.05f, 0.0f }, { 0.2f, 0.1f, 140.0f }, railMetal, MAT_SHEET_METAL);
        m.AddBox({  25.0f, 0.05f, 0.0f }, { 0.2f, 0.1f, 140.0f }, railMetal, MAT_SHEET_METAL);
        m.AddBox({  27.0f, 0.05f, 0.0f }, { 0.2f, 0.1f, 140.0f }, railMetal, MAT_SHEET_METAL);

        // 6 Full Walk-Through Subway Train Cars (Hollow car bodies with doors & windows)
        for (int sideX = -1; sideX <= 1; sideX += 2) {
            Color tCol = (sideX < 0) ? trainBlue : trainRed;
            float tx = sideX * 26.0f;
            for (int tz = -48; tz <= 48; tz += 48) {
                // Main train car body
                m.AddBox({ tx, 1.45f, (float)tz }, { 3.6f, 2.7f, 18.0f }, tCol, MAT_SHEET_METAL);
                // Hollow interior walkthrough corridor (vaultable door cutouts)
                m.AddBox({ tx, 0.20f, (float)tz }, { 3.2f, 0.2f, 18.0f }, concGrey, MAT_SHEET_METAL);
                // Interior bench seats
                m.AddBox({ tx - 1.2f, 0.55f, (float)tz }, { 0.6f, 0.5f, 12.0f }, woodCol, MAT_WOOD_CRATE);
                m.AddBox({ tx + 1.2f, 0.55f, (float)tz }, { 0.6f, 0.5f, 12.0f }, woodCol, MAT_WOOD_CRATE);
            }
        }

        // Platform Colonnades & Kiosks on Central Platform
        for (int pz = -54; pz <= 54; pz += 18) {
            m.AddBox({ -6.0f, 2.5f, (float)pz }, { 1.4f, 5.0f, 1.4f }, pillarCol, MAT_CONCRETE);
            m.AddBox({  6.0f, 2.5f, (float)pz }, { 1.4f, 5.0f, 1.4f }, pillarCol, MAT_CONCRETE);
            if (std::abs(pz) == 18) {
                m.AddCoverCluster({ 0.0f, 0.0f, (float)pz }, concGrey, woodCol);
            }
        }

        // 4. LEVEL 1: Upper Mezzanine Concourse & Shopping Atrium (y = 3.6m)
        // Central concourse with an opening looking down onto the platform
        m.AddBox({ 0.0f, 3.6f, -28.0f }, { 64.0f, 0.35f, 32.0f }, tileWhite, MAT_CONCRETE); // North Food Court
        m.AddBox({ 0.0f, 3.6f,  28.0f }, { 64.0f, 0.35f, 32.0f }, tileWhite, MAT_CONCRETE); // South Ticket Lobby
        // Bridge walkways spanning across the central atrium
        m.AddCatwalk({ -18.0f, 3.6f, -12.0f }, { -18.0f, 3.6f, 12.0f }, 4.2f, tileWhite, pillarCol);
        m.AddCatwalk({  18.0f, 3.6f, -12.0f }, {  18.0f, 3.6f, 12.0f }, 4.2f, tileWhite, pillarCol);
        // Continuous Side Mezzanines running full length (x = +/-44m)
        m.AddBox({ -44.0f, 3.6f, 0.0f }, { 18.0f, 0.35f, 140.0f }, tileWhite, MAT_CONCRETE);
        m.AddBox({  44.0f, 3.6f, 0.0f }, { 18.0f, 0.35f, 140.0f }, tileWhite, MAT_CONCRETE);

        // 5. Vertical Escalators & Emergency Stairwells (Connecting Level 0 to Level 1)
        int escSteps = 14;
        for (int sideZ = -1; sideZ <= 1; sideZ += 2) {
            for (int sideX = -1; sideX <= 1; sideX += 2) {
                float escX = sideX * 12.0f;
                // Escalator ramps
                for (int i = 0; i < escSteps; ++i) {
                    float frac = (float)i / (float)escSteps;
                    float h = (i + 1) * (3.6f / escSteps);
                    float z = sideZ * (46.0f - frac * 18.0f);
                    m.AddBox({ escX, h * 0.5f, z }, { 4.6f, h, 1.4f }, { 88, 94, 104, 255 }, MAT_CONCRETE);
                }
                // Handrails
                m.AddBox({ escX - 2.4f, 2.4f, sideZ * 37.0f }, { 0.2f, 1.4f, 19.0f }, { 45, 140, 195, 255 }, MAT_SHEET_METAL);
                m.AddBox({ escX + 2.4f, 2.4f, sideZ * 37.0f }, { 0.2f, 1.4f, 19.0f }, { 45, 140, 195, 255 }, MAT_SHEET_METAL);
            }
            // End Emergency Stairwells at z = +/- 62m
            m.AddRampStairs({ -44.0f, 0.0f, sideZ * 66.0f }, { -44.0f, 3.6f, sideZ * 54.0f }, 3.5f, 12, concGrey, MAT_CONCRETE);
            m.AddRampStairs({  44.0f, 0.0f, sideZ * 66.0f }, {  44.0f, 3.6f, sideZ * 54.0f }, 3.5f, 12, concGrey, MAT_CONCRETE);
        }

        // Upper Concourse Ticket Booths, Turnstiles & Partition Kiosks
        for (int sideZ = -1; sideZ <= 1; sideZ += 2) {
            float kz = sideZ * 28.0f;
            m.AddBox({ -16.0f, 4.5f, kz }, { 4.5f, 1.8f, 1.2f }, woodCol, MAT_WOOD_CRATE);
            m.AddBox({  16.0f, 4.5f, kz }, { 4.5f, 1.8f, 1.2f }, woodCol, MAT_WOOD_CRATE);
            m.AddCoverCluster({ 0.0f, 3.6f, kz }, pillarCol, woodCol);
        }

        m.phantomSpawns = { { -44, 1.0f, -72 }, { 0, 1.0f, -74 }, { 44, 1.0f, -72 }, { 0, 4.0f, -48 } };
        m.ghostSpawns   = { { -44, 1.0f,  72 }, { 0, 1.0f,  74 }, { 44, 1.0f,  72 }, { 0, 4.0f,  48 } };
        m.waypoints = {
            // Level 0 Outer Service Corridors (y = 0.25f)
            { -48.0f, 0.25f, -46.0f }, { -48.0f, 0.25f, 0.0f }, { -48.0f, 0.25f, 46.0f },
            {  48.0f, 0.25f, -46.0f }, {  48.0f, 0.25f, 0.0f }, {  48.0f, 0.25f, 46.0f },
            {   0.0f, 3.85f,   0.0f }, // Elevated Track Crossover Bridge Midpoint
            // Level 0 Platforms & Tracks (y = 0.35f)
            {   0.0f, 0.35f, -62.0f }, {   0.0f, 0.35f, -36.0f }, {   0.0f, 0.35f, 0.0f },
            {   0.0f, 0.35f,  36.0f }, {   0.0f, 0.35f,  62.0f },
            { -26.0f, 0.35f, -48.0f }, { -26.0f, 0.35f, 0.0f }, { -26.0f, 0.35f, 48.0f }, // West train walk
            {  26.0f, 0.35f, -48.0f }, {  26.0f, 0.35f, 0.0f }, {  26.0f, 0.35f, 48.0f }, // East train walk
            { -44.0f, 0.35f, -50.0f }, { -44.0f, 0.35f, 0.0f }, { -44.0f, 0.35f, 50.0f },
            {  44.0f, 0.35f, -50.0f }, {  44.0f, 0.35f, 0.0f }, {  44.0f, 0.35f, 50.0f },
            // Escalators (Top landings at y = 3.85f, z = +/-28m)
            { -12.0f, 3.85f, -28.0f }, { 12.0f, 3.85f, -28.0f },
            { -12.0f, 3.85f,  28.0f }, { 12.0f, 3.85f,  28.0f },
            // Level 1 Concourse & Atrium (y = 3.85f)
            {   0.0f, 3.85f, -28.0f }, {   0.0f, 3.85f,  28.0f },
            { -18.0f, 3.85f,   0.0f }, {  18.0f, 3.85f,   0.0f },
            { -44.0f, 3.85f, -40.0f }, { -44.0f, 3.85f, 0.0f }, { -44.0f, 3.85f, 40.0f },
            {  44.0f, 3.85f, -40.0f }, {  44.0f, 3.85f, 0.0f }, {  44.0f, 3.85f, 40.0f },
            // Spawns
            { -44.0f, 0.35f, -70.0f }, {  44.0f, 0.35f, -70.0f },
            { -44.0f, 0.35f,  70.0f }, {  44.0f, 0.35f,  70.0f }
        };
        m.flarePoints = {
            { "A", { -44.0f, 3.8f, 0.0f }, 5.5f, -1, 0.0f }, // West Mezzanine Balcony
            { "B", {   0.0f, 3.8f, 0.0f }, 6.0f, -1, 0.0f }, // Central Grand Concourse
            { "C", {  44.0f, 3.8f, 0.0f }, 5.5f, -1, 0.0f }  // East Mezzanine Balcony
        };
        m.hillPoint = { "HILL", { 0.0f, 3.8f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_WAREHOUSE) {
        m.name = "WAREHOUSE";
        m.subtitle = "Expanded 160m Industrial Logistics Complex - 5 Warehouse Bays, Rail Yard & Office Annex";
        m.skyColor = { 142, 165, 190, 255 };
        m.groundColor = { 82, 86, 92, 255 };

        Color plywoodCol = { 188, 148, 96, 255 };
        Color sheetCol   = { 130, 142, 152, 255 };
        Color crateCol   = { 155, 110, 62, 255 };
        Color contBlue   = { 45, 92, 158, 255 };
        Color contOrange = { 188, 92, 42, 255 };
        Color brickRed   = { 165, 78, 62, 255 };

        // 1. Massive 160m x 160m Outer Boundary Perimeter
        float W = 160.0f;
        float D = 160.0f;
        m.AddBox({ 0, 4.5f, -D * 0.5f }, { W, 9.0f, 1.8f }, sheetCol, MAT_CONCRETE);
        m.AddBox({ 0, 4.5f,  D * 0.5f }, { W, 9.0f, 1.8f }, sheetCol, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 4.5f, 0 }, { 1.8f, 9.0f, D }, sheetCol, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 4.5f, 0 }, { 1.8f, 9.0f, D }, sheetCol, MAT_CONCRETE);

        // 2. CENTRAL BAY 0: Mega-Distribution Hub (32m x 32m, height 5.2m)
        // High-bay pallet racking, forklift ramps, and signature paper-thin plywood wallbang walls!
        m.AddBuilding({ 0.0f, 0.0f, 0.0f }, 32.0f, 32.0f, 5.2f, plywoodCol, MAT_WOOD_CRATE, false);
        // Overhead Gantry Crane Catwalk (y = 4.8m) spanning East-West across Central Bay
        m.AddCatwalk({ -15.0f, 4.8f, 0.0f }, { 15.0f, 4.8f, 0.0f }, 2.8f, sheetCol, sheetCol, MAT_SHEET_METAL);
        m.AddRampStairs({ -15.0f, 0.0f, -12.0f }, { -15.0f, 4.8f, 0.0f }, 2.2f, 12, sheetCol, MAT_SHEET_METAL);
        m.AddRampStairs({  15.0f, 0.0f,  12.0f }, {  15.0f, 4.8f, 0.0f }, 2.2f, 12, sheetCol, MAT_SHEET_METAL);
        // Interior high-bay racking and thin plywood office cubicle partitions (0.18m effective thickness!)
        for (int pz = -10; pz <= 10; pz += 10) {
            m.AddBox({ -7.5f, 1.8f, (float)pz }, { 8.5f, 3.6f, 0.35f }, plywoodCol, MAT_WOOD_CRATE);
            m.AddBox({  7.5f, 1.8f, (float)pz }, { 8.5f, 3.6f, 0.35f }, plywoodCol, MAT_WOOD_CRATE);
        }
        m.AddCoverCluster({ 0.0f, 0.0f, -7.5f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);
        m.AddCoverCluster({ 0.0f, 0.0f,  7.5f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);

        // 3. 4 OUTER WAREHOUSE WINGS (22m x 18m, height 4.2m)
        // Bay 1 (Northwest: Automated sorting depot) & Bay 2 (Southwest: Shipping container docks)
        m.AddBuilding({ -46.0f, 0.0f, -38.0f }, 22.0f, 18.0f, 4.2f, sheetCol,   MAT_SHEET_METAL, true);
        m.AddBuilding({ -46.0f, 0.0f,  38.0f }, 22.0f, 18.0f, 4.2f, plywoodCol, MAT_WOOD_CRATE,  true);
        // Bay 3 (Northeast: Metal fabrication workshop) & Bay 4 (Southeast: Packaging facility)
        m.AddBuilding({  46.0f, 0.0f, -38.0f }, 22.0f, 18.0f, 4.2f, plywoodCol, MAT_WOOD_CRATE,  true);
        m.AddBuilding({  46.0f, 0.0f,  38.0f }, 22.0f, 18.0f, 4.2f, sheetCol,   MAT_SHEET_METAL, true);

        // Inter-Roof Elevated Catwalk Skyways linking Bay roofs to Bay 0
        m.AddCatwalk({ -35.0f, 4.2f, -38.0f }, { -16.0f, 5.2f, -16.0f }, 2.0f, sheetCol, sheetCol, MAT_SHEET_METAL);
        m.AddCatwalk({  35.0f, 4.2f, -38.0f }, {  16.0f, 5.2f, -16.0f }, 2.0f, sheetCol, sheetCol, MAT_SHEET_METAL);
        m.AddCatwalk({ -35.0f, 4.2f,  38.0f }, { -16.0f, 5.2f,  16.0f }, 2.0f, sheetCol, sheetCol, MAT_SHEET_METAL);
        m.AddCatwalk({  35.0f, 4.2f,  38.0f }, {  16.0f, 5.2f,  16.0f }, 2.0f, sheetCol, sheetCol, MAT_SHEET_METAL);

        // 4. NORTH RAIL CARGO YARD (z = -56m to -72m)
        // Twin rail spurs with 3 freight boxcars and outdoor cargo staging
        m.AddBox({ -32.0f, 1.6f, -64.0f }, { 14.0f, 3.2f, 3.4f }, contBlue,   MAT_SHEET_METAL);
        m.AddBox({   0.0f, 1.6f, -66.0f }, { 14.0f, 3.2f, 3.4f }, contOrange, MAT_SHEET_METAL);
        m.AddBox({  32.0f, 1.6f, -64.0f }, { 14.0f, 3.2f, 3.4f }, contBlue,   MAT_SHEET_METAL);
        m.AddCoverCluster({ -16.0f, 0.0f, -56.0f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);
        m.AddCoverCluster({  16.0f, 0.0f, -56.0f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);

        // 5. SOUTH ADMINISTRATIVE OFFICE ANNEX (z = +56m to +72m)
        // Multi-room brick building with offices, reception & breakroom
        m.AddBuilding({ 0.0f, 0.0f, 64.0f }, 30.0f, 14.0f, 4.0f, brickRed, MAT_CONCRETE, true);
        // Outdoor parking lot container barrier
        m.AddBox({ -26.0f, 1.4f, 64.0f }, { 3.0f, 2.8f, 7.5f }, contBlue,   MAT_SHEET_METAL);
        m.AddBox({  26.0f, 1.4f, 64.0f }, { 3.0f, 2.8f, 7.5f }, contOrange, MAT_SHEET_METAL);

        // Flanking yard container stacks
        for (int side = -1; side <= 1; side += 2) {
            float fx = side * 48.0f;
            m.AddBox({ fx, 1.4f, 0.0f }, { 3.2f, 2.8f, 12.0f }, (side < 0) ? contOrange : contBlue, MAT_SHEET_METAL);
            m.AddCoverCluster({ fx, 0.0f, -18.0f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);
            m.AddCoverCluster({ fx, 0.0f,  18.0f }, crateCol, plywoodCol, MAT_WOOD_CRATE, MAT_WOOD_CRATE);
        }

        m.phantomSpawns = { { -46, 1, -70 }, { -14, 1, -72 }, { 14, 1, -72 }, { 46, 1, -70 } };
        m.ghostSpawns   = { { -46, 1,  70 }, { -14, 1,  72 }, { 14, 1,  72 }, { 46, 1,  70 } };
        m.waypoints.insert(m.waypoints.end(), {
            {   0.0f, 0.25f,   0.0f }, // Bay 0 Center Floor
            {   0.0f, 5.05f,   0.0f }, // Bay 0 Overhead Crane Catwalk
            { -14.0f, 0.25f, -14.0f }, {  14.0f, 0.25f, -14.0f },
            { -14.0f, 0.25f,  14.0f }, {  14.0f, 0.25f,  14.0f },
            // Outer Bays 1, 2, 3, 4
            { -46.0f, 0.25f, -38.0f }, { -46.0f, 4.45f, -38.0f },
            { -46.0f, 0.25f,  38.0f }, { -46.0f, 4.45f,  38.0f },
            {  46.0f, 0.25f, -38.0f }, {  46.0f, 4.45f, -38.0f },
            {  46.0f, 0.25f,  38.0f }, {  46.0f, 4.45f,  38.0f },
            // North Rail Yard & South Office
            { -32.0f, 0.25f, -64.0f }, {   0.0f, 0.25f, -64.0f }, {  32.0f, 0.25f, -64.0f },
            { -24.0f, 0.25f,  64.0f }, {   0.0f, 0.25f,  64.0f }, {  24.0f, 0.25f,  64.0f },
            // Flank Loading Docks & Yards
            { -48.0f, 0.25f,   0.0f }, {  48.0f, 0.25f,   0.0f },
            { -24.0f, 0.25f, -38.0f }, {  24.0f, 0.25f, -38.0f },
            { -24.0f, 0.25f,  38.0f }, {  24.0f, 0.25f,  38.0f }
        });
        m.flarePoints = {
            { "A", { -48.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }, // West Logistics Docks
            { "B", {   0.0f, 0.5f, 0.0f }, 6.0f, -1, 0.0f }, // Central Bay 0
            { "C", {  48.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }  // East Rail Yard
        };
        m.hillPoint = { "HILL", { 0.0f, 0.5f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_FOUNDRY) {
        m.name = "FOUNDRY";
        m.subtitle = "Expanded 180m Steel Mill Complex - 3-Tier Casting Hall, Blast Furnaces & Flue Tunnel";
        m.skyColor = { 142, 118, 104, 255 };
        m.groundColor = { 58, 56, 54, 255 };

        Color millRust   = { 148, 78, 54, 255 };
        Color darkSteel  = { 54, 58, 66, 255 };
        Color hopperGrey = { 104, 110, 118, 255 };
        Color slagGlow   = { 242, 112, 28, 255 };
        Color crateWood  = { 154, 112, 66, 255 };

        // 1. Massive 180m x 120m Outer Mill Boundary
        float W = 180.0f;
        float D = 120.0f;
        m.AddBox({ 0, 5.0f, -D * 0.5f }, { W, 10.0f, 1.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({ 0, 5.0f,  D * 0.5f }, { W, 10.0f, 1.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 5.0f, 0 }, { 1.8f, 10.0f, D }, darkSteel, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 5.0f, 0 }, { 1.8f, 10.0f, D }, darkSteel, MAT_CONCRETE);

        // 2. CENTRAL CASTING HALL & SMELTER CORE (44m x 36m, 3 Tiers, Height 8.5m)
        m.AddBuilding({ 0.0f, 0.0f, 0.0f }, 44.0f, 36.0f, 6.0f, millRust, MAT_SHEET_METAL, false);
        // Level 2 (y = 4.0m): Steel Mesh Inspection Catwalk Wrapping Around the Smelter
        m.AddCatwalk({ -18.0f, 4.0f, -14.0f }, { 18.0f, 4.0f, -14.0f }, 3.2f, darkSteel, hopperGrey, MAT_SHEET_METAL);
        m.AddCatwalk({ -18.0f, 4.0f,  14.0f }, { 18.0f, 4.0f,  14.0f }, 3.2f, darkSteel, hopperGrey, MAT_SHEET_METAL);
        m.AddCatwalk({ -18.0f, 4.0f, -14.0f }, { -18.0f, 4.0f, 14.0f }, 3.2f, darkSteel, hopperGrey, MAT_SHEET_METAL);
        m.AddCatwalk({  18.0f, 4.0f, -14.0f }, {  18.0f, 4.0f, 14.0f }, 3.2f, darkSteel, hopperGrey, MAT_SHEET_METAL);
        // Dual Catwalk Stairs inside Casting Hall
        m.AddRampStairs({ -16.0f, 0.0f, -8.0f }, { -16.0f, 4.0f, 4.0f }, 2.4f, 11, darkSteel, MAT_SHEET_METAL);
        m.AddRampStairs({  16.0f, 0.0f,  8.0f }, {  16.0f, 4.0f, -4.0f }, 2.4f, 11, darkSteel, MAT_SHEET_METAL);
        // Level 3 (y = 8.0m): High-Altitude Ladle Crane Bridge spanning across center
        m.AddCatwalk({ 0.0f, 8.0f, -14.0f }, { 0.0f, 8.0f, 14.0f }, 3.4f, darkSteel, slagGlow, MAT_SHEET_METAL);

        // Molten Slag Crucible Vats & Ingot Molds (carefully keeping line between (0, 1.5, 0) and (0, 1.0, 4.0) clear)
        m.AddBox({ -10.0f, 0.9f,  8.5f }, { 3.8f, 1.8f, 3.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({ -10.0f, 1.85f, 8.5f }, { 3.0f, 0.15f, 3.0f }, slagGlow, MAT_CONCRETE);
        m.AddBox({  10.0f, 0.9f,  8.5f }, { 3.8f, 1.8f, 3.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({  10.0f, 1.85f, 8.5f }, { 3.0f, 0.15f, 3.0f }, slagGlow, MAT_CONCRETE);
        m.AddBox({ -10.0f, 0.9f, -8.5f }, { 3.8f, 1.8f, 3.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({ -10.0f, 1.85f,-8.5f }, { 3.0f, 0.15f, 3.0f }, slagGlow, MAT_CONCRETE);
        m.AddBox({  10.0f, 0.9f, -8.5f }, { 3.8f, 1.8f, 3.8f }, darkSteel, MAT_CONCRETE);
        m.AddBox({  10.0f, 1.85f,-8.5f }, { 3.0f, 0.15f, 3.0f }, slagGlow, MAT_CONCRETE);

        // 3. BLAST FURNACE TOWERS (West & East Flanks at x = +/-52m, Height 10m)
        for (int side = -1; side <= 1; side += 2) {
            float fx = side * 52.0f;
            m.AddBox({ fx, 5.0f, 0.0f }, { 9.0f, 10.0f, 9.0f }, darkSteel, MAT_CONCRETE);
            m.AddBox({ fx, 8.2f, 0.0f }, { 11.5f, 0.4f, 11.5f }, hopperGrey, MAT_SHEET_METAL); // Balcony
            m.AddRampStairs({ fx - side * 14.0f, 0.0f, 0.0f }, { fx - side * 6.0f, 8.2f, 0.0f }, 2.4f, 14, darkSteel, MAT_SHEET_METAL);
        }

        // 4. 4 ORE & COKE HOPPERS (x = +/-30m, z = +/-40m)
        m.AddBuilding({ -30.0f, 0.0f, -40.0f }, 24.0f, 18.0f, 4.4f, hopperGrey, MAT_SHEET_METAL, false);
        m.AddBuilding({  30.0f, 0.0f, -40.0f }, 24.0f, 18.0f, 4.4f, hopperGrey, MAT_SHEET_METAL, false);
        m.AddBuilding({ -30.0f, 0.0f,  40.0f }, 24.0f, 18.0f, 4.4f, hopperGrey, MAT_SHEET_METAL, false);
        m.AddBuilding({  30.0f, 0.0f,  40.0f }, 24.0f, 18.0f, 4.4f, hopperGrey, MAT_SHEET_METAL, false);

        // 5. ENCLOSED SLAG CONVEYOR GALLERY & ROOF CATWALK (Ground y = 0.0m, Roof Catwalk y = 3.0m)
        m.AddTunnelPassage({ 0.0f, 0.0f, -42.0f }, { 0.0f, 0.0f, 42.0f }, 3.6f, 2.8f, darkSteel, MAT_CONCRETE);
        m.AddRampStairs({ 0.0f, 0.0f, -50.0f }, { 0.0f, 3.0f, -42.0f }, 2.8f, 8, darkSteel, MAT_CONCRETE);
        m.AddRampStairs({ 0.0f, 0.0f,  50.0f }, { 0.0f, 3.0f,  42.0f }, 2.8f, 8, darkSteel, MAT_CONCRETE);

        // 6. SCRAP SORTING YARDS & CRUSHER BLOCKS (North & South exterior yards)
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                m.AddBox({ sx * 16.0f, 1.2f, sz * 42.0f }, { 4.5f, 2.4f, 2.8f }, hopperGrey, MAT_SHEET_METAL);
                m.AddCoverCluster({ sx * 44.0f, 0.0f, sz * 24.0f }, darkSteel, crateWood, MAT_SHEET_METAL, MAT_WOOD_CRATE);
            }
        }

        m.phantomSpawns = { { -78, 1, -12 }, { -78, 1, -4 }, { -78, 1, 4 }, { -78, 1, 12 } };
        m.ghostSpawns   = { {  78, 1, -12 }, {  78, 1, -4 }, {  78, 1, 4 }, {  78, 1, 12 } };
        m.waypoints.insert(m.waypoints.end(), {
            {   0.0f, 0.25f,   0.0f }, // Casting Hall Ground
            {   0.0f, 4.25f,   0.0f }, // Casting Hall Mid-Catwalk
            {   0.0f, 8.25f,   0.0f }, // Ladle Crane Bridge High Ground
            {   0.0f, 3.25f,   0.0f }, // Gallery Roof Catwalk
            {   0.0f, 0.25f, -30.0f }, {   0.0f, 0.25f,  30.0f }, // Conveyor Gallery Interior
            // Blast Furnace Towers
            { -52.0f, 0.25f,   0.0f }, { -52.0f, 8.45f,   0.0f },
            {  52.0f, 0.25f,   0.0f }, {  52.0f, 8.45f,   0.0f },
            // North & South Yards
            {   0.0f, 0.25f, -42.0f }, {   0.0f, 0.25f,  42.0f },
            { -30.0f, 0.25f, -40.0f }, {  30.0f, 0.25f, -40.0f },
            { -30.0f, 0.25f,  40.0f }, {  30.0f, 0.25f,  40.0f },
            // Flanks & Spawns
            { -72.0f, 0.25f,   0.0f }, {  72.0f, 0.25f,   0.0f },
            { -44.0f, 0.25f, -24.0f }, {  44.0f, 0.25f, -24.0f },
            { -44.0f, 0.25f,  24.0f }, {  44.0f, 0.25f,  24.0f }
        });
        m.flarePoints = {
            { "A", { 0.0f, 0.5f, -42.0f }, 5.5f, -1, 0.0f }, // North Ore Yard
            { "B", { 0.0f, 4.2f,   0.0f }, 6.0f, -1, 0.0f }, // Central Casting Hall Catwalk
            { "C", { 0.0f, 0.5f,  42.0f }, 5.5f, -1, 0.0f }  // South Smelter Yard
        };
        m.hillPoint = { "HILL", { 0.0f, 0.5f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_DOCKSIDE_CUTOFF) {
        m.name = "DOCKSIDE CUTOFF";
        m.subtitle = "Expanded 180m Container Port - Moored Cargo Ship, Twin Gantry Cranes & Cold Storage";
        m.skyColor = { 18, 26, 42, 255 };
        m.groundColor = { 62, 68, 76, 255 };

        Color waterBlue  = { 24, 56, 86, 255 };
        Color contBlue   = { 42, 95, 165, 255 };
        Color contOrange = { 195, 88, 38, 255 };
        Color contRed    = { 165, 45, 48, 255 };
        Color contGreen  = { 58, 112, 72, 255 };
        Color vaultGrey  = { 118, 126, 136, 255 };
        Color craneGold  = { 218, 168, 42, 255 };
        Color shipHull   = { 36, 40, 48, 255 };
        Color darkSteel  = { 54, 58, 66, 255 };

        // 1. Massive 180m x 120m Outer Port Boundary
        float W = 180.0f;
        float D = 120.0f;
        m.AddBox({ 0, 5.0f, -D * 0.5f }, { W, 10.0f, 1.8f }, vaultGrey, MAT_CONCRETE);
        m.AddBox({ 0, 5.0f,  D * 0.5f }, { W, 10.0f, 1.8f }, vaultGrey, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 5.0f, 0 }, { 1.8f, 10.0f, D }, vaultGrey, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 5.0f, 0 }, { 1.8f, 10.0f, D }, vaultGrey, MAT_CONCRETE);

        // 2. SOUTH HARBOR WATER BASIN & THE "SS PHANTOM" MOORED CONTAINER CARGO SHIP (z = +36m to +58m)
        m.AddBox({ 0.0f, 0.05f, 46.0f }, { 174.0f, 0.10f, 26.0f }, waterBlue, MAT_CONCRETE);
        // Ship Main Hull (Spans x = -55m to +55m, Width 18m, Deck y = 2.4m)
        m.AddBox({ 0.0f, 1.2f, 46.0f }, { 110.0f, 2.4f, 18.0f }, shipHull, MAT_SHEET_METAL);
        m.AddBox({ 0.0f, 2.45f, 46.0f }, { 108.0f, 0.20f, 17.5f }, vaultGrey, MAT_SHEET_METAL); // Main deck
        // Ship Bulwarks / Railings
        m.AddBox({ 0.0f, 3.05f, 37.25f }, { 108.0f, 1.0f, 0.35f }, shipHull, MAT_SHEET_METAL);
        m.AddBox({ 0.0f, 3.05f, 54.75f }, { 108.0f, 1.0f, 0.35f }, shipHull, MAT_SHEET_METAL);
        // Superstructure Aft Bridge & Wheelhouse (x = +40m to +52m, Height 7.2m)
        m.AddBuilding({ 44.0f, 2.4f, 46.0f }, 16.0f, 14.0f, 4.8f, vaultGrey, MAT_CONCRETE, true);
        // Cargo Hold Hatch Stacks on Ship Deck
        m.AddBox({ -32.0f, 3.8f, 46.0f }, { 12.0f, 2.7f, 5.5f }, contBlue,   MAT_SHEET_METAL);
        m.AddBox({ -16.0f, 3.8f, 46.0f }, { 12.0f, 2.7f, 5.5f }, contOrange, MAT_SHEET_METAL);
        m.AddBox({   8.0f, 3.8f, 46.0f }, { 12.0f, 2.7f, 5.5f }, contRed,    MAT_SHEET_METAL);
        // 2 Gangway Ramps Connecting Ship Deck (y = 2.4m) to Concrete Pier (y = 0.0m)
        m.AddRampStairs({ -36.0f, 0.0f, 32.0f }, { -36.0f, 2.4f, 38.0f }, 3.2f, 8, vaultGrey, MAT_SHEET_METAL);
        m.AddRampStairs({  24.0f, 0.0f, 32.0f }, {  24.0f, 2.4f, 38.0f }, 3.2f, 8, vaultGrey, MAT_SHEET_METAL);

        // 3. TWIN PORTAL GANTRY SHIP-TO-SHORE CRANES (x = -34m and +34m, z = +20m, Height 14m)
        for (int side = -1; side <= 1; side += 2) {
            float gcx = side * 34.0f;
            // 4 Massive Vertical Steel Legs Straddling the Wharf Road
            m.AddBox({ gcx - 3.8f, 6.0f, 15.0f }, { 0.9f, 12.0f, 0.9f }, craneGold, MAT_SHEET_METAL);
            m.AddBox({ gcx + 3.8f, 6.0f, 15.0f }, { 0.9f, 12.0f, 0.9f }, craneGold, MAT_SHEET_METAL);
            m.AddBox({ gcx - 3.8f, 6.0f, 25.0f }, { 0.9f, 12.0f, 0.9f }, craneGold, MAT_SHEET_METAL);
            m.AddBox({ gcx + 3.8f, 6.0f, 25.0f }, { 0.9f, 12.0f, 0.9f }, craneGold, MAT_SHEET_METAL);
            // Gantry Machine Bridge & Operator Cabin at y = 12.0m
            m.AddBox({ gcx, 12.2f, 20.0f }, { 10.0f, 1.2f, 12.0f }, craneGold, MAT_SHEET_METAL);
            m.AddBox({ gcx, 13.8f, 20.0f }, { 4.5f, 2.0f, 4.5f }, craneGold, MAT_SHEET_METAL);
            // Horizontal Boom Catwalk extending over the ship (z = 20m to 48m at y = 12.5m)
            m.AddCatwalk({ gcx, 12.5f, 15.0f }, { gcx, 12.5f, 44.0f }, 2.4f, craneGold, craneGold, MAT_SHEET_METAL);
            // Access Stairs up to Gantry Deck
            m.AddRampStairs({ gcx - 3.8f, 0.0f, 8.0f }, { gcx - 3.8f, 12.0f, 15.0f }, 2.0f, 16, darkSteel, MAT_SHEET_METAL);
        }

        // 4. 3-TIER CONTAINER STACK LABYRINTH (Central Wharf, z = -18m to +16m)
        // Multi-level stacks with climbable stairs and open container interior shortcut tunnels
        for (int side = -1; side <= 1; side += 2) {
            float cx = side * 52.0f;
            // North stack: 2-high (y = 5.4m)
            m.AddBox({ cx - 7.0f, 1.4f, -14.0f }, { 12.0f, 2.7f, 5.5f }, contBlue,   MAT_SHEET_METAL);
            m.AddBox({ cx - 7.0f, 4.1f, -14.0f }, { 12.0f, 2.7f, 5.5f }, contRed,    MAT_SHEET_METAL);
            // Center stack: 3-high (y = 8.1m)
            m.AddBox({ cx + 7.0f, 1.4f, -4.0f }, { 12.0f, 2.7f, 5.5f }, contOrange, MAT_SHEET_METAL);
            m.AddBox({ cx + 7.0f, 4.1f, -4.0f }, { 12.0f, 2.7f, 5.5f }, contGreen,  MAT_SHEET_METAL);
            m.AddBox({ cx + 7.0f, 6.8f, -4.0f }, { 12.0f, 2.7f, 5.5f }, contBlue,   MAT_SHEET_METAL);
            m.AddRampStairs({ cx + 14.0f, 0.0f, -4.0f }, { cx + 7.0f, 5.4f, -4.0f }, 2.2f, 12, vaultGrey, MAT_SHEET_METAL);

            // Center Alley Flank Containers (z = 0m and +12m)
            m.AddBox({ side * 18.0f, 1.4f,  0.0f }, { 14.0f, 2.8f, 5.5f }, (side < 0) ? contOrange : contBlue, MAT_SHEET_METAL);
            m.AddBox({ side * 18.0f, 1.4f, 12.0f }, { 14.0f, 2.8f, 5.5f }, (side < 0) ? contRed : contGreen,  MAT_SHEET_METAL);
            m.AddCoverCluster({ side * 68.0f, 0.0f, 0.0f }, contBlue, contOrange, MAT_SHEET_METAL, MAT_SHEET_METAL);
        }

        // 5. COLD STORAGE WAREHOUSE & CUSTOMS INSPECTION COMPLEX (North Flank, z = -36m to -52m)
        m.AddBuilding({ 0.0f, 0.0f, -42.0f }, 46.0f, 22.0f, 4.6f, vaultGrey, MAT_CONCRETE, true);
        // Roof refrigeration compressor chillers
        m.AddBox({ -12.0f, 5.4f, -42.0f }, { 4.5f, 1.6f, 3.8f }, darkSteel, MAT_SHEET_METAL);
        m.AddBox({  12.0f, 5.4f, -42.0f }, { 4.5f, 1.6f, 3.8f }, darkSteel, MAT_SHEET_METAL);

        m.phantomSpawns = { { -24, 1, -52 }, { -8, 1, -54 }, { 8, 1, -54 }, { 24, 1, -52 } };
        m.ghostSpawns   = { { -28, 3.4f, 46 }, { -12, 3.4f, 46 }, { 12, 3.4f, 46 }, { 28, 3.4f, 46 } };
        m.waypoints.insert(m.waypoints.end(), {
            // Ship Deck (y = 2.65f)
            { -36.0f, 2.65f, 46.0f }, { 0.0f, 2.65f, 46.0f }, { 36.0f, 2.65f, 46.0f },
            {  44.0f, 7.45f, 46.0f }, // Ship Bridge High Ground
            // Gantry Cranes (High boom at y = 12.75f)
            { -34.0f, 12.75f, 22.0f }, { 34.0f, 12.75f, 22.0f },
            // Wharf Road & Center Alley
            { -34.0f, 0.25f, 20.0f }, {  34.0f, 0.25f, 20.0f },
            {   0.0f, 0.25f, 20.0f }, {   0.0f, 0.25f,  6.0f }, { 0.0f, 0.25f, -8.0f },
            { -18.0f, 0.25f,  6.0f }, {  18.0f, 0.25f,  6.0f },
            { -52.0f, 0.25f, -14.0f }, {  52.0f, 0.25f, -14.0f },
            { -52.0f, 5.65f,  -4.0f }, {  52.0f, 5.65f,  -4.0f }, // Container Top High Ground
            // North Cold Storage & Customs
            { -22.0f, 0.25f, -42.0f }, {   0.0f, 0.25f, -42.0f }, { 22.0f, 0.25f, -42.0f },
            {   0.0f, 4.85f, -42.0f },
            // Spawns & Outer Perimeters
            { -68.0f, 0.25f, 0.0f }, { 68.0f, 0.25f, 0.0f },
            { -68.0f, 0.25f, -36.0f }, { 68.0f, 0.25f, -36.0f }
        });
        m.flarePoints = {
            { "A", { -34.0f, 0.5f,  20.0f }, 5.5f, -1, 0.0f }, // West Gantry Crane Pier
            { "B", {   0.0f, 0.5f,   0.0f }, 6.0f, -1, 0.0f }, // Central Container Alley
            { "C", {   0.0f, 0.5f, -42.0f }, 5.5f, -1, 0.0f }  // North Cold Storage Vault
        };
        m.hillPoint = { "HILL", { 0.0f, 0.5f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_HIGHRISE_ATLAS) {
        // ====================================================================
        // BLUEPRINT 3: HIGHRISE ATLAS (15v15 Financial District - 9 Towers & Skybridges)
        // SVG Blueprint (300x180 -> 150m x 90m):
        // - Diagonal Corner Spawns: Team A NW (4..30, 4..30), Team B SE (270..296, 150..176)
        // - 3x3 Grid of 9 Towers (cols x = -42.5, 0, +42.5; rows z = -27.5, 0, +27.5)
        // - Interior stairwells to 2nd floor/roofs + Elevated Skybridges connecting towers!
        // - 5 Hardpoint Intersections around the Center Tower
        // ====================================================================
        m.name = "HIGHRISE ATLAS";
        m.subtitle = "Expanded 180m Financial District - 9 Skyscrapers, Two-Tier Skybridges & Sunken Plaza";
        m.skyColor = { 165, 198, 232, 255 };
        m.groundColor = { 70, 74, 82, 255 };

        Color towerBlue  = { 86, 108, 132, 255 };
        Color towerSlate = { 104, 114, 126, 255 };
        Color bridgeCol  = { 132, 144, 158, 255 };
        Color glassRail  = { 145, 205, 240, 255 };
        Color plazaStone = { 148, 152, 158, 255 };
        Color heliYellow = { 235, 195, 45, 255 };

        // 1. Massive 180m x 120m Outer City Boundary
        float W = 180.0f;
        float D = 120.0f;
        m.AddBox({ 0, 6.0f, -D * 0.5f }, { W, 12.0f, 1.8f }, towerSlate, MAT_CONCRETE);
        m.AddBox({ 0, 6.0f,  D * 0.5f }, { W, 12.0f, 1.8f }, towerSlate, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, towerSlate, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, towerSlate, MAT_CONCRETE);

        // 2. 3x3 GRID OF 9 SKYSCRAPERS (cols x = -50, 0, +50; rows z = -32, 0, +32)
        // Center Tower: The Atlas Tower (26m x 22m, 3 Stories, Height 12.0m)
        m.AddBuilding({ 0.0f, 0.0f, 0.0f }, 26.0f, 22.0f, 8.4f, towerBlue, MAT_SANDSTONE, true);
        // Penthouse Floor & Rooftop Helipad atop Center Tower (y = 8.4m to 12.0m)
        m.AddBox({ 0.0f, 10.2f, 0.0f }, { 16.0f, 3.6f, 14.0f }, towerBlue, MAT_SANDSTONE);
        m.AddBox({ 0.0f, 12.1f, 0.0f }, { 18.0f, 0.25f, 16.0f }, plazaStone, MAT_CONCRETE); // Helipad
        m.AddBox({ 0.0f, 12.25f, 0.0f }, { 8.0f, 0.05f, 8.0f }, heliYellow, MAT_SHEET_METAL); // Helipad 'H' marking
        m.AddBox({ 0.0f, 14.2f, -7.0f }, { 0.4f, 4.0f, 0.4f }, towerSlate, MAT_SHEET_METAL); // Radio mast

        // 8 Perimeter Skyscraper Towers (2-3 Stories, Heights 8.0m to 10.5m)
        float towerX[3] = { -52.0f, 0.0f, 52.0f };
        float towerZ[3] = { -32.0f, 0.0f, 32.0f };
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                if (r == 1 && c == 1) continue; // Center tower already built
                Color tCol = ((r + c) % 2 == 0) ? towerBlue : towerSlate;
                float th = ((r + c) % 3 == 0) ? 9.5f : 8.0f;
                m.AddBuilding({ towerX[c], 0.0f, towerZ[r] }, 22.0f, 18.0f, th, tCol, MAT_SANDSTONE, true);
            }
        }

        // 3. TWO-TIER ELEVATED SKYBRIDGE NETWORK
        // Tier 1 Skybridges at y = 4.2m (East-West across streets)
        for (int rowSide = -1; rowSide <= 1; rowSide += 2) {
            float bz = rowSide * 32.0f;
            for (int bSide = -1; bSide <= 1; bSide += 2) {
                float bx = bSide * 26.0f;
                m.AddCatwalk({ bx - 14.0f, 4.2f, bz }, { bx + 14.0f, 4.2f, bz }, 3.6f, bridgeCol, glassRail);
            }
        }
        // Tier 2 High-Altitude Skybridges at y = 8.4m linking North/South Towers into Center Tower Roof
        m.AddCatwalk({ 0.0f, 8.4f, -22.0f }, { 0.0f, 8.4f, -12.0f }, 3.4f, bridgeCol, glassRail);
        m.AddCatwalk({ 0.0f, 8.4f,  12.0f }, { 0.0f, 8.4f,  22.0f }, 3.4f, bridgeCol, glassRail);

        // 4. COVERED TRANSIT ATRIUM ARCADE & MEZZANINE PLAZA (Ground y = 0.0m, Mezzanine y = 3.8m)
        // Central pedestrian arcade linking North & South street canyons
        m.AddTunnelPassage({ 0.0f, 0.0f, -28.0f }, { 0.0f, 0.0f, -12.0f }, 3.6f, 3.2f, plazaStone, MAT_CONCRETE);
        m.AddTunnelPassage({ 0.0f, 0.0f,  12.0f }, { 0.0f, 0.0f,  28.0f }, 3.6f, 3.2f, plazaStone, MAT_CONCRETE);
        // Elevated Central Plaza Deck (y = 3.8m)
        m.AddBox({ 0.0f, 3.8f, 0.0f }, { 28.0f, 0.30f, 24.0f }, plazaStone, MAT_CONCRETE);
        m.AddRampStairs({ 0.0f, 0.0f, -32.0f }, { 0.0f, 3.8f, -20.0f }, 2.8f, 9, plazaStone, MAT_CONCRETE);
        m.AddRampStairs({ 0.0f, 0.0f,  32.0f }, { 0.0f, 3.8f,  20.0f }, 2.8f, 9, plazaStone, MAT_CONCRETE);

        // 5. STREET CANYON COVER (Armored Vans, Concrete Planters & Bus Shelters)
        Vector2 hardpoints[4] = { { -26.0f, -16.0f }, { 26.0f, -16.0f }, { -26.0f, 16.0f }, { 26.0f, 16.0f } };
        for (int i = 0; i < 4; ++i) {
            float hx = hardpoints[i].x;
            float hz = hardpoints[i].y;
            // Armored security van
            m.AddBox({ hx, 1.2f, hz }, { 5.2f, 2.4f, 2.4f }, towerSlate, MAT_SHEET_METAL);
            m.AddCoverCluster({ hx - 4.5f, 0.0f, hz }, plazaStone, towerBlue);
            m.AddCoverCluster({ hx + 4.5f, 0.0f, hz }, plazaStone, towerBlue);
        }

        m.phantomSpawns = { { -74, 1, -48 }, { -70, 1, -48 }, { -74, 1, -42 }, { -68, 1, -44 } };
        m.ghostSpawns   = { {  74, 1,  48 }, {  70, 1,  48 }, {  74, 1,  42 }, {  68, 1,  44 } };
        m.waypoints.insert(m.waypoints.end(), {
            // Center Tower (Ground, Roof & Helipad)
            {   0.0f, 0.25f,   0.0f }, {   0.0f, 8.65f,   0.0f }, {   0.0f, 12.35f,   0.0f },
            // Transit Arcade & Mezzanine Plaza Deck
            {   0.0f, 4.05f,   0.0f }, {   0.0f, 0.25f, -20.0f }, {   0.0f, 0.25f,  20.0f },
            // Street Hardpoint Intersections
            { -26.0f, 0.25f, -16.0f }, {  26.0f, 0.25f, -16.0f },
            { -26.0f, 0.25f,  16.0f }, {  26.0f, 0.25f,  16.0f },
            // Tier 1 Skybridges (y = 4.45f)
            { -26.0f, 4.45f, -32.0f }, {  26.0f, 4.45f, -32.0f },
            { -26.0f, 4.45f,  32.0f }, {  26.0f, 4.45f,  32.0f },
            // Tier 2 Skybridges (y = 8.65f)
            {   0.0f, 8.65f, -16.0f }, {   0.0f, 8.65f,  16.0f },
            // Tower Entrances & Roofs
            { -52.0f, 0.25f, -32.0f }, { -52.0f, 8.25f, -32.0f },
            {  52.0f, 0.25f, -32.0f }, {  52.0f, 8.25f, -32.0f },
            { -52.0f, 0.25f,  32.0f }, { -52.0f, 8.25f,  32.0f },
            {  52.0f, 0.25f,  32.0f }, {  52.0f, 8.25f,  32.0f },
            // Outer Spawns & Avenues
            { -70.0f, 0.25f, -44.0f }, {  70.0f, 0.25f,  44.0f },
            { -70.0f, 0.25f,   0.0f }, {  70.0f, 0.25f,   0.0f },
            {   0.0f, 0.25f, -44.0f }, {   0.0f, 0.25f,  44.0f }
        });
        m.flarePoints = {
            { "A", { -26.0f, 0.5f, -16.0f }, 5.5f, -1, 0.0f }, // NW Street Intersection
            { "B", {   0.0f, 8.6f,   0.0f }, 6.0f, -1, 0.0f }, // Center Atlas Tower Helipad
            { "C", {  26.0f, 0.5f,  16.0f }, 5.5f, -1, 0.0f }  // SE Street Intersection
        };
        m.hillPoint = { "HILL", { 0.0f, 0.5f, 0.0f }, 6.5f, -1, 0.0f };

    } else if (id == MAP_SALT_FLATS) {
        m.name = "SALT FLATS OUTPOST";
        m.subtitle = "Expanded 200m Desert Basin - 2-Level Mesa Caverns, Radar Summit & Canyon Trails";
        m.skyColor = { 175, 214, 250, 255 };
        m.groundColor = { 232, 224, 206, 255 };

        Color mesaRock   = { 196, 126, 84, 255 };
        Color canyonDark = { 168, 104, 68, 255 };
        Color outpostTan = { 205, 185, 150, 255 };
        Color buggyOlive = { 82, 94, 68, 255 };
        Color crateWood  = { 156, 114, 68, 255 };

        // 1. Massive 200m x 130m Outer Desert Canyon Boundary
        float W = 200.0f;
        float D = 130.0f;
        m.AddBox({ 0, 6.0f, -D * 0.5f }, { W, 12.0f, 1.8f }, canyonDark, MAT_CONCRETE);
        m.AddBox({ 0, 6.0f,  D * 0.5f }, { W, 12.0f, 1.8f }, canyonDark, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, canyonDark, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 6.0f, 0 }, { 1.8f, 12.0f, D }, canyonDark, MAT_CONCRETE);

        // 2. CENTRAL MESA GEOLOGICAL COMPLEX (46m x 38m, Height 6.2m)
        // Level 0: 4 Massive Rock Pillars forming a Cruciform Underpass Cavern at Ground Level
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                m.AddBox({ sx * 14.0f, 2.8f, sz * 11.0f }, { 16.0f, 5.6f, 14.0f }, mesaRock, MAT_SANDSTONE);
            }
        }
        // Level 1: Interior Cavern Balcony Ledge (y = 3.0m)
        m.AddBox({ -7.0f, 2.9f, 0.0f }, { 4.0f, 0.3f, 16.0f }, canyonDark, MAT_SANDSTONE);
        m.AddBox({  7.0f, 2.9f, 0.0f }, { 4.0f, 0.3f, 16.0f }, canyonDark, MAT_SANDSTONE);
        m.AddRampStairs({ -14.0f, 0.0f, 0.0f }, { -7.0f, 3.0f, 0.0f }, 2.4f, 8, canyonDark, MAT_SANDSTONE);
        m.AddRampStairs({  14.0f, 0.0f, 0.0f }, {  7.0f, 3.0f, 0.0f }, 2.4f, 8, canyonDark, MAT_SANDSTONE);

        // Level 2: Mesa Summit Plateau (y = 6.2m, 44m x 36m)
        m.AddBox({ 0.0f, 6.2f, 0.0f }, { 44.0f, 0.45f, 36.0f }, mesaRock, MAT_SANDSTONE);
        // Summit Parapet Rocks & Sniper Bluffs
        m.AddBox({ 0.0f, 7.0f, -17.5f }, { 28.0f, 1.2f, 1.0f }, canyonDark, MAT_SANDSTONE);
        m.AddBox({ 0.0f, 7.0f,  17.5f }, { 28.0f, 1.2f, 1.0f }, canyonDark, MAT_SANDSTONE);
        // Decommissioned Radar Dome Shack on Summit
        m.AddBox({ 0.0f, 8.2f, 0.0f }, { 8.5f, 3.5f, 8.5f }, outpostTan, MAT_CONCRETE);
        m.AddBox({ 0.0f, 10.6f, 0.0f }, { 6.0f, 1.4f, 6.0f }, buggyOlive, MAT_SHEET_METAL);
        // 4 Carved Rock Ramps up to the Mesa Summit (from North, South, West, East)
        m.AddRampStairs({ -30.0f, 0.0f, -11.0f }, { -21.0f, 6.2f, -11.0f }, 3.6f, 14, canyonDark, MAT_SANDSTONE);
        m.AddRampStairs({  30.0f, 0.0f,  11.0f }, {  21.0f, 6.2f,  11.0f }, 3.6f, 14, canyonDark, MAT_SANDSTONE);
        m.AddRampStairs({ 0.0f, 0.0f, -26.0f }, { 0.0f, 6.2f, -17.0f }, 3.6f, 14, canyonDark, MAT_SANDSTONE);
        m.AddRampStairs({ 0.0f, 0.0f,  26.0f }, { 0.0f, 6.2f,  17.0f }, 3.6f, 14, canyonDark, MAT_SANDSTONE);

        // 3. NORTH & SOUTH CANYON PASS CORRIDORS (z = +/-46m to +/-60m)
        for (int sideZ = -1; sideZ <= 1; sideZ += 2) {
            float cz = sideZ * 46.0f;
            // High ridge cliff barrier
            m.AddBox({ -34.0f, 4.0f, cz }, { 46.0f, 8.0f, 5.0f }, canyonDark, MAT_CONCRETE);
            m.AddBox({  34.0f, 4.0f, cz }, { 46.0f, 8.0f, 5.0f }, canyonDark, MAT_CONCRETE);
            // Canyon pass boulders
            m.AddBox({ -16.0f, 1.2f, sideZ * 54.0f }, { 3.5f, 2.4f, 3.5f }, mesaRock, MAT_SANDSTONE);
            m.AddBox({  16.0f, 1.2f, sideZ * 54.0f }, { 3.5f, 2.4f, 3.5f }, mesaRock, MAT_SANDSTONE);
        }

        // 4. FORWARD OPERATING BASES (West FOB at x = -72m, East FOB at x = +72m)
        for (int sideX = -1; sideX <= 1; sideX += 2) {
            float fx = sideX * 72.0f;
            m.AddBuilding({ fx, 0.0f, 0.0f }, 16.0f, 22.0f, 4.2f, outpostTan, MAT_SANDSTONE, true);
            m.AddBox({ fx, 1.2f, -18.0f }, { 5.5f, 2.4f, 2.6f }, buggyOlive, MAT_SHEET_METAL); // Armored truck
            m.AddCoverCluster({ fx, 0.0f, 18.0f }, outpostTan, crateWood, MAT_SANDSTONE, MAT_WOOD_CRATE);
        }

        // 4 Mid-Map Outpost Bunkers (x = +/-38m, z = +/-24m)
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                m.AddBuilding({ sx * 42.0f, 0.0f, sz * 24.0f }, 12.0f, 12.0f, 3.8f, outpostTan, MAT_SANDSTONE, false);
                m.AddBox({ sx * 26.0f, 0.9f, sz * 8.0f }, { 4.5f, 1.8f, 2.2f }, buggyOlive, MAT_SHEET_METAL);
            }
        }

        m.phantomSpawns = { { -90, 1, -14 }, { -90, 1, -4 }, { -90, 1, 4 }, { -90, 1, 14 } };
        m.ghostSpawns   = { {  90, 1, -14 }, {  90, 1, -4 }, {  90, 1, 4 }, {  90, 1, 14 } };
        m.waypoints.insert(m.waypoints.end(), {
            // Mesa Cavern (Level 0) & Mesa Summit (Level 2)
            {   0.0f, 0.25f,   0.0f }, {   0.0f, 6.45f,   0.0f },
            { -14.0f, 0.25f,   0.0f }, {  14.0f, 0.25f,   0.0f },
            {   0.0f, 0.25f, -14.0f }, {   0.0f, 0.25f,  14.0f },
            { -7.0f, 3.25f,    0.0f }, {   7.0f, 3.25f,   0.0f }, // Cavern Ledges
            { -20.0f, 6.45f, -11.0f }, {  20.0f, 6.45f,  11.0f },
            {   0.0f, 6.45f, -16.0f }, {   0.0f, 6.45f,  16.0f },
            // Canyon Passes (North & South)
            { -40.0f, 0.25f, -54.0f }, {   0.0f, 0.25f, -54.0f }, {  40.0f, 0.25f, -54.0f },
            { -40.0f, 0.25f,  54.0f }, {   0.0f, 0.25f,  54.0f }, {  40.0f, 0.25f,  54.0f },
            // West & East FOBs
            { -72.0f, 0.25f,   0.0f }, { -72.0f, 4.45f,   0.0f },
            {  72.0f, 0.25f,   0.0f }, {  72.0f, 4.45f,   0.0f },
            // Mid-Map Bunkers & Flats
            { -42.0f, 0.25f, -24.0f }, {  42.0f, 0.25f, -24.0f },
            { -42.0f, 0.25f,  24.0f }, {  42.0f, 0.25f,  24.0f },
            { -88.0f, 0.25f,   0.0f }, {  88.0f, 0.25f,   0.0f }
        });
        m.flarePoints = {
            { "A", { -68.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }, // West Outpost FOB
            { "B", {   0.0f, 6.2f, 0.0f }, 6.0f, -1, 0.0f }, // Central Mesa Summit
            { "C", {  68.0f, 0.5f, 0.0f }, 5.5f, -1, 0.0f }  // East Outpost FOB
        };
        m.hillPoint = { "HILL", { 0.0f, 0.5f, 0.0f }, 6.5f, -1, 0.0f }; // Mesa Cavern Underpass

    } else { // MAP_MONASTERY_RIDGE
        m.name = "MONASTERY RIDGE";
        m.subtitle = "Expanded 180m Mountain Ridge - 4 Terraces, Catacomb Crypt & Bell Tower Spire";
        m.skyColor = { 158, 188, 224, 255 };
        m.groundColor = { 88, 102, 82, 255 };

        Color cliffStone  = { 126, 122, 114, 255 };
        Color plasterWall = { 216, 206, 188, 255 };
        Color roofTerra   = { 172, 78, 52, 255 };
        Color ruinStone   = { 148, 142, 132, 255 };
        Color darkWood    = { 115, 82, 54, 255 };

        // 1. Massive 180m x 130m Mountain Perimeter
        float W = 180.0f;
        float D = 130.0f;
        m.AddBox({ 0, 7.0f, -D * 0.5f }, { W, 14.0f, 1.8f }, cliffStone, MAT_CONCRETE);
        m.AddBox({ 0, 7.0f,  D * 0.5f }, { W, 14.0f, 1.8f }, cliffStone, MAT_CONCRETE);
        m.AddBox({ -W * 0.5f, 7.0f, 0 }, { 1.8f, 14.0f, D }, cliffStone, MAT_CONCRETE);
        m.AddBox({  W * 0.5f, 7.0f, 0 }, { 1.8f, 14.0f, D }, cliffStone, MAT_CONCRETE);

        // 2. 4 VERTICAL ELEVATION TERRACES
        // Terrace 1: Lower Outer Bastion (y = 3.4m, spans x in [-50..+40], z in [0..+36])
        m.AddBox({ -5.0f, 1.7f, 18.0f }, { 96.0f, 3.4f, 38.0f }, cliffStone, MAT_CONCRETE);
        // Terrace 2: Monastery Cloister Courtyard (y = 6.8m, spans x in [-20..+70], z in [-28..+10])
        m.AddBox({ 25.0f, 3.4f, -9.0f }, { 94.0f, 6.8f, 38.0f }, cliffStone, MAT_CONCRETE);
        // Terrace 3: Cathedral Sanctuary Ridge (y = 10.2m, spans x in [+20..+82], z in [-60..-28])
        m.AddBox({ 52.0f, 5.1f, -44.0f }, { 68.0f, 10.2f, 34.0f }, cliffStone, MAT_CONCRETE);

        // 3. ASCENT ROUTE A: Grand Switchback Carriage Road
        // Switchback 1: Valley SW -> Terrace 1
        m.AddRampStairs({ -64.0f, 0.0f, 28.0f }, { -50.0f, 3.4f, 28.0f }, 5.5f, 12, ruinStone, MAT_SANDSTONE);
        // Switchback 2: Terrace 1 -> Terrace 2
        m.AddRampStairs({ -24.0f, 3.4f, 6.0f }, { -14.0f, 6.8f, -4.0f }, 5.5f, 12, ruinStone, MAT_SANDSTONE);
        // Switchback 3: Terrace 2 -> Terrace 3
        m.AddRampStairs({  16.0f, 6.8f, -22.0f }, { 26.0f, 10.2f, -32.0f }, 5.5f, 12, ruinStone, MAT_SANDSTONE);

        // 4. ASCENT ROUTE B: Secret Mountain Catacomb Crypt Tunnel (bypasses Gatehouse)
        // Ascends inside the mountain from Valley (y = 0.5m) to Cloister (y = 6.8m)
        m.AddTunnelPassage({ -44.0f, 1.8f, 36.0f }, { -4.0f, 4.8f, 10.0f }, 3.4f, 2.8f, ruinStone, MAT_CONCRETE);
        m.AddRampStairs({ -52.0f, 0.0f, 36.0f }, { -44.0f, 1.8f, 36.0f }, 2.8f, 6, ruinStone, MAT_CONCRETE);
        m.AddRampStairs({  -4.0f, 4.8f, 10.0f }, {   4.0f, 6.8f, 10.0f }, 2.8f, 6, ruinStone, MAT_CONCRETE);

        // 5. ASCENT ROUTE C: Exposed Southern Goat Trail (Sheer cliffside flank)
        m.AddRampStairs({  46.0f, 0.0f, 44.0f }, {  46.0f, 3.4f, 32.0f }, 3.6f, 10, ruinStone, MAT_SANDSTONE);
        m.AddRampStairs({  46.0f, 3.4f, 24.0f }, {  46.0f, 6.8f,  6.0f }, 3.6f, 10, ruinStone, MAT_SANDSTONE);
        m.AddRampStairs({  68.0f, 6.8f, -6.0f }, {  68.0f, 10.2f,-24.0f }, 3.6f, 10, ruinStone, MAT_SANDSTONE);

        // 6. MONASTERY ARCHITECTURE & FORTIFICATIONS
        // Lower Gatehouse Fortress on Terrace 1 (Flag A)
        m.AddBuilding({ -34.0f, 3.4f, 18.0f }, 24.0f, 14.0f, 4.2f, plasterWall, MAT_SANDSTONE, true);
        // Monastery Cloister Courtyard on Terrace 2 (Flag B)
        m.AddBuilding({ 0.0f, 6.8f, 0.0f }, 26.0f, 20.0f, 4.2f, plasterWall, MAT_SANDSTONE, false);
        m.AddBox({ 0.0f, 7.3f, 0.0f }, { 6.0f, 0.8f, 6.0f }, ruinStone, MAT_CONCRETE); // Central cloister fountain
        // Cathedral Basilica & Iconic Bell Tower Citadel on Terrace 3 (Flag C)
        m.AddBuilding({ 44.0f, 10.2f, -26.0f }, 32.0f, 20.0f, 5.2f, plasterWall, MAT_SANDSTONE, true);
        // Colossal 16m Bell Tower Spire
        m.AddBox({ 54.0f, 14.0f, -26.0f }, { 6.5f, 8.0f, 6.5f }, plasterWall, MAT_SANDSTONE);
        m.AddBox({ 54.0f, 18.2f, -26.0f }, { 7.8f, 1.4f, 7.8f }, roofTerra, MAT_CONCRETE);
        m.AddRampStairs({ 48.0f, 10.2f, -26.0f }, { 54.0f, 18.2f, -26.0f }, 2.0f, 16, darkWood, MAT_WOOD_CRATE);

        // Roadside Retaining Walls & Ruin Cover Pieces
        m.AddBox({ -38.0f, 4.5f, 28.0f }, { 18.0f, 2.2f, 1.2f }, ruinStone, MAT_SANDSTONE);
        m.AddBox({  -6.0f, 7.9f, -10.0f }, { 18.0f, 2.2f, 1.2f }, ruinStone, MAT_SANDSTONE);
        m.AddBox({  32.0f, 11.3f,-32.0f }, { 18.0f, 2.2f, 1.2f }, ruinStone, MAT_SANDSTONE);

        m.phantomSpawns = { { -78, 1.0f, 52 }, { -72, 1.0f, 54 }, { -66, 1.0f, 52 }, { -78, 1.0f, 58 } };
        m.ghostSpawns   = { {  76, 11.2f, -50 }, {  70, 11.2f, -46 }, {  78, 11.2f, -42 }, {  72, 11.2f, -54 } };
        m.waypoints.insert(m.waypoints.end(), {
            // Valley & Olive Grove (y = 0.25f)
            { -70.0f, 0.25f, 46.0f }, { -50.0f, 0.25f, 46.0f }, { 0.0f, 0.25f, 48.0f }, { 46.0f, 0.25f, 46.0f },
            // Crypt Catacomb Tunnel
            { -44.0f, 2.05f, 36.0f }, { -24.0f, 3.55f, 22.0f }, { -4.0f, 5.05f, 10.0f },
            // Terrace 1: Lower Gatehouse (y = 3.65f)
            { -50.0f, 3.65f, 28.0f }, { -34.0f, 3.65f, 18.0f }, { -14.0f, 3.65f, 18.0f },
            { -34.0f, 7.85f, 18.0f }, // Gatehouse Roof
            // Terrace 2: Cloister Courtyard (y = 7.05f)
            { -14.0f, 7.05f, -4.0f }, {   0.0f, 7.05f,  0.0f }, { 16.0f, 7.05f,  0.0f },
            {   0.0f, 7.05f, 10.0f }, {  16.0f, 7.05f, -22.0f },
            // Terrace 3: Cathedral Sanctuary & Bell Tower (y = 10.45f)
            {  26.0f, 10.45f, -32.0f }, { 44.0f, 10.45f, -26.0f }, { 64.0f, 10.45f, -26.0f },
            {  54.0f, 18.45f, -26.0f }, // Bell Tower Spire Apex
            // Goat Trail Flanks
            {  46.0f, 3.65f,  28.0f }, { 46.0f, 7.05f, 12.0f }, { 68.0f, 10.45f, -12.0f },
            // Spawns
            { -76.0f, 0.25f, 52.0f }, { 76.0f, 10.45f, -48.0f }
        });
        m.flarePoints = {
            { "A", { -34.0f, 3.6f,  18.0f }, 5.5f, -1, 0.0f }, // Lower Gatehouse Fortress
            { "B", {   0.0f, 7.0f,   0.0f }, 6.0f, -1, 0.0f }, // Cloister Courtyard
            { "C", {  44.0f, 10.4f, -26.0f }, 5.5f, -1, 0.0f }  // Cathedral Sanctuary
        };
        m.hillPoint = { "HILL", { 0.0f, 7.0f, 0.0f }, 6.5f, -1, 0.0f };
    }

    m.BuildNavGraph();
    return m;
}

inline const char* GetMapShortName(MapId id) {
    switch (id) {
        case MAP_DESERT_STORM:    return "DESERT STORM";
        case MAP_METRO:           return "METRO";
        case MAP_WAREHOUSE:       return "WAREHOUSE";
        case MAP_FOUNDRY:         return "FOUNDRY";
        case MAP_DOCKSIDE_CUTOFF: return "DOCKSIDE";
        case MAP_HIGHRISE_ATLAS:  return "HIGHRISE";
        case MAP_SALT_FLATS:      return "SALT FLATS";
        case MAP_MONASTERY_RIDGE: return "MONASTERY";
        default:                  return "UNKNOWN";
    }
}

