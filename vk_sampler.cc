#include "vk_sampler.h"
#include "core/logging.h"

void vk_create_sampler(VkDevice device, VkFilter mag_filter, VkFilter min_filter, VkSampler *sampler) {
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = mag_filter;
    sampler_create_info.minFilter = min_filter;

    VkResult result = vkCreateSampler(device, &sampler_create_info, nullptr, sampler);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_sampler(VkDevice device, VkSampler sampler) {
    vkDestroySampler(device, sampler, nullptr);
}
