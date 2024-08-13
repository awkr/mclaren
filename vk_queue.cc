#include "vk_queue.h"
#include "vk_context.h"
#include "logging.h"

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence) {
    VkResult result = vkQueueSubmit2KHR(queue, 1, submit_info, fence);
    ASSERT(result == VK_SUCCESS);
}

VkResult vk_present(VkContext *vk_context, uint32_t image_index, VkSemaphore wait_semaphore) {
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
