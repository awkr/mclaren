#pragma once

#include <volk.h>

struct VkContext;

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence);

void vk_queue_submit(VkQueue queue, VkCommandBuffer command_buffer, VkFence fence);

VkResult vk_queue_present(VkContext *vk_context, uint32_t image_index, VkSemaphore wait_semaphore);
