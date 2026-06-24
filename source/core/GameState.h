#pragma once
#include <3ds.h>

enum class GameState : u8 {
    BOOT,
    TITLE_SCREEN,
    MAIN_MENU,
    LOADING,
    GAMEPLAY,
    DIALOGUE,
    INVENTORY,
    SHOP,
    COMBAT,
    PAUSE_MENU,
    GAME_OVER,
    CREDITS
};

class StateManager {
public:
    StateManager() : top(0) {
        stack[0] = GameState::BOOT;
    }

    void push(GameState s) {
        if (top + 1 < STACK_DEPTH) {
            stack[++top] = s;
        }
        // Overflow (more than STACK_DEPTH nested states) is silently
        // ignored — the push is dropped and current() keeps returning
        // whatever was already on top. STACK_DEPTH=4 comfortably covers
        // the current usage (TITLE_SCREEN / GAMEPLAY, no nested menus
        // yet); revisit if a future milestone adds deep menu nesting.
    }

    void pop() {
        if (top > 0) {
            top--;
        }
    }

    GameState current() const {
        return stack[top];
    }

    bool is(GameState s) const {
        return stack[top] == s;
    }

private:
    static constexpr int STACK_DEPTH = 4;
    GameState stack[STACK_DEPTH];
    int top;
};

