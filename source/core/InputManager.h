#pragma once
#include <3ds.h>

class InputManager {
public:
    void update() {
        hidScanInput();
        held = hidKeysHeld();
        down = hidKeysDown();
        up   = hidKeysUp();
    }

    bool isHeld(u32 key) const { return held & key; }
    bool isDown(u32 key) const { return down & key; } // pressed this frame
    bool isUp(u32 key) const   { return up & key; }   // released this frame

    // Convenience: 4-direction movement intent from D-pad only.
    // No analog circle-pad reliance — keeps controls identical on Old/New 3DS.
    void getMoveDir(s8& dx, s8& dy) const {
        dx = 0;
        dy = 0;
        if (held & KEY_DLEFT)  dx -= 1;
        if (held & KEY_DRIGHT) dx += 1;
        if (held & KEY_DUP)    dy -= 1;
        if (held & KEY_DDOWN)  dy += 1;
    }

private:
    u32 held = 0;
    u32 down = 0;
    u32 up   = 0;
};
