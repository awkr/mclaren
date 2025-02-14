#include "vk.h"
#include "logging.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_context.h"
#include "vk_descriptor_allocator.h"
#include "vk_device.h"
#include "vk_fence.h"
#include "vk_instance.h"
#include "vk_semaphore.h"
#include "vk_swapchain.h"
#include <SDL3/SDL_vulkan.h>

void vk_init(VkContext *vk_context, SDL_Window *window, uint32_t width, uint32_t height) {
    VkResult result = volkInitialize();
    ASSERT(result == VK_SUCCESS);

    uint32_t version;
    result = vkEnumerateInstanceVersion(&version);
    ASSERT(result == VK_SUCCESS);
    const uint32_t major = VK_VERSION_MAJOR(version);
    const uint32_t minor = VK_VERSION_MINOR(version);
    const uint32_t patch = VK_VERSION_PATCH(version);

    bool succeed = vk_create_instance(vk_context, "mclaren", VK_MAKE_API_VERSION(0, major, minor, patch));
    ASSERT(succeed);
    succeed = SDL_Vulkan_CreateSurface(window, vk_context->instance, nullptr, &vk_context->surface);
    ASSERT(succeed);
    vk_create_device(vk_context);
    vk_create_swapchain(vk_context, width, height);
    vk_create_command_pool(vk_context->device, vk_context->graphics_queue_family_index, &vk_context->command_pool);
}

void vk_terminate(VkContext *vk_context) {
    vk_destroy_command_pool(vk_context->device, vk_context->command_pool);
    vk_destroy_swapchain(vk_context);
    vk_destroy_device(vk_context);
    vkDestroySurfaceKHR(vk_context->instance, vk_context->surface, nullptr);
    vk_destroy_instance(vk_context);
}

void vk_resize(VkContext *vk_context, uint32_t width, uint32_t height) {
    vk_wait_idle(vk_context);
    vk_destroy_swapchain(vk_context);
    vk_create_swapchain(vk_context, width, height);
}

void vk_wait_idle(VkContext *vk_context) { vkDeviceWaitIdle(vk_context->device); }

void vk_acquire_next_image(VkContext *vk_context, VkSemaphore present_complete_semaphore, uint32_t *image_index) {
    VkResult result = vkAcquireNextImageKHR(vk_context->device, vk_context->swapchain, UINT64_MAX, present_complete_semaphore, VK_NULL_HANDLE, image_index);
    ASSERT(result == VK_SUCCESS);
}
