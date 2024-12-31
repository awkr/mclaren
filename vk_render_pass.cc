#include "vk_render_pass.h"
#include "logging.h"

void vk_create_render_pass(VkDevice device, const AttachmentConfig &color_attachment_config, const AttachmentConfig &depth_attachment_config, VkRenderPass *render_pass) {
  VkAttachmentDescription attachments[2] = {};

  attachments[0].format = color_attachment_config.format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = color_attachment_config.load_op;
  attachments[0].storeOp = color_attachment_config.store_op;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = color_attachment_config.initial_layout;
  attachments[0].finalLayout = color_attachment_config.final_layout;

  attachments[1].format = depth_attachment_config.format;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = depth_attachment_config.load_op;
  attachments[1].storeOp = depth_attachment_config.store_op;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = depth_attachment_config.initial_layout;
  attachments[1].finalLayout = depth_attachment_config.final_layout;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 2;
  render_pass_info.pAttachments = attachments;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;

  VkResult result = vkCreateRenderPass(device, &render_pass_info, nullptr, render_pass);
  ASSERT(result == VK_SUCCESS);

  log_debug("vk render pass created");
}

void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass) {
  vkDestroyRenderPass(device, render_pass, nullptr);
}

void vk_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, const VkExtent2D &extent, const VkClearValue *clear_values, uint32_t clear_value_count) {
  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = render_pass;
  render_pass_begin_info.framebuffer = framebuffer;
  render_pass_begin_info.renderArea.extent = extent;
  render_pass_begin_info.clearValueCount = clear_value_count;
  render_pass_begin_info.pClearValues = clear_values;
  vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_end_render_pass(VkCommandBuffer command_buffer) { vkCmdEndRenderPass(command_buffer); }
