#pragma once

#include "vk_defines.h"
#include <unordered_map>
#include <vector>

struct DescriptorPoolSizeRatio {
    VkDescriptorType type;
    uint8_t ratio;
};

// dynamic descriptor allocator
struct DescriptorAllocator {
    std::unordered_map<VkDescriptorType, uint32_t> descriptor_count;
    uint32_t max_sets;

    std::vector<VkDescriptorPool> exhausted_pools;
    std::vector<VkDescriptorPool> pools;

    std::unordered_map<VkDescriptorSet, VkDescriptorPool> descriptor_set_allocation;
};

void vk_descriptor_allocator_create(VkDevice device, uint32_t max_sets,
                                    const std::unordered_map<VkDescriptorType, uint32_t> &descriptor_count,
                                    DescriptorAllocator *allocator);

void vk_descriptor_allocator_destroy(VkDevice device, DescriptorAllocator *allocator);

void vk_descriptor_allocator_reset(VkDevice device, DescriptorAllocator *allocator);

void vk_descriptor_allocator_alloc(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSetLayout layout, VkDescriptorSet *descriptor_set);

void vk_descriptor_allocator_free(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSet descriptor_set);
