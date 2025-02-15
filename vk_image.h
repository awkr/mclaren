#pragma once

#include <volk.h>

struct VkContext;

void vk_create_image(VkContext *vk_context, const VkExtent2D &extent, VkFormat format, VkImageUsageFlags usage, bool enable_mipmap, VkImage *image);

void vk_destroy_image(VkContext *vk_context, VkImage image, VkDeviceMemory device_memory);

void vk_alloc_image_memory(VkContext *vk_context, VkImage image, VkMemoryPropertyFlags memory_property_flags, VkDeviceMemory *memory);

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout);

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask);
