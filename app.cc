#include "app.h"
#include "deletion_queue.h"
#include "vk.h"
#include "vk_context.h"
#include "vk_command_buffer.h"
#include "vk_image.h"
#include "vk_fence.h"
#include "vk_semaphore.h"
#include "vk_queue.h"
#include "vk_image.h"
#include "vk_image_view.h"
#include "vk_descriptor.h"
#include "vk_pipeline.h"
#include "logging.h"
#include <SDL3/SDL.h>
#include <imgui.h>

void app_create(SDL_Window *window, App **app) {
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    *app = new App();
    (*app)->window = window;
    (*app)->vk_context = new VkContext();
    vk_init((*app)->vk_context, window, width, height);

    VkContext *vk_context = (*app)->vk_context;

    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_STORAGE_BIT /* can be written by computer shader */;
    vk_create_image(vk_context, vk_context->swapchain_extent.width, vk_context->swapchain_extent.height, format, usage,
                    &(*app)->drawable_image, &(*app)->drawable_image_allocation);
    vk_create_image_view(vk_context->device, (*app)->drawable_image, format, &(*app)->drawable_image_view);

    // create a descriptor pool that holds `max_sets` sets with 1 image each
    uint32_t max_sets = 1;
    std::vector<VkDescriptorPoolSize> pool_sizes;
    pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 * max_sets});
    vk_create_descriptor_pool(vk_context->device, max_sets, pool_sizes, &(*app)->descriptor_pool);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
    vk_create_descriptor_set_layout(vk_context->device, bindings, &(*app)->descriptor_set_layout);

    vk_allocate_descriptor_set(vk_context->device, (*app)->descriptor_pool, (*app)->descriptor_set_layout,
                               &(*app)->descriptor_set);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = (*app)->drawable_image_view;

    vk_update_descriptor_set(vk_context->device, (*app)->descriptor_set, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                             &image_info);

    {// create compute pipeline
        VkShaderModule compute_shader_module;
        vk_create_shader_module(vk_context->device, "shaders/gradient.comp.spv", &compute_shader_module);

        vk_create_pipeline_layout(vk_context->device, (*app)->descriptor_set_layout, &(*app)->compute_pipeline_layout);
        vk_create_compute_pipeline(vk_context->device, (*app)->compute_pipeline_layout, compute_shader_module,
                                   &(*app)->compute_pipeline);

        vk_destroy_shader_module(vk_context->device, compute_shader_module);
    }

    { // create graphics pipeline
        VkShaderModule vert_shader_module;
        vk_create_shader_module(vk_context->device, "shaders/colored-triangle.vert.spv", &vert_shader_module);

        VkShaderModule frag_shader_module;
        vk_create_shader_module(vk_context->device, "shaders/colored-triangle.frag.spv", &frag_shader_module);

        vk_create_pipeline_layout(vk_context->device, VK_NULL_HANDLE, &(*app)->graphics_pipeline_layout);
        vk_create_graphics_pipeline(vk_context->device, (*app)->graphics_pipeline_layout, format, VK_FORMAT_UNDEFINED,
                                    {{VK_SHADER_STAGE_VERTEX_BIT,   vert_shader_module},
                                     {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_module}}, &(*app)->graphics_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader_module);
        vk_destroy_shader_module(vk_context->device, vert_shader_module);
    }

    // create ui
    (*app)->gui_context = ImGui::CreateContext();

    (*app)->frame_number = 0;
}

void app_destroy(App *app) {
    vkDeviceWaitIdle(app->vk_context->device);

    vk_destroy_pipeline(app->vk_context->device, app->graphics_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->graphics_pipeline_layout);

    ImGui::DestroyContext(app->gui_context);

    vk_destroy_pipeline(app->vk_context->device, app->compute_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->compute_pipeline_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->descriptor_set_layout);
    vk_destroy_descriptor_pool(app->vk_context->device, app->descriptor_pool);
    vk_destroy_image_view(app->vk_context->device, app->drawable_image_view);
    vk_destroy_image(app->vk_context, app->drawable_image, app->drawable_image_allocation);

    vk_terminate(app->vk_context);
    delete app->vk_context;
    delete app;
}

void draw_background(App *app, VkCommandBuffer command_buffer) {
    vk_command_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline);
    vk_command_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline_layout, 0, 1,
                                    &app->descriptor_set, 0, nullptr);
    vk_command_dispatch(command_buffer, std::ceil(app->vk_context->swapchain_extent.width / 16.0),
                        std::ceil(app->vk_context->swapchain_extent.height / 16.0), 1);
}

void draw_geometry(App *app, VkCommandBuffer command_buffer, VkImageView image_view) {
    VkRenderingAttachmentInfo color_attachment_info{};
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.imageView = image_view;
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    const VkExtent2D *extent = &app->vk_context->swapchain_extent;

    vk_command_begin_rendering(command_buffer, extent, &color_attachment_info, 1);

    vk_command_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphics_pipeline);

    vk_command_set_viewport(command_buffer, 0, 0, extent->width, extent->height);
    vk_command_set_scissor(command_buffer, 0, 0, extent->width, extent->height);

    vk_command_draw(command_buffer, 3, 1, 0, 0);

    vk_command_end_rendering(command_buffer);
}

void app_update(App *app) {
    app->frame_index = app->frame_number % FRAMES_IN_FLIGHT;

    Frame *frame = &app->vk_context->frames[app->frame_index];

    vk_wait_fence(app->vk_context->device, frame->in_flight_fence);
    vk_reset_fence(app->vk_context->device, frame->in_flight_fence);

    uint32_t image_index;
    VkResult result = vk_acquire_next_image(app->vk_context, frame->image_acquired_semaphore, &image_index);
    ASSERT(result == VK_SUCCESS);

    VkImage swapchain_image = app->vk_context->swapchain_images[image_index];

    // log_debug("frame %lld, frame index %d, image index %d", app->frame_number, app->frame_index, image_index);

    VkCommandBuffer command_buffer = frame->command_buffer;
    {
        vk_begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vk_transition_image_layout(command_buffer, app->drawable_image,
                                   VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                                   VK_PIPELINE_STAGE_2_BLIT_BIT, // could be in layout transition or computer shader writing or blit operation of current frame or previous frame
                                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                   VK_ACCESS_2_TRANSFER_READ_BIT,
                                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        draw_background(app, command_buffer);

        vk_transition_image_layout(command_buffer, app->drawable_image,
                                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                   VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                   VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                                   VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        draw_geometry(app, command_buffer, app->drawable_image_view);

        vk_transition_image_layout(command_buffer, app->drawable_image,
                                   VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                   VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_2_TRANSFER_READ_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vk_transition_image_layout(command_buffer, swapchain_image,
                                   VK_PIPELINE_STAGE_2_NONE,
                                   VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                   VK_ACCESS_2_NONE,
                                   VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkExtent2D extent = {app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height};
        vk_command_blit_image(command_buffer, app->drawable_image, swapchain_image, &extent);

        vk_transition_image_layout(command_buffer, swapchain_image,
                                   VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                   VK_ACCESS_2_NONE,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vk_end_command_buffer(command_buffer);
    }

    VkSemaphoreSubmitInfoKHR wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore,
                                                                       VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkSemaphoreSubmitInfoKHR signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore,
                                                                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkCommandBufferSubmitInfoKHR command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

    result = vk_queue_present(app->vk_context, image_index, frame->render_finished_semaphore);
    ASSERT(result == VK_SUCCESS);

    ++app->frame_number;
}
