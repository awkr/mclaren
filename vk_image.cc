#include "vk_image.h"
#include "vk_context.h"
#include "logging.h"

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

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
                                VkImageLayout new_layout) {
    VkImageMemoryBarrier2 image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        image_memory_barrier.dstAccessMask = 0;
    } else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        image_memory_barrier.dstAccessMask = 0;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        image_memory_barrier.dstAccessMask = 0;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_GENERAL) {
        image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else {
        ASSERT_MESSAGE(false, "unsupported image layout transition");
    }

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

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void vk_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent) {
    VkImageBlit image_blit_region{};
    image_blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.srcSubresource.mipLevel = 0;
    image_blit_region.srcSubresource.baseArrayLayer = 0;
    image_blit_region.srcSubresource.layerCount = 1;
    image_blit_region.srcOffsets[1].x = (int32_t) extent->width;
    image_blit_region.srcOffsets[1].y = (int32_t) extent->height;
    image_blit_region.srcOffsets[1].z = 1;
    image_blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.dstSubresource.mipLevel = 0;
    image_blit_region.dstSubresource.baseArrayLayer = 0;
    image_blit_region.dstSubresource.layerCount = 1;
    image_blit_region.dstOffsets[1].x = (int32_t) extent->width;
    image_blit_region.dstOffsets[1].y = (int32_t) extent->height;
    image_blit_region.dstOffsets[1].z = 1;

    vkCmdBlitImage(command_buffer,
                   src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &image_blit_region,
                   VK_FILTER_LINEAR);
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
