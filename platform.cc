#include "platform.h"
#include "app.h"
#include "core/logging.h"
#include "event_system.h"
#include "input_system.h"
#include <SDL3/SDL.h>
#include <thread>

void create_window(PlatformContext *platform_context, uint16_t width, uint16_t height) {
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
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
                if (event.key.key == SDLK_ESCAPE) {
                    quit = true;
                    break;
                }
                if (event.key.key == SDLK_W) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_S) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_A) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_D) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_Q) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_E) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_UP) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_DOWN) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_LEFT) {
                    app_key_up(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_RIGHT) {
                    app_key_up(platform_context->app, event.key.key);
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_W) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_S) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_A) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_D) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_Q) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_E) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_UP) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_DOWN) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_LEFT) {
                    app_key_down(platform_context->app, event.key.key);
                }
                if (event.key.key == SDLK_RIGHT) {
                    app_key_down(platform_context->app, event.key.key);
                }
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                app_resize(platform_context->app, event.window.data1, event.window.data2);
            }
            if (quit) { break; }
        } // end polling events

        app_update(platform_context->app);
    }
}

void platform_terminate(PlatformContext *platform_context) {
    app_destroy(platform_context->app);
    input_system_destroy(platform_context->input_system_state);
    event_system_destroy(platform_context->event_system_state);
    SDL_DestroyWindow(platform_context->window);
}
