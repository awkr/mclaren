#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;

void vk_create_image(VkContext *vk_context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                     VkImage *image, VmaAllocation *allocation);

void vk_destroy_image(VkContext *vk_context, VkImage image, VmaAllocation allocation);

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image,
                                VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask,
                                VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask,
                                VkImageLayout old_layout, VkImageLayout new_layout);

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask);
