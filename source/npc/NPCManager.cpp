//-----------------------------------------------------------------------------
// NPCManager.cpp  (Milestone 10 — consumes NPCDatabase)
//-----------------------------------------------------------------------------

#include "NPCManager.h"
#include "../core/Movement.h"
#include "../quest/QuestManager.h"
#include "../player/PlayerState.h"
#include "../core/Logger.h"
#include <cstring>
#include <cmath>
#include <algorithm>

//=============================================================================
// NPCManager
//
// Milestone 10 change: NPC instances are populated from NPCDatabase's
// static table (one getNPCDef() call per slot) instead of three
// hardcoded initBlacksmith()/initMerchant()/initGuard() functions. The
// data itself (names, dialogue, schedules) is unchanged — see
// NPCDatabase.cpp, where it now lives.
//
// A side effect noticed while migrating this code, not introduced by
// it: the old initX() functions each set npc.pos_x/pos_y to a specific
// tile, but init() always calls snapToSchedule() immediately afterward
// for every NPC, which unconditionally overwrites pos_x/pos_y from the
// schedule's first applicable entry. The initX() position assignment
// was already dead code before Milestone 10 — confirmed by tracing
// init()'s exact call sequence, not assumed — so there's no equivalent
// "initial position" field on NPCDef; schedule[0]'s destination is
// already what determines starting position, same as before.
//=============================================================================

NPCManager::NPCManager() : m_activeDialogueIndex(-1) {
    memset(m_npcs, 0, sizeof(m_npcs));
}

void NPCManager::init(int currentMinute) {
    for (int i = 0; i < MAX_NPCS; i++) m_npcs[i].active = false;

    int defCount = getNPCDefCount();
    for (int i = 0; i < defCount && i < MAX_NPCS; i++) {
        m_npcs[i].defIndex            = static_cast<u8>(i);
        m_npcs[i].dialogue_override   = nullptr;
        m_npcs[i].dialogue_active     = false;
        m_npcs[i].active_entry        = -1;
        m_npcs[i].active              = true;
    }

    for (int i = 0; i < MAX_NPCS; i++) {
        if (!m_npcs[i].active) continue;
        snapToSchedule(m_npcs[i], currentMinute);
    }
    LOG("NPCManager: initialized %d NPCs at time %d", defCount, currentMinute);
}

void NPCManager::evaluateSchedule(NPC& npc, int currentMinute) {
    const NPCDef& def = getNPCDef(npc.defIndex);
    if (def.schedule_count == 0) return;

    // Find the entry with the LARGEST start_minute that is still
    // <= currentMinute — independent of array order. This is a real
    // fix, not a refactor: the previous version picked whichever
    // qualifying entry came LAST in array order, which only matches
    // "largest start_minute so far" if the data happens to be sorted
    // ascending. Every NPC's actual schedule places its midnight-
    // wraparound entry (start_minute == 0) last in array order for
    // narrative readability ("Sleep" reads naturally as the final
    // event of the written day) — and since 0 <= currentMinute holds
    // for every possible time of day, that entry always "qualified
    // last" and always won, regardless of what time it actually was.
    // Confirmed this was already true before any Milestone 12 change
    // by re-running the same check against the pre-refactor code.
    int best          = -1;
    int bestStart     = -1;
    int overallLatest = 0; // index of the single largest start_minute,
                            // used for the before-the-first-entry-of-
                            // the-day wraparound case below.
    for (int i = 0; i < def.schedule_count; i++) {
        int start = def.schedule[i].start_minute;
        if (start > def.schedule[overallLatest].start_minute) overallLatest = i;
        if (start <= currentMinute && start > bestStart) {
            best      = i;
            bestStart = start;
        }
    }
    if (best < 0) {
        // No entry's start_minute has been reached yet today (e.g. it's
        // 02:00 and the earliest entry is 06:00) — the active entry is
        // still the latest one overall, carried over from "yesterday".
        best = overallLatest;
    }

    if (npc.active_entry != best) {
        npc.active_entry = best;
        const ScheduleEntry& entry = def.schedule[best];
        npc.target_x = entry.dest_tx * TILE_SIZE + 1.0f;
        npc.target_y = entry.dest_ty * TILE_SIZE + 1.0f;
        LOG("NPC %s -> %s (%d,%d)", def.name, entry.label, entry.dest_tx, entry.dest_ty);
    }
}

void NPCManager::snapToSchedule(NPC& npc, int currentMinute) {
    npc.active_entry = -1;
    evaluateSchedule(npc, currentMinute);
    npc.pos_x = npc.target_x;
    npc.pos_y = npc.target_y;
}

// moveNPC() was removed in Milestone 12 — its logic is now the shared
// seekTowardTarget() in source/core/Movement.cpp, called directly from
// update() below with NPC_SPEED/ARRIVAL_THRESHOLD passed through
// unchanged.
void NPCManager::update(ZoneID currentZone, int currentMinute,
                         bool hourChanged, const TileMap& map, float dt) {
    for (int i = 0; i < MAX_NPCS; i++) {
        NPC& npc = m_npcs[i];
        if (!npc.active) continue;
        const NPCDef& def = getNPCDef(npc.defIndex);
        if (def.home_zone != currentZone)  continue;
        if (npc.dialogue_active)           continue;
        if (hourChanged) evaluateSchedule(npc, currentMinute);
        seekTowardTarget(npc.pos_x, npc.pos_y, npc.target_x, npc.target_y,
                         NPC_SPEED, ARRIVAL_THRESHOLD, dt, map, npc.anim);
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
        const NPCDef& def = getNPCDef(activeNpc.defIndex);
        questMgr.onDialogueClosed(def.npc_id, playerState);
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
        const NPCDef& def = getNPCDef(npc.defIndex);
        // Ask QuestManager if this NPC has context-sensitive dialogue
        const char* override_text = questMgr.onTalkToNPC(def.npc_id);
        npc.dialogue_override  = override_text;  // nullptr if no override
        npc.dialogue_active    = true;
        m_activeDialogueIndex  = closestIdx;
        LOG("Dialogue opened: %s%s", def.name,
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

bool NPCManager::isTalkingToMerchant() const {
    if (m_activeDialogueIndex < 0) return false;
    const NPCDef& def = getNPCDef(m_npcs[m_activeDialogueIndex].defIndex);
    return def.shop_id != NO_SHOP;
}
