//-----------------------------------------------------------------------------
// WorldObjectManager.cpp
//-----------------------------------------------------------------------------

#include "WorldObjectManager.h"
#include "../quest/QuestManager.h"
#include "../core/Logger.h"
#include <cstring>
#include <cmath>

//=============================================================================
// OBJECT DEFINITIONS
//
// Object 0 — BRIDGE (Town)
//   Tiles (10,21) and (11,21) are TILE_WALL (16) in the south perimeter.
//   Repair opens them to TILE_DIRT (1), creating a second south exit.
//   Interact point: center of the two tiles = pixel (10.5*16, 21*16+8) ≈ (168, 344)
//   broken_tile = 0xFF → no override needed in INACTIVE (ZoneData already solid).
//   repaired_tile = TILE_DIRT (1) → applied on repair.
//
// Object 1 — ROPE LADDER (Forest)
//   Tiles (0,3),(0,4),(0,5) are TILE_TREE (17) on the left forest wall.
//   Repair opens them to TILE_FOREST_FLOOR (2), giving a west-side shortcut.
//   Interact point: center of tile (0,4) = pixel (8, 72).
//   broken_tile = 0xFF → ZoneData already has TILE_TREE there.
//   repaired_tile = TILE_FOREST_FLOOR (2).
//
// Object 2 — FALLEN TREE (Forest)
//   Tiles (22,11) and (22,12) are TILE_FOREST_FLOOR (2) in ZoneData —
//   passable. INACTIVE state overrides them with TILE_TREE (17) to create
//   the blockage. Repair removes the override → tiles revert to floor (2).
//   Interact point: center of tile (22,11) = pixel (360, 184).
//   broken_tile = TILE_TREE (17) → applied in INACTIVE state.
//   repaired_tile = TILE_FOREST_FLOOR (2) → applied on repair.
//=============================================================================

static void defineObjects(WorldObject* obj) {
    // --- Object 0: Bridge ---
    obj[0].id            = 0;
    obj[0].type          = WorldObjectType::BRIDGE;
    obj[0].state         = WorldObjectState::INACTIVE;
    obj[0].zone          = ZoneID::TOWN;
    obj[0].interact_x    = 10 * TILE_SIZE + 8.0f;   // 168
    obj[0].interact_y    = 21 * TILE_SIZE + 8.0f;   // 344
    obj[0].cost          = { /*wood=*/5, /*rope=*/0 };
    obj[0].label         = "Bridge";
    obj[0].repair_message = "Bridge repaired.";
    obj[0].clear_message  = "Bridge repaired.";

    obj[0].tiles[0] = { 10, 21, 0xFF, TILE_DIRT };  // 0xFF = no INACTIVE override
    obj[0].tiles[1] = { 11, 21, 0xFF, TILE_DIRT };
    obj[0].tile_count = 2;
    obj[0].active    = true;

    // --- Object 1: Rope Ladder ---
    obj[1].id            = 1;
    obj[1].type          = WorldObjectType::LADDER;
    obj[1].state         = WorldObjectState::INACTIVE;
    obj[1].zone          = ZoneID::FOREST;
    obj[1].interact_x    = 0 * TILE_SIZE + 8.0f;    // 8
    obj[1].interact_y    = 4 * TILE_SIZE + 8.0f;    // 72
    obj[1].cost          = { /*wood=*/0, /*rope=*/3 };
    obj[1].label         = "Rope Ladder";
    obj[1].repair_message = "Ladder lowered.";
    obj[1].clear_message  = "Ladder lowered.";

    obj[1].tiles[0] = { 0, 3, 0xFF, TILE_FOREST_FLOOR };
    obj[1].tiles[1] = { 0, 4, 0xFF, TILE_FOREST_FLOOR };
    obj[1].tiles[2] = { 0, 5, 0xFF, TILE_FOREST_FLOOR };
    obj[1].tile_count = 3;
    obj[1].active    = true;

    // --- Object 2: Fallen Tree ---
    obj[2].id            = 2;
    obj[2].type          = WorldObjectType::OBSTACLE;
    obj[2].state         = WorldObjectState::INACTIVE;
    obj[2].zone          = ZoneID::FOREST;
    obj[2].interact_x    = 22 * TILE_SIZE + 8.0f;   // 360
    obj[2].interact_y    = 11 * TILE_SIZE + 8.0f;   // 184
    obj[2].cost          = { /*wood=*/8, /*rope=*/0 };
    obj[2].label         = "Fallen Tree";
    obj[2].repair_message = "Obstacle cleared.";
    obj[2].clear_message  = "Obstacle cleared.";

    // Fallen tree ADDS a blockage (broken_tile = TILE_TREE, applied in INACTIVE)
    obj[2].tiles[0] = { 22, 11, TILE_TREE, TILE_FOREST_FLOOR };
    obj[2].tiles[1] = { 22, 12, TILE_TREE, TILE_FOREST_FLOOR };
    obj[2].tile_count = 2;
    obj[2].active    = true;
}

//=============================================================================

WorldObjectManager::WorldObjectManager()
    : m_lastMessage(nullptr)
    , m_messageTimer(0.0f)
{
    memset(m_objects, 0, sizeof(m_objects));
}

void WorldObjectManager::init(TileMap& map) {
    defineObjects(m_objects);

    // Apply overrides for the starting zone (Town)
    onZoneLoaded(ZoneID::TOWN, map);
    LOG("WorldObjectManager: initialized 3 objects");
}

void WorldObjectManager::applyOverrides(const WorldObject& obj, TileMap& map) {
    for (int i = 0; i < obj.tile_count; i++) {
        const TileOverride& to = obj.tiles[i];

        if (obj.state == WorldObjectState::INACTIVE) {
            if (to.broken_tile != 0xFF) {
                map.setTileOverride(to.tx, to.ty, to.broken_tile);
            }
            // broken_tile == 0xFF → ZoneData base tile is already correct; no override
        } else {
            // REPAIRED — apply passable tile
            map.setTileOverride(to.tx, to.ty, to.repaired_tile);
        }
    }
}

void WorldObjectManager::onZoneLoaded(ZoneID zone, TileMap& map) {
    // On every zone load, clear all overrides then re-apply for this zone's objects.
    // (TileMap was just freshly loaded from ZoneData — overrides were wiped.)
    map.clearOverrides();

    for (int i = 0; i < MAX_WORLD_OBJECTS; i++) {
        const WorldObject& obj = m_objects[i];
        if (!obj.active)        continue;
        if (obj.zone != zone)   continue;
        applyOverrides(obj, map);
    }
}

InteractResult WorldObjectManager::tryInteract(float playerX, float playerY,
                                                bool aPressed, TileMap& map,
                                                PlayerState& ps,
                                                QuestManager* questMgr) {
    if (!aPressed) return InteractResult::NONE;

    // Find closest object in range in any zone (caller filters by current zone
    // via interact_x/y being inside the loaded map).
    float  closest = OBJECT_INTERACT_RANGE;
    int    bestIdx = -1;

    for (int i = 0; i < MAX_WORLD_OBJECTS; i++) {
        const WorldObject& obj = m_objects[i];
        if (!obj.active) continue;

        float dx   = playerX - obj.interact_x;
        float dy   = playerY - obj.interact_y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < closest) {
            closest = dist;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) return InteractResult::NONE;

    WorldObject& obj = m_objects[bestIdx];

    if (obj.state == WorldObjectState::REPAIRED) {
        m_lastMessage  = obj.repair_message;
        m_messageTimer = MESSAGE_DURATION;
        return InteractResult::ALREADY_REPAIRED;
    }

    // Check resources
    bool canAfford = (ps.wood >= obj.cost.wood) && (ps.rope >= obj.cost.rope);
    if (!canAfford) {
        m_lastMessage  = "Need more materials.";
        m_messageTimer = MESSAGE_DURATION;
        LOG("WorldObject %d: cannot afford (need wood=%d rope=%d, have wood=%d rope=%d)",
            obj.id, obj.cost.wood, obj.cost.rope, ps.wood, ps.rope);
        return InteractResult::NEED_RESOURCES;
    }

    repair(obj, map, ps, questMgr);
    return InteractResult::REPAIRED;
}

void WorldObjectManager::repair(WorldObject& obj, TileMap& map,
                                 PlayerState& ps, QuestManager* questMgr) {
    // Consume resources
    ps.wood -= obj.cost.wood;
    ps.rope -= obj.cost.rope;

    // Change state
    obj.state = WorldObjectState::REPAIRED;

    // Apply tile overrides
    applyOverrides(obj, map);

    // Message
    m_lastMessage  = obj.repair_message;
    m_messageTimer = MESSAGE_DURATION;

    LOG("WorldObject %d (%s) repaired. Wood remaining: %d, Rope remaining: %d",
        obj.id, obj.label, ps.wood, ps.rope);

    // Quest hook
    onWorldObjectRepaired(obj.id, questMgr);
}

void WorldObjectManager::onWorldObjectRepaired(u8 id, QuestManager* questMgr) {
    // Hook for future quest integration.
    // QuestManager does not use this yet — the signature is established here.
    (void)questMgr;
    LOG("WorldObject hook: onWorldObjectRepaired(%d)", id);
}

void WorldObjectManager::updateMessageTimer(float dt) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
        if (m_messageTimer < 0.0f) m_messageTimer = 0.0f;
    }
}

WorldObjectState WorldObjectManager::getState(u8 id) const {
    for (int i = 0; i < MAX_WORLD_OBJECTS; i++) {
        if (m_objects[i].active && m_objects[i].id == id)
            return m_objects[i].state;
    }
    return WorldObjectState::INACTIVE;
}

void WorldObjectManager::setState(u8 id, WorldObjectState s, TileMap& map) {
    for (int i = 0; i < MAX_WORLD_OBJECTS; i++) {
        if (m_objects[i].active && m_objects[i].id == id) {
            m_objects[i].state = s;
            applyOverrides(m_objects[i], map);
            return;
        }
    }
}

void WorldObjectManager::setStatesFromSave(const u8* states, int count, TileMap& map) {
    // states[] is slot-indexed (0..MAX_WORLD_OBJECTS-1), same order as m_objects.
    //
    // IMPORTANT: this function restores STATE ONLY. It deliberately does not
    // touch tile overrides. Applying overrides here used to mean every active
    // object in EVERY zone got its overrides written into whichever map
    // happens to be loaded at the time — so a Forest object's override could
    // silently land on a Town tile that shares the same (tx,ty) coordinate.
    //
    // The correct sequence, and the only one the caller should use, is:
    //   1. zones.loadZone(savedZone)            — load the correct TileMap
    //   2. worldObjects.setStatesFromSave(...)  — restore state values only
    //   3. worldObjects.onZoneLoaded(savedZone) — apply THIS zone's overrides
    // (map parameter is unused now but kept so the call site doesn't need to
    // change shape; remove once no caller depends on the old signature.)
    (void)map;
    int n = count < MAX_WORLD_OBJECTS ? count : MAX_WORLD_OBJECTS;
    for (int i = 0; i < n; i++) {
        if (!m_objects[i].active) continue;
        m_objects[i].state = static_cast<WorldObjectState>(states[i]);
    }
}
