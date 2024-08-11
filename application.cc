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
    app->frame_index = app->frame_number % FRAMES_IN_FLIGHT;
    Frame *frame = &app->vk_context->frames[app->frame_index];

    vk_wait_fence(app->vk_context->device, frame->in_flight_fence);
    vk_reset_fence(app->vk_context->device, frame->in_flight_fence);

    uint32_t image_index;
    vk_acquire_next_image(app->vk_context, frame->image_acquired_semaphore, &image_index);

    log_debug("frame %lld, frame index %d, image index %d", app->frame_number, app->frame_index, image_index);

    VkCommandBuffer command_buffer = frame->command_buffer;
    {
        vk_begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vk_transition_image_layout(command_buffer, app->vk_context->swapchain_images[image_index],
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clear_color{};
        clear_color.float32[0] = 0.0f;
        clear_color.float32[1] = 1.0f;
        clear_color.float32[2] = 1.0f;
        clear_color.float32[3] = 1.0f;

        vk_command_clear_color_image(command_buffer, app->vk_context->swapchain_images[image_index],
                                     VK_IMAGE_LAYOUT_GENERAL, &clear_color);

        vk_transition_image_layout(command_buffer, app->vk_context->swapchain_images[image_index],
                                   VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vk_end_command_buffer(command_buffer);
    }

    VkSemaphoreSubmitInfoKHR wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore,
                                                                       VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    VkSemaphoreSubmitInfoKHR signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore, 0);
    VkCommandBufferSubmitInfoKHR command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

    vk_present(app->vk_context, image_index, frame->render_finished_semaphore);

    ++app->frame_number;
}
