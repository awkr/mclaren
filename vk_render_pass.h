#pragma once

#include <volk.h>

void vk_create_render_pass(VkDevice device, VkFormat image_format, VkRenderPass *render_pass);
void vk_destroy_render_pass(VkDevice device, VkRenderPass render_pass);
