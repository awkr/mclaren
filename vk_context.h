#pragma once

#include <vector>
#include <volk.h>
#include <vk_mem_alloc.h>

#define FRAMES_IN_FLIGHT 2

struct Frame {
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_acquired_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
};

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

    Frame frames[FRAMES_IN_FLIGHT];
};
