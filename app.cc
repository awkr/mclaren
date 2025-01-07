#include "app.h"
#include "core/deletion_queue.h"
#include "frame_graph.h"
#include "logging.h"
#include "mesh_loader.h"
#include "vk.h"
#include "vk_buffer.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_context.h"
#include "vk_descriptor.h"
#include "vk_descriptor_allocator.h"
#include "vk_fence.h"
#include "vk_framebuffer.h"
#include "vk_image.h"
#include "vk_image_view.h"
#include "vk_pipeline.h"
#include "vk_queue.h"
#include "vk_render_pass.h"
#include "vk_sampler.h"
#include "vk_semaphore.h"
#include "vk_swapchain.h"
#include <glm/ext/scalar_reciprocal.hpp>
#include <glm/gtc/type_ptr.inl>
#include <glm/gtx/string_cast.hpp>
#include <thread>
#include <chrono>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <microprofile.h>

// vulkan clip space has inverted Y and half Z
glm::mat4 clip = glm::mat4(
    // clang-format off
    1.0f,  0.0f, 0.0f, 0.0f, // 1st column
    0.0f, -1.0f, 0.0f, 0.0f, // 2nd column
    0.0f,  0.0f, 0.5f, 0.0f, // 3rd column
    0.0f,  0.0f, 0.5f, 1.0f  // 4th column
    // clang-format on
);

glm::vec2 world_position_to_screen_position(const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, uint16_t screen_width, uint16_t screen_height, const glm::vec3 &world_pos) {
    glm::vec4 clip_pos = projection_matrix * view_matrix * glm::vec4(world_pos, 1.0f);
    glm::vec3 ndc_pos = glm::vec3(clip_pos) / clip_pos.w;
    glm::vec2 screen_pos = (glm::vec2(ndc_pos.x, ndc_pos.y) * 0.5f + 0.5f) * glm::vec2(screen_width, screen_height);
    return screen_pos;
}

// z_ndc: 0 for near plane, 1 for far plane
glm::vec3 screen_position_to_world_position(const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, uint32_t viewport_size_width, uint32_t viewport_size_height, float z_ndc, float screen_pos_x, float screen_pos_y) {
    // return glm::unProject(glm::vec3(screen_pos_x, viewport_size_height - screen_pos_y, z_ndc), view_matrix, projection_matrix, glm::vec4(0, 0, viewport_size_width, viewport_size_height));
    const float x_ndc = (2.0f * screen_pos_x) / (float) viewport_size_width - 1.0f;
    const float y_ndc = (2.0f * (viewport_size_height - screen_pos_y)) / (float) viewport_size_height - 1.0f;
    glm::vec4 ndc_pos(x_ndc, y_ndc, z_ndc, 1.0f);
    glm::vec4 world_pos = glm::inverse(projection_matrix * view_matrix) * ndc_pos;
    world_pos /= world_pos.w;
    return glm::vec3(world_pos);
}

// fire a ray from camera position to the world position of the screen position
Ray ray_from_screen(const VkExtent2D &viewport_extent, const float screen_pos_x, const float screen_pos_y, const glm::vec3 &camera_pos, const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) {
    const glm::vec3 near_pos = screen_position_to_world_position(projection_matrix, view_matrix, viewport_extent.width, viewport_extent.height, 0.0f, screen_pos_x, screen_pos_y);
    Ray ray{};
    ray.origin = camera_pos;
    ray.direction = glm::normalize(near_pos - camera_pos);
    return ray;
}

void create_color_image(App *app, const VkFormat format) {
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_STORAGE_BIT /* can be written by computer shader */;
    vk_create_image(app->vk_context, app->vk_context->swapchain_extent, format, usage, false, &app->color_image);
    vk_create_image_view(app->vk_context->device, app->color_image->handle, format, VK_IMAGE_ASPECT_COLOR_BIT, &app->color_image_view);
}

void create_depth_image(App *app, const VkFormat format) {
    vk_create_image(app->vk_context, app->vk_context->swapchain_extent, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, false, &app->depth_image);
    vk_create_image_view(app->vk_context->device, app->depth_image->handle, format, VK_IMAGE_ASPECT_DEPTH_BIT, &app->depth_image_view);
}

void app_create(SDL_Window *window, App **out_app) {
    int render_width, render_height;
    SDL_GetWindowSizeInPixels(window, &render_width, &render_height);

    App *app = new App();
    *out_app = app;
    app->window = window;
    app->vk_context = new VkContext();

    vk_init(app->vk_context, window, render_width, render_height);

    VkContext *vk_context = app->vk_context;

    {
      AttachmentConfig color_attachment_config = {vk_context->swapchain_image_format, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
      AttachmentConfig depth_attachment_config = {VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
      vk_create_render_pass(vk_context->device, &color_attachment_config, &depth_attachment_config, &app->lit_render_pass);
    }
    {
      AttachmentConfig depth_attachment_config = {VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
      vk_create_render_pass(vk_context->device, nullptr, &depth_attachment_config, &app->entity_picking_render_pass);
    }
    {
      AttachmentConfig color_attachment_config = {vk_context->swapchain_image_format, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
      AttachmentConfig depth_attachment_config = {VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
      vk_create_render_pass(vk_context->device, &color_attachment_config, &depth_attachment_config, &app->vertex_lit_render_pass);
    }

    create_depth_image(app, VK_FORMAT_D32_SFLOAT);

    app->framebuffers.resize(vk_context->swapchain_image_count);

    for (size_t i = 0; i < vk_context->swapchain_image_count; ++i) {
      VkImageView attachments[2] = {vk_context->swapchain_image_views[i], app->depth_image_view};
      vk_create_framebuffer(vk_context->device, vk_context->swapchain_extent, app->lit_render_pass, attachments, 2, &app->framebuffers[i]);
    }

    // 创建 entity picking 相关资源 todo 移入类似 on_plugin_prepare 方法
    app->entity_picking_framebuffers.resize(FRAMES_IN_FLIGHT);
    app->entity_picking_storage_buffers.resize(FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
      {
        VkImageView attachments[1] = {app->depth_image_view};
        vk_create_framebuffer(vk_context->device, vk_context->swapchain_extent, app->entity_picking_render_pass, attachments, 1, &app->entity_picking_framebuffers[i]);
      }

      size_t storage_buffer_size = sizeof(uint32_t); // size of one pixel
      vk_create_buffer_vma(app->vk_context, storage_buffer_size,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                           VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                           &app->entity_picking_storage_buffers[i]);
    }

    // 创建 gizmo render pass 相关资源 todo 移入类似 on_plugin_prepare 方法
    app->gizmo_framebuffers.resize(app->vk_context->swapchain_image_count);
    for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
      VkImageView attachments[2] = {app->vk_context->swapchain_image_views[i], app->depth_image_view};
      vk_create_framebuffer(vk_context->device, vk_context->swapchain_extent, app->vertex_lit_render_pass, attachments, 2, &app->gizmo_framebuffers[i]);
    }

    ASSERT(FRAMES_IN_FLIGHT <= vk_context->swapchain_image_count);

    app->present_complete_semaphores.resize(vk_context->swapchain_image_count, VK_NULL_HANDLE);
    app->render_complete_semaphores.resize(vk_context->swapchain_image_count, VK_NULL_HANDLE);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        RenderFrame *frame = &app->frames[i];
        vk_create_command_pool(vk_context->device, vk_context->graphics_queue_family_index, &frame->command_pool);

        vk_create_fence(vk_context->device, true, &frame->in_flight_fence);

        uint32_t max_sets = 100;
        std::unordered_map<VkDescriptorType, uint32_t> descriptor_count;
        descriptor_count.insert({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100});
        descriptor_count.insert({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100});
        descriptor_count.insert({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100});
        descriptor_count.insert({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100});
        vk_descriptor_allocator_create(vk_context->device, max_sets, descriptor_count, &frame->descriptor_allocator);

        vk_create_buffer_vma(vk_context, sizeof(GlobalState), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, &frame->global_uniform_buffer);
    }

    VkFormat color_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    create_color_image(app, color_image_format);

    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->single_storage_image_descriptor_set_layout);
    }
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->single_storage_buffer_descriptor_set_layout);
    }
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->global_uniform_buffer_descriptor_set_layout);
    }
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->single_combined_image_sampler_descriptor_set_layout);
    }

    {
        VkShaderModule compute_shader_module;
        vk_create_shader_module(vk_context->device, "shaders/gradient.comp.spv", &compute_shader_module);

        vk_create_pipeline_layout(vk_context->device, 1, &app->single_storage_image_descriptor_set_layout, nullptr, &app->compute_pipeline_layout);
        vk_create_compute_pipeline(vk_context->device, app->compute_pipeline_layout, compute_shader_module, &app->compute_pipeline);

        vk_destroy_shader_module(vk_context->device, compute_shader_module);
    }

    {
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/lit.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/lit.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(InstanceState);

        VkDescriptorSetLayout descriptor_set_layouts[2];
        descriptor_set_layouts[0] = app->global_uniform_buffer_descriptor_set_layout;
        descriptor_set_layouts[1] = app->single_combined_image_sampler_descriptor_set_layout;
        vk_create_pipeline_layout(vk_context->device, 2, descriptor_set_layouts, &push_constant_range, &app->lit_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        DepthConfig depth_config{};
        depth_config.enable_test = true;
        depth_config.enable_write = true;
        depth_config.is_depth_test_dynamic = false;
        DepthBiasConfig depth_bias_config{};
        depth_bias_config.enable = true;
        depth_bias_config.constant_factor = 1.0f;
        depth_bias_config.slope_factor = 0.5f;
        vk_create_graphics_pipeline(vk_context->device, app->lit_pipeline_layout, true, depth_config, depth_bias_config,
                                    {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, primitive_topologies, VK_POLYGON_MODE_FILL, app->lit_render_pass, &app->lit_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
      VkShaderModule vert_shader, frag_shader;
      vk_create_shader_module(vk_context->device, "shaders/wireframe.vert.spv", &vert_shader);
      vk_create_shader_module(vk_context->device, "shaders/wireframe.frag.spv", &frag_shader);

      VkPushConstantRange push_constant_range{};
      push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      push_constant_range.size = sizeof(InstanceState);

      VkDescriptorSetLayout descriptor_set_layouts[1];
      descriptor_set_layouts[0] = app->global_uniform_buffer_descriptor_set_layout;
      vk_create_pipeline_layout(vk_context->device, 1, descriptor_set_layouts, &push_constant_range, &app->wireframe_pipeline_layout);
      std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
      DepthConfig depth_config{};
      depth_config.enable_test = true;
      depth_config.enable_write = false;
      depth_config.is_depth_test_dynamic = false;
      vk_create_graphics_pipeline(vk_context->device, app->wireframe_pipeline_layout, true, depth_config, {},
                                  {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, primitive_topologies, VK_POLYGON_MODE_LINE, app->lit_render_pass, &app->wireframe_pipeline);

      vk_destroy_shader_module(vk_context->device, frag_shader);
      vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
      VkShaderModule vert_shader, frag_shader;
      vk_create_shader_module(vk_context->device, "shaders/vertex-lit.vert.spv", &vert_shader);
      vk_create_shader_module(vk_context->device, "shaders/vertex-lit.frag.spv", &frag_shader);

      VkPushConstantRange push_constant_range{};
      push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      push_constant_range.size = sizeof(InstanceState);

      std::vector<VkDescriptorSetLayout> descriptor_set_layouts{app->global_uniform_buffer_descriptor_set_layout};
      vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->vertex_lit_pipeline_layout);
      std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
      DepthConfig depth_config{};
      depth_config.enable_test = true;
      depth_config.enable_write = false;
      depth_config.is_depth_test_dynamic = true;
      vk_create_graphics_pipeline(vk_context->device, app->vertex_lit_pipeline_layout, true, depth_config, {}, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                  primitive_topologies, VK_POLYGON_MODE_FILL, app->vertex_lit_render_pass, &app->vertex_lit_pipeline);

      vk_destroy_shader_module(vk_context->device, frag_shader);
      vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
      VkShaderModule vert_shader, frag_shader;
      vk_create_shader_module(vk_context->device, "shaders/line.vert.spv", &vert_shader);
      vk_create_shader_module(vk_context->device, "shaders/line.frag.spv", &frag_shader);

      VkPushConstantRange push_constant_range{};
      push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      push_constant_range.size = sizeof(InstanceState);

      std::vector<VkDescriptorSetLayout> descriptor_set_layouts{app->global_uniform_buffer_descriptor_set_layout};
      vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->line_pipeline_layout);
      std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP};
      DepthConfig depth_config{};
      depth_config.enable_test = true;
      depth_config.enable_write = false;
      depth_config.is_depth_test_dynamic = true;
      vk_create_graphics_pipeline(vk_context->device, app->line_pipeline_layout, true, depth_config, {}, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                  primitive_topologies, VK_POLYGON_MODE_FILL, app->lit_render_pass, &app->line_pipeline);

      vk_destroy_shader_module(vk_context->device, frag_shader);
      vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
      VkShaderModule vert_shader, frag_shader;
      vk_create_shader_module(vk_context->device, "shaders/entity-picking.vert.spv", &vert_shader);
      vk_create_shader_module(vk_context->device, "shaders/entity-picking.frag.spv", &frag_shader);

      VkPushConstantRange push_constant_range{};
      push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      push_constant_range.size = sizeof(EntityPickingInstanceState);

      std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};
      descriptor_set_layouts.push_back(app->global_uniform_buffer_descriptor_set_layout);
      descriptor_set_layouts.push_back(app->single_storage_buffer_descriptor_set_layout);
      vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->entity_picking_pipeline_layout);
      std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
      DepthConfig depth_config{};
      depth_config.enable_test = true;
      depth_config.enable_write = false;
      depth_config.is_depth_test_dynamic = false;
      vk_create_graphics_pipeline(vk_context->device, app->entity_picking_pipeline_layout, false, depth_config, {}, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                  primitive_topologies, VK_POLYGON_MODE_FILL, app->entity_picking_render_pass, &app->entity_picking_pipeline);

      vk_destroy_shader_module(vk_context->device, frag_shader);
      vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    { // create a checkerboard image
      uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
      uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
      uint32_t pixels[16 * 16]; // 16x16 checkerboard texture
      for (uint8_t x = 0; x < 16; ++x) {
        for (uint8_t y = 0; y < 16; ++y) {
          pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
      }
      vk_create_image(vk_context, {16, 16}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, false, &app->checkerboard_image);
      vk_create_image_view(vk_context->device, app->checkerboard_image->handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &app->checkerboard_image_view);

      Buffer *staging_buffer = nullptr;
      size_t size = 16 * 16 * 4;
      vk_create_buffer_vma(vk_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, &staging_buffer);
      vk_copy_data_to_buffer(vk_context, staging_buffer, pixels, size);

      vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
        vk_cmd_pipeline_image_barrier2(command_buffer, app->checkerboard_image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        vk_cmd_copy_buffer_to_image(command_buffer, staging_buffer->handle, app->checkerboard_image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 16, 16);
        vk_cmd_pipeline_image_barrier2(command_buffer, app->checkerboard_image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT);
      });

      vk_destroy_buffer_vma(vk_context, staging_buffer);
    }

    {
      constexpr size_t width = 8;
      std::array<uint8_t, 32> palette = {
          255, 102, 159, 255, 255, 159, 102, 255, 236, 255, 102, 255, 121, 255, 102, 255,
          102, 255, 198, 255, 102, 198, 255, 255, 121, 102, 255, 255, 236, 102, 255, 255};
      std::vector<uint8_t> texture_data(width * width * 4, 0);
      for (size_t y = 0; y < width; ++y) {
        size_t offset = width * y * 4;
        std::copy(palette.begin(), palette.end(), texture_data.begin() + offset);
        std::rotate(palette.rbegin(), palette.rbegin() + 4, palette.rend()); // 向右旋转4个元素
      }

      vk_create_image(vk_context, {width, width}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, false, &app->uv_debug_image);
      vk_create_image_view(vk_context->device, app->uv_debug_image->handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &app->uv_debug_image_view);

      Buffer *staging_buffer = nullptr;
      size_t size = width * width * 4;
      vk_create_buffer_vma(vk_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, &staging_buffer);
      vk_copy_data_to_buffer(vk_context, staging_buffer, texture_data.data(), size);

      vk_command_buffer_submit(vk_context, [&](VkCommandBuffer command_buffer) {
        vk_cmd_pipeline_image_barrier2(command_buffer, app->uv_debug_image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT);
        vk_cmd_copy_buffer_to_image(command_buffer, staging_buffer->handle, app->uv_debug_image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, width, width);
        vk_cmd_pipeline_image_barrier2(command_buffer, app->uv_debug_image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT);
      });

      vk_destroy_buffer_vma(vk_context, staging_buffer);
    }

    vk_create_sampler(vk_context->device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, &app->sampler_nearest);

    create_camera(&app->camera, glm::vec3(-1.0f, 1.0f, 7.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    create_gizmo(&app->mesh_system_state, vk_context, glm::vec3(0.0f, 1.5f, 0.0f), &app->gizmo);
    // app->gizmo.transform.scale = glm::vec3(glm::length(app->camera.position - app->gizmo.transform.position) * 0.2f);

    // create ui
    // (*app)->gui_context = ImGui::CreateContext();

    {
      Geometry geometry{};
      // load_gltf(app->vk_context, "models/cube.gltf", &app->gltf_model_geometry);
      load_gltf(&app->mesh_system_state, app->vk_context, "models/chinese-dragon.gltf", &geometry);
      // load_gltf(app->vk_context, "models/Fox.glb", &app->gltf_model_geometry);
      // load_gltf(app->vk_context, "models/suzanne/scene.gltf", &app->gltf_model_geometry);
      geometry.transform.position = glm::vec3(0.0f, -3.0f, 0.0f);
      geometry.transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      geometry.transform.scale = glm::vec3(0.25f, 0.25f, 0.25f);
      app->lit_geometries.push_back(geometry);
    }

    {
      Geometry geometry{};
      create_plane_geometry(&app->mesh_system_state, vk_context, 1.5f, 1.0f, &geometry);
      geometry.transform.position = glm::vec3(0.0f, 0.0f, 2.0f);
      app->lit_geometries.push_back(geometry);
    }

    {
      Geometry geometry{};
      create_cube_geometry(&app->mesh_system_state, vk_context, 1.0f, &geometry);
      app->lit_geometries.push_back(geometry);
    }

    {
      Geometry geometry{};
      create_uv_sphere_geometry(&app->mesh_system_state, vk_context, 1, 16, 16, &geometry);
      geometry.transform.position = glm::vec3(0.0f, 0.0f, -5.0f);
      app->lit_geometries.push_back(geometry);
      app->wireframe_geometries.push_back(geometry); // just reference the geometry
    }

    {
      GeometryConfig config{};
      generate_cone_geometry_config_lit(0.5f, 1.0f, 8, &config);
      Geometry geometry{};
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-3.0f, -1.0f, 0.0f);
      app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      generate_solid_circle_geometry_config(glm::vec3(0, 0, 0), true, 0.5f, 16, &config);
      Geometry geometry{};
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-4.0f, 0.0f, 0.0f);
      geometry.transform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    { // add cylinder geometry
      GeometryConfig config{};
      // generate_cylinder_geometry_config(2, 0.5f, 16, &config);
      Geometry geometry{};
      // create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-5.0f, 1.0f, 0.0f);
      // app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      // generate_torus_geometry_config(1.5f, 0.25f, 64, 8, &config);
      Geometry geometry{};
      // create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-4.0f, 1.0f, 2.0f);
      // app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      generate_cylinder_geometry_config(app->gizmo.config.axis_length, app->gizmo.config.axis_radius, 8, &config);
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &app->gizmo.axis_geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      generate_cone_geometry_config_vertex_lit(app->gizmo.config.arrow_radius, app->gizmo.config.arrow_length, 8, &config);
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &app->gizmo.arrow_geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      generate_torus_geometry_config(app->gizmo.config.ring_major_radius, app->gizmo.config.ring_minor_radius, 64, 8, &config);
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &app->gizmo.ring_geometry);
      dispose_geometry_config(&config);
    }
    {
      create_cube_geometry(&app->mesh_system_state, vk_context, app->gizmo.config.cube_size, &app->gizmo.cube_geometry);
    }

    app->frame_count = 0;
    app->selected_mesh_id = UINT32_MAX;
}

static VkSemaphore pop_semaphore_from_pool(App *app) {
  if (app->semaphore_pool.empty()) {
    log_warning("semaphore pool is empty, creating a new one");
    VkSemaphore semaphore = VK_NULL_HANDLE;
    vk_create_semaphore(app->vk_context->device, &semaphore);
    return semaphore;
  }
  VkSemaphore semaphore = app->semaphore_pool.front();
  app->semaphore_pool.pop();
  return semaphore;
}

static void push_semaphore_to_pool(App *app, VkSemaphore semaphore) {
  app->semaphore_pool.push(semaphore);
}

void app_destroy(App *app) {
    vk_wait_idle(app->vk_context);

    destroy_camera(&app->camera);

    for (auto &geometry : app->lit_geometries) { destroy_geometry(&app->mesh_system_state, app->vk_context, &geometry); }
    for (auto &geometry : app->line_geometries) { destroy_geometry(&app->mesh_system_state, app->vk_context, &geometry); }
    destroy_gizmo(&app->gizmo, &app->mesh_system_state, app->vk_context);

    vk_destroy_sampler(app->vk_context->device, app->sampler_nearest);
    vk_destroy_image_view(app->vk_context->device, app->uv_debug_image_view);
    vk_destroy_image(app->vk_context, app->uv_debug_image);
    vk_destroy_image_view(app->vk_context->device, app->checkerboard_image_view);
    vk_destroy_image(app->vk_context, app->checkerboard_image);

    // ImGui::DestroyContext(app->gui_context);
    vk_destroy_pipeline(app->vk_context->device, app->line_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->line_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->entity_picking_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->entity_picking_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->vertex_lit_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->vertex_lit_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->wireframe_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->wireframe_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->lit_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->lit_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->compute_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->compute_pipeline_layout);

    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_combined_image_sampler_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->global_uniform_buffer_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_storage_buffer_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_storage_image_descriptor_set_layout);

    // 清理 gizmo render pass 相关资源 todo 移入类似 on_plugin_clean_up 方法
    for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
      vk_destroy_framebuffer(app->vk_context->device, app->gizmo_framebuffers[i]);
    }
    app->gizmo_framebuffers.clear();

    // 清理 entity picking 相关资源 todo 移入类似 on_plugin_clean_up 方法
    for (uint16_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
      vk_destroy_buffer_vma(app->vk_context, app->entity_picking_storage_buffers[i]);
      vk_destroy_framebuffer(app->vk_context->device, app->entity_picking_framebuffers[i]);
    }
    app->entity_picking_storage_buffers.clear();
    app->entity_picking_framebuffers.clear();
    vk_destroy_render_pass(app->vk_context->device, app->entity_picking_render_pass);

    vk_destroy_render_pass(app->vk_context->device, app->vertex_lit_render_pass);

    vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
    vk_destroy_image(app->vk_context, app->depth_image);

    vk_destroy_image_view(app->vk_context->device, app->color_image_view);
    vk_destroy_image(app->vk_context, app->color_image);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
      vk_destroy_buffer_vma(app->vk_context, app->frames[i].global_uniform_buffer);
        vk_descriptor_allocator_destroy(app->vk_context->device, &app->frames[i].descriptor_allocator);
        vk_destroy_fence(app->vk_context->device, app->frames[i].in_flight_fence);
        vk_destroy_command_pool(app->vk_context->device, app->frames[i].command_pool);
    }
    for (uint16_t i = 0; i < app->vk_context->swapchain_image_count; ++i) { // 归还 semaphore 到 pool
      if (app->present_complete_semaphores[i]) { push_semaphore_to_pool(app, app->present_complete_semaphores[i]); }
      if (app->render_complete_semaphores[i]) { push_semaphore_to_pool(app, app->render_complete_semaphores[i]); }
    }
    app->present_complete_semaphores.clear();
    app->render_complete_semaphores.clear();
    while (!app->semaphore_pool.empty()) { // 由 pool 统一销毁
      vk_destroy_semaphore(app->vk_context->device, app->semaphore_pool.front());
      app->semaphore_pool.pop();
    }

    for (size_t i = 0; i < app->framebuffers.size(); ++i) {
        vk_destroy_framebuffer(app->vk_context->device, app->framebuffers[i]);
    }
    app->framebuffers.clear();
    vk_destroy_render_pass(app->vk_context->device, app->lit_render_pass);
    vk_terminate(app->vk_context);
    delete app->vk_context;
    delete app;
}

glm::fvec2 mouse_positions[FRAMES_IN_FLIGHT] = {};

struct {
  bool up;
  uint64_t frame_count;
} is_mouse_start_up[FRAMES_IN_FLIGHT] = {};

struct {
  Geometry *geometry;
  uint8_t frame_index;
} rotation_sector_geometry{};

Geometry *rotation_sector_geometries_delete_queue[FRAMES_IN_FLIGHT] = {};

void draw_background(const App *app, VkCommandBuffer command_buffer, RenderFrame *frame) {
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline);

    VkDescriptorSet descriptor_set;
    vk_descriptor_allocator_alloc(app->vk_context->device, &frame->descriptor_allocator, app->single_storage_image_descriptor_set_layout, &descriptor_set);

    VkDescriptorImageInfo image_info = vk_descriptor_image_info(VK_NULL_HANDLE, app->color_image_view, VK_IMAGE_LAYOUT_GENERAL);
    VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_set, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &image_info, nullptr);
    vk_update_descriptor_sets(app->vk_context->device, 1, &write_descriptor_set);

    vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline_layout, 1, &descriptor_set);
    vk_cmd_dispatch(command_buffer, std::ceil(app->vk_context->swapchain_extent.width / 16.0), std::ceil(app->vk_context->swapchain_extent.height / 16.0), 1);
}

void draw_world(App *app, VkCommandBuffer command_buffer, RenderFrame *frame) {
  vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent);
  vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent);

  { // lit pipeline
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->lit_pipeline);

    std::vector<VkDescriptorSet> descriptor_sets;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::deque<VkDescriptorImageInfo> image_infos;
    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    {
      vk_copy_data_to_buffer(app->vk_context, frame->global_uniform_buffer, &app->global_state, sizeof(GlobalState));

      vk_descriptor_allocator_alloc(app->vk_context->device, &frame->descriptor_allocator, app->global_uniform_buffer_descriptor_set_layout, &frame->global_uniform_buffer_descriptor_set);
      descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);

      VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_uniform_buffer->handle, sizeof(GlobalState));
      buffer_infos.push_back(descriptor_buffer_info);

      VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
      write_descriptor_sets.push_back(write_descriptor_set);
    }
    {
      VkDescriptorSet descriptor_set;
      vk_descriptor_allocator_alloc(app->vk_context->device, &frame->descriptor_allocator, app->single_combined_image_sampler_descriptor_set_layout, &descriptor_set);
      descriptor_sets.push_back(descriptor_set);

      VkDescriptorImageInfo descriptor_image_info = vk_descriptor_image_info(app->sampler_nearest, app->uv_debug_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      image_infos.push_back(descriptor_image_info);

      VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos.back(), nullptr);
      write_descriptor_sets.push_back(write_descriptor_set);
    }
    vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
    vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->lit_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

    for (const Geometry &geometry : app->lit_geometries) {
      const glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

      for (const Mesh &mesh : geometry.meshes) {
        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
        vk_cmd_push_constants(command_buffer, app->lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
    }
  }

  // { // draw wireframe
  //   vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline);
  //
  //   std::vector<VkDescriptorSet> descriptor_sets;
  //   descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);
  //   vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());
  //
  //   for (const Geometry &geometry : app->wireframe_geometries) {
  //     glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);
  //
  //     for (const Mesh &mesh : geometry.meshes) {
  //       InstanceState instance_state{};
  //       instance_state.model_matrix = model_matrix;
  //       instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
  //       vk_cmd_push_constants(command_buffer, app->wireframe_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
  //       for (const Primitive &primitive : mesh.primitives) {
  //         vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
  //         vk_cmd_draw_indexed(command_buffer, primitive.index_count);
  //       }
  //     }
  //   }
  // }

  { // draw line, aabb
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline);
    vk_cmd_set_primitive_topology(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    vk_cmd_set_depth_test_enable(command_buffer, true);

    {
      std::vector<VkDescriptorSet> descriptor_sets;
      descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);
      vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

      for (const Geometry &geometry : app->line_geometries) {
        const glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

        for (const Mesh &mesh : geometry.meshes) {
          InstanceState instance_state{};
          instance_state.model_matrix = model_matrix;
          instance_state.color = glm::vec4(1, 1, 1, 1);
          instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
          vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
          for (const Primitive &primitive : mesh.primitives) {
            vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
          }
        }
      }
    }

    {
      std::vector<VkDescriptorSet> descriptor_sets;
      descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);
      vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

      for (const Geometry &geometry : app->lit_geometries) {
        if (!is_aabb_valid(geometry.aabb)) {
          continue;
        }
        const glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        instance_state.color = glm::vec4(1, 1, 1, 1);
        instance_state.vertex_buffer_device_address = geometry.aabb_mesh.vertex_buffer_device_address;
        vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : geometry.aabb_mesh.primitives) {
          vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
        }
      }
    }
  }
}

void draw_gizmo(App *app, VkCommandBuffer command_buffer, RenderFrame *frame, uint8_t frame_index) {
  vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent);
  vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent);

  vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->vertex_lit_pipeline);
  vk_cmd_set_depth_test_enable(command_buffer, false);

  std::vector<VkDescriptorSet> descriptor_sets;
  descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);
  vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->vertex_lit_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

  const glm::mat4 gizmo_model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));

  if ((app->gizmo.mode & GIZMO_MODE_ROTATE) == GIZMO_MODE_ROTATE) {
    if (Geometry *geometry = rotation_sector_geometry.geometry; geometry) {
      // log_debug("frame %d frame index %d, rendering rotation sector geometry %p index buffer %p", app->frame_count, frame_index, geometry, geometry->meshes.front().index_buffer->handle);
      for (const Mesh &mesh : geometry->meshes) {
        glm::mat4 model_matrix(1.0f);
        model_matrix = gizmo_model_matrix * model_matrix;

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        instance_state.color = glm::vec4(0.4f, 0.4f, 0.0f, 0.4f);
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
        vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
    }
  }

  { // 绘制旋转环
    for (const Mesh &mesh : app->gizmo.ring_geometry.meshes) {
      { // y axis
        glm::mat4 model_matrix(1.0f);
        model_matrix = gizmo_model_matrix * model_matrix;

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_ROTATE && app->gizmo.axis & GIZMO_AXIS_Y) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
      { // z
        glm::mat4 model_matrix(1.0f);
        model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model_matrix = gizmo_model_matrix * model_matrix;

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_ROTATE && app->gizmo.axis & GIZMO_AXIS_Z) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
      { // x
        glm::mat4 model_matrix(1.0f);
        model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model_matrix = gizmo_model_matrix * model_matrix;

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_ROTATE && app->gizmo.axis & GIZMO_AXIS_X) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
    }
  }

  for (const Mesh &mesh : app->gizmo.axis_geometry.meshes) {
    { // x axis body
      glm::mat4 model_matrix(1.0f);
      model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      model_matrix = gizmo_model_matrix * model_matrix;

      InstanceState instance_state{};
      instance_state.model_matrix = model_matrix;
      glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
      if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_X) {
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
      } else if (app->gizmo.is_active) {
        color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
      }
      instance_state.color = color;
      instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

      vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
      for (const Primitive &primitive : mesh.primitives) {
        vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
        vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      }
    }
    { // y axis
      glm::mat4 model_matrix(1.0f);
      model_matrix = gizmo_model_matrix * model_matrix;

      InstanceState instance_state{};
      instance_state.model_matrix = model_matrix;
      glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
      if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_Y) {
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
      } else if (app->gizmo.is_active) {
        color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
      }
      instance_state.color = color;
      instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

      vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
      for (const Primitive &primitive : mesh.primitives) {
        vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
        vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      }
    }
    { // z axis
      glm::mat4 model_matrix(1.0f);
      model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      model_matrix = gizmo_model_matrix * model_matrix;

      InstanceState instance_state{};
      instance_state.model_matrix = model_matrix;
      glm::vec4 color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
      if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_Z) {
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
      } else if (app->gizmo.is_active) {
        color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
      }
      instance_state.color = color;
      instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

      vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
      for (const Primitive &primitive : mesh.primitives) {
        vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
        vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      }
    }
  }

  {
    for (const Mesh &mesh : app->gizmo.arrow_geometry.meshes) {
      { // x axis
        glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(app->gizmo.config.axis_length, 0.0f, 0.0f));
        model_matrix = glm::rotate(model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_X) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
      { // y axis
        glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(0.0f, app->gizmo.config.axis_length, 0.0f));

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_Y) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
      { // z axis
        glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(0.0f, 0.0f, app->gizmo.config.axis_length));
        model_matrix = glm::rotate(model_matrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        glm::vec4 color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (app->gizmo.mode & GIZMO_MODE_TRANSLATE && app->gizmo.axis & GIZMO_AXIS_Z) {
          color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (app->gizmo.is_active) {
          color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        }
        instance_state.color = color;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
    }
  }

  {
    for (const Mesh &mesh : app->gizmo.cube_geometry.meshes) {
      { // x axis
        glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(app->gizmo.config.axis_length + app->gizmo.config.arrow_length + app->gizmo.config.cube_offset + app->gizmo.config.cube_size * 0.5f, 0.0f, 0.0f));

        InstanceState instance_state{};
        instance_state.model_matrix = model_matrix;
        instance_state.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->vertex_lit_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        for (const Primitive &primitive : mesh.primitives) {
          vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
          vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        }
      }
      //         { // y axis
      //             glm::mat4 model = glm::translate(model_matrix, glm::vec3(0.0f, 1.0f, 0.0f));
      //
      //             InstanceState instance_state{};
      //             instance_state.model_matrix = model;
      //             instance_state.color = glm::vec3(0.0f, 1.0f, 0.0f);
      //             instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
      //
      //             vk_cmd_push_constants(command_buffer, app->gizmo_triangle_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
      //             for (const Primitive &primitive : mesh.primitives) {
      //                 vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
      //                 vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      //             }
      //         }
      //         { // z axis
      //             glm::mat4 model = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, 1.0f));
      //             model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      //
      //             InstanceState instance_state{};
      //             instance_state.model_matrix = model;
      //             instance_state.color = glm::vec3(0.0f, 0.0f, 1.0f);
      //             instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
      //
      //             vk_cmd_push_constants(command_buffer, app->gizmo_triangle_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
      //             for (const Primitive &primitive : mesh.primitives) {
      //                 vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
      //                 vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      //             }
      //         }
    }
  }
}

void draw_gui(const App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {}

void draw_entity_picking(App *app, VkCommandBuffer command_buffer, RenderFrame *frame, uint8_t frame_index) {
  vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent);
  vk_cmd_set_scissor(command_buffer, app->mouse_pos.x, app->mouse_pos.y, {1, 1});

  std::vector<VkDescriptorSet> descriptor_sets{};
  descriptor_sets.push_back(frame->global_uniform_buffer_descriptor_set);
  {
    std::vector<VkWriteDescriptorSet> write_descriptor_sets{};

    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    vk_descriptor_allocator_alloc(app->vk_context->device, &frame->descriptor_allocator, app->single_storage_buffer_descriptor_set_layout, &descriptor_set);
    descriptor_sets.push_back(descriptor_set);

    VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(app->entity_picking_storage_buffers[frame_index]->handle, VK_WHOLE_SIZE);

    VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &descriptor_buffer_info);
    write_descriptor_sets.push_back(write_descriptor_set);

    vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
  }
  vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->entity_picking_pipeline);
  vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->entity_picking_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

  for (const Geometry &geometry : app->lit_geometries) {
    const glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);
    for (const Mesh &mesh : geometry.meshes) {
      EntityPickingInstanceState instance_state{};
      instance_state.model_matrix = model_matrix;
      instance_state.id = mesh.id;
      instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
      vk_cmd_push_constants(command_buffer, app->entity_picking_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EntityPickingInstanceState), &instance_state);
      for (const Primitive &primitive : mesh.primitives) {
        vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
        vk_cmd_draw_indexed(command_buffer, primitive.index_count);
      }
    }
  }
}

void scene_raycast(App *app, const Ray *ray, RaycastHitResult *result) {
    // todo need some sort spatial partitioning to speed this up
    for (const Geometry &geometry : app->lit_geometries) {
        const glm::mat4 model = model_matrix_from_transform(geometry.transform); // todo 不需要每次都计算 model matrix
        if (float distance; raycast_obb(*ray, geometry.aabb, model, distance)) {
            log_debug("hit geometry, distance: %f", distance);
        }
    }
    // sort the hit results by distance
}

void update_scene(App *app) {
    camera_update(&app->camera);

    app->global_state.view_matrix = app->camera.view_matrix;

    const float fov_y = 45.0f;
    const float z_near = 0.05f, z_far = 100.0f;
    const glm::mat4 projection_matrix = glm::perspective(glm::radians(fov_y), (float) app->vk_context->swapchain_extent.width / (float) app->vk_context->swapchain_extent.height, z_near, z_far);
    app->projection_matrix = projection_matrix;

    app->global_state.projection_matrix = clip * projection_matrix;

    app->global_state.sunlight_dir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

  if (app->selected_mesh_id != UINT32_MAX) {
    for (const Geometry &geometry : app->lit_geometries) {
      for (const Mesh &mesh : geometry.meshes) {
        if (mesh.id == app->selected_mesh_id) {
          gizmo_set_position(&app->gizmo, geometry.transform.position);
          break;
        }
      }
    }
  }
  gizmo_set_scale(&app->gizmo, glm::vec3(glm::length(app->camera.position - app->gizmo.transform.position) * 0.2f));
}

bool begin_frame(App *app) { return false; }

void end_frame(App *app) {}

void app_update(App *app, InputSystemState *input_system_state) {
  uint8_t frame_index = app->frame_count % FRAMES_IN_FLIGHT;

  if (app->frame_count >= FRAMES_IN_FLIGHT - 1) {
    RenderFrame *frame = &app->frames[frame_index];
    vk_wait_fence(app->vk_context->device, frame->in_flight_fence, UINT64_MAX);

    if (is_mouse_start_up[frame_index].up && app->frame_count >= is_mouse_start_up[frame_index].frame_count) {
      uint32_t id = 0;
      vk_read_data_from_buffer(app->vk_context, app->entity_picking_storage_buffers[frame_index], &id, sizeof(uint32_t));
      // log_debug("frame %d frame index %d, data: %u (early frame %d)", app->frame_count, app->frame_count % FRAMES_IN_FLIGHT, id, frame_index);
      if (id > 0) { app->selected_mesh_id = id; }
    }

    if (Geometry *geometry = rotation_sector_geometries_delete_queue[frame_index]; geometry) {
      // log_debug("frame %d frame index %d, destroy rotation sector geometry %p index buffer %p", app->frame_count, frame_index, geometry, geometry->meshes.front().index_buffer->handle);
      destroy_geometry_v2(&app->mesh_system_state, app->vk_context, geometry);
      delete geometry;
      rotation_sector_geometries_delete_queue[frame_index] = nullptr;
    }

    // reset frame data before corresponding frame begins
    mouse_positions[frame_index] = {};
    is_mouse_start_up[frame_index] = {};
  }

  update_scene(app);
  RenderFrame *frame = &app->frames[frame_index];
  vk_reset_fence(app->vk_context->device, frame->in_flight_fence);
  frame->global_uniform_buffer_descriptor_set = VK_NULL_HANDLE;
  vk_descriptor_allocator_reset(app->vk_context->device, &frame->descriptor_allocator);
  vk_reset_command_pool(app->vk_context->device, frame->command_pool);
  VkSemaphore present_complete_semaphore = pop_semaphore_from_pool(app);
  uint32_t image_index = UINT32_MAX;
  vk_acquire_next_image(app->vk_context, present_complete_semaphore, &image_index);
  if (app->present_complete_semaphores[image_index]) { push_semaphore_to_pool(app, app->present_complete_semaphores[image_index]); }
  app->present_complete_semaphores[image_index] = present_complete_semaphore;
  // log_debug("frame %lld frame index %d, image index %d", app->frame_count, frame_index, image_index);
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  vk_alloc_command_buffers(app->vk_context->device, frame->command_pool, 1, &command_buffer);
  vk_begin_one_flight_command_buffer(command_buffer);

  // 清理颜色附件（在初始布局为 UNDEFINED 时，必须保证颜色附件已准备好用于写入）
  vk_cmd_pipeline_image_barrier2(command_buffer,
                                app->vk_context->swapchain_images[image_index],
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_ACCESS_2_NONE,
                                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
  vk_cmd_pipeline_image_barrier2(command_buffer,
                                 app->depth_image->handle,
                                 VK_IMAGE_ASPECT_DEPTH_BIT,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                 VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                 VK_ACCESS_2_NONE,
                                 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  {
    VkClearValue clear_values[2] = {};
    clear_values[0].color = {.float32 = {0.2f, 0.1f, 0.1f, 0.2f}};
    clear_values[1].depthStencil = {1.0f, 0};
    vk_begin_render_pass(command_buffer, app->lit_render_pass, app->framebuffers[image_index], app->vk_context->swapchain_extent, clear_values, 2);
    draw_world(app, command_buffer, frame);
    vk_end_render_pass(command_buffer);
  }
  vk_cmd_pipeline_image_barrier2(command_buffer,
                                 app->depth_image->handle,
                                 VK_IMAGE_ASPECT_DEPTH_BIT,
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
  vk_cmd_pipeline_buffer_barrier2(command_buffer,
                                  app->entity_picking_storage_buffers[frame_index]->handle,
                                  0,
                                  VK_WHOLE_SIZE,
                                  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                  VK_ACCESS_2_SHADER_WRITE_BIT,
                                  VK_ACCESS_2_SHADER_WRITE_BIT);
  {
    vk_clear_buffer(app->vk_context, app->entity_picking_storage_buffers[frame_index], sizeof(uint32_t));

    vk_begin_render_pass(command_buffer, app->entity_picking_render_pass, app->entity_picking_framebuffers[frame_index], app->vk_context->swapchain_extent, nullptr, 0);
    draw_entity_picking(app, command_buffer, frame, frame_index);
    vk_end_render_pass(command_buffer);
  }
  {
    vk_begin_render_pass(command_buffer, app->vertex_lit_render_pass, app->gizmo_framebuffers[image_index], app->vk_context->swapchain_extent, nullptr, 0);
    draw_gizmo(app, command_buffer, frame, frame_index);
    vk_end_render_pass(command_buffer);
  }

  vk_end_command_buffer(command_buffer);
  if (app->render_complete_semaphores[image_index] == VK_NULL_HANDLE) { app->render_complete_semaphores[image_index] = pop_semaphore_from_pool(app); }
  vk_queue_submit(app->vk_context->graphics_queue, command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, present_complete_semaphore, app->render_complete_semaphores[image_index], frame->in_flight_fence);
  VkResult result = vk_queue_present(app->vk_context, app->render_complete_semaphores[image_index], image_index);
  ASSERT(result == VK_SUCCESS);
  mouse_positions[frame_index] = app->mouse_pos;
  ++app->frame_count;

  // update_scene(app);
    //
    // frame_index = app->frame_count % FRAMES_IN_FLIGHT;
    //
    // RenderFrame *frame = &app->frames[frame_index];
    //
    // vk_wait_fence(app->vk_context->device, frame->in_flight_fence, UINT64_MAX);
    // vk_reset_fence(app->vk_context->device, frame->in_flight_fence);
    //
    // frame->global_uniform_buffer_descriptor_set = VK_NULL_HANDLE;
    // vk_descriptor_allocator_reset(app->vk_context->device, &frame->descriptor_allocator);
    // vk_reset_command_pool(app->vk_context->device, frame->command_pool);
    //
    // uint32_t image_index;
    // VkResult result = vk_acquire_next_image(app->vk_context, frame->present_complete_semaphore, &image_index);
    // ASSERT(result == VK_SUCCESS);
    //
    // VkImage swapchain_image = app->vk_context->swapchain_images[image_index];
    //
    // // log_debug("frame %lld, frame index %d, image index %d", app->frame_count, frame_index, image_index);
    //
    // VkCommandBuffer command_buffer = frame->command_buffer;
    // {
    //     vk_begin_one_flight_command_buffer(command_buffer);
    //
    //     vk_transition_image_layout(command_buffer, app->color_image->image,
    //                                VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
    //                                VK_PIPELINE_STAGE_2_BLIT_BIT, // could be in layout transition or computer shader writing or blit operation of current frame or previous frame
    //                                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    //                                VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT |
    //                                VK_ACCESS_2_TRANSFER_READ_BIT,
    //                                VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
    //                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    //
    //     draw_background(app, command_buffer, frame);
    //
    //     vk_transition_image_layout(command_buffer, app->color_image->image, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    //     VkRenderingAttachmentInfo color_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    //     color_attachment.imageView = app->color_image_view;
    //     color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //     color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    //     color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //
    //     VkRenderingAttachmentInfo depth_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    //     depth_attachment.imageView = app->depth_image_view;
    //     depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    //     depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //     depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //     depth_attachment.clearValue.depthStencil.depth = 1.0f;
    //
    //     VkOffset2D offset = {.x = 0, .y = 0};
    //     VkExtent2D extent = app->vk_context->swapchain_extent;
    //     vk_cmd_begin_rendering(command_buffer, offset, extent, &color_attachment, 1, &depth_attachment);
    //     draw_world(app, command_buffer, frame);
    //     draw_gizmo(app, command_buffer, frame);
    //     // draw_gui(app, command_buffer, frame);
    //     vk_cmd_end_rendering(command_buffer);
    //
    //     if (was_mouse_button_down(input_system_state, MOUSE_BUTTON_LEFT) && is_mouse_button_up(input_system_state, MOUSE_BUTTON_LEFT)) {
    //       VkRenderingAttachmentInfo color_attachment_info = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    //       color_attachment_info.imageView = app->entity_picking_color_image_view;
    //       color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //       color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //       color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //
    //       VkRenderingAttachmentInfo depth_attachment_info = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    //       depth_attachment_info.imageView = app->object_picking_depth_image_view;
    //       depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    //       depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //       depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //       depth_attachment_info.clearValue.depthStencil.depth = 1.0f;
    //
    //       offset.x = app->mouse_pos[0];
    //       offset.y = app->mouse_pos[1];
    //       extent.width = 1;
    //       extent.height = 1;
    //       vk_cmd_begin_rendering(command_buffer, offset, extent, &color_attachment_info, 1, &depth_attachment_info);
    //       draw_entity_picking(app, command_buffer, frame);
    //       vk_cmd_end_rendering(command_buffer);
    //
    //       vk_transition_image_layout(command_buffer, app->entity_picking_color_image->image, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //       vk_cmd_copy_image_to_buffer(command_buffer, app->entity_picking_color_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, offset, extent, app->object_picking_buffer->handle);
    //       vk_cmd_pipeline_barrier(command_buffer, app->object_picking_buffer->handle, 0, sizeof(uint32_t), VK_PIPELINE_STAGE_2_COPY_BIT_KHR, VK_PIPELINE_STAGE_2_COPY_BIT_KHR, VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR, VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR);
    //       vk_transition_image_layout(command_buffer, app->entity_picking_color_image->image, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    //       app->clicked = true;
    //     }
    //
    //     vk_transition_image_layout(command_buffer, app->color_image->image, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    //
    //     vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //
    //     vk_cmd_blit_image(command_buffer, app->color_image->image, swapchain_image, &app->vk_context->swapchain_extent);
    //
    //     vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    //
    //     vk_end_command_buffer(command_buffer);
    // }
    //
    // VkSemaphoreSubmitInfo wait_semaphore = vk_semaphore_submit_info(frame->present_complete_semaphore, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    // VkSemaphoreSubmitInfo signal_semaphore = vk_semaphore_submit_info(frame->render_complete_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    // VkCommandBufferSubmitInfo command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    // VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    // vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);
    //
    // if (app->clicked) {
    //   vk_wait_fence(app->vk_context->device, frame->in_flight_fence, UINT64_MAX);
    //
    //   uint32_t id = 0;
    //   vk_read_data_from_buffer(app->vk_context, app->object_picking_buffer, &id, sizeof(uint32_t));
    //   if (id > 0) {
    //     app->selected_mesh_id = id;
    //     log_debug("mesh id: %u", id);
    //   }
    //
    //   app->clicked = false;
    // }
    //
    // result = vk_queue_present(app->vk_context, frame->render_complete_semaphore, image_index);
    // ASSERT(result == VK_SUCCESS);
    //
    // ++app->frame_count;
}

void app_resize(App *app, uint32_t width, uint32_t height) {
  vk_resize(app->vk_context, width, height);

  for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
    vk_destroy_fence(app->vk_context->device, app->frames[i].in_flight_fence);
    vk_create_fence(app->vk_context->device, true, &app->frames[i].in_flight_fence);
    vk_reset_command_pool(app->vk_context->device, app->frames[i].command_pool);
  }
  for (uint16_t i = 0; i < app->vk_context->swapchain_image_count; ++i) { // 归还 semaphore 到 pool
    if (app->present_complete_semaphores[i]) { push_semaphore_to_pool(app, app->present_complete_semaphores[i]); }
    if (app->render_complete_semaphores[i]) { push_semaphore_to_pool(app, app->render_complete_semaphores[i]); }
    app->present_complete_semaphores[i] = VK_NULL_HANDLE;
    app->render_complete_semaphores[i] = VK_NULL_HANDLE;
  }

  vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
  vk_destroy_image(app->vk_context, app->depth_image);

  vk_destroy_image_view(app->vk_context->device, app->color_image_view);
  vk_destroy_image(app->vk_context, app->color_image);

  for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
    vk_destroy_framebuffer(app->vk_context->device, app->framebuffers[i]);
  }
  app->framebuffers.clear();

  create_depth_image(app, VK_FORMAT_D32_SFLOAT);
  app->framebuffers.resize(app->vk_context->swapchain_image_count);
  for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
    VkImageView attachments[2] = {app->vk_context->swapchain_image_views[i], app->depth_image_view};
    vk_create_framebuffer(app->vk_context->device, app->vk_context->swapchain_extent, app->lit_render_pass, attachments, 2, &app->framebuffers[i]);
  }

  create_color_image(app, VK_FORMAT_R16G16B16A16_SFLOAT);

  { // 按序清理各 render pass 资源
    // 清理 gizmo render pass 相关资源 todo 移入类似 on_plugin_clean_up 方法
    for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
      vk_destroy_framebuffer(app->vk_context->device, app->gizmo_framebuffers[i]);
    }
    app->gizmo_framebuffers.clear();

    // 清理 entity picking 相关资源 todo 移入类似 on_plugin_clean_up 方法
    for (uint16_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
      vk_destroy_buffer_vma(app->vk_context, app->entity_picking_storage_buffers[i]);
      vk_destroy_framebuffer(app->vk_context->device, app->entity_picking_framebuffers[i]);
    }
    app->entity_picking_storage_buffers.clear();
    app->entity_picking_framebuffers.clear();
  }

  { // 按序重建各 render pass 资源
    // 重建 entity picking 相关资源 todo 移入类似 on_plugin_prepare 方法
    app->entity_picking_framebuffers.resize(FRAMES_IN_FLIGHT);
    app->entity_picking_storage_buffers.resize(FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
      {
        VkImageView attachments[1] = {app->depth_image_view};
        vk_create_framebuffer(app->vk_context->device, app->vk_context->swapchain_extent, app->entity_picking_render_pass, attachments, 1, &app->entity_picking_framebuffers[i]);
      }

      size_t storage_buffer_size = sizeof(uint32_t); // size of one pixel
      vk_create_buffer_vma(app->vk_context, storage_buffer_size,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                           VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                           &app->entity_picking_storage_buffers[i]);
    }

    // 重建 gizmo render pass 相关资源 todo 移入类似 on_plugin_prepare 方法
    app->gizmo_framebuffers.resize(app->vk_context->swapchain_image_count);
    for (size_t i = 0; i < app->vk_context->swapchain_image_count; ++i) {
      VkImageView attachments[2] = {app->vk_context->swapchain_image_views[i], app->depth_image_view};
      vk_create_framebuffer(app->vk_context->device, app->vk_context->swapchain_extent, app->vertex_lit_render_pass, attachments, 2, &app->gizmo_framebuffers[i]);
    }
  }

  app->mouse_pos = glm::vec2(0.0f);
  memset(mouse_positions, 0, sizeof(mouse_positions));
  app->frame_count = 0;
}

void app_key_down(App *app, Key key) {
    if (key == KEY_W) {
        Camera *camera = &app->camera;
        camera_forward(camera, 0.2f);
    } else if (key == KEY_S) {
        Camera *camera = &app->camera;
        camera_backward(camera, 0.2f);
    } else if (key == KEY_A) {
        Camera *camera = &app->camera;
        camera_left(camera, 0.2f);
    } else if (key == KEY_D) {
        Camera *camera = &app->camera;
        camera_right(camera, 0.2f);
    } else if (key == KEY_Q) {
        Camera *camera = &app->camera;
        camera_up(camera, 0.2f);
    } else if (key == KEY_E) {
        Camera *camera = &app->camera;
        camera_down(camera, 0.2f);
    } else if (key == KEY_UP) {
        Camera *camera = &app->camera;
        camera_pitch(camera, 2.0f);
    } else if (key == KEY_DOWN) {
        Camera *camera = &app->camera;
        camera_pitch(camera, -2.0f);
    } else if (key == KEY_LEFT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, 2.0f);
    } else if (key == KEY_RIGHT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, -2.0f);
    }
}

void app_key_up(App *app, Key key) {
    if (key == KEY_W) {
        Camera *camera = &app->camera;
        camera_forward(camera, 0.2f);
    } else if (key == KEY_S) {
        Camera *camera = &app->camera;
        camera_backward(camera, 0.2f);
    } else if (key == KEY_A) {
        Camera *camera = &app->camera;
        camera_left(camera, 0.2f);
    } else if (key == KEY_D) {
        Camera *camera = &app->camera;
        camera_right(camera, 0.2f);
    } else if (key == KEY_Q) {
        Camera *camera = &app->camera;
        camera_up(camera, 0.2f);
    } else if (key == KEY_E) {
        Camera *camera = &app->camera;
        camera_down(camera, 0.2f);
    } else if (key == KEY_UP) {
        Camera *camera = &app->camera;
        camera_pitch(camera, 2.0f);
    } else if (key == KEY_DOWN) {
        Camera *camera = &app->camera;
        camera_pitch(camera, -2.0f);
    } else if (key == KEY_LEFT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, 2.0f);
    } else if (key == KEY_RIGHT) {
        Camera *camera = &app->camera;
        camera_yaw(camera, -2.0f);
    }
}

void app_mouse_button_down(App *app, MouseButton mouse_button, float x, float y) {
  // log_debug("frame %d frame index %d, mouse button down: %d, %f, %f", app->frame_count, app->frame_count % FRAMES_IN_FLIGHT, mouse_button, x, y);
    if (mouse_button != MOUSE_BUTTON_LEFT) {
        return;
    }
  app->mouse_pos_down = glm::fvec2(x, y);

  const Ray ray = ray_from_screen(app->vk_context->swapchain_extent, x, y, app->camera.position, app->camera.view_matrix, app->projection_matrix);
  const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));

  if (app->gizmo.mode & GIZMO_MODE_TRANSLATE) {
    glm::vec4 n(0.0f);
    if (app->gizmo.axis & GIZMO_AXIS_X) {
      n = glm::normalize(model_matrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
    } else if (app->gizmo.axis & GIZMO_AXIS_Y) {
      n = glm::normalize(model_matrix * glm::vec4(camera_backward_dir(app->camera), 0.0f));
    } else if (app->gizmo.axis & GIZMO_AXIS_Z) {
      n = glm::normalize(model_matrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    } else {
      ASSERT(false);
    }
    app->gizmo.intersection_plane = create_plane(app->gizmo.transform.position, glm::vec3(n));
    bool hit = raycast_plane(ray, app->gizmo.intersection_plane, app->gizmo.intersection_position);
    ASSERT(hit);
    app->gizmo.is_active = true;
  } else if (app->gizmo.mode & GIZMO_MODE_ROTATE) {
    // 记录旋转起点，即被激活的旋转轴的圆心到射线与该旋转轴所在平面的交点形成的向量与该旋转轴的交点

    Ray ray_in_model_space{};
    transform_ray_to_model_space(&ray, model_matrix, &ray_in_model_space);

    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 center = glm::vec3(0.0f);
    if (app->gizmo.axis & GIZMO_AXIS_X) {
      normal = glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (app->gizmo.axis & GIZMO_AXIS_Y) {
      normal = glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (app->gizmo.axis & GIZMO_AXIS_Z) {
      normal = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (glm::dot(glm::normalize(ray_in_model_space.origin - center), normal) < 0.0f) {
      normal = -normal;
    }

    app->gizmo.rotation_plane_normal = normal;

    const glm::vec3 center_to_ray_origin = ray_in_model_space.origin - center; // 从圆心指向射线起点的向量
    const float denom = glm::dot(ray_in_model_space.direction, normal); // 射线方向与圆环法向量点积

    glm::vec3 point_on_plane;
    float t = -glm::dot(center_to_ray_origin, normal) / denom;
    point_on_plane = ray_in_model_space.origin + t * ray_in_model_space.direction; // 射线方程

    app->gizmo.rotation_start_pos = center + glm::normalize(point_on_plane - center) * app->gizmo.config.ring_major_radius;
    if (app->selected_mesh_id != UINT32_MAX) {
      for (Geometry &geometry : app->lit_geometries) {
        for (Mesh &mesh : geometry.meshes) {
          if (mesh.id == app->selected_mesh_id) {
            app->selected_mesh_transform = geometry.transform;
            break;
          }
        }
      }
    }

    app->gizmo.is_active = true;
  }
}

void gizmo_check_ray(App *app, const Ray *ray) {
  const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));

  Ray ray_in_model_space{};
  transform_ray_to_model_space(ray, model_matrix, &ray_in_model_space);

  // reset gizmo states
  app->gizmo.mode = GIZMO_MODE_NONE;
  app->gizmo.axis = GIZMO_AXIS_NONE;

  {
    float min_distance = 0.03f;
    const Axis axes[3] = {
        {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length},
        {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length},
        {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length},
    };
    for (size_t i = 0; i < 3; ++i) {
      float t, s;
      if (const float d = ray_axis_shortest_distance(ray_in_model_space, axes[i], t, s); d < min_distance) {
        min_distance = d;
        app->gizmo.axis = (GizmoAxis) (1 << i);
      }
    }
    if (app->gizmo.axis != GIZMO_AXIS_NONE) {
      app->gizmo.mode = GIZMO_MODE_TRANSLATE;
      return;
    }
  }

  {
    float min_distance = 0.03f;
    const StrokeCircle circles[3] = {
        {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), app->gizmo.config.ring_major_radius},
        {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), app->gizmo.config.ring_major_radius},
        {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), app->gizmo.config.ring_major_radius},
    };
    for (size_t i = 0; i < 3; ++i) {
      // 根据射线与平面的夹角，动态计算触发距离
      const float theta = glm::acos(glm::dot(glm::normalize(circles[i].normal), glm::normalize(-ray_in_model_space.direction)));
      float padding;
      if (glm::degrees(theta) < 90.0f) {
        padding = glm::max(glm::tan(theta) * 0.03f, 0.03f);
      } else if (glm::degrees(theta) > 90.0f) {
        const float beta = glm::radians(90.0f - (glm::degrees(theta) - 90.0f));
        padding = glm::min(glm::max(glm::tan(beta) * 0.03f, 0.03f), 0.06f);
      } else {
        app->gizmo.axis = (GizmoAxis) (1 << i);
        break;
      }
      if (const float d = raycast_ring(ray_in_model_space, circles[i].center, circles[i].normal, app->gizmo.config.ring_major_radius - padding, app->gizmo.config.ring_major_radius + padding); d < min_distance) {
        min_distance = d;
        app->gizmo.axis = (GizmoAxis) (1 << i);
      }
    }
    if (app->gizmo.axis != GIZMO_AXIS_NONE) {
      app->gizmo.mode = GIZMO_MODE_ROTATE;
      return;
    }
  }

  {
    // todo scale
  }
}

void app_mouse_button_up(App *app, MouseButton mouse_button, float x, float y) {
  // log_debug("frame %d frame index %d, mouse button %d up at screen position (%f, %f)", app->frame_count, app->frame_count % FRAMES_IN_FLIGHT, mouse_button, x, y);
  if (mouse_button != MOUSE_BUTTON_LEFT) {
    return;
  }
  {
    const Ray ray = ray_from_screen(app->vk_context->swapchain_extent, x, y, app->camera.position, app->camera.view_matrix, app->projection_matrix);

    Vertex vertices[2];
    memcpy(vertices[0].position, glm::value_ptr(ray.origin), sizeof(float) * 3);
    memcpy(vertices[1].position, glm::value_ptr(ray.origin + ray.direction * 100.0f), sizeof(float) * 3);
    Geometry geometry{};
    create_geometry(&app->mesh_system_state, app->vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), nullptr, 0, 0, nullptr, &geometry);
    app->line_geometries.push_back(geometry);
  }

  // reset gizmo runtime data
  app->gizmo.is_active = false;
  if (app->gizmo.mode & GIZMO_MODE_ROTATE) {
    app->gizmo.is_rotation_clock_dir_locked = false;
    app->gizmo.rotation_clock_dir = '0';
    app->gizmo.rotation_start_pos = glm::vec3(FLT_MAX);

    if (rotation_sector_geometry.geometry) {
      rotation_sector_geometries_delete_queue[rotation_sector_geometry.frame_index] = rotation_sector_geometry.geometry;
    }
    rotation_sector_geometry.geometry = nullptr;
  }
  app->gizmo.axis = GIZMO_AXIS_NONE; // todo remove this line

  app->mouse_pos_down = glm::fvec2(-1.0f, -1.0f);
  is_mouse_start_up[app->frame_count % FRAMES_IN_FLIGHT] = {.up = true, .frame_count = app->frame_count};
}

void app_mouse_move(App *app, float x, float y) {
  app->mouse_pos = glm::fvec2(x, y);
  const Ray ray = ray_from_screen(app->vk_context->swapchain_extent, x, y, app->camera.position, app->camera.view_matrix, app->projection_matrix);
  if (app->gizmo.is_active) {
    if (app->gizmo.mode & GIZMO_MODE_TRANSLATE) {
      const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));
      glm::vec3 intersection_position(0.0f);
      bool hit = raycast_plane(ray, app->gizmo.intersection_plane, intersection_position);
      ASSERT(hit);
      if (app->gizmo.axis & GIZMO_AXIS_X) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      if (app->gizmo.axis & GIZMO_AXIS_Y) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      if (app->gizmo.axis & GIZMO_AXIS_Z) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      app->gizmo.intersection_position = intersection_position;

      if (app->selected_mesh_id != UINT32_MAX) {
        for (Geometry &geometry : app->lit_geometries) {
          for (Mesh &mesh : geometry.meshes) {
            if (mesh.id == app->selected_mesh_id) {
              geometry.transform.position = app->gizmo.transform.position;
              break;
            }
          }
        }
      }
    } else if (app->gizmo.mode & GIZMO_MODE_ROTATE) {
      // 记录旋转当前点

      const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));
      Ray ray_in_model_space{};
      transform_ray_to_model_space(&ray, model_matrix, &ray_in_model_space);

      glm::vec3 center = glm::vec3(0.0f);

      const glm::vec3 center_to_ray_origin = ray_in_model_space.origin - center; // 从圆心指向射线起点的向量
      const float denom = glm::dot(ray_in_model_space.direction, app->gizmo.rotation_plane_normal); // 射线方向与圆环法向量点积

      glm::vec3 point_on_plane;
      float t = -glm::dot(center_to_ray_origin, app->gizmo.rotation_plane_normal) / denom;
      point_on_plane = ray_in_model_space.origin + t * ray_in_model_space.direction; // 射线方程

      app->gizmo.rotation_end_pos = center + glm::normalize(point_on_plane - center) * app->gizmo.config.ring_major_radius;

      // 计算旋转方向

      const glm::vec3 norm_start = glm::normalize(app->gizmo.rotation_start_pos);
      const glm::vec3 norm_end = glm::normalize(app->gizmo.rotation_end_pos);
      const float dot = glm::dot(glm::cross(norm_start, norm_end), app->gizmo.rotation_plane_normal);
      float angle = glm::acos(glm::dot(norm_start, norm_end));

      if (app->gizmo.is_rotation_clock_dir_locked) {
        if ((app->gizmo.rotation_clock_dir == 'C' ? dot > 0.0f : dot < 0.0f) && angle < glm::half_pi<float>()) {
          app->gizmo.is_rotation_clock_dir_locked = false;
        }
      }
      if (!app->gizmo.is_rotation_clock_dir_locked) {
        app->gizmo.rotation_clock_dir = dot > 0.0f ? 'C' : 'c';
        if ((app->gizmo.rotation_clock_dir == 'C' ? dot > 0.0f : dot < 0.0f) && angle > glm::half_pi<float>()) {
          app->gizmo.is_rotation_clock_dir_locked = true;
        }
      }

      {
        GeometryConfig config{};
        generate_sector_geometry_config(app->gizmo.rotation_plane_normal, app->gizmo.rotation_start_pos, app->gizmo.rotation_end_pos, app->gizmo.rotation_clock_dir, 64, &config);
        Geometry *geometry = new Geometry();
        create_geometry_from_config_v2(&app->mesh_system_state, app->vk_context, &config, geometry);
        dispose_geometry_config(&config);
        if (rotation_sector_geometry.geometry) {
          rotation_sector_geometries_delete_queue[rotation_sector_geometry.frame_index] = rotation_sector_geometry.geometry;
        }
        rotation_sector_geometry.geometry = geometry;
        rotation_sector_geometry.frame_index = app->frame_count % FRAMES_IN_FLIGHT;
        // log_debug("frame %d frame index %d, rotation sector geometry created %p index buffer %p", app->frame_count, app->frame_count % FRAMES_IN_FLIGHT, geometry, geometry->meshes.front().index_buffer->handle);
      }

      if (app->selected_mesh_id != UINT32_MAX) {
        for (Geometry &geometry : app->lit_geometries) {
          for (Mesh &mesh : geometry.meshes) {
            if (mesh.id == app->selected_mesh_id) {
              if (app->gizmo.rotation_clock_dir == 'C') {
                if (dot < 0.0f) {
                  angle = 2 * glm::pi<float>() - angle;
                }
              } else if (app->gizmo.rotation_clock_dir == 'c') {
                angle = -angle;
                if (dot > 0.0f) {
                  angle = -2 * glm::pi<float>() - angle;
                }
              }
              glm::quat rotation = glm::angleAxis(angle, app->gizmo.rotation_plane_normal);
              geometry.transform.rotation = rotation * app->selected_mesh_transform.rotation;
              break;
            }
          }
        }
      }
    }
  } else {
    gizmo_check_ray(app, &ray);
  }
}

void app_capture(App *app) {}
