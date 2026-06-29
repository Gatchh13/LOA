#pragma once

//-----------------------------------------------------------------------------
// SaveManager.h
// Handles serialization of all game state to/from sdmc:/loa_save.bin.
//
// API:
//   saveGame(...)  — write current state to SD card
//   loadGame(...)  — read state from SD card and apply to all systems
//   hasSave()      — true if a valid save file exists
//   deleteSave()   — remove the save file
//
// All functions are static — SaveManager holds no state of its own.
// The save file is written atomically: data goes to loa_save.tmp first,
// then renamed to loa_save.bin. This prevents corruption on power-off
// during a write (the previous save stays intact until the new one is
// complete).
//
// Load validation:
//   1. File must exist and be exactly sizeof(SaveData) bytes.
//   2. magic must equal SAVE_MAGIC.
//   3. version must equal SAVE_VERSION.
//   4. CRC32 of the data block must match the stored checksum.
//   If any check fails, loadGame() returns false and the caller starts
//   a new game.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "SaveData.h"
#include "../player/PlayerState.h"
#include "../quest/QuestManager.h"
#include "../world/WorldObjectManager.h"
#include "../world/ZoneManager.h"
#include "../core/WorldClock.h"
#include "../entities/Player.h"

class SaveManager {
public:
    // Gather current game state into SaveData and write to SD card.
    // Returns true on success.
    static bool saveGame(const Player&             player,
                         const ZoneManager&         zones,
                         const WorldClock&          worldClock,
                         const PlayerState&         playerState,
                         const QuestManager&        questMgr,
                         const WorldObjectManager&  worldObjects);

    // Read SaveData from SD card and apply to all game systems.
    // Returns true if a valid save was loaded.
    // On failure (no file, bad magic, wrong version, bad CRC) returns false.
    static bool loadGame(Player&             player,
                         ZoneManager&         zones,
                         WorldClock&          worldClock,
                         PlayerState&         playerState,
                         QuestManager&        questMgr,
                         WorldObjectManager&  worldObjects,
                         TileMap&             map);

    // Returns true if a save file exists that passes all validation.
    static bool hasSave();

    // Delete the save file. Returns true if deleted (or didn't exist).
    static bool deleteSave();

private:
    static constexpr const char* SAVE_PATH = "sdmc:/loa_save.bin";
    static constexpr const char* TEMP_PATH = "sdmc:/loa_save.tmp";

    // Fill a SaveData from current game state.
    static void gather(SaveData&                 out,
                       const Player&             player,
                       const ZoneManager&        zones,
                       const WorldClock&         worldClock,
                       const PlayerState&        playerState,
                       const QuestManager&       questMgr,
                       const WorldObjectManager& worldObjects);

    // Apply a validated SaveData to all game systems.
    static void apply(const SaveData&      sd,
                      Player&              player,
                      ZoneManager&         zones,
                      WorldClock&          worldClock,
                      PlayerState&         playerState,
                      QuestManager&        questMgr,
                      WorldObjectManager&  worldObjects,
                      TileMap&             map);

    // Validate a SaveData: magic, version, CRC.
    static bool validate(const SaveData& sd);
};
