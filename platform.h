#pragma once

struct SDL_Window;
struct Application;

struct PlatformContext {
    SDL_Window *window;
    Application *app;
};

void platform_init(PlatformContext *platform_context);

void platform_main_loop(PlatformContext *platform_context);

void platform_terminate(PlatformContext *platform_context);
