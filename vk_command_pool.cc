#include "vk_command_pool.h"
#include <volk.h>

bool vk_create_command_pool(VkDevice device, uint32_t graphics_queue_family_index, VkCommandPool *command_pool) {
    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = graphics_queue_family_index;
    VkResult result = vkCreateCommandPool(device, &command_pool_info, nullptr, command_pool);
    return result == VK_SUCCESS;
}

void vk_destroy_command_pool(VkDevice device, VkCommandPool command_pool) {
    vkDestroyCommandPool(device, command_pool, nullptr);
}

bool vk_allocate_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                                 VkCommandBuffer *command_buffers) {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;
    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, command_buffers);
    return result == VK_SUCCESS;
}

void vk_free_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                             VkCommandBuffer *command_buffers) {
    vkFreeCommandBuffers(device, command_pool, count, command_buffers);
}
