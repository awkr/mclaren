#include "platform.h"
#include "app.h"
#include "core/logging.h"
#include "event_system.h"
#include "input_system.h"
#include <SDL3/SDL.h>
#include <thread>

void create_window(PlatformContext *platform_context, uint16_t width, uint16_t height) {
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    // flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    platform_context->window = SDL_CreateWindow("mclaren", width, height, flags);
    ASSERT_MESSAGE(platform_context->window, "SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_bool succeed = SDL_RaiseWindow(platform_context->window);
    ASSERT_MESSAGE(succeed == SDL_TRUE, "SDL_RaiseWindow failed: %s", SDL_GetError());
}

void platform_init(PlatformContext *platform_context) {
    SDL_bool succeed = SDL_Init(SDL_INIT_VIDEO);
    ASSERT_MESSAGE(succeed == SDL_TRUE, "SDL_Init failed: %s", SDL_GetError());
    create_window(platform_context, 640, 480);
    event_system_create(&platform_context->event_system_state);
    input_system_create(&platform_context->input_system_state);
    {
        unsigned int cpu_processors = std::thread::hardware_concurrency();
        log_debug("number of cpu processors: %d", cpu_processors);
    }
    app_create(platform_context->window, &platform_context->app);
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

void platform_main_loop(PlatformContext *platform_context) {
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                quit = true;
                break;
            } else if (event.type == SDL_EVENT_KEY_UP) {
                Key key = sdl_key_to_key(event.key.key);
                input_process_key(platform_context->input_system_state, key, false);
                if (key == KEY_ESC) {
                    quit = true;
                    break;
                }
                app_key_up(platform_context->app, key);
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                Key key = sdl_key_to_key(event.key.key);
                input_process_key(platform_context->input_system_state, key, true);
                app_key_down(platform_context->app, key);
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                app_resize(platform_context->app, event.window.data1, event.window.data2);
            }
            if (quit) { break; }
        } // end polling events

        input_system_update(platform_context->input_system_state);
        app_update(platform_context->app);
    }
}

void platform_terminate(PlatformContext *platform_context) {
    app_destroy(platform_context->app);
    input_system_destroy(platform_context->input_system_state);
    event_system_destroy(platform_context->event_system_state);
    SDL_DestroyWindow(platform_context->window);
}
