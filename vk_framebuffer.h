#pragma once

#include <volk.h>

void vk_create_framebuffer(VkDevice device, uint32_t width, uint32_t height, VkRenderPass render_pass, const VkImageView *attachments, uint32_t attachment_count, VkFramebuffer *framebuffer);
void vk_destroy_framebuffer(VkDevice device, VkFramebuffer framebuffer);
