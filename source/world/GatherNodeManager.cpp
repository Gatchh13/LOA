//-----------------------------------------------------------------------------
// GatherNodeManager.cpp
//-----------------------------------------------------------------------------

#include "GatherNodeManager.h"
#include "../core/Logger.h"
#include <cstring>
#include <cstdio>
#include <cmath>

//=============================================================================
// NODE DEFINITIONS
//
// All four nodes live in Forest for Milestone 6 — Town is a settlement with
// no wild terrain to harvest from, and Forest is the only zone with the
// open floor tiles to spare. Positions were checked against
// s_forestTiles in ZoneData.h to land on plain TILE_FOREST_FLOOR (2),
// clear of the rope-ladder WorldObject (tiles 0,3–0,5), the fallen-tree
// WorldObject (tiles 22,11–22,12), the spawn points, and the north-south
// dirt path (columns 13–14).
//
// Node 0 — Wood, tile (5,5)   — pixel (88, 88)
// Node 1 — Wood, tile (24,6)  — pixel (392, 104)
// Node 2 — Rope, tile (8,16)  — pixel (136, 264)
// Node 3 — Rope, tile (20,18) — pixel (328, 296)
//=============================================================================

static void defineNodes(GatherNode* nodes) {
    nodes[0].id       = 0;
    nodes[0].zone     = ZoneID::FOREST;
    nodes[0].pos_x    = 5 * TILE_SIZE + 8.0f;
    nodes[0].pos_y    = 5 * TILE_SIZE + 8.0f;
    nodes[0].resource = GatherResource::WOOD;
    nodes[0].amount   = 2;
    nodes[0].cooldown = 0.0f;
    nodes[0].active   = true;

    nodes[1].id       = 1;
    nodes[1].zone     = ZoneID::FOREST;
    nodes[1].pos_x    = 24 * TILE_SIZE + 8.0f;
    nodes[1].pos_y    = 6 * TILE_SIZE + 8.0f;
    nodes[1].resource = GatherResource::WOOD;
    nodes[1].amount   = 2;
    nodes[1].cooldown = 0.0f;
    nodes[1].active   = true;

    nodes[2].id       = 2;
    nodes[2].zone     = ZoneID::FOREST;
    nodes[2].pos_x    = 8 * TILE_SIZE + 8.0f;
    nodes[2].pos_y    = 16 * TILE_SIZE + 8.0f;
    nodes[2].resource = GatherResource::ROPE;
    nodes[2].amount   = 1;
    nodes[2].cooldown = 0.0f;
    nodes[2].active   = true;

    nodes[3].id       = 3;
    nodes[3].zone     = ZoneID::FOREST;
    nodes[3].pos_x    = 20 * TILE_SIZE + 8.0f;
    nodes[3].pos_y    = 18 * TILE_SIZE + 8.0f;
    nodes[3].resource = GatherResource::ROPE;
    nodes[3].amount   = 1;
    nodes[3].cooldown = 0.0f;
    nodes[3].active   = true;
}

//=============================================================================

GatherNodeManager::GatherNodeManager()
    : m_lastMessage(nullptr)
    , m_messageTimer(0.0f)
{
    memset(m_nodes, 0, sizeof(m_nodes));
    m_messageBuf[0] = '\0';
}

void GatherNodeManager::init() {
    defineNodes(m_nodes);
    LOG("GatherNodeManager: initialized %d nodes", MAX_GATHER_NODES);
}

void GatherNodeManager::update(float dt) {
    for (int i = 0; i < MAX_GATHER_NODES; i++) {
        if (!m_nodes[i].active) continue;
        if (m_nodes[i].cooldown > 0.0f) {
            m_nodes[i].cooldown -= dt;
            if (m_nodes[i].cooldown < 0.0f) m_nodes[i].cooldown = 0.0f;
        }
    }
}

GatherResult GatherNodeManager::tryHarvest(float playerX, float playerY,
                                           ZoneID currentZone, bool aPressed,
                                           PlayerState& playerState) {
    if (!aPressed) return GatherResult::NONE;

    // Find the closest in-range node in the current zone (same
    // closest-wins pattern as WorldObjectManager::tryInteract).
    float closest = GATHER_INTERACT_RANGE;
    int   bestIdx = -1;

    for (int i = 0; i < MAX_GATHER_NODES; i++) {
        const GatherNode& node = m_nodes[i];
        if (!node.active)            continue;
        if (node.zone != currentZone) continue;

        float dx   = playerX - node.pos_x;
        float dy   = playerY - node.pos_y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < closest) {
            closest = dist;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) return GatherResult::NONE;

    GatherNode& node = m_nodes[bestIdx];

    if (node.cooldown > 0.0f) {
        m_lastMessage  = "Already harvested. Check back later.";
        m_messageTimer = MESSAGE_DURATION;
        return GatherResult::ON_COOLDOWN;
    }

    // Grant resource
    const char* resourceName = "Wood";
    if (node.resource == GatherResource::WOOD) {
        playerState.addWood(node.amount);
        resourceName = "Wood";
    } else {
        playerState.addRope(node.amount);
        resourceName = "Rope";
    }

    node.cooldown = GATHER_COOLDOWN_SECONDS;

    snprintf(m_messageBuf, sizeof(m_messageBuf), "+%u %s", node.amount, resourceName);
    m_lastMessage  = m_messageBuf;
    m_messageTimer = MESSAGE_DURATION;

    LOG("GatherNode %d: harvested %u %s. Wood=%d Rope=%d",
        node.id, node.amount, resourceName, playerState.wood, playerState.rope);

    return GatherResult::HARVESTED;
}

void GatherNodeManager::updateMessageTimer(float dt) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
        if (m_messageTimer < 0.0f) m_messageTimer = 0.0f;
    }
}
