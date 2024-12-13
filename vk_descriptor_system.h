#pragma once

#include "vk_defines.h"
#include <vector>

struct DescriptorPoolSizeRatio {
    VkDescriptorType type;
    uint8_t ratio;
};

// dynamic descriptor allocator
struct DescriptorAllocator {
    std::vector<DescriptorPoolSizeRatio> size_ratios;
    uint32_t sets_per_pool;

    std::vector<VkDescriptorPool> full_pools;
    std::vector<VkDescriptorPool> available_pools;
};

void vk_descriptor_allocator_create(VkDevice device, uint32_t sets_per_pool,
                                    const std::vector<DescriptorPoolSizeRatio> &size_ratios,
                                    DescriptorAllocator **out_allocator);

void vk_descriptor_allocator_destroy(VkDevice device, DescriptorAllocator *allocator);

void vk_descriptor_allocator_reset(VkDevice device, DescriptorAllocator *allocator);

void vk_descriptor_allocator_alloc(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSetLayout layout, VkDescriptorSet *descriptor_set);

void vk_descriptor_allocator_free(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSetLayout layout, VkDescriptorSet *descriptor_set);
