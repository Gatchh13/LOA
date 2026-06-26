#pragma once

//-----------------------------------------------------------------------------
// QuestDatabase.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Thin aggregator. Includes every per-quest file (source/quest/data/
// Quest_*.h) and assembles them into one lookup table. This replaces
// the old QuestDef.h, which held the shared types (QuestStep, etc — now
// in QuestStep.h) AND both quests' actual data inline in one file.
//
// What actually changed, and what didn't:
//   - QuestManager.h/.cpp: ZERO changes. QuestManager already only ever
//     called getQuestDef(quest_id) — it never touched s_questDefs[]
//     directly. "QuestManager should consume a database instead of
//     owning hardcoded quest arrays" was, on inspection, already true
//     of QuestManager itself; the hardcoded arrays the assignment means
//     are QuestDef.h's old s_questDefs[]/s_missingPackageSteps[], which
//     this file and the two Quest_*.h files replace.
//   - Adding a third quest: create source/quest/data/Quest_NewThing.h
//     (steps array + a constexpr QuestDef, following either existing
//     file as a template), add one enum entry to QuestID (QuestStep.h),
//     add one #include line below, add one row to s_questDefs[] below.
//     Four small, obvious edits — no gameplay code touched.
//
// MEMORY: unchanged. Both quests' data is static const in ROM — 0 bytes
// RAM either way; the aggregation happens entirely at compile time via
// the array-of-QuestDef literal below, which itself is also .rodata.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "QuestStep.h"
#include "data/Quest_MissingPackage.h"
#include "data/Quest_WellRepair.h"

static const QuestDef s_questDefs[] = {
    s_missingPackageDef,
    s_wellRepairDef,
};

// Number of quests actually defined in s_questDefs. Kept separate from
// MAX_QUESTS (the SaveData/runtime array capacity) intentionally — this
// catches the case where QuestID gains an entry but s_questDefs doesn't
// (or vice versa).
static constexpr int QUEST_DEF_COUNT = sizeof(s_questDefs) / sizeof(s_questDefs[0]);
static_assert(QUEST_DEF_COUNT == static_cast<int>(QuestID::COUNT),
              "s_questDefs and QuestID registry are out of sync — "
              "every entry added to one must be added to the other");
static_assert(static_cast<int>(QuestID::COUNT) <= MAX_QUESTS,
              "More quests defined than MAX_QUESTS allows — "
              "grow MAX_QUESTS and the SaveData quest arrays first");

// Accessor. quest_id must be < QUEST_DEF_COUNT.
inline const QuestDef& getQuestDef(u8 quest_id) {
    return s_questDefs[quest_id];
}

// Iteration helpers (Milestone 10 — new, same rationale as
// ItemDatabase.h's getItemCount()/getItemByIndex(): nothing previously
// needed to enumerate "every quest," but a real content pipeline
// should support it).
inline int getQuestCount() {
    return QUEST_DEF_COUNT;
}

inline const QuestDef& getQuestByIndex(int index) {
    return s_questDefs[index];
}
