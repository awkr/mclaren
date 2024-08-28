#pragma once

#include <volk.h>

void vk_create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask,
                          VkImageView *image_view);

void vk_destroy_image_view(VkDevice device, VkImageView image_view);
