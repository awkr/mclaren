#pragma once

#include <cstdint>

struct VkContext;

bool vk_create_swapchain(VkContext *vk_context, uint32_t width, uint32_t height);

void vk_destroy_swapchain(VkContext *vk_context);
