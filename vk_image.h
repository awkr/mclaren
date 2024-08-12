#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;

void vk_create_image(VkContext *vk_context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                     VkImage *image, VmaAllocation *allocation);

void vk_destroy_image(VkContext *vk_context, VkImage image, VmaAllocation allocation);

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
                                VkImageLayout new_layout);

void vk_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent);

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask);
