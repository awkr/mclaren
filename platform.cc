#include "platform.h"
#include "app.h"
#include "core/logging.h"
#include <SDL3/SDL.h>

void create_window(PlatformContext *platform_context, uint16_t width, uint16_t height) {
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    platform_context->window = SDL_CreateWindow("mclaren", width, height, flags);
    int succeed = SDL_RaiseWindow(platform_context->window);
    ASSERT(succeed == 0);
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
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_ESCAPE) {
                    quit = true;
                } else if (event.key.key == SDLK_W) {
                    // app_key_up(platform_context->app, KEY_W);
                    Camera *camera = &platform_context->app->camera;
                    camera_forward(camera, 0.2f);
                } else if (event.key.key == SDLK_S) {
                    // app_key_up(platform_context->app, KEY_S);
                    Camera *camera = &platform_context->app->camera;
                    camera_backward(camera, 0.2f);
                } else if (event.key.key == SDLK_A) {
                    // app_key_up(platform_context->app, KEY_A);
                    Camera *camera = &platform_context->app->camera;
                    camera_left(camera, 0.2f);
                } else if (event.key.key == SDLK_D) {
                    // app_key_up(platform_context->app, KEY_D);
                    Camera *camera = &platform_context->app->camera;
                    camera_right(camera, 0.2f);
                } else if (event.key.key == SDLK_Q) {
                    // app_key_up(platform_context->app, KEY_Q);
                    Camera *camera = &platform_context->app->camera;
                    camera_up(camera, 0.2f);
                } else if (event.key.key == SDLK_E) {
                    // app_key_up(platform_context->app, KEY_E);
                    Camera *camera = &platform_context->app->camera;
                    camera_down(camera, 0.2f);
                } else if (event.key.key == SDLK_UP) {
                    Camera *camera = &platform_context->app->camera;
                    camera_pitch(camera, 2.0f);
                } else if (event.key.key == SDLK_DOWN) {
                    Camera *camera = &platform_context->app->camera;
                    camera_pitch(camera, -2.0f);
                } else if (event.key.key == SDLK_LEFT) {
                    Camera *camera = &platform_context->app->camera;
                    camera_yaw(camera, 2.0f);
                } else if (event.key.key == SDLK_RIGHT) {
                    Camera *camera = &platform_context->app->camera;
                    camera_yaw(camera, -2.0f);
                }
            }
            if (quit) { break; }
        }

        app_update(platform_context->app);
    }
}

void platform_terminate(PlatformContext *platform_context) {
    app_destroy(platform_context->app);
    SDL_DestroyWindow(platform_context->window);
}
