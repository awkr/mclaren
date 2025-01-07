#pragma once

#include <volk.h>

struct AttachmentConfig {
  VkFormat format;
  VkAttachmentLoadOp load_op;
  VkAttachmentStoreOp store_op;
  VkImageLayout initial_layout;
  VkImageLayout final_layout;
};

void vk_create_render_pass(VkDevice device, const AttachmentConfig *color_attachment_config, const AttachmentConfig *depth_attachment_config, VkRenderPass *render_pass);
void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass);
void vk_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, const VkExtent2D &extent, const VkClearValue *clear_values, uint32_t clear_value_count);
void vk_end_render_pass(VkCommandBuffer command_buffer);
