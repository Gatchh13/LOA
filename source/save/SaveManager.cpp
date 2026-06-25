//-----------------------------------------------------------------------------
// SaveManager.cpp
//-----------------------------------------------------------------------------

#include "SaveManager.h"
#include "../core/Logger.h"

#include <cstdio>
#include <cstring>

//-----------------------------------------------------------------------------
// gather — snapshot all live game state into a SaveData
//-----------------------------------------------------------------------------
void SaveManager::gather(SaveData&                 out,
                          const Player&             player,
                          const ZoneManager&        zones,
                          const WorldClock&         worldClock,
                          const PlayerState&        playerState,
                          const QuestManager&       questMgr,
                          const WorldObjectManager& worldObjects)
{
    memset(&out, 0, sizeof(out));

    // Header (checksum and save_count filled after data is written)
    out.magic   = SAVE_MAGIC;
    out.version = SAVE_VERSION;

    // Player position
    out.zone_id   = static_cast<u8>(zones.getCurrentZoneDef().id);
    out.player_pad = 0;
    out.player_x  = player.getX();
    out.player_y  = player.getY();

    // World clock
    out.total_minutes = static_cast<u16>(worldClock.getTotalMinutes());

    // PlayerState
    out.gold = playerState.gold;
    out.wood = playerState.wood;
    out.rope = playerState.rope;
    out.hp     = playerState.hp;
    out.maxHp  = playerState.maxHp;

    // Inventory (Milestone 7) — POD copy, slot order preserved.
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        out.inventory[i] = playerState.inventory.slots[i];
    }

    // Equipment (Milestone 9)
    out.equipped_weapon = playerState.equippedWeapon;
    out.equipped_armor  = playerState.equippedArmor;

    // Quest states
    for (int i = 0; i < MAX_QUESTS; i++) {
        out.quest_status[i]       = static_cast<u8>(questMgr.getStatus(static_cast<u8>(i)));
        out.quest_current_step[i] = questMgr.getCurrentStep(static_cast<u8>(i));
    }

    // World object states (slot-indexed)
    const WorldObject* objs = worldObjects.getObjects();
    for (int i = 0; i < MAX_WORLD_OBJECTS; i++) {
        out.world_object_states[i] = static_cast<u8>(objs[i].state);
    }

    // Reserved blocks stay zeroed (memset above)
}

//-----------------------------------------------------------------------------
// apply — restore all game state from a validated SaveData
//-----------------------------------------------------------------------------
void SaveManager::apply(const SaveData&      sd,
                         Player&              player,
                         ZoneManager&         zones,
                         WorldClock&          worldClock,
                         PlayerState&         playerState,
                         QuestManager&        questMgr,
                         WorldObjectManager&  worldObjects,
                         TileMap&             map)
{
    // 1. Load the saved zone first (this resets the TileMap base layer)
    ZoneID savedZone = static_cast<ZoneID>(sd.zone_id);
    if (sd.zone_id >= ZONE_COUNT) savedZone = ZoneID::TOWN;  // safety clamp
    zones.loadZone(savedZone, 0);  // spawn index 0 — position overridden below

    // 2. Restore world object states (values only — no overrides applied yet)
    worldObjects.setStatesFromSave(sd.world_object_states, MAX_WORLD_OBJECTS, map);

    // 2b. Apply overrides for ONLY the zone we just loaded. This must come
    //     after setStatesFromSave() and after loadZone() above — onZoneLoaded
    //     filters by obj.zone == savedZone, so objects belonging to other
    //     zones never touch this TileMap.
    worldObjects.onZoneLoaded(savedZone, map);

    // 3. Place the player at the saved position
    player.setPosition(sd.player_x, sd.player_y);

    // 4. Restore world clock
    worldClock.setTime(sd.total_minutes);

    // 5. Restore player resources
    playerState.gold = sd.gold;
    playerState.wood = sd.wood;
    playerState.rope = sd.rope;
    playerState.hp     = sd.hp;
    playerState.maxHp  = sd.maxHp;

    // 5b. Restore inventory (Milestone 7)
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        playerState.inventory.slots[i] = sd.inventory[i];
    }

    // 5c. Restore equipment (Milestone 9)
    playerState.equippedWeapon = sd.equipped_weapon;
    playerState.equippedArmor  = sd.equipped_armor;

    // 6. Restore quest states
    for (int i = 0; i < MAX_QUESTS; i++) {
        QuestStatus status = static_cast<QuestStatus>(sd.quest_status[i]);
        questMgr.setStateFromSave(static_cast<u8>(i), status, sd.quest_current_step[i]);
    }

    LOG("SaveManager: loaded — zone=%d pos=(%.0f,%.0f) time=%d gold=%u wood=%u rope=%u hp=%u/%u weapon=%u armor=%u",
        sd.zone_id, sd.player_x, sd.player_y, sd.total_minutes,
        sd.gold, sd.wood, sd.rope, sd.hp, sd.maxHp,
        sd.equipped_weapon, sd.equipped_armor);
}

//-----------------------------------------------------------------------------
// validate — check magic, version, CRC
//-----------------------------------------------------------------------------
bool SaveManager::validate(const SaveData& sd) {
    if (sd.magic != SAVE_MAGIC) {
        WARN("SaveManager: bad magic 0x%08X", sd.magic);
        return false;
    }
    if (sd.version != SAVE_VERSION) {
        WARN("SaveManager: version mismatch (file=%u, expected=%u)",
             sd.version, SAVE_VERSION);
        return false;
    }
    // CRC covers everything after the 14-byte header
    const u8* dataStart = reinterpret_cast<const u8*>(&sd) + CHECKSUM_OFFSET;
    u32 dataLen         = static_cast<u32>(sizeof(SaveData)) - CHECKSUM_OFFSET;
    u32 computed        = loa_crc32(dataStart, dataLen);
    if (computed != sd.checksum) {
        WARN("SaveManager: CRC mismatch (stored=0x%08X, computed=0x%08X)",
             sd.checksum, computed);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// saveGame
//-----------------------------------------------------------------------------
bool SaveManager::saveGame(const Player&             player,
                            const ZoneManager&        zones,
                            const WorldClock&         worldClock,
                            const PlayerState&        playerState,
                            const QuestManager&       questMgr,
                            const WorldObjectManager& worldObjects)
{
    SaveData sd;
    gather(sd, player, zones, worldClock, playerState, questMgr, worldObjects);

    // Increment save counter (read existing count if possible, else start at 1)
    {
        FILE* existing = fopen(SAVE_PATH, "rb");
        if (existing) {
            SaveData prev;
            if (fread(&prev, sizeof(SaveData), 1, existing) == 1 &&
                prev.magic == SAVE_MAGIC)
            {
                sd.save_count = prev.save_count + 1;
            } else {
                sd.save_count = 1;
            }
            fclose(existing);
        } else {
            sd.save_count = 1;
        }
    }

    // Compute CRC over data block (after the 14-byte header)
    const u8* dataStart = reinterpret_cast<const u8*>(&sd) + CHECKSUM_OFFSET;
    u32 dataLen         = static_cast<u32>(sizeof(SaveData)) - CHECKSUM_OFFSET;
    sd.checksum         = loa_crc32(dataStart, dataLen);

    // Atomic write: write to .tmp, then rename
    FILE* f = fopen(TEMP_PATH, "wb");
    if (!f) {
        ERR("SaveManager: cannot open %s for writing", TEMP_PATH);
        return false;
    }

    bool ok = (fwrite(&sd, sizeof(SaveData), 1, f) == 1);
    fclose(f);

    if (!ok) {
        ERR("SaveManager: fwrite failed");
        remove(TEMP_PATH);
        return false;
    }

    // Rename temp → final (atomic on FAT32 on 3DS)
    remove(SAVE_PATH);
    if (rename(TEMP_PATH, SAVE_PATH) != 0) {
        ERR("SaveManager: rename failed");
        return false;
    }

    LOG("SaveManager: saved (slot #%u, %zu bytes)", sd.save_count, sizeof(SaveData));
    return true;
}

//-----------------------------------------------------------------------------
// loadGame
//-----------------------------------------------------------------------------
bool SaveManager::loadGame(Player&             player,
                            ZoneManager&        zones,
                            WorldClock&         worldClock,
                            PlayerState&        playerState,
                            QuestManager&       questMgr,
                            WorldObjectManager& worldObjects,
                            TileMap&            map)
{
    FILE* f = fopen(SAVE_PATH, "rb");
    if (!f) {
        LOG("SaveManager: no save file found at %s", SAVE_PATH);
        return false;
    }

    SaveData sd;
    bool readOk = (fread(&sd, sizeof(SaveData), 1, f) == 1);
    fclose(f);

    if (!readOk) {
        WARN("SaveManager: fread failed (file truncated?)");
        return false;
    }

    if (!validate(sd)) {
        WARN("SaveManager: validation failed — starting new game");
        return false;
    }

    apply(sd, player, zones, worldClock, playerState, questMgr, worldObjects, map);
    return true;
}

//-----------------------------------------------------------------------------
// hasSave
//-----------------------------------------------------------------------------
bool SaveManager::hasSave() {
    FILE* f = fopen(SAVE_PATH, "rb");
    if (!f) return false;

    SaveData sd;
    bool readOk = (fread(&sd, sizeof(SaveData), 1, f) == 1);
    fclose(f);

    return readOk && validate(sd);
}

//-----------------------------------------------------------------------------
// deleteSave
//-----------------------------------------------------------------------------
bool SaveManager::deleteSave() {
    int result = remove(SAVE_PATH);
    if (result == 0) {
        LOG("SaveManager: save file deleted");
    }
    return (result == 0);
}
