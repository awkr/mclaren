#include "vk_framebuffer.h"
#include "logging.h"

void vk_create_framebuffer(VkDevice device, uint32_t width, uint32_t height, VkRenderPass render_pass, const VkImageView *attachments, uint32_t attachment_count, VkFramebuffer *framebuffer) {
  VkFramebufferCreateInfo framebuffer_info = {};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = render_pass;
  framebuffer_info.attachmentCount = attachment_count;
  framebuffer_info.pAttachments = attachments;
  framebuffer_info.width = width;
  framebuffer_info.height = height;
  framebuffer_info.layers = 1;

  VkResult result = vkCreateFramebuffer(device, &framebuffer_info, nullptr, framebuffer);
  ASSERT(result == VK_SUCCESS);
}

void vk_destroy_framebuffer(VkDevice device, VkFramebuffer framebuffer) {
  vkDestroyFramebuffer(device, framebuffer, nullptr);
}
