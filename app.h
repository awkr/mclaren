#pragma once

#include "camera.h"
#include "geometry.h"
#include "gizmo.h"
#include "input_system.h"
#include "vk_pipeline.h"
#include <cstdint>
#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;
struct SDL_Window;
struct ImGuiContext;
struct Image;
struct DescriptorAllocator;

#define FRAMES_IN_FLIGHT 2

struct RenderFrame {
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkSemaphore image_acquired_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    DescriptorAllocator *descriptor_allocator;

    Buffer *global_state_buffer;
};

struct GlobalState {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::vec3 sunlight_dir; // in world space
    // glm::vec4 sunlight_color; // sunlight color and intensity ( power )
};

struct InstanceState {
    glm::mat4 model_matrix;
    glm::vec3 color;
    VkDeviceAddress vertex_buffer_device_address;
};

struct App {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_number;
    uint32_t frame_index;

    RenderFrame frames[FRAMES_IN_FLIGHT];

    Image *color_image;
    VkImageView color_image_view;

    Image *depth_image;
    VkImageView depth_image_view;

    VkDescriptorSetLayout single_storage_image_descriptor_set_layout;
    VkDescriptorSetLayout single_combined_image_sampler_descriptor_set_layout;
    VkDescriptorSetLayout global_state_descriptor_set_layout; // single uniform buffer used for global state, such as view matrix, projection matrix, etc.

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    VkPipelineLayout lit_pipeline_layout;
    VkPipeline lit_pipeline;

    VkPipelineLayout wireframe_pipeline_layout;
    VkPipeline wireframe_pipeline;

    Image *checkerboard_image;
    VkImageView checkerboard_image_view;
    Image *uv_debug_image;
    VkImageView uv_debug_image_view;
    VkSampler default_sampler_nearest;

    std::vector<Geometry> lit_geometries;
    std::vector<Geometry> wireframe_geometries;
    std::vector<Geometry> line_geometries;

    Camera camera;
    glm::mat4 projection_matrix;
    GlobalState global_state;

    ImGuiContext *gui_context;

    VkPipelineLayout gizmo_triangle_pipeline_layout;
    VkPipeline gizmo_triangle_pipeline;

    VkPipelineLayout gizmo_line_pipeline_layout;
    VkPipeline gizmo_line_pipeline;

    Gizmo gizmo;
    // one geometry, contains 2 meshes, primitves under each mesh will be rendered with different pipelines
    Geometry gizmo_line_geometry; // the body of the axes
    Geometry gizmo_cone_geometry; // the head of the axes
    Geometry gizmo_stroke_circle_geometry;
    Geometry gizmo_cube_geometry;

    Geometry *selected_geometry = nullptr;
};

void app_create(SDL_Window *window, App **out_app);

void app_destroy(App *app);

void app_update(App *app);

void app_resize(App *app, uint32_t width, uint32_t height);

void app_key_down(App *app, Key key);
void app_key_up(App *app, Key key);

void app_mouse_button_down(App *app, MouseButton mouse_button, float x, float y);
void app_mouse_button_up(App *app, MouseButton mouse_button, float x, float y);
void app_mouse_move(App *app, float x, float y);

void app_capture(App *app);
