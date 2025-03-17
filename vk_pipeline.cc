#include "vk_pipeline.h"
#include "logging.h"
#include <fstream>
#include <vector>

void read_file(const char *filepath, bool binary, std::vector<char> *buffer) {
    std::ifstream::openmode mode = std::ios::ate;
    if (binary) { mode |= std::ios::binary; }

    std::ifstream file(filepath, mode);
    ASSERT_MESSAGE(file.is_open(), "failed to open file: %s", filepath);

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
                               const VkPushConstantRange *push_constant_range, VkPipelineLayout *pipeline_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;

    if (push_constant_range) {
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = push_constant_range;
    }

    VkResult result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, pipeline_layout);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout pipeline_layout) {
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
}

void vk_create_graphics_pipeline(VkDevice device, VkPipelineLayout layout, const ColorConfig &color_config, const DepthConfig &depth_config, const DepthBiasConfig &depth_bias_config,
                                 const std::vector<std::pair<VkShaderStageFlagBits, VkShaderModule>> &shader_modules,
                                 const std::vector<VkPrimitiveTopology> &primitive_topologies, VkPolygonMode polygon_mode, VkRenderPass render_pass, VkPipeline *pipeline) {
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
    for (const auto &[shader_stage, shader_module]: shader_modules) {
        VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info{};
        pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_info.stage = shader_stage;
        pipeline_shader_stage_create_info.module = shader_module;
        pipeline_shader_stage_create_info.pName = "main";
        shader_stage_create_infos.push_back(pipeline_shader_stage_create_info);
    }

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state{};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = polygon_mode;
    rasterization_state.lineWidth = 1.0f;
    rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable = depth_bias_config.enable ? VK_TRUE : VK_FALSE;
    if (depth_bias_config.enable) {
      rasterization_state.depthBiasConstantFactor = depth_bias_config.constant_factor;
      rasterization_state.depthBiasSlopeFactor = depth_bias_config.slope_factor;
    }

    VkPipelineMultisampleStateCreateInfo multisample_state{};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = depth_config.enable_test ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthWriteEnable = depth_config.enable_write ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;
    depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depth_stencil_state.front = depth_stencil_state.back;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.blendEnable = color_config.enable_blend ? VK_TRUE : VK_FALSE;
    color_blend_attachment_state.colorWriteMask = color_config.enable_write ? (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
                                                                            : 0;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state{};
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment_state;

    VkPipelineVertexInputStateCreateInfo vertex_input_state{};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = primitive_topologies[0]; // 默认使用第一个拓扑
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    std::vector<VkDynamicState> dynamic_states = {};
    dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);
    if (depth_config.is_depth_test_dynamic) { dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE); }
    if (primitive_topologies.size() > 1) { dynamic_states.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY); }

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = dynamic_states.size();
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = layout;
    pipeline_info.stageCount = shader_stage_create_infos.size();
    pipeline_info.pStages = shader_stage_create_infos.data();
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.renderPass = render_pass;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, pipeline);
    ASSERT(result == VK_SUCCESS);
}

void vk_create_compute_pipeline(VkDevice device, VkPipelineLayout layout, VkShaderModule shader_module, VkPipeline *pipeline) {
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
