#pragma once
#include <citro2d.h>

class Renderer {
public:
    bool init() {
        gfxInitDefault();
        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
        C2D_Prepare();

        top_screen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        bottom_screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

        if (!top_screen || !bottom_screen) {
            return false;
        }
        return true;
    }

    void shutdown() {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
    }

    void beginFrame() {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    }

    void beginTopScreen(u32 clear_color = 0x000000FF) {
        C2D_TargetClear(top_screen, clear_color);
        C2D_SceneBegin(top_screen);
    }

    void beginBottomScreen(u32 clear_color = 0x000000FF) {
        C2D_TargetClear(bottom_screen, clear_color);
        C2D_SceneBegin(bottom_screen);
    }

    void endFrame() {
        C3D_FrameEnd(0);
    }

private:
    C3D_RenderTarget* top_screen = nullptr;
    C3D_RenderTarget* bottom_screen = nullptr;
};
