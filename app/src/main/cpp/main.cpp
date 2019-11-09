#include <android/log.h>
#include <android_native_app_glue.h>

#include <mutex>

#include "VkHelper.h"

#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "VKDEMO", __VA_ARGS__))

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
    VkHelper mVkHelper;
    bool mIsAnimating;

    // Engine constants
    static constexpr const uint64_t kLogInterval = 100;
};

bool Engine::isAnimating() {
    std::lock_guard<std::mutex> lock(mLock);
    return mIsAnimating;
}

void Engine::drawFrame() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mIsAnimating) {
        return;
    }

    mVkHelper.drawFrame();

    if (++mState.frameCount % kLogInterval == 0) {
        ALOGD("%s[%" PRIu64 "]", __FUNCTION__, mState.frameCount);
    }
}

void Engine::onInitWindow(ANativeWindow* window, AAssetManager* assetManager) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mVkHelper.initialize(window, assetManager);
}

void Engine::onLostFocus() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mIsAnimating = false;
}

void Engine::onGainedFocus() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mIsAnimating = true;
}

void Engine::onTermWindow() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("%s", __FUNCTION__);
    mIsAnimating = false;
    mVkHelper.destroy();
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
        ALOGD("AINPUT_EVENT_TYPE_MOTION X[%d] Y[%d]", mState.inputX, mState.inputY);
        return 1;
    }
    return 0;
}

static int32_t handleInputEvent(android_app* app, AInputEvent* event) {
    ALOGD("%s", __FUNCTION__);
    return static_cast<Engine*>(app->userData)->onInputEvent(event);
}

static void handleAppCmd(android_app* app, int32_t cmd) {
    ALOGD("%s", __FUNCTION__);
    auto engine = static_cast<Engine*>(app->userData);
    switch (cmd) {
        case APP_CMD_INPUT_CHANGED:
            ALOGD("APP_CMD_INPUT_CHANGED");
            break;
        case APP_CMD_INIT_WINDOW:
            ALOGD("APP_CMD_INIT_WINDOW");
            engine->onInitWindow(app->window, app->activity->assetManager);
            break;
        case APP_CMD_TERM_WINDOW:
            ALOGD("APP_CMD_TERM_WINDOW");
            engine->onTermWindow();
            break;
        case APP_CMD_WINDOW_RESIZED:
            ALOGD("APP_CMD_WINDOW_RESIZED");
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
            ALOGD("APP_CMD_WINDOW_REDRAW_NEEDED");
            break;
        case APP_CMD_CONTENT_RECT_CHANGED:
            ALOGD("APP_CMD_RECT_CHANGED");
            break;
        case APP_CMD_GAINED_FOCUS:
            ALOGD("APP_CMD_GAINED_FOCUS");
            engine->onGainedFocus();
            break;
        case APP_CMD_LOST_FOCUS:
            ALOGD("APP_CMD_LOST_FOCUS");
            engine->onLostFocus();
            break;
        case APP_CMD_CONFIG_CHANGED:
            ALOGD("APP_CMD_CONFIG_CHANGED");
            break;
        case APP_CMD_LOW_MEMORY:
            ALOGD("APP_CMD_LOW_MEMORY");
            break;
        case APP_CMD_START:
            ALOGD("APP_CMD_START");
            if (app->savedState != nullptr) {
                engine->onStart(app->savedState);
            }
            break;
        case APP_CMD_RESUME:
            ALOGD("APP_CMD_RESUME");
            break;
        case APP_CMD_SAVE_STATE:
            ALOGD("APP_CMD_SAVE_STATE");
            engine->onSaveState(&app->savedState, &app->savedStateSize);
            break;
        case APP_CMD_PAUSE:
            ALOGD("APP_CMD_PAUSE");
            break;
        case APP_CMD_STOP:
            ALOGD("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            ALOGD("APP_CMD_DESTROY");
            break;
        default:
            ALOGD("UNKNOWN CMD[%d]", cmd);
            break;
    }
}

static void handleNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
    ALOGD("%s", __FUNCTION__);
    (void)activity;
    (void)window;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app* app) {
    Engine engine;

    app->userData = &engine;
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;
    app->activity->callbacks->onNativeWindowResized = handleNativeWindowResized;

    while (true) {
        int events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while (ALooper_pollAll(engine.isAnimating() ? 0 : -1, nullptr, &events, (void**)&source) >=
               0) {
            if (source != nullptr) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                ALOGD("Destroy requested");
                engine.onTermWindow();
                return;
            }
        }

        engine.drawFrame();
    }
}
