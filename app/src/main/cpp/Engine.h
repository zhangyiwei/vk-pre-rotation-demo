#pragma once

#include <android_native_app_glue.h>

#include <mutex>

#include "Renderer.h"

class Engine {
private:
    struct State {
        int32_t inputX;
        int32_t inputY;
        uint64_t frameCount;

        State() : inputX(0), inputY(0), frameCount(0) {}
    };

public:
    explicit Engine() : mIsAnimating(false) {}
    bool isAnimating();
    void drawFrame();
    void onInitWindow(ANativeWindow* window, AAssetManager* assetManager);
    void onLostFocus();
    void onGainedFocus();
    void onTermWindow();
    void onSaveState(void** outSavedState, size_t* outSize);
    void onStart(void* savedState);
    int32_t onInputEvent(AInputEvent* event);

private:
    // mLock protects all members below
    std::mutex mLock;
    State mState;
    Renderer mRenderer;
    bool mIsAnimating;

    // Engine constants
    static constexpr const uint64_t kLogInterval = 100;
};
