#pragma once

#include <volk.h>

// void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
//                                 VkImageLayout new_layout);

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask);
