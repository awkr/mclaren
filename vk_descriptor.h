#pragma once

#include <vector>
#include <volk.h>

void vk_create_descriptor_set_layout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings,
                                     VkDescriptorSetLayout *descriptor_set_layout);

void vk_destroy_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

void vk_create_descriptor_pool(VkDevice device, uint32_t max_sets, const std::vector<VkDescriptorPoolSize> &pool_sizes,
                               VkDescriptorPool *descriptor_pool);

void vk_destroy_descriptor_pool(VkDevice device, VkDescriptorPool descriptor_pool);

void vk_reset_descriptor_pool(VkDevice device, VkDescriptorPool descriptor_pool);

VkResult vk_allocate_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout layout,
                                    VkDescriptorSet *descriptor_set);

VkResult vk_allocate_descriptor_sets(VkDevice device, VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout *layouts, uint32_t descriptor_set_count, VkDescriptorSet *descriptor_sets);

void vk_free_descriptor_set(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set);

void
vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, uint32_t descriptor_count,
                         VkDescriptorType descriptor_type, const VkDescriptorImageInfo *image_info);

void vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding,
                              VkDescriptorType descriptor_type, const VkDescriptorBufferInfo *buffer_info);

void vk_update_descriptor_set(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, VkDescriptorType descriptor_type, const VkDescriptorImageInfo *image_info);

void vk_update_descriptor_sets(VkDevice device, uint32_t descriptor_write_count, const VkWriteDescriptorSet *descriptor_writes);
