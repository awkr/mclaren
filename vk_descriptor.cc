#include "vk_descriptor.h"
#include "core/logging.h"

void vk_create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings,
                                     VkDescriptorSetLayout *descriptor_set_layout) {
    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pBindings = bindings.data();
    create_info.bindingCount = bindings.size();
    VkResult result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, descriptor_set_layout);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout) {
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
}

void vk_create_descriptor_pool(VkDevice device, uint32_t max_sets, const std::vector<VkDescriptorPoolSize> &pool_sizes,
                               VkDescriptorPool *descriptor_pool) {
    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.flags = 0;
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

VkResult vk_allocate_descriptor_sets(VkDevice device, VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout *layouts, uint32_t descriptor_set_count, VkDescriptorSet *descriptor_sets) {
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = descriptor_set_count;
    allocate_info.pSetLayouts = layouts;
    return vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets);
}

void vk_free_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set) {
    VkResult result = vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set);
    ASSERT(result == VK_SUCCESS);
}

void
vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, uint32_t descriptor_count,
                         VkDescriptorType descriptor_type, const VkDescriptorImageInfo *image_info) {
    VkWriteDescriptorSet write_descriptor_set{};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.descriptorCount = descriptor_count;
    write_descriptor_set.descriptorType = descriptor_type;
    write_descriptor_set.pImageInfo = image_info;
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}

void vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding,
                              VkDescriptorType descriptor_type, const VkDescriptorBufferInfo *buffer_info) {
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = descriptor_type;
    write_descriptor_set.pBufferInfo = buffer_info;
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}

void vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, VkDescriptorType descriptor_type, const VkDescriptorImageInfo *image_info) {
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = descriptor_type;
    write_descriptor_set.pImageInfo = image_info;
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}

void vk_update_descriptor_sets(VkDevice device, uint32_t descriptor_write_count, const VkWriteDescriptorSet *descriptor_writes) {
    vkUpdateDescriptorSets(device, descriptor_write_count, descriptor_writes, 0, nullptr);
}
