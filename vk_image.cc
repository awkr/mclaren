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
    image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

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
    VkImageMemoryBarrier2 image_barrier{};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL) {
        image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        image_barrier.srcAccessMask = 0;
        image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        image_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        image_barrier.dstAccessMask = 0;
    } else {
        ASSERT_MESSAGE(false, "unsupported image layout transition");
    }

    image_barrier.oldLayout = old_layout;
    image_barrier.newLayout = new_layout;

    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = image;

    VkImageAspectFlags aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                                                            : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange = vk_image_subresource_range(aspect_mask);


    VkDependencyInfo dependency_info{};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void vk_copy_image_to_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *src_extent,
                            const VkExtent2D *dst_extent) {
    VkImageBlit2 blit_region{};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcOffsets[1].x = src_extent->width;
    blit_region.srcOffsets[1].y = src_extent->height;
    blit_region.srcOffsets[1].z = 1;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstOffsets[1].x = dst_extent->width;
    blit_region.dstOffsets[1].y = dst_extent->height;
    blit_region.dstOffsets[1].z = 1;

    VkBlitImageInfo2 blit_image_info{};
    blit_image_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blit_image_info.srcImage = src;
    blit_image_info.dstImage = dst;
    blit_image_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blit_image_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_image_info.filter = VK_FILTER_NEAREST;
    blit_image_info.regionCount = 1;
    blit_image_info.pRegions = &blit_region;

    vkCmdBlitImage2(command_buffer, &blit_image_info);
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
