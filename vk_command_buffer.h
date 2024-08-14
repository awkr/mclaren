#pragma once

#include <volk.h>

bool vk_reset_command_buffer(VkCommandBuffer command_buffer);

bool vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags);

bool vk_end_command_buffer(VkCommandBuffer command_buffer);

VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer);

VkSubmitInfo2 vk_submit_info(VkCommandBufferSubmitInfo *command_buffer, VkSemaphoreSubmitInfo *wait_semaphore,
                             VkSemaphoreSubmitInfo *signal_semaphore);

void vk_command_clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_layout,
                                  VkClearColorValue *clear_color);

void vk_command_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent);

void vk_command_bind_pipeline(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline);

void vk_command_bind_descriptor_sets(VkCommandBuffer command_buffer, VkPipelineLayout layout, uint32_t first_set,
                                     uint32_t descriptor_set_count, const VkDescriptorSet *descriptor_sets,
                                     uint32_t dynamic_offset_count, const uint32_t *dynamic_offsets);

void vk_command_bind_descriptor_sets(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
                                     VkPipelineLayout pipeline_layout, uint32_t first_set, uint32_t set_count,
                                     const VkDescriptorSet *descriptor_sets, uint32_t dynamic_offset_count,
                                     const uint32_t *dynamic_offsets);

void vk_command_dispatch(VkCommandBuffer command_buffer, uint32_t group_count_x, uint32_t group_count_y,
                         uint32_t group_count_z);
