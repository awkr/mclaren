#pragma once

#include <volk.h>

void vk_create_render_pass(VkDevice device, VkFormat color_image_format, VkFormat depth_image_format, VkRenderPass *render_pass);
void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass);
void vk_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, const VkExtent2D &extent, const VkClearValue *clear_values, uint32_t clear_value_count);
void vk_end_render_pass(VkCommandBuffer command_buffer);
