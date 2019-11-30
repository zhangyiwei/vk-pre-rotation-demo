#include "Engine.h"

#include "Utils.h"

bool Engine::isAnimating() {
    std::lock_guard<std::mutex> lock(mLock);
    return mState.isAnimating;
}

void Engine::drawFrame() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mState.isAnimating || !mIsRendererReady) {
        return;
    }
    mRenderer.drawFrame();
}

void Engine::onInitWindow(ANativeWindow* window, AAssetManager* assetManager) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mRenderer.initialize(window, assetManager);
    mIsRendererReady = true;
}

void Engine::onLostFocus() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mState.isAnimating = false;
}

void Engine::onGainedFocus() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mState.isAnimating = true;
}

void Engine::onTermWindow() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mRenderer.destroy();
    mIsRendererReady = false;
}

void Engine::onSaveState(void** outSavedState, size_t* outSize) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    *outSize = sizeof(State);
    *outSavedState = malloc(*outSize);
    memcpy(*outSavedState, &mState, *outSize);
}

void Engine::onStart(void* savedState) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    if (savedState) {
        memcpy(&mState, savedState, sizeof(State));
        ALOGD("Restored from previous saved state");
    }
}

int32_t Engine::onInputEvent(AInputEvent* event) {
    std::lock_guard<std::mutex> lock(mLock);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        mState.inputX = static_cast<int32_t>(AMotionEvent_getX(event, 0));
        mState.inputY = static_cast<int32_t>(AMotionEvent_getY(event, 0));
        ALOGD("%s: X[%d] Y[%d]", __FUNCTION__, mState.inputX, mState.inputY);
        return 1;
    }
    return 0;
}
