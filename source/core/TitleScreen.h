#pragma once

//-----------------------------------------------------------------------------
// TitleScreen.h  (Milestone 6 — Foundation of Feel)
//
// The screen shown on boot, before any gameplay systems are touched.
// Three options: New Game / Continue / Credits.
//   - Continue is disabled (skipped during navigation, drawn dimmed) when
//     no valid save exists — SaveManager::hasSave() is checked once on
//     enter() rather than every frame, since the save file can't change
//     while this screen is up.
//   - Credits is a stub for now: selecting it shows a short static credits
//     panel; B returns to the title options.
//
// This class owns only UI/navigation state. It does NOT touch any
// gameplay system (Player, ZoneManager, etc) — main.cpp reads
// TitleScreen::getResult() after update() to decide what to do next.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../input/InputManager.h"

enum class TitleOption : u8 {
    NEW_GAME = 0,
    CONTINUE = 1,
    CREDITS  = 2,
    COUNT    = 3,
};

enum class TitleResult : u8 {
    NONE,        // still on the title screen, no decision made yet
    START_NEW,   // player chose New Game
    START_LOAD,  // player chose Continue
};

class TitleScreen {
public:
    // hasSave: whether Continue should be selectable. Checked once here
    // rather than polled every frame — see header comment.
    void enter(bool hasSave) {
        m_hasSave       = hasSave;
        m_selected      = TitleOption::NEW_GAME;
        m_showingCredits = false;
        m_result        = TitleResult::NONE;
    }

    // Returns the result of this frame's input. NONE until the player
    // confirms an actionable option. main.cpp should check this every
    // frame while the title screen is active and transition out on
    // anything other than NONE.
    TitleResult update(const InputManager& input) {
        if (m_showingCredits) {
            if (input.isPressed(KEY_B) || input.isPressed(KEY_A)) {
                m_showingCredits = false;
            }
            return TitleResult::NONE;
        }

        if (input.isPressed(KEY_DOWN)) {
            advance(+1);
        } else if (input.isPressed(KEY_UP)) {
            advance(-1);
        }

        if (input.isPressed(KEY_A) || input.isPressed(KEY_START)) {
            switch (m_selected) {
                case TitleOption::NEW_GAME:
                    return TitleResult::START_NEW;
                case TitleOption::CONTINUE:
                    // Should be unreachable if !m_hasSave (advance() skips
                    // it), but guard anyway in case selection state is ever
                    // set some other way in the future.
                    if (m_hasSave) return TitleResult::START_LOAD;
                    break;
                case TitleOption::CREDITS:
                    m_showingCredits = true;
                    break;
                default:
                    break;
            }
        }

        return TitleResult::NONE;
    }

    TitleOption getSelected() const { return m_selected; }
    bool        hasSave() const { return m_hasSave; }
    bool        isShowingCredits() const { return m_showingCredits; }

private:
    TitleOption m_selected       = TitleOption::NEW_GAME;
    TitleResult m_result         = TitleResult::NONE;
    bool        m_hasSave        = false;
    bool        m_showingCredits = false;

    // Move selection by `dir` (+1 down, -1 up), wrapping around, and
    // skipping CONTINUE entirely if no save exists so it can never be
    // landed on by navigation (only NEW_GAME and CREDITS are reachable).
    void advance(int dir) {
        int count = static_cast<int>(TitleOption::COUNT);
        int next  = static_cast<int>(m_selected);
        for (int i = 0; i < count; i++) {
            next = (next + dir + count) % count;
            if (static_cast<TitleOption>(next) == TitleOption::CONTINUE && !m_hasSave) {
                continue; // skip over the disabled option
            }
            break;
        }
        m_selected = static_cast<TitleOption>(next);
    }
};
