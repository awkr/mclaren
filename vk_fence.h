#pragma once

#include <volk.h>

#define FENCE_TIMEOUT_ONE_SECOND 1000000000

bool vk_create_fence(VkDevice device, bool signaled, VkFence *fence);

void vk_destroy_fence(VkDevice device, VkFence fence);

void vk_wait_fence(VkDevice device, VkFence fence, uint64_t timeout);

void vk_reset_fence(VkDevice device, VkFence fence);
