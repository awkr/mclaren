#pragma once

#include <volk.h>

struct VkContext;

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence);

void vk_queue_submit(VkQueue queue, VkCommandBuffer command_buffer, VkFence fence);
void vk_queue_submit(VkQueue queue, VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence fence);

VkResult vk_queue_present(VkContext *vk_context, VkSemaphore wait_semaphore, uint32_t image_index);
