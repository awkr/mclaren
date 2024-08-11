#include "vk.h"
#include "vk_context.h"
#include "vk_instance.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_command_pool.h"
#include "vk_fence.h"
#include "vk_semaphore.h"
#include "logging.h"
#include <SDL3/SDL_vulkan.h>

void vk_init(VkContext *vk_context, SDL_Window *window) {
    vk_create_instance(vk_context, "mclaren", true);
    int ok = SDL_Vulkan_CreateSurface(window, vk_context->instance, nullptr, &vk_context->surface);
    ASSERT(ok == 0);
    vk_create_device(vk_context);

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    vk_create_swapchain(vk_context, width, height);

    ASSERT(vk_context->swapchain_image_count >= FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_create_command_pool(vk_context->device, vk_context->graphics_queue_family_index,
                               &vk_context->frames[i].command_pool);
        vk_allocate_command_buffers(vk_context->device, vk_context->frames[i].command_pool, 1,
                                    &vk_context->frames[i].command_buffer);

        vk_create_fence(vk_context->device, VK_FENCE_CREATE_SIGNALED_BIT, &vk_context->frames[i].in_flight_fence);
        vk_create_semaphore(vk_context->device, &vk_context->frames[i].image_acquired_semaphore);
        vk_create_semaphore(vk_context->device, &vk_context->frames[i].render_finished_semaphore);
    }
}

void vk_terminate(VkContext *vk_context) {
    vkDeviceWaitIdle(vk_context->device);
    for (uint16_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_destroy_semaphore(vk_context->device, vk_context->frames[i].render_finished_semaphore);
        vk_destroy_semaphore(vk_context->device, vk_context->frames[i].image_acquired_semaphore);
        vk_destroy_fence(vk_context->device, vk_context->frames[i].in_flight_fence);
        vk_destroy_command_pool(vk_context->device, vk_context->frames[i].command_pool);
    }
    vk_destroy_swapchain(vk_context);
    vk_destroy_device(vk_context);
    vkDestroySurfaceKHR(vk_context->instance, vk_context->surface, nullptr);
    vk_destroy_instance(vk_context);
}

void vk_acquire_next_image(VkContext *vk_context, VkSemaphore signal_semaphore, uint32_t *image_index) {
    VkResult result = vkAcquireNextImageKHR(vk_context->device, vk_context->swapchain, UINT64_MAX, signal_semaphore,
                                            VK_NULL_HANDLE, image_index);
    ASSERT(result == VK_SUCCESS);
}

void vk_present(VkContext *vk_context, uint32_t image_index, VkSemaphore wait_semaphore) {
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vk_context->swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.waitSemaphoreCount = 1;

    VkResult result = vkQueuePresentKHR(vk_context->graphics_queue, &present_info);
    ASSERT(result == VK_SUCCESS);
}
