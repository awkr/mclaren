#include "vk_render_pass.h"
#include "logging.h"
#include <vector>

void vk_create_render_pass(VkDevice device, const char *name, const AttachmentConfig *color_attachment_config, const AttachmentConfig *depth_attachment_config, VkRenderPass *render_pass) {
  std::vector<VkAttachmentDescription> attachments{};

  VkAttachmentReference color_attachment_ref{};
  VkAttachmentReference depth_attachment_ref{};

  if (color_attachment_config) {
    VkAttachmentDescription attachment_description{};
    attachment_description.format = color_attachment_config->format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = color_attachment_config->load_op;
    attachment_description.storeOp = color_attachment_config->store_op;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = color_attachment_config->initial_layout;
    attachment_description.finalLayout = color_attachment_config->final_layout;
    attachments.push_back(attachment_description);

    color_attachment_ref.attachment = attachments.size() - 1;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }

  if (depth_attachment_config) {
    VkAttachmentDescription attachment_description{};
    attachment_description.format = depth_attachment_config->format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = depth_attachment_config->load_op;
    attachment_description.storeOp = depth_attachment_config->store_op;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = depth_attachment_config->initial_layout;
    attachment_description.finalLayout = depth_attachment_config->final_layout;
    attachments.push_back(attachment_description);

    depth_attachment_ref.attachment = attachments.size() - 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = color_attachment_config ? 1 : 0;
  subpass.pColorAttachments = color_attachment_config ? &color_attachment_ref : nullptr;
  subpass.pDepthStencilAttachment = depth_attachment_config ? &depth_attachment_ref : nullptr;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = attachments.size();
  render_pass_info.pAttachments = attachments.data();
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;

  VkResult result = vkCreateRenderPass(device, &render_pass_info, nullptr, render_pass);
  ASSERT(result == VK_SUCCESS);

  log_debug("vk render pass created '%s' %p", name, *render_pass);
}

void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass) {
  vkDestroyRenderPass(device, render_pass, nullptr);
  log_debug("vk render pass destroyed %p", render_pass);
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
