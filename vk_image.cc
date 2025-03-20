#include "vk_image.h"
#include "logging.h"
#include "vk_buffer.h"
#include "vk_command_buffer.h"
#include "vk_context.h"
#include "vk_fence.h"
#include "vk_queue.h"

void vk_create_image(VkContext *vk_context, const VkExtent2D &extent, VkFormat format, VkImageUsageFlags usage, bool enable_mipmap, VkImage *image) {
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = extent.width;
    image_create_info.extent.height = extent.height;
    image_create_info.extent.depth = 1;
    if (enable_mipmap) {
        image_create_info.mipLevels = (uint32_t) std::floor(std::log2(std::max(extent.width, extent.height))) + 1;
    } else {
        image_create_info.mipLevels = 1;
    }
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = usage;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(vk_context->device, &image_create_info, nullptr, image);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_image(VkContext *vk_context, VkImage image, VkDeviceMemory device_memory) {
    vkDestroyImage(vk_context->device, image, nullptr);
    if (device_memory) {
        vkFreeMemory(vk_context->device, device_memory, nullptr);
    }
}

void vk_alloc_image_memory(VkContext *vk_context, VkImage image, VkMemoryPropertyFlags memory_property_flags, VkDeviceMemory *memory) {
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vk_context->device, image, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_context->physical_device, &memory_properties);

    uint32_t memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((memory_requirements.memoryTypeBits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags)) {
          memory_type_index = i;
          break;
        }
    }
    ASSERT(memory_type_index != UINT32_MAX);

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;

    VkResult result = vkAllocateMemory(vk_context->device, &memory_allocate_info, nullptr, memory);
    ASSERT(result == VK_SUCCESS);

    result = vkBindImageMemory(vk_context->device, image, *memory, 0);
    ASSERT(result == VK_SUCCESS);
}

void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout) {
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

    const VkImageAspectFlags aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
