#pragma once

//-----------------------------------------------------------------------------
// SaveData.h
// Fixed-size binary save structure for Legends of Aetheria.
//
// RULES:
//   - POD only. No pointers. No virtual functions. No constructors.
//   - All fields are primitive types or fixed-size arrays.
//   - #pragma pack(1) ensures no compiler padding surprises.
//   - sizeof(SaveData) is static_assert'd to catch accidental changes.
//   - The struct is written/read as one fwrite/fread block.
//
// VERSIONING:
//   SAVE_VERSION is bumped whenever the layout changes.
//   SaveManager::loadGame() rejects saves with mismatched versions and
//   falls back to a new game rather than crashing on garbage data.
//
// FUTURE EXTENSION:
//   Each "reserved" block is named for its intended future system.
//   To add inventory: replace reserved_inventory[64] with an actual
//   ItemSlot array. Bump SAVE_VERSION. No other changes needed.
//
// SIZE: 236 bytes (well under the 8 KB target).
//
// NPC POSITIONS:
//   Not saved. NPCs reconstruct their schedule position from the saved
//   world clock time. This is the correct approach — it avoids NPC
//   de-sync bugs and keeps the save file minimal.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../quest/PlayerState.h"
#include "../world/WorldObject.h"
#include "../quest/QuestManager.h"

static constexpr u16 SAVE_VERSION   = 5;     // bump on every layout change
static constexpr u32 SAVE_MAGIC     = 0x4C4F4100;  // "LOA\0"

//-----------------------------------------------------------------------------
// Inline CRC32 — no external library needed.
// Computes a 32-bit checksum over a byte buffer.
// Declared here so SaveManager can include just this header.
//-----------------------------------------------------------------------------
inline u32 loa_crc32(const void* data, u32 length) {
    const u8* bytes = static_cast<const u8*>(data);
    u32 crc = 0xFFFFFFFFu;
    for (u32 i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
        }
    }
    return ~crc;
}

#pragma pack(push, 1)
struct SaveData {
    //-------------------------------------------------------------------------
    // Header (14 bytes)
    //-------------------------------------------------------------------------
    u32 magic;          // SAVE_MAGIC — validation sentinel
    u16 version;        // SAVE_VERSION — layout version
    u32 save_count;     // incremented on every save (for debugging)
    u32 checksum;       // CRC32 of bytes [offsetof(zone_id) .. end of struct]
                        // Header itself is excluded from checksum.

    //-------------------------------------------------------------------------
    // Player position (10 bytes)
    //-------------------------------------------------------------------------
    u8  zone_id;        // ZoneID cast to u8
    u8  player_pad;     // reserved, write 0
    f32 player_x;       // pixel position (top-left of player rect)
    f32 player_y;

    //-------------------------------------------------------------------------
    // World clock (4 bytes)
    //-------------------------------------------------------------------------
    u16 total_minutes;  // 0–1439, in-game minutes since midnight
    u8  time_pad[2];    // reserved, write 0

    //-------------------------------------------------------------------------
    // Player resources (8 bytes) — PlayerState layout mirrored manually
    // We mirror rather than embed PlayerState directly because PlayerState
    // has a constructor (making it non-trivially constructible) which can
    // prevent placement inside a #pragma pack struct on some compilers.
    //-------------------------------------------------------------------------
    u32 gold;
    u8  wood;
    u8  rope;
    u8  resource_pad[2];

    //-------------------------------------------------------------------------
    // Quest states (32 bytes) — MAX_QUESTS = 16 entries × 2 bytes each
    //-------------------------------------------------------------------------
    u8  quest_status[MAX_QUESTS];        // QuestStatus cast to u8
    u8  quest_current_step[MAX_QUESTS];  // current step index

    //-------------------------------------------------------------------------
    // World object states (16 bytes) — MAX_WORLD_OBJECTS = 16 × 1 byte each
    //-------------------------------------------------------------------------
    u8  world_object_states[MAX_WORLD_OBJECTS]; // WorldObjectState cast to u8

    //-------------------------------------------------------------------------
    // Reserved blocks for future systems
    // Replace with real structs when those milestones arrive.
    // Bump SAVE_VERSION when any block changes.
    //-------------------------------------------------------------------------
    u8  reserved_inventory[64];    // Milestone 6: item slots
    u8  reserved_skills[32];       // Future: skill XP levels
    u8  reserved_reputation[16];   // Future: faction reputation
    u8  reserved_titles[8];        // Future: unlocked title bitfield
    u8  reserved_crafting[16];     // Future: crafting unlock flags
    u8  reserved_housing[16];      // Future: housing state
};
#pragma pack(pop)

// Compile-time size verification.
// If this fails, the layout has changed without a version bump.
static_assert(sizeof(SaveData) == 236, "SaveData size changed — bump SAVE_VERSION");

// Offset of the first checksummed byte (everything after the header).
static constexpr u32 CHECKSUM_OFFSET = 14u;
