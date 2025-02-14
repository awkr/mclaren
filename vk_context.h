#pragma once

#include "vk_defines.h"
#include <vector>

struct VkContext {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchain_extent;
    VkFormat swapchain_image_format;
    uint16_t swapchain_image_count;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

    VkCommandPool command_pool;
};
