#include "vk_pipeline.h"
#include "core/logging.h"
#include <fstream>
#include <vector>

void read_file(const char *filepath, bool binary, std::vector<char> *buffer) {
    std::ifstream::openmode mode = std::ios::ate;
    if (binary) { mode |= std::ios::binary; }

    std::ifstream file(filepath, mode);
    ASSERT(file.is_open());

    long file_size = file.tellg();
    buffer->resize(file_size);

    file.seekg(0);
    file.read(buffer->data(), file_size);
    file.close();
}

void vk_create_shader_module(VkDevice device, const char *filepath, VkShaderModule *shader_module) {
    std::vector<char> buffer;
    read_file(filepath, true, &buffer);

    VkShaderModuleCreateInfo shader_module_create_info{};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    shader_module_create_info.codeSize = buffer.size() * sizeof(char);
    shader_module_create_info.pCode = (uint32_t *) buffer.data();

    VkResult result = vkCreateShaderModule(device, &shader_module_create_info, nullptr, shader_module);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module) {
    vkDestroyShaderModule(device, shader_module, nullptr);
}

void vk_create_pipeline_layout(VkDevice device, uint32_t descriptor_set_layout_count, const VkDescriptorSetLayout *descriptor_set_layouts,
                               const VkPushConstantRange *push_constant, VkPipelineLayout *pipeline_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;

    if (push_constant) {
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = push_constant;
    }

    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, pipeline_layout);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout) {
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
}

void vk_create_graphics_pipeline(VkDevice device, VkPipelineLayout layout, VkFormat color_attachment_format,
                                 VkFormat depth_attachment_format,
                                 const std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>> &shader_modules,
                                 VkPipeline *pipeline) {
    VkPipelineRenderingCreateInfo rendering_create_info{};
    rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_create_info.colorAttachmentCount = 1;
    rendering_create_info.pColorAttachmentFormats = &color_attachment_format;
    rendering_create_info.depthAttachmentFormat = depth_attachment_format;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
    for (const auto &[shader_stage, shader_module]: shader_modules) {
        VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info{};
        pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_info.stage = shader_stage;
        pipeline_shader_stage_create_info.module = shader_module;
        pipeline_shader_stage_create_info.pName = "main";
        shader_stage_create_infos.push_back(pipeline_shader_stage_create_info);
    }

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = &rendering_create_info;
    pipeline_create_info.layout = layout;
    pipeline_create_info.stageCount = shader_stage_create_infos.size();
    pipeline_create_info.pStages = shader_stage_create_infos.data();
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, pipeline);
    ASSERT(result == VK_SUCCESS);
}

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout layout, VkShaderModule shader_module,
                                VkPipeline *pipeline) {
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info{};
    pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_shader_stage_create_info.module = shader_module;
    pipeline_shader_stage_create_info.pName = "main";

    VkComputePipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = pipeline_shader_stage_create_info;
    pipeline_create_info.layout = layout;

    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, pipeline);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_pipeline(VkDevice device, VkPipeline pipeline) { vkDestroyPipeline(device, pipeline, nullptr); }
