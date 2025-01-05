#include "platform.h"
#include "app.h"
#include "event_system.h"
#include "input_system.h"
#include "logging.h"
#include <SDL3/SDL.h>
#include <thread>

static bool on_key_down(EventCode event_code, void *sender, void *listener, EventContext event_context);
static bool on_key_up(EventCode event_code, void *sender, void *listener, EventContext event_context);
static bool on_mouse_button_down(EventCode event_code, void *sender, void *listener, EventContext event_context);
static bool on_mouse_button_up(EventCode event_code, void *sender, void *listener, EventContext event_context);

void create_window(PlatformContext *platform_context, uint16_t width, uint16_t height) {
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    // flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    platform_context->window = SDL_CreateWindow("mclaren", width, height, flags);
    ASSERT_MESSAGE(platform_context->window, "SDL_CreateWindow failed: %s", SDL_GetError());
    bool succeed = SDL_RaiseWindow(platform_context->window);
    ASSERT_MESSAGE(succeed, "SDL_RaiseWindow failed: %s", SDL_GetError());
}

void platform_init(PlatformContext *platform_context) {
    bool succeed = SDL_Init(SDL_INIT_VIDEO);
    ASSERT_MESSAGE(succeed, "SDL_Init failed: %s", SDL_GetError());
    create_window(platform_context, 768, 576);
    event_system_create(&platform_context->event_system_state);
    input_system_create(platform_context->event_system_state, &platform_context->input_system_state);
    {
        unsigned int cpu_processors = std::thread::hardware_concurrency();
        log_debug("number of cpu processors: %d", cpu_processors);
    }
    app_create(platform_context->window, &platform_context->app);

    event_register(platform_context->event_system_state, EVENT_CODE_KEY_DOWN, platform_context->app, on_key_down);
    event_register(platform_context->event_system_state, EVENT_CODE_KEY_UP, platform_context->app, on_key_up);
    event_register(platform_context->event_system_state, EVENT_CODE_MOUSE_BUTTON_DOWN, platform_context->app, on_mouse_button_down);
    event_register(platform_context->event_system_state, EVENT_CODE_MOUSE_BUTTON_UP, platform_context->app, on_mouse_button_up);
}

static Key sdl_key_to_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_W: return KEY_W;
        case SDLK_A: return KEY_A;
        case SDLK_S: return KEY_S;
        case SDLK_D: return KEY_D;
        case SDLK_Q: return KEY_Q;
        case SDLK_E: return KEY_E;
        case SDLK_ESCAPE: return KEY_ESC;
        case SDLK_UP: return KEY_UP;
        case SDLK_DOWN: return KEY_DOWN;
        case SDLK_LEFT: return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;
        case SDLK_SPACE: return KEY_SPACE;
        default: return KEY_UNKNOWN;
    }
}

static MouseButton sdl_mouse_button_to_mouse_button(Uint8 button) {
    switch (button) {
        case 1: return MOUSE_BUTTON_LEFT;
        case 2: return MOUSE_BUTTON_MIDDLE;
        case 3: return MOUSE_BUTTON_RIGHT;
        default: return MOUSE_BUTTON_UNKNOWN;
    }
}

void platform_main_loop(PlatformContext *platform_context) {
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (const Key key = sdl_key_to_key(event.key.key); key == KEY_ESC) {
                    quit = true;
                } else {
                    input_process_key(platform_context->input_system_state, key, false);
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                const Key key = sdl_key_to_key(event.key.key);
                input_process_key(platform_context->input_system_state, key, true);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                const MouseButton mouse_button = sdl_mouse_button_to_mouse_button(event.button.button);
                input_process_mouse_button(platform_context->input_system_state, mouse_button, true, event.button.x, event.button.y);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                const MouseButton mouse_button = sdl_mouse_button_to_mouse_button(event.button.button);
                input_process_mouse_button(platform_context->input_system_state, mouse_button, false, event.button.x, event.button.y);
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                input_process_mouse_move(platform_context->input_system_state, event.motion.x, event.motion.y);
                app_mouse_move(platform_context->app, event.motion.x, event.motion.y);
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                app_resize(platform_context->app, event.window.data1, event.window.data2);
            }
            if (quit) { break; }
        } // end polling events

        event_system_update(platform_context->event_system_state);
        app_update(platform_context->app, platform_context->input_system_state);
        input_system_update(platform_context->input_system_state);
    }
}

void platform_terminate(PlatformContext *platform_context) {
  event_unregister(platform_context->event_system_state, EVENT_CODE_MOUSE_BUTTON_UP, platform_context->app, on_mouse_button_up);
    event_unregister(platform_context->event_system_state, EVENT_CODE_MOUSE_BUTTON_DOWN, platform_context->app, on_mouse_button_down);
    event_unregister(platform_context->event_system_state, EVENT_CODE_KEY_UP, platform_context->app, on_key_up);
    event_unregister(platform_context->event_system_state, EVENT_CODE_KEY_DOWN, platform_context->app, on_key_down);
    app_destroy(platform_context->app);
    input_system_destroy(platform_context->input_system_state);
    event_system_destroy(platform_context->event_system_state);
    SDL_DestroyWindow(platform_context->window);
}

bool on_key_down(EventCode event_code, void *sender, void *listener, EventContext event_context) {
    app_key_down((App *) listener, (Key) event_context.data.u32[0]);
    return true;
}

bool on_key_up(EventCode event_code, void *sender, void *listener, EventContext event_context) {
    app_key_up((App *) listener, (Key) event_context.data.u32[0]);
    return true;
}

bool on_mouse_button_down(EventCode event_code, void *sender, void *listener, EventContext event_context) {
    app_mouse_button_down((App *) listener, (MouseButton) event_context.data.u32[0], event_context.data.f32[1], event_context.data.f32[2]);
    return true;
}

bool on_mouse_button_up(EventCode event_code, void *sender, void *listener, EventContext event_context) {
    app_mouse_button_up((App *) listener, (MouseButton) event_context.data.u32[0], event_context.data.f32[1], event_context.data.f32[2]);
    return true;
}
