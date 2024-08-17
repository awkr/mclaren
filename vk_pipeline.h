#pragma once

#include <vector>
#include <volk.h>

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module);

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module);

void vk_create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout,
                               const VkPushConstantRange *push_constant, VkPipelineLayout *pipeline_layout);

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout);

void vk_create_graphics_pipeline(VkDevice device, VkPipelineLayout layout, VkFormat color_attachment_format,
                                 VkFormat depth_attachment_format,
                                 const std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>> &shader_modules,
                                 VkPipeline *pipeline);

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout layout, VkShaderModule shader_module,
                                VkPipeline *pipeline);

void vk_destroy_pipeline(VkDevice device, VkPipeline pipeline);
