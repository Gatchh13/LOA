# Legends of Aetheria — Road to a Playable Demo
**Document version:** 1.0  
**Codebase state:** Milestone 5 complete (SaveData v5)  
**Target hardware:** Nintendo 3DS (Old 3DS compatible), Azahar emulator

---

## Section 1: Current State Assessment

### Systems Currently Present and Functional

| System | File(s) | Notes |
|--------|---------|-------|
| Player movement | `entities/Player.h/.cpp` | 8-dir, frame-rate independent, D-Pad + Circle Pad |
| Tile collision | `world/Collision.h/.cpp` | Axis-separated AABB, correct wall-slide |
| Zone loading | `world/ZoneManager.h/.cpp` | Fade transition, spawn points, name banner |
| Zone transitions | `world/ZoneData.h` | 3 zones: Town, Forest, Dungeon Entrance |
| TileMap w/ overrides | `world/TileMap.h/.cpp` | Runtime override layer, 16-entry table |
| Camera | `render/Camera.h/.cpp` | Follows player, clamps to map bounds |
| World clock | `core/WorldClock.h/.cpp` | 24-hr, configurable TIME_SCALE |
| Day/night cycle | `world/DayNight.h/.cpp` | 4 phases, smooth tint transitions |
| NPC framework | `npc/NPC.h` | Struct: id, name, dialogue, schedule, position |
| NPC schedules | `npc/NPCManager.h/.cpp` | 3 NPCs, hour-driven L-path movement |
| Dialogue system | `npc/NPCManager.cpp` + Renderer | Static single-line, A to open/close |
| Quest framework | `quest/QuestManager.h/.cpp` | Linear steps: TALK, REACH_MARKER, RETURN |
| Quest data | `quest/QuestDef.h` | "The Missing Package" — 3-step proof-of-concept |
| World objects | `world/WorldObject.h` + Manager | Bridge, Ladder, Obstacle — repair mechanic |
| PlayerState | `quest/PlayerState.h` | gold, wood, rope — test values hardcoded |
| Save / Load | `save/SaveManager.h/.cpp` + `SaveData.h` | Atomic write, CRC32, 236-byte file |
| Renderer | `render/Renderer.h/.cpp` | citro2d, fallback colors, top+bottom screen |
| Input | `input/InputManager.h/.cpp` | All buttons, Circle Pad with dead zone |
| FPS / debug | `core/Clock.h/.cpp` | Delta time, FPS counter |
| Logger | `core/Logger.h/.cpp` | Debug-build printf, stripped in release |

### Systems Partially Present

| System | What Exists | What Is Missing |
|--------|-------------|-----------------|
| **Inventory** | `PlayerState` has `gold`, `wood`, `rope` (3 resources) | No item definitions, no item slots, no UI screen, no pickup system, no item-based quest rewards |
| **Dialogue** | Single static line per NPC; quest override (2 lines split on `\n`) | No branching, no multi-page text, no text scroll, no portrait |
| **Quest rewards** | Gold granted via `PlayerState::addGold()` | No item rewards; quest reward struct has only `gold` field |
| **Gathering** | `PlayerState` has `wood` and `rope` with test starting values | No resource nodes, no gather action, no respawn logic |
| **World object visuals** | Placeholder colored rectangles in `Renderer.cpp` | No sprites; described as "placeholder" throughout |
| **NPC visuals** | 14×14 blue square with white dot | No distinct NPC sprites, no directional facing |
| **Player visuals** | 14×14 red square with white dot | No animation frames, no directional sprite |
| **Zone content** | 3 zones with functional tile maps | Dungeon Entrance has no content (no enemies, no items, no second exit) |
| **Bottom screen HUD** | Quest objective + gold/wood/rope display | No map, no minimap, no equipment display |
| **Audio** | Nothing | No BGM, no SFX — requires ndsp setup |

### Systems Missing Entirely

| System | Why It Blocks the Demo |
|--------|------------------------|
| **Gathering / resource nodes** | Player has test values (wood=20, rope=10) hardcoded; no way to acquire more after spending |
| **Inventory (item slots)** | Quest rewards beyond gold cannot be delivered; shops have no stock; crafting impossible |
| **Item definitions** | No `ItemDef` table; no `item_id` concept exists anywhere |
| **Enemy system** | Dungeon Entrance is a dead end; no combat, no threat |
| **Combat** | No attack input, no damage model, no enemy AI combat states |
| **Crafting** | No crafting screen, no recipe table |
| **Shop / economy** | Merchant NPC cannot sell anything |
| **Minimap** | Player has no orientation aid in larger zones |
| **Audio** | Complete silence — significant to perceived quality |
| **Title screen** | Game boots directly into gameplay; no new/continue flow |
| **Pixel art assets** | All visuals are colored rectangles; `.t3x` sprite sheet optional but missing |

### Dependency Graph (critical path only)

```
Title Screen
    └─ [no dependencies — pure UI]

Gathering System
    └─ requires: resource node data in ZoneData
    └─ blocks: sustainable resource economy

Inventory (item slots)
    └─ requires: ItemDef table
    └─ blocks: item rewards, shops, crafting, equipment

Item Definitions
    └─ no code dependencies
    └─ blocks: everything above

Quest item rewards
    └─ requires: inventory item slots
    └─ requires: item definitions

Enemy System
    └─ requires: entity pool (already exists via NPC pattern)
    └─ requires: combat damage model
    └─ blocks: dungeon content, KILL quest objectives

Combat
    └─ requires: enemy system
    └─ requires: player attack hitbox
    └─ blocks: any meaningful dungeon experience

Audio
    └─ requires: ndsp init + .bcstm/.bcwav assets
    └─ blocks: nothing technically, but critical for demo feel

Pixel art assets
    └─ requires: tex3ds pipeline (already documented)
    └─ blocks: nothing technically, but demo-defining
```

---

## Section 2: Vertical Slice Definition

### Target experience (exact)

> Start game → walk around town → talk to NPCs → accept quest → travel to forest → complete objective → return to town → receive reward → save → load → world state persists

This loop is **already mechanically complete** in Milestone 5. The quest runs. The save persists. The zone transitions work. The NPCs schedule and move.

### What prevents it from feeling like a real game

The loop works but the demo fails on perceived quality. Ranked by impact:

1. **No title screen** — game boots into a world with no context.
2. **Placeholder art** — colored squares destroy immersion immediately. A player handed this cannot tell NPCs from the player from objects.
3. **No audio** — silence is the most damaging single gap for demo feel.
4. **Hardcoded test resources** — `wood=20, rope=10` are magic numbers. There is no way to get more. This is fine for testing but breaks any demo where a player exhausts materials.
5. **Merchant cannot sell anything** — the quest loop ends with gold but gold does nothing.
6. **Dungeon Entrance is a dead end** — leads nowhere, contains nothing, creates a misleading sense of incompleteness.
7. **Single quest** — "The Missing Package" works, but there's nothing to do next.

### Exact systems required for the vertical slice

These are the **only** things that need to exist for a meaningful demo handoff:

| System | Minimum viable form |
|--------|---------------------|
| Title screen | New Game / Continue selection, 2 options, no animation required |
| Pixel art | Player sprite (4-dir walk, 2 frames), 3 NPC appearances, grass/wall/dirt tiles only |
| Gathering nodes | 3–4 wood piles in Forest; tap A to collect; respawn on zone re-entry |
| Basic inventory | 8 item slots, display on bottom screen — no equipment, no sorting |
| Second quest | A second 3-step quest to provide "what do I do next" after the first completes |
| Shop transaction | Mira can sell 1–2 items for gold; buy screen on bottom; no complex UI |
| Audio (BGM only) | 1 town track, 1 forest track, streaming — silence is worse than 8-bit placeholder |
| SFX (3 sounds) | footstep, interact, repair — covers all primary player actions |
| Minimap (simple) | Bottom screen: zone name + player dot on a 64×48 black rectangle showing walls |

Everything else — crafting, equipment, dungeon combat, skills, reputation, housing — is post-demo.

---

## Section 3: Milestone Reordering

### Current milestone order

```
M1 World Exploration     ✓
M2 Living Town           ✓
M3 Quest Framework       ✓
M4 Infrastructure        ✓
M5 Save / Load           ✓
M6 [unspecified]
M7 [unspecified]
M8 [unspecified]
```

### Assessment of M1–M5 ordering

M1–M4 are correctly ordered. Each milestone required the previous one to be meaningful. M5 save/load is also correctly placed — it needed the full state surface (quests, world objects, resources) before a useful save struct could be defined.

### Recommended M6–M8 ordering

The naive continuation would be combat → inventory → crafting. This is **wrong for a demo**. Combat requires enemies, enemy AI, sprites, hitboxes, balance — all high-risk, high-scope work that does not contribute to the specific vertical slice defined above.

The correct order prioritizes **closing the demo loop** over expanding the game:

```
M6: Title Screen + Sprite Pipeline + Gathering Nodes
M7: Basic Inventory + Shop + Second Quest
M8: Audio + Minimap + Demo Polish Pass
```

**Why this ordering:**

- **M6 first** because placeholder art and no title screen are the two things that break the demo experience the hardest with zero warning. Gathering nodes also eliminate the hardcoded test values, which must be removed before any meaningful playtesting.

- **M7 second** because once resources can be gathered, the player needs somewhere to spend them (shop). The second quest gives them a reason to keep moving after "The Missing Package" completes. Inventory is the minimal plumbing both require.

- **M8 third** because audio and minimap are polish — they make the already-functional demo feel finished. They have no blocking dependencies and carry the lowest implementation risk.

**What this defers deliberately:**

- Combat (post-demo)
- Enemy system (post-demo)
- Crafting (post-demo)
- Equipment (post-demo)
- Skills (post-demo)
- Factions / reputation (post-demo)

These are not cut — they are sequenced after the demo milestone is validated.

---

## Section 4: Next Three Milestones

---

### Milestone 6 — Foundation of Feel
**Goal:** Make the game look and behave like a real game. Remove all placeholder barriers to demo handoff.

**Subgoals:**
1. Title screen with New Game / Continue
2. Pixel art pipeline: player walk cycle, 3 NPC appearances, tile sprites
3. Gathering nodes: wood piles in Forest, collectible with A, resource counts update
4. Remove hardcoded `wood=20, rope=10` test values from `PlayerState` constructor

**Files added:**

| File | Purpose |
|------|---------|
| `source/ui/TitleScreen.h/.cpp` | State machine: TITLE, NEW_GAME, CONTINUE; queries `SaveManager::hasSave()` |
| `source/world/GatherNode.h` | Struct: id, zone, tile position, resource type, quantity, respawn flag |
| `source/world/GatherNodeManager.h/.cpp` | Owns nodes, checks A-press proximity, updates PlayerState, re-populates on zone load |
| `romfs/gfx/tiles.t3x` | First real sprite sheet: grass, wall, dirt, player 4-dir, 3 NPC bodies |
| `romfs/gfx/player.t3x` | Player walk cycle (4 directions × 2 frames = 8 sprites, 16×16 each) |

**Files modified:**

| File | Change |
|------|--------|
| `source/core/GameState.h` | Add `TITLE` state; route to `TitleScreen` on boot |
| `source/main.cpp` | Boot → TITLE → GAMEPLAY flow; wire GatherNodeManager |
| `source/quest/PlayerState.h` | Remove test starting values; `wood=0, rope=0` default |
| `source/render/Renderer.h/.cpp` | Add `drawGatherNodes()`, improve `drawNPCs()` and `drawPlayer()` for sprite sheet |
| `source/save/SaveData.h` | Add `u8 gather_node_states[16]` in `reserved_inventory` region; **SAVE_VERSION → 6** |

**Save compatibility impact:**
Breaking change — `SAVE_VERSION` bumps from 5 to 6. M5 saves are rejected by validation and fall back to new game. Acceptable: no real-player saves exist yet.

The `reserved_inventory[64]` block absorbs the gather node state (16 bytes), leaving 48 bytes for inventory proper. No layout resize needed.

**Implementation risks:**
- Sprite pipeline (`tex3ds` → `.t3x` → `C2D_SpriteSheetLoad`) has never been proven on real hardware. Must validate early in M6.
- 4-direction player animation requires an animation state machine. Keep it minimal: `facing` (4 values) + `walk_frame` (0–1) driven by `moving` boolean.
- `GameState` currently lives as an enum in `main.cpp` with no dedicated header. Extracting it is a small refactor but must be done before title screen compiles cleanly.

**Estimated complexity:** Medium. The sprite pipeline is the highest-risk unknown; everything else is straightforward.

**Test plan:**
1. Boot → title screen appears with "New Game" / "Continue" options.
2. "Continue" is greyed out (or absent) when no save exists; enabled when save exists.
3. Select "New Game" → world loads with `wood=0, rope=0`.
4. Walk to forest → wood pile nodes visible. Press A near one → `wood` increments in HUD.
5. Walk away, re-enter forest → nodes repopulate.
6. Player sprite is visible with directional facing. NPCs have distinct appearances.
7. Save with M5 → load on boot → title screen shows "Continue" correctly.

---

### Milestone 7 — Closed Economy Loop
**Goal:** Give the player something to do with their resources and something to pursue after the first quest.

**Subgoals:**
1. Basic item definitions (8 items sufficient for demo)
2. 8-slot inventory — display only, no equipment
3. Shop: Mira sells 2–3 items; buy removes gold
4. Second quest: "The Broken Well" — uses WorldObject system; requires gathered wood
5. Quest rewards can grant items (not just gold)

**Files added:**

| File | Purpose |
|------|---------|
| `source/item/ItemDef.h` | `ItemDef` struct: id, name, icon_tile, buy_price, sell_price, flags; static table of 8 items |
| `source/item/Inventory.h/.cpp` | 8-slot fixed array (mirroring `reserved_inventory` in SaveData); `addItem`, `removeItem`, `hasItem` |
| `source/ui/ShopScreen.h/.cpp` | Bottom-screen buy UI: NPC stock list, player gold, confirm purchase |

**Files modified:**

| File | Change |
|------|--------|
| `source/quest/QuestDef.h` | Add second quest "The Broken Well"; add `ITEM_REWARD` to `QuestReward` struct |
| `source/quest/QuestManager.cpp` | Handle `ITEM_REWARD` in `applyReward()` — calls `inventory.addItem()` |
| `source/npc/NPCManager.h/.cpp` | `tryInteract()` checks if NPC has `shop_id` set; opens `ShopScreen` instead of dialogue |
| `source/npc/NPC.h` | Add `shop_id` field (u8, 0xFF = not a shopkeeper) |
| `source/render/Renderer.h/.cpp` | Add `drawInventory()`, `drawShopScreen()` for bottom screen |
| `source/save/SaveData.h` | Replace `reserved_inventory[64]` with `ItemSlot inventory_slots[16]` (4 bytes × 16 = 64); **SAVE_VERSION → 7** |
| `source/main.cpp` | Wire Inventory; pass to QuestManager and ShopScreen |

**Save compatibility impact:**
Breaking change — `SAVE_VERSION` bumps to 7. The `reserved_inventory[64]` block becomes `inventory_slots[16]` — same 64 bytes, now typed. `sizeof(SaveData)` stays at 236; `static_assert` still passes.

**Implementation risks:**
- The shop UI requires bottom-screen scroll if item lists exceed display height. For demo scope, cap shop stock at 4 items — no scroll needed.
- "The Broken Well" quest uses the WorldObject system but needs a fourth world object. Ensure `MAX_WORLD_OBJECTS = 16` is sufficient (it is).
- Item rewards in `QuestDef` add complexity to `QuestReward` struct. Keep it minimal: one optional `item_id` + `item_quantity` field.

**Estimated complexity:** Medium-High. Inventory and shop UI are new UI patterns not yet established in the renderer. Keep screens simple — list of text lines, no graphical item icons required for demo.

**Test plan:**
1. Boot → new game → `wood=0, rope=0, inventory empty`.
2. Gather wood in forest → inventory HUD shows Wood: N.
3. Talk to Mira → shop screen appears. Buy item → gold decreases, item appears in inventory.
4. Accept "The Broken Well" quest. Complete it (repair well using gathered wood). Receive item reward — appears in inventory.
5. Save → load → inventory contents persist correctly.
6. Attempt to buy with insufficient gold → purchase rejected, no gold deducted.

---

### Milestone 8 — Demo Polish Pass
**Goal:** Make the vertical slice feel finished enough to hand to a player with no explanation.

**Subgoals:**
1. BGM: 1 town track + 1 forest track (streaming `.bcstm`)
2. SFX: footstep, interact confirm, repair success (`.bcwav`)
3. Minimap: bottom-screen zone overview with player dot
4. NPC "busy" indicator when NPC is mid-schedule and player approaches
5. Quest completion fanfare (screen flash + short SFX)
6. Controls reminder screen (shown once on first boot, dismissible)

**Files added:**

| File | Purpose |
|------|---------|
| `source/audio/AudioManager.h/.cpp` | `ndsp` wrapper; `playBGM(id)`, `playSFX(id)`, `stopBGM()`; streaming for BGM, RAM-loaded for SFX |
| `source/ui/Minimap.h/.cpp` | Renders 64×48 tile-downsampled zone overview + player dot on bottom screen |
| `romfs/audio/bgm_town.bcstm` | Town BGM (streaming, ~4 MB) |
| `romfs/audio/bgm_forest.bcstm` | Forest BGM (streaming, ~4 MB) |
| `romfs/audio/sfx_step.bcwav` | Footstep SFX (~8 KB) |
| `romfs/audio/sfx_interact.bcwav` | Interaction confirm (~8 KB) |
| `romfs/audio/sfx_repair.bcwav` | Repair success (~8 KB) |

**Files modified:**

| File | Change |
|------|--------|
| `source/world/ZoneManager.cpp` | Call `AudioManager::playBGM()` on zone load |
| `source/render/Renderer.h/.cpp` | Add `drawMinimap()` to bottom screen idle state |
| `source/main.cpp` | Init `AudioManager`; trigger SFX on interact/repair events; first-boot controls screen |
| `source/save/SaveData.h` | No layout change — audio state is not persisted; `SAVE_VERSION` stays at 7 |

**Save compatibility impact:**
None. Audio state is ephemeral. `SAVE_VERSION` does not change.

**Implementation risks:**
- `ndsp` audio on 3DS has a known initialization order dependency with `gfxInitDefault()`. Must call `ndspInit()` after graphics are up. This is documented in devkitPro examples — not a research problem, just a sequencing constraint.
- BGM streaming requires a background thread or DSP interrupt — `ndsp` handles this internally with a callback. The main loop does not need modification beyond calling `ndspFrameCallback()` appropriately.
- Minimap rendering: sampling a 30×24 tile map down to a 64×48 pixel display requires a simple loop — 1 pixel per tile at 2:1 scale. No complexity. The only risk is if the bottom screen is already crowded (it is — quest HUD + inventory share it). Minimap should be a toggleable overlay or a secondary bottom-screen state, not always-on.
- First-boot controls screen: requires a `first_boot` flag in `SaveData`. Add it to `reserved_titles[8]` — 1 bit. Does not require a version bump if it fits within the reserved block.

**Estimated complexity:** Low–Medium. Audio plumbing is the only novel system. Minimap is arithmetic. Polish items are additive.

**Test plan:**
1. Boot into town → BGM plays. Walk to forest → BGM cross-fades.
2. Press A near NPC → SFX fires. Repair bridge → repair SFX fires. Walk → footstep SFX at regular interval.
3. Bottom screen minimap visible. Player dot moves with player. Zone name matches.
4. Complete quest → brief white screen flash + success SFX.
5. First boot shows controls screen. Dismissed with A. Does not appear on second boot.
6. Save → load → audio resumes correct track for loaded zone.

---

## Section 5: Demo Exit Criteria

The demo is ready to hand to a player with no explanation when **all of the following are true**:

### Core loop
- [ ] Game boots to a title screen. "New Game" and "Continue" are clearly distinct.
- [ ] Player spawns in Town with zero resources (no hardcoded test values).
- [ ] Three NPCs visible in Town, visually distinct from each other and from the player.
- [ ] Player can speak to Mira and accept "The Missing Package" quest.
- [ ] Quest objective is visible on the bottom screen at all times.
- [ ] Player can walk south to Forest without explanation.
- [ ] Forest marker is visible and the player can reach it.
- [ ] Player can return to Town and complete the quest.
- [ ] Gold reward is visible on the bottom screen after completion.
- [ ] A second quest is immediately available after the first completes.

### World state
- [ ] At least 2 world objects are visible and interactable (bridge, fallen tree).
- [ ] Repairing an object visibly changes the world (passage opens, obstacle disappears).
- [ ] Resources can be gathered in the Forest (wood piles, A to collect).
- [ ] Gathered resources appear in the HUD immediately.

### Economy
- [ ] Mira's shop can be opened and an item purchased for gold.
- [ ] Purchased item appears in inventory.
- [ ] Attempting to buy with insufficient gold shows a clear rejection message.

### Persistence
- [ ] Pressing SELECT saves the game. "Game Saved." message appears.
- [ ] Closing and reopening the game (or pressing START to reload) restores: player position, zone, gold, resources, inventory, quest state, world object states, and time of day.
- [ ] A repaired bridge is still repaired after reload.
- [ ] A completed quest is still completed after reload.

### Feel
- [ ] BGM plays in Town and Forest (different tracks).
- [ ] At least one SFX fires on interaction (does not need to be final audio).
- [ ] No zone or object is a dead end with no content (Dungeon Entrance needs at minimum a sign or barrier saying "deeper tunnels — not yet open").
- [ ] Day/night cycle is visible during a 10-minute play session (TIME_SCALE must be ≥ 60).
- [ ] Frame rate is stable at 60 fps on Old 3DS hardware for at least the first two zones.

### Stability
- [ ] No crash during normal play through the full quest loop.
- [ ] No crash on repeated save/load cycles (minimum 5 consecutive).
- [ ] No crash when entering and exiting all three zones in sequence.
- [ ] Dialogue can be opened and closed repeatedly without visual corruption.

**The demo is NOT ready if any item on this checklist is unchecked.**

---

## Section 6: Technical Debt Review

### SaveData architecture

**Status: Good. No immediate action needed.**

The `#pragma pack(1)` + `static_assert(sizeof == 236)` combination is the correct approach. The reserved block strategy (6 named blocks totaling 152 bytes) is adequate for M6–M8 without resizing. The CRC32 validation correctly rejects corrupt or version-mismatched saves.

**One issue to fix before M6:** The `CHECKSUM_OFFSET = 14` constant is computed manually and not derived from the struct. If the header fields ever change order (unlikely but possible), the constant would silently become wrong. Fix: use `offsetof(SaveData, zone_id)` instead of the literal `14`.

```cpp
// Change in SaveData.h:
static constexpr u32 CHECKSUM_OFFSET = offsetof(SaveData, zone_id);
```

**One issue to fix before M7:** `PlayerState` has a constructor, making it non-trivially constructible. This is why `SaveData` mirrors PlayerState fields manually rather than embedding the struct directly. Before M7 adds inventory, decide: either remove the constructor from `PlayerState` (make it a pure POD, initialize fields manually at start-up) or accept the mirroring pattern permanently. The mirroring works but creates a maintenance surface where `PlayerState` and `SaveData` must be kept in sync manually.

Recommendation: make `PlayerState` a POD struct. Move initialization to a `PlayerState::init()` free function called at game start. This costs 10 lines of change and eliminates the mirror pattern permanently.

---

### WorldObject architecture

**Status: Good. One latent correctness issue.**

The `setStatesFromSave()` function in `WorldObjectManager.cpp` contains this comment:

```cpp
// Safe: applyOverrides checks tx/ty against map bounds via setTileOverride,
// and setTileOverride simply ignores out-of-range coords gracefully.
```

This assumption is incorrect. `setTileOverride()` does **not** bounds-check `tx` and `ty` — it casts them directly to `u8` and stores them. If `setStatesFromSave()` is called while the wrong zone is loaded, overrides for out-of-zone objects will be written into the override table and silently applied to wrong tiles (whichever tile happens to share those coordinates in the current zone's map).

**Fix before M6:** `setStatesFromSave()` should be replaced by `onZoneLoaded()` calls. The correct pattern after a load is:

```cpp
// In SaveManager::apply(), after zones.loadZone():
worldObjects.setStatesFromSave(sd.world_object_states, MAX_WORLD_OBJECTS, map);
// Then the caller should call:
worldObjects.onZoneLoaded(currentZone, map);
// onZoneLoaded already does clearOverrides() + filtered applyOverrides()
```

This means `setStatesFromSave()` should only set the `state` field on each object — it should **not** call `applyOverrides()` or `clearOverrides()`. The caller (`SaveManager::apply()`) must then call `onZoneLoaded()` to apply the correct subset. This separation also makes `setStatesFromSave()` zone-agnostic, which is the correct abstraction.

---

### Quest architecture

**Status: Good for current scope. One scalability note.**

`QuestDef.h` defines quest steps as a static array with `const char*` pointers to string literals. This is correct for ROM storage and has zero heap cost. The architecture handles linear quests perfectly.

**One issue:** `findActiveQuest()` returns the first `IN_PROGRESS` quest. If two quests are simultaneously `IN_PROGRESS`, only the first one gets proximity checks, HUD display, and NPC dialogue overrides. The current hardcoded data has only one quest so this never fires — but it is a silent bug waiting to be triggered when a second quest is added in M7.

**Fix before M7:** Decide on the policy: LOA supports one active quest at a time (simplest, correct for the demo) or multiple. If one at a time, enforce it: `QuestManager::startQuest()` should check `findActiveQuest() >= 0` and reject a second start. Document this as a design decision, not an oversight.

**One improvement worth making in M7:** `QuestDef.h` currently has quest step data as a raw `const QuestStep*` pointer + count. When the second quest is added, both quest definitions live in the same file. This is fine at 2 quests; it becomes a maintenance problem at 10+. Before M7, split quest data into one `.h` file per quest under `source/quest/data/`, with a master include in `QuestDef.h`. This is a 15-minute refactor that pays for itself at the third quest.

---

### Zone architecture

**Status: Good. One missing feature blocks the demo.**

The `ZoneData.h` approach — static `const u8[]` arrays directly in a header — is the correct strategy for a 3DS ROM. Zero heap, zero file I/O at runtime, zero parsing. It works correctly for 3 zones.

**The one gap that blocks the demo:** There is no mechanism for an NPC or sign to block a zone exit with a message ("The deeper tunnels are collapsed — come back later"). The Dungeon Entrance has no second exit and no explanation. A player will reach it, find a dead end, and assume the game is broken.

Fix: Add a `BlockedExit` struct to `ZoneData.h` alongside `TransitionDef`:

```cpp
struct BlockedExit {
    u8          trigger_tx;
    u8          trigger_ty;
    const char* message;   // shown when player walks into this tile
};
```

`ZoneManager` checks blocked exits the same way it checks transitions — on player tile position. If a blocked exit fires, it shows the message in the HUD for 3 seconds and does not transition. No new systems needed. This is a 2-hour addition in M8 that eliminates the dead-end problem.

**One longer-term issue:** `ZoneData.h` is currently 254 lines for 3 zones. At 10 zones it will be ~850 lines. At 20 zones (full game) it will be ~1700 lines. This is manageable but unwieldy. Before the full content authoring phase (post-demo), split into one file per zone under `source/world/zones/zone_town.h`, etc., with a master `ZoneData.h` that includes them. This is a zero-logic refactor that can be done in one session.
