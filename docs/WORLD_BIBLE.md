# Legends of Aetheria — World Bible v1
## Physical Geography and Level Design Reference

---

## Overview

The Version 1 world consists of three connected zones arranged on a linear
north-south axis with one eastern branch.

```
[ TOWN ]
    |
    | (south road)
    |
[ FOREST ]
    |
    +-------- (east trail) -------- [ DUNGEON ENTRANCE ]
```

Total map area: 78 × 24 tiles across all zones (not contiguous — zone-based streaming).

---

## Zone 1 — Town

**Dimensions:** 28 × 22 tiles (448 × 352 pixels)

### Identity
A small walled market town. The perimeter is entirely bordered by stone walls,
giving it a fortified feel without requiring castle art. The interior is divided
into three loose districts separated by the central market plaza.

Visual palette: warm greens (grass), cobblestone grey (paths and plaza),
tan stone (walls and buildings), brown wood (market stalls and fences).

### Layout

```
+----------------------------+
|  [Forge]  [?]  [?]  [?]   |  ← Row 0–4: Northern District
|                            |
|    [Fence A]   [Fence B]   |  ← Row 5–11: Market District
|    [ M A R K E T  P L A Z A ]
|    [Fence A]   [Fence B]   |
|                            |
|  [?]    [South Dist]  [?]  |  ← Row 12–20: Southern District
|            ||              |
+------------||______________+  ← Row 21: South wall with road gap
             ↓
          (to Forest)
```

### Districts

**Northern District (rows 1–4)**
- Two building clusters in the top-left and top-right corners (tiles using TILE_WALL blocks).
- The forge / blacksmith is in the top-left cluster (tiles 2–3, cols 2–3).
- Open grassy area in the center-north.
- The guard starts their patrol here.

**Market District (rows 5–11)**
- Central fenced plaza (TILE_FENCE border, 9×7 tile interior).
- The plaza interior uses dirt path tiles and open grass for foot traffic.
- The merchant NPC occupies the center of the plaza during trading hours.
- Two smaller fenced yards flank the plaza east and west (the blacksmith's
  courtyard on the left, a storage yard on the right).

**Southern District (rows 12–20)**
- Less developed — open grass with scattered building remnants.
- Two building clusters in the south-west and south-east (mirror of north).
- The south road (dirt path tiles, 2 tiles wide at cols 13–14) runs from
  the market plaza south to the exit gap in the perimeter wall.
- Row 21 is the solid bottom wall with a 2-tile gap at cols 13–14.

### Landmarks
- **The Forge:** Top-left building cluster. Distinguishable by the anvil prop tile.
- **The Market Plaza:** Central fenced square. The single largest open area in town.
- **The South Road:** Dirt path running the full height of the southern district.
  Visible from the market, leading the player's eye toward the exit.

### Roads
- **South Road:** Cols 13–14, rows 5–21. Dirt tile (TILE_DIRT). 2 tiles wide.
  Connects market plaza to forest.
- **Inner paths:** Informal — the game does not enforce road tiles inside the plaza,
  but the dirt floor implies foot traffic routes.

### Entry / Exit Points
- **South exit** (row 21, cols 13–14): leads to Forest zone, Forest spawn[1].

---

## Zone 2 — Forest

**Dimensions:** 30 × 24 tiles (480 × 384 pixels)

### Identity
A dense mixed forest with a single winding dirt path running north-south
through the center and an east-branching trail in the middle section.
The path is always visible — players should never feel lost in the forest.
Trees fill the rest of the map and provide natural walls without requiring
stone wall tiles.

Visual palette: multiple greens (forest floor, undergrowth, tree canopy),
brown dirt (path), dark tree trunks. Noticeably darker than town, especially
at dusk and night due to the day/night tint system.

### Layout

```
+------+------+------+------+------+------+
|######|######|######| PATH |######|######|  ← rows 0–3: Dense tree border
|######|  .   |  .   | PATH |  .   |######|
|#  .  |  .   |  .   | PATH |  .   |  .  #|
|#  .  |  .   | PATH-+-PATH |  .   |  .  #|  ← rows 9–13: Path widens
|#  .  |  .   | PATH   PATH |  .   |  .  #|       east branch begins
|#  .  |  .   | PATH   PATH |  .   | PATH→|  ← rows 11–12: East exit
|#  .  |  .   | PATH   PATH |  .   | PATH→|
|#  .  |  .   | PATH   PATH |  .   |  .  #|
|#  .  |  .   |  .   | PATH |  .   |  .  #|
|######|  .   |  .   | PATH |  .   |######|
|######|######|######| PATH |######|######|  ← rows 21–23: Dense tree border
+------+------+------+------+------+------+
                                        ↓ East exit (cols 29, rows 11–12)
```

`#` = solid tree tile.  `.` = forest floor (passable).  `PATH` = dirt path.

### Path Structure
- **North-south path:** Cols 13–14 (matching the town south road width), runs
  rows 0–23. Two tiles wide throughout. Tapers to forest floor approaching exits.
- **East branch:** At rows 9–13 the path widens to 4 tiles (cols 12–15) before
  the east branch splits off. The east branch runs cols 15–29 at rows 11–12.
  This creates a T-junction that naturally guides the player east toward the dungeon.

### Landmarks
- **The T-Junction (rows 9–13, cols 12–15):** Where the east trail branches. The
  path widening signals the player that a choice point is ahead.
- **Tree clusters:** The trees are arranged to feel dense and natural. Isolated
  tree pairs and triplets break up the wall of green. No single species or
  cluster pattern repeats more than three times adjacently.

### Roads
- **North-South Path:** Dirt tiles, cols 13–14. Connects Town (north) to Forest
  interior.
- **East Trail:** Dirt tiles, rows 11–12. Connects T-junction to Dungeon
  Entrance (east edge, col 29).

### Entry / Exit Points
- **North exit** (row 0, cols 13–14): leads to Town, Town spawn[1] (south road arrival).
- **East exit** (col 29, rows 11–12): leads to Dungeon Entrance, Dungeon spawn[1].

### Visual Notes
- The perimeter (row 0, row 23, col 0, col 29) is entirely solid tree tiles.
  This prevents the player seeing the void outside the map.
- The tree density decreases moving toward the path center, giving a natural
  feel of a cleared trail through wilderness.

---

## Zone 3 — Dungeon Entrance

**Dimensions:** 20 × 18 tiles (320 × 288 pixels)

### Identity
A stone-floored corridor complex — the outermost accessible section of a
dungeon carved into the rock. It functions as a transition zone between the
outdoor world and the deeper dungeon (not yet implemented). The layout
suggests a structure built by intelligent hands: regular walls, symmetric
corridors, blocked side chambers.

Visual palette: dark stone floor (grey), stone wall (dark grey), near-black
background. Distinctly the darkest zone. Night tint makes it nearly unlit —
intentional atmosphere for a dungeon threshold.

### Layout

```
+--------------------+
|####################|  row 0: solid top wall
|# . . . . . . . . #|  row 1: open interior
|# .##. . . . .##. #|  rows 2–5: side alcoves (blocked)
|# .##.##. .##.##. #|
|# . .##. . .##. . #|
|# . . . . . . . . #|  row 5: open corridor
|########. . ########|  row 6: HORIZONTAL WALL — divides north/south
|# . . .##. .##. . #|  rows 7–10: south section (two sub-rooms)
 . . . .##. .##. . #|  row 8: WEST EXIT (col 0 open)
 . . . .##. .##. . #|  row 9: WEST EXIT (col 0 open)
|# . . .##. .##. . #|  row 10
|########. . ########|  row 11: HORIZONTAL WALL — divides mid/south
|# . . . . . . . . #|  rows 12–16: large southern chamber
|# .##. . . . .##. #|  rows 13–14: symmetrical pillars
|# .##. . . . .##. #|
|# . . . . . . . . #|
|# . . . . . . . . #|
|####################|  row 17: solid bottom wall
+--------------------+
↑
West exit (col 0, rows 8–9)
```

### Structural Description

**North Section (rows 1–5)**
Open room with two walled alcoves (cols 2–3 and 17–18). These read as
side chambers or guard posts. Entirely passable in the center.

**Dividing Corridor (row 6)**
A full-width wall with a 4-tile opening in the center (cols 8–11). Forces
the player through a chokepoint — the classic dungeon corridor feel.

**Middle Section (rows 7–10)**
Two sub-rooms separated by a central wall pair (cols 7–8 and 12–13 are
solid, cols 8–11 open). The west exit is in the left sub-room (rows 8–9).
Arrival from the forest deposits the player here.

**Second Dividing Corridor (row 11)**
Identical layout to row 6. Creates a sense of depth — the dungeon goes deeper.

**Southern Chamber (rows 12–16)**
The largest open space in the zone. Two symmetrical pillar pairs (cols 3–4
and 14–15, rows 13–14) suggest a hall or antechamber. This is where
future dungeon content (enemies, chest, boss door) will be placed.
The south wall (row 17) is solid — no exit here yet. A locked door or
blocked passage prop can be placed here when the dungeon interior is built.

### Landmarks
- **The Chokepoint (row 6):** The horizontal wall forcing players through the
  center opening. Signals that this is a structured place, not a natural cave.
- **The Antechamber (rows 12–16):** The large southern room. Its size and
  symmetrical pillars suggest importance. Reserved for the dungeon's first boss
  door or key puzzle.
- **West Corridor Exit (rows 8–9, col 0):** The passage back to the forest.
  Two tiles wide for comfortable navigation.

### Roads
- **West Corridor:** Rows 8–9 through the middle section. Stone floor tiles
  leading to the west exit.

### Entry / Exit Points
- **West exit** (col 0, rows 8–9): leads to Forest, Forest spawn[2] (east arrival).

### Visual Notes
- All walls are TILE_STONE_WALL (dark grey, ID 18). No grass or dirt tiles.
- The background clear color is near-black (5, 5, 10) — the darkest of the
  three zones.
- At night the day/night tint pushes this zone into near-total darkness,
  which is intentional and atmospheric. Future torchlight props will provide
  relief.
- The 20×18 tile size (320×288 px) means the zone is only slightly larger
  than the top screen (400×240). Camera clamping may show black edges at
  extreme positions — acceptable for a dungeon corridor.

---

## Inter-Zone Connections Summary

| From | Exit Direction | Tile Coordinates | To | Arrival Spawn |
|------|---------------|------------------|----|---------------|
| Town | South (row 21) | cols 13–14 | Forest | spawn[1] — north path |
| Forest | North (row 0) | cols 13–14 | Town | spawn[1] — south road |
| Forest | East (col 29) | rows 11–12 | Dungeon Entrance | spawn[1] — west corridor |
| Dungeon Entrance | West (col 0) | rows 8–9 | Forest | spawn[2] — east trail |

---

## Planned Expansions (not in v1 scope)

The following zones are documented here for layout continuity. They do not
exist in the current build.

**Dungeon Interior** — connected via the south wall of Dungeon Entrance.
Planned as 3–4 linked zones forming a linear dungeon with one boss room.

**Mountain Pass** — connected via a northern town exit (not yet present in
town map). Leads to mountain biome with cave entrances.

**Village 1 (West Village)** — connected via a west town exit. Smaller than
Town, different visual identity (thatch roofs, muddy paths).

**Village 2 (East Village)** — connected via an east forest exit. Fishing
village, water tiles along one edge.

These zones will be added in later milestones without requiring changes to
the existing three zones.
