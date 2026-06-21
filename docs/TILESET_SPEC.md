# Legends of Aetheria — Tileset Specification v1

## Constraints

- **Tile size:** 16×16 pixels
- **Sprite sheet format:** 128×128 px per sheet = 64 tiles per sheet
- **Target sheets:** 4 (town, forest, mountain/dungeon, shared props)
- **VRAM budget:** ~64 KB per active sheet (RGBA8) — all 4 fit in <300 KB
- **Developer:** solo creator; prioritize reuse over unique art

---

## Sheet 1 — Town Tileset (128×128, 64 tiles)

### Ground (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 00 | Grass — base | Primary outdoor fill |
| 01 | Grass — detail A | Slight variation (darker patch) |
| 02 | Cobblestone — center | Plaza/road fill |
| 03 | Cobblestone — worn | Variation tile |
| 04 | Dirt path | Between buildings |
| 05 | Dirt — edge | Transition to grass |
| 06 | Floor boards — dark | Interior building floors |
| 07 | Floor boards — light | Variation |

### Walls and Buildings (16 tiles)
| ID | Name | Notes |
|----|------|-------|
| 08 | Stone wall — face | Exterior building side |
| 09 | Stone wall — top | Parapet / roof edge |
| 10 | Brick wall — face | Tavern/inn style |
| 11 | Brick wall — top | |
| 12 | Wood wall — face | Market stall style |
| 13 | Wood wall — top | |
| 14 | Building door — closed | Reused on all buildings |
| 15 | Building door — open | Same shape, open variant |
| 16 | Rooftop — red tile | Row tiles, reuse fills rows |
| 17 | Rooftop — brown thatch | |
| 18 | Rooftop — peak left | |
| 19 | Rooftop — peak center | |
| 20 | Rooftop — peak right | |
| 21 | Window — dark | Closed |
| 22 | Window — lit | Night variant (same tile, warm tint) |
| 23 | Chimney top | |

### Fences and Borders (6 tiles)
| ID | Name | Notes |
|----|------|-------|
| 24 | Wooden fence — horizontal | |
| 25 | Wooden fence — vertical | |
| 26 | Wooden fence — corner | Rotated 4× in code |
| 27 | Stone border — horizontal | Plaza edging |
| 28 | Stone border — corner | |
| 29 | Gate — open | |

### Props (14 tiles)
| ID | Name | Notes |
|----|------|-------|
| 30 | Market stall — canopy left | |
| 31 | Market stall — canopy right | |
| 32 | Market stall — front | |
| 33 | Barrel | Reused everywhere |
| 34 | Crate | |
| 35 | Well top | |
| 36 | Well base | |
| 37 | Fountain — center | |
| 38 | Fountain — edge | |
| 39 | Lamppost — base | |
| 40 | Lamppost — top (unlit) | |
| 41 | Lamppost — top (lit) | Night variant |
| 42 | Sign blank | Text added in engine |
| 43 | Anvil | Blacksmith prop |

### Water and Transitions (6 tiles)
| ID | Name | Notes |
|----|------|-------|
| 44 | Water — center | Animated: 2-frame swap |
| 45 | Water — frame 2 | Swap every ~30 frames |
| 46 | Grass→water edge N | |
| 47 | Grass→water edge E | Rotated from N |
| 48 | Grass→water corner NE | |
| 49 | Grass→cobble transition | |

**Sheet 1 total: 50 tiles** (14 empty slots reserved for expansion)

---

## Sheet 2 — Forest Tileset (128×128, 64 tiles)

### Ground (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 00 | Forest floor — dark | Dense undergrowth fill |
| 01 | Forest floor — light | Open area variation |
| 02 | Grass — forest clearing | Lighter, distinct from town |
| 03 | Dirt path — forest | Same shape as town dirt |
| 04 | Mud | Wet path |
| 05 | Root ground | Exposed roots, passable |
| 06 | Rock ground | Rocky clearing |
| 07 | Dead leaves | Autumn variation |

### Trees (12 tiles)
| ID | Name | Notes |
|----|------|-------|
| 08 | Tree trunk — base | Solid lower tile |
| 09 | Tree canopy — single | Small tree top |
| 10 | Tree canopy — large TL | 2×2 canopy top-left |
| 11 | Tree canopy — large TR | |
| 12 | Tree canopy — large BL | |
| 13 | Tree canopy — large BR | |
| 14 | Tree canopy — dark | Dense cluster variant |
| 15 | Fallen log — horizontal | |
| 16 | Fallen log — vertical | |
| 17 | Stump | Passable (cleared area) |
| 18 | Bush — small | Passable prop |
| 19 | Bush — large | Solid |

### Rocks and Landmarks (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 20 | Rock — small | Passable obstacle |
| 21 | Rock — medium solid | |
| 22 | Rock — large TL | 2×2 boulder |
| 23 | Rock — large TR | |
| 24 | Rock — large BL | |
| 25 | Rock — large BR | |
| 26 | Cave entrance — top | |
| 27 | Cave entrance — bottom | |

### Water (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 28 | Stream — horizontal | |
| 29 | Stream — vertical | |
| 30 | Stream — corner NE | |
| 31 | Stream — corner NW | |
| 32 | Stream — corner SE | |
| 33 | Stream — corner SW | |
| 34 | Pond — center | |
| 35 | Pond — shore N | |

### Props and Details (6 tiles)
| ID | Name | Notes |
|----|------|-------|
| 36 | Mushroom — red | Prop |
| 37 | Mushroom — brown | |
| 38 | Flower — white | |
| 39 | Flower — yellow | |
| 40 | Shrine — base | |
| 41 | Shrine — top | |

**Sheet 2 total: 42 tiles** (22 empty slots reserved)

---

## Sheet 3 — Dungeon / Mountain Tileset (128×128, 64 tiles)

### Ground (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 00 | Stone floor — dark | Primary dungeon fill |
| 01 | Stone floor — cracked | Damage variation |
| 02 | Stone floor — wet | Near water |
| 03 | Stone floor — light | Torchlit area |
| 04 | Mountain path — stone | Exterior mountain |
| 05 | Mountain path — gravel | |
| 06 | Snow — flat | High altitude |
| 07 | Rubble | Impassable debris |

### Walls (12 tiles)
| ID | Name | Notes |
|----|------|-------|
| 08 | Stone wall — face | |
| 09 | Stone wall — top | |
| 10 | Stone wall — carved | Boss room variation |
| 11 | Stone wall — mossy | Wet area |
| 12 | Brick wall — dungeon | |
| 13 | Brick wall — top | |
| 14 | Dungeon door — closed | Iron bars |
| 15 | Dungeon door — open | |
| 16 | Archway — top left | |
| 17 | Archway — top center | |
| 18 | Archway — top right | |
| 19 | Cave wall — rough | Natural cave section |

### Mountain Exterior (8 tiles)
| ID | Name | Notes |
|----|------|-------|
| 20 | Mountain face — dark | Cliff wall |
| 21 | Mountain face — light | Lit face |
| 22 | Mountain peak | |
| 23 | Snow cap — left | |
| 24 | Snow cap — center | |
| 25 | Snow cap — right | |
| 26 | Pine tree — top | |
| 27 | Pine tree — base | Solid |

### Props (10 tiles)
| ID | Name | Notes |
|----|------|-------|
| 28 | Torch — unlit | |
| 29 | Torch — lit | Animated 2-frame |
| 30 | Torch — frame 2 | |
| 31 | Chest — closed | |
| 32 | Chest — open | |
| 33 | Pillar — base | |
| 34 | Pillar — shaft | |
| 35 | Pillar — top | |
| 36 | Skull | Dungeon prop |
| 37 | Bones pile | |

**Sheet 3 total: 38 tiles** (26 empty slots reserved)

---

## Sheet 4 — Shared Props / UI Elements (128×128, 64 tiles)

### Character Sprites (16 tiles, 16×16 each)
| ID | Name | Notes |
|----|------|-------|
| 00 | Player — walk S frame 1 | |
| 01 | Player — walk S frame 2 | |
| 02 | Player — walk N frame 1 | |
| 03 | Player — walk N frame 2 | |
| 04 | Player — walk E frame 1 | |
| 05 | Player — walk E frame 2 | |
| 06 | NPC body A — front | Reuse for multiple NPCs |
| 07 | NPC body A — side | |
| 08 | NPC body B — front | Second body type |
| 09 | NPC body B — side | |
| 10–15 | Reserved for enemies | |

### Item Icons (16 tiles, 16×16 each)
(Populated in a later milestone)
| 16–31 | Item icons | Small 16×16 icons |

### UI Chrome (16 tiles)
| ID | Name | Notes |
|----|------|-------|
| 32 | Dialogue box — TL corner | 9-slice panel |
| 33 | Dialogue box — top | Tiling horizontal |
| 34 | Dialogue box — TR corner | |
| 35 | Dialogue box — left | Tiling vertical |
| 36 | Dialogue box — center fill | |
| 37 | Dialogue box — right | |
| 38 | Dialogue box — BL corner | |
| 39 | Dialogue box — bottom | |
| 40 | Dialogue box — BR corner | |
| 41 | Arrow — right (menu cursor) | |
| 42 | Arrow — down (scroll) | |
| 43 | Button icon — A | |
| 44 | Button icon — B | |
| 45 | Heart — full | HP |
| 46 | Heart — empty | |
| 47 | Coin | Gold icon |

**Sheet 4 total: 48 tiles** (16 empty slots reserved)

---

## Summary

| Sheet | Subject | Tiles Used | Empty Slots |
|-------|---------|-----------|-------------|
| 1 | Town | 50 | 14 |
| 2 | Forest | 42 | 22 |
| 3 | Dungeon/Mountain | 38 | 26 |
| 4 | Shared/UI | 48 | 16 |
| **Total** | | **178 tiles** | **78 slots** |

---

## Reuse Opportunities

**Palette swaps (same tile, different color pass in C2D_DrawSpriteTinted):**
- Window lit/unlit — same tile, tint applied at night
- Torches — same base tile, animated brightness
- Lamppost lit/unlit — one tile, night tint
- NPC bodies — 2 body shapes cover all 50 NPCs via hair/clothing tint

**Shared tiles across sheets:**
- Dirt path is identical in Town and Forest (only sheet changes)
- Door open/closed shape reused in Town and Dungeon (tint distinguishes)
- Barrel and Crate used in all zones

**Tileset-per-zone memory:**
Only one sheet is VRAM-resident at a time (except Sheet 4 which is always loaded).
Peak VRAM: Sheet 1 or 2 (64 KB) + Sheet 4 (64 KB) = **128 KB** — well within the 6 MB VRAM limit.
