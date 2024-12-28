#pragma once

#include <vector>
#include <volk.h>

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module);

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module);

void vk_create_pipeline_layout(VkDevice device, uint32_t descriptor_set_layout_count, const VkDescriptorSetLayout *descriptor_set_layouts,
                               const VkPushConstantRange *push_constant, VkPipelineLayout *pipeline_layout);

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout);

struct DepthBiasConfig {
  bool enable;
  float constant_factor;
  float slope_factor;
};

struct DepthConfig {
  bool enable_test;
  bool enable_write;
  bool is_depth_test_dynamic;
};

void vk_create_graphics_pipeline(VkDevice device, VkPipelineLayout layout, VkFormat color_attachment_format, bool enable_blend, const DepthConfig &depth_config, const DepthBiasConfig &depth_bias_config, VkFormat depth_attachment_format,
                                 const std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>> &shader_modules, const std::vector<VkPrimitiveTopology> &primitive_topologies, VkPolygonMode polygon_mode, VkPipeline *pipeline);

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout layout, VkShaderModule shader_module, VkPipeline *pipeline);

void vk_destroy_pipeline(VkDevice device, VkPipeline pipeline);
