#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;

struct Image {
    VkImage image;
    VmaAllocation allocation;
    uint16_t mip_levels;
};

void vk_create_image(VkContext *vk_context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, bool enable_mipmap, Image **out_image);

void vk_create_image_from_data(VkContext *vk_context, VkCommandBuffer command_buffer, const void *data, uint32_t width, uint32_t height,
                               VkFormat format, VkImageUsageFlags usage, bool enable_mipmap, Image **out_image);

void vk_destroy_image(VkContext *vk_context, Image *image);

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout);

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask);
