#pragma once

#include <volk.h>

bool vk_create_semaphore(VkDevice device, VkSemaphore *semaphore);

void vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore);

VkSemaphoreSubmitInfo vk_semaphore_submit_info(VkSemaphore semaphore, VkPipelineStageFlags2 stage_mask);
