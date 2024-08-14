#pragma once

#include <volk.h>

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module);

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module);
