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
