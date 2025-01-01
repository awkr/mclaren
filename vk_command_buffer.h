#pragma once

#include <functional>
#include <volk.h>

struct VkContext;

bool vk_alloc_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                              VkCommandBuffer *command_buffers);

bool vk_alloc_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer *command_buffer);

void vk_free_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                             VkCommandBuffer *command_buffers);

void vk_free_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer);

bool vk_reset_command_buffer(VkCommandBuffer command_buffer);

void vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags);

void vk_begin_one_flight_command_buffer(VkCommandBuffer command_buffer);

bool vk_end_command_buffer(VkCommandBuffer command_buffer);

VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer);

VkSubmitInfo2 vk_submit_info(VkCommandBufferSubmitInfo *command_buffer, VkSemaphoreSubmitInfo *wait_semaphore,
                             VkSemaphoreSubmitInfo *signal_semaphore);

void vk_command_buffer_submit(VkContext *vk_context, const std::function<void(VkCommandBuffer command_buffer)> &func);

void vk_cmd_clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_layout, VkClearColorValue *clear_color);

void vk_cmd_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent);

void vk_cmd_bind_pipeline(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline);

void vk_cmd_bind_descriptor_sets(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
                                 VkPipelineLayout pipeline_layout, uint32_t set_count, const VkDescriptorSet *descriptor_sets);

void vk_cmd_bind_vertex_buffer(VkCommandBuffer command_buffer, VkBuffer buffer, uint64_t offset);
void vk_cmd_bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer buffer, uint64_t offset);

void vk_cmd_push_constants(VkCommandBuffer command_buffer, VkPipelineLayout layout, VkShaderStageFlags stage_flags,
                           uint32_t size, const void *data);

void vk_cmd_dispatch(VkCommandBuffer command_buffer, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

void vk_cmd_begin_rendering(VkCommandBuffer command_buffer, const VkOffset2D &offset, const VkExtent2D &extent,
                            const VkRenderingAttachmentInfo *color_attachments, uint32_t color_attachment_count,
                            const VkRenderingAttachmentInfo *depth_attachment);
void vk_cmd_end_rendering(VkCommandBuffer command_buffer);

void vk_cmd_set_viewport(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void vk_cmd_set_scissor(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void vk_cmd_set_primitive_topology(VkCommandBuffer command_buffer, VkPrimitiveTopology primitive_topology);
void vk_cmd_set_depth_bias(VkCommandBuffer command_buffer, float constant_factor, float clamp, float slope_factor);
void vk_cmd_set_depth_test_enable(VkCommandBuffer command_buffer, bool enable);

void vk_cmd_draw(VkCommandBuffer command_buffer, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
                 uint32_t first_instance);

void vk_cmd_draw_indexed(VkCommandBuffer command_buffer, uint32_t index_count);

void vk_cmd_copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkBuffer dst, uint32_t size, uint32_t src_offset, uint32_t dst_offset);

void vk_cmd_copy_buffer_to_image(VkCommandBuffer command_buffer, VkBuffer src, VkImage dst, VkImageLayout layout, uint32_t width, uint32_t height);
void vk_cmd_copy_image_to_buffer(VkCommandBuffer command_buffer, VkImage src_image, const VkOffset2D &image_offset, const VkExtent2D &image_extent, VkBuffer dst_buffer) noexcept;

void vk_cmd_pipeline_image_barrier(VkCommandBuffer command_buffer, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask);
void vk_cmd_pipeline_barrier2(VkCommandBuffer command_buffer, VkBuffer buffer, uint64_t offset, uint64_t size, VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask);
