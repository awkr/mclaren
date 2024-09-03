#pragma once

#include "vk_defines.h"
#include <vector>

struct DescriptorWriter {
    std::vector<VkDescriptorImageInfo> image_infos;
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
};

// 根据不同的 `type`, `sampler`、`image_view`、`layout` 的入参不同：
// `type` 为 VK_DESCRIPTOR_TYPE_SAMPLER 时，传入 `sampler`
// `type` 为 VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE 时，传入 `image_view` 和 `layout`
// `type` 为 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER 时，传入 `sampler`、 `image_view` 和 `layout`
// `type` 为 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE 时，传入 `image_view` 和 `layout`
void vk_descriptor_writer_write_image(DescriptorWriter *writer, uint32_t binding, VkDescriptorType type,
                                      VkSampler sampler, VkImageView image_view, VkImageLayout layout);

void vk_descriptor_writer_write_buffer(DescriptorWriter *writer, uint32_t binding, VkDescriptorType type,
                                       VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);

void vk_descriptor_writer_clear(DescriptorWriter *writer);

void
vk_descriptor_writer_update_descriptor_set(DescriptorWriter *writer, VkDevice device, VkDescriptorSet descriptor_set);
