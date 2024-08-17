#include "vk_image.h"
#include "vk_context.h"
#include "core/logging.h"

void vk_create_image(VkContext *vk_context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                     VkImage *image, VmaAllocation *allocation) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocation_create_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkResult result = vmaCreateImage(vk_context->allocator, &image_create_info, &allocation_create_info, image,
                                     allocation, nullptr);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_image(VkContext *vk_context, VkImage image, VmaAllocation allocation) {
    vmaDestroyImage(vk_context->allocator, image, allocation);
}

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image,
                                VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask,
                                VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask,
                                VkImageLayout old_layout, VkImageLayout new_layout) {
    ASSERT(old_layout != new_layout);

    VkImageMemoryBarrier2 image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    image_memory_barrier.srcStageMask = src_stage_mask;
    image_memory_barrier.dstStageMask = dst_stage_mask;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;

    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;

    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = image;

    VkImageAspectFlags aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                                                            : VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange = vk_image_subresource_range(aspect_mask);

    VkDependencyInfo dependency_info{};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;

    vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);
}

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask) {
    VkImageSubresourceRange subresource_range{};
    subresource_range.aspectMask = aspect_mask;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return subresource_range;
}
