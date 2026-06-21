//-----------------------------------------------------------------------------
// NPCManager.cpp
//-----------------------------------------------------------------------------

#include "NPCManager.h"
#include "../core/Logger.h"
#include <cstring>
#include <cmath>
#include <algorithm>

//=============================================================================
// NPC DATA DEFINITIONS
// Town has 28×22 tiles. Tile positions below reference the town map.
// Tile (tx,ty) → pixel center: (tx*16+8, ty*16+8)
//
// Town layout reference (from ZoneData.h town map):
//   Forge area    : tiles ~(4–6, 2–4)    (top-left building cluster)
//   Market plaza  : tiles ~(9–17, 5–11)  (central fenced square)
//   Tavern        : tiles ~(3–6, 9–11)   (left fenced yard)
//   Guard post    : tiles ~(1, 1)        (top-left open area)
//   South road    : tiles ~(13, 18–20)
//=============================================================================

static void initBlacksmith(NPC& n) {
    strncpy(n.name,     "Aldric",                         MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "The forge never rests. Neither do I.", MAX_DIALOGUE_LEN - 1);
    n.home_zone = ZoneID::TOWN;

    // Start position: forge area
    n.pos_x = 4 * TILE_SIZE + 1.0f;
    n.pos_y = 2 * TILE_SIZE + 1.0f;

    n.schedule[0] = {  6 * 60, 4,  2, "Wake" };    // 06:00 — at forge (early start)
    n.schedule[1] = {  8 * 60, 4,  3, "Forge" };   // 08:00 — working forge
    n.schedule[2] = { 12 * 60, 5,  9, "Tavern" };  // 12:00 — lunch at tavern yard
    n.schedule[3] = { 13 * 60, 4,  3, "Forge" };   // 13:00 — back to forge
    n.schedule[4] = { 18 * 60, 4,  2, "Wind down"};// 18:00 — cleaning up
    n.schedule[5] = { 20 * 60, 4,  9, "Home" };    // 20:00 — home (same fenced yard)
    n.schedule[6] = {  0 * 60, 4,  9, "Sleep" };   // 00:00 — sleep
    n.schedule_count = 7;
    n.active       = true;
}

static void initMerchant(NPC& n) {
    strncpy(n.name,     "Mira",                            MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "Finest goods this side of the mountains!", MAX_DIALOGUE_LEN - 1);
    n.home_zone = ZoneID::TOWN;

    // Start position: market plaza entrance
    n.pos_x = 10 * TILE_SIZE + 1.0f;
    n.pos_y =  6 * TILE_SIZE + 1.0f;

    n.schedule[0] = {  7 * 60, 10,  6, "Open stall" };  // 07:00 — set up market
    n.schedule[1] = {  8 * 60, 12,  8, "Market" };      // 08:00 — working market center
    n.schedule[2] = { 13 * 60, 10,  9, "Lunch break" }; // 13:00 — edge of plaza
    n.schedule[3] = { 14 * 60, 14,  7, "Afternoon" };   // 14:00 — other side of market
    n.schedule[4] = { 18 * 60, 10,  6, "Close stall" }; // 18:00 — packing up
    n.schedule[5] = { 19 * 60, 21,  2, "Home" };        // 19:00 — home (top-right buildings)
    n.schedule[6] = { 22 * 60, 21,  3, "Sleep" };       // 22:00 — sleep
    n.schedule[7] = {  0 * 60, 21,  3, "Night" };       // midnight — stay home
    n.schedule_count = 8;
    n.active       = true;
}

static void initGuard(NPC& n) {
    strncpy(n.name,     "Brennan",                              MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "Move along. Nothing to see here.", MAX_DIALOGUE_LEN - 1);
    n.home_zone = ZoneID::TOWN;

    // Start position: northern entrance
    n.pos_x =  1 * TILE_SIZE + 1.0f;
    n.pos_y =  1 * TILE_SIZE + 1.0f;

    // Guard patrols: two routes alternating through the day
    // Route A: north perimeter
    // Route B: south road watch
    n.schedule[0] = {  0 * 60,  1,  1, "Patrol A1" }; // 00:00 — north-west
    n.schedule[1] = {  3 * 60, 13, 18, "Patrol A2" }; // 03:00 — south road
    n.schedule[2] = {  6 * 60,  1,  1, "Patrol A3" }; // 06:00 — back north
    n.schedule[3] = {  8 * 60, 26,  1, "Patrol B1" }; // 08:00 — north-east corner
    n.schedule[4] = { 12 * 60,  1, 20, "Patrol B2" }; // 12:00 — south-west
    n.schedule[5] = { 15 * 60, 13, 18, "Patrol B3" }; // 15:00 — south road
    n.schedule[6] = { 18 * 60, 26, 20, "Patrol B4" }; // 18:00 — south-east
    n.schedule[7] = { 21 * 60,  1,  1, "Night post"}; // 21:00 — return to post
    n.schedule_count = 8;
    n.active       = true;
}

//=============================================================================
// NPCManager
//=============================================================================

NPCManager::NPCManager()
    : m_activeDialogueIndex(-1)
{
    memset(m_npcs, 0, sizeof(m_npcs));
}

void NPCManager::init(int currentMinute) {
    // Clear all slots
    for (int i = 0; i < MAX_NPCS; i++) {
        m_npcs[i].active = false;
    }

    // Populate the three town NPCs
    initBlacksmith(m_npcs[0]);
    initMerchant  (m_npcs[1]);
    initGuard     (m_npcs[2]);

    // Snap each NPC to their scheduled position for the current time
    for (int i = 0; i < MAX_NPCS; i++) {
        if (!m_npcs[i].active) continue;
        snapToSchedule(m_npcs[i], currentMinute);
    }

    LOG("NPCManager: initialized 3 NPCs at time %d", currentMinute);
}

void NPCManager::evaluateSchedule(NPC& npc, int currentMinute) {
    if (npc.schedule_count == 0) return;

    // Find the entry with the highest start_minute <= currentMinute.
    // Schedule entries must be sorted ascending by start_minute.
    int best = 0;
    for (int i = 0; i < npc.schedule_count; i++) {
        if (npc.schedule[i].start_minute <= currentMinute) {
            best = i;
        }
    }

    // If no entry starts at or before now, use the last entry of the previous
    // day (the entry with the highest start_minute overall — i.e. the last one).
    // This handles midnight wrap: if it's 01:00 and first entry is 06:00,
    // we fall through to the last entry (e.g. 00:00 or 23:00 "sleep").
    if (npc.schedule[0].start_minute > currentMinute) {
        best = npc.schedule_count - 1;
    }

    if (npc.active_entry != best) {
        npc.active_entry = best;
        const ScheduleEntry& entry = npc.schedule[best];
        npc.target_x = entry.dest_tx * TILE_SIZE + 1.0f;
        npc.target_y = entry.dest_ty * TILE_SIZE + 1.0f;
        LOG("NPC %s -> %s (%d,%d)", npc.name, entry.label, entry.dest_tx, entry.dest_ty);
    }
}

void NPCManager::snapToSchedule(NPC& npc, int currentMinute) {
    npc.active_entry = -1; // force re-evaluation
    evaluateSchedule(npc, currentMinute);

    // Teleport directly to target
    npc.pos_x = npc.target_x;
    npc.pos_y = npc.target_y;
}

bool NPCManager::moveNPC(NPC& npc, const TileMap& map, float dt) {
    float dx = npc.target_x - npc.pos_x;
    float dy = npc.target_y - npc.pos_y;

    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= ARRIVAL_THRESHOLD) {
        return false; // already there
    }

    float step = NPC_SPEED * dt;

    // Move horizontally first, then vertically.
    // This gives simple L-shaped paths without pathfinding.
    if (fabsf(dx) > ARRIVAL_THRESHOLD) {
        float moveX = (dx > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dx));
        float newX  = npc.pos_x + moveX;

        // Simple tile collision for NPC
        int tx = static_cast<int>(newX + 8) / TILE_SIZE; // center of NPC
        int ty = static_cast<int>(npc.pos_y + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            npc.pos_x = newX;
        }
        // If blocked horizontally, try to continue below
    } else if (fabsf(dy) > ARRIVAL_THRESHOLD) {
        float moveY = (dy > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dy));
        float newY  = npc.pos_y + moveY;

        int tx = static_cast<int>(npc.pos_x + 8) / TILE_SIZE;
        int ty = static_cast<int>(newY + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            npc.pos_y = newY;
        }
    }

    return true;
}

void NPCManager::update(ZoneID currentZone, int currentMinute,
                         bool hourChanged, const TileMap& map, float dt) {
    for (int i = 0; i < MAX_NPCS; i++) {
        NPC& npc = m_npcs[i];
        if (!npc.active)                 continue;
        if (npc.home_zone != currentZone) continue;
        if (npc.dialogue_active)          continue; // frozen while talking

        // Re-evaluate schedule whenever the hour ticks
        if (hourChanged) {
            evaluateSchedule(npc, currentMinute);
        }

        // Move toward current target
        moveNPC(npc, map, dt);
    }
}

void NPCManager::tryInteract(float playerX, float playerY, bool aPressed) {
    // If dialogue already open, A closes it
    if (m_activeDialogueIndex >= 0 && aPressed) {
        closeDialogue();
        return;
    }

    if (!aPressed) return;

    // Find closest NPC within range in the current zone
    float closestDist = INTERACT_RANGE_PX;
    int   closestIdx  = -1;

    for (int i = 0; i < MAX_NPCS; i++) {
        const NPC& npc = m_npcs[i];
        if (!npc.active) continue;

        float dx   = (npc.pos_x + 8.0f) - playerX;
        float dy   = (npc.pos_y + 8.0f) - playerY;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < closestDist) {
            closestDist = dist;
            closestIdx  = i;
        }
    }

    if (closestIdx >= 0) {
        m_npcs[closestIdx].dialogue_active = true;
        m_activeDialogueIndex = closestIdx;
        LOG("Dialogue opened: %s", m_npcs[closestIdx].name);
    }
}

void NPCManager::closeDialogue() {
    if (m_activeDialogueIndex >= 0) {
        m_npcs[m_activeDialogueIndex].dialogue_active = false;
        m_activeDialogueIndex = -1;
    }
}

bool NPCManager::isDialogueOpen() const {
    return m_activeDialogueIndex >= 0;
}

const NPC* NPCManager::getActiveDialogueNPC() const {
    if (m_activeDialogueIndex < 0) return nullptr;
    return &m_npcs[m_activeDialogueIndex];
}
