#include "vk_sampler.h"
#include "logging.h"

void vk_create_sampler(VkDevice device, VkFilter mag_filter, VkFilter min_filter, VkSampler *sampler) {
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = mag_filter;
    sampler_create_info.minFilter = min_filter;
    sampler_create_info.minLod = 0.0f; // 不限制最低 LOD
    sampler_create_info.maxLod = VK_LOD_CLAMP_NONE; // 不限制最高 LOD

    VkResult result = vkCreateSampler(device, &sampler_create_info, nullptr, sampler);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_sampler(VkDevice device, VkSampler sampler) {
    vkDestroySampler(device, sampler, nullptr);
}
