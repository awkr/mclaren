#include "app.h"
#include "core/deletion_queue.h"
#include "core/logging.h"
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
#include "vk_buffer.h"
#include <SDL3/SDL.h>
#include <imgui.h>

// vulkan clip space has inverted Y and half Z
glm::mat4 clip = glm::mat4(
        // clang-format off
        1.0f,  0.0f, 0.0f, 0.0f, // 1st column
        0.0f, -1.0f, 0.0f, 0.0f, // 2nd column
        0.0f,  0.0f, 0.5f, 0.5f, // 3rd column
        0.0f,  0.0f, 0.0f, 1.0f  // 4th column
        // clang-format on
);

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
    vk_create_image_view(vk_context->device, (*app)->drawable_image, format, VK_IMAGE_ASPECT_COLOR_BIT,
                         &(*app)->drawable_image_view);

    // create depth image
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
    vk_create_image(vk_context, vk_context->swapchain_extent.width, vk_context->swapchain_extent.height, depth_format,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &(*app)->depth_image, &(*app)->depth_image_allocation);
    vk_create_image_view(vk_context->device, (*app)->depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT,
                         &(*app)->depth_image_view);

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

        vk_create_pipeline_layout(vk_context->device, (*app)->descriptor_set_layout, nullptr,
                                  &(*app)->compute_pipeline_layout);
        vk_create_compute_pipeline(vk_context->device, (*app)->compute_pipeline_layout, compute_shader_module,
                                   &(*app)->compute_pipeline);

        vk_destroy_shader_module(vk_context->device, compute_shader_module);
    }

    { // create mesh pipeline
        VkShaderModule vert_shader;
        vk_create_shader_module(vk_context->device, "shaders/mesh.vert.spv", &vert_shader);

        VkShaderModule frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/mesh.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(MeshPushConstants);

        vk_create_pipeline_layout(vk_context->device, VK_NULL_HANDLE, &push_constant_range,
                                  &(*app)->mesh_pipeline_layout);
        vk_create_graphics_pipeline(vk_context->device, (*app)->mesh_pipeline_layout, format, depth_format,
                                    {{VK_SHADER_STAGE_VERTEX_BIT,   vert_shader},
                                     {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, &(*app)->mesh_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    // create ui
    (*app)->gui_context = ImGui::CreateContext();

    // load_gltf((*app)->vk_context, "models/cube.gltf", &(*app)->geometry);
    load_gltf((*app)->vk_context, "models/chinese-dragon.gltf", &(*app)->geometry);

    create_camera(&(*app)->camera, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f));

    (*app)->frame_number = 0;
}

void app_destroy(App *app) {
    vkDeviceWaitIdle(app->vk_context->device);

    destroy_camera(&app->camera);

    destroy_geometry(app->vk_context, &app->geometry);

    vk_destroy_pipeline(app->vk_context->device, app->mesh_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->mesh_pipeline_layout);

    ImGui::DestroyContext(app->gui_context);

    vk_destroy_pipeline(app->vk_context->device, app->compute_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->compute_pipeline_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->descriptor_set_layout);
    vk_destroy_descriptor_pool(app->vk_context->device, app->descriptor_pool);

    vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
    vk_destroy_image(app->vk_context, app->depth_image, app->depth_image_allocation);

    vk_destroy_image_view(app->vk_context->device, app->drawable_image_view);
    vk_destroy_image(app->vk_context, app->drawable_image, app->drawable_image_allocation);

    vk_terminate(app->vk_context);
    delete app->vk_context;
    delete app;
}

void draw_background(const App *app, VkCommandBuffer command_buffer) {
    vk_command_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline);
    vk_command_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline_layout, 0, 1,
                                    &app->descriptor_set, 0, nullptr);
    vk_command_dispatch(command_buffer, std::ceil(app->vk_context->swapchain_extent.width / 16.0),
                        std::ceil(app->vk_context->swapchain_extent.height / 16.0), 1);
}

void draw_geometry(const App *app, VkCommandBuffer command_buffer, VkImageView image_view) {
    VkRenderingAttachmentInfo color_attachment = {};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.imageView = image_view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depth_attachment = {};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.imageView = app->depth_image_view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue.depthStencil.depth = 1.0f;

    const VkExtent2D *extent = &app->vk_context->swapchain_extent;

    vk_command_begin_rendering(command_buffer, extent, &color_attachment, 1, &depth_attachment);

    vk_command_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline);

    vk_command_set_viewport(command_buffer, 0, 0, extent->width, extent->height);
    vk_command_set_scissor(command_buffer, 0, 0, extent->width, extent->height);

    for (const Mesh &mesh: app->geometry.meshes) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));

        glm::mat4 view = app->camera.view_matrix;

        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(60.0f), (float) app->vk_context->swapchain_extent.width /
                                                           (float) app->vk_context->swapchain_extent.height,
                                      0.1f, 1000.0f);

        MeshPushConstants mesh_instance_state{};
        mesh_instance_state.model = model;
        mesh_instance_state.view = view;
        mesh_instance_state.projection = clip * projection;
        mesh_instance_state.vertex_buffer_device_address = mesh.mesh_buffer.vertex_buffer_device_address;

        vk_command_push_constants(command_buffer, app->mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                                  sizeof(MeshPushConstants), &mesh_instance_state);

        for (const Primitive &primitive: mesh.primitives) {
            vk_command_bind_index_buffer(command_buffer, mesh.mesh_buffer.index_buffer.handle, primitive.index_offset);
            vk_command_draw_indexed(command_buffer, primitive.index_count);
        }
    }

    vk_command_end_rendering(command_buffer);
}

void app_update(App *app) {
    app->frame_index = app->frame_number % FRAMES_IN_FLIGHT;

    camera_update(&app->camera);

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
        vk_begin_one_flight_command_buffer(command_buffer);

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

        vk_transition_image_layout(command_buffer, app->depth_image,
                                   VK_PIPELINE_STAGE_2_NONE | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                   VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                   VK_ACCESS_2_NONE | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

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

    VkSemaphoreSubmitInfo wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore,
                                                                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkSemaphoreSubmitInfo signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore,
                                                                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkCommandBufferSubmitInfo command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

    result = vk_queue_present(app->vk_context, image_index, frame->render_finished_semaphore);
    ASSERT(result == VK_SUCCESS);

    ++app->frame_number;
}

void app_key_up(App *app, uint32_t key) {
    if (key == SDLK_W) {
        Camera *camera = &app->camera;
        camera_forward(camera, 0.2f);
    } else if (key == SDLK_S) {
        Camera *camera = &app->camera;
        camera_backward(camera, 0.2f);
    } else if (key == SDLK_A) {
        Camera *camera = &app->camera;
        camera_left(camera, 0.2f);
    } else if (key == SDLK_D) {
        Camera *camera = &app->camera;
        camera_right(camera, 0.2f);
    } else if (key == SDLK_Q) {
        Camera *camera = &app->camera;
        camera_up(camera, 0.2f);
    } else if (key == SDLK_E) {
        Camera *camera = &app->camera;
        camera_down(camera, 0.2f);
    } else if (key == SDLK_UP) {
        Camera *camera = &app->camera;
        camera_pitch(camera, 2.0f);
    } else if (key == SDLK_DOWN) {
        Camera *camera = &app->camera;
        camera_pitch(camera, -2.0f);
    } else if (key == SDLK_LEFT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, 2.0f);
    } else if (key == SDLK_RIGHT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, -2.0f);
    }
}

void app_key_down(App *app, uint32_t key) {
    if (key == SDLK_W) {
        Camera *camera = &app->camera;
        camera_forward(camera, 0.2f);
    } else if (key == SDLK_S) {
        Camera *camera = &app->camera;
        camera_backward(camera, 0.2f);
    } else if (key == SDLK_A) {
        Camera *camera = &app->camera;
        camera_left(camera, 0.2f);
    } else if (key == SDLK_D) {
        Camera *camera = &app->camera;
        camera_right(camera, 0.2f);
    } else if (key == SDLK_Q) {
        Camera *camera = &app->camera;
        camera_up(camera, 0.2f);
    } else if (key == SDLK_E) {
        Camera *camera = &app->camera;
        camera_down(camera, 0.2f);
    } else if (key == SDLK_UP) {
        Camera *camera = &app->camera;
        camera_pitch(camera, 2.0f);
    } else if (key == SDLK_DOWN) {
        Camera *camera = &app->camera;
        camera_pitch(camera, -2.0f);
    } else if (key == SDLK_LEFT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, 2.0f);
    } else if (key == SDLK_RIGHT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, -2.0f);
    }
}
