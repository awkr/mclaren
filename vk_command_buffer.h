#pragma once

#include <volk.h>

bool vk_alloc_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                              VkCommandBuffer *command_buffers);

void vk_free_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                             VkCommandBuffer *command_buffers);

bool vk_reset_command_buffer(VkCommandBuffer command_buffer);

bool vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags);

bool vk_begin_one_flight_command_buffer(VkCommandBuffer command_buffer);

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

void vk_command_begin_rendering(VkCommandBuffer command_buffer, const VkExtent2D *extent,
                                const VkRenderingAttachmentInfo *attachments, uint32_t attachment_count);

void vk_command_end_rendering(VkCommandBuffer command_buffer);

void vk_command_set_viewport(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w,
                             uint32_t h);

void vk_command_set_scissor(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

void
vk_command_draw(VkCommandBuffer command_buffer, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
                uint32_t first_instance);

void
vk_command_copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkBuffer dst, uint32_t size, uint32_t src_offset,
                       uint32_t dst_offset);
