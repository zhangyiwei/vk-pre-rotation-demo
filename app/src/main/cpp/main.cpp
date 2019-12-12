#include <android_native_app_glue.h>

#include "Engine.h"
#include "Utils.h"

static void handleAppCmd(android_app* app, int32_t cmd) {
    auto engine = static_cast<Engine*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            engine->onInitWindow(app->window, app->activity->assetManager);
            break;
        case APP_CMD_TERM_WINDOW:
            engine->onTermWindow();
            break;
        case APP_CMD_START:
            ALOGD("APP_CMD_START");
            if (app->savedState != nullptr) {
                engine->onLoadState(app->savedState);
            }
            break;
        case APP_CMD_SAVE_STATE:
            ALOGD("APP_CMD_SAVE_STATE");
            engine->onSaveState(&app->savedState, &app->savedStateSize);
            break;
        default:
            break;
    }
}

static int32_t handleInputEvent(android_app* app, AInputEvent* event) {
    ALOGV("%s", __FUNCTION__);
    return static_cast<Engine*>(app->userData)->onInputEvent(event);
}

static void handleNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
    auto engine = static_cast<Engine*>(static_cast<android_app*>(activity->instance)->userData);
    const int32_t width = ANativeWindow_getWidth(window);
    const int32_t height = ANativeWindow_getHeight(window);
    ALOGD("%s: W[%d], H[%d]", __FUNCTION__, width, height);

    if (width < 0 || height < 0) {
        return;
    }
    engine->onWindowResized((uint32_t)width, (uint32_t)height);
}

void android_main(android_app* app) {
    Engine engine;

    app->userData = &engine;
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;
    app->activity->callbacks->onNativeWindowResized = handleNativeWindowResized;

    while (true) {
        int events;
        android_poll_source* source;

        while (ALooper_pollAll(engine.isReady() ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
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
