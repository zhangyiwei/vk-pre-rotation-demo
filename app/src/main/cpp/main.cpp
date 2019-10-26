#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include "vk_helper.h"

#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "VKDEMO", __VA_ARGS__))

struct State {
    float angle;
    int32_t x;
    int32_t y;

    State() : angle(0), x(0), y(0) {}
};

struct Engine {
    struct android_app* app;
    int animating;
    struct State state;
    vk_helper vk;

    Engine() : app(nullptr), animating(0), state(), vk() {}
};

static void engine_init_display(struct Engine* engine) {
    ALOGD("engine_init_display");
    engine->vk.initialize(engine->app->window, engine->app->activity->assetManager);
}

static void engine_draw_frame(struct Engine* engine) {
    static uint64_t draw_count = 0;
    engine->vk.drawFrame();
    if (++draw_count % 100 == 0) {
        ALOGD("engine_draw_frame[%" PRIu64 "]", draw_count);
    }
}

static void engine_term_display(struct Engine* engine) {
    ALOGD("engine_term_display");
    engine->animating = 0;
    engine->vk.destroy();
}

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    ALOGD("engine_handle_input");
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
            ALOGD("SAVE_STATE");
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct State));
            *((struct State*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct State);
            break;
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != nullptr) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
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
            break;
        case APP_CMD_LOST_FOCUS:
            ALOGD("LOST_FOCUS");
            // Stop animating.
            engine->animating = 0;
            break;
        default:
            ALOGD("UNKNOWN CMD[%d]", cmd);
            break;
    }
}

static void on_window_resized(ANativeActivity* activity, ANativeWindow* window) {
    ALOGD("onNativeWindowResized");
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