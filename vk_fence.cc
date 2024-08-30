#include "vk_fence.h"
#include "core/logging.h"

bool vk_create_fence(VkDevice device, bool signaled, VkFence *fence) {
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VkResult result = vkCreateFence(device, &fence_create_info, nullptr, fence);
    return result == VK_SUCCESS;
}

void vk_destroy_fence(VkDevice device, VkFence fence) { vkDestroyFence(device, fence, nullptr); }

void vk_wait_fence(VkDevice device, VkFence fence) {
    VkResult result = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    ASSERT(result == VK_SUCCESS);
}

void vk_reset_fence(VkDevice device, VkFence fence) {
    VkResult result = vkResetFences(device, 1, &fence);
    ASSERT(result == VK_SUCCESS);
}
