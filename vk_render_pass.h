#pragma once

#include <volk.h>

void vk_create_render_pass(VkDevice device, VkFormat image_format, VkRenderPass *render_pass);
void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass);
void vk_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, const VkOffset2D &offset, const VkExtent2D &extent, const VkClearValue &clear_value);
void vk_end_render_pass(VkCommandBuffer command_buffer);
