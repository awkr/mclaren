#pragma once

struct SDL_Window;
struct EventSystemState;
struct InputSystemState;
struct App;

struct PlatformContext {
    SDL_Window *window;
    EventSystemState *event_system_state;
    InputSystemState *input_system_state;
    App *app;
};

void platform_init(PlatformContext *platform_context);

void platform_main_loop(PlatformContext *platform_context);

void platform_terminate(PlatformContext *platform_context);
