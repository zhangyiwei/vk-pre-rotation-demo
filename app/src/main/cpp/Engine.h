#pragma once

#include <android_native_app_glue.h>

#include <mutex>

#include "Renderer.h"

class Engine {
private:
    struct State {
        int32_t inputX;
        int32_t inputY;

        State() : inputX(0), inputY(0) {}
    };

public:
    explicit Engine() : mIsRendererReady(false) {}
    bool isReady();
    void drawFrame();
    void onInitWindow(ANativeWindow* window, AAssetManager* assetManager);
    void onWindowResized(uint32_t width, uint32_t height);
    void onTermWindow();
    void onSaveState(void** outSavedState, size_t* outSize);
    void onLoadState(void* savedState);
    int32_t onInputEvent(AInputEvent* event);
    uint32_t getDelayMillis(int64_t frameTimeNanos);

private:
    // mLock protects all members below
    std::mutex mLock;
    State mState;
    Renderer mRenderer;
    bool mIsRendererReady;

    // defer 13ms to target 60Hz on a 60Hz display or 45Hz on a 90Hz display
    static constexpr const uint32_t kDelayMillis = 13;
};
