#include "app.h"
#include "core/deletion_queue.h"
#include "core/logging.h"
#include <glm/ext/scalar_reciprocal.hpp>
#include <glm/gtc/type_ptr.inl>
#include <glm/gtx/string_cast.hpp>
#include "mesh_loader.h"
#include "vk.h"
#include "vk_buffer.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_context.h"
#include "vk_descriptor.h"
#include "vk_descriptor_allocator.h"
#include "vk_fence.h"
#include "vk_image.h"
#include "vk_image_view.h"
#include "vk_pipeline.h"
#include "vk_queue.h"
#include "vk_sampler.h"
#include "vk_semaphore.h"
#include "vk_swapchain.h"

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

Ray ray_from_screen(const glm::vec2 &screen_pos, const VkExtent2D &viewport_extent, const glm::vec3 &origin, const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) {
    // fire a ray from `origin` to the world position of the `screen_pos`
    const glm::vec3 near_pos = screen_position_to_world_position(projection_matrix, view_matrix, viewport_extent.width, viewport_extent.height, 0.0f, screen_pos.x, screen_pos.y);
    Ray ray{};
    ray.origin = origin;
    ray.direction = glm::normalize(near_pos - origin);
    return ray;
}

void create_color_image(App *app, const VkFormat format) {
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_STORAGE_BIT /* can be written by computer shader */;
    vk_create_image(app->vk_context, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height, format, usage, false, &app->color_image);
    vk_create_image_view(app->vk_context->device, app->color_image->image, format, VK_IMAGE_ASPECT_COLOR_BIT, app->color_image->mip_levels, &app->color_image_view);
}

void create_depth_image(App *app, const VkFormat format) {
    vk_create_image(app->vk_context, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, false, &app->depth_image);
    vk_create_image_view(app->vk_context->device, app->depth_image->image, format, VK_IMAGE_ASPECT_DEPTH_BIT, app->depth_image->mip_levels, &app->depth_image_view);

    // transition depth image layout once and for all
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    vk_alloc_command_buffers(app->vk_context->device, app->vk_context->command_pool, 1, &command_buffer);

    vk_begin_one_flight_command_buffer(command_buffer);
    vk_transition_image_layout(command_buffer, app->depth_image->image,
                               VK_PIPELINE_STAGE_2_NONE,
                               VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                               VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                               VK_ACCESS_2_NONE,
                               VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                               VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    vk_end_command_buffer(command_buffer);

    VkFence fence;
    vk_create_fence(app->vk_context->device, false, &fence);

    vk_queue_submit(app->vk_context->graphics_queue, command_buffer, fence);

    vk_wait_fence(app->vk_context->device, fence);
    vk_destroy_fence(app->vk_context->device, fence);

    vk_free_command_buffer(app->vk_context->device, app->vk_context->command_pool, command_buffer);
}

void create_object_picking_color_image(App *app) {
  constexpr VkFormat format = VK_FORMAT_R32_UINT;
  VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  vk_create_image(app->vk_context, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height, format, usage, false, &app->object_picking_color_image);
  vk_create_image_view(app->vk_context->device, app->object_picking_color_image->image, format, VK_IMAGE_ASPECT_COLOR_BIT, app->object_picking_color_image->mip_levels, &app->object_picking_color_image_view);

  vk_command_buffer_submit(app->vk_context, [&](VkCommandBuffer command_buffer) {
    vk_transition_image_layout(command_buffer, app->object_picking_color_image->image, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  });
}

void create_object_picking_depth_image(App *app, const VkFormat format) {
  vk_create_image(app->vk_context, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, false, &app->object_picking_depth_image);
  vk_create_image_view(app->vk_context->device, app->object_picking_depth_image->image, format, VK_IMAGE_ASPECT_DEPTH_BIT, app->object_picking_depth_image->mip_levels, &app->object_picking_depth_image_view);

  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  vk_alloc_command_buffers(app->vk_context->device, app->vk_context->command_pool, 1, &command_buffer);

  vk_begin_one_flight_command_buffer(command_buffer);
  vk_transition_image_layout(command_buffer, app->object_picking_depth_image->image,
                             VK_PIPELINE_STAGE_2_NONE,
                             VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                             VK_ACCESS_2_NONE,
                             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  vk_end_command_buffer(command_buffer);

  VkFence fence;
  vk_create_fence(app->vk_context->device, false, &fence);

  vk_queue_submit(app->vk_context->graphics_queue, command_buffer, fence);

  vk_wait_fence(app->vk_context->device, fence);
  vk_destroy_fence(app->vk_context->device, fence);

  vk_free_command_buffer(app->vk_context->device, app->vk_context->command_pool, command_buffer);
}

void create_object_picking_staging_buffer(App *app) {
  constexpr size_t buffer_size = 4; // size of one pixel
  vk_create_buffer(app->vk_context, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &app->object_picking_buffer);
}

void app_create(SDL_Window *window, App **out_app) {
    int render_width, render_height;
    SDL_GetWindowSizeInPixels(window, &render_width, &render_height);

    App *app = new App();
    app->window = window;
    app->vk_context = new VkContext();

    vk_init(app->vk_context, window, render_width, render_height);

    VkContext *vk_context = app->vk_context;

    ASSERT(FRAMES_IN_FLIGHT <= vk_context->swapchain_image_count);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        RenderFrame *frame = &app->frames[i];
        vk_create_command_pool(vk_context->device, vk_context->graphics_queue_family_index, &frame->command_pool);
        vk_alloc_command_buffers(vk_context->device, app->frames[i].command_pool, 1, &frame->command_buffer);

        vk_create_fence(vk_context->device, true, &frame->in_flight_fence);
        vk_create_semaphore(vk_context->device, &frame->image_acquired_semaphore);
        vk_create_semaphore(vk_context->device, &frame->render_finished_semaphore);

        uint32_t max_sets = 5; // 计算着色器一个 set，lit pipeline 中 shader 两个 set，wireframe pipeline 一个 set，line pipeline 一个 set
        std::vector<DescriptorPoolSizeRatio> size_ratios;
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3});
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
        vk_descriptor_allocator_create(vk_context->device, max_sets, size_ratios, &frame->descriptor_allocator);

        vk_create_buffer(vk_context, sizeof(GlobalState), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &frame->global_state_buffer);
    }

    VkFormat color_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    create_color_image(app, color_image_format);
    create_object_picking_color_image(app);

    VkFormat depth_image_format = VK_FORMAT_D32_SFLOAT;
    create_depth_image(app, depth_image_format);
    create_object_picking_depth_image(app, depth_image_format);

    create_object_picking_staging_buffer(app);

    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->single_storage_image_descriptor_set_layout);
    }
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
        vk_create_descriptor_set_layout(vk_context->device, bindings, &app->global_state_descriptor_set_layout);
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
        descriptor_set_layouts[0] = app->global_state_descriptor_set_layout;
        descriptor_set_layouts[1] = app->single_combined_image_sampler_descriptor_set_layout;
        vk_create_pipeline_layout(vk_context->device, 2, descriptor_set_layouts, &push_constant_range, &app->lit_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        vk_create_graphics_pipeline(vk_context->device, app->lit_pipeline_layout, color_image_format, true, true, false, true, true, depth_image_format,
                                    {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, primitive_topologies, VK_POLYGON_MODE_FILL, &app->lit_pipeline);

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
        descriptor_set_layouts[0] = app->global_state_descriptor_set_layout;
        vk_create_pipeline_layout(vk_context->device, 1, descriptor_set_layouts, &push_constant_range, &app->wireframe_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        vk_create_graphics_pipeline(vk_context->device, app->wireframe_pipeline_layout, color_image_format, true, true /* endable deep test */, false, false /* disable deep write */, false, depth_image_format,
                                    {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, primitive_topologies, VK_POLYGON_MODE_LINE, &app->wireframe_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/gizmo.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/gizmo.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(InstanceState);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts{app->global_state_descriptor_set_layout};
        vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->gizmo_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        vk_create_graphics_pipeline(vk_context->device, app->gizmo_pipeline_layout, color_image_format, true, true, true, false, false, depth_image_format, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                    primitive_topologies, VK_POLYGON_MODE_FILL, &app->gizmo_pipeline);

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

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts{app->global_state_descriptor_set_layout};
        vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->line_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP};
        vk_create_graphics_pipeline(vk_context->device, app->line_pipeline_layout, color_image_format, true, true, true, false, false, depth_image_format, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                    primitive_topologies, VK_POLYGON_MODE_FILL, &app->line_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    {
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/object-picking.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/object-picking.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant_range.size = sizeof(ObjectPickingInstanceState);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts{app->global_state_descriptor_set_layout};
        vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->object_picking_pipeline_layout);
        std::vector<VkPrimitiveTopology> primitive_topologies{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        vk_create_graphics_pipeline(vk_context->device, app->object_picking_pipeline_layout, VK_FORMAT_R32_UINT, false, true, true, true, false, depth_image_format, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                    primitive_topologies, VK_POLYGON_MODE_FILL, &app->object_picking_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    { // create checkerboard image
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
        uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        uint32_t pixels[16 * 16]; // 16x16 checkerboard texture
        for (uint8_t x = 0; x < 16; ++x) {
            for (uint8_t y = 0; y < 16; ++y) {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }
        vk_create_image_from_data(vk_context, pixels, 16, 16, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false, &app->checkerboard_image);
        vk_create_image_view(vk_context->device, app->checkerboard_image->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, &app->checkerboard_image_view);
    }

    {
        constexpr size_t texture_size = 8;
        std::array<uint8_t, 32> palette = {
            255, 102, 159, 255, 255, 159, 102, 255, 236, 255, 102, 255, 121, 255, 102, 255,
            102, 255, 198, 255, 102, 198, 255, 255, 121, 102, 255, 255, 236, 102, 255, 255};
        std::vector<uint8_t> texture_data(texture_size * texture_size * 4, 0);
        for (size_t y = 0; y < texture_size; ++y) {
            size_t offset = texture_size * y * 4;
            std::copy(palette.begin(), palette.end(), texture_data.begin() + offset);
            std::rotate(palette.rbegin(), palette.rbegin() + 4, palette.rend()); // 向右旋转4个元素
        }

        vk_create_image_from_data(vk_context, texture_data.data(), texture_size, texture_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false, &app->uv_debug_image);
        vk_create_image_view(vk_context->device, app->uv_debug_image->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, &app->uv_debug_image_view);
    }

    vk_create_sampler(vk_context->device, VK_FILTER_NEAREST, VK_FILTER_NEAREST, &app->default_sampler_nearest);

    create_camera(&app->camera, glm::vec3(-1.0f, 1.0f, 7.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    create_gizmo(&app->mesh_system_state, vk_context, glm::vec3(0.0f, 1.5f, 0.0f), &app->gizmo);
    app->gizmo.transform.scale = glm::vec3(glm::length(app->camera.position - app->gizmo.transform.position) * 0.2f);

    // create ui
    // (*app)->gui_context = ImGui::CreateContext();

    Geometry gltf_model_geometry{};
    // load_gltf(app->vk_context, "models/cube.gltf", &app->gltf_model_geometry);
    load_gltf(&app->mesh_system_state, app->vk_context, "models/chinese-dragon.gltf", &gltf_model_geometry);
    // load_gltf(app->vk_context, "models/Fox.glb", &app->gltf_model_geometry);
    // load_gltf(app->vk_context, "models/suzanne/scene.gltf", &app->gltf_model_geometry);
    gltf_model_geometry.transform.position = glm::vec3(0.0f, -3.0f, 0.0f);
    gltf_model_geometry.transform.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
    gltf_model_geometry.transform.scale = glm::vec3(0.25f, 0.25f, 0.25f);
    app->lit_geometries.push_back(gltf_model_geometry);

    Geometry plane_geometry{};
    create_plane_geometry(&app->mesh_system_state, vk_context, 1.5f, 1.0f, &plane_geometry);
    plane_geometry.transform.position = glm::vec3(0.0f, 0.0f, 2.0f);
    app->lit_geometries.push_back(plane_geometry);

    Geometry cube_geometry{};
    create_cube_geometry(&app->mesh_system_state, vk_context, 1.0f, &cube_geometry);
    app->lit_geometries.push_back(cube_geometry);

    Geometry uv_sphere_geometry{};
    create_uv_sphere_geometry(&app->mesh_system_state, vk_context, 1, 16, 16, &uv_sphere_geometry);
    uv_sphere_geometry.transform.position = glm::vec3(0.0f, 0.0f, -5.0f);
    app->lit_geometries.push_back(uv_sphere_geometry);
    app->wireframe_geometries.push_back(uv_sphere_geometry); // just reference the same geometry

    { // create a cone geometry
        // GeometryConfig config{};
        // generate_cone_geometry_config(0.5f, 1.0f, 8, 1, &config);
        // Geometry geometry;
        // create_geometry_from_config(vk_context, &config, &geometry);
        // cone_geometry.position = glm::vec3(-3.0f, 0.0f, 0.0f);
        // app->lit_geometries.push_back(cone_geometry);
        // dispose_geometry_config(&config);

        float radius = 0.5f;
        float height = 1.0f;
        uint32_t sector = 8;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        const float sector_step = 2 * glm::pi<float>() / (float) sector;

        // base circle

        { // center vertex
            Vertex vertex{};
            vertex.position[0] = 0.0f;
            vertex.position[1] = 0.0f;
            vertex.position[2] = 0.0f;
            vertex.tex_coord[0] = 0.5f;
            vertex.tex_coord[1] = 0.5f;
            vertex.normal[0] = 0.0f;
            vertex.normal[1] = -1.0f;
            vertex.normal[2] = 0.0f;
            vertices.push_back(vertex);
        }

        for (size_t i = 0; i <= sector; ++i) {
            float sector_angle = i * sector_step;
            float a = cos(sector_angle);
            float b = sin(sector_angle);
            Vertex vertex{};
            vertex.position[0] = a * radius; // x
            vertex.position[1] = 0.0f; // y
            vertex.position[2] = b * radius; // z
            vertex.tex_coord[0] = a * 0.5f + 0.5f;
            vertex.tex_coord[1] = b * 0.5f + 0.5f;
            vertex.normal[0] = 0.0f;
            vertex.normal[1] = -1.0f;
            vertex.normal[2] = 0.0f;
            vertices.push_back(vertex);
        }

        for (size_t i = 0; i < sector; ++i) {
            indices.push_back(0);
            indices.push_back(i + 1);
            indices.push_back(i + 2);
        }

        // side

        uint32_t base_index = vertices.size();
        for (size_t i = 0; i < sector; ++i) {
            /*  2
             * /  \
             * 0 - 1
             */
            float sector_angle = i * sector_step;

            float face_angle = sector_angle + sector_step * 0.5f;
            float alpha = atan(radius / height); // 斜面与 Y 轴的夹角
            glm::vec3 normal;
            normal.x = sin(alpha) * height * cos(alpha) * cos(face_angle);
            normal.y = sin(alpha) * height * sin(alpha);
            normal.z = -sin(alpha) * height * cos(alpha) * sin(face_angle);
            normal = glm::normalize(normal);

            { // 0
                Vertex vertex{};
                vertex.position[0] = cos(sector_angle) * radius; // x
                vertex.position[1] = 0.0f; // y
                vertex.position[2] = -sin(sector_angle) * radius; // z
                vertex.tex_coord[0] = (float) i / (float) sector;
                vertex.tex_coord[1] = 0.0f;
                vertex.normal[0] = normal.x;
                vertex.normal[1] = normal.y;
                vertex.normal[2] = normal.z;
                vertices.push_back(vertex);
            }
            { // 1
                Vertex vertex{};
                vertex.position[0] = cos(sector_angle + sector_step) * radius; // x
                vertex.position[1] = 0.0f; // y
                vertex.position[2] = -sin(sector_angle + sector_step) * radius; // z
                vertex.tex_coord[0] = (float) (i + 1) / (float) sector;
                vertex.tex_coord[1] = 0.0f;
                vertex.normal[0] = normal.x;
                vertex.normal[1] = normal.y;
                vertex.normal[2] = normal.z;
                vertices.push_back(vertex);
            }
            { // 2
                Vertex vertex{};
                vertex.position[0] = 0.0f; // x
                vertex.position[1] = height; // y
                vertex.position[2] = 0.0f; // z
                vertex.tex_coord[0] = ((float) i / (float) sector + (float) (i + 1) / (float) sector) * 0.5f;
                vertex.tex_coord[1] = 1.0f;
                vertex.normal[0] = normal.x;
                vertex.normal[1] = normal.y;
                vertex.normal[2] = normal.z;
                vertices.push_back(vertex);
            }
        }

        for (size_t i = 0; i < sector; ++i) {
            indices.push_back(base_index + i * 3);
            indices.push_back(base_index + i * 3 + 1);
            indices.push_back(base_index + i * 3 + 2);
        }

        AABB aabb{};
        generate_aabb_from_vertices(vertices.data(), vertices.size(), &aabb);

        Geometry geometry{};
        create_geometry(&app->mesh_system_state, vk_context, vertices.data(), vertices.size(), sizeof(Vertex), indices.data(), indices.size(), sizeof(uint32_t), aabb, &geometry);
        geometry.transform.position = glm::vec3(-3.0f, -1.0f, 0.0f);
        app->lit_geometries.push_back(geometry);
    }

    {
        GeometryConfig config{};
        generate_solid_circle_geometry_config(glm::vec3(0, 0, 0), true, 0.5f, 16, &config);
        Geometry geometry{};
        create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
        geometry.transform.position = glm::vec3(-4.0f, 0.0f, 0.0f);
        geometry.transform.rotation = glm::vec3(90.0f, 0.0f, 0.0f);
        app->lit_geometries.push_back(geometry);
        dispose_geometry_config(&config);
    }

    { // add cylinder geometry
      GeometryConfig config{};
      generate_cylinder_geometry_config(2, 0.5f, 16, &config);
      Geometry geometry{};
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-5.0f, 1.0f, 0.0f);
      app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    { // add torus geometry
      GeometryConfig config{};
      generate_torus_geometry_config(1.5f, 0.25f, 64, 8, &config);
      Geometry geometry{};
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &geometry);
      geometry.transform.position = glm::vec3(-4.0f, 1.0f, 2.0f);
      app->lit_geometries.push_back(geometry);
      dispose_geometry_config(&config);
    }

    {
      GeometryConfig config{};
      generate_cylinder_geometry_config(app->gizmo.config.axis_length, app->gizmo.config.axis_radius, 8, &config);
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &app->gizmo.axis_geometry);
      dispose_geometry_config(&config);
    }

    {
        const float radius = app->gizmo.config.arrow_radius;
        const float height = app->gizmo.config.arrow_length;
        constexpr uint32_t sector = 8;

        std::vector<ColoredVertex> vertices;
        std::vector<uint32_t> indices;

        const float sector_step = 2 * glm::pi<float>() / (float) sector;

        // base circle

        { // center vertex
            ColoredVertex vertex{};
            vertex.position = glm::vec3(0.0f);
            vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertices.push_back(vertex);
        }

        for (size_t i = 0; i <= sector; ++i) {
            float sector_angle = i * sector_step;
            float a = cos(sector_angle);
            float b = sin(sector_angle);
            ColoredVertex vertex{};
            vertex.position = glm::vec3(a * radius, 0.0f, b * radius);
            vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertices.push_back(vertex);
        }

        for (size_t i = 0; i < sector; ++i) {
            indices.push_back(0);
            indices.push_back(i + 1);
            indices.push_back(i + 2);
        }

        // side

        uint32_t base_index = vertices.size();
        for (size_t i = 0; i < sector; ++i) {
            /*  2
             * /  \
             * 0 - 1
             */
            float sector_angle = i * sector_step;

            float face_angle = sector_angle + sector_step * 0.5f;
            float alpha = atan(radius / height); // 斜面与 Y 轴的夹角
            glm::vec3 normal;
            normal.x = sin(alpha) * height * cos(alpha) * cos(face_angle);
            normal.y = sin(alpha) * height * sin(alpha);
            normal.z = -sin(alpha) * height * cos(alpha) * sin(face_angle);
            normal = glm::normalize(normal);

            { // 0
                ColoredVertex vertex{};
                vertex.position = glm::vec3(cos(sector_angle) * radius, 0.0f, -sin(sector_angle) * radius);
                vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                vertex.normal = normal;
                vertices.push_back(vertex);
            }
            { // 1
                ColoredVertex vertex{};
                vertex.position = glm::vec3(cos(sector_angle + sector_step) * radius, 0.0f, -sin(sector_angle + sector_step) * radius);
                vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                vertex.normal = normal;
                vertices.push_back(vertex);
            }
            { // 2
                ColoredVertex vertex{};
                vertex.position = glm::vec3(0.0f, height, 0.0f);
                vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                vertex.normal = normal;
                vertices.push_back(vertex);
            }
        }

        for (size_t i = 0; i < sector; ++i) {
            indices.push_back(base_index + i * 3);
            indices.push_back(base_index + i * 3 + 1);
            indices.push_back(base_index + i * 3 + 2);
        }

        AABB aabb{};
        for (size_t i = 0; i < 4; ++i) {
            const ColoredVertex &vertex = vertices[i];
            aabb.min = glm::min(aabb.min, vertex.position);
            aabb.max = glm::max(aabb.max, vertex.position);
        }

        create_geometry(&app->mesh_system_state, vk_context, vertices.data(), vertices.size(), sizeof(ColoredVertex), indices.data(), indices.size(), sizeof(uint32_t), aabb, &app->gizmo.arrow_geometry);
    }

    {
      GeometryConfig config{};
      generate_torus_geometry_config(app->gizmo.config.ring_major_radius, app->gizmo.config.ring_minor_radius, 64, 8, &config);
      create_geometry_from_config(&app->mesh_system_state, vk_context, &config, &app->gizmo.ring_geometry);
      dispose_geometry_config(&config);
    }
    {
        create_cube_geometry(&app->mesh_system_state, vk_context, 0.06f, &app->gizmo.cube_geometry);
    }
    {
      const float radius = app->gizmo.config.ring_major_radius;
      constexpr uint32_t sector = 16;

      std::vector<ColoredVertex> vertices;
      std::vector<uint32_t> indices;

      const float sector_step = glm::pi<float>() / (float) sector;

      { // center vertex
          ColoredVertex vertex{};
          vertex.position = glm::vec3(0.0f);
          vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
          vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
          vertices.push_back(vertex);
      }

      for (size_t i = 0; i <= sector; ++i) {
          float sector_angle = i * sector_step;
          float a = cos(sector_angle);
          float b = sin(sector_angle);
          ColoredVertex vertex{};
          vertex.position = glm::vec3(a * radius, 0.0f, b * radius);
          vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
          vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
          vertices.push_back(vertex);
      }

      for (size_t i = 0; i < sector; ++i) {
          indices.push_back(0);
          indices.push_back(i + 1);
          indices.push_back(i + 2);
      }

      AABB aabb{};
      for (size_t i = 0; i < 4; ++i) {
        const ColoredVertex &vertex = vertices[i];
        aabb.min = glm::min(aabb.min, vertex.position);
        aabb.max = glm::max(aabb.max, vertex.position);
      }

      create_geometry(&app->mesh_system_state, vk_context, vertices.data(), vertices.size(), sizeof(ColoredVertex), indices.data(), indices.size(), sizeof(uint32_t), aabb, &app->gizmo.sector_geometry);
    }

    app->frame_number = 0;
    memset(app->mouse_pos, -1.0f, sizeof(float) * 2);
    app->selected_mesh_id = UINT32_MAX;

    *out_app = app;
}

void app_destroy(App *app) {
    vk_wait_idle(app->vk_context);

    destroy_camera(&app->camera);

    for (auto &geometry : app->lit_geometries) { destroy_geometry(&app->mesh_system_state, app->vk_context, &geometry); }
    for (auto &geometry : app->line_geometries) { destroy_geometry(&app->mesh_system_state, app->vk_context, &geometry); }
    destroy_gizmo(&app->gizmo, &app->mesh_system_state, app->vk_context);

    vk_destroy_sampler(app->vk_context->device, app->default_sampler_nearest);
    vk_destroy_image_view(app->vk_context->device, app->uv_debug_image_view);
    vk_destroy_image(app->vk_context, app->uv_debug_image);
    vk_destroy_image_view(app->vk_context->device, app->checkerboard_image_view);
    vk_destroy_image(app->vk_context, app->checkerboard_image);

    // ImGui::DestroyContext(app->gui_context);
    vk_destroy_pipeline(app->vk_context->device, app->line_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->line_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->object_picking_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->object_picking_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->gizmo_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->gizmo_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->wireframe_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->wireframe_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->lit_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->lit_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->compute_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->compute_pipeline_layout);

    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_combined_image_sampler_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->global_state_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_storage_image_descriptor_set_layout);

    vk_destroy_buffer(app->vk_context, app->object_picking_buffer);

    vk_destroy_image_view(app->vk_context->device, app->object_picking_depth_image_view);
    vk_destroy_image(app->vk_context, app->object_picking_depth_image);

    vk_destroy_image_view(app->vk_context->device, app->object_picking_color_image_view);
    vk_destroy_image(app->vk_context, app->object_picking_color_image);

    vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
    vk_destroy_image(app->vk_context, app->depth_image);

    vk_destroy_image_view(app->vk_context->device, app->color_image_view);
    vk_destroy_image(app->vk_context, app->color_image);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_destroy_buffer(app->vk_context, app->frames[i].global_state_buffer);
        vk_descriptor_allocator_destroy(app->vk_context->device, app->frames[i].descriptor_allocator);
        vk_destroy_semaphore(app->vk_context->device, app->frames[i].render_finished_semaphore);
        vk_destroy_semaphore(app->vk_context->device, app->frames[i].image_acquired_semaphore);
        vk_destroy_fence(app->vk_context->device, app->frames[i].in_flight_fence);
        vk_destroy_command_pool(app->vk_context->device, app->frames[i].command_pool);
    }

    vk_terminate(app->vk_context);
    delete app->vk_context;
    delete app;
}

void draw_background(const App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline);

    VkDescriptorSet descriptor_set;
    vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->single_storage_image_descriptor_set_layout, &descriptor_set);

    VkDescriptorImageInfo image_info = vk_descriptor_image_info(VK_NULL_HANDLE, app->color_image_view, VK_IMAGE_LAYOUT_GENERAL);
    VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_set, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &image_info, nullptr);
    vk_update_descriptor_sets(app->vk_context->device, 1, &write_descriptor_set);

    vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_pipeline_layout, 1, &descriptor_set);
    vk_cmd_dispatch(command_buffer, std::ceil(app->vk_context->swapchain_extent.width / 16.0), std::ceil(app->vk_context->swapchain_extent.height / 16.0), 1);
}

void draw_world(App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
    vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
    vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);

    { // lit pipeline
        vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->lit_pipeline);

        vk_cmd_set_depth_bias(command_buffer, 1.0f, 0.0f, .5f); // 在非 reversed-z 情况下，使物体离相机更远

        std::vector<VkDescriptorSet> descriptor_sets; // todo 1）提前预留空间，防止 resize 导致被其他地方引用的原有元素失效；2）如何释放这些 descriptor_set
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::deque<VkDescriptorImageInfo> image_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            vk_copy_data_to_buffer(app->vk_context, frame->global_state_buffer, &app->global_state, sizeof(GlobalState));

            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->single_combined_image_sampler_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorImageInfo descriptor_image_info = vk_descriptor_image_info(app->default_sampler_nearest, app->uv_debug_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            image_infos.push_back(descriptor_image_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos.back(), nullptr);
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->lit_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Geometry &geometry : app->lit_geometries) {
            glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

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

    { // wireframe pipeline
        vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline);

        std::vector<VkDescriptorSet> descriptor_sets; // todo 1）提前预留空间，防止 resize 导致被其他地方引用的原有元素失效；2）如何释放这些 descriptor_set
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Geometry &geometry : app->wireframe_geometries) {
            glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

            for (const Mesh &mesh : geometry.meshes) {
                InstanceState instance_state{};
                instance_state.model_matrix = model_matrix;
                instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

                vk_cmd_push_constants(command_buffer, app->wireframe_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

                for (const Primitive &primitive : mesh.primitives) {
                    vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
                    vk_cmd_draw_indexed(command_buffer, primitive.index_count);
                }
            }
        }
    }

    { // draw aabbs
        vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline);
        vk_cmd_set_primitive_topology(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        vk_cmd_set_depth_test_enable(command_buffer, true);

        std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Geometry &geometry : app->lit_geometries) {
            glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

            InstanceState instance_state{};
            instance_state.model_matrix = model_matrix;
            instance_state.color = glm::vec4(1, 1, 1, 1);
            instance_state.vertex_buffer_device_address = geometry.aabb_mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
            vk_cmd_bind_vertex_buffer(command_buffer, geometry.aabb_mesh.vertex_buffer->handle, 0);
            for (const Primitive &primitive : geometry.aabb_mesh.primitives) {
                vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
            }
        }
    }

    { // draw lines
        vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline);
        vk_cmd_set_primitive_topology(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        vk_cmd_set_depth_test_enable(command_buffer, true);

        std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->line_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Geometry &geometry : app->line_geometries) {
            glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);

            for (const Mesh &mesh : geometry.meshes) {
                InstanceState instance_state{};
                instance_state.model_matrix = model_matrix;
                instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

                vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
                vk_cmd_bind_vertex_buffer(command_buffer, mesh.vertex_buffer->handle, 0);
                for (const Primitive &primitive : mesh.primitives) {
                    vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
                }
            }
        }
    }
}

void draw_gizmo(App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
    vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
    vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);

    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline);
    vk_cmd_set_depth_test_enable(command_buffer, false);

    std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    {
        VkDescriptorSet descriptor_set;
        vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
        descriptor_sets.push_back(descriptor_set);

        VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
        buffer_infos.push_back(descriptor_buffer_info);

        VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
        write_descriptor_sets.push_back(write_descriptor_set);
    }
    vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
    vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

    const glm::mat4 gizmo_model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));

    {
      std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
      std::deque<VkDescriptorBufferInfo> buffer_infos;
      std::vector<VkWriteDescriptorSet> write_descriptor_sets;
      {
        VkDescriptorSet descriptor_set;
        vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
        descriptor_sets.push_back(descriptor_set);

        VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
        buffer_infos.push_back(descriptor_buffer_info);

        VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
        write_descriptor_sets.push_back(write_descriptor_set);
      }
      vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
      vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

      for (const Mesh &mesh : app->gizmo.ring_geometry.meshes) {
        { // y
          glm::mat4 model_matrix(1.0f);
          model_matrix = gizmo_model_matrix * model_matrix;

          InstanceState state{};
          state.model_matrix = model_matrix;
          state.color = (app->gizmo.active_axis & GIZMO_AXIS_Y) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
          state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

          vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &state);

          for (const Primitive &primitive : mesh.primitives) {
            vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
            vk_cmd_draw_indexed(command_buffer, primitive.index_count);
          }
        }
        { // z
          glm::mat4 model_matrix(1.0f);
          model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
          model_matrix = gizmo_model_matrix * model_matrix;

          InstanceState state{};
          state.model_matrix = model_matrix;
          state.color = (app->gizmo.action & GIZMO_AXIS_Z) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
          state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

          vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &state);

          for (const Primitive &primitive : mesh.primitives) {
            vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
            vk_cmd_draw_indexed(command_buffer, primitive.index_count);
          }
        }
        { // x
          glm::mat4 model_matrix(1.0f);
          model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
          model_matrix = gizmo_model_matrix * model_matrix;

          InstanceState state{};
          state.model_matrix = model_matrix;
          state.color = (app->gizmo.active_axis & GIZMO_AXIS_X) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
          state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

          vk_cmd_push_constants(command_buffer, app->line_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &state);

          for (const Primitive &primitive : mesh.primitives) {
            vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
            vk_cmd_draw_indexed(command_buffer, primitive.index_count);
          }
        }
      }
    }

    {
      std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
      std::deque<VkDescriptorBufferInfo> buffer_infos;
      std::vector<VkWriteDescriptorSet> write_descriptor_sets;
      {
        VkDescriptorSet descriptor_set;
        vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
        descriptor_sets.push_back(descriptor_set);

        VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
        buffer_infos.push_back(descriptor_buffer_info);

        VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
        write_descriptor_sets.push_back(write_descriptor_set);
      }
      vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
      vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

      for (const Mesh &mesh : app->gizmo.sector_geometry.meshes) {
        // {
        //   glm::mat4 model_matrix(1.0f);
        //   model_matrix = gizmo_model_matrix * model_matrix;
        //
        //   InstanceState instance_state{};
        //   instance_state.model_matrix = model_matrix;
        //   instance_state.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);
        //   instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;
        //
        //   vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        //   for (const Primitive &primitive : mesh.primitives) {
        //     vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
        //     vk_cmd_draw_indexed(command_buffer, primitive.index_count);
        //   }
        // }
        {
          glm::mat4 model_matrix(1.0f);
          model_matrix = gizmo_model_matrix * model_matrix;

          InstanceState instance_state{};
          instance_state.model_matrix = model_matrix;
          instance_state.color = glm::vec4(0.8f, 0.0f, 0.0f, 0.8f);
          instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

          vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
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
            instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_X) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
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
            instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_Y) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
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
            instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_Z) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
            for (const Primitive &primitive : mesh.primitives) {
              vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
              vk_cmd_draw_indexed(command_buffer, primitive.index_count);
            }
        }
    }

    {
        std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Mesh &mesh : app->gizmo.arrow_geometry.meshes) {
            { // x axis
                glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(app->gizmo.config.axis_length, 0.0f, 0.0f));
                model_matrix = glm::rotate(model_matrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

                InstanceState instance_state{};
                instance_state.model_matrix = model_matrix;
                instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_X) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

                vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
                for (const Primitive &primitive : mesh.primitives) {
                  vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
                    vk_cmd_draw_indexed(command_buffer, primitive.index_count);
                }
            }
            { // y axis
                glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(0.0f, app->gizmo.config.axis_length, 0.0f));

                InstanceState instance_state{};
                instance_state.model_matrix = model_matrix;
                instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_Y) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
                instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

                vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
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
                instance_state.color = (app->gizmo.active_axis & GIZMO_AXIS_Z) ? glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

                vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
                for (const Primitive &primitive : mesh.primitives) {
                    vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
                    vk_cmd_draw_indexed(command_buffer, primitive.index_count);
                }
            }
        }
    }

    {
        std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
        std::deque<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        {
            VkDescriptorSet descriptor_set;
            vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
            descriptor_sets.push_back(descriptor_set);

            VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
            buffer_infos.push_back(descriptor_buffer_info);

            VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
            write_descriptor_sets.push_back(write_descriptor_set);
        }
        vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

        for (const Mesh &mesh : app->gizmo.cube_geometry.meshes) {
        { // x axis
            glm::mat4 model_matrix = glm::translate(gizmo_model_matrix, glm::vec3(1.382f, 0.0f, 0.0f));

            InstanceState instance_state{};
            instance_state.model_matrix = model_matrix;
            instance_state.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
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

void pick_object(App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
  vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
  vk_cmd_set_scissor(command_buffer, app->mouse_pos[0], app->mouse_pos[1], 1, 1);

  std::vector<VkDescriptorSet> descriptor_sets; // todo 1）提前预留空间，防止 resize 导致被其他地方引用的原有元素失效；2）如何释放这些 descriptor_set
  std::deque<VkDescriptorBufferInfo> buffer_infos;
  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  {
    VkDescriptorSet descriptor_set;
    vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
    descriptor_sets.push_back(descriptor_set);

    VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer->handle, sizeof(GlobalState));
    buffer_infos.push_back(descriptor_buffer_info);

    VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
    write_descriptor_sets.push_back(write_descriptor_set);
  }
  vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
  vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->object_picking_pipeline);
  vk_cmd_set_depth_test_enable(command_buffer, true);
  vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->object_picking_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

  for (const Geometry &geometry : app->lit_geometries) {
    const glm::mat4 model_matrix = model_matrix_from_transform(geometry.transform);
    for (const Mesh &mesh : geometry.meshes) {
      ObjectPickingInstanceState instance_state{};
      instance_state.model_matrix = model_matrix;
      instance_state.id = mesh.id;
      instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

      vk_cmd_push_constants(command_buffer, app->object_picking_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectPickingInstanceState), &instance_state);

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

  if (app->selected_mesh_id > 0) {
    for (const Geometry &geometry : app->lit_geometries) {
      for (const Mesh &mesh : geometry.meshes) {
        if (mesh.id == app->selected_mesh_id) {
          app->gizmo.transform.position = geometry.transform.position;
          break;
        }
      }
    }
  }
  // app->gizmo.transform.scale = glm::vec3(glm::length(app->camera.position - app->gizmo.transform.position) * 0.2f);
}

bool begin_frame(App *app) { return false; }

void end_frame(App *app) {}

void app_update(App *app, InputSystemState *input_system_state) {
    update_scene(app);

    app->frame_index = app->frame_number % FRAMES_IN_FLIGHT;
    // log_debug("frame_index: %u", app->frame_index);

    RenderFrame *frame = &app->frames[app->frame_index];

    vk_wait_fence(app->vk_context->device, frame->in_flight_fence);
    vk_reset_fence(app->vk_context->device, frame->in_flight_fence);

    vk_descriptor_allocator_reset(app->vk_context->device, frame->descriptor_allocator);

    uint32_t image_index;
    VkResult result = vk_acquire_next_image(app->vk_context, frame->image_acquired_semaphore, &image_index);
    ASSERT(result == VK_SUCCESS);

    VkImage swapchain_image = app->vk_context->swapchain_images[image_index];

    // log_debug("frame %lld, frame index %d, image index %d", app->frame_number, app->frame_index, image_index);

    VkCommandBuffer command_buffer = frame->command_buffer;
    {
        vk_begin_one_flight_command_buffer(command_buffer);

        vk_transition_image_layout(command_buffer, app->color_image->image,
                                   VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                                   VK_PIPELINE_STAGE_2_BLIT_BIT, // could be in layout transition or computer shader writing or blit operation of current frame or previous frame
                                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                   VK_ACCESS_2_TRANSFER_READ_BIT,
                                   VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        draw_background(app, command_buffer, frame);

        vk_transition_image_layout(command_buffer, app->color_image->image, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo color_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        color_attachment.imageView = app->color_image_view;
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depth_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depth_attachment.imageView = app->depth_image_view;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.clearValue.depthStencil.depth = 1.0f;

        VkOffset2D offset = {.x = 0, .y = 0};
        VkExtent2D extent = app->vk_context->swapchain_extent;
        vk_cmd_begin_rendering(command_buffer, offset, extent, &color_attachment, 1, &depth_attachment);
        draw_world(app, command_buffer, frame);
        draw_gizmo(app, command_buffer, frame);
        // draw_gui(app, command_buffer, frame);
        vk_cmd_end_rendering(command_buffer);

        if (was_mouse_button_down(input_system_state, MOUSE_BUTTON_LEFT) && is_mouse_button_up(input_system_state, MOUSE_BUTTON_LEFT)) {
          VkRenderingAttachmentInfo color_attachment_info = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
          color_attachment_info.imageView = app->object_picking_color_image_view;
          color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
          color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

          VkRenderingAttachmentInfo depth_attachment_info = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
          depth_attachment_info.imageView = app->object_picking_depth_image_view;
          depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
          depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
          depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
          depth_attachment_info.clearValue.depthStencil.depth = 1.0f;

          offset.x = app->mouse_pos[0];
          offset.y = app->mouse_pos[1];
          extent.width = 1;
          extent.height = 1;
          vk_cmd_begin_rendering(command_buffer, offset, extent, &color_attachment_info, 1, &depth_attachment_info);
          pick_object(app, command_buffer, frame);
          vk_cmd_end_rendering(command_buffer);

          vk_transition_image_layout(command_buffer, app->object_picking_color_image->image, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
          vk_cmd_copy_image_to_buffer(command_buffer, app->object_picking_color_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, offset, extent, app->object_picking_buffer->handle);
          vk_cmd_pipeline_barrier(command_buffer, app->object_picking_buffer->handle, 0, sizeof(uint32_t), VK_PIPELINE_STAGE_2_COPY_BIT_KHR, VK_PIPELINE_STAGE_2_COPY_BIT_KHR, VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR, VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR);
          vk_transition_image_layout(command_buffer, app->object_picking_color_image->image, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          app->clicked = true;
        }

        vk_transition_image_layout(command_buffer, app->color_image->image, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk_cmd_blit_image(command_buffer, app->color_image->image, swapchain_image, &app->vk_context->swapchain_extent);

        vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vk_end_command_buffer(command_buffer);
    }

    VkSemaphoreSubmitInfo wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkSemaphoreSubmitInfo signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkCommandBufferSubmitInfo command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

    if (app->clicked) {
      vk_wait_fence(app->vk_context->device, frame->in_flight_fence);

      uint32_t id = 0;
      vk_read_data_from_buffer(app->vk_context, app->object_picking_buffer, &id, sizeof(uint32_t));
      if (id > 0) {
        app->selected_mesh_id = id;
        log_debug("mesh id: %u", id);
      }

      app->clicked = false;
    }

    // end frame
    result = vk_queue_present(app->vk_context, image_index, frame->render_finished_semaphore);
    ASSERT(result == VK_SUCCESS);

    ++app->frame_number;
}

void app_resize(App *app, uint32_t width, uint32_t height) {
    vk_resize(app->vk_context, width, height);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        vk_destroy_semaphore(app->vk_context->device, app->frames[i].render_finished_semaphore);
        vk_destroy_semaphore(app->vk_context->device, app->frames[i].image_acquired_semaphore);
        vk_destroy_fence(app->vk_context->device, app->frames[i].in_flight_fence);

        vk_create_semaphore(app->vk_context->device, &app->frames[i].image_acquired_semaphore);
        vk_create_semaphore(app->vk_context->device, &app->frames[i].render_finished_semaphore);
        vk_create_fence(app->vk_context->device, true, &app->frames[i].in_flight_fence);

        vk_reset_command_pool(app->vk_context->device, app->frames[i].command_pool);
    }

    vk_destroy_buffer(app->vk_context, app->object_picking_buffer);

    vk_destroy_image_view(app->vk_context->device, app->object_picking_depth_image_view);
    vk_destroy_image(app->vk_context, app->object_picking_depth_image);

    vk_destroy_image_view(app->vk_context->device, app->object_picking_color_image_view);
    vk_destroy_image(app->vk_context, app->object_picking_color_image);

    vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
    vk_destroy_image(app->vk_context, app->depth_image);

    vk_destroy_image_view(app->vk_context->device, app->color_image_view);
    vk_destroy_image(app->vk_context, app->color_image);

    create_color_image(app, VK_FORMAT_R16G16B16A16_SFLOAT);
    create_object_picking_color_image(app);
    create_depth_image(app, VK_FORMAT_D32_SFLOAT);
    create_object_picking_depth_image(app, VK_FORMAT_D32_SFLOAT);
    create_object_picking_staging_buffer(app);
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
    if (mouse_button != MOUSE_BUTTON_LEFT) {
        return;
    }
  app->is_mouse_any_button_down = true;
  app->mouse_pos_down[0] = x;
  app->mouse_pos_down[1] = y;
  if (app->gizmo.action & GIZMO_ACTION_TRANSLATE) {
    const Ray ray = ray_from_screen(glm::vec2(x, y), app->vk_context->swapchain_extent, app->camera.position, app->camera.view_matrix, app->projection_matrix);
    const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));
    glm::vec4 n(0.0f);
    if (app->gizmo.active_axis & GIZMO_AXIS_X) {
      n = glm::normalize(model_matrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
    } else if (app->gizmo.active_axis & GIZMO_AXIS_Y) {
      n = glm::normalize(model_matrix * glm::vec4(camera_backward_dir(app->camera), 0.0f));
    } else if (app->gizmo.active_axis & GIZMO_AXIS_Z) {
      n = glm::normalize(model_matrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    } else {
      ASSERT(false);
    }
    app->gizmo.intersection_plane = create_plane(app->gizmo.transform.position, glm::vec3(n));
    app->gizmo.intersection_plane_back = create_plane(app->gizmo.transform.position, glm::vec3(n * -1.0f));
    if (bool hit = raycast_plane(ray, app->gizmo.intersection_plane, app->gizmo.intersection_position); !hit) {
      hit = raycast_plane(ray, app->gizmo.intersection_plane_back, app->gizmo.intersection_position);
      ASSERT(hit);
    }
  }
}

void gizmo_check_ray(App *app, const Ray *ray) {
  const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));

  Ray ray_in_model_space{};
  transform_ray_to_model_space(ray, model_matrix, &ray_in_model_space);

  app->gizmo.action = GIZMO_ACTION_TRANSLATE;
  app->gizmo.active_axis = GIZMO_AXIS_NONE;

  {
    float min_distance = 0.04f;
    const Axis axes[3] = {
        {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length, 'x'},
        {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length, 'y'},
        {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), app->gizmo.config.axis_length + app->gizmo.config.arrow_length, 'z'},
    };
    for (size_t i = 0; i < 3; ++i) {
      float t, s;
      if (const float d = ray_axis_shortest_distance(ray_in_model_space, axes[i], t, s); d < min_distance) {
        min_distance = d;
        app->gizmo.active_axis = (GizmoAxis) (i + 1);
      }
    }
    if (app->gizmo.action != GIZMO_ACTION_NONE) {
      log_debug("op: %d", app->gizmo.action);
      return;
    }
  }

  {
    float min_distance = 0.04f;
    const StrokeCircle circles[3] = {
        {glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), app->gizmo.config.ring_major_radius, 'x'},
        {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), app->gizmo.config.ring_major_radius, 'y'},
        {glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), app->gizmo.config.ring_major_radius, 'z'},
    };
    for (size_t i = 0; i < 3; ++i) {
      // 根据射线与平面的夹角，动态计算触发距离
      const float theta = glm::acos(glm::dot(glm::normalize(circles[i].normal), glm::normalize(-ray_in_model_space.direction)));
      float padding;
      if (glm::degrees(theta) < 90.0f) {
        padding = glm::max(glm::tan(theta) * 0.04f, 0.04f);
      } else if (glm::degrees(theta) > 90.0f) {
        const float beta = glm::radians(90.0f - (glm::degrees(theta) - 90.0f));
        padding = glm::min(glm::max(glm::tan(beta) * 0.04f, 0.04f), 0.06f);
      } else {
        app->gizmo.active_axis = (GizmoAxis) (i + 1);
        break;
      }
      if (const float d = ray_ring_shortest_distance(ray_in_model_space, circles[i].center, circles[i].normal, app->gizmo.config.ring_major_radius - padding, app->gizmo.config.ring_major_radius + padding); d < min_distance) {
        min_distance = d;
        app->gizmo.active_axis = (GizmoAxis) (i + 1);
      }
    }
    if (app->gizmo.action != GIZMO_ACTION_NONE) {
      log_debug("op: %d", app->gizmo.action);
      return;
    }
  }

  {
    // todo scale
  }
}

void app_mouse_button_up(App *app, MouseButton mouse_button, float x, float y) {
  // log_debug("mouse button %d up at screen position (%f, %f)", mouse_button, x, y);
  if (mouse_button != MOUSE_BUTTON_LEFT) {
    return;
  }
  {
    const Ray ray = ray_from_screen(glm::vec2(x, y), app->vk_context->swapchain_extent, app->camera.position, app->camera.view_matrix, app->projection_matrix);
    gizmo_check_ray(app, &ray);

    Vertex vertices[2];
    memcpy(vertices[0].position, glm::value_ptr(ray.origin), sizeof(float) * 3);
    memcpy(vertices[1].position, glm::value_ptr(ray.origin + ray.direction * 100.0f), sizeof(float) * 3);
    AABB aabb{};
    generate_aabb_from_vertices(vertices, sizeof(vertices) / sizeof(Vertex), &aabb);
    Geometry geometry{};
    create_geometry(&app->mesh_system_state, app->vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), nullptr, 0, 0, aabb, &geometry);
    app->line_geometries.push_back(geometry);
  }

  reset_plane(app->gizmo.intersection_plane_back);
  reset_plane(app->gizmo.intersection_plane);
  memset(app->mouse_pos_down, -1.0f, sizeof(float) * 2);
  app->is_mouse_any_button_down = false;
}

void set_geometry_world_position(Geometry &geometry, const glm::vec3 &world_position) {
  geometry.transform.position = world_position;
}

void app_mouse_move(App *app, float x, float y) {
  // log_debug("mouse moving %f, %f", x, y);
  app->mouse_pos[0] = x;
  app->mouse_pos[1] = y;
  const Ray ray = ray_from_screen(glm::vec2(x, y), app->vk_context->swapchain_extent, app->camera.position, app->camera.view_matrix, app->projection_matrix);
  if (app->is_mouse_any_button_down) {
    if (app->gizmo.action & GIZMO_ACTION_TRANSLATE) {
      const glm::mat4 model_matrix = model_matrix_from_transform(gizmo_get_transform(&app->gizmo));
      glm::vec3 intersection_position(0.0f);
      if (bool is_hit = raycast_plane(ray, app->gizmo.intersection_plane, intersection_position); !is_hit) {
        is_hit = raycast_plane(ray, app->gizmo.intersection_plane_back, intersection_position);
        ASSERT(is_hit);
      }
      if (app->gizmo.active_axis & GIZMO_AXIS_X) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      if (app->gizmo.active_axis & GIZMO_AXIS_Y) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      if (app->gizmo.active_axis & GIZMO_AXIS_Z) {
        const glm::vec3 v = intersection_position - app->gizmo.intersection_position;
        const glm::vec3 d = glm::normalize(glm::vec3(model_matrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
        const glm::vec3 translate = glm::dot(d, v) * d;
        app->gizmo.transform.position += translate;
      }
      app->gizmo.intersection_position = intersection_position;

      if (app->selected_mesh_id > 0) {
        for (Geometry &geometry : app->lit_geometries) {
          for (Mesh &mesh : geometry.meshes) {
            if (mesh.id == app->selected_mesh_id) {
              set_geometry_world_position(geometry, app->gizmo.transform.position);
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
