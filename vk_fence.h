#pragma once

#include <volk.h>

bool vk_create_fence(VkDevice device, bool signaled, VkFence *fence);

void vk_destroy_fence(VkDevice device, VkFence fence);

void vk_wait_fence(VkDevice device, VkFence fence);

void vk_reset_fence(VkDevice device, VkFence fence);
