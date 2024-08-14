#pragma once

#include <volk.h>

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module);

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module);

void vk_create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout,
                               VkPipelineLayout *pipeline_layout);

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout);

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkShaderModule compute_shader_module,
                                VkPipeline *pipeline);

void vk_create_graphics_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkRenderPass render_pass,
                                 VkShaderModule vertex_shader_module, VkShaderModule fragment_shader_module,
                                 VkPipeline *pipeline);

void vk_destroy_pipeline(VkDevice device, VkPipeline pipeline);
