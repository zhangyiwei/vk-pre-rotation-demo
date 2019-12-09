#include <android_native_app_glue.h>

#include "Engine.h"
#include "Utils.h"

static void handleAppCmd(android_app* app, int32_t cmd) {
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
            break;
        case APP_CMD_LOST_FOCUS:
            ALOGD("APP_CMD_LOST_FOCUS");
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
                engine->onLoadState(app->savedState);
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

static int32_t handleInputEvent(android_app* app, AInputEvent* event) {
    ALOGD("%s", __FUNCTION__);
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

    ALOGD("Platform sdkVersion = %d", app->activity->sdkVersion);

    while (true) {
        int events;
        android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
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
