/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Engine.h"

#include "Utils.h"

bool Engine::isReady() {
    std::lock_guard<std::mutex> lock(mLock);
    return mIsRendererReady;
}

void Engine::drawFrame() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mIsRendererReady) {
        mRenderer.drawFrame();
    }
}

void Engine::onInitWindow(ANativeWindow* window, AAssetManager* assetManager) {
    ALOGD("%s", __FUNCTION__);
    std::lock_guard<std::mutex> lock(mLock);
    mRenderer.initialize(window, assetManager);
    mIsRendererReady = true;
}

void Engine::onWindowResized(uint32_t width, uint32_t height) {
    ALOGD("%s", __FUNCTION__);
    std::lock_guard<std::mutex> lock(mLock);
    if (mIsRendererReady) {
        mRenderer.updateSurface(width, height);
    }
}

void Engine::onTermWindow() {
    ALOGD("%s", __FUNCTION__);
    std::lock_guard<std::mutex> lock(mLock);
    if (mIsRendererReady) {
        mRenderer.destroy();
        mIsRendererReady = false;
    }
}

void Engine::onSaveState(void** outSavedState, size_t* outSize) {
    ALOGD("%s", __FUNCTION__);
    *outSize = sizeof(State);
    *outSavedState = malloc(*outSize);

    std::lock_guard<std::mutex> lock(mLock);
    memcpy(*outSavedState, &mState, *outSize);
}

void Engine::onLoadState(void* savedState) {
    if (savedState) {
        ALOGD("%s", __FUNCTION__);
        std::lock_guard<std::mutex> lock(mLock);
        memcpy(&mState, savedState, sizeof(State));
    }
}

int32_t Engine::onInputEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        auto inputX = static_cast<int32_t>(AMotionEvent_getX(event, 0));
        auto inputY = static_cast<int32_t>(AMotionEvent_getY(event, 0));
        ALOGD("%s inputX[%d] inputY[%d]", __FUNCTION__, inputX, inputY);

        std::lock_guard<std::mutex> lock(mLock);
        mState.inputX = inputX;
        mState.inputY = inputY;
        return 1;
    }
    return 0;
}

uint32_t Engine::getDelayMillis(int64_t /*frameTimeNanos*/) {
    // we can play around with frameTimeNanos to add more dynamic callback delay control
    return kDelayMillis;
}
