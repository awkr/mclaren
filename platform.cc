#include "platform.h"
#include "app.h"
#include "logging.h"
#include <SDL3/SDL.h>

void create_window(PlatformContext *platform_context, uint16_t width, uint16_t height) {
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    platform_context->window = SDL_CreateWindow("mclaren", width, height, flags);
}

void platform_init(PlatformContext *platform_context) {
    int ok = SDL_Init(SDL_INIT_VIDEO);
    ASSERT(ok == 0);
    create_window(platform_context, 320, 240);
    app_create(platform_context->window, &platform_context->app);
}

void platform_main_loop(PlatformContext *platform_context) {
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        app_update(platform_context->app);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_ESCAPE) {
                    quit = true;
                }
            }
            if (quit) { break; }
        }
    }
}

void platform_terminate(PlatformContext *platform_context) {
    app_destroy(platform_context->app);
    SDL_DestroyWindow(platform_context->window);
}
