#pragma once

#include "vk_defines.h"
#include <vector>
#include <vk_mem_alloc.h>

struct VkContext {
    uint32_t api_version;
    bool is_debugging_mode;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue;
    VmaAllocator allocator;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchain_extent;
    VkFormat swapchain_image_format;
    uint16_t swapchain_image_count;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

    VkCommandPool command_pool;
};
