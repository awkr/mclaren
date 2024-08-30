#pragma once

#include <cstdint>
#include <volk.h>

bool vk_create_command_pool(VkDevice device, uint32_t graphics_queue_family_index, VkCommandPool *command_pool);

void vk_destroy_command_pool(VkDevice device, VkCommandPool command_pool);

void vk_reset_command_pool(VkDevice device, VkCommandPool command_pool);
