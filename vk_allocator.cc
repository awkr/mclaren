#include "vk_allocator.h"
#include "logging.h"
#include "vk_context.h"

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

void vk_create_allocator(VkContext *vk_context) {
    VmaVulkanFunctions vulkan_functions{};
    vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = vk_context->physical_device;
    allocator_info.device = vk_context->device;
    allocator_info.instance = vk_context->instance;
    allocator_info.pVulkanFunctions = &vulkan_functions;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_1;

    VkResult result = vmaCreateAllocator(&allocator_info, &vk_context->allocator);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_allocator(VkContext *vk_context) {
    if (vk_context->is_debugging) {
        VmaTotalStatistics stats;
        vmaCalculateStatistics(vk_context->allocator, &stats);
        log_debug("vk device memory stats: allocation %d, allocated %d bytes", stats.total.statistics.allocationCount,
                  stats.total.statistics.allocationBytes);
    }
    vmaDestroyAllocator(vk_context->allocator);
}
