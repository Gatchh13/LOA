# Legends of Aetheria — Technical Architecture
**Platform:** Nintendo 3DS Homebrew | **Stack:** devkitPro, C++, citro2d, citro3d  
**Version:** 1.0 | **Target:** Solo Developer, v1 Alpha Scope

---

## Table of Contents
1. [Hardware Constraints](#1-hardware-constraints)
2. [Folder Structure](#2-folder-structure)
3. [Game State Management](#3-game-state-management)
4. [Save System](#4-save-system)
5. [Entity Component Structure](#5-entity-component-structure)
6. [World Map Architecture](#6-world-map-architecture)
7. [Quest System](#7-quest-system)
8. [Inventory System](#8-inventory-system)
9. [Combat System](#9-combat-system)
10. [NPC Scheduling](#10-npc-scheduling)
11. [Rendering Pipeline](#11-rendering-pipeline)
12. [Asset Management](#12-asset-management)
13. [Milestone Roadmap](#13-milestone-roadmap)

---

## 1. Hardware Constraints

All architecture decisions are bounded by these hard limits.

| Resource | Limit | Design Response |
|---|---|---|
| RAM | ~128 MB (New 3DS), ~64 MB (Old 3DS) | Target 48 MB working set; never assume New 3DS |
| CPU | Dual-core ARM11 @ 268 MHz | Single-thread game loop; no multithreading |
| VRAM | ~6 MB (FCRAM shared) | Sprite sheets; strict texture atlas budget |
| Screen (top) | 400×240 px | 16×16 tiles = 25×15 tile viewport max |
| Screen (bottom) | 320×240 px | UI, map, inventory only |
| Storage | SD card | No size limit, but keep save files small |
| Controls | D-pad, A/B/X/Y, L/R, Start/Select, touchscreen | No analog stick on old 3DS; design around D-pad |

**Memory Budget (target total: ~48 MB)**

| System | Budget |
|---|---|
| Tileset textures (active zone) | 4 MB |
| Sprite sheet atlas | 2 MB |
| Active map data | 1 MB |
| Entity pool | 512 KB |
| UI textures | 1 MB |
| Audio (streaming) | 4 MB |
| Game logic / code | 4 MB |
| Misc / headroom | 4 MB |
| **Total** | **~20 MB** (safe margin for OS overhead) |

---

## 2. Folder Structure

```
legends-of-aetheria/
├── source/
│   ├── main.cpp                  # Entry point, game loop
│   ├── core/
│   │   ├── GameState.h/.cpp      # State machine
│   │   ├── SaveSystem.h/.cpp     # Save/load
│   │   ├── InputManager.h/.cpp   # 3DS input abstraction
│   │   ├── Clock.h/.cpp          # In-game time
│   │   └── Logger.h/.cpp         # Debug logging (stripped in release)
│   ├── ecs/
│   │   ├── EntityManager.h/.cpp  # Entity pool
│   │   ├── Components.h          # All component structs (POD only)
│   │   └── Systems.h/.cpp        # Update systems
│   ├── world/
│   │   ├── World.h/.cpp          # Zone manager
│   │   ├── Zone.h/.cpp           # Single map zone
│   │   ├── TileMap.h/.cpp        # Tile layer data
│   │   └── Collision.h/.cpp      # Tile collision
│   ├── gameplay/
│   │   ├── Player.h/.cpp         # Player controller
│   │   ├── Combat.h/.cpp         # Combat resolution
│   │   ├── Inventory.h/.cpp      # Item container
│   │   ├── Skills.h/.cpp         # Skill progression
│   │   ├── Crafting.h/.cpp       # Crafting system
│   │   ├── QuestManager.h/.cpp   # Quest tracking
│   │   └── ShopSystem.h/.cpp     # Buy/sell logic
│   ├── npc/
│   │   ├── NPC.h/.cpp            # NPC base
│   │   ├── NPCScheduler.h/.cpp   # Schedule engine
│   │   └── Dialogue.h/.cpp       # Dialogue tree runner
│   ├── render/
│   │   ├── Renderer.h/.cpp       # citro2d wrapper
│   │   ├── SpriteSheet.h/.cpp    # Sprite/frame manager
│   │   ├── TileRenderer.h/.cpp   # Tile draw calls
│   │   └── UIRenderer.h/.cpp     # Bottom screen UI
│   └── data/
│       ├── ItemDB.h/.cpp         # Static item table
│       ├── EnemyDB.h/.cpp        # Static enemy table
│       └── QuestDB.h/.cpp        # Static quest definitions
├── romfs/
│   ├── gfx/
│   │   ├── tilesets/             # .t3x texture files (citro2d format)
│   │   ├── sprites/              # Character/enemy atlas .t3x
│   │   └── ui/                   # UI elements .t3x
│   ├── maps/
│   │   └── zones/                # Zone data files (.bin)
│   ├── data/
│   │   ├── items.bin             # Compiled item table
│   │   ├── enemies.bin           # Compiled enemy table
│   │   ├── quests.bin            # Compiled quest definitions
│   │   └── npc_schedules.bin     # NPC schedule data
│   ├── audio/
│   │   ├── bgm/                  # .bcstm streaming music
│   │   └── sfx/                  # .bcwav sound effects
│   └── dialogue/
│       └── *.bin                 # Compiled dialogue trees
├── tools/
│   ├── map_editor/               # (optional) offline zone compiler
│   ├── data_compiler/            # Converts CSV → .bin tables
│   └── tileset_packer/           # Packs tilesets into .t3x
├── Makefile
└── README.md
```

**Why this structure:**
- `source/` is pure C++; no engine framework dependencies.
- `romfs/` mirrors how devkitPro mounts the read-only filesystem.
- All game data is pre-compiled to `.bin` at build time — no JSON parsing at runtime.
- Tools live outside the build and are run offline.

---

## 3. Game State Management

### Architecture: Flat State Machine

Use a simple enum-driven state machine. No polymorphic state objects — they add vtable overhead and complicate save/restore.

```cpp
// core/GameState.h

enum class GameState : u8 {
    BOOT,
    TITLE_SCREEN,
    MAIN_MENU,
    LOADING,
    GAMEPLAY,
    DIALOGUE,
    INVENTORY,
    SHOP,
    COMBAT,
    PAUSE_MENU,
    GAME_OVER,
    CREDITS
};

class StateManager {
public:
    void push(GameState s);
    void pop();
    GameState current() const;
    bool is(GameState s) const;

private:
    static constexpr int STACK_DEPTH = 4;
    GameState stack[STACK_DEPTH];
    int top = 0;
};
```

**Stack depth of 4 is sufficient:**  
`GAMEPLAY → DIALOGUE → TRADE → INVENTORY` is the deepest realistic stack.

### Main Loop

```cpp
// main.cpp
while (aptMainLoop()) {
    inputManager.update();
    float dt = clock.getDelta();

    switch (stateManager.current()) {
        case GameState::GAMEPLAY:   updateGameplay(dt);   break;
        case GameState::DIALOGUE:   updateDialogue(dt);   break;
        case GameState::INVENTORY:  updateInventory(dt);  break;
        case GameState::COMBAT:     updateCombat(dt);     break;
        // ...
    }

    renderer.beginFrame();
    renderCurrentState();
    renderer.endFrame();
}
```

**Performance note:** Single switch dispatch, no virtual calls, no heap allocation per frame.

### Update Budget (target 60 fps = 16.6 ms/frame)

| System | Time Budget |
|---|---|
| Input | <0.1 ms |
| Game logic + ECS | <6 ms |
| NPC schedules | <1 ms |
| Collision | <2 ms |
| Render (tile + sprite) | <6 ms |
| UI draw | <1 ms |
| **Total** | **~16 ms** |

---

## 4. Save System

### Design: Single Binary Save File

No JSON, no XML. One compact binary struct written directly to the SD card via devkitPro's stdio. Keep it small enough to fit in one SD write block.

```cpp
// core/SaveSystem.h

#pragma pack(push, 1)
struct SaveData {
    // Header (16 bytes)
    u32 magic;           // 0x4C4F4100 "LOA\0"
    u16 version;         // Save format version
    u16 playtime_hours;
    u32 checksum;        // CRC32 of remaining data
    u32 reserved;

    // Player state (64 bytes)
    char player_name[16];
    s16 pos_x, pos_y;
    u8  zone_id;
    u8  level;
    u32 gold;
    u8  hp_current, hp_max;
    u8  pad[36];

    // Skills (32 bytes) — one byte per skill, 0–255 XP level
    u8 skills[32];

    // Inventory (400 bytes) — 100 slots × 4 bytes each
    struct ItemSlot {
        u16 item_id;   // 0 = empty
        u8  quantity;
        u8  flags;     // equipped, flagged, etc.
    } inventory[100];

    // Equipment (12 bytes) — 6 equipment slots × 2 bytes
    u16 equipped[6];

// Quest state (128 bytes) — bitfield for 30 quests × 4 states
    // Each quest: NOT_STARTED, IN_PROGRESS, COMPLETE, FAILED
    u8 quest_state[64];
    u8 quest_flags[64];        // per-quest objective bits
    u8 quest_acknowledged[4];  // NEW: 1 bit per quest (30 quests fits in 4 bytes)

    // World flags (64 bytes) — world event/legacy bitfield
    u8 world_flags[64];

    // NPC relationship (50 bytes) — one signed byte per NPC
    s8 npc_relation[50];

    // Day/night (4 bytes)
    u16 game_day;
    u8  game_hour;
    u8  game_minute;
};
#pragma pack(pop)
// Total: ~780 bytes per save slot
```

**Three save slots = ~2.4 KB. Trivially small.**

### Save/Load Implementation

```cpp
bool SaveSystem::save(int slot, const SaveData& data) {
    char path[64];
    snprintf(path, sizeof(path), "sdmc:/loa/save%d.bin", slot);

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    SaveData out = data;
    out.magic = 0x4C4F4100;
    out.version = SAVE_VERSION;
    out.checksum = crc32(&out.player_name, sizeof(SaveData) - offsetof(SaveData, player_name));

    fwrite(&out, sizeof(SaveData), 1, f);
    fclose(f);
    return true;
}
```

**Checksum guards against corruption. Version field allows future migration.**

### Implementation Order
1. Define `SaveData` struct first (everything else depends on it)
2. Implement write/read/validate
3. Implement slot selection on title screen
4. Add autosave trigger on zone transition

### Risks
- Save corruption on power-off during write → write to temp file, then rename
- Struct packing differences across compiler versions → `#pragma pack(1)` + static assert on size

---

## 5. Entity Component Structure

### Architecture: Fixed-Size Pool, Struct-of-Arrays

No heap allocation during gameplay. All entities live in pre-allocated pools. Components are stored as parallel arrays — cache-friendly for iteration.

```cpp
// ecs/Components.h — all POD structs

struct TransformComponent {
    s16 x, y;          // world pixel position
    u8  zone_id;
    u8  facing;        // 0=down 1=up 2=left 3=right
};

struct SpriteComponent {
    u8  sheet_id;      // index into loaded sprite sheet
    u8  frame;         // current animation frame
    u8  anim_id;       // current animation sequence
    u8  anim_timer;    // frames until next frame
};

struct CollisionComponent {
    s8  ox, oy;        // offset from transform
    u8  w, h;          // hitbox size in pixels
    u8  flags;         // solid, trigger, etc.
};

struct StatsComponent {
    u8  hp, max_hp;
    u8  atk, def, spd;
    u8  level;
};

struct NPCComponent {
    u16 npc_id;
    u8  schedule_id;
    u8  dialogue_id;
    u8  state;         // IDLE, WALKING, INTERACTING
};

struct LootComponent {
    u16 item_id;
    u8  quantity;
    u8  flags;
};
```

```cpp
// ecs/EntityManager.h

static constexpr int MAX_ENTITIES = 128;  // never exceeded in one zone

class EntityManager {
public:
    // Parallel component arrays
    TransformComponent  transforms[MAX_ENTITIES];
    SpriteComponent     sprites[MAX_ENTITIES];
    CollisionComponent  colliders[MAX_ENTITIES];
    StatsComponent      stats[MAX_ENTITIES];
    NPCComponent        npcs[MAX_ENTITIES];
    LootComponent       loot[MAX_ENTITIES];

    u32 active_mask;     // bitmask of live entities (max 32; expand to u64 if needed)
    u8  component_mask[MAX_ENTITIES];  // which components this entity has

    int  createEntity();
    void destroyEntity(int id);
    bool isActive(int id) const;
};
```

**Memory: 128 entities × ~32 bytes average = ~4 KB. Negligible.**

### System Pattern

```cpp
// Systems iterate only over entities with required components
void moveSystem(EntityManager& em, float dt) {
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!em.isActive(id)) continue;
        if (!(em.component_mask[id] & COMP_TRANSFORM)) continue;
        // update transform...
    }
}
```

### Entity Lifecycle

- On **zone load**: populate entity pool from zone data (NPCs, enemies, items)
- On **zone unload**: clear entity pool; persist changed NPC states to zone cache
- Player entity is **never destroyed** — it persists across zones via `SaveData`

### Implementation Order
1. `EntityManager` + pool alloc
2. `TransformComponent` + movement
3. `SpriteComponent` + animation
4. `CollisionComponent` + tile collision
5. `StatsComponent` + combat entities
6. `NPCComponent` + dialogue trigger
7. `LootComponent` + item drops

---

## 6. World Map Architecture

### Architecture: Zone-Based Streaming

The world is divided into discrete zones (rooms/areas). Only one zone is loaded at a time. Zone transitions are triggered by walking into a border tile or activating a transition point.

**Zone size target: 40×30 tiles maximum = 640×480 px**  
Larger zones should be split.

```cpp
// world/Zone.h

static constexpr int ZONE_MAX_W = 40;
static constexpr int ZONE_MAX_H = 30;
static constexpr int TILE_SIZE  = 16;
static constexpr int LAYER_COUNT = 3;  // ground, overlay, decoration

struct ZoneHeader {
    u8  zone_id;
    u8  tileset_id;       // which tileset texture to load
    u8  width, height;    // in tiles
    u8  bgm_id;
    u8  ambient_light;    // base lighting level
    u8  num_transitions;  // exit points to other zones
    u8  num_entities;     // NPCs/enemies/items to spawn
};

struct TileTransition {
    u8  trigger_x, trigger_y;
    u8  dest_zone_id;
    u8  dest_x, dest_y;
    u8  facing;
};

struct EntitySpawn {
    u8  type;       // NPC, ENEMY, ITEM, CHEST
    u8  id;         // which NPC/enemy/item
    u8  x, y;
    u8  flags;      // respawn, one-time, etc.
};

// Zone file layout (binary):
// [ZoneHeader][TileTransition × N][EntitySpawn × M][TileData (layers × w × h)]
```

### TileMap

```cpp
// world/TileMap.h

class TileMap {
public:
    u8 layers[LAYER_COUNT][ZONE_MAX_H][ZONE_MAX_W];
    u8 collision[ZONE_MAX_H][ZONE_MAX_W];  // bitfield: solid, water, etc.

    void load(const u8* data, int w, int h);
    bool isSolid(int tx, int ty) const;
    u8   getTile(int layer, int tx, int ty) const;
};
```

**Memory:** 3 layers × 40×30 = 3,600 bytes + 1,200 collision bytes = ~5 KB per zone. Negligible.

### World Manager

```cpp
// world/World.h

class World {
public:
    void loadZone(u8 zone_id);
    void unloadCurrentZone();
    void checkTransitions(int player_x, int player_y);

private:
    Zone       current_zone;
    EntityManager entities;
    u8         zone_visited[256];   // bitfield — which zones visited
    s16        zone_cache_x[256];   // last player pos per zone (for return trips)
    s16        zone_cache_y[256];
};
```

### Tileset System

- Each tileset is a single 128×128 px texture = 256 tiles (16×16 each)
- One tileset texture loaded per zone (~64 KB VRAM)
- Multiple zones can share tilesets (town tiles reused across villages)
- Color palette swap: same tileset, different palette = visual variety at zero cost

### Camera

Simple centered camera with bounds clamping. No smooth scroll needed — pixel-lock camera is acceptable and simpler.

```cpp
void Camera::update(int player_x, int player_y, int map_w, int map_h) {
    cx = player_x - (SCREEN_W / 2);
    cy = player_y - (SCREEN_H / 2);
    cx = std::clamp(cx, 0, map_w * TILE_SIZE - SCREEN_W);
    cy = std::clamp(cy, 0, map_h * TILE_SIZE - SCREEN_H);
}
```

### Implementation Order
1. `TileMap` load/draw
2. Camera
3. `TileTransition` triggers
4. `World::loadZone` / `unloadCurrentZone`
5. Entity spawning from zone data
6. Visited zone tracking

### Performance
- Tile rendering: draw only tiles in viewport (25×15 = 375 tiles × 3 layers = 1,125 draw calls per frame)
- citro2d sprite batching reduces this to a few draw calls if tiles are on one texture
- No dynamic tile modification needed in v1 — tiles are read-only

---

## 7. Quest System

### Architecture: Data-Driven, Bitfield State

Quests are defined in static data. Runtime state is a compact bitfield in `SaveData`. No dynamic quest objects are allocated.

```cpp
// data/QuestDB.h

enum class QuestState : u8 {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETE    = 2,
    FAILED      = 3
};

struct QuestObjective {
    u8  type;      // KILL, COLLECT, TALK, REACH, CRAFT
    u16 target_id; // enemy_id, item_id, npc_id, zone_id
    u8  count;     // required count
    u8  flag_bit;  // which bit in quest_flags tracks this objective
};

struct QuestOutcome {
    u8  required_flag_mask;     // which objective_flags bits trigger this outcome
    u16 reward_item_id;
    u8  reward_quantity;
    u32 reward_gold;
    u8  world_flag_bit;
    s8  npc_relation_delta;     // reputation change with giver_npc_id
    u8  affected_npc_count;     // 0-3 additional NPCs affected
    u8  affected_npc_id[3];
    s8  affected_npc_delta[3];
};

struct QuestDef {
    u16 quest_id;
    u8  giver_npc_id;
    u8  num_objectives;
    QuestObjective objectives[4];  // max 4 objectives per quest
    u8  outcome_count;             // how many distinct resolutions (1-3)
    QuestOutcome outcomes[3];
    u8  unlock_quest_id;    // quest unlocked on completion (0 = none)
};
```

```cpp
// gameplay/QuestManager.h

class QuestManager {
public:
    QuestState getState(u8 quest_id) const;
    void       setState(u8 quest_id, QuestState s);
    bool       isObjectiveMet(u8 quest_id, u8 obj_index) const;
    void       setObjectiveFlag(u8 quest_id, u8 flag_bit);
    void       onKill(u16 enemy_id);        // called by combat
    void       onPickup(u16 item_id);       // called by inventory
    void       onTalkTo(u16 npc_id);        // called by dialogue
    void       checkAutoComplete(u8 quest_id);
    u8         resolveOutcome(u8 quest_id) const;        // picks matching outcome index
    void       applyOutcome(u8 quest_id, u8 outcome_idx); // grants rewards, sets flags, adjusts relations
    bool       isAcknowledged(u8 quest_id) const;         // has NPC commented on completion yet
    void       setAcknowledged(u8 quest_id);

private:
    SaveData* save;  // direct reference — no copy
};
```

**All state lives in `SaveData::quest_state` and `quest_flags` — no separate quest runtime objects.**

### Quest Flow

```
NPC dialogue trigger
    → QuestManager::startQuest()
    → setState(IN_PROGRESS)
    → notify player UI

[gameplay events call onKill / onPickup / onTalkTo]
    → setObjectiveFlag()
    → checkAutoComplete()

Player returns to NPC
    → dialogue checks getState() == IN_PROGRESS + all flags set
    → setState(COMPLETE)
    → grant rewards
    → set world_flag_bit
    → unlock next quest
```

### Implementation Order
1. `QuestDef` data structs + `QuestDB` loader
2. `QuestManager` bitfield get/set
3. Hook `onKill`, `onPickup`, `onTalkTo` into their respective systems
4. Quest UI (active quests list on bottom screen)
5. Reward granting

### Memory
- 30 quests × ~20 bytes = 600 bytes in ROM (static table)
- Runtime state: 128 bytes in `SaveData` (already accounted for)

---

## 8. Inventory System

### Architecture: Fixed Slot Array

100 slots, each 4 bytes. No dynamic allocation. Items are defined in a static lookup table.

```cpp
// data/ItemDB.h

enum class ItemType : u8 {
    CONSUMABLE, WEAPON, ARMOR, ACCESSORY,
    MATERIAL, KEY_ITEM, AMMO
};

struct ItemDef {
    u16  item_id;
    char name[20];
    u8   type;
    u8   icon_tile;    // index in item sprite sheet
    s8   atk_bonus;
    s8   def_bonus;
    s8   hp_restore;
    u16  buy_price;
    u16  sell_price;
    u8   flags;        // stackable, equippable, quest_item
};

// 150 items × 32 bytes = ~4.7 KB in ROM
```

```cpp
// gameplay/Inventory.h

static constexpr int INV_SLOTS = 100;

class Inventory {
public:
    bool addItem(u16 item_id, u8 quantity = 1);
    bool removeItem(u16 item_id, u8 quantity = 1);
    bool hasItem(u16 item_id, u8 quantity = 1) const;
    int  findSlot(u16 item_id) const;
    int  findEmptySlot() const;
    void sortInventory();

    SaveData::ItemSlot slots[INV_SLOTS];  // direct pointer to save data
};
```

**`slots` is a direct pointer into `SaveData::inventory` — no duplication, no sync needed.**

### Equipment

Six equipment slots: Head, Body, Hands, Feet, Weapon, Offhand.

```cpp
void equipItem(int inv_slot) {
    u16 item_id = slots[inv_slot].item_id;
    const ItemDef& def = ItemDB::get(item_id);
    if (!(def.flags & ITEM_EQUIPPABLE)) return;

    u8 equip_slot = getEquipSlot(def.type);
    u16 current = save->equipped[equip_slot];
    if (current) unequipSlot(equip_slot);     // auto-unequip
    save->equipped[equip_slot] = item_id;
    slots[inv_slot].flags |= FLAG_EQUIPPED;
    recalcStats();
}
```

### Shop System

```cpp
// gameplay/ShopSystem.h

struct ShopDef {
    u8   shop_id;
    u8   num_items;
    u16  item_ids[20];   // what this shop sells
    u8   restock_day;    // restock every N days
};

class ShopSystem {
public:
    void openShop(u8 shop_id);
    void buy(u8 shop_slot, u8 quantity);
    void sell(u8 inv_slot);
};
```

Buy/sell is pure arithmetic on `SaveData::gold` + `Inventory::addItem/removeItem`. No complexity needed.

### Implementation Order
1. `ItemDef` + `ItemDB` loader
2. `Inventory` slot management
3. Item pickup from world
4. Equipment + stat recalc
5. Inventory UI (bottom screen grid)
6. Shop UI + buy/sell

---

## 9. Combat System

### Architecture: Real-Time, Turn-Resolved

Fully real-time movement. Combat resolution is a simple formula — no action queue, no turn order engine. Player swings weapon on button press; hit detection is AABB against enemy hitboxes.

```cpp
// gameplay/Combat.h

struct CombatResult {
    bool   hit;
    u8     damage;
    bool   critical;
    bool   killed_enemy;
    u16    killed_enemy_id;
};

class Combat {
public:
    CombatResult playerAttack(EntityManager& em, int player_id);
    void         enemyAttack(EntityManager& em, int enemy_id, int player_id, float dt);
    void         applyDamage(EntityManager& em, int target_id, u8 damage);

private:
    u8 calcDamage(u8 atk, u8 def) const;
    bool checkHit(const CollisionComponent& weapon_box,
                  const CollisionComponent& target_box) const;
};
```

### Damage Formula

Keep it simple and predictable:

```cpp
u8 Combat::calcDamage(u8 atk, u8 def) const {
    int base = (int)atk - (int)(def / 2);
    if (base < 1) base = 1;
    // ±20% variance
    int variance = (rand() % (base / 5 + 1));
    return (u8)std::min(base + variance, 255);
}
```

### Weapon Hitbox

Player attack spawns a **single-frame hitbox** in front of the player based on facing direction. No projectile entity needed for melee.

```cpp
void Combat::playerAttack(EntityManager& em, int player_id) {
    TransformComponent& t = em.transforms[player_id];
    // Build a one-frame hitbox tile in front of player
    int hx = t.x + facing_offsets[t.facing].x;
    int hy = t.y + facing_offsets[t.facing].y;
    // Check all enemy entities for overlap with (hx, hy, 16, 16)
    // ...
}
```

### Enemy AI

State machine per enemy: IDLE → PATROL → CHASE → ATTACK → FLEE.  
Implementation: simple distance checks, no pathfinding in v1.

```cpp
enum class EnemyState : u8 { IDLE, PATROL, CHASE, ATTACK };

void updateEnemy(EntityManager& em, int id, int player_id, float dt) {
    float dist = distance(em.transforms[id], em.transforms[player_id]);
    EnemyState& state = /* ... */;

    if      (dist < ATTACK_RANGE) state = EnemyState::ATTACK;
    else if (dist < CHASE_RANGE)  state = EnemyState::CHASE;
    else                          state = EnemyState::IDLE;

    switch (state) {
        case EnemyState::CHASE:  moveToward(em, id, player_id, dt); break;
        case EnemyState::ATTACK: combat.enemyAttack(em, id, player_id, dt); break;
        default: break;
    }
}
```

**No A* in v1.** Enemies navigate by direct movement with wall sliding. Sufficient for small zone rooms.

### Skill Progression Hook

```cpp
// After successful player attack:
if (result.hit) {
    skills.addXP(SKILL_COMBAT, 1);
    if (result.killed_enemy) {
        skills.addXP(SKILL_COMBAT, 5);
        questManager.onKill(killed_enemy_id);
        spawnLoot(em, enemy_pos, enemy_def.loot_table);
    }
}
```

### Implementation Order
1. Player hitbox spawn on button press
2. AABB hit detection against enemy colliders
3. Damage formula + HP reduction
4. Enemy death + loot spawn
5. Enemy AI state machine
6. Knockback effect (simple velocity impulse, 2–3 frames)
7. Player damage reception + iframe window

---

## 10. NPC Scheduling

### Architecture: Schedule Table, Time-Driven State

NPCs follow a simple daily schedule: a list of (time, destination, animation) entries. Evaluated once per in-game hour — no continuous pathfinding.

```cpp
// npc/NPCScheduler.h

struct ScheduleEntry {
    u8  hour;           // 0–23
    u8  dest_x, dest_y; // tile position target
    u8  anim_id;        // STAND, SIT, WORK, SLEEP
    u8  zone_id;        // which zone (NPCs can "leave" to interior zones)
};

struct NPCScheduleDef {
    u8  schedule_id;
    u8  num_entries;
    ScheduleEntry entries[8];  // max 8 schedule points per NPC
};
```

```cpp
class NPCScheduler {
public:
    void update(EntityManager& em, u8 current_hour);

private:
    const ScheduleEntry* getActiveEntry(u8 npc_schedule_id, u8 hour) const;
    void moveNPCToward(EntityManager& em, int npc_id,
                       u8 dest_x, u8 dest_y, float dt);
};
```

### Update Strategy

- Evaluate schedule **once per in-game hour** transition, not every frame
- NPCs in the current zone: update their `dest` target
- NPCs in other zones: skip (they teleport to their scheduled position on zone load)
- NPCs walk toward their target tile each frame (simple step-by-step, no A*)

```cpp
void NPCScheduler::update(EntityManager& em, u8 current_hour) {
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (!em.isActive(id)) continue;
        if (!(em.component_mask[id] & COMP_NPC)) continue;

        NPCComponent& npc = em.npcs[id];
        const ScheduleEntry* entry = getActiveEntry(npc.schedule_id, current_hour);
        if (!entry) continue;

        npc.target_x = entry->dest_x;
        npc.target_y = entry->dest_y;
        npc.current_anim = entry->anim_id;
    }
}
```

**CPU impact: ~50 NPCs, evaluated 24 times/day = 1,200 lookups/day. Negligible.**

### Day/Night Cycle

```cpp
// core/Clock.h

class Clock {
public:
    void update(float dt);
    u8   getHour() const;
    u8   getMinute() const;
    bool isNight() const;   // hour >= 20 || hour < 6
    bool hourChanged();     // true for one frame when hour ticks over

private:
    float seconds_per_game_minute = 2.0f; // 1 real second = 0.5 game minutes
    float accumulator;
    u16   game_day;
    u8    hour, minute;
    bool  hour_changed;
};
// 24-hour day = 48 real minutes. Adjust seconds_per_game_minute to taste.
```

**Shop open/close:** Shops check `clock.getHour()` on open attempt. If `hour < 8 || hour >= 20`, refuse sale and show "closed" message.

### Implementation Order
1. `Clock` update + save/load
2. `ScheduleDef` data + loader
3. `NPCScheduler::update` on hour tick
4. NPC movement toward target
5. Shop hour check
6. Day/night visual (ambient light value passed to renderer)

---

## 11. Rendering Pipeline

### Architecture: citro2d Layered Sprite Rendering

citro2d handles 2D sprite batching on top of citro3d. The render pipeline is:

```
Bottom Screen (320×240):     Top Screen (400×240):
  UI background                Ground layer tiles
  Map minimap                  Overlay layer tiles
  Stats bars                   Entity sprites (sorted by Y)
  Active quest                 Decoration layer tiles
  Hotbar                       UI overlays (damage numbers)
```

### Render Order (top screen)

```cpp
void renderGameplay() {
    C2D_TargetClear(top_screen, C2D_Color32(0,0,0,255));
    C2D_SceneBegin(top_screen);

    // 1. Ground tiles
    tileRenderer.drawLayer(tilemap, 0, camera);

    // 2. Overlay tiles (walls, objects below player)
    tileRenderer.drawLayer(tilemap, 1, camera);

    // 3. Entities (Y-sorted: lower Y = drawn first = behind)
    renderEntitiesSorted(entityManager, camera);

    // 4. Decoration tiles (things drawn on top of sprites: rooftops, tree canopy)
    tileRenderer.drawLayer(tilemap, 2, camera);

    // 5. Combat effects (damage numbers, hit sparks)
    effectRenderer.draw(camera);

    C2D_Flush();
}
```

### Tile Renderer

```cpp
void TileRenderer::drawLayer(const TileMap& map, int layer, const Camera& cam) {
    // Calculate visible tile range
    int tx0 = cam.x / TILE_SIZE;
    int ty0 = cam.y / TILE_SIZE;
    int tx1 = std::min(tx0 + VIEWPORT_W + 1, map.width);
    int ty1 = std::min(ty0 + VIEWPORT_H + 1, map.height);

    for (int ty = ty0; ty < ty1; ty++) {
        for (int tx = tx0; tx < tx1; tx++) {
            u8 tile = map.getTile(layer, tx, ty);
            if (tile == 0) continue;  // transparent tile

            float sx = (tile % 16) * TILE_SIZE;  // source x in tileset
            float sy = (tile / 16) * TILE_SIZE;  // source y in tileset
            float dx = tx * TILE_SIZE - cam.x;   // dest x on screen
            float dy = ty * TILE_SIZE - cam.y;   // dest y on screen

            C2D_DrawImageAt(tileset_img, dx, dy, 0.5f,
                            nullptr, 1.0f, 1.0f);
            // Note: use C2D_DrawSpriteTinted for palette effects
        }
    }
}
```

### Entity Sprite Rendering

Entities are Y-sorted each frame (insertion sort on ~50 entities = fast enough):

```cpp
void renderEntitiesSorted(EntityManager& em, const Camera& cam) {
    // Build sorted index array (stack-allocated)
    int sorted[MAX_ENTITIES];
    int count = 0;
    for (int id = 0; id < MAX_ENTITIES; id++) {
        if (em.isActive(id) && (em.component_mask[id] & COMP_SPRITE))
            sorted[count++] = id;
    }
    // Insertion sort by Y position
    for (int i = 1; i < count; i++) {
        int key = sorted[i];
        int j = i - 1;
        while (j >= 0 && em.transforms[sorted[j]].y > em.transforms[key].y) {
            sorted[j+1] = sorted[j--];
        }
        sorted[j+1] = key;
    }
    // Draw in order
    for (int i = 0; i < count; i++) drawEntity(em, sorted[i], cam);
}
```

### Animation System

```cpp
// Each entity has anim_id (which animation) + frame + anim_timer
// Animation table maps anim_id → sequence of sprite sheet indices + frame durations

struct AnimFrame {
    u8 sprite_index;  // index in sprite sheet (32×32 grid)
    u8 duration;      // frames to hold
};

struct AnimDef {
    u8        num_frames;
    bool      loop;
    AnimFrame frames[8];  // max 8 frames per anim
};
```

Frame advance runs every entity update:

```cpp
if (--sprite.anim_timer == 0) {
    sprite.frame = (sprite.frame + 1) % anim_def.num_frames;
    sprite.anim_timer = anim_def.frames[sprite.frame].duration;
}
```

### Memory (VRAM budget)

| Asset | Size |
|---|---|
| Active tileset (128×128 RGBA8) | 64 KB |
| Character sprite atlas (256×128 RGBA8) | 128 KB |
| Enemy sprite atlas (256×128 RGBA8) | 128 KB |
| UI atlas (128×64 RGBA8) | 32 KB |
| Item icon atlas (128×128 RGBA8) | 64 KB |
| **Total** | **~416 KB** (well within 6 MB VRAM limit) |

Use `GPU_RGBA4` or `GPU_RGB565` formats for non-transparent textures to halve size.

---

## 12. Asset Management

### Philosophy: Load Once Per Zone, Never During Gameplay

Texture loads happen only on zone transitions (masked by a fade screen). No runtime texture swapping.

```cpp
// render/AssetManager.h — thin wrapper, not a complex resource system

class AssetManager {
public:
    void loadTileset(u8 tileset_id);
    void loadSpriteSheet(u8 sheet_id);
    void unloadAll();

    C2D_SpriteSheet tilesets[4];     // max 4 tilesets loaded (usually 1)
    C2D_SpriteSheet sprite_sheets[4]; // characters + enemies + UI + items
};
```

### Zone Transition Load Sequence

```
1. Fade to black (0.3 seconds)
2. unloadCurrentZone() → releases entity pool
3. AssetManager::unloadAll() → frees VRAM
4. Load new zone file from romfs
5. AssetManager::loadTileset(zone.tileset_id)
6. Load NPC/enemy sprite sheets if different from current
7. EntityManager: spawn entities from zone's entity list
8. Fade in
```

### Static Data Tables (loaded once at boot)

All game data (items, enemies, quests, NPC schedules) is loaded at startup into BSS/heap:

```cpp
// ~15 KB total for all static tables — load once, never free
ItemDB::load("romfs:/data/items.bin");       // 150 items × 32 bytes = 4.8 KB
EnemyDB::load("romfs:/data/enemies.bin");    // 20 enemies × 48 bytes = 0.96 KB
QuestDB::load("romfs:/data/quests.bin");     // 30 quests × 64 bytes = 1.9 KB
```

### Tileset Strategy

- **1 tileset = 1 texture** (128×128 px = 256 tiles of 16×16)
- Target tilesets for v1:
  - `tileset_town.t3x` — cobblestone, buildings, market stalls (shared: capital + villages)
  - `tileset_forest.t3x` — grass, trees, path, shrubs
  - `tileset_mountain.t3x` — rock, snow, cave entrance
  - `tileset_dungeon.t3x` — stone walls, torches, doors (shared across all 4 dungeons)
- **4 tilesets total** — draw the entire world from 4 textures

### Sprite Sheet Strategy

- `sprites_player.t3x` — 32×32 sprites, all player animations (walk 4-dir × 4 frames, attack, etc.)
- `sprites_npc.t3x` — all 50 NPC appearances (use palette variation for cheap recoloring)
- `sprites_enemies.t3x` — all 20 enemy types
- `sprites_items.t3x` — 16×16 item icons for inventory UI

**NPC visual variety from one sheet:** NPCs share body/walk animations; use `C2D_DrawSpriteTinted` to apply hair/clothing color from a per-NPC palette. This gives visual diversity without extra sprites.

### Audio

- BGM: `.bcstm` streaming format (decoded on-the-fly, ~4 MB working buffer)
- SFX: `.bcwav` (small, loaded into RAM at zone load)
- Target: 1 BGM per region (5 tracks total), ~20 SFX
- Use `ndsp` (Nintendo DSP) via devkitPro for audio

---

## 13. Milestone Roadmap

### Phase 0 — Foundation (Weeks 1–4)
Goal: Triangle on screen, input working, main loop stable.

- [ ] devkitPro project scaffold (Makefile, 3DS target)
- [ ] citro2d init + basic sprite draw
- [ ] Input manager (D-pad, buttons, touch)
- [ ] Game state machine (2 states: TITLE, GAMEPLAY)
- [ ] Clock + delta time
- [ ] Logger (debug build only)
- [ ] Basic file I/O from romfs

**Exit criteria:** Sprite moves on screen, controlled by D-pad.

---

### Phase 1 — World (Weeks 5–9)
Goal: A single zone loads and is explorable.

- [ ] TileMap load from binary file
- [ ] Tile renderer (ground layer)
- [ ] Camera with bounds
- [ ] Player entity: transform, sprite, movement
- [ ] Tile collision
- [ ] Zone transition (hardcoded test zones)
- [ ] Overlay + decoration tile layers
- [ ] Basic entity pool

**Exit criteria:** Player walks around a tile map, transitions between 2 zones.

---

### Phase 2 — Save & Data (Weeks 10–12)
Goal: Game can be saved and loaded.

- [ ] `SaveData` struct finalized
- [ ] Save/load to SD card
- [ ] Title screen with slot selection
- [ ] Static data tables: `ItemDB`, `EnemyDB`, `QuestDB` (with placeholder data)
- [ ] Data compiler tool (CSV → .bin)

**Exit criteria:** Walk around, save, reload — player position persists.

---

### Phase 3 — Combat (Weeks 13–17)
Goal: Enemies exist and can be killed.

- [ ] Enemy entity: stats, sprite, AI state machine
- [ ] Player attack hitbox (melee)
- [ ] AABB hit detection
- [ ] Damage formula + HP bars
- [ ] Enemy death + loot drop
- [ ] Skill XP gain on kill
- [ ] Player damage + iframe window
- [ ] Basic combat feedback (hit flash, damage number)

**Exit criteria:** Player fights, kills, and loots 3 enemy types in 1 dungeon zone.

---

### Phase 4 — NPC & Dialogue (Weeks 18–22)
Goal: NPCs exist, talk, and follow schedules.

- [ ] NPC entity spawn from zone data
- [ ] Dialogue tree runner + UI
- [ ] NPC interaction trigger
- [ ] Clock day/night cycle (visual)
- [ ] NPC schedule system (hour-based)
- [ ] Shop NPC: buy/sell UI

**Exit criteria:** Player talks to 5 NPCs, buys an item, sees NPCs move on schedule.

---

### Phase 5 — Quests & Inventory (Weeks 23–27)
Goal: Quests can be accepted and completed.

- [ ] Inventory UI (grid, equip, drop)
- [ ] Equipment slot system + stat recalc
- [ ] Quest accept/track/complete flow
- [ ] Quest UI (active list on bottom screen)
- [ ] Item pickup from world
- [ ] Crafting system (recipe table + UI)
- [ ] Minimap on bottom screen

**Exit criteria:** Accept quest, kill enemies, collect items, return, complete quest, receive reward.

---

### Phase 6 — World Build (Weeks 28–40)
Goal: Full v1 content authored.

- [ ] Capital city zone set (3–4 zones)
- [ ] Village 1 + Village 2 zone sets
- [ ] Forest biome zones
- [ ] Mountain biome zones
- [ ] 4 dungeon zone sets
- [ ] 50 NPCs authored + scheduled
- [ ] 30 quests authored
- [ ] 20 enemy types balanced
- [ ] 150 items authored
- [ ] All tilesets finalized
- [ ] All sprite sheets finalized
- [ ] BGM + SFX authored

**This phase is the longest and the highest risk. Protect it from scope creep.**

---

### Phase 7 — Polish & Alpha (Weeks 41–48)
Goal: Playable from start to finish without crashing.

- [ ] Full play-through test
- [ ] Bug fix pass
- [ ] Save/load edge case hardening
- [ ] Performance profiling on Old 3DS hardware
- [ ] Audio integration + volume controls
- [ ] Settings menu (text speed, sound on/off)
- [ ] Credits screen
- [ ] Release build (stripped logging, optimized)

**Exit criteria:** Game boots on physical 3DS hardware, completes without crash, save persists.

---

## Appendix: System Priority Summary

| System | v1 Include | Risk | Complexity |
|---|---|---|---|
| Game state machine | ✅ Required | Low | Low |
| Save system | ✅ Required | Medium | Low |
| Tile map + renderer | ✅ Required | Low | Low |
| Player movement | ✅ Required | Low | Low |
| Collision | ✅ Required | Low | Medium |
| Zone transitions | ✅ Required | Low | Low |
| Combat | ✅ Required | Medium | Medium |
| Inventory | ✅ Required | Low | Low |
| Equipment | ✅ Required | Low | Low |
| Dialogue | ✅ Required | Low | Medium |
| Quests | ✅ Required | Medium | Medium |
| NPC schedules | ✅ Required | Low | Low |
| Shops | ✅ Required | Low | Low |
| Skill progression | ✅ Required | Low | Low |
| Crafting | ✅ Required | Low | Low |
| Day/night cycle | ✅ Required | Low | Low |
| Enemy AI (simple) | ✅ Required | Low | Low |
| Minimap | ✅ Required | Low | Low |
| Pathfinding (A*) | ❌ Defer | High | High |
| Multiplayer | ❌ Never in v1 | Very High | Very High |
| Dynamic lighting | ❌ Defer | Medium | High |
| Mount/vehicle system | ❌ Defer | Medium | High |
| Housing system | ❌ Defer | Medium | High |
