#pragma once

#include <cstdint>
#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;
struct SDL_Window;

struct Application {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_number;
    uint32_t frame_index;
    VkImage drawable_image;
    VmaAllocation drawable_image_allocation;
    VkImageView drawable_image_view;
};

void app_create(SDL_Window *window, Application **app);

void app_destroy(Application *app);

void app_update(Application *app);
