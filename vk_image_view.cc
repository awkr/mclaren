#include "vk_image_view.h"
#include "core/logging.h"

void vk_create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask,
                          VkImageView *image_view) {
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.image = image;
    image_view_create_info.format = format;
    image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    VkResult result = vkCreateImageView(device, &image_view_create_info, nullptr, image_view);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_image_view(VkDevice device, VkImageView image_view) {
    vkDestroyImageView(device, image_view, nullptr);
}
