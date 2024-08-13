#include "vk_command_buffer.h"
#include "vk_image.h"

bool vk_reset_command_buffer(VkCommandBuffer command_buffer) {
    VkResult result = vkResetCommandBuffer(command_buffer, 0);
    return result == VK_SUCCESS;
}

bool vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = flags;
    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    return result == VK_SUCCESS;
}

bool vk_end_command_buffer(VkCommandBuffer command_buffer) {
    VkResult result = vkEndCommandBuffer(command_buffer);
    return result == VK_SUCCESS;
}

VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer) {
    VkCommandBufferSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    submit_info.commandBuffer = command_buffer;
    submit_info.deviceMask = 0;
    return submit_info;
}

VkSubmitInfo2 vk_submit_info(VkCommandBufferSubmitInfo *command_buffer, VkSemaphoreSubmitInfo *wait_semaphore,
                             VkSemaphoreSubmitInfo *signal_semaphore) {
    VkSubmitInfo2 submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = command_buffer;
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = wait_semaphore;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = signal_semaphore;
    return submit_info;
}

void vk_command_clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_layout,
                                  VkClearColorValue *clear_color) {
    VkImageSubresourceRange clear_range = vk_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(command_buffer, image, image_layout, clear_color, 1, &clear_range);
}

void vk_command_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent) {
    VkImageBlit blit_region{};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcOffsets[1].x = (int32_t) extent->width;
    blit_region.srcOffsets[1].y = (int32_t) extent->height;
    blit_region.srcOffsets[1].z = 1;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstOffsets[1].x = (int32_t) extent->width;
    blit_region.dstOffsets[1].y = (int32_t) extent->height;
    blit_region.dstOffsets[1].z = 1;

    vkCmdBlitImage(command_buffer,
                   src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit_region,
                   VK_FILTER_NEAREST);
}
