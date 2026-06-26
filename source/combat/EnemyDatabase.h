#pragma once

//-----------------------------------------------------------------------------
// EnemyDatabase.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Declarations only — mirrors ItemDatabase.h/NPCDatabase.h's split (the
// actual const data table lives in exactly one .cpp). See EnemyDef.h
// for the struct itself.
//
// Adding a new enemy type: add one EnemyType entry, one EnemyDef row in
// EnemyDatabase.cpp, done. EnemyManager.cpp's AI logic (Idle/Chase/
// Attack/Return) is already written generically against whatever
// EnemyDef an Enemy instance references — it doesn't special-case Wolf
// anywhere, so a second type needs zero changes to EnemyManager.cpp's
// behavior, only a new SpawnTable entry (see SpawnTable.h) to actually
// place an instance of it in a zone.
//-----------------------------------------------------------------------------

#include "EnemyDef.h"
