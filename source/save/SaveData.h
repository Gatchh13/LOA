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
//   Milestone 7 bumps SAVE_VERSION 5 -> 6: reserved_inventory[64] is
//   replaced by inventory[8] (16 bytes) + inventory_pad[48] (48 bytes,
//   explicitly named so the remaining reserved headroom stays visible
//   rather than silently shrinking). Total block size is unchanged at
//   64 bytes, so sizeof(SaveData) does not change — only the meaning of
//   those 64 bytes does.
//
//   Milestone 8 bumps SAVE_VERSION 6 -> 7: adds hp/maxHp (4 bytes —
//   u16 each, per the Milestone 8 assignment's recommendation). To keep
//   sizeof(SaveData) unchanged, reserved_skills shrinks from 32 to 28
//   bytes (still generously sized for a future skill-XP system; the 4
//   bytes weren't otherwise allocated to anything HP-shaped, so this
//   was the least disruptive place to take them from — borrowing from
//   reserved_reputation/titles/crafting/housing would be a category
//   mismatch, since none of those are player-vitals-shaped either).
//   This is, as with every prior version bump in this project, a hard
//   break, not a migration — see the existing policy below.
//
//   Every prior version bump in this project has been handled the same
//   way (no migration code exists anywhere; validate() already
//   rejects mismatched versions and the caller falls back to a new
//   game). Pre-Milestone-8 saves (version 6 and earlier) will be
//   rejected and the player will start a fresh game.
//
//   Milestone 9 bumps SAVE_VERSION 7 -> 8: adds equipped_weapon/
//   equipped_armor (2 bytes — u8 ItemID each, or NO_EQUIPMENT/0xFF for
//   an empty slot; see PlayerState.h). To keep sizeof(SaveData)
//   unchanged, reserved_titles shrinks from 8 to 6 bytes — the
//   "unlocked title bitfield" concept is just as unrelated to equipment
//   as Milestone 8's HP fields were to skills, so this follows the same
//   "take from the least-related reserved block" reasoning rather than
//   touching reserved_skills again. Pre-Milestone-9 saves (version 7
//   and earlier) will be rejected and the player will start a fresh
//   game, per the same no-migration policy as every prior bump.
//
// FUTURE EXTENSION:
//   Each remaining "reserved" block is named for its intended future
//   system. inventory_pad[48] is reserved specifically for inventory
//   growth (more slots, or a stack count wider than u8) without another
//   version bump touching unrelated blocks.
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
#include "../quest/Inventory.h"
#include "../world/WorldObject.h"
#include "../quest/QuestManager.h"
#include <cstddef>  // offsetof

static constexpr u16 SAVE_VERSION   = 8;     // bump on every layout change
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
    //
    // PlayerState is now a plain POD struct (no constructor) and could be
    // embedded directly here. Still mirrored rather than embedded for now —
    // doing that swap is scheduled for Milestone 7 alongside the inventory
    // layout change (Architectural Review Recommendation 3), not as an
    // incidental change here. Until then, gold/wood/rope must be kept in
    // sync by hand if PlayerState's fields ever change.
    //-------------------------------------------------------------------------
    u32 gold;
    u8  wood;
    u8  rope;
    u8  resource_pad[2];

    //-------------------------------------------------------------------------
    // Player health (4 bytes) — Milestone 8
    //-------------------------------------------------------------------------
    u16 hp;
    u16 maxHp;

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
    // Inventory (16 bytes) — Milestone 7
    // InventorySlot is POD ({u8 item_id; u8 quantity;}), so it serializes
    // byte-for-byte without any conversion step in gather()/apply() beyond
    // a plain loop. inventory_pad keeps the remaining headroom from the
    // original 64-byte reserved_inventory block explicit and visible
    // (see Inventory.h for why 8 slots was chosen) rather than silently
    // shrinking the reserved space.
    //-------------------------------------------------------------------------
    InventorySlot inventory[INVENTORY_SLOTS];   // 8 * 2 bytes = 16 bytes
    u8            inventory_pad[48];            // reserved for future inventory growth

    //-------------------------------------------------------------------------
    // Equipment (2 bytes) — Milestone 9
    // u8 ItemID each, or NO_EQUIPMENT (0xFF) for an empty slot — see
    // PlayerState.h. Only two slots exist (weapon, armor) per the
    // Milestone 9 assignment's explicit scope, so these are two named
    // fields, not an array.
    //-------------------------------------------------------------------------
    u8  equipped_weapon;
    u8  equipped_armor;

    //-------------------------------------------------------------------------
    // Reserved blocks for future systems
    // Replace with real structs when those milestones arrive.
    // Bump SAVE_VERSION when any block changes.
    //-------------------------------------------------------------------------
    u8  reserved_skills[28];       // Future: skill XP levels (4 bytes
                                    // donated to hp/maxHp in Milestone 8 —
                                    // was 32 bytes, see version-history
                                    // comment above for why this block)
    u8  reserved_reputation[16];   // Future: faction reputation
    u8  reserved_titles[6];        // Future: unlocked title bitfield (2
                                    // bytes donated to equipped_weapon/
                                    // equipped_armor in Milestone 9 —
                                    // was 8 bytes)
    u8  reserved_crafting[16];     // Future: crafting unlock flags
    u8  reserved_housing[16];      // Future: housing state
};
#pragma pack(pop)

// Compile-time size verification.
// If this fails, the layout has changed without a version bump.
static_assert(sizeof(SaveData) == 236, "SaveData size changed — bump SAVE_VERSION");

// Offset of the first checksummed byte (everything after the header).
// Tied directly to the struct layout via offsetof() so it can never drift
// out of sync if a header field is added, removed, or resized — a literal
// constant here would silently checksum the wrong bytes and invalidate
// every existing save without any compiler warning.
static constexpr u32 CHECKSUM_OFFSET = static_cast<u32>(offsetof(SaveData, zone_id));
