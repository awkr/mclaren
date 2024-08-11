#include "vk_image.h"

// void vk_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
//                                 VkImageLayout new_layout) {
//     VkImageMemoryBarrier2 image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
//     image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
//     image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
//     image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
//     image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
//     image_barrier.oldLayout = old_layout;
//     image_barrier.newLayout = new_layout;
//
//     VkImageAspectFlags aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ? VK_IMAGE_ASPECT_DEPTH_BIT
//                                                                                             : VK_IMAGE_ASPECT_COLOR_BIT;
//     VkImageSubresourceRange subresource_range = vk_image_subresource_range(aspect_mask);
//     image_barrier.subresourceRange = subresource_range;
//
//     image_barrier.image = image;
//
//     VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
//     dependency_info.imageMemoryBarrierCount = 1;
//     dependency_info.pImageMemoryBarriers = &image_barrier;
//
//     vkCmdPipelineBarrier2(command_buffer, &dependency_info);
// }

VkImageSubresourceRange vk_image_subresource_range(VkImageAspectFlags aspect_mask) {
    VkImageSubresourceRange subresource_range{};
    subresource_range.aspectMask = aspect_mask;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return subresource_range;
}
