#pragma once

#include "camera.h"
#include "geometry.h"
#include "gizmo.h"
#include "input_system.h"
#include "vk_descriptor_allocator.h"
#include "vk_pipeline.h"
#include <cstdint>
#include <queue>
#include <vk_mem_alloc.h>
#include <volk.h>

struct VkContext;
struct SDL_Window;
struct ImGuiContext;
struct Image;

#define FRAMES_IN_FLIGHT 2

struct RenderFrame {
    VkCommandPool command_pool;

    VkFence in_flight_fence;

    DescriptorAllocator descriptor_allocator;

    Buffer *global_uniform_buffer;
    VkDescriptorSet global_uniform_buffer_descriptor_set;
};

struct GlobalState {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::vec3 sunlight_dir; // in world space
    // glm::vec4 sunlight_color; // sunlight color and intensity ( power )
};

// 推荐不超过 128 bytes
struct InstanceState {
    glm::mat4 model_matrix;
    glm::vec4 color;
    VkDeviceAddress vertex_buffer_device_address;
};

struct ObjectPickingInstanceState {
    glm::mat4 model_matrix;
    uint32_t id;
    VkDeviceAddress vertex_buffer_device_address;
};

struct App {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_count;

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> present_complete_semaphores;
    std::vector<VkSemaphore> render_complete_semaphores;
    std::queue<VkSemaphore> semaphore_pool;

    RenderFrame frames[FRAMES_IN_FLIGHT];

    Image *color_image;
    VkImageView color_image_view;

    Image *depth_image;
    VkImageView depth_image_view;

    VkDescriptorSetLayout single_storage_image_descriptor_set_layout;
    VkDescriptorSetLayout single_combined_image_sampler_descriptor_set_layout;
    VkDescriptorSetLayout global_uniform_buffer_descriptor_set_layout; // single uniform buffer used for global state, such as view matrix, projection matrix, etc.

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
    VkSampler sampler_nearest;

    MeshSystemState mesh_system_state;

    std::vector<Geometry> lit_geometries;
    std::vector<Geometry> wireframe_geometries;
    std::vector<Geometry> line_geometries;

    Camera camera;
    glm::mat4 projection_matrix;
    GlobalState global_state;

    ImGuiContext *gui_context;

    VkPipelineLayout vertex_lit_pipeline_layout;
    VkPipeline vertex_lit_pipeline;

    VkPipelineLayout line_pipeline_layout;
    VkPipeline line_pipeline;

    VkPipelineLayout object_picking_pipeline_layout;
    VkPipeline object_picking_pipeline;

    Image *object_picking_color_image;
    VkImageView object_picking_color_image_view;

    Image *object_picking_depth_image;
    VkImageView object_picking_depth_image_view;

    Buffer *object_picking_buffer;

    Gizmo gizmo;

    float mouse_pos[2];
    uint32_t selected_mesh_id;
    Transform selected_mesh_transform;
    bool is_mouse_any_button_down;
    float mouse_pos_down[2];
    bool clicked;
};

void app_create(SDL_Window *window, App **out_app);

void app_destroy(App *app);

void app_update(App *app, InputSystemState *input_system_state);

void app_resize(App *app, uint32_t width, uint32_t height);

void app_key_down(App *app, Key key);
void app_key_up(App *app, Key key);

void app_mouse_button_down(App *app, MouseButton mouse_button, float x, float y);
void app_mouse_button_up(App *app, MouseButton mouse_button, float x, float y);
void app_mouse_move(App *app, float x, float y);

void app_capture(App *app);
