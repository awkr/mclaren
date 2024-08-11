#pragma once

struct VkContext;
struct SDL_Window;

struct Application {
    VkContext *vk_context;
};

void app_create(SDL_Window *window, Application **app);

void app_destroy(Application *app);

void app_update(Application *app);
