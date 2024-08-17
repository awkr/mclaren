#include "vk_allocator.h"
#include "logging.h"
#include "vk_context.h"

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

void vk_create_allocator(VkContext *vk_context) {
    VmaVulkanFunctions vulkan_functions{};
    vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info{};
    allocator_create_info.physicalDevice = vk_context->physical_device;
    allocator_create_info.device = vk_context->device;
    allocator_create_info.instance = vk_context->instance;
    allocator_create_info.pVulkanFunctions = &vulkan_functions;
    allocator_create_info.vulkanApiVersion = vk_context->api_version;
    allocator_create_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VkResult result = vmaCreateAllocator(&allocator_create_info, &vk_context->allocator);
    ASSERT(result == VK_SUCCESS);
}

void vk_destroy_allocator(VkContext *vk_context) {
    if (vk_context->is_debugging_mode) {
        VmaTotalStatistics stats;
        vmaCalculateStatistics(vk_context->allocator, &stats);
        log_debug("vk device memory stats: allocation %d, allocated %d bytes", stats.total.statistics.allocationCount,
                  stats.total.statistics.allocationBytes);
    }
    vmaDestroyAllocator(vk_context->allocator);
}
