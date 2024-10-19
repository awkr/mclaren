#include "app.h"
#include "core/deletion_queue.h"
#include "core/logging.h"
#include "mesh_loader.h"
#include "vk.h"
#include "vk_context.h"
#include "vk_command_buffer.h"
#include "vk_command_pool.h"
#include "vk_image.h"
#include "vk_fence.h"
#include "vk_semaphore.h"
#include "vk_queue.h"
#include "vk_image.h"
#include "vk_image_view.h"
#include "vk_descriptor.h"
#include "vk_descriptor_allocator.h"
#include "vk_pipeline.h"
#include "vk_sampler.h"
#include "vk_swapchain.h"
#include "vk_buffer.h"
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
glm::vec3 screen_position_to_world_position(const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix, float viewport_size_width, float viewport_size_height, float z_ndc, float screen_pos_x, float screen_pos_y) {
    // return glm::unProject(glm::vec3(screen_pos_x, viewport_size_height - screen_pos_y, z_ndc), view_matrix, projection_matrix, glm::vec4(0, 0, viewport_size_width, viewport_size_height));
    const float x_ndc = (2.0f * screen_pos_x) / viewport_size_width - 1.0f;
    const float y_ndc = (2.0f * (viewport_size_height - screen_pos_y)) / viewport_size_height - 1.0f;
    glm::vec4 ndc_pos(x_ndc, y_ndc, z_ndc, 1.0f);
    glm::vec4 world_pos = glm::inverse(projection_matrix * view_matrix) * ndc_pos;
    world_pos /= world_pos.w;
    return glm::vec3(world_pos);
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

void app_create(SDL_Window *window, App **out_app) {
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    App *app = new App();
    app->window = window;
    app->vk_context = new VkContext();

    vk_init(app->vk_context, window, width, height);

    VkContext *vk_context = app->vk_context;

    ASSERT(FRAMES_IN_FLIGHT <= vk_context->swapchain_image_count);

    for (uint8_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        RenderFrame *frame = &app->frames[i];
        vk_create_command_pool(vk_context->device, vk_context->graphics_queue_family_index, &frame->command_pool);
        vk_alloc_command_buffers(vk_context->device, app->frames[i].command_pool, 1, &frame->command_buffer);

        vk_create_fence(vk_context->device, true, &frame->in_flight_fence);
        vk_create_semaphore(vk_context->device, &frame->image_acquired_semaphore);
        vk_create_semaphore(vk_context->device, &frame->render_finished_semaphore);

        uint32_t max_sets = 4; // 计算着色器一个 set，mesh pipeline 中 shader 两个 set，wireframe pipeline 一个 set
        std::vector<DescriptorPoolSizeRatio> size_ratios;
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2});
        size_ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
        vk_descriptor_allocator_create(vk_context->device, max_sets, size_ratios, &frame->descriptor_allocator);

        vk_create_buffer(vk_context, sizeof(GlobalState), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &frame->global_state_buffer);
    }

    VkFormat color_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    create_color_image(app, color_image_format);

    VkFormat depth_image_format = VK_FORMAT_D32_SFLOAT;
    create_depth_image(app, depth_image_format);

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

    {// create compute pipeline
        VkShaderModule compute_shader_module;
        vk_create_shader_module(vk_context->device, "shaders/gradient.comp.spv", &compute_shader_module);

        vk_create_pipeline_layout(vk_context->device, 1, &app->single_storage_image_descriptor_set_layout, nullptr, &app->compute_pipeline_layout);
        vk_create_compute_pipeline(vk_context->device, app->compute_pipeline_layout, compute_shader_module, &app->compute_pipeline);

        vk_destroy_shader_module(vk_context->device, compute_shader_module);
    }

    { // create mesh pipeline
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/mesh.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/mesh.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(InstanceState);

        VkDescriptorSetLayout descriptor_set_layouts[2];
        descriptor_set_layouts[0] = app->global_state_descriptor_set_layout;
        descriptor_set_layouts[1] = app->single_combined_image_sampler_descriptor_set_layout;
        vk_create_pipeline_layout(vk_context->device, 2, descriptor_set_layouts, &push_constant_range, &app->mesh_pipeline_layout);
        app->mesh_pipeline_primitive_topologies.resize(1);
        app->mesh_pipeline_primitive_topologies[0] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        vk_create_graphics_pipeline(vk_context->device, app->mesh_pipeline_layout, color_image_format, true, true, true, depth_image_format,
                                    {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, app->mesh_pipeline_primitive_topologies, VK_POLYGON_MODE_FILL, &app->mesh_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    { // create wireframe pipeline
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/wireframe.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/wireframe.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(InstanceState);

        VkDescriptorSetLayout descriptor_set_layouts[1];
        descriptor_set_layouts[0] = app->global_state_descriptor_set_layout;
        vk_create_pipeline_layout(vk_context->device, 1, descriptor_set_layouts, &push_constant_range, &app->wireframe_pipeline_layout);
        app->wireframe_pipeline_primitive_topologies.resize(1);
        app->wireframe_pipeline_primitive_topologies[0] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        vk_create_graphics_pipeline(vk_context->device, app->wireframe_pipeline_layout, color_image_format, true, false, false, depth_image_format,
                                    {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}}, app->wireframe_pipeline_primitive_topologies, VK_POLYGON_MODE_LINE, &app->wireframe_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);
    }

    { // create gizmo pipeline
        VkShaderModule vert_shader, frag_shader;
        vk_create_shader_module(vk_context->device, "shaders/bounding-box.vert.spv", &vert_shader);
        vk_create_shader_module(vk_context->device, "shaders/bounding-box.frag.spv", &frag_shader);

        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.size = sizeof(InstanceState);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        descriptor_set_layouts.push_back(app->global_state_descriptor_set_layout);
        vk_create_pipeline_layout(vk_context->device, descriptor_set_layouts.size(), descriptor_set_layouts.data(), &push_constant_range, &app->gizmo_pipeline_layout);
        app->gizmo_pipeline_primitive_topologies.resize(2);
        app->gizmo_pipeline_primitive_topologies[0] = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        app->gizmo_pipeline_primitive_topologies[1] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        vk_create_graphics_pipeline(vk_context->device, app->gizmo_pipeline_layout, color_image_format, true, false, false, depth_image_format, {{VK_SHADER_STAGE_VERTEX_BIT, vert_shader}, {VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader}},
                                    app->gizmo_pipeline_primitive_topologies, VK_POLYGON_MODE_LINE, &app->gizmo_pipeline);

        vk_destroy_shader_module(vk_context->device, frag_shader);
        vk_destroy_shader_module(vk_context->device, vert_shader);

        // todo create gizmo geometries
        // [x] three axis
        // [x] three spheres
        // [ ] three cones
        // [ ] three circles
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

    // create ui
    // (*app)->gui_context = ImGui::CreateContext();

    // load_gltf(app->vk_context, "models/cube.gltf", &app->gltf_model_geometry);
    load_gltf(app->vk_context, "models/chinese-dragon.gltf", &app->gltf_model_geometry);
    // load_gltf(app->vk_context, "models/Fox.glb", &app->gltf_model_geometry);
    // load_gltf(app->vk_context, "models/suzanne/scene.gltf", &app->gltf_model_geometry);

    create_plane_geometry(vk_context, 1.5f, 1.0f, &app->plane_geometry);
    app->plane_geometry.position = glm::vec3(0.0f, 0.0f, 2.0f);

    create_cube_geometry(vk_context, &app->cube_geometry);

    create_uv_sphere_geometry(vk_context, 1, 16, 16, &app->uv_sphere_geometry);
    app->uv_sphere_geometry.position = glm::vec3(0.0f, 0.0f, -5.0f);

    { // create a bounding box geometry
        app->bounding_box.min = glm::vec3(-0.5f, -0.5f, -0.5f);
        app->bounding_box.max = glm::vec3(0.5f, 0.5f, 0.5f);

        std::vector<ColoredVertex> vertices;
        vertices.resize(24); // 12 lines, 24 vertices
        // clang-format off
        { // front
            // top line
            vertices[0].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[0].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[1].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[1].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            // right line
            vertices[2].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[2].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[3].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[3].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            // bottom line
            vertices[4].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[4].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[5].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[5].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            // left line
            vertices[6].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[6].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[7].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[7].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        { // back
            // top line
            vertices[8].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[8].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            vertices[9].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[9].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            // right line
            vertices[10].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[10].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            vertices[11].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[11].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            // bottom line
            vertices[12].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[12].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            vertices[13].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[13].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            // left line
            vertices[14].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[14].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
            vertices[15].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[15].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        { // top
            // left line
            vertices[16].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[16].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[17].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[17].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            // right line
            vertices[18].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.max.z);
            vertices[18].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[19].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.max.y, app->bounding_box.min.z);
            vertices[19].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        { // bottom
            // left line
            vertices[20].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[20].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[21].position = glm::vec3(app->bounding_box.min.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[21].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            // right line
            vertices[22].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.max.z);
            vertices[22].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertices[23].position = glm::vec3(app->bounding_box.max.x, app->bounding_box.min.y, app->bounding_box.min.z);
            vertices[23].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        // clang-format on
        create_geometry(vk_context, vertices.data(), vertices.size(), sizeof(ColoredVertex), nullptr, 0, 0, &app->bounding_box_geometry);
        app->bounding_box_geometry.position = glm::vec3(-2.0f, 0.0f, 0.0f);
    }

    create_axis_geometry(vk_context, 1.0f, &app->translation_gizmo_geometry);
    app->translation_gizmo_geometry.position = glm::vec3(0.0f, 2.0f, 0.0f);

    create_camera(&app->camera, glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));

    app->frame_number = 0;

    *out_app = app;
}

void app_destroy(App *app) {
    vk_wait_idle(app->vk_context);

    destroy_camera(&app->camera);

    for (auto &geometry : app->geometries) { destroy_geometry(app->vk_context, &geometry); }
    destroy_geometry(app->vk_context, &app->translation_gizmo_geometry);
    destroy_geometry(app->vk_context, &app->bounding_box_geometry);
    destroy_geometry(app->vk_context, &app->uv_sphere_geometry);
    destroy_geometry(app->vk_context, &app->cube_geometry);
    destroy_geometry(app->vk_context, &app->plane_geometry);
    destroy_geometry(app->vk_context, &app->gltf_model_geometry);

    vk_destroy_sampler(app->vk_context->device, app->default_sampler_nearest);
    vk_destroy_image_view(app->vk_context->device, app->uv_debug_image_view);
    vk_destroy_image(app->vk_context, app->uv_debug_image);
    vk_destroy_image_view(app->vk_context->device, app->checkerboard_image_view);
    vk_destroy_image(app->vk_context, app->checkerboard_image);

    // ImGui::DestroyContext(app->gui_context);
    vk_destroy_pipeline(app->vk_context->device, app->gizmo_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->gizmo_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->wireframe_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->wireframe_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->mesh_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->mesh_pipeline_layout);

    vk_destroy_pipeline(app->vk_context->device, app->compute_pipeline);
    vk_destroy_pipeline_layout(app->vk_context->device, app->compute_pipeline_layout);

    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_combined_image_sampler_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->global_state_descriptor_set_layout);
    vk_destroy_descriptor_set_layout(app->vk_context->device, app->single_storage_image_descriptor_set_layout);

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

void draw_world(const App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline);

    vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
    vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
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
    vk_cmd_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());

    // for (const Mesh &mesh: app->gltf_model_geometry.meshes) {
    //     glm::mat4 model = glm::mat4(1.0f);
    //     model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f)); // todo use model matrix from mesh itself
    //
    //     InstanceState instance_state{};
    //     instance_state.model = model;
    //     instance_state.vertex_buffer_device_address = mesh.mesh_buffer.vertex_buffer_device_address;
    //
    //     vk_command_push_constants(command_buffer, app->mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
    //
    //     for (const Primitive &primitive: mesh.primitives) {
    //         vk_command_bind_index_buffer(command_buffer, mesh.mesh_buffer.index_buffer.handle, primitive.index_offset);
    //         vk_command_draw_indexed(command_buffer, primitive.index_count);
    //     }
    // }

    std::vector<Geometry> geometries;
    geometries.push_back(app->uv_sphere_geometry);
    geometries.push_back(app->cube_geometry);
    geometries.push_back(app->plane_geometry);

    for (const Geometry &geometry : geometries) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, geometry.position);

        for (const Mesh &mesh : geometry.meshes) {
            InstanceState instance_state{};
            instance_state.model = model;
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

            for (const Primitive &primitive : mesh.primitives) {
                vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
                vk_cmd_draw_indexed(command_buffer, primitive.index_count);
            }
        }
    }

    // draw wireframe on selected entity
    if (static uint32_t n = 0; n++ % 1000 < 800) {
        vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline);

        // for (const Mesh &mesh : app->gltf_model_geometry.meshes) {
        //     vk_command_set_viewport(command_buffer, 0, 0, extent->width, extent->height);
        //     vk_command_set_scissor(command_buffer, 0, 0, extent->width, extent->height);
        //
        //     std::vector<VkDescriptorSet> descriptor_sets; // todo 提前预留空间，防止 resize 导致被其他地方引用的原有元素失效
        //     std::deque<VkDescriptorBufferInfo> buffer_infos;
        //     std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        //     {
        //         vk_copy_data_to_buffer(app->vk_context, &frame->global_state_buffer, &app->global_state, sizeof(GlobalState));
        //
        //         VkDescriptorSet descriptor_set;
        //         vk_descriptor_allocator_alloc(app->vk_context->device, frame->descriptor_allocator, app->global_state_descriptor_set_layout, &descriptor_set);
        //         descriptor_sets.push_back(descriptor_set);
        //
        //         VkDescriptorBufferInfo descriptor_buffer_info = vk_descriptor_buffer_info(frame->global_state_buffer.handle, sizeof(GlobalState));
        //         buffer_infos.push_back(descriptor_buffer_info);
        //
        //         VkWriteDescriptorSet write_descriptor_set = vk_write_descriptor_set(descriptor_sets.back(), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos.back());
        //         write_descriptor_sets.push_back(write_descriptor_set);
        //     }
        //     vk_update_descriptor_sets(app->vk_context->device, write_descriptor_sets.size(), write_descriptor_sets.data());
        //     vk_command_bind_descriptor_sets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->wireframe_pipeline_layout, descriptor_sets.size(), descriptor_sets.data());
        //
        //     glm::mat4 model = glm::mat4(1.0f);
        //     model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f)); // todo use model matrix from mesh itself
        //
        //     InstanceState instance_state{};
        //     instance_state.model = model;
        //     instance_state.vertex_buffer_device_address = mesh.mesh_buffer.vertex_buffer_device_address;
        //
        //     vk_command_push_constants(command_buffer, app->wireframe_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        //
        //     for (const Primitive &primitive : mesh.primitives) {
        //         vk_command_bind_index_buffer(command_buffer, mesh.mesh_buffer.index_buffer.handle, primitive.index_offset);
        //         vk_command_draw_indexed(command_buffer, primitive.index_count);
        //     }
        // }

        for (const Mesh &mesh : app->uv_sphere_geometry.meshes) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, app->uv_sphere_geometry.position);

            InstanceState instance_state{};
            instance_state.model = model;
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->wireframe_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);

            for (const Primitive &primitive: mesh.primitives) {
                vk_cmd_bind_index_buffer(command_buffer, mesh.index_buffer->handle, primitive.index_offset);
                vk_cmd_draw_indexed(command_buffer, primitive.index_count);
            }
        }
    }
}

void draw_gizmo(const App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {
    vk_cmd_bind_pipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gizmo_pipeline);

    vk_cmd_set_viewport(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
    vk_cmd_set_scissor(command_buffer, 0, 0, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height);
    vk_cmd_set_primitive_topology(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

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

    for (const Mesh &mesh : app->bounding_box_geometry.meshes) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, app->bounding_box_geometry.position);

        InstanceState instance_state{};
        instance_state.model = model;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        vk_cmd_bind_vertex_buffer(command_buffer, mesh.vertex_buffer->handle, 0);
        for (const Primitive &primitive: mesh.primitives) {
            vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
        }
    }

    for (const Mesh &mesh : app->translation_gizmo_geometry.meshes) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, app->translation_gizmo_geometry.position);
        {
            const float factor = 0.5f;
            const float scale = glm::length(app->camera.position - app->translation_gizmo_geometry.position) * factor;
            model = glm::scale(model, glm::vec3(scale));
        }
        // {
        //     float fov_y = 45.0f;
        //     float dist = glm::length(app->camera.position - position);
        //     float scale = ((2.0f * tan(fov_y * 0.5f)) * dist) * 0.5f;
        //     model = glm::scale(model, glm::vec3(scale));
        // }

        InstanceState instance_state{};
        instance_state.model = model;
        instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

        vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
        vk_cmd_bind_vertex_buffer(command_buffer, mesh.vertex_buffer->handle, 0);
        for (const Primitive &primitive: mesh.primitives) {
            vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
        }
    }

    for (const auto &geometry : app->geometries) {
        for (const Mesh &mesh : geometry.meshes) {
            glm::mat4 model = glm::mat4(1.0f);

            InstanceState instance_state{};
            instance_state.model = model;
            instance_state.vertex_buffer_device_address = mesh.vertex_buffer_device_address;

            vk_cmd_push_constants(command_buffer, app->gizmo_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(InstanceState), &instance_state);
            vk_cmd_bind_vertex_buffer(command_buffer, mesh.vertex_buffer->handle, 0);
            for (const Primitive &primitive : mesh.primitives) {
                vk_cmd_draw(command_buffer, primitive.vertex_count, 1, 0, 0);
            }
        }
    }
}

void draw_gui(const App *app, VkCommandBuffer command_buffer, const RenderFrame *frame) {}

void update_scene(App *app) {
    camera_update(&app->camera);

    app->global_state.view_matrix = app->camera.view_matrix;

    glm::mat4 projection_matrix = glm::mat4(1.0f);
    float fov_y = 45.0f;
    float z_near = 0.05f, z_far = 100.0f;
    projection_matrix = glm::perspective(glm::radians(fov_y), (float) app->vk_context->swapchain_extent.width / (float) app->vk_context->swapchain_extent.height, z_near, z_far);
    app->projection_matrix = projection_matrix;

    app->global_state.projection_matrix = clip * projection_matrix;

    app->global_state.sunlight_dir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
}

bool begin_frame(App *app) { return false; }

void end_frame(App *app) {}

void app_update(App *app) {
    update_scene(app);

    app->frame_index = app->frame_number % FRAMES_IN_FLIGHT;

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

        vk_cmd_begin_rendering(command_buffer, &app->vk_context->swapchain_extent, &color_attachment, 1, &depth_attachment);
        draw_world(app, command_buffer, frame);
        draw_gizmo(app, command_buffer, frame);
        draw_gui(app, command_buffer, frame);
        vk_cmd_end_rendering(command_buffer);

        vk_transition_image_layout(command_buffer, app->color_image->image, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vk_cmd_blit_image(command_buffer, app->color_image->image, swapchain_image, &app->vk_context->swapchain_extent);

        vk_transition_image_layout(command_buffer, swapchain_image, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vk_end_command_buffer(command_buffer);
    }

    VkSemaphoreSubmitInfo wait_semaphore = vk_semaphore_submit_info(frame->image_acquired_semaphore,
                                                                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    VkSemaphoreSubmitInfo signal_semaphore = vk_semaphore_submit_info(frame->render_finished_semaphore,
                                                                      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkCommandBufferSubmitInfo command_buffer_submit_info = vk_command_buffer_submit_info(command_buffer);
    VkSubmitInfo2 submit_info = vk_submit_info(&command_buffer_submit_info, &wait_semaphore, &signal_semaphore);
    vk_queue_submit(app->vk_context->graphics_queue, &submit_info, frame->in_flight_fence);

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

    vk_destroy_image_view(app->vk_context->device, app->depth_image_view);
    vk_destroy_image(app->vk_context, app->depth_image);

    vk_destroy_image_view(app->vk_context->device, app->color_image_view);
    vk_destroy_image(app->vk_context, app->color_image);

    create_color_image(app, VK_FORMAT_R16G16B16A16_SFLOAT);
    create_depth_image(app, VK_FORMAT_D32_SFLOAT);
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
}

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

void app_mouse_button_up(App *app, MouseButton mouse_button, float x, float y) {
    log_debug("mouse button %d up at screen position %f %f", mouse_button, x, y);
    if (mouse_button != MOUSE_BUTTON_LEFT) {
        return;
    }
    {
        // fire a ray from camera position to the world position of the mouse cursor
        const glm::vec3 near_pos = screen_position_to_world_position(app->projection_matrix, app->camera.view_matrix, app->vk_context->swapchain_extent.width, app->vk_context->swapchain_extent.height, 0.0f, x, y);

        Ray ray;
        ray.origin = camera_get_position(&app->camera);
        ray.direction = glm::normalize(near_pos - app->camera.position);

        ColoredVertex vertices[2];
        vertices[0].position = ray.origin;
        vertices[0].color = glm::vec4(1, 0, 0, 1);
        vertices[1].position = ray.origin + ray.direction * 100.0f;
        vertices[1].color = glm::vec4(0, 1, 0, 1);
        Geometry geometry;
        create_geometry(app->vk_context, vertices, sizeof(vertices) / sizeof(ColoredVertex), sizeof(ColoredVertex), nullptr, 0, 0, &geometry);
        app->geometries.push_back(geometry);

        {
            // loop through all geometries to find the cloest intersection point
        }
    }
}

void app_mouse_move(App *app, float x, float y) {}

void app_capture(App *app) {}
