#pragma once

#include "camera.h"
#include "mesh_loader.h"
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

    Buffer global_state_buffer;
};

struct GlobalState {
    glm::mat4 view;
    glm::mat4 projection;
};

struct InstanceState {
    glm::mat4 model;
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
    VkDescriptorSetLayout global_state_descriptor_set_layout;

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    VkPipelineLayout mesh_pipeline_layout;
    VkPipeline mesh_pipeline;

    Image *default_gray_image;
    Image *default_checkerboard_image;
    VkImageView default_checkerboard_image_view;
    VkSampler default_sampler_nearest;

    Geometry geometry;
    MeshBuffer mesh_buffer;

    Camera camera;
    GlobalState global_state;

    ImGuiContext *gui_context;
};

void app_create(SDL_Window *window, App **out_app);

void app_destroy(App *app);

void app_update(App *app);

void app_resize(App *app, uint32_t width, uint32_t height);

void app_key_up(App *app, uint32_t key);

void app_key_down(App *app, uint32_t key);
