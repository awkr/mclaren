#include "vk_command_buffer.h"
#include "vk_context.h"
#include "vk_fence.h"
#include "vk_image.h"
#include "vk_queue.h"
#include "core/logging.h"

bool vk_alloc_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count,
                              VkCommandBuffer *command_buffers) {
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = count;
    VkResult result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers);
    return result == VK_SUCCESS;
}

bool vk_alloc_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer *command_buffer) {
    return vk_alloc_command_buffers(device, command_pool, 1, command_buffer);
}

void vk_free_command_buffers(VkDevice device, VkCommandPool command_pool, uint32_t count, VkCommandBuffer *command_buffers) {
    vkFreeCommandBuffers(device, command_pool, count, command_buffers);
}

void vk_free_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer command_buffer) {
    vk_free_command_buffers(device, command_pool, 1, &command_buffer);
}

bool vk_reset_command_buffer(VkCommandBuffer command_buffer) {
    VkResult result = vkResetCommandBuffer(command_buffer, 0);
    return result == VK_SUCCESS;
}

bool vk_begin_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = flags;
    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    return result == VK_SUCCESS;
}

bool vk_begin_one_flight_command_buffer(VkCommandBuffer command_buffer) {
    return vk_begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

bool vk_end_command_buffer(VkCommandBuffer command_buffer) {
    VkResult result = vkEndCommandBuffer(command_buffer);
    return result == VK_SUCCESS;
}

VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer) {
    VkCommandBufferSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    submit_info.commandBuffer = command_buffer;
    submit_info.deviceMask = 0;
    return submit_info;
}

VkSubmitInfo2 vk_submit_info(VkCommandBufferSubmitInfo *command_buffer, VkSemaphoreSubmitInfo *wait_semaphore,
                             VkSemaphoreSubmitInfo *signal_semaphore) {
    ASSERT(command_buffer);

    VkSubmitInfo2 submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = command_buffer;
    if (wait_semaphore) {
        submit_info.waitSemaphoreInfoCount = 1;
        submit_info.pWaitSemaphoreInfos = wait_semaphore;
    }
    if (signal_semaphore) {
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = signal_semaphore;
    }
    return submit_info;
}

void vk_command_buffer_submit(VkContext *vk_context, const std::function<void(VkCommandBuffer command_buffer)> &func) {
    VkCommandBuffer command_buffer;
    vk_alloc_command_buffers(vk_context->device, vk_context->command_pool, 1, &command_buffer);
    vk_begin_one_flight_command_buffer(command_buffer);
    func(command_buffer);
    vk_end_command_buffer(command_buffer);

    VkFence fence;
    vk_create_fence(vk_context->device, false, &fence);

    vk_queue_submit(vk_context->graphics_queue, command_buffer, fence); // todo: 区分 graphics queue 和 transfer queue

    vk_wait_fence(vk_context->device, fence);
    vk_destroy_fence(vk_context->device, fence);
}

void vk_cmd_clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_layout, VkClearColorValue *clear_color) {
    VkImageSubresourceRange clear_range = vk_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(command_buffer, image, image_layout, clear_color, 1, &clear_range);
}

void vk_cmd_blit_image(VkCommandBuffer command_buffer, VkImage src, VkImage dst, const VkExtent2D *extent) {
    VkImageBlit2KHR image_blit_region{};
    image_blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
    image_blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.srcSubresource.layerCount = 1;
    image_blit_region.srcOffsets[1].x = (int32_t) extent->width;
    image_blit_region.srcOffsets[1].y = (int32_t) extent->height;
    image_blit_region.srcOffsets[1].z = 1;
    image_blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_blit_region.dstSubresource.layerCount = 1;
    image_blit_region.dstOffsets[1].x = (int32_t) extent->width;
    image_blit_region.dstOffsets[1].y = (int32_t) extent->height;
    image_blit_region.dstOffsets[1].z = 1;

    VkBlitImageInfo2 blit_image_info{};
    blit_image_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blit_image_info.srcImage = src;
    blit_image_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blit_image_info.dstImage = dst;
    blit_image_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_image_info.regionCount = 1;
    blit_image_info.pRegions = &image_blit_region;
    blit_image_info.filter = VK_FILTER_NEAREST;

    vkCmdBlitImage2KHR(command_buffer, &blit_image_info);
}

void vk_cmd_bind_pipeline(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline) {
    vkCmdBindPipeline(command_buffer, bind_point, pipeline);
}

void vk_cmd_bind_descriptor_sets(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point,
                                 VkPipelineLayout pipeline_layout, uint32_t set_count, const VkDescriptorSet *descriptor_sets) {
    vkCmdBindDescriptorSets(command_buffer, bind_point, pipeline_layout, 0, set_count, descriptor_sets, 0, nullptr);
}

void vk_cmd_bind_vertex_buffer(VkCommandBuffer command_buffer, VkBuffer buffer, uint64_t offset) {
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &offset);
}

void vk_cmd_bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer buffer, uint64_t offset) {
    vkCmdBindIndexBuffer(command_buffer, buffer, offset, VK_INDEX_TYPE_UINT32);
}

void vk_cmd_push_constants(VkCommandBuffer command_buffer, VkPipelineLayout layout, VkShaderStageFlags stage_flags,
                           uint32_t size, const void *data) {
    vkCmdPushConstants(command_buffer, layout, stage_flags, 0, size, data);
}

void vk_cmd_dispatch(VkCommandBuffer command_buffer, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) {
    vkCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z);
}

void vk_cmd_begin_rendering(VkCommandBuffer command_buffer, const VkExtent2D *extent,
                            const VkRenderingAttachmentInfo *color_attachments, uint32_t color_attachment_count,
                            const VkRenderingAttachmentInfo *depth_attachment) {
    VkRenderingInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent = *extent;
    rendering_info.colorAttachmentCount = color_attachment_count;
    rendering_info.pColorAttachments = color_attachments;
    rendering_info.pDepthAttachment = depth_attachment;
    rendering_info.layerCount = 1;
    vkCmdBeginRenderingKHR(command_buffer, &rendering_info);
}

void vk_cmd_end_rendering(VkCommandBuffer command_buffer) { vkCmdEndRenderingKHR(command_buffer); }

void vk_cmd_set_viewport(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    VkViewport viewport{};
    viewport.x = (float) x;
    viewport.y = (float) y;
    viewport.width = (float) w;
    viewport.height = (float) h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

void vk_cmd_set_scissor(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    VkRect2D scissor{};
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = w;
    scissor.extent.height = h;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void vk_cmd_set_primitive_topology(VkCommandBuffer command_buffer, VkPrimitiveTopology primitive_topology) {
    vkCmdSetPrimitiveTopologyEXT(command_buffer, primitive_topology);
}

void vk_cmd_set_depth_bias(VkCommandBuffer command_buffer, float constant_factor, float clamp, float slope_factor) {
    vkCmdSetDepthBias(command_buffer, constant_factor, clamp, slope_factor);
}

void vk_cmd_set_depth_test_enable(VkCommandBuffer command_buffer, bool enable) { vkCmdSetDepthTestEnableEXT(command_buffer, enable); }

void vk_cmd_draw(VkCommandBuffer command_buffer, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
    vkCmdDraw(command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void vk_cmd_draw_indexed(VkCommandBuffer command_buffer, uint32_t index_count) {
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
}

void vk_cmd_copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkBuffer dst, uint32_t size, uint32_t src_offset, uint32_t dst_offset) {
    VkBufferCopy2 buffer_copy{};
    buffer_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
    buffer_copy.size = size;
    buffer_copy.srcOffset = src_offset;
    buffer_copy.dstOffset = dst_offset;

    VkCopyBufferInfo2 buffer_copy_info{};
    buffer_copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
    buffer_copy_info.srcBuffer = src;
    buffer_copy_info.dstBuffer = dst;
    buffer_copy_info.regionCount = 1;
    buffer_copy_info.pRegions = &buffer_copy;

    vkCmdCopyBuffer2KHR(command_buffer, &buffer_copy_info);
}

void vk_cmd_copy_buffer_to_image(VkCommandBuffer command_buffer, VkBuffer src, VkImage dst, VkImageLayout layout, uint32_t width, uint32_t height) {
    VkBufferImageCopy buffer_image_copy{};
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset = {0, 0, 0};
    buffer_image_copy.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer, src, dst, layout, 1, &buffer_image_copy);
}
