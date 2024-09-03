#include "vk_descriptor_allocator.h"
#include "vk_descriptor.h"
#include "core/logging.h"

static VkDescriptorPool create_pool(VkDevice device, DescriptorAllocator *allocator) {
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (const DescriptorPoolSizeRatio &ratio: allocator->size_ratios) {
        VkDescriptorPoolSize pool_size = {};
        pool_size.type = ratio.type;
        pool_size.descriptorCount = ratio.ratio * allocator->sets_per_pool;
        pool_sizes.push_back(pool_size);
    }

    VkDescriptorPool pool;
    vk_create_descriptor_pool(device, allocator->sets_per_pool, pool_sizes, &pool);
    return pool;
}

static VkDescriptorPool pop_or_create_pool(VkDevice device, DescriptorAllocator *allocator) {
    if (!allocator->available_pools.empty()) {
        VkDescriptorPool pool = allocator->available_pools.back();
        allocator->available_pools.pop_back();
        return pool;
    }
    VkDescriptorPool pool = create_pool(device, allocator);
    return pool;
}

void vk_descriptor_allocator_create(VkDevice device, uint32_t sets_per_pool,
                                    const std::vector<DescriptorPoolSizeRatio> &size_ratios,
                                    DescriptorAllocator **out_allocator) {
    DescriptorAllocator *allocator = new DescriptorAllocator();
    allocator->size_ratios = size_ratios;
    allocator->sets_per_pool = sets_per_pool;
    *out_allocator = allocator;

    // create initial pool
    VkDescriptorPool pool = create_pool(device, allocator);
    allocator->available_pools.push_back(pool);
}

void vk_descriptor_allocator_destroy(VkDevice device, DescriptorAllocator *allocator) {
    for (VkDescriptorPool pool: allocator->available_pools) { vk_destroy_descriptor_pool(device, pool); }
    allocator->available_pools.clear();
    for (VkDescriptorPool pool: allocator->full_pools) { vk_destroy_descriptor_pool(device, pool); }
    allocator->full_pools.clear();
    delete allocator;
}

void vk_descriptor_allocator_reset(VkDevice device, DescriptorAllocator *allocator) {
    for (VkDescriptorPool pool: allocator->available_pools) { vk_reset_descriptor_pool(device, pool); }
    for (VkDescriptorPool pool: allocator->full_pools) {
        vk_reset_descriptor_pool(device, pool);
        allocator->available_pools.push_back(pool);
    }
    allocator->full_pools.clear();
}

void
vk_descriptor_allocator_alloc(VkDevice device, DescriptorAllocator *allocator, VkDescriptorSetLayout layout,
                              VkDescriptorSet *descriptor_set) {
    VkDescriptorPool pool = pop_or_create_pool(device, allocator);

    VkResult result = vk_allocate_descriptor_set(device, pool, layout, descriptor_set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        allocator->full_pools.push_back(pool);

        pool = create_pool(device, allocator);
        result = vk_allocate_descriptor_set(device, pool, layout, descriptor_set);
    }
    ASSERT(result == VK_SUCCESS);

    allocator->available_pools.push_back(pool);
}
