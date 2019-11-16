#pragma once

#include <android_native_app_glue.h>

#include <mutex>

#include "Renderer.h"

class Engine {
private:
    struct State {
        uint64_t frameCount;
        int32_t inputX;
        int32_t inputY;
        bool isAnimating;

        State() : frameCount(0), inputX(0), inputY(0), isAnimating(false) {}
    };

public:
    explicit Engine() : mIsRendererReady(false) {}
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
    bool mIsRendererReady;

    // Engine constants
    static constexpr const uint64_t kLogInterval = 100;
};
