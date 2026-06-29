#pragma once

//-----------------------------------------------------------------------------
// SpriteIndex.h  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Named indices into tiles.t3x, replacing the bare 0/1/2 literals that
// used to sit directly in Renderer::init(). This is the entire "asset
// pipeline preparation" this milestone does — per the brief's explicit
// "do NOT create a full asset manager if unnecessary, keep it
// lightweight," a real SpriteSheet/TextureManager class was considered
// and rejected: there is exactly one sprite sheet, with exactly 3
// sprites, and zero real art currently shipping (every sprite still
// uses Renderer's fallback colored rects — see m_useFallbackColors).
// Building a general-purpose asset manager for 3 indices that aren't
// even in use yet would be exactly the speculative architecture this
// milestone's brief warns against.
//
// What this DOES fix: "0", "1", "2" as bare literals at the only 3
// call sites that read from the sheet is real but minor friction —
// the moment a 4th sprite is added, every one of those literals has to
// be re-checked by hand for an off-by-one. A named enum costs nothing
// and removes that risk completely. Nothing more was built because
// nothing more is currently needed; see the Milestone 12 design doc's
// "Document Future Refactors" section for what a real asset pipeline
// (hundreds of sprites, multiple sheets) should look like and why it's
// explicitly NOT built now.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

enum class TileSpriteIndex : u8 {
    GRASS  = 0,
    WALL   = 1,
    PLAYER = 2,
};
