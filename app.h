#pragma once

#include "camera.h"
#include "mesh_loader.h"
#include <cstdint>
#include <volk.h>
#include <vk_mem_alloc.h>

struct VkContext;
struct SDL_Window;
struct ImGuiContext;

struct App {
    SDL_Window *window;
    VkContext *vk_context;
    uint64_t frame_number;
    uint32_t frame_index;

    VkImage color_image;
    VmaAllocation color_image_allocation;
    VkImageView color_image_view;

    VkImage depth_image;
    VmaAllocation depth_image_allocation;
    VkImageView depth_image_view;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;

    VkPipelineLayout compute_pipeline_layout;
    VkPipeline compute_pipeline;

    VkPipelineLayout mesh_pipeline_layout;
    VkPipeline mesh_pipeline;

    Geometry geometry;
    MeshBuffer mesh_buffer;

    Camera camera;

    ImGuiContext *gui_context;

    bool is_resizing;
};

void app_create(SDL_Window *window, App **app);

void app_destroy(App *app);

void app_update(App *app);

void app_resize(App *app, uint32_t width, uint32_t height);

void app_key_up(App *app, uint32_t key);

void app_key_down(App *app, uint32_t key);
