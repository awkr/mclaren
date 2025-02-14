#include "vk_descriptor.h"
#include "logging.h"

void vk_create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings, const void *next, VkDescriptorSetLayout *descriptor_set_layout) {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = next;
    descriptor_set_layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    descriptor_set_layout_create_info.pBindings = bindings.data();
    descriptor_set_layout_create_info.bindingCount = bindings.size();
    VkResult result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, descriptor_set_layout);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout) {
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
}

void vk_create_descriptor_pool(VkDevice device, uint32_t max_sets, const std::vector<VkDescriptorPoolSize> &pool_sizes,
                               VkDescriptorPool *descriptor_pool) {
    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    create_info.maxSets = max_sets;
    create_info.poolSizeCount = pool_sizes.size();
    create_info.pPoolSizes = pool_sizes.data();
    VkResult result = vkCreateDescriptorPool(device, &create_info, nullptr, descriptor_pool);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_descriptor_pool(VkDevice device, VkDescriptorPool descriptor_pool) {
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
}

void vk_reset_descriptor_pool(VkDevice device, VkDescriptorPool descriptor_pool) {
    VkResult result = vkResetDescriptorPool(device, descriptor_pool, 0);
    ASSERT(result == VK_SUCCESS);
}

VkResult vk_allocate_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout layout,
                                    VkDescriptorSet *descriptor_set) {
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    return vkAllocateDescriptorSets(device, &allocate_info, descriptor_set);
}

VkResult vk_allocate_descriptor_sets(VkDevice device, VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout *layouts, const void *next, uint32_t descriptor_set_count, VkDescriptorSet *descriptor_sets) {
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = next;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = descriptor_set_count;
    allocate_info.pSetLayouts = layouts;
    return vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets);
}

void vk_free_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set) {
    VkResult result = vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
    ASSERT(result == VK_SUCCESS);
}

void vk_update_descriptor_sets(VkDevice device, uint32_t count, const VkWriteDescriptorSet *writes) {
    vkUpdateDescriptorSets(device, count, writes, 0, nullptr);
}

VkDescriptorBufferInfo vk_descriptor_buffer_info(VkBuffer buffer, uint64_t size) {
    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = size;
    return descriptor_buffer_info;
}

VkDescriptorImageInfo vk_descriptor_image_info(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout) {
    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = sampler;
    descriptor_image_info.imageView = image_view;
    descriptor_image_info.imageLayout = image_layout;
    return descriptor_image_info;
}

VkWriteDescriptorSet vk_write_descriptor_set(VkDescriptorSet descriptor_set, uint32_t binding, VkDescriptorType descriptor_type, const VkDescriptorImageInfo *image_info, const VkDescriptorBufferInfo *buffer_info) {
    VkWriteDescriptorSet write_descriptor_set = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.descriptorType = descriptor_type;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = image_info;
    write_descriptor_set.pBufferInfo = buffer_info;
    return write_descriptor_set;
}
