#pragma once

//-----------------------------------------------------------------------------
// GatherNodeManager.h  (Milestone 6 — Foundation of Feel)
//
// Owns all GatherNode instances. Handles interaction, cooldown ticking,
// and granting resources to PlayerState on harvest.
//
// Lifecycle:
//   1. init()       — populate nodes (once, at game start)
//   2. update()     — tick cooldowns every frame with real dt
//   3. tryHarvest() — called when A is pressed, returns GatherResult
//   4. getNodes()   — used by Renderer for placeholder visuals
//
// Not save-persisted in Milestone 6: node cooldown state resets on load.
// This is a deliberate scope cut, not an oversight — see the comment in
// tryHarvest() and SaveData.h's reserved_inventory block, which is where
// per-node state would eventually live if this needs to be persisted later.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "GatherNode.h"
#include "../player/PlayerState.h"

class GatherNodeManager {
public:
    GatherNodeManager();

    // Populate all nodes. Call once at game start (mirrors
    // WorldObjectManager::init(), but gather nodes don't touch TileMap —
    // they're drawn as overlays, not baked into tile overrides).
    void init();

    // Tick all active cooldowns down. Call once per frame with real dt.
    void update(float dt);

    // Check if the player is within range of any node in the current zone
    // and A was pressed. Returns the result and grants resources on a
    // successful harvest.
    // playerX/Y: player center in pixels.
    // currentZone: only nodes in this zone are considered.
    // aPressed: true only on the frame A is pressed.
    // playerState: resources granted on harvest.
    GatherResult tryHarvest(float playerX, float playerY, ZoneID currentZone,
                            bool aPressed, PlayerState& playerState);

    // The message from the last tryHarvest call (for HUD display).
    // nullptr if no message. Valid until the next tryHarvest call.
    const char* getLastMessage() const { return m_lastMessage; }
    bool        hasMessage() const { return m_messageTimer > 0.0f; }
    void        updateMessageTimer(float dt);

    // Access nodes for rendering.
    const GatherNode* getNodes()     const { return m_nodes; }
    int                getNodeCount() const { return MAX_GATHER_NODES; }

    static constexpr float MESSAGE_DURATION = 2.0f; // seconds to show message

private:
    GatherNode  m_nodes[MAX_GATHER_NODES];
    const char* m_lastMessage;
    float       m_messageTimer;

    // Small fixed buffer for "+N Wood" style messages — needs to outlive
    // the call that produces it (m_lastMessage just points at this), so
    // it's a member, not a local.
    char m_messageBuf[32];
};
