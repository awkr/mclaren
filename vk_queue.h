#pragma once

#include <volk.h>

struct VkContext;

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence);

VkResult vk_present(VkContext *vk_context, uint32_t image_index, VkSemaphore wait_semaphore);
