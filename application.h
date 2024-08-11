#pragma once

#include <cstdint>

struct VkContext;
struct SDL_Window;

struct Application {
    VkContext *vk_context;
    uint64_t frame_number;
    uint32_t frame_index;
};

void app_create(SDL_Window *window, Application **app);

void app_destroy(Application *app);

void app_update(Application *app);
