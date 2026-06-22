**PURPOSE:**
This document outlines the strict scope reduction and production realities for Version 1 (V1) of *Legends of Aetheria*. To ensure a commercially viable 10–20 hour RPG built by a solo developer for Nintendo 3DS hardware, we must aggressively prioritize core biomes, delay complex traversal mechanics, and streamline dungeon designs.

---

## 1. World Tiers

To manage production, all previously designed regions are now strictly tiered.

### Version 1 (Launch Content)

* **The Verdant Cradle:** Required as a low-complexity tutorial zone. Grassland assets are cheap to produce and reuse.
* **The Whispering Woods:** Builds directly on Cradle assets (flora/trees) but introduces lighting changes and dense navigation.
* **Glimmering Loch:** Provides high visual contrast (water focus) to the first two regions without requiring extreme verticality.
* **Aetheria Heartlands:** Required as the climax of V1 and the central hub connecting all future zones.

### Version 2 (Post-Launch / DLC)

* **The Sunstone Plateaus:** *Deferred.* The sheer verticality, canyon sightlines, and reliance on wind/gliding mechanics pose significant 3DS camera and performance risks for a V1 launch.
* **The Ashen Tangle:** *Deferred.* Requires an entirely bespoke, late-game asset set (petrified wood, mist shaders) that breaks the solo developer asset pipeline for V1.

### Expansion Content (Long-Term Reserve)

* **The Shimmering Wastes, Skybound Archipelago, Abyssal Trench, Clockwork Badlands, Ashen Tropics.** *Deferred indefinitely.* These require massive mechanical additions (swimming, flight) and entirely unique visual identities.

---

## 2. Version 1 Scope

The physical world for V1 is locked to the following exact parameters to guarantee a 10–20 hour completion time.

**Regions (4)**

1. The Verdant Cradle
2. The Whispering Woods
3. Glimmering Loch
4. Aetheria Heartlands

**Towns (3)**

1. Oakhaven (The Cradle)
2. Mistwood Village (The Woods)
3. Lapis Cove (The Loch)

**Capital City (1)**

1. Aethercrest (Heartlands)

**Major Dungeons (4)**

1. The Overgrown Aqueduct (The Cradle)
2. Roots of the Great Ancestor (The Woods)
3. The Sunken Observatory (The Loch)
4. The Clockwork Belfry (Heartlands)

**Optional Mini-Dungeons (2)**

1. *Fungal Grotto* (Repurposed from a major dungeon to a 2-room mini-dungeon beneath Glimmering Loch for optional loot).
2. *The Hollowed Elder Tree* (Adapted from a landmark into a short, combat-lite traversal challenge in The Woods).

*(Note: Emberforge, Highwind Enclave, and the remaining 6 dungeons are strictly cut from V1).*

---

## 3. Progression Route

The player's physical journey through V1 is designed to loop back on itself, reusing spaces through shortcuts rather than building endless linear corridors.

**Phase 1: The Awakening**

1. Oakhaven (Starter Town)
2. The Verdant Cradle (Exploration)
3. The Overgrown Aqueduct (Dungeon 1)

**Phase 2: The Canopy**
4.  The Whispering Woods (Exploration)
5.  Mistwood Village (Town 2)
6.  Roots of the Great Ancestor (Dungeon 2)

**Phase 3: The Waters**
7.  *Shortcut Unlocked:* Woods back to Cradle
8.  Glimmering Loch (Exploration)
9.  Lapis Cove (Town 3)
10. The Sunken Observatory (Dungeon 3)

**Phase 4: The Core**
11. Aetheria Heartlands (Exploration)
12. Aethercrest (Capital City)
13. The Clockwork Belfry (Dungeon 4 - V1 Finale)

---

## 4. Landmark Prioritization

Of the 50 originally proposed landmarks, we are severely reducing the scope for V1. Building 50 unique POI assets will sink the solo development schedule.

### Essential (Build for V1)

*Reasoning: These act as critical visual anchors ("Weenies") to guide the player through the critical path without UI waypoints.*

* The Giant's Windmill (Cradle)
* The Overgrown Archway (Cradle)
* The Hollowed Elder Tree (Woods - upgraded to Mini-Dungeon)
* The Rusted Paddlewheel (Loch)
* The Lighthouse of Lanterns (Loch - integrated into Lapis Cove)
* The Grand Aqueduct (Heartlands)
* The King's Crossroads (Heartlands)

### Nice To Have (Build if time permits)

*Reasoning: High visual impact, but can be built using scaled-up or re-tinted existing modular assets.*

* The Twin Ponds (Cradle)
* The Luminous Glade (Woods)
* The Giant Ship Ribs (Loch)
* The Crystal Pavilion (Heartlands)

### Future Content (Do NOT build for V1)

*Reasoning: These exist in V2/Expansion regions, or require bespoke modeling that steals time from core gameplay.*

* The Howling Needle, The Stepped Oasis, The Shattered Astrolabe, The Petrified Cascade, etc. (All Plateaus/Tangle landmarks).
* Any landmark requiring unique particle effects (e.g., The Scorched Crater, Steaming Fissure).

---

## 5. Mechanical Dependencies

To achieve a stable 3DS framerate and respect the gameplay programmer's time, we must separate world design from unpromised mechanics.

**Available At Launch (Design V1 around these):**

* **Walking / Running:** Primary traversal.
* **Basic Shortcuts:** Kick-down ladders, one-way doors, breakable rock walls.
* **Bridge Repair:** Diegetic progression gating (resource sinks).
* **Water Manipulation (Basic):** Raising/lowering static water planes via switches (Aqueduct dungeon).

**Requires Future Mechanic (Do NOT design V1 around these):**

* **Mounts:** Aetheria Heartlands has wide roads to accommodate them eventually, but V1 traversal is strictly on foot.
* **Ferry Travel:** *Deferred.* Lapis Cove will have docks, but active boat traversal on the Loch is cut for V1. The Loch will be traversed via wooden boardwalks and island-hopping.
* **Gliding / Wind Drafts:** *Deferred to V2.* * **Minecarts:** *Deferred to V2.*
* **Bioluminescent Spore Clearing:** *Deferred.* Fungal Grotto will rely on standard key/door progression instead of complex throwing/cloud-clearing mechanics.

---

## 6. Production Risk Assessment

As the Lead World Designer, I have identified the following physical design elements that threaten the V1 solo-developer production schedule:

**Risk 1: The Clockwork Belfry (Dungeon 4)**

* *Issue:* "Rotating entire rooms" is a physics and camera nightmare, especially on the 3DS. It will break collision meshes and cause clipping.
* *Simplification:* Do not rotate rooms. Rotate a central pillar of stairs/bridges. The rooms remain static; only the connecting pathways move.

**Risk 2: Glimmering Loch Water Rendering**

* *Issue:* Extensive translucent water planes with reflections will tank the 3DS framerate.
* *Simplification:* Design the Loch with thick, low-hanging mist (fog) to cull draw distance. Use stylized, opaque turquoise textures for the water surface with animated UV maps, rather than true transparency.

**Risk 3: Verticality in Mistwood Village**

* *Issue:* Navigating treehouses via thin branches creates frustrating platforming and camera occlusion in a top-down/isometric RPG.
* *Simplification:* Restrict the village to two wide, flat planes (Ground Floor, Canopy Floor). Use wide, fenced spiral ramps to connect them. Remove precision jumping requirements.

**Risk 4: Aethercrest (Capital) Population**

* *Issue:* A city of "5,000" implies massive scale and too many empty houses to model.
* *Simplification:* Design Aethercrest as a "districts" map. The player only accesses the Merchant Plaza, the Crafting Tier, and the Palace Steps. Residential areas are blocked by decorative gates and painted into the skybox/background.

---

## 7. Version 1 World Map Summary

This is the finalized, production-ready blueprint for the physical release of *Legends of Aetheria* V1.

* **Total Regions:** 4 (Cradle, Woods, Loch, Heartlands)
* **Total Towns:** 3 + 1 Capital (Oakhaven, Mistwood, Lapis Cove + Aethercrest)
* **Total Dungeons:** 4 Major, 2 Mini
* **Core Loop:** On-foot exploration, linear biome progression unlocking a centralized hub, heavy reliance on physical shortcuts (ladders/bridges) to interconnect the 4 zones.
* **Exploration Vibe:** Dense, cozy, and handcrafted. By shrinking the map size and cutting mounts/vehicles, the world will feel packed with intention rather than stretched thin.

*All other previously designed concepts are officially documented and securely archived for post-launch development.*
