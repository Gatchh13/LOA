//-----------------------------------------------------------------------------
// QuestManager.cpp
//-----------------------------------------------------------------------------

#include "QuestManager.h"
#include "../quest/PlayerState.h"
#include "../core/Logger.h"
#include <cstring>
#include <cmath>

QuestManager::QuestManager()
    : m_questJustCompleted(false)
    , m_pendingTalkNpcId(0)
    , m_pendingTalkActive(false)
{
    memset(m_states, 0, sizeof(m_states));
}

void QuestManager::init() {
    for (int i = 0; i < MAX_QUESTS; i++) {
        m_states[i].status       = QuestStatus::NOT_STARTED;
        m_states[i].current_step = 0;
        m_states[i].step_just_completed = false;
    }
    // Auto-start the Missing Package quest so it's available immediately.
    // The first step is TALK_TO_NPC — the quest won't actually progress
    // until the player speaks to Mira, so this is just "available".
    bool started = startQuest(QUEST_MISSING_PACKAGE);
    (void)started; // always true here: nothing else is IN_PROGRESS yet
    LOG("QuestManager: initialized. Missing Package quest available.");
}

bool QuestManager::startQuest(u8 quest_id) {
    int active = findActiveQuest();
    if (active >= 0 && active != static_cast<int>(quest_id)) {
        // Another quest is already IN_PROGRESS. v1's design (HUD text,
        // quest markers, NPC dialogue overrides) assumes exactly one active
        // quest — findActiveQuest() only ever returns the first match, so
        // silently allowing a second one here would mean the first quest's
        // UI and dialogue hooks just stop updating with no error anywhere.
        // Refuse instead of corrupting that assumption.
        ERR("QuestManager::startQuest(%u) refused — quest %d is already "
            "IN_PROGRESS and only one active quest is supported (see "
            "findActiveQuest()). Complete or design multi-quest support "
            "before adding a second concurrent quest.", quest_id, active);
        return false;
    }
    if (quest_id >= MAX_QUESTS) return false;

    m_states[quest_id].status             = QuestStatus::IN_PROGRESS;
    m_states[quest_id].current_step       = 0;
    m_states[quest_id].step_just_completed = false;
    return true;
}

int QuestManager::findActiveQuest() const {
    for (int i = 0; i < MAX_QUESTS; i++) {
        if (m_states[i].status == QuestStatus::IN_PROGRESS) return i;
    }
    return -1;
}

void QuestManager::update(ZoneID currentZone, float playerX, float playerY) {
    m_questJustCompleted = false;

    int qi = findActiveQuest();
    if (qi < 0) return;

    QuestState&    qs  = m_states[qi];
    const QuestDef& qd = getQuestDef(static_cast<u8>(qi));

    if (qs.current_step >= qd.step_count) return;
    const QuestStep& step = qd.steps[qs.current_step];

    // Only REACH_MARKER is checked here; TALK steps are event-driven.
    if (step.type == QuestStepType::REACH_MARKER) {
        if (currentZone != step.marker_zone) return;

        float dx   = playerX - step.marker_x;
        float dy   = playerY - step.marker_y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist <= MARKER_RADIUS) {
            LOG("Quest %d step %d: REACH_MARKER met", qi, qs.current_step);
            // Dummy PlayerState for marker step (no reward given mid-quest,
            // but advanceStep() may call applyReward() if this happens to be
            // the final step of some future quest — init() so that read
            // is never against indeterminate memory).
            PlayerState dummy;
            dummy.init();
            advanceStep(static_cast<u8>(qi), dummy);
        }
    }
}

const char* QuestManager::onTalkToNPC(u8 npc_id) {
    // Record the NPC being talked to so onDialogueClosed can act on it.
    m_pendingTalkNpcId   = npc_id;
    m_pendingTalkActive  = true;

    int qi = findActiveQuest();
    if (qi >= 0) {
        const QuestState&  qs  = m_states[qi];
        const QuestDef&    qd  = getQuestDef(static_cast<u8>(qi));

        if (qs.current_step < qd.step_count) {
            const QuestStep& step = qd.steps[qs.current_step];

            // Return dialogue override if this NPC is the target of the
            // current step.
            bool npcIsTarget = (step.type == QuestStepType::TALK_TO_NPC ||
                                step.type == QuestStepType::RETURN_TO_NPC) &&
                               (step.target_npc_id == npc_id);

            if (npcIsTarget && step.npc_dialogue_override != nullptr) {
                return step.npc_dialogue_override;
            }
        }
    }

    // Milestone 11: no active-quest override applied. Check whether a
    // COMPLETED quest's final step targeted this NPC and has flavor
    // dialogue for after the fact (e.g. Mira commenting on the
    // delivered package once there's no active quest pointing at her
    // anymore). Lower priority than an active quest on purpose — if a
    // second quest later also targets this NPC while it's IN_PROGRESS,
    // that quest's own current-step override (checked above) always
    // wins; this is purely a fallback for "nothing is currently asking
    // anything of this NPC."
    for (int i = 0; i < getQuestCount(); i++) {
        const QuestDef& qd = getQuestByIndex(i);
        if (m_states[qd.quest_id].status != QuestStatus::COMPLETE) continue;
        if (qd.step_count == 0) continue;
        if (qd.post_complete_dialogue == nullptr) continue;

        const QuestStep& finalStep = qd.steps[qd.step_count - 1];
        bool finalStepTargetsThisNPC =
            (finalStep.type == QuestStepType::TALK_TO_NPC ||
             finalStep.type == QuestStepType::RETURN_TO_NPC) &&
            (finalStep.target_npc_id == npc_id);

        if (finalStepTargetsThisNPC) {
            return qd.post_complete_dialogue;
        }
    }

    return nullptr;
}

void QuestManager::onDialogueClosed(u8 npc_id, PlayerState& playerState) {
    if (!m_pendingTalkActive) return;
    m_pendingTalkActive = false;

    if (m_pendingTalkNpcId != npc_id) return;

    int qi = findActiveQuest();
    if (qi < 0) return;

    QuestState&    qs  = m_states[qi];
    const QuestDef& qd = getQuestDef(static_cast<u8>(qi));

    if (qs.current_step >= qd.step_count) return;
    const QuestStep& step = qd.steps[qs.current_step];

    bool stepRequiresThisNPC =
        (step.type == QuestStepType::TALK_TO_NPC ||
         step.type == QuestStepType::RETURN_TO_NPC) &&
        (step.target_npc_id == npc_id);

    if (stepRequiresThisNPC) {
        LOG("Quest %d step %d: TALK met (npc_id=%d)", qi, qs.current_step, npc_id);
        advanceStep(static_cast<u8>(qi), playerState);
    }
}

void QuestManager::advanceStep(u8 quest_id, PlayerState& playerState) {
    QuestState&    qs  = m_states[quest_id];
    const QuestDef& qd = getQuestDef(quest_id);

    qs.step_just_completed = true;
    qs.current_step++;

    if (qs.current_step >= qd.step_count) {
        // All steps done — quest complete
        qs.status            = QuestStatus::COMPLETE;
        m_questJustCompleted = true;
        applyReward(qd, playerState);
        LOG("Quest '%s' COMPLETE! Reward: %u gold", qd.title, qd.reward.gold);

        // Milestone 10: Well Repair becomes available once Missing
        // Package completes. This is a single, explicit, named
        // relationship — not a general "quest chain" or "unlock"
        // system. With exactly two quests and exactly one dependency
        // between them, a generic system would be speculative
        // architecture; this one `if` is the smallest correct way to
        // express "the second quest unlocks after the first."
        if (quest_id == QUEST_MISSING_PACKAGE) {
            bool started = startQuest(QUEST_WELL_REPAIR);
            (void)started; // always true here: Missing Package just
                            // went COMPLETE, so it's no longer the
                            // active quest blocking startQuest()'s guard
            LOG("Quest 'The Town Well' is now available.");
        }
    } else {
        LOG("Quest '%s' advanced to step %d: %s",
            qd.title, qs.current_step,
            qd.steps[qs.current_step].objective_text);
    }
}

void QuestManager::applyReward(const QuestDef& def, PlayerState& playerState) {
    playerState.addGold(def.reward.gold);

    if (def.reward.item_qty > 0) {
        bool added = playerState.inventory.addItem(def.reward.item_id, def.reward.item_qty);
        if (!added) {
            // Inventory was full — gold is still granted above, but the
            // item is lost rather than blocking quest completion. Logged
            // so it's visible during testing; not expected to happen in
            // practice with only one item-rewarding quest and 8 slots.
            WARN("Quest '%s' reward item lost — inventory full (item_id=%u, qty=%u)",
                 def.title, def.reward.item_id, def.reward.item_qty);
        }
        LOG("Reward applied: +%u gold, +%u x item %u (total gold: %u)",
            def.reward.gold, def.reward.item_qty, def.reward.item_id, playerState.gold);
    } else {
        LOG("Reward applied: +%u gold (total: %u)", def.reward.gold, playerState.gold);
    }
}

bool QuestManager::hasActiveQuest() const {
    return findActiveQuest() >= 0;
}

const char* QuestManager::getActiveObjectiveText() const {
    int qi = findActiveQuest();
    if (qi < 0) return nullptr;

    const QuestState&  qs = m_states[qi];
    const QuestDef&    qd = getQuestDef(static_cast<u8>(qi));

    if (qs.current_step >= qd.step_count) return nullptr;
    return qd.steps[qs.current_step].objective_text;
}

QuestStatus QuestManager::getStatus(u8 quest_id) const {
    return m_states[quest_id].status;
}

bool QuestManager::markerVisible(ZoneID currentZone) const {
    int qi = findActiveQuest();
    if (qi < 0) return false;

    const QuestState&  qs  = m_states[qi];
    const QuestDef&    qd  = getQuestDef(static_cast<u8>(qi));

    if (qs.current_step >= qd.step_count) return false;
    const QuestStep& step = qd.steps[qs.current_step];

    return (step.type == QuestStepType::REACH_MARKER) &&
           (step.marker_zone == currentZone);
}

float QuestManager::getMarkerX() const {
    int qi = findActiveQuest();
    if (qi < 0) return 0.0f;
    const QuestState& qs = m_states[qi];
    const QuestDef&   qd = getQuestDef(static_cast<u8>(qi));
    if (qs.current_step >= qd.step_count) return 0.0f;
    return qd.steps[qs.current_step].marker_x;
}

float QuestManager::getMarkerY() const {
    int qi = findActiveQuest();
    if (qi < 0) return 0.0f;
    const QuestState& qs = m_states[qi];
    const QuestDef&   qd = getQuestDef(static_cast<u8>(qi));
    if (qs.current_step >= qd.step_count) return 0.0f;
    return qd.steps[qs.current_step].marker_y;
}

u8 QuestManager::getCurrentStep(u8 quest_id) const {
    if (quest_id >= MAX_QUESTS) return 0;
    return m_states[quest_id].current_step;
}

void QuestManager::setStateFromSave(u8 quest_id, QuestStatus status, u8 step) {
    if (quest_id >= MAX_QUESTS) return;
    m_states[quest_id].status             = status;
    m_states[quest_id].current_step       = step;
    m_states[quest_id].step_just_completed = false;
}
