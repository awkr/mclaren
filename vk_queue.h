#pragma once

#include <volk.h>

void vk_queue_submit(VkQueue queue, const VkSubmitInfo2 *submit_info, VkFence fence);
