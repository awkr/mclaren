#pragma once

struct SDL_Window;
struct App;

struct PlatformContext {
    SDL_Window *window;
    App *app;
};

void platform_init(PlatformContext *platform_context);

void platform_main_loop(PlatformContext *platform_context);

void platform_terminate(PlatformContext *platform_context);
