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
    if (qi < 0) return nullptr;

    const QuestState&  qs  = m_states[qi];
    const QuestDef&    qd  = getQuestDef(static_cast<u8>(qi));

    if (qs.current_step >= qd.step_count) return nullptr;
    const QuestStep& step = qd.steps[qs.current_step];

    // Return dialogue override if this NPC is the target of the current step.
    bool npcIsTarget = (step.type == QuestStepType::TALK_TO_NPC ||
                        step.type == QuestStepType::RETURN_TO_NPC) &&
                       (step.target_npc_id == npc_id);

    if (npcIsTarget && step.npc_dialogue_override != nullptr) {
        return step.npc_dialogue_override;
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
    } else {
        LOG("Quest '%s' advanced to step %d: %s",
            qd.title, qs.current_step,
            qd.steps[qs.current_step].objective_text);
    }
}

void QuestManager::applyReward(const QuestDef& def, PlayerState& playerState) {
    playerState.addGold(def.reward.gold);
    LOG("Reward applied: +%u gold (total: %u)", def.reward.gold, playerState.gold);
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
