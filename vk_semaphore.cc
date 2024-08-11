#include "vk_semaphore.h"

bool vk_create_semaphore(VkDevice device, VkSemaphore *semaphore) {
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkResult result = vkCreateSemaphore(device, &semaphore_create_info, nullptr, semaphore);
    return result == VK_SUCCESS;
}

void vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore) {
    vkDestroySemaphore(device, semaphore, nullptr);
}

VkSemaphoreSubmitInfo vk_semaphore_submit_info(VkSemaphore semaphore, VkPipelineStageFlags2 stage_mask) {
    VkSemaphoreSubmitInfo semaphore_submit_info{};
    semaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    semaphore_submit_info.semaphore = semaphore;
    semaphore_submit_info.stageMask = stage_mask;
    return semaphore_submit_info;
}
