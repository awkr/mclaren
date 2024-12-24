#include "vk_descriptor_allocator.h"
#include "core/logging.h"
#include "vk_descriptor.h"

static VkDescriptorPool create_descriptor_pool(VkDevice device, DescriptorAllocator *allocator) {
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (const auto &[descriptor_type, descriptor_count]: allocator->descriptor_count) {
        VkDescriptorPoolSize pool_size = {};
        pool_size.type = descriptor_type;
        pool_size.descriptorCount = descriptor_count;
        pool_sizes.push_back(pool_size);
    }

    VkDescriptorPool pool = VK_NULL_HANDLE;
    vk_create_descriptor_pool(device, allocator->max_sets, pool_sizes, &pool);
    return pool;
}

static VkDescriptorPool pop_or_create_descriptor_pool(VkDevice device, DescriptorAllocator *allocator) {
    if (!allocator->pools.empty()) {
        VkDescriptorPool pool = allocator->pools.back();
        allocator->pools.pop_back();
        return pool;
    }
    VkDescriptorPool pool = create_descriptor_pool(device, allocator);
    return pool;
}

void vk_descriptor_allocator_create(VkDevice device, uint32_t max_sets,
                                    const std::unordered_map<VkDescriptorType, uint32_t> &descriptor_count,
                                    DescriptorAllocator *allocator) {
    allocator->descriptor_count = descriptor_count;
    allocator->max_sets = max_sets;

    VkDescriptorPool pool = create_descriptor_pool(device, allocator); // create an initial pool
    allocator->pools.push_back(pool);
}

void vk_descriptor_allocator_destroy(VkDevice device, DescriptorAllocator *allocator) {
    for (VkDescriptorPool pool: allocator->pools) { vk_destroy_descriptor_pool(device, pool); }
    allocator->pools.clear();
    for (VkDescriptorPool pool: allocator->exhausted_pools) { vk_destroy_descriptor_pool(device, pool); }
    allocator->exhausted_pools.clear();
}

void vk_descriptor_allocator_reset(VkDevice device, DescriptorAllocator *allocator) {
    for (VkDescriptorPool pool: allocator->pools) { vk_reset_descriptor_pool(device, pool); }
    for (VkDescriptorPool pool: allocator->exhausted_pools) {
        vk_reset_descriptor_pool(device, pool);
        allocator->pools.push_back(pool);
    }
    allocator->exhausted_pools.clear();
}

void vk_descriptor_allocator_alloc(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSetLayout layout, VkDescriptorSet *descriptor_set) {
    VkDescriptorPool pool = pop_or_create_descriptor_pool(device, allocator);

    VkResult result = vk_allocate_descriptor_set(device, pool, layout, descriptor_set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        allocator->exhausted_pools.push_back(pool);

        pool = create_descriptor_pool(device, allocator);
        result = vk_allocate_descriptor_set(device, pool, layout, descriptor_set);
    }
    ASSERT(result == VK_SUCCESS);

    allocator->descriptor_set_allocation[*descriptor_set] = pool;
    allocator->pools.push_back(pool);
}

void vk_descriptor_allocator_free(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSet descriptor_set) {
    VkDescriptorPool pool = allocator->descriptor_set_allocation.at(descriptor_set);
    vk_free_descriptor_set(device, pool, descriptor_set);
}
