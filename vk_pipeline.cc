#include "vk_pipeline.h"
#include "logging.h"
#include <fstream>
#include <vector>

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    ASSERT(file.is_open());

    long file_size = file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    shader_module_create_info.codeSize = buffer.size() * sizeof(char);
    shader_module_create_info.pCode = (uint32_t *) buffer.data();

    VkResult result = vkCreateShaderModule(device, &shader_module_create_info, nullptr, shader_module);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module) {
    vkDestroyShaderModule(device, shader_module, nullptr);
}

void vk_create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout,
                               VkPipelineLayout *pipeline_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, pipeline_layout);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout) {
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
}

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkShaderModule compute_shader_module,
                                VkPipeline *pipeline) {
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = {};
    pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_shader_stage_create_info.module = compute_shader_module;
    pipeline_shader_stage_create_info.pName = "main";

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = pipeline_shader_stage_create_info;
    pipeline_create_info.layout = pipeline_layout;

    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, pipeline);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_pipeline(VkDevice device, VkPipeline pipeline) { vkDestroyPipeline(device, pipeline, nullptr); }
