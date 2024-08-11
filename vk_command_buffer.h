#pragma once

#include <volk.h>

bool vk_reset_command_buffer(VkCommandBuffer command_buffer);

bool vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags);

bool vk_end_command_buffer(VkCommandBuffer command_buffer);

VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer);

VkSubmitInfo2 vk_submit_info(VkCommandBufferSubmitInfo *command_buffer, VkSemaphoreSubmitInfo *wait_semaphore,
                             VkSemaphoreSubmitInfo *signal_semaphore);

void vk_command_clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_layout,
                                  VkClearColorValue *clear_color);
