#include <jni.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include "vk_helper.h"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "VKDEMO", __VA_ARGS__))

struct State {
    float angle;
    int32_t x;
    int32_t y;

    State(): angle(0), x(0), y(0) {}
};

struct Engine {
    struct android_app* app;
    int animating;
    struct State state;
    vk_helper vk;

    Engine(): app(nullptr), animating(0), state(), vk() {}
};

static void engine_draw_frame(struct Engine* engine) {
    engine->vk.drawFrame();
}

static void engine_term_display(struct Engine* engine) {
    (void)engine;
}

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    auto engine = static_cast<Engine*>(app->userData);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = static_cast<int32_t>(AMotionEvent_getX(event, 0));
        engine->state.y = static_cast<int32_t>(AMotionEvent_getY(event, 0));
        return 1;
    }
    return 0;
}

static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    auto engine = static_cast<Engine*>(app->userData);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            LOGD("HAHA: SAVE_STATE");
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct State));
            *((struct State*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct State);
            break;
        case APP_CMD_INIT_WINDOW:
            LOGD("HAHA: INIT_WINDOW");
            engine->vk.initialize(engine->app->window, app->activity->assetManager);
            engine_draw_frame(engine);
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_CONTENT_RECT_CHANGED:
            LOGD("HAHA: RECT_CHANGED");
            break;
        case APP_CMD_CONFIG_CHANGED:
            LOGD("HAHA: CONFIG_CHANGED");
            break;
        case APP_CMD_WINDOW_RESIZED:
            LOGD("HAHA: WINDOW_RESIZED");
            break;
        case APP_CMD_INPUT_CHANGED:
            LOGD("HAHA: INPUT_CHANGED");
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
            LOGD("HAHA: WINDOW_REDRAW_NEEDED");
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGD("HAHA: GAINED_FOCUS");
            break;
        case APP_CMD_LOST_FOCUS:
            LOGD("HAHA: LOST_FOCUS");
            // Stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
        default:
            LOGD("HAHA: UNKNOWN CMD[%d]", cmd);
            break;
    }
}

static void on_window_resized(ANativeActivity *activity, ANativeWindow *window) {
    LOGD("HAHA: onNativeWindowResized");
    (void)activity;
    (void)window;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* app) {
    struct Engine engine;

    app->userData = &engine;
    app->onAppCmd = engine_handle_cmd;
    app->onInputEvent = engine_handle_input;
    app->activity->callbacks->onNativeWindowResized = on_window_resized;

    engine.app = app;

    if (app->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct State*)app->savedState;
    }

    while (true) {
        // Read all pending events.
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while (ALooper_pollAll(engine.animating ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(app, source);
            }

            // Check if we are exiting.
            if (app->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}