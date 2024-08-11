#include "vk_queue.h"
#include "logging.h"

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence) {
    VkResult result = vkQueueSubmit2KHR(queue, 1, submit_info, fence);
    ASSERT(result == VK_SUCCESS);
}
