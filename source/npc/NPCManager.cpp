//-----------------------------------------------------------------------------
// NPCManager.cpp  (Milestone 3 — Quest integration)
//-----------------------------------------------------------------------------

#include "NPCManager.h"
#include "../quest/QuestManager.h"
#include "../quest/PlayerState.h"
#include "../core/Logger.h"
#include <cstring>
#include <cmath>
#include <algorithm>

//=============================================================================
// NPC DATA DEFINITIONS
//=============================================================================

static void initBlacksmith(NPC& n) {
    n.npc_id   = 0;
    strncpy(n.name,     "Aldric",                                  MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "The forge never rests. Neither do I.",    MAX_DIALOGUE_LEN - 1);
    n.home_zone        = ZoneID::TOWN;
    n.dialogue_override = nullptr;

    n.pos_x = 4 * TILE_SIZE + 1.0f;
    n.pos_y = 2 * TILE_SIZE + 1.0f;

    n.schedule[0] = {  6 * 60, 4,  2, "Wake"      };
    n.schedule[1] = {  8 * 60, 4,  3, "Forge"     };
    n.schedule[2] = { 12 * 60, 5,  9, "Tavern"    };
    n.schedule[3] = { 13 * 60, 4,  3, "Forge"     };
    n.schedule[4] = { 18 * 60, 4,  2, "Wind down" };
    n.schedule[5] = { 20 * 60, 4,  9, "Home"      };
    n.schedule[6] = {  0 * 60, 4,  9, "Sleep"     };
    n.schedule_count = 7;
    n.active = true;
}

static void initMerchant(NPC& n) {
    n.npc_id   = 1;
    strncpy(n.name,     "Mira",                                          MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "Finest goods this side of the mountains!",      MAX_DIALOGUE_LEN - 1);
    n.home_zone         = ZoneID::TOWN;
    n.dialogue_override = nullptr;

    n.pos_x = 10 * TILE_SIZE + 1.0f;
    n.pos_y =  6 * TILE_SIZE + 1.0f;

    n.schedule[0] = {  7 * 60, 10,  6, "Open stall"  };
    n.schedule[1] = {  8 * 60, 12,  8, "Market"      };
    n.schedule[2] = { 13 * 60, 10,  9, "Lunch break" };
    n.schedule[3] = { 14 * 60, 14,  7, "Afternoon"   };
    n.schedule[4] = { 18 * 60, 10,  6, "Close stall" };
    n.schedule[5] = { 19 * 60, 21,  2, "Home"        };
    n.schedule[6] = { 22 * 60, 21,  3, "Sleep"       };
    n.schedule[7] = {  0 * 60, 21,  3, "Night"       };
    n.schedule_count = 8;
    n.active = true;
}

static void initGuard(NPC& n) {
    n.npc_id   = 2;
    strncpy(n.name,     "Brennan",                                  MAX_NPC_NAME_LEN - 1);
    strncpy(n.dialogue, "Move along. Nothing to see here.",         MAX_DIALOGUE_LEN - 1);
    n.home_zone         = ZoneID::TOWN;
    n.dialogue_override = nullptr;

    n.pos_x =  1 * TILE_SIZE + 1.0f;
    n.pos_y =  1 * TILE_SIZE + 1.0f;

    n.schedule[0] = {  0 * 60,  1,  1, "Patrol A1" };
    n.schedule[1] = {  3 * 60, 13, 18, "Patrol A2" };
    n.schedule[2] = {  6 * 60,  1,  1, "Patrol A3" };
    n.schedule[3] = {  8 * 60, 26,  1, "Patrol B1" };
    n.schedule[4] = { 12 * 60,  1, 20, "Patrol B2" };
    n.schedule[5] = { 15 * 60, 13, 18, "Patrol B3" };
    n.schedule[6] = { 18 * 60, 26, 20, "Patrol B4" };
    n.schedule[7] = { 21 * 60,  1,  1, "Night post"};
    n.schedule_count = 8;
    n.active = true;
}

//=============================================================================
// NPCManager
//=============================================================================

NPCManager::NPCManager() : m_activeDialogueIndex(-1) {
    memset(m_npcs, 0, sizeof(m_npcs));
}

void NPCManager::init(int currentMinute) {
    for (int i = 0; i < MAX_NPCS; i++) m_npcs[i].active = false;

    initBlacksmith(m_npcs[0]);
    initMerchant  (m_npcs[1]);
    initGuard     (m_npcs[2]);

    for (int i = 0; i < MAX_NPCS; i++) {
        if (!m_npcs[i].active) continue;
        snapToSchedule(m_npcs[i], currentMinute);
    }
    LOG("NPCManager: initialized 3 NPCs at time %d", currentMinute);
}

void NPCManager::evaluateSchedule(NPC& npc, int currentMinute) {
    if (npc.schedule_count == 0) return;
    int best = 0;
    for (int i = 0; i < npc.schedule_count; i++) {
        if (npc.schedule[i].start_minute <= currentMinute) best = i;
    }
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
    npc.active_entry = -1;
    evaluateSchedule(npc, currentMinute);
    npc.pos_x = npc.target_x;
    npc.pos_y = npc.target_y;
}

bool NPCManager::moveNPC(NPC& npc, const TileMap& map, float dt) {
    float dx   = npc.target_x - npc.pos_x;
    float dy   = npc.target_y - npc.pos_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= ARRIVAL_THRESHOLD) return false;

    float step = NPC_SPEED * dt;
    if (fabsf(dx) > ARRIVAL_THRESHOLD) {
        float moveX = (dx > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dx));
        float newX  = npc.pos_x + moveX;
        int tx = static_cast<int>(newX + 8) / TILE_SIZE;
        int ty = static_cast<int>(npc.pos_y + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) npc.pos_x = newX;
    } else if (fabsf(dy) > ARRIVAL_THRESHOLD) {
        float moveY = (dy > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dy));
        float newY  = npc.pos_y + moveY;
        int tx = static_cast<int>(npc.pos_x + 8) / TILE_SIZE;
        int ty = static_cast<int>(newY + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) npc.pos_y = newY;
    }
    return true;
}

void NPCManager::update(ZoneID currentZone, int currentMinute,
                         bool hourChanged, const TileMap& map, float dt) {
    for (int i = 0; i < MAX_NPCS; i++) {
        NPC& npc = m_npcs[i];
        if (!npc.active)                  continue;
        if (npc.home_zone != currentZone)  continue;
        if (npc.dialogue_active)           continue;
        if (hourChanged) evaluateSchedule(npc, currentMinute);
        moveNPC(npc, map, dt);
    }
}

void NPCManager::tryInteract(float playerX, float playerY, bool aPressed,
                              QuestManager& questMgr, PlayerState& playerState) {
    // Close open dialogue on A press
    if (m_activeDialogueIndex >= 0 && aPressed) {
        NPC& activeNpc = m_npcs[m_activeDialogueIndex];
        activeNpc.dialogue_active  = false;
        activeNpc.dialogue_override = nullptr;
        // Notify quest manager that dialogue with this NPC closed
        questMgr.onDialogueClosed(activeNpc.npc_id, playerState);
        m_activeDialogueIndex = -1;
        return;
    }

    if (!aPressed) return;

    // Find closest NPC in range
    float closestDist = INTERACT_RANGE_PX;
    int   closestIdx  = -1;
    for (int i = 0; i < MAX_NPCS; i++) {
        const NPC& npc = m_npcs[i];
        if (!npc.active) continue;
        float dx   = (npc.pos_x + 8.0f) - playerX;
        float dy   = (npc.pos_y + 8.0f) - playerY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < closestDist) { closestDist = dist; closestIdx = i; }
    }

    if (closestIdx >= 0) {
        NPC& npc = m_npcs[closestIdx];
        // Ask QuestManager if this NPC has context-sensitive dialogue
        const char* override_text = questMgr.onTalkToNPC(npc.npc_id);
        npc.dialogue_override  = override_text;  // nullptr if no override
        npc.dialogue_active    = true;
        m_activeDialogueIndex  = closestIdx;
        LOG("Dialogue opened: %s%s", npc.name,
            override_text ? " [quest dialogue]" : "");
    }
}

void NPCManager::closeDialogue() {
    if (m_activeDialogueIndex >= 0) {
        m_npcs[m_activeDialogueIndex].dialogue_active   = false;
        m_npcs[m_activeDialogueIndex].dialogue_override = nullptr;
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
