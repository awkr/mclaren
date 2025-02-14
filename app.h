#pragma once

#include "camera.h"
#include "geometry.h"
#include "geometry_system.h"
#include "gizmo.h"
#include "input_system.h"
#include "light.h"
#include "mesh_system.h"
#include "vk_descriptor_allocator.h"
#include "vk_pipeline.h"
#include <cstdint>
#include <queue>
#include <volk.h>

struct VkContext;
struct SDL_Window;
struct ImGuiContext;
struct Image;

#define FRAMES_IN_FLIGHT 2
#define MAX_TEXTURE_COUNT 100

struct RenderFrame {
    VkCommandPool command_pool;

    VkFence in_flight_fence;

    DescriptorAllocator descriptor_allocator;

    Buffer *global_state_uniform_buffer;
    VkDescriptorSet global_state_uniform_buffer_descriptor_set;

    Buffer *dir_light_uniform_buffer;
};

struct GlobalState {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

struct UnlitInstanceState {
  glm::mat4 model_matrix;
  glm::vec4 color;
  VkDeviceAddress vertex_buffer_device_address;
};

struct LitInstanceState {
  glm::mat4 model_matrix;
  glm::mat4 normal_matrix;
  glm::vec4 color;
  VkDeviceAddress vertex_buffer_device_address;
  uint32_t texture_index;
};

struct VertexLitInstanceState {
  glm::mat4 model_matrix;
  glm::mat4 normal_matrix;
  glm::vec4 color;
  VkDeviceAddress vertex_buffer_device_address;
};

struct EntityPickingInstanceState {
  glm::mat4 model_matrix;
  uint32_t id;
  VkDeviceAddress vertex_buffer_device_address;
};

struct App {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_count;

    VkRenderPass lit_render_pass;
    VkRenderPass vertex_lit_render_pass;
    VkRenderPass entity_picking_render_pass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> present_complete_semaphores;
    std::vector<VkSemaphore> render_complete_semaphores;
    std::queue<VkSemaphore> semaphore_pool;

    RenderFrame frames[FRAMES_IN_FLIGHT];

    VkImage color_image;
    VkDeviceMemory color_image_device_memory;
    VkImageView color_image_view;

    VkImage depth_image;
    VkDeviceMemory depth_image_device_memory;
    VkImageView depth_image_view;

    VkDescriptorSetLayout single_storage_image_descriptor_set_layout;
    VkDescriptorSetLayout single_storage_buffer_descriptor_set_layout;
    VkDescriptorSetLayout single_combined_image_sampler_descriptor_set_layout;
    VkDescriptorSetLayout global_state_uniform_buffer_descriptor_set_layout; // single uniform buffer used for global state, such as view matrix, projection matrix, etc.
    VkDescriptorSetLayout dir_light_uniform_buffer_descriptor_set_layout; // single uniform buffer used for global state, such as view matrix, projection matrix, etc.
    VkDescriptorSetLayout descriptor_set_layout;

    VkDescriptorPool descriptor_pools[FRAMES_IN_FLIGHT];
    VkDescriptorSet descriptor_sets[FRAMES_IN_FLIGHT];

    // 存放所有的纹理资源
    VkImage images[FRAMES_IN_FLIGHT][MAX_TEXTURE_COUNT];
    VkDeviceMemory image_device_memories[FRAMES_IN_FLIGHT][MAX_TEXTURE_COUNT];
    VkImageView image_views[FRAMES_IN_FLIGHT][MAX_TEXTURE_COUNT];
    VkSampler samplers[FRAMES_IN_FLIGHT][MAX_TEXTURE_COUNT];

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    VkPipelineLayout lit_pipeline_layout;
    VkPipeline lit_pipeline;

    VkPipelineLayout wireframe_pipeline_layout;
    VkPipeline wireframe_pipeline;

    VkImage checkerboard_image;
    VkDeviceMemory checkerboard_image_device_memory;
    VkImageView checkerboard_image_view;
    VkImage uv_debug_image;
    VkDeviceMemory uv_debug_image_device_memory;
    VkImageView uv_debug_image_view;
    VkSampler sampler_nearest;

    MeshSystemState mesh_system_state;
    GeometrySystemState geometry_system_state;

    std::vector<Geometry> lit_geometries;
    std::vector<Geometry> wireframe_geometries;
    std::vector<Geometry> line_geometries;

    Camera camera;
    glm::mat4 projection_matrix;
    GlobalState global_state;
    DirLight dir_light;

    ImGuiContext *gui_context;

    VkPipelineLayout vertex_lit_pipeline_layout;
    VkPipeline vertex_lit_pipeline;

    VkPipelineLayout line_pipeline_layout;
    VkPipeline line_pipeline;

    VkPipelineLayout entity_picking_pipeline_layout;
    VkPipeline entity_picking_pipeline;

    std::vector<VkFramebuffer> entity_picking_framebuffers;
    std::vector<Buffer *> entity_picking_storage_buffers;

    std::vector<VkFramebuffer> gizmo_framebuffers;

    Gizmo gizmo;

    glm::fvec2 mouse_pos;
    uint32_t selected_mesh_id;
    Transform selected_mesh_transform;
    glm::fvec2 mouse_pos_down;
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
