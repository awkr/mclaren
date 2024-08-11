#include "application.h"
#include "vk.h"
#include "vk_context.h"
#include "vk_command_buffer.h"
#include "vk_image.h"
#include "vk_fence.h"
#include "vk_semaphore.h"
#include "vk_queue.h"
#include "logging.h"

void app_create(SDL_Window *window, Application **app) {
    *app = new Application();
    (*app)->vk_context = new VkContext();
    vk_init((*app)->vk_context, window);
}

void app_destroy(Application *app) {
    vk_terminate(app->vk_context);
    delete app->vk_context;
    delete app;
}

void app_update(Application *app) {
    app->vk_context->frame_index = app->vk_context->frame_number % FRAMES_IN_FLIGHT;
    uint32_t frame_index = app->vk_context->frame_index;
    Frame *frame = &app->vk_context->frames[frame_index];

    vk_wait_fence(app->vk_context->device, frame->in_flight_fence);
    vk_reset_fence(app->vk_context->device, frame->in_flight_fence);

    uint32_t image_index;
    vk_acquire_next_image(app->vk_context, frame->image_acquired_semaphore, &image_index);

    printf("frame index %d, image index %d\n", frame_index, image_index);

    VkCommandBuffer command_buffer = frame->command_buffer;
    {
        vk_reset_command_buffer(command_buffer);
        vk_begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        {
            VkImageMemoryBarrier2 image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            image_barrier.srcAccessMask = 0;
            image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageSubresourceRange subresource_range = vk_image_subresource_range(aspect_mask);
            image_barrier.subresourceRange = subresource_range;

            image_barrier.image = app->vk_context->swapchain_images[image_index];

            VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependency_info.imageMemoryBarrierCount = 1;
            dependency_info.pImageMemoryBarriers = &image_barrier;

            vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);
        }
        VkClearColorValue clear_color{};
        clear_color.float32[0] = 0.0f;
        clear_color.float32[1] = 1.0f;
        clear_color.float32[2] = 1.0f;
        clear_color.float32[3] = 1.0f;

        vk_command_clear_color_image(command_buffer, app->vk_context->swapchain_images[image_index],
                                     VK_IMAGE_LAYOUT_GENERAL, &clear_color);
        {
            VkImageMemoryBarrier2 image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
            image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barrier.dstAccessMask = 0;
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageSubresourceRange subresource_range = vk_image_subresource_range(aspect_mask);
            image_barrier.subresourceRange = subresource_range;

            image_barrier.image = app->vk_context->swapchain_images[image_index];

            VkDependencyInfo dependency_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependency_info.imageMemoryBarrierCount = 1;
            dependency_info.pImageMemoryBarriers = &image_barrier;

            vkCmdPipelineBarrier2KHR(command_buffer, &dependency_info);
        }

        vk_end_command_buffer(command_buffer);
    }

    VkSemaphoreSubmitInfoKHR wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore,
                                                                       VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    VkSemaphoreSubmitInfoKHR signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore, 0);
    VkCommandBufferSubmitInfoKHR command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

    vk_present(app->vk_context, image_index, frame->render_finished_semaphore);

    ++app->vk_context->frame_number;
}
