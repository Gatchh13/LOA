#pragma once

//-----------------------------------------------------------------------------
// NPCDatabase.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Just the include + declarations for NPCDatabase.cpp's table — mirrors
// ItemDatabase.h/.cpp's split (declarations in the header, the actual
// const data table in exactly one .cpp). See NPCDef.h for the struct
// itself and the accessor function signatures.
//
// Adding a new NPC: write one schedule array + one NPCDef row in
// NPCDatabase.cpp, add one entry to MAX_NPCS-worth of slots if needed
// (currently 16, 3 used), done. No changes anywhere in NPCManager.cpp's
// logic (update/moveNPC/tryInteract/etc. are all already defIndex-
// driven, not per-NPC special-cased).
//-----------------------------------------------------------------------------

#include "NPCDef.h"
