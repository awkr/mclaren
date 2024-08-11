#pragma once

#include <cstdint>
#include <volk.h>

struct VkContext;
struct SDL_Window;

void vk_init(VkContext *vk_context, SDL_Window *window, uint32_t width, uint32_t height);

void vk_terminate(VkContext *vk_context);

void vk_acquire_next_image(VkContext *vk_context, VkSemaphore signal_semaphore, uint32_t *image_index);

void vk_present(VkContext *vk_context, uint32_t image_index, VkSemaphore wait_semaphore);
