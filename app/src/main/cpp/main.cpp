#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <mutex>

#include "VkHelper.h"

#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "VKDEMO", __VA_ARGS__))

struct State {
    int32_t inputX;
    int32_t inputY;

    State() : inputX(0), inputY(0) {}
};

struct Engine {
    android_app* app;

    // this lock protects all fields below
    std::mutex lock;
    State state;
    VkHelper vk;
    bool animating;
    uint64_t count;

    Engine() : app(nullptr), lock(), state(), vk(), animating(false), count(0) {}
};

static bool engineIsAnimating(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    return engine->animating;
}

static void engineInitDisplay(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->count = 0;
    engine->vk.initialize(engine->app->window, engine->app->activity->assetManager);
}

static void engineDrawFrame(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    if (!engine->animating) {
        return;
    }

    engine->vk.drawFrame();

    static constexpr const uint64_t kLogInterval = 100;
    if (++engine->count % kLogInterval == 0) {
        ALOGD("%s[%" PRIu64 "]", __FUNCTION__, engine->count);
    }
}

static void enginePauseDisplay(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->animating = false;
}

static void engineResumeDisplay(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->animating = true;
}

static void engineTermDisplay(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->animating = false;
    engine->count = 0;
    engine->vk.destroy();
}

static void engineSaveState(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->app->savedState = malloc(sizeof(State));
    *((State*)engine->app->savedState) = engine->state;
    engine->app->savedStateSize = sizeof(State);
}

static void engineRestoreState(Engine* engine) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    engine->state = *(State*)engine->app->savedState;
}

static int32_t engineUpdateState(Engine* engine, AInputEvent* event) {
    std::lock_guard<std::mutex> lock(engine->lock);
    ALOGD("%s", __FUNCTION__);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->state.inputX = static_cast<int32_t>(AMotionEvent_getX(event, 0));
        engine->state.inputY = static_cast<int32_t>(AMotionEvent_getY(event, 0));
        return 1;
    }
    return 0;
}

static int32_t handleInputEvent(android_app* app, AInputEvent* event) {
    ALOGD("%s", __FUNCTION__);
    return engineUpdateState(static_cast<Engine*>(app->userData), event);
}

static void handleAppCmd(android_app* app, int32_t cmd) {
    ALOGD("%s", __FUNCTION__);
    auto engine = static_cast<Engine*>(app->userData);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            ALOGD("SAVE_STATE");
            engineSaveState(engine);
            break;
        case APP_CMD_INIT_WINDOW:
            ALOGD("INIT_WINDOW");
            engineInitDisplay(engine);
            break;
        case APP_CMD_TERM_WINDOW:
            ALOGD("TERM_WINDOW");
            engineTermDisplay(engine);
            break;
        case APP_CMD_CONTENT_RECT_CHANGED:
            ALOGD("RECT_CHANGED");
            break;
        case APP_CMD_CONFIG_CHANGED:
            ALOGD("CONFIG_CHANGED");
            break;
        case APP_CMD_WINDOW_RESIZED:
            ALOGD("WINDOW_RESIZED");
            break;
        case APP_CMD_INPUT_CHANGED:
            ALOGD("INPUT_CHANGED");
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
            ALOGD("WINDOW_REDRAW_NEEDED");
            break;
        case APP_CMD_GAINED_FOCUS:
            ALOGD("GAINED_FOCUS");
            engineResumeDisplay(engine);
            break;
        case APP_CMD_LOST_FOCUS:
            ALOGD("LOST_FOCUS");
            enginePauseDisplay(engine);
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

    engine.app = app;

    if (app->savedState != nullptr) {
        engineRestoreState(&engine);
    }

    while (true) {
        int events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while (ALooper_pollAll(engineIsAnimating(&engine) ? 0 : -1, nullptr, &events,
                               (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                engineTermDisplay(&engine);
                return;
            }
        }

        engineDrawFrame(&engine);
    }
}
