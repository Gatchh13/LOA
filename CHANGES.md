# LOA — Code Fixes (post-Milestone 5 cleanup)

Everything here was verified by syntax-checking against the project's actual
headers (stubbing only `<3ds.h>`/`<citro2d.h>`, which aren't available
outside devkitPro) — not just read-through. All non-renderer game logic
compiles cleanly as one combined translation unit after these changes.
`SaveData` is still exactly 236 bytes and `CHECKSUM_OFFSET` still resolves
to 14, so **existing save files remain valid**.

How to use: either apply `fixes.patch` with `git apply fixes.patch` from
your repo root, or diff `source-fixed/` against your own `source/`/`include/`
and merge by hand.

---

## Build-breaking bugs fixed

These would have failed to compile (or only compiled by accident) before
this pass:

1. **`source/world/TileMap.h` had duplicated/orphaned code outside the
   class body.** Looked like a bad merge — `getTile()`, `isSolid()`, and
   the member variables were declared twice, with the second copy sitting
   outside any class after the closing brace. Removed the duplicate.

2. **`ZoneManager::getTileMap()` only had a `const` overload**, but several
   call sites (`WorldObjectManager::init()`, `SaveManager::loadGame()` /
   `apply()`) need a mutable `TileMap&`. Added the non-const overload
   (standard idiom: const member → const ref, non-const member → non-const
   ref to the same data).

3. **`ZoneData.h` and `DayNight.cpp` called `C2D_Color32()` without
   including `<citro2d.h>`.** This only worked because `main.cpp` happens
   to include `<citro2d.h>` before anything else — fragile, and would break
   the moment either header is used standalone (tests, reordered includes,
   a new translation unit). Added the missing includes directly to the
   files that use the symbol. `TileMap.cpp` had the same gap (constructor's
   default background color) — fixed too.

4. **Dead file `source/core/InputManager.h`** — an earlier-milestone
   duplicate of `source/input/InputManager.h`, not included anywhere.
   Deleted.

## Correctness bugs fixed

5. **Cross-zone tile corruption on save load (highest-priority bug from
   the architectural review).** `WorldObjectManager::setStatesFromSave()`
   was re-applying tile overrides for *every* active world object
   regardless of which zone it belonged to, into whatever `TileMap` was
   currently loaded. A Forest object's override could silently land on a
   Town tile that happens to share the same `(tx, ty)` coordinate.
   - `setStatesFromSave()` now only restores `state` values — no override
     application.
   - `SaveManager::apply()` now calls `worldObjects.onZoneLoaded(savedZone, map)`
     immediately after, which correctly filters by `obj.zone == savedZone`.
   - Removed the now-redundant `onZoneLoaded()` calls that `main.cpp` was
     making after every `loadGame()` (the bug's old workaround pattern).
   - Added a defense-in-depth fix in `TileMap::setTileOverride()` itself:
     it now bounds-checks `(tx, ty)` against the map's actual dimensions
     and silently ignores out-of-range writes, so this class of bug can't
     resurface even if some future caller gets the zone wrong again.

6. **`CHECKSUM_OFFSET = 14` was a magic number.** Replaced with
   `offsetof(SaveData, zone_id)`, matching what the header comment already
   said it should be. If any header field is ever added/removed/resized,
   the checksum will automatically cover the right range instead of
   silently invalidating every save with no compiler warning.
   Verified: `offsetof(SaveData, zone_id) == 14`, so this is a no-op for
   existing saves.

## Hardening for known fragile assumptions

7. **`PlayerState` test values removed from the constructor.** It's now a
   pure POD struct (no constructor) with an explicit `init()` for new-game
   starting values — this is also what un-blocks embedding `PlayerState`
   directly inside `SaveData` later (scheduled for M7) instead of
   hand-mirroring fields.
   - `main.cpp` now calls `playerState.init()` on the fresh-new-game path
     only (the loaded-save path is unaffected — `SaveManager::apply()`
     overwrites gold/wood/rope directly from the save file).
   - The starting wood/rope amounts (20/10) are **kept**, not zeroed — with
     no gathering system yet, zeroing them would silently break the only
     testable gameplay loop in the M5 demo (repairing the bridge/ladder/
     fallen tree). They're now an explicit, clearly-labeled
     `PLACEHOLDER_STARTING_WOOD/_ROPE` constant with a `TODO(Milestone 6)`
     marker instead of being buried as constructor defaults.
   - Fixed a related latent bug: `QuestManager.cpp` declares a throwaway
     `PlayerState dummy` for REACH_MARKER steps. Without the old
     constructor this would have been read from indeterminate memory —
     added `dummy.init()`.

8. **Quest ID registry.** Replaced the lone `QUEST_MISSING_PACKAGE`
   constant with a `QuestID` enum (`QuestID::MISSING_PACKAGE`, etc., per
   Architectural Review Recommendation 6). Added `static_assert`s tying
   `QuestID::COUNT` to both `MAX_QUESTS` and the actual size of
   `s_questDefs[]`, so a quest added to one but not the other fails to
   compile instead of silently mismatching.

9. **Guard against silent multi-active-quest corruption** (Recommendation
   7). `QuestManager::findActiveQuest()` has always returned only the
   *first* `IN_PROGRESS` quest — every consumer (HUD text, quest markers,
   NPC dialogue overrides) depends on this. With one quest this is
   invisible; the moment a second quest exists and both end up
   `IN_PROGRESS`, the first quest's UI/dialogue silently stops updating
   with no error anywhere.
   - Added `QuestManager::startQuest(quest_id)` as the one legitimate entry
     point for transitioning a quest to `IN_PROGRESS`. It checks
     `findActiveQuest()` first and refuses (logging via `ERR`, which is
     always compiled in) if a *different* quest is already active.
   - `init()`'s auto-start of "The Missing Package" now goes through
     `startQuest()` instead of writing the state directly.
   - This doesn't add multi-quest support — it makes the single-quest
     constraint an enforced, loud failure instead of a silent one, exactly
     as recommended, so it's caught immediately when M7 adds a second quest.

---

## Not touched (deliberately)

- **Multi-active-quest support itself** — real feature work, not a bug fix.
- **`PlayerState` embedding directly into `SaveData`** — scheduled for M7
  alongside the inventory layout change; doing it now would be an
  incidental save-format change bundled into an unrelated cleanup pass.
- **NPC zone-bucketing, ZoneData.h splitting, dialogue buffer size, renderer
  batching** — all real, all flagged in the architectural review, all
  scoped as M6+ work rather than bug fixes.
