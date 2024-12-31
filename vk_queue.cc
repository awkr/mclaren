#include "vk_queue.h"
#include "logging.h"
#include "vk_command_buffer.h"
#include "vk_context.h"

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence) {
    VkResult result = vkQueueSubmit2KHR(queue, 1, submit_info, fence);
    ASSERT(result == VK_SUCCESS);
}

void vk_queue_submit(VkQueue queue, VkCommandBuffer command_buffer, VkFence fence) {
    VkCommandBufferSubmitInfo command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, nullptr, nullptr);
    VkResult result = vkQueueSubmit2KHR(queue, 1, &submit_info, fence);
    ASSERT(result == VK_SUCCESS);
}

VkResult vk_queue_present(VkContext *vk_context, VkSemaphore wait_semaphore, uint32_t image_index) {
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vk_context->swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.waitSemaphoreCount = 1;

    VkResult result = vkQueuePresentKHR(vk_context->graphics_queue, &present_info);
    return result;
}
